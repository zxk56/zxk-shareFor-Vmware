#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"
#include "memory.h"

// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;
// 程序管理器
ProgramManager programManager;
// 内存管理器
MemoryManager memoryManager;

void first_thread(void *arg)
{
    // ============================================================
    // Assignment 4.1: FIFO 页面置换算法模拟
    // ============================================================
    printf("====================================================\n");
    printf("  Assignment 4: FIFO Page Replacement Simulation\n");
    printf("====================================================\n\n");

    // 定义虚拟页访问序列
    // 这是一个经典的页面引用串，用于展示FIFO的行为
    int accessSequence[] = {
        0, 1, 2, 3,  // 初始加载4个不同的页
        0, 1,        // 两次命中
        4,           // 缺页，淘汰页0 (FIFO最早的)
        0,           // 缺页，淘汰页1
        1,           // 缺页，淘汰页2
        2,           // 缺页，淘汰页3
        3,           // 缺页，淘汰页4
        4,           // 缺页，淘汰页0
        0, 1, 2,     // 更多访问
        5,           // 新页面
        5,           // 命中
        0,           // 可能命中或缺页
    };
    int seqLen = sizeof(accessSequence) / sizeof(accessSequence[0]);

    // --------------------------------------------------------
    // 测试1: 使用4个物理帧
    // --------------------------------------------------------
    printf("--- Test 1: FIFO with 4 physical frames ---\n");
    printf("Access sequence: ");
    for (int i = 0; i < seqLen; ++i)
    {
        printf("%d ", accessSequence[i]);
    }
    printf("\n\n");

    // 初始化FIFO置换模拟，4个物理帧
    memoryManager.initPageReplacement(4);

    // 依次访问每个虚拟页
    for (int i = 0; i < seqLen; ++i)
    {
        memoryManager.accessPage(accessSequence[i]);
    }

    printf("\nFinal frame status:\n");
    memoryManager.printFrameStatus();
    memoryManager.printStatistics();

    // --------------------------------------------------------
    // 测试2: 使用3个物理帧 (对比)
    // --------------------------------------------------------
    printf("\n--- Test 2: FIFO with 3 physical frames ---\n");
    printf("Same access sequence for comparison.\n\n");

    // 重新初始化，3个物理帧
    memoryManager.initPageReplacement(3);

    for (int i = 0; i < seqLen; ++i)
    {
        memoryManager.accessPage(accessSequence[i]);
    }

    printf("\nFinal frame status:\n");
    memoryManager.printFrameStatus();
    memoryManager.printStatistics();

    // --------------------------------------------------------
    // 测试3: Belady异常演示 (FIFO特有的现象)
    // --------------------------------------------------------
    printf("\n--- Test 3: Belady's Anomaly Demo ---\n");
    printf("Reference string: 1,2,3,4,1,2,5,1,2,3,4,5\n\n");

    int beladySeq[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    int beladyLen = sizeof(beladySeq) / sizeof(beladySeq[0]);

    // 3帧
    printf("With 3 frames:\n");
    memoryManager.initPageReplacement(3);
    for (int i = 0; i < beladyLen; ++i)
    {
        memoryManager.accessPage(beladySeq[i]);
    }
    memoryManager.printStatistics();

    // 4帧
    printf("\nWith 4 frames:\n");
    memoryManager.initPageReplacement(4);
    for (int i = 0; i < beladyLen; ++i)
    {
        memoryManager.accessPage(beladySeq[i]);
    }
    memoryManager.printStatistics();

    printf("\nNote: FIFO may show MORE faults with 4 frames than 3!\n");
    printf("This counter-intuitive behavior is Belady's anomaly.\n");

    // --------------------------------------------------------
    // 测试4: 较大访问序列
    // --------------------------------------------------------
    printf("\n--- Test 4: Larger access sequence (8 frames) ---\n");
    int largeSeq[] = {
        7, 0, 1, 2, 0, 3, 0, 4, 2, 3,
        0, 3, 2, 1, 2, 0, 1, 7, 0, 1,
        5, 6, 5, 6, 7, 8, 5, 6, 7, 8
    };
    int largeLen = sizeof(largeSeq) / sizeof(largeSeq[0]);

    printf("Access sequence (%d refs): ", largeLen);
    for (int i = 0; i < largeLen; ++i)
    {
        printf("%d ", largeSeq[i]);
    }
    printf("\n\n");

    memoryManager.initPageReplacement(8);
    for (int i = 0; i < largeLen; ++i)
    {
        memoryManager.accessPage(largeSeq[i]);
    }
    memoryManager.printStatistics();

    printf("\n====================================================\n");
    printf("  Assignment 4 Complete!\n");
    printf("====================================================\n");

    asm_halt();
}

extern "C" void setup_kernel()
{
    // 中断管理器
    interruptManager.initialize();
    interruptManager.enableTimeInterrupt();
    interruptManager.setTimeInterrupt((void *)asm_time_interrupt_handler);

    // 输出管理器
    stdio.initialize();

    // 进程/线程管理器
    programManager.initialize();

    // 内存管理器
    memoryManager.openPageMechanism();
    memoryManager.initialize();

    // 创建第一个线程
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
