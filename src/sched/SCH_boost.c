char SCH_boost_c_rcsid[]="$Id: SCH_boost.c,v 1.6 2006/02/08 09:55:17 jgonzale Exp $";
/*
 * Boost priorities scheduling policy routines
 * 
 * Sergi Girona (sergi@ac.upc.es)
 * 
 * (c) DAC-UPC 1993-98
 * 
 */

/* Includes 'por defecto' */
#include "define.h"
#include "types.h"

/* Include propio */
#include "SCH_boost.h"

/* Dependencias con otros fuentes */
#include "cpu.h"
#include "extern.h"
#include "list.h"
#include "mallocame.h"
#include "paraver.h"
#include "schedule.h"
#include "subr.h"

static struct t_boost boost[MAX_MACHINES];

#define BOOST boost[machine->id-1]

void
SCH_boost_thread_to_ready(struct t_thread *thread)
{
  struct t_node      *node;
  struct t_cpu       *cpu, *cpu_to_preemp;
  struct t_SCH_boost *sch_boost;
  t_priority          priority;
  struct t_SCH_boost *sch_boost_cur;
  struct t_thread    *thread_current;
  struct t_machine   *machine;


  sch_boost = (struct t_SCH_boost *) thread->sch_parameters;
  priority = sch_boost->priority;

  node    = get_node_of_thread (thread);
  machine = node->machine;

  if (
    (machine->scheduler.priority_preemptive) && 
    (select_free_cpu (node, thread) == C_NIL)
  )
  {
    cpu_to_preemp = C_NIL;
    /* Select processor to preemp */
    for(cpu  = (struct t_cpu*) head_queue (&(node->Cpus));
        cpu != C_NIL;
        cpu  = (struct t_cpu *) next_queue (&(node->Cpus))
    )
    {
      thread_current = cpu->current_thread;
      sch_boost_cur = (struct t_SCH_boost *)thread_current->sch_parameters;
      if ((sch_boost_cur->priority>sch_boost->priority) && 
          (sch_boost_cur->priority>priority))
      {
        priority      = sch_boost_cur->priority;
        cpu_to_preemp = cpu;
      }
    }
    if (cpu_to_preemp != C_NIL)
    {
      thread = SCHEDULER_preemption(thread, cpu_to_preemp);
    }
    
    if (thread == TH_NIL)
    {
      return;
    }
  }

  /* Puede ser que haya cambiado de thread */
  sch_boost = (struct t_SCH_boost *) thread->sch_parameters;
  priority  = sch_boost->priority;

  if (thread->action == AC_NIL)
  {
    panic (
      "Thread P%02d T%02d (t%02d) to ready without actions\n",
      IDENTIFIERS (thread)
    );
  }
   
  if ((thread->loose_cpu) || (machine->scheduler.lost_cpu_on_send))
  {
    insert_queue (&(node->ready), (char *) thread, priority);
  }
  else
  {
    insert_first_queue (&(node->ready), (char *) thread, priority);
  }
}

t_micro
SCH_boost_get_execution_time(struct t_thread *thread)
{
  struct t_action    *action;
  t_micro             ex_time;
  struct t_SCH_boost *sch_boost;
  struct t_machine   *machine;
  struct t_node      *node;

  node = get_node_of_thread (thread);
  machine = node->machine;

  sch_boost = (struct t_SCH_boost *) thread->sch_parameters;
  action = thread->action;
  if (action->action != WORK)
    panic (
      "Trying to work when innaproppiate P%d T%d t%d", IDENTIFIERS (thread));

  if ((thread->loose_cpu) ||
      (sch_boost->last_quantum == machine->scheduler.quantum))
  {
    ex_time = MIN (action->desc.compute.cpu_time, machine->scheduler.quantum);
    sch_boost->last_quantum = ex_time;
  }
  else
  {
    ex_time = MIN (machine->scheduler.quantum - sch_boost->last_quantum,
                   action->desc.compute.cpu_time);
    sch_boost->last_quantum += ex_time;
  }
  action->desc.compute.cpu_time -= ex_time;
  if (action->desc.compute.cpu_time == 0)
  {
    thread->action = action->next;
    freeame ((char *) action, sizeof (struct t_action));
  }

  thread->loose_cpu = TRUE;
  return (ex_time);
}

struct t_thread *
SCH_boost_next_thread_to_run(struct t_node *node)
{
  return ((struct t_thread *) outFIFO_queue (&(node->ready)));
}

void
SCH_boost_init_scheduler_parameters(struct t_thread *thread)
{
  struct t_SCH_boost *sch_boost;

  sch_boost = (struct t_SCH_boost *) mallocame (sizeof (struct t_SCH_boost));
  sch_boost->priority = thread->base_priority;
  sch_boost->last_quantum = (t_micro) 0;
  thread->sch_parameters = (char *) sch_boost;
}

void
SCH_boost_clear_parameters(struct t_thread *thread)
{
  struct t_SCH_boost *sch_boost;
  struct t_node      *node;
  struct t_machine   *machine;

  node = get_node_of_thread (thread);
  machine = node->machine;

  sch_boost = (struct t_SCH_boost *) thread->sch_parameters;
  sch_boost->priority = BOOST.base_prio;
  sch_boost->last_quantum = (t_micro) 0;
}

int
SCH_boost_info(int info, struct t_thread *th_s, struct t_thread *th_r)
{
  struct t_SCH_boost *sch_boost;
  struct t_node      *node;
  t_priority          old_prio;
  struct t_thread    *thread;
  struct t_cpu       *cpu;
  struct t_machine   *machine;

  node = get_node_of_thread (th_s);
  machine = node->machine;

  /* Suposo que els dos threads son com a minim de la mateixa maquina */
  switch (info)
  {
    case SCH_INFO_SEND:
      sch_boost = (struct t_SCH_boost *) th_s->sch_parameters;
      old_prio  = sch_boost->priority;
      thread    = th_s;
      /* Sender arrives when receiver is ready */
      sch_boost->priority = old_prio + BOOST.sender_last;
      if ((paraver_priorities) && ((int) old_prio != (int) sch_boost->priority))
      {
        node = get_node_of_thread (th_s);
      }
      break;
    
    case SCH_INFO_RECV_MISS:
      sch_boost = (struct t_SCH_boost *) th_r->sch_parameters;
      old_prio  = sch_boost->priority;
      thread    = th_r;
      /* Receiver arrives when message is ready */
      sch_boost->priority = old_prio + BOOST.recv_first;

      if ((paraver_priorities) && ((int) old_prio != (int) sch_boost->priority))
      {
        node = get_node_of_thread (th_r);
      }
      break;

    case SCH_INFO_RECV_HIT:
      sch_boost = (struct t_SCH_boost *) th_r->sch_parameters;
      old_prio  = sch_boost->priority;
      thread    = th_r;
      /* Receiver arrives when message is ready */
      sch_boost->priority = old_prio + BOOST.recv_last;

      if ((paraver_priorities) && ((int) old_prio != (int) sch_boost->priority))
      {
        node = get_node_of_thread (th_r);
      }
      break;

    case SCH_PRIORITY:
      sch_boost = (struct t_SCH_boost *) th_s->sch_parameters;
      return ((int)sch_boost->priority);

    default:
      panic ("Info invalid", "SCH_boost_info", "SCH_boost.c");
      exit (1);
      break;
  }
  /* Bounded priority */
  if (sch_boost->priority < BOOST.best)
    sch_boost->priority = BOOST.best;
  if (sch_boost->priority > BOOST.worst)
    sch_boost->priority = BOOST.worst;

  if (debug)
  {
    PRINT_TIMER (current_time);
    printf (": BOOST recompute priority P%d T%d t%d from %.1f to %.1f\n",
            IDENTIFIERS(th_r),
            old_prio,
            sch_boost->priority);
  }

  if (old_prio != sch_boost->priority)
  {
    node = get_node_of_thread(thread);
    cpu = get_cpu_of_thread (thread);
    Paraver_event(cpu->unique_number,IDENTIFIERS(thread), 
    current_time, 191, (int)sch_boost->priority);
  }

  return (0);
}

void
SCH_boost_init(char *filename, struct t_machine *machine)
{
  FILE           *file;
  int             r;
  char buf[256],str[256];

  BOOST.best        = BOOST_MAX_PRIO;
  BOOST.worst       = BOOST_MIN_PRIO;
  BOOST.base_prio   = BASE_PRIORITY;
  BOOST.recv_first  = BOOST_RECV_FIRST;
  BOOST.recv_last   = BOOST_RECV_LAST;
  BOOST.sender_last = BOOST_SENDER_LAST;

  if (filename == (char *) 0)
  {
    fprintf (stderr, "No boost scheduler parameters. Using default\n");
    return;
  }
  file = MYFOPEN (filename, "r");
  if (file == NULL)
  {
    fprintf (stderr, "Cant open boost parameter file %s\n", filename);
    fprintf (stderr, "No boost scheduler parameters. Using default\n");
    return;
  }

  fgets (str, 256, file);
  r = sscanf (str, "Policy: %s", buf);
  if (r!=1)
    panic ("Invalid format in file %s.\nInvalid policy name %s\n",
           filename,
           buf);

  fgets (str, 256, file);
  r = sscanf (str, "Best priority: %d", &BOOST.best);
  if (r != 1)
  {
    panic ("Invalid Best priority in boost scheduler file %s\n", filename);
  }

  fgets (str, 256, file);
  r = sscanf (str, "Worst priority: %d", &BOOST.worst);
  if (r != 1)
  {
    panic ("Invalid worst priority in Boost scheduler file %s\n", filename);
  }

  if (BOOST.best > BOOST.worst)
  {
    BOOST.best = BOOST.worst;
    fprintf (stderr, "Worst priority must be greater than best priority\n");
    fprintf (stderr, "Truncated. Best==worst\n");
  }

  fgets (str, 256, file);
  r = sscanf (str, "Base priority: %d", &BOOST.base_prio);
  if (r != 1)
  {
    panic ("Invalid Best priority in Boost scheduler file %s\n", filename);
  }

  if ((BOOST.base_prio < BOOST.best) || (BOOST.base_prio > BOOST.worst))
  {
    fprintf (stderr, "Base priority must be into [best,worst]\n");
    fprintf (stderr, "Truncated\n");
    BOOST.base_prio = (BOOST.base_prio < BOOST.best ? BOOST.best : BOOST.worst);
  }

  fgets (str, 256, file);
  r = sscanf (str, "Boost if receiver arrives first: %d", &BOOST.recv_first);
  if (r != 1)
  {
    panic ("Invalid recv first priority in Boost scheduler file",filename);
  }

  fgets (str, 256, file);
  r = sscanf (str, "Boost if receiver arrives last: %d", &BOOST.recv_last);
  if (r != 1)
  {
    panic ("Invalid recv last priority in Boost scheduler file",filename);
  }

  fgets (str, 256, file);
  r = sscanf (str, "Boost if sender arrives last: %d", &BOOST.sender_last);
  if (r != 1)
  {
    panic ("Invalid sender last priority in Boost scheduler file",filename);
  }
  if (debug)
  {
    fprintf (stderr, "Boost parameters: [%d,%d], (%d,%d),%d , %d\n",
             BOOST.best, BOOST.worst,
             BOOST.recv_first, BOOST.recv_last,
             BOOST.sender_last,
             BOOST.base_prio);
  }
}

void
SCH_boost_copy_parameters(struct t_thread *th_o, struct t_thread *th_d)
{
  struct t_SCH_boost *sch_boost1;
  struct t_SCH_boost *sch_boost2;


  sch_boost1 = (struct t_SCH_boost *) th_o->sch_parameters;
  sch_boost2 = (struct t_SCH_boost *) th_d->sch_parameters;
  sch_boost2->priority = sch_boost1->priority;
  sch_boost2->last_quantum = sch_boost1->last_quantum;
}

void
SCH_boost_free_parameters(struct t_thread *thread)
{
  struct t_SCH_boost *sch_boost;

  sch_boost = (struct t_SCH_boost *) thread->sch_parameters;
  freeame ((char *) sch_boost, sizeof (struct t_SCH_boost));
}
