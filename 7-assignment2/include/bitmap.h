#ifndef BITMAP_H
#define BITMAP_H

#include "os_type.h"

class BitMap
{
public:
    // 被管理的资源个数，bitmap的总位数
    int length;
    // bitmap的起始地址
    char *bitmap;
    // Next Fit算法的搜索起始位置
    int nextFitIndex;
public:
    // 初始化
    BitMap();
    // 设置BitMap，bitmap=起始地址，length=总位数(即被管理的资源个数)
    void initialize(char *bitmap, const int length);
    // 获取第index个资源的状态，true=allocated，false=free
    bool get(const int index) const;
    // 设置第index个资源的状态，true=allocated，false=free
    void set(const int index, const bool status);
    // 分配count个连续的资源(默认首次适应)，若没有则返回-1，否则返回分配的第1个资源单元序号
    int allocate(const int count);
    // 释放第index个资源开始的count个资源
    void release(const int index, const int count);
    // 返回Bitmap存储区域
    char *getBitmap();
    // 返回Bitmap的大小
    int size() const;

    // ====== Assignment 2: 动态分区分配算法 ======

    // 首次适应算法(First Fit): 从头开始找到第一个足够大的空闲分区
    int firstFitAllocate(const int count);
    // 最佳适应算法(Best Fit): 找到最小的足够大的空闲分区
    int bestFitAllocate(const int count);
    // 最坏适应算法(Worst Fit): 找到最大的空闲分区
    int worstFitAllocate(const int count);
    // 循环首次适应算法(Next Fit): 从上次分配位置开始搜索
    int nextFitAllocate(const int count);

    // ====== 内存统计函数 ======

    // 返回已分配的资源数量
    int getUsedCount() const;
    // 返回最大连续空闲块的大小(资源个数)
    int getMaxFreeBlock() const;
    // 返回不连续的空闲块个数(碎片数量)
    int getFreeFragmentCount() const;

private:
    // 禁止Bitmap之间的赋值
    BitMap(const BitMap &) {}
    void operator=(const BitMap&) {}
};

#endif
