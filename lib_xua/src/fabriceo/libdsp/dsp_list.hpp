#ifndef _DSP_LIST_H_
#define _DSP_LIST_H_

#include <stdint.h>

#if defined(__XS2__) || defined(__XS3__)
#include <xs1.h>
#include <platform.h>
#endif


class dspLock {
    volatile unsigned lock;
public:
    dspLock() { lock = 0; }
    void aquire() { 
        unsigned ID;
        asm("get r11,id ; add %0,r11,1":"=r"(ID)::"r11");
        do { while ( lock ) { }; //wait for a free lock 
            lock = ID; //try to acquire
            //delay to cope with potential low-priority threads
            //that may already be in the process of issuing an stw instructuction
            asm volatile("nop;nop;nop;nop;nop;nop;nop");
        } while( lock != ID );
        }
    void release() { 
        lock = 0; 
        //memory barrier. some shared memory have been modified
        asm volatile("":::"memory");    
    }
};



//light solution to chain with a single pointer

template < class base > class dspChainItem { public:
    base * prevItem;
};

template < class base > class dspChain { public:
    volatile base * last;
    //each chain "base" requires its own lock
    static dspLock lock;

    dspChain()  { init(); }

    void init() { last = nullptr; }

    void append(base & item) {
        lock.aquire();
        item.prevItem = (base *)last;
        last = &item;
        lock.release();
    }

    base * next(base &item) {
        lock.aquire();
        base * next = nullptr;
        base * ptr = (base *)last;
        while (ptr && (ptr != &item)) {
            next = ptr;
            ptr = ptr->prevItem;
        }
        lock.release();
        return next;
    }

    void remove(base &item) {
        lock.aquire();
        if (last == &item) last = item.prevItem;
        else 
            next(item)->prevItem = item.prevItem;
        lock.release();
    }

    void clear() {
        lock.aquire();
        while (last) { 
            base * ptr = (base *)last;
            last = last->prevItem;
            ptr->prevItem = nullptr;
        }
        lock.release();
    }

    uint32_t count() {
        lock.aquire();
        uint32_t count = 0;
        base * ptr = (base *)last;
        while (ptr) { 
            count ++;
            ptr = ptr->prevItem;
        }
        lock.release();
        return count;
    }
};

//specialization of static memebers
#ifndef dspChain_base_chainLock
#define dspChain_base_chainLock 1
template< class base > dspLock dspChain<base>::lock;
#endif



#if 0 //not used

class dspListItem { public:
    //4words used to maintain the chain
    dspListItem * next;     //next object in the chain
    dspListItem * prev;     //prev object in the chain
    //constructor
    dspListItem() : next(nullptr), prev(nullptr) {}
};


//container for maintaing a list of items give as parameter
template < class base = dspListItem > class dspList { public:
    base * first;
    base * last;
    uint32_t count;

    void init() { 
        //dspLock lock;
        first = last = nullptr; 
        count = 0; }

    void clear() { 
        //dspLock lock;        
        base * item = last;
        while (item) {
            base * prev = item->prev;
            item->prev = item->next = nullptr;
            item = prev;
        }
        init();
    }

    void append(base& item) {
        //dspLock lock;
        item.prev  = last;
        item.next = nullptr;
        if (last) last->next =  &item;
        last = &item;
        if (first) first = &item;    
        count++; 
    }

    void remove(base&  item) {
        //dspLock lock;
        if (&item == first) first = item.next;
        else item.prev->next = item.next;
        if (&item == last) last = item.prev;
        else item.next->prev = item.prev;
    }

};


//the list of object type that may be in the list of object
enum class dspListObjectType {   
        empty,
        mainCore,
        otherCore,
        io,
        mem,
        gain,
        gainslew,
        filter,
        filterState,
        filterStateSlew,
        filterStateSlewVpu,
        filterStateSlewVpu8,
        biquad,
        delay,
};

//generic object belonging to a list

class dspListObject : public dspListItem { public:
    dspListObjectType type;     //type of this object, listed in enu class above
    void * self;                //point on the object data related to this item
    //constructor, each object supposed to give its type and "this" value
    dspListObject() : type(dspListObjectType::empty), self(this) {}
    dspListObject(dspListObjectType t, void * pthis) : 
        type(t), self(pthis) {  };
};


#endif

#endif //_DSP_LIST_H_
