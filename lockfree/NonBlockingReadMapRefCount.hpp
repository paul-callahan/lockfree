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

class ReferenceCountedStringMap  {
protected:
    StringMap fUnderlyingMap;
    std::atomic<int> fReferenceCount;
    
public:
    ReferenceCountedStringMap() {
        fReferenceCount = 0;
    }
    
    ReferenceCountedStringMap(ReferenceCountedStringMap const &map) {
        fReferenceCount = 0;
        fUnderlyingMap = StringMap(map.fUnderlyingMap);
    }
    
    void referenceIncrement() {
        fReferenceCount++;
    }
    
    void referenceDecrement() {
        fReferenceCount--;
    }
    
    int getReferenceCount() {
        return fReferenceCount.load();
    }
    
    std::string at(std::string &key) {
        return fUnderlyingMap.at(key);
    }
    
    void insert(std::pair<std::string, std::string> pair) {
        fUnderlyingMap.insert(pair);
    }
    
    void clear() {
        fUnderlyingMap.clear();
    }
};

class NonBlockingReadMapRefCount {
    
private:
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
        ReferenceCountedStringMap *map =  fReadMapReference.load();
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


