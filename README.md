# 1. Segundo trabalho de Sistemas Operacionais Embarcados

Para este trabalho foi feita uma chamada Sistema ```sem_waiters()``` no kernel do Nuttx que informa quais tarefas estão bloqueadas à espera de um semáforo:

```c
int sem_waiters(FAR sem_t *sem, FAR pid_t *pids, int max);
```

Escreve em pids os pids das tarefas bloqueadas (da maior para a menor prioridade) e retorna quantas são. Em caso de parâmetros inválidos retorna **-1** com **errno = EINVAL**.

# 2. Justificativa da escolha

O NuttX não tinha forma de obter esta informação: o ps mostra que uma tarefa está Waiting,Semaphore, mas não em qual semáforo nem quem mais está à espera dele. 
Saber isso ajuda a diagnosticar bloqueios e deadlocks.
# 3. Funcionamento

Cada semáforo tem uma lista de espera ```(waitlist)```. Quando uma tarefa faz ```sem_wait()``` e bloqueia, o kernel coloca o TCB dela nessa lista, ordenada por prioridade.
A ```sem_waiters()``` percorre essa lista e copia o pid de cada tarefa:

```c
flags = enter_critical_section();

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

leave_critical_section(flags);
```

**Sincronização:** a lista muda quando alguém faz ```sem_post()```, inclusive dentro de uma interrupção. 
Por isso o percurso é feito numa seção crítica (interrupções desligadas), para que nenhuma tarefa saia da lista enquanto a lemos.

**Resultado dos testes**: A aplicação cria 3 tarefas(prioridade 80,110 e 90) que bloqueiam no mesmo semáforo e testa também entradas invalidas

```
2) tres tarefas fazem sem_wait() (prioridades 80, 110, 90)
  3 tarefa(s) bloqueada(s): pid 9 (prio 110) pid 10 (prio 90) pid 8 (prio 80)
3) sem_post(): acorda a tarefa de maior prioridade
  2 tarefa(s) bloqueada(s): pid 10 (prio 90) pid 8 (prio 80)
4) vetor com espaco para 1 pid
  retorno=2, pids[0]=10 -> OK
5) entradas invalidas
  sem = NULL             ret=-1 errno=22 -> OK
  max = -1               ret=-1 errno=22 -> OK
  pids = NULL, max = 8   ret=-1 errno=22 -> OK

```

  As tarefas foram criadas pela ordem ```8, 9, 10```, mas aparecem como ```9, 10, 8```: a ordem de prioridade em que o kernel guarda a lista. 
  Depois do ```sem_post()``` sai a de maior prioridade (pid 9).
