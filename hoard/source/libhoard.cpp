/* -*- C++ -*- */

/*
  The Hoard Multiprocessor Memory Allocator
  www.hoard.org

  Author: Emery Berger, http://www.cs.umass.edu/~emery
 
  Copyright (c) 1998-2012 Emery Berger

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * @file   libhoard.cpp
 * @brief  This file replaces malloc etc. in your application.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 */
#define _POSIX_C_SOURCE 200809L

#include "heaplayers.h"
using namespace HL;

#include <new>
//#include "hash_maps.h"
#include <numa.h>
#include <time.h>
#include <inttypes.h>
#include "heaplayers/wrappers/mmapwrapper.h"
//#include <migration.h>


pthread_mutex_t mutex;

#define MAXPAGELISTSZ 1024*1024*100

// The undef below ensures that any pthread_* calls get strong
// linkage.  Otherwise, our versions here won't replace them.  It is
// IMPERATIVE that this line appear before any files get included.

#undef __GXX_WEAK__ 

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN

// Maximize the degree of inlining.
#pragma inline_depth(255)

// Turn inlining hints into requirements.
#define inline __forceinline
#pragma warning(disable:4273)
#pragma warning(disable: 4098)  // Library conflict.
#pragma warning(disable: 4355)  // 'this' used in base member initializer list.
#pragma warning(disable: 4074)	// initializers put in compiler reserved area.
#pragma warning(disable: 6326)  // comparison between constants.

#endif

#if HOARD_NO_LOCK_OPT
// Disable lock optimization.
volatile bool anyThreadCreated = true;
#else
// The normal case. See heaplayers/spinlock.h.
volatile bool anyThreadCreated = false;
#endif

namespace Hoard {
  
  // HOARD_MMAP_PROTECTION_MASK defines the protection flags used for
  // freshly-allocated memory. The default case is that heap memory is
  // NOT executable, thus preventing the class of attacks that inject
  // executable code on the heap.
  // 
  // While this is not recommended, you can define HL_EXECUTABLE_HEAP as
  // 1 in heaplayers/heaplayers.h if you really need to (i.e., you're
  // doing dynamic code generation into malloc'd space).
  
#if HL_EXECUTABLE_HEAP
#define HOARD_MMAP_PROTECTION_MASK (PROT_READ | PROT_WRITE | PROT_EXEC)
#else
#define HOARD_MMAP_PROTECTION_MASK (PROT_READ | PROT_WRITE)
#endif

}

#include "hoardtlab.h"

//
// The base Hoard heap.
//


using namespace Hoard;

/// Maintain a single instance of the main Hoard heap.

HoardHeapType * getMainHoardHeap (void) {
  // This function is C++ magic that ensures that the heap is
  // initialized before its first use. First, allocate a static buffer
  // to hold the heap.

  static double thBuf[sizeof(HoardHeapType) / sizeof(double) + 1];

  // Now initialize the heap into that buffer.
  static HoardHeapType * th = new (thBuf) HoardHeapType;
  return th;
}

TheCustomHeapType * getCustomHeap();

extern "C" {
 
  static size_t xxactalloc;
  static size_t xxtotalloc;
 
  void addallocsz(size_t sz){	 
		xxactalloc += sz;
		xxtotalloc += sz;
  }	

  void suballocsz(size_t sz){	 
		xxactalloc -= sz;
  }		 	
  void printallocsz(size_t sz){
	fprintf(stderr,"active alloc size %zu, tot alloc %zu\n",xxactalloc, xxtotalloc);	
  }


 	 

void * xxmalloc (size_t sz) {

    TheCustomHeapType * h = getCustomHeap();
    void * ptr = h->malloc (sz);
#ifdef _DEBUG
    addallocsz(sz);
    printallocsz(sz);
#endif
    //fprintf(stdout,"recording address %u\n",sz);
    //record_addr(ptr,sz);
    return ptr;
  }

void xxfree (void * ptr) {
#ifdef _DEBUG
    if(ptr)
      suballocsz(getCustomHeap()->getSize (ptr));	
#endif
    getCustomHeap()->free (ptr);
}

void* xx_reserve(size_t sz){
	return xxmalloc (1);
}


/*Function to get the diff between large map range*/
size_t xx_get_largemmap_range (void * start, void *end) {

    size_t diff = 0; //MmapWrapper::get_large_mmap_range(start, end);	
    return diff;	
}

size_t xxmalloc_usable_size (void * ptr) {
    return getCustomHeap()->getSize (ptr);
}

void xxmalloc_lock() {
    // Undefined for Hoard.
}

void xxmalloc_unlock() {
    // Undefined for Hoard.
}

}
