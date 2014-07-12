//
//  NonBlockingReadMapAtomic.hpp
//  lockfree
//
//  Created by Paul Callahan on 7/9/14.
//  Copyright (c) 2014 Paul Callahan. All rights reserved.
//

#pragma once



#include <unordered_map>
#include <atomic>
#include <pthread.h>
#include <boost/lockfree/queue.hpp>

typedef std::unordered_map<std::string, std::string> StringMap;

class NonBlockingReadMapAtomic {
    
private:
    pthread_mutex_t fMutex;
    std::atomic<StringMap*> fReadMapReference;
    //boost::queue<>
    
    
public:
    
    

    
    NonBlockingReadMapAtomic() {
        fReadMapReference.store(new StringMap());
    }
    
    ~NonBlockingReadMapAtomic() {
        delete fReadMapReference.load();
    }
    

    


    std::string get(std::string &key) {
        return fReadMapReference.load()->at(key);
    }
    
    void put(std::string &key, std::string &value) {
        pthread_mutex_lock(&fMutex);
        StringMap *pCurrentReadMap = fReadMapReference.load();
        std::unique_ptr<StringMap> upMapCopy(new StringMap(*pCurrentReadMap));
        std::pair<std::string, std::string> kvPair(key, value);
        upMapCopy->insert(kvPair);
        fReadMapReference.store(upMapCopy.release());
        delete pCurrentReadMap;   //  <-- Boom!  get() might still be using it.
        pthread_mutex_unlock(&fMutex);
    }
    
    void clear() {
        pthread_mutex_lock(&fMutex);
        StringMap *pCurrentReadMap = fReadMapReference.load();
        std::unique_ptr<StringMap> upMapCopy(new StringMap());
        fReadMapReference.store(upMapCopy.release());
        pCurrentReadMap->clear();
        delete pCurrentReadMap;  //  <-- Boom!  get() might still be using it.
        pthread_mutex_unlock(&fMutex);
    }
    
};


