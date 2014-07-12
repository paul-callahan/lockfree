//
//  NonBlockingReadMapSharedPtr.hpp
//  agent-php
//
//  Created by Paul Callahan on 7/8/14.
//  Copyright (c) 2014 Paul Callahan. All rights reserved.
//

#pragma once

#include <iostream>

#include <unordered_map>
#include <atomic>
#include <pthread.h>
#include <memory>

typedef std::unordered_map<std::string, std::string> StringMap;



class NonBlockingReadMapSharedPtr {
private:
    pthread_mutex_t fMutex;
    std::shared_ptr<StringMap> fspReadMapReference;

    
public:
    
    NonBlockingReadMapSharedPtr() {
        fspReadMapReference = std::make_shared<StringMap>();
    }
    
    ~NonBlockingReadMapSharedPtr() {
        //so, nothing here?
    }
    
    std::string get(const char* key) {
        std::string keyStr(key);
        return get(keyStr);
    }
    
    std::string get(std::string &key) {
        return std::atomic_load(&fspReadMapReference)->at(key);
        //return fspReadMapReference->at(key);
    }
    
    void put(const char *key, const char *value) {
        std::string keyStr(key);
        std::string valueStr(value);
        put(keyStr, valueStr);
    }
    
    void put(std::string &key, std::string &value) {
        pthread_mutex_lock(&fMutex);
        std::shared_ptr<StringMap> spMapCopy = std::make_shared<StringMap>(*fspReadMapReference);
        std::pair<std::string, std::string> kvPair(key, value);
        spMapCopy->insert(kvPair);
        std::atomic_store(&fspReadMapReference, spMapCopy);
        pthread_mutex_unlock(&fMutex);
    }
    
    void clear() {
        pthread_mutex_lock(&fMutex);
        std::shared_ptr<StringMap> spMapCopy = std::make_shared<StringMap>(*fspReadMapReference);
        std::atomic_store(&fspReadMapReference, spMapCopy);
        //fspReadMapReference.swap(spMapCopy);
        pthread_mutex_unlock(&fMutex);
    }

    bool isLockFree() {
        return std::atomic_is_lock_free(&fspReadMapReference);
    }
    
};

