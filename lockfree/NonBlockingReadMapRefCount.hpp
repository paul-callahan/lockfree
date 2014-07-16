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
    StringMap *fUnderlyingMap;
    std::atomic<int> fReferenceCount;
    std::atomic_bool fReserved;
    
public:

    ReferenceCountedStringMap() : fReferenceCount(0), fUnderlyingMap(new StringMap) {}
    
    ReferenceCountedStringMap(StringMap &map) : fReferenceCount(0), fUnderlyingMap(new StringMap(map)) {}
    
    
    ~ReferenceCountedStringMap() {
        delete fUnderlyingMap;
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
    
    bool contains(std::string &key) {
        return fUnderlyingMap->count(key) != 0;
    }
    
    std::string at(std::string &key) {
        return fUnderlyingMap->at(key);
    }
    
    void insert(std::pair<std::string, std::string> pair) {
        fUnderlyingMap->insert(pair);
    }
    
    void setReserved(bool r) {
        fReserved.exchange(r);
    }
    
    bool isReserved() {
        return fReserved;
    }
    
    void clear() {
        fUnderlyingMap->clear();
    }
    
    StringMap *getMap() {
        return fUnderlyingMap;
    }
};

class NonBlockingReadMapRefCount {
    
private:
    pthread_mutex_t fMutex;
    std::atomic<ReferenceCountedStringMap*> fReadMapReference;

    
public:
    
    NonBlockingReadMapRefCount() {
        fReadMapReference.store(new ReferenceCountedStringMap(*new StringMap()));

    }
    
    ~NonBlockingReadMapRefCount() {
        delete fReadMapReference.load();
    }
    
    std::string get(std::string &key) {
        ReferenceCountedStringMap *map;
        //get a reference of the member fReadMapReference into our thread/stack and
        //increment the reference count only if the map didn't change out from under us.
        while (true) {
            fReadMapReference.load()->setReserved(true);
            map = fReadMapReference.load();
            //at this point, a put() could have changed the member fReadMapReference.
            map->referenceIncrement();
            if (map->getMap() == fReadMapReference.load()->getMap())
                //we incremented the right map
                break;
            else
                //the map changed on us.  try again!
                map->referenceDecrement();
        }
        std::string value = map->at(key);
        map->referenceDecrement();
        return value;
    }
    
    bool contains(std::string &key) {
        ReferenceCountedStringMap *map;
        //get a reference of the member fReadMapReference into our thread/stack and
        //increment the reference count only if the map didn't change out from under us.
        while (true) {
            fReadMapReference.load()->setReserved(true);            
            map = fReadMapReference.load();
            //at this point, a put() could have changed the member fReadMapReference.
            map->referenceIncrement();
            if (map->getMap() == fReadMapReference.load()->getMap())
                //we incremented the right map
                break;
            else
                //the map changed on us.  try again!
                map->referenceDecrement();
        }
        map->setReserved(false);
        bool result = map->contains(key);
        map->referenceDecrement();
        return result	;
    }
    
    void put(std::string &key, std::string &value) {
        pthread_mutex_lock(&fMutex);
        ReferenceCountedStringMap *pCurrentReadMap = fReadMapReference.load();
        ReferenceCountedStringMap* pNewMap = new ReferenceCountedStringMap(*pCurrentReadMap->getMap());
        std::pair<std::string, std::string> kvPair(key, value);
        pNewMap->insert(kvPair);
        //don't do anything until the reservation is lifted.
        while (!pCurrentReadMap->isReserved());
        fReadMapReference.store(pNewMap);
        //wait until reference count has gone to zero.
        //todo:  give up after a while in case something went awry.
        while(pCurrentReadMap->getReferenceCount() > 0);
        delete pCurrentReadMap;
        pthread_mutex_unlock(&fMutex);
    }
    
    void clear() {
        pthread_mutex_lock(&fMutex);
        ReferenceCountedStringMap *pCurrentReadMap = fReadMapReference.load();
        ReferenceCountedStringMap* pNewMap = new ReferenceCountedStringMap();
        //don't do anything until the reservation is lifted.
        while (!pCurrentReadMap->isReserved());
        fReadMapReference.store(pNewMap);
        //wait until reference count has gone to zero.
        //todo:  give up after a while in case something went awry.
        while(pCurrentReadMap->getReferenceCount() > 0);
        delete pCurrentReadMap;
        pthread_mutex_unlock(&fMutex);
    }
    
};

void * reader(void * ptr) {
    NonBlockingReadMapRefCount *map = (NonBlockingReadMapRefCount*) ptr;
    map->clear();
    return 0;
}

void foo (){
    const int NUM_THREADS = 100;
    pthread_t threads[NUM_THREADS];
    NonBlockingReadMapRefCount n;
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], nullptr, reader, &n);
    }
  
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], nullptr);
    }
    
}

