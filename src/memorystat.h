#pragma once

#include "config.h"

#if USE_MEMORYSTAT

#ifdef __cplusplus
#include <new>
#include <cstdlib>
#include <cstring>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#elif defined(__GNUC__)
#include <malloc.h>
#endif

struct MemoryStat {
    size_t mallocCount;
    size_t reallocCount;
    size_t freeCount;
    size_t currentSize;
    size_t peakSize;
};

class Memory {
public:
    static Memory& Instance() {
        static Memory memory;
        return memory;
    }

    void* Malloc(size_t size) {
        void* p = malloc(size);
        MallocStat(p);
        return p;
    }

    void* Realloc(void* ptr, size_t size) {
        if (ptr)
            FreeStat(ptr);

        void *p = realloc(ptr, size);

        ReallocStat(p);
        return p;
    }

    void Free(void* ptr) {
        if (ptr) {
            FreeStat(ptr);
            free(ptr);
        }
    }

    const MemoryStat& GetStat() { return *stat_; }

    MemoryStat* SetStat(MemoryStat* stat) {
        MemoryStat* old = stat_;
        stat_ = stat;
        return old;
    }

    void MallocStat(void* p) {
        stat_->currentSize += GetMallocSize(p);
        if (stat_->peakSize < stat_->currentSize)
            stat_->peakSize = stat_->currentSize;
        stat_->mallocCount++;
    }

    void ReallocStat(void *p) {
        stat_->currentSize += GetMallocSize(p);
        if (stat_->peakSize < stat_->currentSize)
            stat_->peakSize = stat_->currentSize;
        stat_->reallocCount++;
    }

    void FreeStat(void *p) {
        size_t size = GetMallocSize(p);
        stat_->currentSize -= size;
        stat_->freeCount++;
    }
private:
    size_t GetMallocSize(void* ptr) {
#if defined(_MSC_VER)
        return _msize(ptr);
#elif defined(__APPLE__)
        return malloc_size(ptr);
#elif defined(__GNUC__)
        return malloc_usable_size(ptr);
#else
#error Must implement Memory::GetMallocSize() in memorystat.h
#endif
    }

    Memory() : stat_(&global_) { memset(&global_, 0, sizeof(global_)); }

    MemoryStat global_;
    MemoryStat* stat_;
};

class MemoryStatScope {
public:
    MemoryStatScope() : old_(Memory::Instance().SetStat(&local_)) {
        memset(&local_, 0, sizeof(local_));        
    }

    ~MemoryStatScope() {
        old_->mallocCount += local_.mallocCount;
        old_->reallocCount += local_.reallocCount;
        old_->freeCount += local_.freeCount;
        old_->peakSize += local_.peakSize;
        old_->currentSize += local_.currentSize;
        Memory::Instance().SetStat(old_);
    }

private:
    MemoryStat local_;
    MemoryStat* old_;
};

#define MEMORYSTAT_SCOPE() MemoryStatScope scope##__LINE__

#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#else
#include <stdlib.h>
#endif

extern void* MemoryStatMalloc(size_t size);
extern void* MemoryStatCalloc(size_t num, size_t size);
extern void* MemoryStatRealloc(void* ptr, size_t size);
extern void MemoryStatFree(void* ptr);

#ifdef __cplusplus
}
#endif

#define malloc MemoryStatMalloc
#define calloc MemoryStatCalloc
#define realloc MemoryStatRealloc
#define free MemoryStatFree

#ifdef __cplusplus
namespace std {
    inline void* MemoryStatMalloc(size_t size) {
        return ::MemoryStatMalloc(size);
    }
    inline void* MemoryStatRealloc(void* ptr, size_t size) {
        return ::MemoryStatRealloc(ptr, size);
    }
    inline void MemoryStatFree(void* ptr) {
        return ::MemoryStatFree(ptr);
    }
};
#endif

#else // USE_MEMORYSTAT

#define MEMORYSTAT_SCOPE()

#endif

// Override strdup
#include <string.h>
#include <stdlib.h>

#ifdef strdup 
#undef strdup
#endif

#define strdup StrDup

#ifdef __cplusplus
extern "C" {
#endif

char* StrDup(const char* src);

#ifdef __cplusplus
}
#endif