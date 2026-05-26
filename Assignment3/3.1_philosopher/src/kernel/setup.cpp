#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"

STDIO stdio;
InterruptManager interruptManager;
ProgramManager programManager;

#define N 5 // 哲学家数量

Semaphore chopstick[N]; // 5根筷子，每根初始为1

// 哲学家线程 id: 0~4
void philosopher(void *arg)
{
    int id = (int)arg;
    int left = id;              // 左手边筷子编号
    int right = (id + 1) % N;   // 右手边筷子编号

    for (int cycle = 0; cycle < 3; ++cycle)
    {
        // --- 思考 ---
        printf("Philosopher %d: thinking...\n", id);
        int delay = 0xfffff;
        while (delay) --delay;

        // --- 饥饿，尝试拿筷子 ---
        printf("Philosopher %d: hungry, trying to pick up chopsticks\n", id);

        chopstick[left].P();    // 拿起左边筷子
        printf("Philosopher %d: picked up left chopstick %d\n", id, left);

        chopstick[right].P();   // 拿起右边筷子
        printf("Philosopher %d: picked up right chopstick %d\n", id, right);

        // --- 吃饭 ---
        printf("Philosopher %d: >>> EATING <<<\n", id);
        delay = 0xfffff;
        while (delay) --delay;

        // --- 放下筷子 ---
        chopstick[left].V();
        chopstick[right].V();
        printf("Philosopher %d: put down both chopsticks, back to thinking\n", id);
    }
}

void first_thread(void *arg)
{
    stdio.moveCursor(0);
    for (int i = 0; i < 25 * 80; ++i)
        stdio.print(' ');
    stdio.moveCursor(0);

    printf("=== Dining Philosophers (Basic) ===\n");
    printf("5 philosophers, 5 chopsticks (may deadlock)\n\n");

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
