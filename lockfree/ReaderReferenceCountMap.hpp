//
//  NonBlockingReadMapV1.hpp
//  lockfree
//
//  Created by Paul Callahan on 7/9/14.
//  Copyright (c) 2014 Paul Callahan. All rights reserved.
//

#pragma once


#include <unordered_map>
#include <atomic>
#include <pthread.h>

typedef std::unordered_map<std::string, std::string> StringMap;

/**
 * Wraps and delegates to a StringMap (std::unordered_map<std::string, std::string>) and
 * adds reference counting.
 *
 */
class ReaderReferenceCountedStringMap {
    StringMap *fMap;
    uint fReaderCount;
    
};

class NonBlockingReadMapRefCount {
    
private:
    std::Atomic<ReaderReferenceCountedStringMap*> fMap;
    
    pthread_mutex_t fMutex;
    std::atomic<ReferenceCountedStringMap*> fReadMapReference;
    
    
public:
    
    NonBlockingReadMapRefCount() {
        fReadMapReference.store(new ReferenceCountedStringMap());
        
    }
    
    ~NonBlockingReadMapRefCount() {
        delete fReadMapReference.load();
    }
    
    std::string get(std::string &key) {
        ReaderReferenceCountedStringMap *currentMap =  fReadMapReference.load();

        ReaderReferenceCountedStringMap newValue { nullptr, 0 };
        do{
            
            newValue.counter = oldValue.counter+1;
            while(target->compare_exchange_strong(oldValue,newValue));
        
        
        map->referenceIncrement();
        std::string value = map->at(key);
        map->referenceIncrement();
        return value;
    }
    
    void put(std::string &key, std::string &value) {
        pthread_mutex_lock(&fMutex);
        ReferenceCountedStringMap *pCurrentReadMap = fReadMapReference.load();
        std::unique_ptr<ReferenceCountedStringMap> upMapCopy(new ReferenceCountedStringMap(*pCurrentReadMap));
        std::pair<std::string, std::string> kvPair(key, value);
        upMapCopy->insert(kvPair);
        fReadMapReference.store(upMapCopy.release());
        //todo:  give up after a while in case something went awry.
        while(pCurrentReadMap->getReferenceCount() > 0);
        delete pCurrentReadMap;
        pthread_mutex_unlock(&fMutex);
    }
    
    void clear() {
        pthread_mutex_lock(&fMutex);
        ReferenceCountedStringMap *pCurrentReadMap = fReadMapReference.load();
        std::unique_ptr<ReferenceCountedStringMap> upMapCopy(new ReferenceCountedStringMap());
        fReadMapReference.store(upMapCopy.release());
        pCurrentReadMap->clear();
        while(pCurrentReadMap->getReferenceCount() > 0);
        delete pCurrentReadMap;
        pthread_mutex_unlock(&fMutex);
    }
    
};


