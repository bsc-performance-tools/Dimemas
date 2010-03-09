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
* Esta clase crea la ventana que muestra los resultados/errores de la simulación.
*/
public class ResultsWindow extends GUIWindow
{
  public static final long serialVersionUID = 19L;
  
  private boolean errorWindow;
  private JPanel buttonPanel = new JPanel(new FlowLayout());
  private JTextArea result = new JTextArea();

  private JButton b_save = createButton("Save results");
  private JButton b_close = createButton("Close window");

  private JScrollPane scrollPanel = new JScrollPane(result,
      ScrollPaneConstants.VERTICAL_SCROLLBAR_AS_NEEDED,
      ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED);;

  // Constructor de la clase ResultsWindow.
  public ResultsWindow(Data d, String title)
  {
    super(d);

    // Propiedades de la ventana.
    errorWindow = title.equalsIgnoreCase("Error");
    setTitle(title);
    setSize(425,375);
    windowPanel.setLayout(new BorderLayout());
    result.setEditable(false);
    result.setBackground(Color.lightGray);

    if(errorWindow)
    {
      setLocation(425,150);
    }
    else
    {
      setLocation(0,150);
      buttonPanel.add(b_save);
    }

    // Añadiendo los componentes a la ventana.
    buttonPanel.add(b_close);
    windowPanel.add(BorderLayout.CENTER,scrollPanel);
    windowPanel.add(BorderLayout.SOUTH,buttonPanel);
  }

  // Método que escribe una línea de datos facilitados por el simulador.
  public void writeLine(String line)
  {
    result.append(line);
    result.append("\n");
  }

  // Método que indica si hay datos.
  public boolean empty()
  {
    if(result.getLineCount() == 0)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  // En este método se ejecutan las respuestas del GUI a las acciones del
  // usuario al interactuar con los elementos de la ventana.
  public void actionPerformed(ActionEvent e)
  {
    if(e.getSource() == b_close)
    {
      dispose();
    }
    else if(e.getSource() == b_save)
    {
      File f;
      Tools.fc.addChoosableFileFilter(new Tools.OUTfilter());
      int action = Tools.fc.showSaveDialog(null);

      if(action == JFileChooser.APPROVE_OPTION)
      {
        if(Tools.fc.getSelectedFile().getAbsolutePath().endsWith(".out"))
        {
          f = new File(Tools.fc.getSelectedFile().getAbsolutePath());
        }
        else
        {
          f = new File(Tools.fc.getSelectedFile().getAbsolutePath() + ".out");
        }

        try
        {
          RandomAccessFile raf = new RandomAccessFile(f,"rw");

          if(raf.length() != 0)
          {
            raf.setLength(0);
          }

          raf.writeBytes(result.getText());
          raf.close();
        } catch(Throwable t)
          {
            Tools.showInformationMessage(t.toString());
          }
      }

      Tools.fc.resetChoosableFileFilters();
    }
  }
}
