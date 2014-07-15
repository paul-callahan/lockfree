//
//  Hazard.h
//  lockfree
//
//  Created by Paul Callahan on 7/11/14.
//  Copyright (c) 2014 Paul Callahan. All rights reserved.
//

#pragma once

#include <iostream>

#include "Hazard.h"
#include <unordered_set>
#include <vector>

//#include <boost/thread/tss.hpp>
#include <atomic>


typedef std::unordered_map<std::string, std::string> StringMap;

class HPRecType;
//bool CAS(HPRecType**, HPRecType*, HPRecType*);
//bool CAS(StringMap**, StringMap*, StringMap*);

// Hazard pointer record
class HPRecType {
    
    //boost::atomic<int> active_;
    std::atomic_int active_;
    // Global header of the HP list
    static std::atomic<HPRecType *> pHead_;
    // The length of the list
    static std::atomic_int listLen_;
public:
    // Can be used by the thread
    // that acquired it
    void * pHazard_;
    HPRecType * pNext_;
    static HPRecType * Head() {
        return pHead_;
    }
    
    // Acquires one hazard pointer
    static HPRecType * Acquire() {
        int one = 1;
        // Try to reuse a retired HP record
        HPRecType * p = pHead_;
        for (; p; p = p->pNext_) {
            if (p->active_ || !p->active_.compare_exchange_strong(one, 0, std::memory_order_acquire))
                continue;
            // Got one!
            return p;
        }
        
        // Increment the list length
        int oldLen;
        do {
            oldLen = listLen_;
            
        } while (!listLen_.compare_exchange_strong(oldLen, oldLen +1, std::memory_order_acquire));
        
        // Allocate a new one
        p = new HPRecType;
        p->active_ = 1;
        p->pHazard_ = 0;
        // Push it to the front
        HPRecType * old;
        do {
            old = pHead_;
            p->pNext_ = old;
        } while (!pHead_.compare_exchange_strong(old, p, std::memory_order_acquire));
        return p;
    }
    
    // Releases a hazard pointer
    static void Release(HPRecType* p) {
        p->pHazard_ = 0;
        p->active_ = 0;
    }
};

// Per-thread private variable
//static boost::thread_specific_ptr<std::vector<StringMap*>> rlist;
static std::vector<StringMap*> *rlist;


class HazardPointerMap {
    std::atomic<StringMap *> pMap_;
    
    
private:
    static void Retire(StringMap * pOld) {
        // put it in the retired list
        rlist->push_back(pOld);
        if (rlist->size() >= 10) {
            Scan(HPRecType::Head());
        }
    }
    
    
    static void Scan(HPRecType * head) {
        // Stage 1: Scan hazard pointers list
        // collecting all non-null ptrs
        std::vector<void*> hp;
        while (head) {
            void * p = head->pHazard_;
            if (p) hp.push_back(p);
            head = head->pNext_;
        }
        // Stage 2: sort the hazard pointers
        sort(hp.begin(), hp.end(), std::less<void*>());
        // Stage 3: Search for'em!
        std::vector<StringMap *>::iterator i = rlist->begin();
        while (i != rlist->end()) {
            if (!binary_search(hp.begin(), hp.end(),  *i)) {
                // Aha!
                delete *i;
                i = rlist->erase(i);
                
                if (&*i != &rlist->back()) {
                    *i = rlist->back();
                }
                rlist->pop_back();
            } else {
                ++i;
            }
        }
    }
    
public:
    void Update(std::string&k, std::string&v){
        StringMap * pNew = 0;
        StringMap * pOld;
        do {
            pOld = pMap_;
            if (pNew) delete pNew;
            pNew = new StringMap(*pOld);
            (*pNew)[k] = v;
        } while (!pMap_.compare_exchange_strong(pOld, pNew, std::memory_order_acq_rel));
        Retire(pOld);
    }
    
    std::string Lookup(const std::string &k){
        HPRecType * pRec = HPRecType::Acquire();
        StringMap *ptr;
        do {
            ptr = pMap_;
            pRec->pHazard_ = ptr;
        } while (pMap_ != ptr);
        // Save Willy
        std::string result = ptr->at(k);
        // pRec can be released now
        // because it's not used anymore
        HPRecType::Release(pRec);
        return result;
    }
    
};