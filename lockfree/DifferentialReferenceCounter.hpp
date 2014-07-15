//
//  DifferentialReferenceCounter.hpp
//  lockfree
//
//  Created by Paul Callahan on 7/13/14.
//  Copyright (c) 2014 Paul Callahan. All rights reserved.
//

#pragma once

#include <atomic>

struct drc_base
{
    // low word is 'strong inner' counter
    // high word is 'basic inner' counter
    std::atomic_dword state;
    
    // destruction function
    void (*dtor)(drc_base*);
};