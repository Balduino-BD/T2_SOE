/*
 * caminho:include/nuttx/semwaiters.h
 *
 * DEC7562 - Trabalho 2: nova chamada de sistema sem_waiters() .
 * Balduino Da Silva
 */

#ifndef __INCLUDE_NUTTX_SEMWAITERS_H
#define __INCLUDE_NUTTX_SEMWAITERS_H

#include <nuttx/config.h>
#include <sys/types.h>
#include <semaphore.h>

/* sem_waiters(): informa quais tarefas estao bloqueadas no semaforo "sem".
 *
 *   pids - vetor onde o kernel escreve os pids das tarefas bloqueadas,
 *          da maior para a menor prioridade (pode ser NULL se max = 0)
 *   max  - tamanho do vetor pids
 *
 * Retorna o numero TOTAL de tarefas bloqueadas (mesmo que seja maior que
 * max), ou -1 (ERROR) com errno = EINVAL se os parametros forem invalidos.
 */

int sem_waiters(FAR sem_t *sem, FAR pid_t *pids, int max);

#endif /* __INCLUDE_NUTTX_SEMWAITERS_H */
