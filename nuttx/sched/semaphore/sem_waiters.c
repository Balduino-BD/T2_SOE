/*
 * caminhho: sched/semaphore/sem_waiters.c
 *
 * DEC7562 - Trabalho 2: nova chamada de sistema sem_waiters() .
 * Balduino Da Silva
 */

#include <nuttx/config.h>

#include <sys/types.h>
#include <errno.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <nuttx/sched.h>
#include <nuttx/semwaiters.h>

int sem_waiters(FAR sem_t *sem, FAR pid_t *pids, int max)
{
  FAR struct tcb_s *tcb;
  irqstate_t flags;
  int count = 0;

  /* 1. Validacao dos parametros */

  if (sem == NULL || max < 0 || (pids == NULL && max > 0))
    {
      set_errno(EINVAL); 
      return ERROR;
    }


  /* 2. Entra na secao critica: um sem_post() (de outra tarefa ou de uma
   *    interrupcao) pode tirar uma tarefa da lista enquanto a percorremos.
   */

  flags = enter_critical_section();

  /* 3. Percorre a lista de espera do semaforo. O kernel guarda nela o TCB
   *    de cada tarefa bloqueada em sem_wait(), ordenada por prioridade.
   */

  for (tcb = (FAR struct tcb_s *)dq_peek(SEM_WAITLIST(sem));
       tcb != NULL;
       tcb = tcb->flink)
    {
      if (count < max)
        {
          pids[count] = tcb->pid;
        }

      count++;
    }

  /* 4. Sai da secao critica e devolve o total */

  leave_critical_section(flags);
  return count;
}
