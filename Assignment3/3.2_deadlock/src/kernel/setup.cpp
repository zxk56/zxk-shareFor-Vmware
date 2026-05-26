#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"

STDIO stdio;
InterruptManager interruptManager;
ProgramManager programManager;

#define N 5

Semaphore chopstick[N];

void philosopher(void *arg)
{
    int id = (int)arg;
    int left = id;
    int right = (id + 1) % N;

    for (int cycle = 0; cycle < 2; ++cycle)
    {
        printf("Philosopher %d: thinking...\n", id);
        int delay = 0xfffff;
        while (delay) --delay;

        printf("Philosopher %d: hungry, trying to pick up left chopstick %d\n", id, left);

        chopstick[left].P();
        printf("Philosopher %d: picked up left chopstick %d\n", id, left);

        // ★ 关键：拿起左边筷子后加一个长延时，
        //    让所有哲学家都有时间拿起左边筷子，从而稳定死锁
        delay = 0xffffff;
        while (delay) --delay;

        printf("Philosopher %d: trying to pick up right chopstick %d\n", id, right);

        chopstick[right].P();
        printf("Philosopher %d: picked up right chopstick %d\n", id, right);

        printf("Philosopher %d: >>> EATING <<<\n", id);
        delay = 0xfffff;
        while (delay) --delay;

        chopstick[left].V();
        chopstick[right].V();
        printf("Philosopher %d: put down both chopsticks\n", id);
    }
}

void first_thread(void *arg)
{
    stdio.moveCursor(0);
    for (int i = 0; i < 25 * 80; ++i)
        stdio.print(' ');
    stdio.moveCursor(0);

    printf("=== Dining Philosophers (DEADLOCK DEMO) ===\n");
    printf("Added delay between left & right chopstick\n");
    printf("All 5 will deadlock after picking up left chopstick\n\n");

    for (int i = 0; i < N; ++i)
        chopstick[i].initialize(1);

    for (int i = 0; i < N; ++i)
        programManager.executeThread(philosopher, (void *)i, "philosopher", 1);

    asm_halt();
}

extern "C" void setup_kernel()
{
    interruptManager.initialize();
    interruptManager.enableTimeInterrupt();
    interruptManager.setTimeInterrupt((void *)asm_time_interrupt_handler);

    stdio.initialize();
    programManager.initialize();

    int pid = programManager.executeThread(first_thread, nullptr, "first thread", 1);
    if (pid == -1)
    {
        printf("can not execute thread\n");
        asm_halt();
    }

    ListItem *item = programManager.readyPrograms.front();
    PCB *firstThread = ListItem2PCB(item, tagInGeneralList);
    firstThread->status = RUNNING;
    programManager.readyPrograms.pop_front();
    programManager.running = firstThread;
    asm_switch_thread(0, firstThread);

    asm_halt();
}
