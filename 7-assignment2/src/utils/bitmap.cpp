#include "bitmap.h"
#include "stdlib.h"
#include "stdio.h"

BitMap::BitMap()
{
}

void BitMap::initialize(char *bitmap, const int length)
{
    this->bitmap = bitmap;
    this->length = length;
    this->nextFitIndex = 0;

    int bytes = ceil(length, 8);

    for (int i = 0; i < bytes; ++i)
    {
        bitmap[i] = 0;
    }
}

bool BitMap::get(const int index) const
{
    int pos = index / 8;
    int offset = index % 8;

    return (bitmap[pos] & (1 << offset));
}

void BitMap::set(const int index, const bool status)
{
    int pos = index / 8;
    int offset = index % 8;

    // 清0
    bitmap[pos] = bitmap[pos] & (~(1 << offset));

    // 置1
    if (status)
    {
        bitmap[pos] = bitmap[pos] | (1 << offset);
    }
}

int BitMap::allocate(const int count)
{
    if (count == 0)
        return -1;

    int index, empty, start;

    index = 0;
    while (index < length)
    {
        // 越过已经分配的资源
        while (index < length && get(index))
            ++index;

        // 不存在连续的count个资源
        if (index == length)
            return -1;

        // 找到1个未分配的资源
        // 检查是否存在从index开始的连续count个资源
        empty = 0;
        start = index;
        while ((index < length) && (!get(index)) && (empty < count))
        {
            ++empty;
            ++index;
        }

        // 存在连续的count个资源
        if (empty == count)
        {
            for (int i = 0; i < count; ++i)
            {
                set(start + i, true);
            }

            return start;
        }
    }

    return -1;
}

void BitMap::release(const int index, const int count)
{
    for (int i = 0; i < count; ++i)
    {
        set(index + i, false);
    }
}

char *BitMap::getBitmap()
{
    return (char *)bitmap;
}

int BitMap::size() const
{
    return length;
}

// ============================================================
// Assignment 2: 动态分区分配算法实现
// ============================================================

// 首次适应算法 (First Fit)
// 从头部开始扫描，找到第一个足够大的连续空闲分区
int BitMap::firstFitAllocate(const int count)
{
    if (count <= 0)
        return -1;

    int consecutive = 0;
    int start = -1;

    for (int i = 0; i < length; ++i)
    {
        if (!get(i))
        {
            if (consecutive == 0)
                start = i;
            ++consecutive;

            if (consecutive == count)
            {
                // 找到足够大的空闲分区，标记为已分配
                for (int j = start; j < start + count; ++j)
                {
                    set(j, true);
                }
                return start;
            }
        }
        else
        {
            consecutive = 0;
            start = -1;
        }
    }

    return -1;
}

// 最佳适应算法 (Best Fit)
// 遍历所有空闲分区，找到最小的足够大的空闲分区
int BitMap::bestFitAllocate(const int count)
{
    if (count <= 0)
        return -1;

    int bestStart = -1;
    int bestSize = length + 1; // 初始化为一个不可能的最大值
    int consecutive = 0;
    int currentStart = -1;

    for (int i = 0; i <= length; ++i)
    {
        // i == length 作为哨兵，确保最后一个空闲块被处理
        bool occupied = (i < length) ? get(i) : true;

        if (!occupied)
        {
            if (consecutive == 0)
                currentStart = i;
            ++consecutive;
        }
        else
        {
            // 遇到已分配区域或到达末尾，检查当前空闲块
            if (consecutive >= count && consecutive < bestSize)
            {
                bestSize = consecutive;
                bestStart = currentStart;
            }
            consecutive = 0;
        }
    }

    if (bestStart != -1)
    {
        for (int j = bestStart; j < bestStart + count; ++j)
        {
            set(j, true);
        }
    }

    return bestStart;
}

// 最坏适应算法 (Worst Fit)
// 遍历所有空闲分区，找到最大的空闲分区
int BitMap::worstFitAllocate(const int count)
{
    if (count <= 0)
        return -1;

    int worstStart = -1;
    int worstSize = -1;
    int consecutive = 0;
    int currentStart = -1;

    for (int i = 0; i <= length; ++i)
    {
        bool occupied = (i < length) ? get(i) : true;

        if (!occupied)
        {
            if (consecutive == 0)
                currentStart = i;
            ++consecutive;
        }
        else
        {
            if (consecutive >= count && consecutive > worstSize)
            {
                worstSize = consecutive;
                worstStart = currentStart;
            }
            consecutive = 0;
        }
    }

    if (worstStart != -1)
    {
        for (int j = worstStart; j < worstStart + count; ++j)
        {
            set(j, true);
        }
    }

    return worstStart;
}

// 循环首次适应算法 (Next Fit)
// 从上次分配结束的位置开始搜索，找到足够大的空闲分区
int BitMap::nextFitAllocate(const int count)
{
    if (count <= 0)
        return -1;

    int consecutive = 0;
    int start = -1;

    // 从nextFitIndex开始搜索，最多搜索整个bitmap一圈
    for (int step = 0; step < length; ++step)
    {
        int i = (nextFitIndex + step) % length;

        if (!get(i))
        {
            if (consecutive == 0)
                start = i;
            ++consecutive;

            if (consecutive == count)
            {
                for (int j = 0; j < count; ++j)
                {
                    set((start + j) % length, true);
                }
                // 更新下次搜索的起始位置
                nextFitIndex = (start + count) % length;
                return start;
            }
        }
        else
        {
            consecutive = 0;
            start = -1;
        }
    }

    return -1;
}

// ============================================================
// 内存统计函数
// ============================================================

// 返回已分配的资源数量
int BitMap::getUsedCount() const
{
    int count = 0;
    for (int i = 0; i < length; ++i)
    {
        if (get(i))
            ++count;
    }
    return count;
}

// 返回最大连续空闲块的大小
int BitMap::getMaxFreeBlock() const
{
    int maxBlock = 0;
    int current = 0;

    for (int i = 0; i < length; ++i)
    {
        if (!get(i))
        {
            ++current;
            if (current > maxBlock)
                maxBlock = current;
        }
        else
        {
            current = 0;
        }
    }

    return maxBlock;
}

// 返回不连续的空闲块个数（碎片数量）
int BitMap::getFreeFragmentCount() const
{
    int fragments = 0;
    bool inFree = false;

    for (int i = 0; i < length; ++i)
    {
        if (!get(i))
        {
            if (!inFree)
            {
                ++fragments;
                inFree = true;
            }
        }
        else
        {
            inFree = false;
        }
    }

    return fragments;
}
