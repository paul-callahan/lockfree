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
#include <memory>

typedef std::unordered_map<std::string, std::string> StringMap;

class NonBlockingReadMap {
private:
    pthread_mutex_t fMutex;    
    std::shared_ptr<StringMap> fspReadMapReference;

    
public:
    
    NonBlockingReadMap() {
        fspReadMapReference = std::make_shared<StringMap>();
    }
    
    ~NonBlockingReadMap() {
        //so, nothing here?
    }
    
    std::string get(std::string &key) {
        return fspReadMapReference->at(key);
    }
    
    void put(std::string &key, std::string &value) {
        pthread_mutex_lock(&fMutex);
        std::shared_ptr<StringMap> spMapCopy = std::make_shared<StringMap>(*fspReadMapReference);
        std::pair<std::string, std::string> kvPair(key, value);
        spMapCopy->insert(kvPair);
        fspReadMapReference.swap(spMapCopy);
        pthread_mutex_unlock(&fMutex);
    }
    
    void clear() {
        pthread_mutex_lock(&fMutex);
        std::shared_ptr<StringMap> spMapCopy = std::make_shared<StringMap>(*fspReadMapReference);
        fspReadMapReference.swap(spMapCopy);
        spMapCopy->clear();
        pthread_mutex_unlock(&fMutex);
    }
    
};

#endif /* defined(__agent_php__NonBlockingReadMap__) */
