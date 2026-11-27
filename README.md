

# Segundo trabalho de Sistemas Operacionais Embarcados (DEC7562)

**Aluno:** Balduino José da Silva 

## 1. Funcionalidade

Foi criada no kernel do NuttX a chamada de sistema `sem_waiters()`, que informa quais tarefas estão bloqueadas à espera de um semáforo:

```c
int sem_waiters(FAR sem_t *sem, FAR pid_t *pids, int max);
```

Escreve em `pids` os pids das tarefas bloqueadas (da maior para a menor prioridade) e retorna quantas são. Em caso de parâmetros inválidos retorna `-1` com `errno = EINVAL`.

## 2. Justificativa da escolha

O NuttX não tinha forma de obter esta informação: o `ps` mostra que uma tarefa está `Waiting,Semaphore`, mas não em qual semáforo nem quem mais está à espera dele. Saber isso ajuda a diagnosticar bloqueios e *deadlocks*.

## 3. Funcionamento

Cada semaforo tem uma lista de espera (`waitlist`). Quando uma tarefa faz `sem_wait()` e bloqueia, o kernel coloca o TCB dela nessa lista, ordenada por prioridade. A `sem_waiters()` percorre essa lista e copia o pid de cada tarefa:

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

**Sincronização:** a lista muda quando alguém faz `sem_post()`, inclusive dentro de uma interrupção. Por isso o percurso é feito numa seção crítica (interrupções desligadas), para que nenhuma tarefa saia da lista enquanto a lemos.

## 4. Estruturas e funções do NuttX utilizadas

| Elemento | Onde | Para quê |
|---|---|---|
| `sem_t` → campo `waitlist` | `include/semaphore.h` | lista das tarefas bloqueadas no semáforo |
| `SEM_WAITLIST(sem)` | `include/semaphore.h` | dá acesso a essa lista |
| `struct tcb_s` (`pid`, `flink`) | `include/nuttx/sched.h` | cada elemento da lista é o TCB de uma tarefa; `flink` aponta para o seguinte |
| `dq_peek()` | `include/nuttx/queue.h` | devolve o primeiro elemento da lista |
| `enter_critical_section()` / `leave_critical_section()` | `include/nuttx/irq.h` | desligam/religam as interrupções durante o percurso |
| `nxsched_add_prioritized()` | `sched/semaphore/sem_wait.c` | insere a tarefa na lista, ordenada por prioridade |

## 5. Arquivos adicionados e modificados no NuttX

| Arquivo | Situação |
|---|---|
| `nuttx/include/nuttx/semwaiters.h` | **novo** – declaração da função |
| `nuttx/sched/semaphore/sem_waiters.c` | **novo** – implementação no kernel |
| `nuttx/include/sys/syscall_lookup.h` | **modificado** (+1 linha) – registro da chamada de sistema |
| `nuttx/sched/semaphore/Make.defs` | **modificado** (+2 linhas) – compilação do novo arquivo |
| `nuttx/sched/semaphore/CMakeLists.txt` | **modificado** (+1 linha) - compilação com CMake |
| `apps/examples/semwaiters/` | **novo** – aplicação de teste |

Linhas acrescentadas aos arquivos existentes (Estão todas marcadas com `DEC7562`):

```c
/* include/sys/syscall_lookup.h */
SYSCALL_LOOKUP(sem_waiters,                3)  /* DEC7562 */
```

```make
# sched/semaphore/Make.defs
# DEC7562: nova chamada de sistema sem_waiters()
CSRCS += sem_waiters.c
```

```cmake
# sched/semaphore/CMakeLists.txt (no fim da lista CSRCS)
    sem_waiters.c) # DEC7562: nova chamada de sistema sem_waiters()
```

O `syscall/syscall.csv` não foi alterado: no modelo FLAT a aplicação chama a função do kernel diretamente.

## 6. Compilação e execução

A placa usada para este trabalho foi a Heltec WiFi LoRa 32 V2 (ESP32), configuração `esp32-devkitc:nsh`

```bash
cd ~/nuttxspace/nuttx
./tools/configure.sh -l esp32-devkitc:nsh
echo "CONFIG_ESP32_IGNORE_CHIP_REVISION_CHECK=y" >> .config   # só para ESP32 rev. < 3.0 (o da Heltec é v1.1)
echo "CONFIG_EXAMPLES_SEMWAITERS=y" >> .config
make olddefconfig
make -j$(nproc)
make flash ESPTOOL_PORT=/dev/ttyUSB0 ESPTOOL_BINDIR=./
picocom -b 115200 /dev/ttyUSB0
```

No terminal da placa: `nsh> semwaiters`

## 7. Testes e resultados

A aplicação cria 3 tarefas (prioridades 80, 110 e 90, nesta ordem) que bloqueiam no mesmo semáforo e testa também entradas inválidas:

```
2) tres tarefas fazem sem_wait() (prioridades 80, 110, 90)
  3 tarefa(s) bloqueada(s): pid 5 (prio 110) pid 6 (prio 90) pid 4 (prio 80)
3) sem_post(): acorda a tarefa de maior prioridade
  2 tarefa(s) bloqueada(s): pid 6 (prio 90) pid 4 (prio 80)
4) vetor com espaco para 1 pid
  retorno=2, pids[0]=6 -> OK
5) entradas invalidas
  sem = NULL             ret=-1 errno=22 -> OK
  max = -1               ret=-1 errno=22 -> OK
  pids = NULL, max = 8   ret=-1 errno=22 -> OK
```

As tarefas foram criadas pela ordem 4, 5, 6, mas aparecem como 5, 6, 4: a ordem de prioridade em que o kernel guarda a lista. Depois do `sem_post()` sai a de maior prioridade (pid 5).

No fim, a aplicação espera 15 s antes de libertar as tarefas, para que possam ser vistas com o `ps` (`nsh> semwaiters &` seguido de `nsh> ps`):

```
  PID PRI STATE    EVENT     COMMAND
    3 100 Waiting  Signal    semwaiters
    4  80 Waiting  Semaphore t80
    6  90 Waiting  Semaphore t90
```

O `ps` mostra que as tarefas estão bloqueadas num semáforo, mas só a `sem_waiters()` indica em qual e por que ordem serão acordadas (6 e depois 4).