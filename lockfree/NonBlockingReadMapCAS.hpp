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


struct MyThing {
    std::atomic<void *> pSomeObj;
    std::atomic<uint64_t> counter;
    __attribute__((aligned(16)));
};

bool AtomicCopyAndIncrement(MyThing **dest, MyThing *src) {
    //begin atomic
    dest = &src;
    src->counter++;
    //end atomic
    return true;
}

void blarg() {
    
    MyThing* src;
    MyThing* dest;
    
    AtomicCopyAndIncrement(&dest, src);
    
    std::atomic<MyThing*> asrc;
    std::atomic<MyThing*> adest;
    
    
}

bool AtmoicCopyAndIncrement(MyThing *dest, MyThing *src)
{
    bool swap_result;
    __asm__ __volatile__
    (
     "lock cmpxchg16b %0;"  // cmpxchg16b sets ZF on success
     "setz       %3;"  // if ZF set, set cas_result to 1
     
     : "+m" (*dest), "+a" (dest->pSomeObj), "+d" (dest->counter), "=q" (swap_result)
     : "b" (src->pSomeObj), "c" (src->counter +1)
     : "cc", "memory"
     );
    return swap_result;
}

bool AtmoicDecrement(MyThing *dest)
{
    bool swap_result;
    __asm__ __volatile__
    (
     "lock cmpxchg16b %0;"  // cmpxchg16b sets ZF on success
     "setz       %3;"  // if ZF set, set cas_result to 1
     
     : "+m" (*dest), "+a" (dest->pSomeObj), "+d" (dest->counter), "=q" (swap_result)
     : "b" (dest->pSomeObj), "c" (dest->counter -1)
     : "cc", "memory"
     );
    return swap_result;
}



class NonBlockingReadMapCAS {
    
protected:
    
    //std::queue<StringMapHazardPointer> fHazardQueue;
   // std::atomic<StringMap*> fHazard[MAX_THREADS*8];


    
public:
    
    class StringMapHazardPointer {
        
    public:
        StringMap* fStringMap;
        uint64_t fCounter;
        
        
        bool AtmoicCopyAndIncrement(StringMapHazardPointer *copyFrom)
        {
            bool swap_result;
            __asm__ __volatile__
            (
             "lock cmpxchg16b %0;"  // cmpxchg16b sets ZF on success
             "setz       %3;"  // if ZF set, set cas_result to 1
             
             : "+m" (*this), "+a" (this->fStringMap), "+d" (this->fCounter), "=q" (swap_result)
             : "b" (copyFrom->fStringMap), "c" (copyFrom->fCounter +1)
             : "cc", "memory"
             );
            return swap_result;
        }
    };
    __attribute__((aligned(16)));
    

    std::atomic<StringMapHazardPointer*> fReadMapReference;
    
    NonBlockingReadMapCAS()  {
        fReadMapReference = new StringMapHazardPointer();

    }
    
    ~NonBlockingReadMapCAS() {

    }
    
    std::string get(std::string &key) {
        StringMapHazardPointer *map = new StringMapHazardPointer;
        //populate our counted pointer with the current state of fReadReference and counter +1.
        while (map->AtmoicCopyAndIncrement(fReadMapReference)) {
            
        }
        std::string value = map->fStringMap->at(key);
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

