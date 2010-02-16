/*****************************************************************************\
 *                        ANALYSIS PERFORMANCE TOOLS                         *
 *                                  Dimemas                                  *
 *       Simulation tool for the parametric analysis of the behaviour of     *
 *       message-passing applications on a configurable parallel platform    *
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

  $URL::                  $:  File
  $Rev::                  $:  Revision of last commit
  $Author::               $:  Author of last commit
  $Date::                 $:  Date of last commit

\* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

/*
 * Small interface to use the Socket, ClientSocket classes
 * Remember that Socket/ClientSocket have copyright not owned by us!
 *
 */

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string.h>
#include "ClientSocket.h"
#include "SocketException.h"

/* DIMEMAS includes */
extern "C" {
#include "define.h"
#include "types.h"
#include "extern.h"
#include "events.h"
#include "list.h"
#include "mallocame.h"
void panic(char *fmt, ...);
}

#include "venusclient.h"

using namespace std;

static ClientSocket *channelSocket = 0;
int venusmsgs_in_flight = 0;
int venus_account_bytes_sent = 0;
int venus_account_bytes_recv = 0;
int venus_account_nstops = 0; 
int venus_account_nends = 0; 
int venus_enabled = 0;

int vc_initialize(char const *venusconn) {
	int tries = 0;
	
	if ((!venusconn) || (!strcmp("",venusconn))) {
		venusconn = "127.0.0.1:10000";
	}

	char *s = strdup(venusconn), *saveptr;
	char *host = strtok_r(s, ":", &saveptr);
	if (!host) { host = "127.0.0.1"; }
	char *port = strtok_r(NULL, ":", &saveptr);
	if (!port) { port = "10000"; }
	
	while ((!channelSocket) && (tries < 2)) {
		try {	
			channelSocket = new ClientSocket(host, atoi(port));
		}
		catch (SocketException& e)
		{
                        printf("Could not connect to: \"%s\", host: \"%s\", port \"%s\" ==  %d\n", venusconn, host, port, atoi(port));
			std::cout << "Exception was caught:" << e.description() << "\nExiting.\n";
			tries++;
                        sleep(5);
			//return 0;
		}
	}
        if (!channelSocket) {
                return 0;
        }
	venusmsgs_in_flight = 0;
	venus_account_bytes_sent = 0;
	venus_account_bytes_recv = 0;
	venus_account_nstops = 0; 
	venus_account_nends = 0; 
	if (PRINT_VENUS_INFO) {
		printf("Venus Client is initialized\n");
	}

        // Initialize Interactive Queue...
        create_queue (&Interactive_event_queue);
	free(s);

	return 1;
}

int vc_send(char *s) {
	int l;
	if (!channelSocket) {
		panic("WARNING:venusclient: trying to send through a non initialized socket\n");
		return 0;
	}
	try
	{
		(*channelSocket) << s;
	} catch ( SocketException& e ) 
	{
                std::cout << "WARNING: Exception was caught:" << e.description() << "\n";
		panic("WARNING: venusclient: send failed\n");
		return 0;
	}
	l = strlen(s);
#ifdef VENUS_STATS
	venus_account_bytes_sent += strlen(s);	
	if (!strncmp(s, "STOP", strlen("STOP"))) {
		venus_account_nstops++;
	}
	if (!strncmp(s, "END", strlen("END"))) {
		venus_account_nends++;
	}
#endif
	return l;
}

int vc_recv(char *s, int maxlen) {
	string data;
	int l;

	if (!channelSocket) {
		panic("WARNING:venusclient: trying to send through a non initialized socket\n");
		return 0;
	}

	try
	{
		(*channelSocket) >> data;
	} catch ( SocketException& e ) 
	{
                std::cout << "WARNING: Exception was caught:" << e.description() << "\n";
		panic("WARNING: venusclient: recv failed\n");
		return 0;
	}

	strncpy (s, data.c_str(), maxlen);

	if (data.size() > maxlen) {
		printf("WARNING: venusclient: socket read more bytes (=%d) than fit in the user buffer(%d bytes)\n", data.size(), maxlen);
	}
	l = data.size();
#ifdef VENUS_STATS
	venus_account_bytes_recv += l;
#endif
	return l;
}

int vc_finish() {
	vc_send("FINISH\n");
#ifdef VENUS_STATS
	printf("VENUS STATS: %d bytes sent, %d bytes recv, %d stops, %d ends\n", 
                        venus_account_bytes_sent, venus_account_bytes_recv, venus_account_nstops, venus_account_nends);
#endif
	if (channelSocket) {
		delete channelSocket;
	}
	return 1;
}

int print_event (struct t_event *event) {
	double tmp_timer2;
   	struct t_action *action;

	TIMER_TO_FLOAT(event->event_time, tmp_timer2);
	printf("\t\tTIME IN EVENT: %f us\n", tmp_timer2);
	printf("\t\tMODULE: ");
	switch (event->module) {
		case M_SCH: printf("M_SCH "); break;
		case M_COM: printf("M_COM: "); 
			    switch (event->info) {
				case 0: printf("NO INFO");
				case COM_TIMER_OUT: printf("COM_TIMER_OUT "); break;
				case COM_TIMER_GROUP: printf("COM_TIMER_GROUP "); break;
				case RMA_TIMER_OUT: printf("RMA_TIMER_OUT "); break;
				case COM_EXT_NET_TRAFFIC_TIMER: printf("COM_EXT_NET_TRAFFIC_TIMER "); break;
				case COM_TIMER_GROUP_RESOURCES: printf("COM_TIMER_GROUP_RESOURCES "); break;
				case COM_TIMER_OUT_RESOURCES: printf("COM_TIMER_OUT_RESOURCES "); break;
				default: printf("Unknown INFO: %d", event->info); 
			    }
			    break;
		case M_FS: printf("M_FS "); break;
		case M_EV: printf("M_EV "); break;
		case M_PORT: printf("M_PORT "); break;
		case M_MEM: printf("M_MEM "); break;
		case M_CTXT_SW: printf("M_CTXT_SW "); break;
		case M_RMA: printf("M_RMA "); break;
		default: printf("Unknown MODULE: %d", event->module); 
		
	}
	printf("\n");
	if (event->daemon) { printf("\t\tIS DAEMON\n"); }
	if (event->thread) {
		printf("\t\tTHREAD INFO: PTASK.TASK.THREAD = %d.%d.%d in CPU: %d\n", 
			event->thread->task->Ptask->Ptaskid, 
			event->thread->task->taskid, 
			event->thread->threadid,
			event->thread->cpu->unique_number
		      );
		printf("\t\t%s\n", (event->thread->original_thread) ? "ORIGINAL THREAD" : "COPY THREAD");
		if (event->thread->doing_context_switch) { printf("\t\tDOING CONTEXT SWITCH\n"); }
		if (event->thread->to_be_preempted) { printf("\t\tTO BE PREEMPTED\n"); }
		if (event->thread->doing_busy_wait) { printf("\t\tDOING BUSY WAIT\n"); }
		if (event->thread->doing_startup) { printf("\t\tDOING STARTUP\n"); }
		if (event->thread->startup_done) { printf("\t\tSTARTUP DONE\n"); }
		if (event->thread->doing_copy) { printf("\t\tDOING COPY\n"); }
		if (event->thread->copy_done) { printf("\t\tCOPY DONE\n"); }
		if (event->thread->doing_roundtrip) { printf("\t\tDOING ROUNDTRIP\n"); }
		if (event->thread->roundtrip_done) { printf("\t\tROUNDTRIP DONE\n"); }

		action = event->thread->action;
		if (!action) {
			printf("\t\tNO ACTIONS\n");
		}
		else {
			printf("\t\tACTIONS: ");
			while (action) {
				switch (action->action) {
					case DEAD: printf("DEAD "); break;
					case MEMORY_COPY: printf("MEMORY_COPY "); break;
					case EVEN: printf("EVEN "); break;
					case WORK: printf("WORK "); break;
					case SEND: printf("SEND "); break;
					case RECV: printf("RECV "); break;
					case IRECV: printf("IRECV "); break;
					case WAIT: printf("WAIT "); break;
					case PRIO: printf("PRIO "); break;
					case FS: printf("FS "); break;
					case SEM: printf("SEM "); break;
					case GLOBAL_OP: printf("GLOBAL_OP "); break;
					case MPI_IO: printf("MPI_IO "); break;
					case MPI_OS: printf("MPI_OS "); break;
					default: printf("Unknown ACTION: %d", action->action); 
				}
				action = action->next;
			}
			printf("\n");
		}
	}
	else {
		printf ("\t\tNO THREAD!\n");
	}
		
	return 1;
}


struct t_event*
EVENT_venus_timer (
  dimemas_timer    when,
  int              daemon,
  int              module,
  struct t_thread *thread,
  int              info
)
{
  register struct t_event *event;
  double tmp_timer, tmp_timer2;

  if (daemon == DAEMON)
  {
    /*
    if (
      (are_only_daemons == count_queue (&Event_queue)) &&
       NEQ_0_TIMER (current_time)
    )
    {
      return(E_NIL);
    }
    are_only_daemons++;
    */
  }

  event = (struct t_event *) mallocame (sizeof (struct t_event));

  event->event_time = when;
  event->module     = module;
  event->thread     = thread;
  event->info       = info;
  event->daemon     = daemon;

  if (PRINT_VENUS_INFO) {
          TIMER_TO_FLOAT(current_time, tmp_timer);
          printf("CURRENT TIME is %f us\n", tmp_timer);
          printf("Proposed Event:\n");
          print_event(event);
          //TIMER_TO_FLOAT(TIME_LIMIT, tmp_timer);
          //printf("Proposed Time: %f us\n", tmp_timer);
          //fscanf(orig_stdin, "%*s");
          //FLOAT_TO_TIMER(tmp_timer, event->event_time);
  }

  if (
    ( module != M_CTXT_SW) &&
    ((module != M_COM) || (info != COM_EXT_NET_TRAFFIC_TIMER))
  )
  {
    thread->loose_cpu        = TRUE;
    thread->next_event_timer = when;
  }

  insert_event (&Interactive_event_queue, event);

  if (debug&D_EV)
  {
    PRINT_TIMER (current_time);
    printf (": EVENT Inserted At ");
    PRINT_TIMER (when);
    printf (" Module %d", module);
    if (event->thread == TH_NIL)
    {
      printf("\n");
    }
    else
    {
      printf(
        ". P%02d T%02d (t%02d)\n",
        IDENTIFIERS(event->thread)
      );
    }
  }

  return event;
}

int vc_command_send(double dtime, int src, int dest, int size, void *event, void *event_resources_out) {
	char buffer[10000];
        sprintf(buffer, "SEND %.100lg %d %d %d %p %p %d %d\n", dtime, src, dest, size, event, event_resources_out, src, dest);
        if (PRINT_VENUS_SENDS) {
                printf("%s  [in flight = %d]\n", buffer, venusmsgs_in_flight);
        }
        vc_send(buffer);
        venusmsgs_in_flight++;
	return 1;
}

int vc_command_rdvz_send (double dtime, int src, int dest, int tag, int size) {
	char buffer[10000];
        sprintf(buffer, "PROTO OK_TO_SEND %.100lg %d %d %d %d\n", dtime, src, dest, tag, size);
        if (PRINT_VENUS_SENDS) {
                printf("%s  SEND_READY [in flight = %d]\n", buffer, venusmsgs_in_flight);
        }
        vc_send(buffer);
	return 1;
}

int vc_command_rdvz_ready (double dtime, int src, int dest, int tag, int size, void *event, void *event_resources_out) {
	char buffer[10000];
        sprintf(buffer, "PROTO READY_TO_RECV %.100lg %d %d %d %d %p %p %d %d\n", dtime, src, dest, tag, size, event, event_resources_out, src, dest);
        if (PRINT_VENUS_SENDS) {
                printf("%s  RDVZ [in flight = %d]\n", buffer, venusmsgs_in_flight);
        }
        vc_send(buffer);
        venusmsgs_in_flight++;
	return 1;
}

int venus_outFIFO_event (struct t_queue *q, struct t_item *e) {
	static int processed = 0;
	double tmp_timer, tmp_timer2;
        struct t_event *event;
	double sim_time;


   	e = q->first;
	if (PRINT_VENUS_INFO) {
		printf("EVENT %6d, in QUEUE %p: %d, Interactive Queue = %d\n", processed, q, q->count, Interactive_event_queue.count );
	}
	if (e == ITEM_NIL) {
		printf("\t\tNIL event\n");
	}
	else {
		event = (struct t_event *) e->content;
		if (PRINT_VENUS_INFO) {
			TIMER_TO_FLOAT (e->order.list_time, tmp_timer);
			printf("\t\tTIME: %f us\n", tmp_timer);
			if (e->next != ITEM_NIL) {
				TIMER_TO_FLOAT (e->next->order.list_time, tmp_timer2);
				printf("\t\tTIME OF NEXT EVENT: %f us; DIFF = %f us\n", tmp_timer2, (tmp_timer2-tmp_timer));
			}

			TIMER_TO_FLOAT(event->event_time, tmp_timer2);
			printf("\t\tTIME IN EVENT: %f us, DIFF = %f us\n", tmp_timer2, (tmp_timer2-tmp_timer));
			print_event(event);
		}
	} 

	/* Now STOP simulation, interface VENUS; */
	if (((e != ITEM_NIL) && (event->module == M_COM)) || (top_event(&Interactive_event_queue) != E_NIL)) /* Optimization */
	{
		double tmp_timer;
		char command[10000];
		char *token, *saveptr;
		int endfound = 0;

                if (q->first != ITEM_NIL) {
                        /* STOP AT NEXT RELEVANT EVENT: either M_COM that can
                         * start or progress communications OR last event in
                         * the queue. */
			/*
                        struct t_item *elem;
                        struct t_event *ev;
                        dimemas_timer next_relevant_event;

                        for (elem = q->first; elem != ITEM_NIL; elem = elem->next) {
                                ev = (struct t_event *) elem->content;
                                next_relevant_event = elem->order.list_time;
                                if (ev->module == M_COM) { 
                                        break;
                                }
                        }

                        TIMER_TO_FLOAT(next_relevant_event, tmp_timer);
			*/
                        TIMER_TO_FLOAT(q->first->order.list_time, tmp_timer);
                        sprintf(command, "STOP %.100lg\n", tmp_timer);
                        if (PRINT_VENUS_INFO) {
                                printf("Sending %s", command);
                        }
		        vc_send(command);
                }
                else if (Interactive_event_queue.first != ITEM_NIL) {
                        TIMER_TO_FLOAT(current_time, tmp_timer);
                        sprintf(command, "STOP %.100lg\n", TIME_LIMIT);
                        if (PRINT_VENUS_INFO) {
                                printf("Sending (no events) %s", command);
                        }
		        vc_send(command);
                }
                else {
                        printf("Msgs in flight == %d, but no events in any queue...\n", venusmsgs_in_flight);
                        return 0;
                }
                
		sprintf(command, "END\n");
		if (PRINT_VENUS_INFO) {
			printf("Sending %s", command);
		}
		vc_send(command);

		endfound = 0;
		do {
			if (!vc_recv(command, 10000)) {
				printf("VENUS failed\n");
				return 0;
			}
			token = strtok_r(command, "\n", &saveptr);
			while (token) {
				if (PRINT_VENUS_INFO) {
					printf("VENUS SAYS: %s\n", token);
				}
				if (!strncmp(token, "END", strlen("END"))) {
					endfound = 1;
				}
				else if (!strncmp(token, "COMPLETED SEND", strlen("COMPLETED SEND"))) {
					char buffer[10000];
					double time;
					int from, to, size;
                                        int check_src = -1 , check_dest = -1;
                                        double physical_send_time = 0.0;
					struct t_event *event, *out_resources_event;
					dimemas_timer tmp_timer;
                                        double dimemas_prediction;
                                        buffer[0] = '\0';
                                        int rc;

					if ( (rc = sscanf(token,"COMPLETED SEND %lg %d %d %d %p %p %d %d %lg %[^\n]", &time, &from, &to, &size, &event, &out_resources_event, &check_src, &check_dest, &physical_send_time, buffer)) >= 6) {
        					TIMER_TO_FLOAT(current_time, sim_time);
						if (time < sim_time) {
							fprintf(stderr, "WARNING: DRIFT: STOP scheduled at: %.20lf s, simTime is: %.20lf s; drift of %.20lf us\n",
                                                        	time, sim_time, (sim_time - time)*1000000.0);
                                                		time = sim_time;
						}

                                                if (rc >= 9) {
                                                        if (physical_send_time < event->thread->physical_send) {
                                                                fprintf(stderr,"WARNING: physical send time reported by Venus (= %.20lf s) is smaller than Dimemas physical send (= %.20lf s)\n", physical_send_time, event->thread->physical_send);
                                                        }
                                                        else {
						                FLOAT_TO_TIMER(physical_send_time, tmp_timer);
                                                                ASS_ALL_TIMER (event->thread->physical_send, tmp_timer);
                                                        }  
                                                }

						extract_from_queue(&Interactive_event_queue, (char *)event);
						extract_from_queue(&Interactive_event_queue, (char *)out_resources_event);

                                                TIMER_TO_FLOAT(event->event_time, dimemas_prediction);

						FLOAT_TO_TIMER(time, tmp_timer);
						ASS_ALL_TIMER(out_resources_event->event_time, tmp_timer);
						ASS_ALL_TIMER(event->event_time, tmp_timer);
						insert_event(&Event_queue, out_resources_event);
						insert_event(&Event_queue, event);
						if (PRINT_VENUS_INFO) {
							print_event(event);
							printf("Inserted SEND %.9lf %d %d %d (%p == %p) %s\n", time, from, to, size, event, q->first->content, buffer);
						}
                                                // CHECK RECEIVED 
                                                if (rc >= 8) { 
                                                        //sscanf(buffer, "%d %d", &check_src, &check_dest);
                                                        if (! ( (from == check_src) && (to == check_dest) ) ) {
                                                                printf("ERROR: Venus and Dimemas are not agreeing in their processor mappings\n");
                                                        }
                                                }

					}
					else {
						printf("WARNING: Received SEND, but imcomplete info...\n");
					}
					if (PRINT_VENUS_SENDS) {
						printf("Received SEND %.9lf %d %d %d %p %p %s [in flight: %d] [ev in flight queue: %d] [D_time - V_time = %.9lf]\n", time, from, to, size, event, out_resources_event, buffer, venusmsgs_in_flight, Interactive_event_queue.count, (dimemas_prediction - time));
					}
					/*venusmsgs_in_flight--; --> In COM_TIMER_OUT for internal_network messages */
				}
				token = strtok_r(NULL, "\n", &saveptr);
			}
		}
		while (!endfound);

	}

	processed++;
	return 1;
}

