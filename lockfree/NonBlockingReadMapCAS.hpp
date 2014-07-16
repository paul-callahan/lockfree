//
//  NonBlockingReadMapCAS.hpp
//  lockfree
//
//  Created by Paul Callahan on 7/15/14.
//  Copyright (c) 2014 Paul Callahan. All rights reserved.
//

#pragma once


#include <unordered_map>
#include <atomic>
#include <pthread.h>
#include <queue>


typedef std::unordered_map<std::string, std::string> StringMap;

static const int zero = 0;  //provides an l-value for asm code

class NonBlockingReadMapCAS {

public:
    
    class OctaWordMapWrapper {
    public:
        StringMap* fStringMap;
        std::atomic<uint64_t> fCounter;
        
        OctaWordMapWrapper(OctaWordMapWrapper* copy) : fStringMap(new StringMap(*copy->fStringMap)), fCounter(0) { }
        
        OctaWordMapWrapper() : fStringMap(new StringMap), fCounter(0) { }
        
        ~OctaWordMapWrapper() {
            delete fStringMap;
        }
        
        /**
         * Does a compare and swap on an octa-word - in this case, our two adjacent class members fStringMap 
         * pointer and fCounter.
         */
        static bool inline doubleCAS(OctaWordMapWrapper* target, StringMap* compareMap, uint64_t compareCounter, StringMap* swapMap, uint64_t swapCounter ) {
            bool cas_result;
            __asm__ __volatile__
            (
             "lock cmpxchg16b %0;"    // cmpxchg16b sets ZF on success
             "setz       %3;"         // if ZF set, set cas_result to 1
             
             : "+m" (*target),
               "+a" (compareMap),     //compare target's stringmap pointer to compareMap
               "+d" (compareCounter), //compare target's counter to compareCounter
               "=q" (cas_result)      //results
             : "b"  (swapMap),        //swap target's stringmap pointer with swapMap
               "c"  (swapCounter)     //swap target's counter with swapCounter
             : "cc", "memory"
             );
            return cas_result;
        }
        

        
        OctaWordMapWrapper* atomicIncrementAndGetPointer()
        {
            while(true) {
                if (doubleCAS(this, this->fStringMap, this->fCounter, this->fStringMap, this->fCounter +1))
                    break;
            }
            return this;
        }
        
        OctaWordMapWrapper* atomicDecrement()
        {
            while(true) {
                if (doubleCAS(this, this->fStringMap, this->fCounter, this->fStringMap, this->fCounter -1))
                    break;
            }
            return this;
        }
        
        bool atomicSwapWhenNotReferenced(StringMap* newMap)
        {
            return doubleCAS(this, this->fStringMap, zero, newMap, 0);
        }
    };
    __attribute__((aligned(16)));

    std::atomic<OctaWordMapWrapper*> fReadMapReference;
    pthread_mutex_t fMutex;

    
    NonBlockingReadMapCAS()  {
        fReadMapReference = new OctaWordMapWrapper();
    }
    
    ~NonBlockingReadMapCAS() {

    }
    
    bool contains(const char* key) {
        std::string keyStr(key);
        return contains(keyStr);
    }
    
    bool contains(std::string &key) {
        OctaWordMapWrapper *map = fReadMapReference.load()->atomicIncrementAndGetPointer();
        bool result = map->fStringMap->count(key) != 0;
        map->fCounter--;
        return result;
    }
    
    std::string get(const char* key) {
        std::string keyStr(key);
        return get(keyStr);
    }
    
    std::string get(std::string &key) {
        OctaWordMapWrapper *map = fReadMapReference.load()->atomicIncrementAndGetPointer();
        std::string value = map->fStringMap->at(key);
        map->fCounter--;
        //map->atomicDecrement();
        return value;
    }
    
    void put(const char* key, const char* value) {
        std::string keyStr(key);
        std::string valueStr(value);
        put(keyStr, valueStr);
    }
    
    void put(std::string &key, std::string &value) {
        pthread_mutex_lock(&fMutex);
        OctaWordMapWrapper *oldWrapper = fReadMapReference;
        OctaWordMapWrapper *newWrapper = new OctaWordMapWrapper(oldWrapper);
        std::pair<std::string, std::string> kvPair(key, value);
        newWrapper->fStringMap->insert(kvPair);
        fReadMapReference.store(newWrapper);
        
        while (oldWrapper->fCounter > 0);
        delete oldWrapper;
        pthread_mutex_unlock(&fMutex);

        /*
        StringMap *pOldMap = fReadMapReference->fStringMap;
        StringMap *pNewMap = 0;
        do {
            if (pNewMap) delete pNewMap;
            pNewMap = new StringMap(*pOldMap);
            std::pair<std::string, std::string> kvPair(key, value);
            pNewMap->insert(kvPair);
        } while (!fReadMapReference->atomicSwapWhenNotReferenced(pNewMap));
        delete pOldMap;
         */
    }
    
    void clear() {
        pthread_mutex_lock(&fMutex);
        OctaWordMapWrapper *oldWrapper = fReadMapReference;
        OctaWordMapWrapper *newWrapper = new OctaWordMapWrapper(oldWrapper);
        fReadMapReference.store(newWrapper);
        while (oldWrapper->fCounter > 0);
        delete oldWrapper;
        pthread_mutex_unlock(&fMutex);

    }
    
};

typedef NonBlockingReadMapCAS NonBlockingReadMap;
