#include "memory.h"
#include "os_constant.h"
#include "stdlib.h"
#include "asm_utils.h"
#include "stdio.h"
#include "program.h"
#include "os_modules.h"

MemoryManager::MemoryManager()
{
    initialize();
}

void MemoryManager::initialize()
{
    this->totalMemory = 0;
    this->totalMemory = getTotalMemory();

    // 预留的内存
    int usedMemory = 256 * PAGE_SIZE + 0x100000;
    if (this->totalMemory < usedMemory)
    {
        printf("memory is too small, halt.\n");
        asm_halt();
    }
    // 剩余的空闲的内存
    int freeMemory = this->totalMemory - usedMemory;

    int freePages = freeMemory / PAGE_SIZE;
    int kernelPages = freePages / 2;
    int userPages = freePages - kernelPages;

    int kernelPhysicalStartAddress = usedMemory;
    int userPhysicalStartAddress = usedMemory + kernelPages * PAGE_SIZE;

    int kernelPhysicalBitMapStart = BITMAP_START_ADDRESS;
    int userPhysicalBitMapStart = kernelPhysicalBitMapStart + ceil(kernelPages, 8);
    int kernelVirtualBitMapStart = userPhysicalBitMapStart + ceil(userPages, 8);

    kernelPhysical.initialize(
        (char *)kernelPhysicalBitMapStart,
        kernelPages,
        kernelPhysicalStartAddress);

    userPhysical.initialize(
        (char *)userPhysicalBitMapStart,
        userPages,
        userPhysicalStartAddress);

    kernelVirtual.initialize(
        (char *)kernelVirtualBitMapStart,
        kernelPages,
        KERNEL_VIRTUAL_START);

    printf("total memory: %d bytes ( %d MB )\n",
           this->totalMemory,
           this->totalMemory / 1024 / 1024);

    printf("kernel pool\n"
           "    start address: 0x%x\n"
           "    total pages: %d ( %d MB )\n"
           "    bitmap start address: 0x%x\n",
           kernelPhysicalStartAddress,
           kernelPages, kernelPages * PAGE_SIZE / 1024 / 1024,
           kernelPhysicalBitMapStart);

    printf("user pool\n"
           "    start address: 0x%x\n"
           "    total pages: %d ( %d MB )\n"
           "    bit map start address: 0x%x\n",
           userPhysicalStartAddress,
           userPages, userPages * PAGE_SIZE / 1024 / 1024,
           userPhysicalBitMapStart);

    printf("kernel virtual pool\n"
           "    start address: 0x%x\n"
           "    total pages: %d  ( %d MB ) \n"
           "    bit map start address: 0x%x\n",
           KERNEL_VIRTUAL_START,
           userPages, kernelPages * PAGE_SIZE / 1024 / 1024,
           kernelVirtualBitMapStart);

    // 初始化页面置换模拟数据
    initPageReplacement(SIM_MAX_FRAMES);
}

int MemoryManager::allocatePhysicalPages(enum AddressPoolType type, const int count)
{
    int start = -1;

    if (type == AddressPoolType::KERNEL)
    {
        start = kernelPhysical.allocate(count);
    }
    else if (type == AddressPoolType::USER)
    {
        start = userPhysical.allocate(count);
    }

    return (start == -1) ? 0 : start;
}

void MemoryManager::releasePhysicalPages(enum AddressPoolType type, const int paddr, const int count)
{
    if (type == AddressPoolType::KERNEL)
    {
        kernelPhysical.release(paddr, count);
    }
    else if (type == AddressPoolType::USER)
    {

        userPhysical.release(paddr, count);
    }
}

int MemoryManager::getTotalMemory()
{

    if (!this->totalMemory)
    {
        int memory = *((int *)MEMORY_SIZE_ADDRESS);
        // ax寄存器保存的内容
        int low = memory & 0xffff;
        // bx寄存器保存的内容
        int high = (memory >> 16) & 0xffff;

        this->totalMemory = low * 1024 + high * 64 * 1024;
    }

    return this->totalMemory;
}

void MemoryManager::openPageMechanism()
{
    // 页目录表指针
    int *directory = (int *)PAGE_DIRECTORY;
    //线性地址0~4MB对应的页表
    int *page = (int *)(PAGE_DIRECTORY + PAGE_SIZE);

    // 初始化页目录表
    memset(directory, 0, PAGE_SIZE);
    // 初始化线性地址0~4MB对应的页表
    memset(page, 0, PAGE_SIZE);

    int address = 0;
    // 将线性地址0~1MB恒等映射到物理地址0~1MB
    for (int i = 0; i < 256; ++i)
    {
        // U/S = 1, R/W = 1, P = 1
        page[i] = address | 0x7;
        address += PAGE_SIZE;
    }

    // 初始化页目录项

    // 0~1MB
    directory[0] = ((int)page) | 0x07;
    // 3GB的内核空间
    directory[768] = directory[0];
    // 最后一个页目录项指向页目录表
    directory[1023] = ((int)directory) | 0x7;

    // 初始化cr3，cr0，开启分页机制
    asm_init_page_reg(directory);

    printf("open page mechanism\n");
}

int MemoryManager::allocatePages(enum AddressPoolType type, const int count)
{
    // 第一步：从虚拟地址池中分配若干虚拟页
    int virtualAddress = allocateVirtualPages(type, count);
    if (!virtualAddress)
    {
        return 0;
    }

    bool flag;
    int physicalPageAddress;
    int vaddress = virtualAddress;

    // 依次为每一个虚拟页指定物理页
    for (int i = 0; i < count; ++i, vaddress += PAGE_SIZE)
    {
        flag = false;
        // 第二步：从物理地址池中分配一个物理页
        physicalPageAddress = allocatePhysicalPages(type, 1);
        if (physicalPageAddress)
        {
            // 第三步：为虚拟页建立页目录项和页表项，使虚拟页内的地址经过分页机制变换到物理页内。
            flag = connectPhysicalVirtualPage(vaddress, physicalPageAddress);
        }
        else
        {
            flag = false;
        }

        // 分配失败，释放前面已经分配的虚拟页和物理页表
        if (!flag)
        {
            // 前i个页表已经指定了物理页
            releasePages(type, virtualAddress, i);
            // 剩余的页表未指定物理页
            releaseVirtualPages(type, virtualAddress + i * PAGE_SIZE, count - i);
            return 0;
        }
    }

    return virtualAddress;
}

int MemoryManager::allocateVirtualPages(enum AddressPoolType type, const int count)
{
    int start = -1;

    if (type == AddressPoolType::KERNEL)
    {
        start = kernelVirtual.allocate(count);
    }

    return (start == -1) ? 0 : start;
}

bool MemoryManager::connectPhysicalVirtualPage(const int virtualAddress, const int physicalPageAddress)
{
    // 计算虚拟地址对应的页目录项和页表项
    int *pde = (int *)toPDE(virtualAddress);
    int *pte = (int *)toPTE(virtualAddress);

    // 页目录项无对应的页表，先分配一个页表
    if (!(*pde & 0x00000001))
    {
        // 从内核物理地址空间中分配一个页表
        int page = allocatePhysicalPages(AddressPoolType::KERNEL, 1);
        if (!page)
            return false;

        // 使页目录项指向页表
        *pde = page | 0x7;
        // 初始化页表
        char *pagePtr = (char *)(((int)pte) & 0xfffff000);
        memset(pagePtr, 0, PAGE_SIZE);
    }

    // 使页表项指向物理页
    *pte = physicalPageAddress | 0x7;

    return true;
}

int MemoryManager::toPDE(const int virtualAddress)
{
    return (0xfffff000 + (((virtualAddress & 0xffc00000) >> 22) * 4));
}

int MemoryManager::toPTE(const int virtualAddress)
{
    return (0xffc00000 + ((virtualAddress & 0xffc00000) >> 10) + (((virtualAddress & 0x003ff000) >> 12) * 4));
}

void MemoryManager::releasePages(enum AddressPoolType type, const int virtualAddress, const int count)
{
    int vaddr = virtualAddress;
    int *pte;
    for (int i = 0; i < count; ++i, vaddr += PAGE_SIZE)
    {
        // 第一步，对每一个虚拟页，释放为其分配的物理页
        releasePhysicalPages(type, vaddr2paddr(vaddr), 1);

        // 设置页表项为不存在，防止释放后被再次使用
        pte = (int *)toPTE(vaddr);
        *pte = 0;
    }

    // 第二步，释放虚拟页
    releaseVirtualPages(type, virtualAddress, count);
}

int MemoryManager::vaddr2paddr(int vaddr)
{
    int *pte = (int *)toPTE(vaddr);
    int page = (*pte) & 0xfffff000;
    int offset = vaddr & 0xfff;
    return (page + offset);
}

void MemoryManager::releaseVirtualPages(enum AddressPoolType type, const int vaddr, const int count)
{
    if (type == AddressPoolType::KERNEL)
    {
        kernelVirtual.release(vaddr, count);
    }
}

// ============================================================
// Assignment 4: FIFO 页面置换算法模拟
// ============================================================

// 初始化FIFO页面置换模拟
void MemoryManager::initPageReplacement(int frames)
{
    if (frames > SIM_MAX_FRAMES)
        frames = SIM_MAX_FRAMES;

    numFrames = frames;
    usedFrames = 0;
    fifoHead = 0;
    simAccessCount = 0;
    simFaultCount = 0;
    simEvictionCount = 0;

    for (int i = 0; i < SIM_MAX_FRAMES; ++i)
    {
        frameVirtualPage[i] = -1; // -1 表示空闲
    }
    for (int i = 0; i < SIM_TABLE_SIZE; ++i)
    {
        simPageTable[i] = -1; // -1 表示不在内存中
    }
}

// FIFO淘汰算法: 淘汰队列头部的帧，循环前进
int MemoryManager::fifoEvict()
{
    int evictFrame = fifoHead;
    fifoHead = (fifoHead + 1) % numFrames;
    return evictFrame;
}

// 模拟访问一个虚拟页
int MemoryManager::accessPage(int virtualPageNum)
{
    if (virtualPageNum < 0 || virtualPageNum >= SIM_TABLE_SIZE)
    {
        printf("  ERROR: page number %d out of range!\n", virtualPageNum);
        return -1;
    }

    ++simAccessCount;

    // 检查是否命中 (页已在物理帧中)
    if (simPageTable[virtualPageNum] != -1)
    {
        // 命中
        printf("  [#%d] Page %d -> HIT (frame #%d)\n",
               simAccessCount, virtualPageNum, simPageTable[virtualPageNum]);
        return 0;
    }

    // 缺页
    ++simFaultCount;

    if (usedFrames < numFrames)
    {
        // 还有空闲帧，直接加载
        int newFrame = usedFrames; // 按顺序使用空闲帧
        ++usedFrames;

        frameVirtualPage[newFrame] = virtualPageNum;
        simPageTable[virtualPageNum] = newFrame;

        printf("  [#%d] Page %d -> FAULT (load into free frame #%d)\n",
               simAccessCount, virtualPageNum, newFrame);
        return 1;
    }
    else
    {
        // 无空闲帧，需要FIFO淘汰
        int evictFrame = fifoEvict();
        int evictedPage = frameVirtualPage[evictFrame];

        // 更新被淘汰页的页表项
        if (evictedPage != -1)
        {
            simPageTable[evictedPage] = -1;
        }

        // 将新页放入被淘汰的帧
        frameVirtualPage[evictFrame] = virtualPageNum;
        simPageTable[virtualPageNum] = evictFrame;

        ++simEvictionCount;

        printf("  [#%d] Page %d -> FAULT (evict page %d from frame #%d)\n",
               simAccessCount, virtualPageNum, evictedPage, evictFrame);
        return 2;
    }
}

// 打印当前帧状态
void MemoryManager::printFrameStatus()
{
    printf("  Frames (%d/%d used): [", usedFrames, numFrames);
    for (int i = 0; i < numFrames; ++i)
    {
        if (i > 0)
            printf(", ");
        if (frameVirtualPage[i] == -1)
            printf("_");
        else
            printf("%d", frameVirtualPage[i]);
    }
    printf("]  FIFO head=%d\n", fifoHead);
}

// 打印统计信息
void MemoryManager::printStatistics()
{
    printf("\n--- FIFO Page Replacement Statistics ---\n");
    printf("  Total physical frames: %d\n", numFrames);
    printf("  Total accesses:        %d\n", simAccessCount);
    printf("  Page faults:           %d\n", simFaultCount);
    printf("  Page evictions:        %d\n", simEvictionCount);

    if (simAccessCount > 0)
    {
        int hitCount = simAccessCount - simFaultCount;
        printf("  Hit count:             %d\n", hitCount);
        printf("  Fault rate:            %d%%\n",
               (simFaultCount * 100) / simAccessCount);
        printf("  Hit rate:              %d%%\n",
               (hitCount * 100) / simAccessCount);
    }
    printf("----------------------------------------\n");
}
