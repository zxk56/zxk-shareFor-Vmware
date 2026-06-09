#ifndef MEMORY_H
#define MEMORY_H

#include "address_pool.h"

enum AddressPoolType
{
    USER,
    KERNEL
};

// 页面置换模拟的最大帧数和页表项数
#define SIM_MAX_FRAMES 32
#define SIM_TABLE_SIZE 128

class MemoryManager
{
public:
    // 可管理的内存容量
    int totalMemory;
    // 内核物理地址池
    AddressPool kernelPhysical;
    // 用户物理地址池
    AddressPool userPhysical;
    // 内核虚拟地址池
    AddressPool kernelVirtual;

    // ====== Assignment 4: 页面置换模拟 ======
    // 配置的物理帧总数
    int numFrames;
    // 物理帧中存放的虚拟页号 (-1表示空闲)
    int frameVirtualPage[SIM_MAX_FRAMES];
    // 已使用的物理帧数量
    int usedFrames;
    // FIFO队列头部指针
    int fifoHead;
    // 模拟页表: 虚拟页号 -> 物理帧号 (-1表示不在内存)
    int simPageTable[SIM_TABLE_SIZE];
    // 总访问次数
    int simAccessCount;
    // 缺页次数
    int simFaultCount;
    // 置换次数
    int simEvictionCount;

public:
    MemoryManager();

    // 初始化地址池
    void initialize();

    // 从type类型的物理地址池中分配count个连续的页
    // 成功，返回起始地址；失败，返回0
    int allocatePhysicalPages(enum AddressPoolType type, const int count);

    // 释放从paddr开始的count个物理页
    void releasePhysicalPages(enum AddressPoolType type, const int startAddress, const int count);

    // 获取内存总容量
    int getTotalMemory();

    // 开启分页机制
    void openPageMechanism();

    // 页内存分配
    int allocatePages(enum AddressPoolType type, const int count);

    // 虚拟页分配
    int allocateVirtualPages(enum AddressPoolType type, const int count);

    // 建立虚拟页到物理页的联系
    bool connectPhysicalVirtualPage(const int virtualAddress, const int physicalPageAddress);

    // 计算virtualAddress的页目录项的虚拟地址
    int toPDE(const int virtualAddress);

    // 计算virtualAddress的页表项的虚拟地址
    int toPTE(const int virtualAddress);

    // 页内存释放
    void releasePages(enum AddressPoolType type, const int virtualAddress, const int count);

    // 找到虚拟地址对应的物理地址
    int vaddr2paddr(int vaddr);

    // 释放虚拟页
    void releaseVirtualPages(enum AddressPoolType type, const int vaddr, const int count);

    // ====== Assignment 4: 页面置换模拟函数 ======

    // 初始化FIFO页面置换模拟
    // numFrames: 可用的物理帧数量
    void initPageReplacement(int numFrames);

    // 模拟访问一个虚拟页
    // virtualPageNum: 虚拟页号 (不是地址，是页号)
    // 返回: 0=命中, 1=缺页(无置换), 2=缺页(有置换)
    int accessPage(int virtualPageNum);

    // FIFO淘汰算法: 返回被淘汰的帧号
    int fifoEvict();

    // 打印当前帧状态
    void printFrameStatus();

    // 打印统计信息
    void printStatistics();
};

#endif
