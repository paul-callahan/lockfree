//
//  NonBlockingReadMap.h
//  agent-php
//
//  Created by Paul Callahan on 7/8/14.
//  Copyright (c) 2014 Paul Callahan. All rights reserved.
//

#ifndef __agent_php__NonBlockingReadMap__
#define __agent_php__NonBlockingReadMap__

#include <iostream>

#include <unordered_map>
#include <atomic>
#include <pthread.h>

typedef std::unordered_map<std::string, std::string> StringMap;


class NonBlockingReadMap {
private:
    pthread_mutex_t fMutex;    
    std::atomic<std::shared_ptr<StringMap>> fspReadMapReference;
    
public:
    
    NonBlockingReadMap() {
        fReadMapReference.store(new StringMap());
    }
    
    ~NonBlockingReadMap() {
        //so, nothing here?
    }
    
    std::string get(std::string &key) {
        return fReadMapReference.load()->at(key);
    }
    
    void put(std::string &key, std::string &value) {
        pthread_mutex_lock(&fMutex);
        std::shared_ptr<StringMap> psCurrentReadMap =  fspReadMapReference.load();
        std::shared_ptr<StringMap> spMapCopy(new StringMap(*psCurrentReadMap));
        std::pair<std::string, std::string> kvPair(key, value);
        spMapCopy->insert(kvPair);
        fReadMapReference.store(spMapCopy.get());
        pthread_mutex_unlock(&fMutex);
    }
    
    void clear() {
        pthread_mutex_lock(&fMutex);
        std::shared_ptr<StringMap> psCurrentReadMap =  fspReadMapReference.load();
        std::shared_ptr<StringMap> spMapCopy(new StringMap(*psCurrentReadMap));
        fReadMapReference.store(spMapCopy.get());
        psCurrentReadMap->clear();
        pthread_mutex_unlock(&fMutex);
    }
    
};

#endif /* defined(__agent_php__NonBlockingReadMap__) */
