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

/*
struct MyThing {
    std::atomic<void *> pSomeObj;
    std::atomic<uint64_t> counter;
    //__attribute__((aligned(16)));
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
    
    //std::atomic<MyThing*> asrc;
    //std::atomic<MyThing*> adest;
    
    
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

*/
static const int zero = 0;  //provides an l-value for asm code



class NonBlockingReadMapCAS {

    
public:
    
    class StringMapHazardPointer {
        
    public:
        StringMap* fStringMap;
        std::atomic<uint64_t> fCounter;
        
        StringMapHazardPointer(StringMapHazardPointer* copy) : fStringMap(copy->fStringMap), fCounter(0) { }
        
        StringMapHazardPointer() : fStringMap(new StringMap), fCounter(0) { }
        
        ~StringMapHazardPointer() {
            delete fStringMap;
        }
        
        static bool doubleCAS(StringMapHazardPointer* target, StringMap* compareMap, uint64_t compareCounter, StringMap* swapMap, uint64_t swapCounter ) {
            bool cas_result;
            __asm__ __volatile__
            (
             "lock cmpxchg16b %0;"  // cmpxchg16b sets ZF on success
             "setz       %3;"  // if ZF set, set cas_result to 1
             
             : "+m" (*target),
             "+a" (compareMap),  //compare stringmap pointer
             "+d" (compareCounter),    //compare counter
             "=q" (cas_result)        //results
             : "b"  (swapMap),  //replace stringmap pointer with
             "c"  (swapCounter)  //replace counter with
             : "cc", "memory"
             );
            return cas_result;
        }
        
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
        
        StringMapHazardPointer* atomicIncrementAndGetPointer()
        {
            bool swap_result = false;
            doubleCAS(this, this->fStringMap, this->fCounter, this->fStringMap, this->fCounter +1);
            while (!swap_result) {
                __asm__ __volatile__
                (
                 "lock cmpxchg16b %0;"  // cmpxchg16b sets ZF on success
                 "setz       %3;"  // if ZF set, set cas_result to 1
                 
                 : "+m" (*this),
                   "+a" (this->fStringMap),  //compare stringmap pointer
                   "+d" (this->fCounter),    //compare counter
                   "=q" (swap_result)        //results
                 : "b"  (this->fStringMap),  //replace stringmap pointer with
                   "c"  (this->fCounter +1)  //replace counter with
                 : "cc", "memory"
                 );
            }
            return this;
        }
        
        StringMapHazardPointer* atomicDecrement()
        {
            bool swap_result = false;
            while (!swap_result) {
                __asm__ __volatile__
                (
                 "lock cmpxchg16b %0;"  // cmpxchg16b sets ZF on success
                 "setz       %3;"  // if ZF set, set cas_result to 1
                 
                 : "+m" (*this),
                 "+a" (this->fStringMap),  //compare stringmap pointer
                 "+d" (this->fCounter),    //compare counter
                 "=q" (swap_result)        //results
                 : "b"  (this->fStringMap),  //replace stringmap pointer with
                 "c"  (this->fCounter +1)  //replace counter with
                 : "cc", "memory"
                 );
            }
            return this;
        }
        
        void atomicCopyWhenNotReferenced(StringMap* newMap)
        {
            bool swap_result = false;
            while (!swap_result) {
                __asm__ __volatile__
                (
                 "lock cmpxchg16b %0;"  // cmpxchg16b sets ZF on success
                 "setz       %3;"  // if ZF set, set cas_result to 1
                 
                 : "+m" (*this),
                   "+a" (this->fStringMap),      //compare stringmap pointer
                   "+d" (zero),                  //compare counter
                   "=q" (swap_result)            //results
                 : "b"  (newMap),                //replace stringmap pointer with
                   "c"  (0)                      //replace counter with
                 : "cc", "memory"
                 );
            }
        }
    };
    

__attribute__((aligned(16)));

    StringMapHazardPointer* fReadMapReference;
    
    NonBlockingReadMapCAS()  {
        fReadMapReference = new StringMapHazardPointer();

    }
    
    ~NonBlockingReadMapCAS() {

    }
    
    std::string get(std::string &key) {
        StringMapHazardPointer *map = fReadMapReference->atomicIncrementAndGetPointer();
        std::string value = map->fStringMap->at(key);
        map->fCounter--;
        return value;
    }
    
    void put(std::string &key, std::string &value) {
        StringMapHazardPointer *pCurrentReadMap = fReadMapReference;
        StringMapHazardPointer *pNewMap = new StringMapHazardPointer(pCurrentReadMap);

        std::pair<std::string, std::string> kvPair(key, value);
        pNewMap->fStringMap->insert(kvPair);
        fReadMapReference
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

