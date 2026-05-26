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

// 限制同时拿筷子的哲学家最多4人，打破循环等待
Semaphore limit;

void philosopher(void *arg)
{
    int id = (int)arg;
    int left = id;
    int right = (id + 1) % N;

    for (int cycle = 0; cycle < 3; ++cycle)
    {
        printf("Philosopher %d: thinking...\n", id);
        int delay = 0xfffff;
        while (delay) --delay;

        printf("Philosopher %d: hungry, trying to eat\n", id);

        // ★ 先申请"就餐名额"（最多4人同时拿筷子）
        limit.P();

        chopstick[left].P();
        printf("Philosopher %d: picked up left chopstick %d\n", id, left);

        chopstick[right].P();
        printf("Philosopher %d: picked up right chopstick %d\n", id, right);

        printf("Philosopher %d: >>> EATING <<<\n", id);
        delay = 0xfffff;
        while (delay) --delay;

        chopstick[left].V();
        chopstick[right].V();
        printf("Philosopher %d: put down both chopsticks\n", id);

        limit.V(); // ★ 释放就餐名额

        delay = 0xffffff;
        while (delay) --delay;
    }
}

void first_thread(void *arg)
{
    stdio.moveCursor(0);
    for (int i = 0; i < 25 * 80; ++i)
        stdio.print(' ');
    stdio.moveCursor(0);

    printf("=== Dining Philosophers (Deadlock-Free) ===\n");
    printf("Strategy: max 4 philosophers can hold chopsticks\n\n");

    for (int i = 0; i < N; ++i)
        chopstick[i].initialize(1);

    limit.initialize(N - 1); // 最多允许4人同时拿筷子

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
