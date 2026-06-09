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
    // Assignment 3.1: 虚拟页内存管理 - 分配、释放、重新分配
    // ============================================================
    printf("====================================================\n");
    printf("  Assignment 3.1: Virtual Page Memory Management\n");
    printf("====================================================\n\n");

    // 分配100页虚拟内存 (Batch 1)
    printf("[Step 1] Allocate 100 kernel virtual pages (Batch1):\n");
    int p1 = memoryManager.allocatePages(AddressPoolType::KERNEL, 100);
    printf("  Batch1: start=0x%x, end=0x%x (100 pages)\n",
           p1, p1 + 99 * PAGE_SIZE);

    // 分配10页虚拟内存 (Batch 2)
    printf("\n[Step 2] Allocate 10 kernel virtual pages (Batch2):\n");
    int p2 = memoryManager.allocatePages(AddressPoolType::KERNEL, 10);
    printf("  Batch2: start=0x%x, end=0x%x (10 pages)\n",
           p2, p2 + 9 * PAGE_SIZE);

    // 分配100页虚拟内存 (Batch 3)
    printf("\n[Step 3] Allocate 100 kernel virtual pages (Batch3):\n");
    int p3 = memoryManager.allocatePages(AddressPoolType::KERNEL, 100);
    printf("  Batch3: start=0x%x, end=0x%x (100 pages)\n",
           p3, p3 + 99 * PAGE_SIZE);

    // 验证三批地址的连续性
    printf("\nAddress layout verification:\n");
    printf("  Batch1: 0x%x ~ 0x%x\n", p1, p1 + 100 * PAGE_SIZE - 1);
    printf("  Batch2: 0x%x ~ 0x%x\n", p2, p2 + 10 * PAGE_SIZE - 1);
    printf("  Batch3: 0x%x ~ 0x%x\n", p3, p3 + 100 * PAGE_SIZE - 1);

    // 释放中间的10页 (Batch 2)
    printf("\n[Step 4] Release Batch2 (10 pages @0x%x):\n", p2);
    memoryManager.releasePages(AddressPoolType::KERNEL, p2, 10);
    printf("  Released successfully.\n");

    // 重新分配10页 — 应该复用Batch2的虚拟地址
    printf("\n[Step 5] Re-allocate 10 pages:\n");
    int p4 = memoryManager.allocatePages(AddressPoolType::KERNEL, 10);
    printf("  New alloc: start=0x%x\n", p4);
    printf("  Old Batch2 was at: 0x%x\n", p2);
    printf("  Address reuse? %s\n", p4 == p2 ? "YES (virtual addresses reused!)" : "NO");

    // 再分配100页
    printf("\n[Step 6] Allocate another 100 pages:\n");
    int p5 = memoryManager.allocatePages(AddressPoolType::KERNEL, 100);
    printf("  start=0x%x\n", p5);

    // 验证虚拟地址到物理地址的映射
    printf("\nVirtual-to-Physical address mapping samples:\n");
    if (p1)
    {
        int vaddr = p1;
        int paddr = memoryManager.vaddr2paddr(vaddr);
        printf("  vaddr=0x%x -> paddr=0x%x\n", vaddr, paddr);
    }
    if (p4)
    {
        int vaddr = p4;
        int paddr = memoryManager.vaddr2paddr(vaddr);
        printf("  vaddr=0x%x -> paddr=0x%x\n", vaddr, paddr);
    }

    printf("\n");

    // ============================================================
    // Assignment 3.2: PDE/PTE 虚拟地址构造推导
    // ============================================================
    printf("====================================================\n");
    printf("  Assignment 3.2: PDE/PTE Address Derivation\n");
    printf("====================================================\n\n");

    printf("Current implementation uses entry 1023 (0x3FF) for self-reference.\n");
    printf("  toPDE(v) = 0xFFFFF000 + ((v>>20) & 0xFFC)\n");
    printf("  toPTE(v) = 0xFFC00000 + ((v>>10) & 0xFFC00) + ((v>>10) & 0xFFC)\n\n");

    printf("--- Scenario: Self-ref moved to entry 1000 (0x3E8) ---\n\n");

    // 推导公式
    printf("Derivation formulas when self-ref at entry S=1000:\n");
    printf("  PDE(N) = (S << 22) | (S << 12) | (4 * N)\n");
    printf("         = 0x%x | 0x%x | (4 * N)\n", 0x3E8 << 22, 0x3E8 << 12);
    printf("  PTE(PD, PT) = (S << 22) | (PD << 12) | (4 * PT)\n\n");

    // 计算第141个页目录项的虚拟地址
    int S = 0x3E8; // 1000
    int pde_141 = (S << 22) | (S << 12) | (4 * 141);
    printf("Q1: Virtual address of PDE #141:\n");
    printf("  PDE[141] = (1000 << 22) | (1000 << 12) | (4 * 141)\n");
    printf("           = 0x%x | 0x%x | 0x%x\n",
           S << 22, S << 12, 4 * 141);
    printf("           = 0x%x\n\n", pde_141);

    // 验证: CPU如何解析这个虚拟地址
    printf("  CPU translation of this virtual address:\n");
    printf("    bits[31:22] = %d -> PDE index = %d (self-ref, points to page dir)\n",
           (pde_141 >> 22) & 0x3FF, (pde_141 >> 22) & 0x3FF);
    printf("    bits[21:12] = %d -> PTE index = %d (self-ref again, points to page dir)\n",
           (pde_141 >> 12) & 0x3FF, (pde_141 >> 12) & 0x3FF);
    printf("    bits[11:0]  = 0x%x -> offset in page = entry #%d * 4 bytes\n",
           pde_141 & 0xFFF, 141);

    // 计算第891个页目录项指向的页表中第109个页表项的虚拟地址
    printf("\nQ2: Virtual address of PTE #109 in page table pointed to by PDE #891:\n");
    int pte_891_109 = (S << 22) | (891 << 12) | (4 * 109);
    printf("  PTE[891][109] = (1000 << 22) | (891 << 12) | (4 * 109)\n");
    printf("                = 0x%x | 0x%x | 0x%x\n",
           S << 22, 891 << 12, 4 * 109);
    printf("                = 0x%x\n\n", pte_891_109);

    printf("  CPU translation of this virtual address:\n");
    printf("    bits[31:22] = %d -> PDE index = %d (self-ref, points to page dir)\n",
           (pte_891_109 >> 22) & 0x3FF, (pte_891_109 >> 22) & 0x3FF);
    printf("    bits[21:12] = %d -> PTE index = %d (PDE #891's page table)\n",
           (pte_891_109 >> 12) & 0x3FF, (pte_891_109 >> 12) & 0x3FF);
    printf("    bits[11:0]  = 0x%x -> offset = entry #%d * 4 bytes\n",
           pte_891_109 & 0xFFF, 109);

    printf("\n");

    // ============================================================
    // Assignment 3.3: 虚拟地址到物理地址的验证
    // ============================================================
    printf("====================================================\n");
    printf("  Assignment 3.3: Write & Verify (0xDEADBEEF)\n");
    printf("====================================================\n\n");

    // 分配1页虚拟内存
    printf("[Step 1] Allocate 1 kernel virtual page:\n");
    int testPage = memoryManager.allocatePages(AddressPoolType::KERNEL, 1);
    printf("  Virtual address: 0x%x\n", testPage);

    // 获取对应的物理地址
    int physAddr = memoryManager.vaddr2paddr(testPage);
    printf("  Physical address: 0x%x\n", physAddr);

    // 验证虚拟地址和物理地址不同(映射关系)
    printf("  vaddr != paddr? %s (mapping is working)\n",
           testPage != physAddr ? "YES" : "NO (identity, also OK for low memory)");

    // 向虚拟地址写入 0xDEADBEEF
    printf("\n[Step 2] Write 0xDEADBEEF to virtual address 0x%x:\n", testPage);
    *(int *)testPage = 0xDEADBEEF;

    // 通过虚拟地址读回验证
    int readback = *(int *)testPage;
    printf("  Read back from virtual: 0x%x %s\n",
           readback, readback == (int)0xDEADBEEF ? "(correct!)" : "(ERROR!)");

    // 通过物理地址读回验证(由于0~1MB是恒等映射，只有当物理地址也在映射范围内才可直接访问)
    printf("\n[Step 3] Verification via physical address:\n");
    printf("  Physical address: 0x%x\n", physAddr);
    printf("  (Note: direct physical read only works for identity-mapped region)\n");

    printf("\n[Step 4] Use QEMU Monitor to verify:\n");
    printf("  Run: make monitor\n");
    printf("  Then: telnet 127.0.0.1 45475\n");
    printf("  Then type: xp /1wx 0x%x\n", physAddr);
    printf("  Expected output: 0x%x: 0xdeadbeef\n", physAddr);

    // 额外测试: 多页写入验证
    printf("\n[Extra] Multi-page write test:\n");
    int multiPage = memoryManager.allocatePages(AddressPoolType::KERNEL, 3);
    if (multiPage)
    {
        for (int i = 0; i < 3; ++i)
        {
            int vaddr = multiPage + i * PAGE_SIZE;
            int paddr = memoryManager.vaddr2paddr(vaddr);
            *(int *)vaddr = 0xCAFE0000 + i;
            int readVal = *(int *)vaddr;
            printf("  Page %d: vaddr=0x%x paddr=0x%x wrote=0x%x read=0x%x %s\n",
                   i, vaddr, paddr, 0xCAFE0000 + i, readVal,
                   readVal == 0xCAFE0000 + i ? "OK" : "FAIL");
        }
    }

    printf("\n====================================================\n");
    printf("  Assignment 3 Complete!\n");
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
