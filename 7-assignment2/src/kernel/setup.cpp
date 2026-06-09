#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"
#include "memory.h"
#include "bitmap.h"

// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;
// 程序管理器
ProgramManager programManager;
// 内存管理器
MemoryManager memoryManager;

// 用于测试的静态bitmap存储区域
// 32字节 = 256位，模拟256个资源单元
static char testBitmapStorage[32];

// 打印内存统计信息
void printStats(const char *label, BitMap &bm)
{
    printf("  [%s] Used=%d/%d  MaxFreeBlock=%d  Fragments=%d\n",
           label,
           bm.getUsedCount(), bm.size(),
           bm.getMaxFreeBlock(),
           bm.getFreeFragmentCount());
}

void first_thread(void *arg)
{
    // ============================================================
    // Assignment 2.1: 动态分区分配算法实现与对比
    // ============================================================
    printf("====================================================\n");
    printf("  Assignment 2: Dynamic Partition Allocation\n");
    printf("====================================================\n\n");

    // --------------------------------------------------------
    // 测试A: First Fit (首次适应) 与 Best Fit (最佳适应) 对比
    // --------------------------------------------------------
    printf("--- Part A: First Fit vs Best Fit Comparison ---\n\n");

    // 使用相同的分配序列分别在两个bitmap上测试
    // Bitmap 1: First Fit
    BitMap bmFF;
    static char ffStorage[32];
    bmFF.initialize(ffStorage, 256);

    // Bitmap 2: Best Fit
    BitMap bmBF;
    static char bfStorage[32];
    bmBF.initialize(bfStorage, 256);

    printf("Initial state:\n");
    printStats("FF", bmFF);
    printStats("BF", bmBF);

    // ===== 分配序列 (8次分配) =====
    printf("\n--- Allocation Sequence (8 allocations) ---\n\n");

    // Alloc 1: 10 pages
    int ff1 = bmFF.firstFitAllocate(10);
    int bf1 = bmBF.bestFitAllocate(10);
    printf("Alloc #1 (10 pages): FF=@%d, BF=@%d\n", ff1, bf1);
    printStats("FF", bmFF);
    printStats("BF", bmBF);

    // Alloc 2: 20 pages
    int ff2 = bmFF.firstFitAllocate(20);
    int bf2 = bmBF.bestFitAllocate(20);
    printf("\nAlloc #2 (20 pages): FF=@%d, BF=@%d\n", ff2, bf2);
    printStats("FF", bmFF);
    printStats("BF", bmBF);

    // Alloc 3: 5 pages
    int ff3 = bmFF.firstFitAllocate(5);
    int bf3 = bmBF.bestFitAllocate(5);
    printf("\nAlloc #3 (5 pages):  FF=@%d, BF=@%d\n", ff3, bf3);
    printStats("FF", bmFF);
    printStats("BF", bmBF);

    // Alloc 4: 15 pages
    int ff4 = bmFF.firstFitAllocate(15);
    int bf4 = bmBF.bestFitAllocate(15);
    printf("\nAlloc #4 (15 pages): FF=@%d, BF=@%d\n", ff4, bf4);
    printStats("FF", bmFF);
    printStats("BF", bmBF);

    // Alloc 5: 8 pages
    int ff5 = bmFF.firstFitAllocate(8);
    int bf5 = bmBF.bestFitAllocate(8);
    printf("\nAlloc #5 (8 pages):  FF=@%d, BF=@%d\n", ff5, bf5);
    printStats("FF", bmFF);
    printStats("BF", bmBF);

    // Alloc 6: 30 pages
    int ff6 = bmFF.firstFitAllocate(30);
    int bf6 = bmBF.bestFitAllocate(30);
    printf("\nAlloc #6 (30 pages): FF=@%d, BF=@%d\n", ff6, bf6);
    printStats("FF", bmFF);
    printStats("BF", bmBF);

    // Alloc 7: 3 pages
    int ff7 = bmFF.firstFitAllocate(3);
    int bf7 = bmBF.bestFitAllocate(3);
    printf("\nAlloc #7 (3 pages):  FF=@%d, BF=@%d\n", ff7, bf7);
    printStats("FF", bmFF);
    printStats("BF", bmBF);

    // Alloc 8: 12 pages
    int ff8 = bmFF.firstFitAllocate(12);
    int bf8 = bmBF.bestFitAllocate(12);
    printf("\nAlloc #8 (12 pages): FF=@%d, BF=@%d\n", ff8, bf8);
    printStats("FF", bmFF);
    printStats("BF", bmBF);

    // ===== 释放序列 (4次释放) =====
    printf("\n--- Release Sequence (4 releases) ---\n\n");

    // Release #1: 释放 Alloc 2 (20 pages)
    printf("Release #1: free Alloc#2 (20 pages @%d)\n", ff2);
    bmFF.release(ff2, 20);
    bmBF.release(bf2, 20);
    printStats("FF", bmFF);
    printStats("BF", bmBF);

    // Release #2: 释放 Alloc 4 (15 pages)
    printf("\nRelease #2: free Alloc#4 (15 pages @%d)\n", ff4);
    bmFF.release(ff4, 15);
    bmBF.release(bf4, 15);
    printStats("FF", bmFF);
    printStats("BF", bmBF);

    // Release #3: 释放 Alloc 6 (30 pages)
    printf("\nRelease #3: free Alloc#6 (30 pages @%d)\n", ff6);
    bmFF.release(ff6, 30);
    bmBF.release(bf6, 30);
    printStats("FF", bmFF);
    printStats("BF", bmBF);

    // Release #4: 释放 Alloc 8 (12 pages)
    printf("\nRelease #4: free Alloc#8 (12 pages @%d)\n", ff8);
    bmFF.release(ff8, 12);
    bmBF.release(bf8, 12);
    printStats("FF", bmFF);
    printStats("BF", bmBF);

    // ===== 在碎片化空间中重新分配 =====
    printf("\n--- Re-allocation in fragmented space ---\n\n");

    int rff1 = bmFF.firstFitAllocate(10);
    int rbf1 = bmBF.bestFitAllocate(10);
    printf("Realloc #1 (10 pages): FF=@%d, BF=@%d\n", rff1, rbf1);
    printf("  FF picks first adequate hole, BF picks smallest adequate hole\n");
    printStats("FF", bmFF);
    printStats("BF", bmBF);

    int rff2 = bmFF.firstFitAllocate(8);
    int rbf2 = bmBF.bestFitAllocate(8);
    printf("\nRealloc #2 (8 pages):  FF=@%d, BF=@%d\n", rff2, rbf2);
    printStats("FF", bmFF);
    printStats("BF", bmBF);

    printf("\n");

    // --------------------------------------------------------
    // 测试B: Worst Fit 和 Next Fit 单独测试
    // --------------------------------------------------------
    printf("--- Part B: Worst Fit and Next Fit Tests ---\n\n");

    // Worst Fit 测试
    BitMap bmWF;
    static char wfStorage[32];
    bmWF.initialize(wfStorage, 256);

    printf("[Worst Fit Test]\n");
    printStats("WF-init", bmWF);

    // 先分配一些制造碎片
    int w1 = bmWF.worstFitAllocate(20);
    printf("WF alloc 20 @%d\n", w1);
    int w2 = bmWF.worstFitAllocate(30);
    printf("WF alloc 30 @%d\n", w2);
    int w3 = bmWF.worstFitAllocate(10);
    printf("WF alloc 10 @%d\n", w3);
    printStats("WF-3alloc", bmWF);

    // 释放中间块制造空洞
    bmWF.release(w2, 30);
    printf("WF release 30 @%d\n", w2);
    printStats("WF-1free", bmWF);

    // Worst Fit应该选择最大的空闲块
    int w4 = bmWF.worstFitAllocate(15);
    printf("WF alloc 15 @%d (should pick largest free block)\n", w4);
    printStats("WF-final", bmWF);

    // Next Fit 测试
    printf("\n[Next Fit Test]\n");
    BitMap bmNF;
    static char nfStorage[32];
    bmNF.initialize(nfStorage, 256);
    printStats("NF-init", bmNF);

    int n1 = bmNF.nextFitAllocate(10);
    printf("NF alloc 10 @%d\n", n1);
    int n2 = bmNF.nextFitAllocate(20);
    printf("NF alloc 20 @%d\n", n2);
    int n3 = bmNF.nextFitAllocate(15);
    printf("NF alloc 15 @%d\n", n3);
    int n4 = bmNF.nextFitAllocate(8);
    printf("NF alloc 8  @%d\n", n4);
    printStats("NF-4alloc", bmNF);

    // 释放前两块
    bmNF.release(n1, 10);
    bmNF.release(n2, 20);
    printf("NF release @%d (10) and @%d (20)\n", n1, n2);
    printStats("NF-2free", bmNF);

    // Next Fit 从上次位置继续搜索
    int n5 = bmNF.nextFitAllocate(10);
    printf("NF alloc 10 @%d (search continues from last position)\n", n5);
    printStats("NF-final", bmNF);

    printf("\n");

    // --------------------------------------------------------
    // 测试C: 内存利用率分析
    // --------------------------------------------------------
    printf("--- Part C: Memory Utilization Analysis ---\n\n");

    BitMap bmAnalysis;
    static char analysisStorage[32];
    bmAnalysis.initialize(analysisStorage, 256);

    printf("Using First Fit for utilization analysis:\n\n");

    // 分配序列
    int a[12];
    a[0] = bmAnalysis.firstFitAllocate(10);
    printf("Step  1: alloc 10 @%d\n", a[0]);
    printStats("  ", bmAnalysis);

    a[1] = bmAnalysis.firstFitAllocate(20);
    printf("Step  2: alloc 20 @%d\n", a[1]);
    printStats("  ", bmAnalysis);

    a[2] = bmAnalysis.firstFitAllocate(15);
    printf("Step  3: alloc 15 @%d\n", a[2]);
    printStats("  ", bmAnalysis);

    a[3] = bmAnalysis.firstFitAllocate(8);
    printf("Step  4: alloc 8  @%d\n", a[3]);
    printStats("  ", bmAnalysis);

    a[4] = bmAnalysis.firstFitAllocate(25);
    printf("Step  5: alloc 25 @%d\n", a[4]);
    printStats("  ", bmAnalysis);

    a[5] = bmAnalysis.firstFitAllocate(12);
    printf("Step  6: alloc 12 @%d\n", a[5]);
    printStats("  ", bmAnalysis);

    a[6] = bmAnalysis.firstFitAllocate(5);
    printf("Step  7: alloc 5  @%d\n", a[6]);
    printStats("  ", bmAnalysis);

    a[7] = bmAnalysis.firstFitAllocate(18);
    printf("Step  8: alloc 18 @%d\n", a[7]);
    printStats("  ", bmAnalysis);

    // 释放序列
    printf("\n");
    bmAnalysis.release(a[1], 20);
    printf("Step  9: free 20 @%d\n", a[1]);
    printStats("  ", bmAnalysis);

    bmAnalysis.release(a[3], 8);
    printf("Step 10: free 8  @%d\n", a[3]);
    printStats("  ", bmAnalysis);

    bmAnalysis.release(a[5], 12);
    printf("Step 11: free 12 @%d\n", a[5]);
    printStats("  ", bmAnalysis);

    bmAnalysis.release(a[7], 18);
    printf("Step 12: free 18 @%d\n", a[7]);
    printStats("  ", bmAnalysis);

    printf("\n====================================================\n");
    printf("  Assignment 2 Complete!\n");
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
