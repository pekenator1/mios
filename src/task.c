#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "task.h"
#include "sys.h"
#include "irq.h"
#include "cpu.h"

static struct task_queue readyqueue = TAILQ_HEAD_INITIALIZER(readyqueue);

static task_t idle_task = {
  .t_name = "idle",
  .t_state = TASK_STATE_ZOMBIE,
};

#define STACK_GUARD 0xbadc0de

struct task *curtask = &idle_task;

void *
sys_switch(void *cur_sp)
{
  curtask->t_sp = cur_sp;

  int s = irq_forbid(IRQ_LEVEL_SCHED);

  if(curtask->t_state == TASK_STATE_RUNNING) {
    // Task should be running, re-insert in readyqueue
    TAILQ_INSERT_TAIL(&readyqueue, curtask, t_link);
  }

  task_t *t = TAILQ_FIRST(&readyqueue);
  if(t == NULL) {
    t = &idle_task;
  } else {
    TAILQ_REMOVE(&readyqueue, t, t_link);
  }

#if 0
  printf("Switch from %s to %s\n", curtask->t_name, t->t_name);
#endif

  irq_permit(s);

  curtask = t;
  return t->t_sp;
}


static void
task_end(void)
{
  curtask->t_state = TASK_STATE_ZOMBIE;
  schedule();
  irq_lower();
  while(1) {
  }
}


task_t *
task_create(void *(*entry)(void *arg), void *arg, size_t stack_size,
            const char *name)
{
  assert(stack_size >= 256);
  task_t *t = malloc(sizeof(task_t) + stack_size);
  t->t_name = name;

  uint32_t *stack_bottom = (void *)t->t_stack;
  *stack_bottom = STACK_GUARD;

  t->t_state = 0;
  t->t_sp = cpu_stack_init((void *)t->t_stack + stack_size, entry, arg,
                           task_end);

  printf("Creating task %p %s\n", t, name);

  int s = irq_forbid(IRQ_LEVEL_SCHED);
  TAILQ_INSERT_TAIL(&readyqueue, t, t_link);
  irq_permit(s);

  schedule();
  return t;
}


void
task_wakeup(struct task_queue *waitable, int all)
{
  int s = irq_forbid(IRQ_LEVEL_SCHED);

  task_t *t;
  while((t = TAILQ_FIRST(waitable)) != NULL) {
    assert(t->t_state == TASK_STATE_SLEEPING);
    TAILQ_REMOVE(waitable, t, t_link);
    t->t_state = TASK_STATE_RUNNING;
    TAILQ_INSERT_TAIL(&readyqueue, t, t_link);
    schedule();
    if(!all)
      break;
  }
  irq_permit(s);
}


typedef struct task_sleep {
  task_t *task;
  struct task_queue *waitable;
} task_sleep_t;


static void
task_sleep_timeout(void *opaque)
{
  const task_sleep_t *ts = opaque;
  task_t *t = ts->task;

  const int s = irq_forbid(IRQ_LEVEL_SCHED);

  if(t->t_state == TASK_STATE_SLEEPING) {

    if(ts->waitable != NULL)
      TAILQ_REMOVE(ts->waitable, t, t_link);

    t->t_state = TASK_STATE_RUNNING;
    TAILQ_INSERT_TAIL(&readyqueue, t, t_link);
    schedule();
  }
  irq_permit(s);
}



void
task_sleep(struct task_queue *waitable, int ticks)
{
  timer_t timer;
  task_sleep_t ts;

  const int s = irq_forbid(IRQ_LEVEL_SCHED);
  assert(curtask->t_state == TASK_STATE_RUNNING);
  curtask->t_state = TASK_STATE_SLEEPING;

  if(ticks) {
    ts.task = curtask;
    ts.waitable = waitable;
    timer.t_cb = task_sleep_timeout;
    timer.t_opaque = &ts;
    timer.t_countdown = 0;
    timer_arm(&timer, ticks);
  }

  if(waitable != NULL) {
    TAILQ_INSERT_TAIL(waitable, curtask, t_link);
  }

  while(curtask->t_state == TASK_STATE_SLEEPING) {
    schedule();
    irq_permit(irq_lower());
  }

  if(ticks) {
    timer_disarm(&timer);
  }

  irq_permit(s);
}


void
sleephz(int ticks)
{
  task_sleep(NULL, ticks);
}


void
mutex_init(mutex_t *m)
{
  TAILQ_INIT(&m->waiters);
  m->owner = NULL;
}

void
mutex_lock(mutex_t *m)
{
  const int s = irq_forbid(IRQ_LEVEL_SCHED);

  if(m->owner != NULL) {
    assert(m->owner != curtask);
    curtask->t_state = TASK_STATE_SLEEPING;
    TAILQ_INSERT_TAIL(&m->waiters, curtask, t_link);
    while(m->owner != NULL) {
      schedule();
      irq_permit(irq_lower());
    }
  }
  m->owner = curtask;

  irq_permit(s);
}

void
mutex_unlock(mutex_t *m)
{
  int s = irq_forbid(IRQ_LEVEL_SCHED);
  assert(m->owner == curtask);
  m->owner = NULL;

  task_t *t = TAILQ_FIRST(&m->waiters);
  if(t != NULL) {
    TAILQ_REMOVE(&m->waiters, t, t_link);
    t->t_state = TASK_STATE_RUNNING;
    TAILQ_INSERT_TAIL(&readyqueue, t, t_link);
    schedule();
  }
  irq_permit(s);
}

