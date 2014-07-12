//
//  Hazard.cpp
//  lockfree
//
//  Created by Paul Callahan on 7/11/14.
//  Copyright (c) 2014 Paul Callahan. All rights reserved.
//

#include "Hazard.h"
#include <boost/unordered_map.hpp>
#include <vector>
#include <boost/thread/tss.hpp>
#include <boost/atomic.hpp>

typedef boost::unordered_map<std::string, std::string> StringMap;

class HPRecType;

bool CAS(int*, int, int);
bool CAS(HPRecType**, HPRecType*, HPRecType*);
bool CAS(StringMap**, StringMap*, StringMap*);

// Hazard pointer record
class HPRecType {

    int active_;
    // Global header of the HP list
    static HPRecType * pHead_;
    // The length of the list
    static int listLen_;
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
        // Try to reuse a retired HP record
        HPRecType * p = pHead_;
        for (; p; p = p->pNext_) {
            if (p->active_ || !CAS(&p->active_, 0, 1))
                continue;
            // Got one!
            return p;
        }
        
        // Increment the list length
        int oldLen;
        do {
            oldLen = listLen_;
        } while (!CAS(&listLen_, oldLen, oldLen + 1));
        
        // Allocate a new one
        p = new HPRecType;
        p->active_ = 1;
        p->pHazard_ = 0;
        // Push it to the front
        HPRecType * old;
        do {
            old = pHead_;
            p->pNext_ = old;
        } while (!CAS(&pHead_, old, p));
        return p;
    }
    
    // Releases a hazard pointer
    static void Release(HPRecType* p) {
        p->pHazard_ = 0;
        p->active_ = 0;
    }
};
// Per-thread private variable
static boost::thread_specific_ptr<std::vector<StringMap*>> rlist;


class HazardPointerMap {
    boost::atomic<StringMap *> pMap_;

    
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
        } while (!pMap_.compare_exchange_strong(pOld, pNew, boost::memory_order_acq_rel));
        Retire(pOld);
    }
    
};