//
//  HazardPtrMap.cpp
//  lockfree
//
//  Created by Paul Callahan on 7/10/14.
//  Copyright (c) 2014 Paul Callahan. All rights reserved.
//

#include "HazardPtrMap.h"
#include <vector>
#include <unordered_map>

typedef std::unordered_map<std::string, std::string> Map;


// Hazard pointer record
class HPRecType {
    HPRecType * pNext_;
    int active_;
    // Global header of the HP list
    static HPRecType * pHead_;
    // The length of the list
    static int listLen_;
    
public:
    // Can be used by the thread
    // that acquired it
    void * pHazard_;
    
    
    static HPRecType * Head() {
        return pHead_;
    }
    
    
    // Acquires one hazard pointer
    static HPRecType * Acquire() {
        // Try to reuse a retired HP record
        HPRecType * p = pHead_;
        for (; p; p = p->pNext_) {
            if (p->active_ ||
                !CAS(&p->active_, 0, 1))
                continue;
            // Got one!
            return p;
        }
        // Increment the list length
        int oldLen;
        do {
            oldLen = listLen_;
        } while (!CAS(&listLen_, oldLen,
                      oldLen + 1));
        // Allocate a new one
        HPRecType * p = new HPRecType;
        p->active_ = 1;
        p->pHazard_ = 0;
        // Push it to the front
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
 std::vector<Map *> rlist;