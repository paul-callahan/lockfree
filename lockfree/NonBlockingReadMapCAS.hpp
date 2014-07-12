//
//  NonBlockingReadMapCAS.hpp
//  lockfree
//
//  Created by Paul Callahan on 7/9/14.
//  Copyright (c) 2014 Paul Callahan. All rights reserved.
//

#pragma once


#include <unordered_map>
#include <atomic>
#include <pthread.h>
#include <queue>

#define MAX_THREADS = 100;

typedef std::unordered_map<std::string, std::string> StringMap;

/**
 * Wraps and delegates to a StringMap (std::unordered_map<std::string, std::string>) and
 * adds reference counting.
 *
 */





class NonBlockingReadMapCAS {
    
protected:
    
    std::queue<StringMapHazardPointer> fHazardQueue;
   // std::atomic<StringMap*> fHazard[MAX_THREADS*8];
    std::atomic<StringMapHazardPointer*> fReadMapReference;

    
public:
    
    class StringMapHazardPointer {
        
    public:
        std::atomic<StringMap*> fStringMap;
        std::atomic<uint64_t> fCounter;
        
        StringMap* getMap() {
            return fStringMap.load();
        }
        
        bool CompareAndSwap(StringMap* oldMap, uint64_t oldCounter, StringMap* newMap, uint64_t newCounter)
        {
            bool cas_result;
            __asm__ __volatile__
            (
             "lock cmpxchg16b %0;"  // cmpxchg16b sets ZF on success
             "setz       %3;"  // if ZF set, set cas_result to 1
             
             : "+m" (*this), "+a" (oldMap), "+d" (oldCounter), "=q" (cas_result)
             : "b" (newMap), "c" (newCounter)
             : "cc", "memory"
             );
            return cas_result;
        }
    };
    
    
    __attribute__((aligned(16)));
    
    NonBlockingReadMapCAS() {
        fReadMapReference.store(new StringMapHazardPointer());

    }
    
    ~NonBlockingReadMapCAS() {
        delete fReadMapReference.load();
    }
    
    std::string get(std::string &key) {
        StringMapHazardPointer *map =  fReadMapReference.load();
        fHazard[threadId*8].store(map, std::memory_order_seq_cst);
        std::string value = map->fStringMap.load()->at(key);
        return value;
    }
    
    void put(std::string &key, std::string &value) {
        StringMapHazardPointer *pCurrentReadMap = fReadMapReference.load();
        std::unique_ptr<StringMapHazardPointer> upMapCopy(new ReferenceCountedStringMap(*pCurrentReadMap));
        std::pair<std::string, std::string> kvPair(key, value);
        upMapCopy->insert(kvPair);
        fReadMapReference.store(upMapCopy.release());
        delete pCurrentReadMap;
    }
    
    void clear() {
        ReferenceCountedStringMap *pCurrentReadMap = fReadMapReference.load();
        std::unique_ptr<StringMapHazardPointer> upMapCopy(new StringMapHazardPointer());
        fReadMapReference.store(upMapCopy.release());
        pCurrentReadMap->clear();
        while(pCurrentReadMap->getReferenceCount() > 0);
        delete pCurrentReadMap;
    }
    
};

