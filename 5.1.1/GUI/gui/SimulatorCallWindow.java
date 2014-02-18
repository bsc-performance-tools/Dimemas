/*****************************************************************************\
 *                        ANALYSIS PERFORMANCE TOOLS                         *
 *                               Dimemas GUI                                 *
 *                  GUI for the Dimemas simulation tool                      *
 *                                                                           * 
 *****************************************************************************
 *     ___     This library is free software; you can redistribute it and/or *
 *    /  __         modify it under the terms of the GNU LGPL as published   *
 *   /  /  _____    by the Free Software Foundation; either version 2.1      *
 *  /  /  /     \   of the License, or (at your option) any later version.   *
 * (  (  ( B S C )                                                           *
 *  \  \  \_____/   This library is distributed in hope that it will be      *
 *   \  \__         useful but WITHOUT ANY WARRANTY; without even the        *
 *    \___          implied warranty of MERCHANTABILITY or FITNESS FOR A     *
 *                  PARTICULAR PURPOSE. See the GNU LGPL for more details.   *
 *                                                                           *
 * You should have received a copy of the GNU Lesser General Public License  *
 * along with this library; if not, write to the Free Software Foundation,   *
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA          *
 * The GNU LEsser General Public License is contained in the file COPYING.   *
 *                                 ---------                                 *
 *   Barcelona Supercomputing Center - Centro Nacional de Supercomputacion   *
\*****************************************************************************/

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- *\

  $URL::              $:  File
  $Rev::              $:  Revision of last commit
  $Author::           $:  Author of last commit
  $Date::             $:  Date of last commit

\* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

package gui;

/**
 * Título:       dimemas
 * Descripcion:
 * Copyright:    Copyright (c) 2001
 * Empresa:
 * @author Óscar Bardillo Luján
 * @version 1.0
 */

import data.*;
import tools.*;
import javax.swing.*;
import java.io.*;
import java.awt.*;
import java.awt.event.*;

/*
* Esta clase crea la ventana de permitirá seleccionar las opciones para ejecutar
* el simulador DIMEMAS.
*/
public class SimulatorCallWindow extends GUIWindow
{
  public static final long serialVersionUID = 20L;
  
  private final String TEMP_CONFIG_FILE = "conf0000.tmp";

  private JButton b_sim = createButton("Call simulator");
  private JButton b_close = createButton("Close");

  private JTextField tf_break = new JTextField(15);
  private JTextField tf_start = new JTextField(15);
  private JTextField tf_stop = new JTextField(15);
  private JTextField tf_size = new JTextField(15);
  private JTextField tf_filename = new JTextField(15);

  private JComboBox cb_output = createComboBox();

  private JRadioButton ignoreSync_no;
  private JRadioButton ignoreSync_yes;
  private ButtonGroup syncGroup = createGroup('i');

  private JRadioButton load_no;
  private JRadioButton load_yes;
  private ButtonGroup loadGroup = createGroup('l');

  private JRadioButton type_ascii;
  private JRadioButton type_binary;
  private ButtonGroup typeGroup = createGroup('t');

  /*
  * El método createComboBox genera un selector Swing que tiene como opciones
  * los tipos de archivos que genera Dimemas.
  *
  * @ret JComboBox: Selector Swing creado.
  */
  private JComboBox createComboBox()
  {
    JComboBox cb = new JComboBox();

    cb.addItem("None");
    cb.addItem("Paraver");
    cb.addItem("Vampir");
    cb.addActionListener(this);

    return cb;
  }

  // Método que agrupa las opciones para que solo se pueda escoger una a la vez.
  private ButtonGroup createGroup(char option)
  {
    ButtonGroup group = new ButtonGroup();

    switch(option)
    {
      case 'i': ignoreSync_no = new JRadioButton("NO");
                ignoreSync_yes = new JRadioButton("YES");
                group.add(ignoreSync_no);
                group.add(ignoreSync_yes);
                break;

      case 'l': load_no = new JRadioButton("NO");
                load_yes = new JRadioButton("YES");
                group.add(load_no);
                group.add(load_yes);
                break;

      case 't': type_ascii = new JRadioButton("ASCII");
                type_binary = new JRadioButton("Binary");
                group.add(type_ascii);
                group.add(type_binary);
                break;

      default: break;
    }

    return group;
  }

  // Constructor de la clase SimulatorCallWindow.
  public SimulatorCallWindow(Data d)
  {
    super(d);

    // Propiedades de la ventana.
    setTitle("Simulator options");

    // Añadiendo información.
    tf_break.setText(data.simOptions.getSimTime());
    tf_filename.setText(data.simOptions.getFilename());
    tf_start.setText(data.simOptions.getStartTime());
    tf_stop.setText(data.simOptions.getStopTime());
    tf_size.setText(data.simOptions.getMinSize());

    if(data.simOptions.getLoadInMemory())
    {
      load_yes.setSelected(true);
    }
    else
    {
      load_no.setSelected(true);
    }

    if(data.simOptions.getIsTypeAscii())
    {
      type_ascii.setSelected(true);
    }
    else
    {
      type_binary.setSelected(true);
    }

    if(data.simOptions.getIgnoreSend())
    {
      ignoreSync_yes.setSelected(true);
    }
    else
    {
      ignoreSync_no.setSelected(true);
    }

    cb_output.setSelectedIndex(data.simOptions.getOutputFile());

    // Añadiendo los componentes a la ventana.
    drawLine(new Component[] {new JLabel("Load trace into memory"),load_no,load_yes});
    drawLine(new Component[] {new JLabel("Simulation break time [s]"),tf_break});
    drawLine(new Component[] {new JLabel("Ignore synchronous send trace field"),ignoreSync_no,ignoreSync_yes});
    drawLine(new Component[] {new JLabel("Min. message size for Rendezvous [Bytes]"),tf_size});
    drawLine(new Component[] {new JLabel("Output tracefile"),cb_output});
    drawLine(new Component[] {new JLabel("Output tracefile name"),tf_filename});
    drawLine(new Component[] {new JLabel("Output tracefile type"),type_ascii,type_binary});
    drawLine(new Component[] {new JLabel("Output tracefile start time [s]"),tf_start});
    drawLine(new Component[] {new JLabel("Output tracefile stop time [s]"),tf_stop});
    drawButtons(new Component[] {b_sim,b_close},55,5);

    // Más propiedades de ventana.
    setBounds(225,150,getWidth()+30,getHeight());
    pack();
    setVisible(true);
  }

  /*
  * El método dataOK comprueba algunos aspectos que deben cumplir los datos que
  * se muestran en ese momento en pantalla (no puede haber campos vacíos, el
  * valor 0 no esta permitido en algunos casos, etc).
  */
  private boolean dataOK()
  {
    try
    {
      data.simOptions.setSimTime(tf_break.getText());
      data.simOptions.setMinSize(tf_size.getText());
      data.simOptions.setOutputFile(cb_output.getSelectedIndex());

      if(cb_output.getSelectedIndex() != 0)
      {
        if(tf_filename.getText().equalsIgnoreCase(""))
        {
          Tools.showWarningMessage("Output name");
          return false;
        }
        else
        {
          data.simOptions.setFilename(tf_filename.getText());
        }

        data.simOptions.setStartTime(tf_start.getText());
        data.simOptions.setStopTime(tf_stop.getText());

        if(type_ascii.isSelected())
        {
          data.simOptions.setIsTypeAscii(true);
        }
        else
        {
          data.simOptions.setIsTypeAscii(false);
        }
      }

      if(load_yes.isSelected())
      {
        data.simOptions.setLoadInMemory(true);
      }
      else
      {
        data.simOptions.setLoadInMemory(false);
      }

      if(ignoreSync_yes.isSelected())
      {
        data.simOptions.setIgnoreSend(true);
      }
      else
      {
        data.simOptions.setIgnoreSend(false);
      }
    } catch(Exception e)
      {
        return false;
      }

    return true;
  }

  /*
  * Clase que permitirá comunicar al GUI con el simulador Dimemas y de esa forma
  * obtener los resultados de una simulación.
  */
  class Stream extends Thread
  {
    private String type;
    private InputStream in;

    // Constructor de la clase Stream.
    Stream(InputStream stream, String string, Data data)
    {
      in = stream;
      type = string;
    }

    public void run()
    {
      try
      {
        BufferedReader br = new BufferedReader(new InputStreamReader(in));
        ResultsWindow rw = new ResultsWindow(data,type);
        String line = null;

        while((line = br.readLine()) != null)
        {
          rw.writeLine(line);
        }

        if(!rw.empty())
        {
          rw.setVisible(true);
        }
        else
        {
          rw.dispose();
        }
      } catch(IOException e)
        {
          e.printStackTrace();
        }
    }
  }

  // En este método se ejecutan las respuestas del GUI a las acciones del
  // usuario al interactuar con los elementos de la ventana.
  public void actionPerformed(ActionEvent e)
  {
    if(e.getSource() == b_sim)
    {
      if(dataOK())
      {
        b_sim.setEnabled(false);
        File configFile = new File(TEMP_CONFIG_FILE);
        data.saveToDisk(TEMP_CONFIG_FILE);
        String call = data.simOptions.generateCommand(configFile.getAbsolutePath());

        JFrame window = new JFrame("Work in progress");
        Container layout = window.getContentPane();
        layout.setLayout(new BorderLayout());
        layout.add(new JLabel("Please wait."),BorderLayout.CENTER);
        window.setContentPane(layout);
        window.setIconImage(Toolkit.getDefaultToolkit().getImage(Data.ICON_IMAGE));
        window.setResizable(false);
        window.setBounds(375,275,window.getWidth()+80,window.getHeight()+25);
        window.pack();
        window.setVisible(true);

        try
        {
          Runtime rt = Runtime.getRuntime();
          Process proc = rt.exec(call);
          Stream error = new Stream(proc.getErrorStream(),"Error",data);
          Stream result = new Stream(proc.getInputStream(),"Simulation results",data);
          error.start();
          result.start();
          proc.waitFor();
        } catch(Throwable t)
          {
            Tools.showInformationMessage(t.toString());
          }

        configFile.delete();
        window.dispose();
        b_sim.setEnabled(true);
      }
    }
    else if(e.getSource() == b_close)
    {
      if(dataOK())
      {
        dispose();
      }
    }
    else if(e.getSource() == cb_output)
    {
      if(cb_output.getSelectedIndex() == 0)
      {
        type_ascii.setEnabled(false);
        type_binary.setEnabled(false);
        tf_filename.setEditable(false);
        tf_start.setEditable(false);
        tf_stop.setEditable(false);
      }
      else
      {
        type_ascii.setEnabled(true);
        type_binary.setEnabled(true);
        tf_filename.setEditable(true);
        tf_start.setEditable(true);
        tf_stop.setEditable(true);
      }
    }
  }
}