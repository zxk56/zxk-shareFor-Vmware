#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"
#include "memory.h"
#include "os_constant.h"

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
    // Assignment 1.1: 物理页内存管理测试
    // ============================================================
    printf("====================================================\n");
    printf("  Assignment 1.1: Physical Page Memory Management\n");
    printf("====================================================\n\n");

    // --- 测试1: 从内核物理地址池分配10页 ---
    printf("[Test 1] Allocate 10 pages from KERNEL pool:\n");
    int kernelAddr1[10];
    for (int i = 0; i < 10; ++i)
    {
        kernelAddr1[i] = memoryManager.allocatePhysicalPages(AddressPoolType::KERNEL, 1);
        printf("  page[%d] = 0x%x\n", i, kernelAddr1[i]);
    }

    // --- 测试2: 从用户物理地址池分配20页 ---
    printf("\n[Test 2] Allocate 20 pages from USER pool:\n");
    int userAddr1[20];
    for (int i = 0; i < 20; ++i)
    {
        userAddr1[i] = memoryManager.allocatePhysicalPages(AddressPoolType::USER, 1);
        printf("  page[%d] = 0x%x\n", i, userAddr1[i]);
    }

    // --- 测试3: 从内核物理地址池分配50页 ---
    printf("\n[Test 3] Allocate 50 pages from KERNEL pool:\n");
    int kernelAddr2[50];
    for (int i = 0; i < 50; ++i)
    {
        kernelAddr2[i] = memoryManager.allocatePhysicalPages(AddressPoolType::KERNEL, 1);
    }
    printf("  first page = 0x%x\n", kernelAddr2[0]);
    printf("  last  page = 0x%x\n", kernelAddr2[49]);
    printf("  (50 pages allocated successfully)\n");

    // --- 测试4: 释放部分页后重新分配，验证空间复用 ---
    printf("\n[Test 4] Release and re-allocate to verify reuse:\n");

    // 释放内核池中的偶数编号页 (page 0, 2, 4, 6, 8)
    printf("  Releasing kernel pages [0,2,4,6,8]...\n");
    for (int i = 0; i < 10; i += 2)
    {
        memoryManager.releasePhysicalPages(AddressPoolType::KERNEL, kernelAddr1[i], 1);
        printf("    released 0x%x\n", kernelAddr1[i]);
    }

    // 重新分配5页，应该复用刚才释放的地址
    printf("  Re-allocating 5 pages from KERNEL pool:\n");
    int reusedAddr[5];
    for (int i = 0; i < 5; ++i)
    {
        reusedAddr[i] = memoryManager.allocatePhysicalPages(AddressPoolType::KERNEL, 1);
        printf("    reused[%d] = 0x%x (expected 0x%x) %s\n",
               i, reusedAddr[i], kernelAddr1[i * 2],
               reusedAddr[i] == kernelAddr1[i * 2] ? "MATCH!" : "DIFFERENT");
    }

    // 释放用户池中的前10页
    printf("\n  Releasing user pages [0..9]...\n");
    for (int i = 0; i < 10; ++i)
    {
        memoryManager.releasePhysicalPages(AddressPoolType::USER, userAddr1[i], 1);
    }

    // 重新分配10页用户空间
    printf("  Re-allocating 10 pages from USER pool:\n");
    int reusedUser[10];
    for (int i = 0; i < 10; ++i)
    {
        reusedUser[i] = memoryManager.allocatePhysicalPages(AddressPoolType::USER, 1);
        printf("    reused[%d] = 0x%x (expected 0x%x) %s\n",
               i, reusedUser[i], userAddr1[i],
               reusedUser[i] == userAddr1[i] ? "MATCH!" : "DIFFERENT");
    }

    // --- 测试5: 连续分配测试 ---
    printf("\n[Test 5] Batch contiguous allocation:\n");
    int batchAddr = memoryManager.allocatePhysicalPages(AddressPoolType::KERNEL, 5);
    printf("  Allocate 5 contiguous kernel pages: start = 0x%x\n", batchAddr);
    if (batchAddr)
    {
        for (int i = 0; i < 5; ++i)
        {
            printf("    page[%d] = 0x%x\n", i, batchAddr + i * PAGE_SIZE);
        }
    }
    memoryManager.releasePhysicalPages(AddressPoolType::KERNEL, batchAddr, 5);
    printf("  Released 5 contiguous pages.\n");

    printf("\n");

    // ============================================================
    // Assignment 1.2: 二级分页机制验证
    // ============================================================
    printf("====================================================\n");
    printf("  Assignment 1.2: Paging Mechanism Verification\n");
    printf("====================================================\n\n");

    // 验证页目录表内容
    // 页目录表在物理地址 PAGE_DIRECTORY (0x100000)
    // 由于0~1MB是恒等映射，虚拟地址=物理地址，可以直接访问
    int *directory = (int *)PAGE_DIRECTORY;
    int *pageTable = (int *)(PAGE_DIRECTORY + PAGE_SIZE);

    printf("Page Directory at physical address 0x%x:\n", PAGE_DIRECTORY);
    printf("  directory[0]    = 0x%x\n", directory[0]);
    printf("    -> points to page table, P=1, R/W=1, U/S=1\n");
    printf("  directory[768]  = 0x%x\n", directory[768]);
    printf("    -> 3GB kernel space mapping, should == directory[0]\n");
    printf("  directory[1023] = 0x%x\n", directory[1023]);
    printf("    -> self-reference to page directory at 0x%x\n", PAGE_DIRECTORY);

    // 验证 directory[768] == directory[0]
    printf("\nVerification:\n");
    printf("  directory[768] == directory[0]? %s\n",
           directory[768] == directory[0] ? "YES (correct)" : "NO (error!)");

    // 验证 directory[1023] 指向页目录表本身
    printf("  directory[1023] & 0xfffff000 == 0x%x? %s\n",
           PAGE_DIRECTORY,
           (directory[1023] & 0xfffff000) == PAGE_DIRECTORY ? "YES (correct)" : "NO (error!)");

    // 验证页表内容 (0~1MB恒等映射)
    printf("\nPage Table at 0x%x (identity mapping 0~1MB):\n", (int)pageTable);
    for (int i = 0; i < 10; ++i)
    {
        printf("  pte[%d] = 0x%x  (physical addr: 0x%x) %s\n",
               i, pageTable[i], pageTable[i] & 0xfffff000,
               (pageTable[i] & 0xfffff000) == (i * PAGE_SIZE) ? "OK" : "ERROR");
    }

    // 验证恒等映射的关键条目
    printf("\nIdentity mapping key entries:\n");
    printf("  pte[0]   -> physical 0x%x (expect 0x0)\n", pageTable[0] & 0xfffff000);
    printf("  pte[1]   -> physical 0x%x (expect 0x1000)\n", pageTable[1] & 0xfffff000);
    printf("  pte[255] -> physical 0x%x (expect 0xFF000)\n", pageTable[255] & 0xfffff000);

    printf("\n====================================================\n");
    printf("  Assignment 1 Complete!\n");
    printf("====================================================\n");

    printf("\n[Tip] Use QEMU Monitor to further verify:\n");
    printf("  make monitor\n");
    printf("  telnet 127.0.0.1 45474\n");
    printf("  info mem\n");
    printf("  xp /4wx 0x100000\n");
    printf("  xp /256wx 0x101000\n");

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
    // 第一步：开启分页机制（Assignment 1.2 的三步方案）
    memoryManager.openPageMechanism();
    // 第二步：初始化物理地址池
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
