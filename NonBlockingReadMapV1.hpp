//
//  NonBlockingReadMapV1.hpp
//  lockfree
//
//  Created by Paul Callahan on 7/9/14.
//  Copyright (c) 2014 Paul Callahan. All rights reserved.
//

#ifndef lockfree_NonBlockingReadMapV1_hpp
#define lockfree_NonBlockingReadMapV1_hpp


#include <unordered_map>
#include <atomic>
#include <pthread.h>

typedef std::unordered_map<std::string, std::string> StringMap;

class NonBlockingReadMapV1 {
    
private:
    pthread_mutex_t fMutex;
    std::atomic<StringMap*> fReadMapReference;
    
public:
    
    NonBlockingReadMapV1() {
        fReadMapReference.store(new StringMap());
    }
    
    ~NonBlockingReadMapV1() {
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
        delete pCurrentReadMap;
        pthread_mutex_unlock(&fMutex);
    }
    
    void clear() {
        pthread_mutex_lock(&fMutex);
        StringMap *pCurrentReadMap = fReadMapReference.load();
        std::unique_ptr<StringMap> upMapCopy(new StringMap());
        fReadMapReference.store(upMapCopy.release());
        pCurrentReadMap->clear();
        delete pCurrentReadMap;
        pthread_mutex_unlock(&fMutex);
    }
    
};




#endif
