//
//  main.cpp
//  lockfree
//
//  Created by Paul Callahan on 7/9/14.
//  Copyright (c) 2014 Paul Callahan. All rights reserved.
//

#include <string>
#include <iostream>
#include <boost/unordered_map.hpp>
//#include <atomic>
//#include <memory>
#include <vector>


//#include "NonBlockingReadMapAtomic.hpp"

//#include "NonBlockingReadMapSharedPtr.hpp"
//#include "Hazard.h"


#include "NonBlockingReadMapCAS.hpp"
//#include "NonBlockingReadMapRefCount.hpp"

//#include "NonBlockingReadMapCAS.hpp"

//#include <boost/thread/tss.hpp>


//typedef boost::unordered_map<std::string, std::string> StringMap;

//static boost::thread_specific_ptr<std::vector<StringMap*>> rlist;


struct MyObj {
    char * c;
    int i;
};

void assignAndIncrement(MyObj **dest, MyObj **src ) {
    //begin atomic
    *dest = *src;
    (*src)->i++;
    //end atomic
}



int main()
{
    MyObj * src = new MyObj;
    src->c = "asdf";
    src->i = 0;
    
    MyObj *dest = new MyObj;
    dest->c = "lala";
    dest->i = 10;
    
    assignAndIncrement(&dest, &src);
    
    std::cout << dest->c << " " << dest->i << " " << dest << "\n";
    std::cout << src->c << " " << src->i  << " " << src << (src == dest) << "\n";
    
    
    NonBlockingReadMapCAS map;
    
    map.put("xyz", "123");
    
    std::cout << map.get("xyz") << "\n";
    
    map.put("xyz2", "1232");
    
    
    
    //std::string key("lala");
    //std::string value("xyz");
/*
    NonBlockingReadMapSharedPtr nbrmap;
    

    
    nbrmap.put(key, value);
    nbrmap.put("lala2", "xyz2");
    nbrmap.put("lala3", "xyz3");
    //nbrmap.insert(std::string("lala2"), std::string("xyz2"));
    
    
    std::cout << nbrmap.get(key) << "\n";
    std::cout << nbrmap.get("lala2") << "\n";
    std::cout << nbrmap.get("lala3") << "\n";
    
    std::cout.flush();
    nbrmap.clear();
    
    //nbrmap.get(key);
    
    
    
    nbrmap.put("lala4", "xyz4");
    nbrmap.put("lala5", "xyz5");
    
    std::cout << nbrmap.get("lala4") << "\n";
    std::cout << nbrmap.get("lala5") << "\n";

    
    std::shared_ptr<StringMap> pMap;
    std::atomic_store(&pMap, std::make_shared<StringMap>());
    std::cout << "unordered_map ptr is lock free:" << std::boolalpha << std::atomic_is_lock_free(&pMap) << "\n";
    
    std::shared_ptr<std::string> pString = std::make_shared<std::string>();
    std::cout << "string ptr is lock free:" << std::boolalpha << std::atomic_is_lock_free(&pString) << "\n";
    
    std::shared_ptr<int> pInt = std::make_shared<int>();
    std::cout << "int ptr is lock free:" << std::boolalpha << std::atomic_is_lock_free(&pInt) << "\n";

    
    std::atomic<StringMap*> apMap;
    std::cout << "atomic ptr is lock free:" << std::boolalpha << apMap.is_lock_free() << "\n";
  */  
    return 0;
}

