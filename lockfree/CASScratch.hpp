//
//  CASScratch.hpp
//  lockfree
//
//  Created by Paul Callahan on 7/15/14.
//  Copyright (c) 2014 Paul Callahan. All rights reserved.
//

#ifndef lockfree_CASScratch_hpp
#define lockfree_CASScratch_hpp

 struct MyThing {
 std::atomic<void *> pSomeObj;
 std::atomic<uint64_t> counter;
 //__attribute__((aligned(16)));
 };
 
 bool AtomicCopyAndIncrement(MyThing **dest, MyThing *src) {
 //begin atomic
 dest = &src;
 src->counter++;
 //end atomic
 return true;
 }
 
 void blarg() {
 
 MyThing* src;
 MyThing* dest;
 
 AtomicCopyAndIncrement(&dest, src);
 
 //std::atomic<MyThing*> asrc;
 //std::atomic<MyThing*> adest;
 
 
 }
/*
bool AtmoicCopyAndIncrement(StringMapHazardPointer *copyFrom)
{
    bool swap_result;
    __asm__ __volatile__
    (
     "lock cmpxchg16b %0;"  // cmpxchg16b sets ZF on success
     "setz       %3;"  // if ZF set, set cas_result to 1
     
     : "+m" (*this), "+a" (this->fStringMap), "+d" (this->fCounter), "=q" (swap_result)
     : "b" (copyFrom->fStringMap), "c" (copyFrom->fCounter +1)
     : "cc", "memory"
     );
    return swap_result;
}
*/

 bool AtmoicCopyAndIncrement(MyThing *dest, MyThing *src)
 {
 bool swap_result;
 __asm__ __volatile__
 (
 "lock cmpxchg16b %0;"  // cmpxchg16b sets ZF on success
 "setz       %3;"  // if ZF set, set cas_result to 1
 
 : "+m" (*dest), "+a" (dest->pSomeObj), "+d" (dest->counter), "=q" (swap_result)
 : "b" (src->pSomeObj), "c" (src->counter +1)
 : "cc", "memory"
 );
 return swap_result;
 }
 
 bool AtmoicDecrement(MyThing *dest)
 {
 bool swap_result;
 __asm__ __volatile__
 (
 "lock cmpxchg16b %0;"  // cmpxchg16b sets ZF on success
 "setz       %3;"  // if ZF set, set cas_result to 1
 
 : "+m" (*dest), "+a" (dest->pSomeObj), "+d" (dest->counter), "=q" (swap_result)
 : "b" (dest->pSomeObj), "c" (dest->counter -1)
 : "cc", "memory"
 );
 return swap_result;
 }
 
 


#endif
