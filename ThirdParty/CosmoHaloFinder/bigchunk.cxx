/*
 * Copyright (C) 2011 UChicago Argonne, LLC
 * All Rights Reserved
 *
 * Permission to use, reproduce, prepare derivative works, and to redistribute
 * to others this software, derivatives of this software, and future versions
 * of this software as well as its documentation is hereby granted, provided
 * that this notice is retained thereon and on all copies or modifications.
 * This permission is perpetual, world-wide, and provided on a royalty-free
 * basis. UChicago Argonne, LLC and all other contributors make no
 * representations as to the suitability and operability of this software for
 * any purpose. It is provided "as is" without express or implied warranty.
 *
 * Portions of this software are copyright by UChicago Argonne, LLC. Argonne
 * National Laboratory with facilities in the state of Illinois, is owned by
 * The United States Government, and operated by UChicago Argonne, LLC under
 * provision of a contract with the Department of Energy.
 *
 * PORTIONS OF THIS SOFTWARE  WERE PREPARED AS AN ACCOUNT OF WORK SPONSORED BY
 * AN AGENCY OF THE UNITED STATES GOVERNMENT. NEITHER THE UNITED STATES
 * GOVERNMENT NOR ANY AGENCY THEREOF, NOR THE UNIVERSITY OF CHICAGO, NOR ANY OF
 * THEIR EMPLOYEES OR OFFICERS, MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR
 * ASSUMES ANY LEGAL LIABILITY OR RESPONSIBILITY FOR THE ACCURACY,
 * COMPLETENESS, OR USEFULNESS OF ANY INFORMATION, APPARATUS, PRODUCT, OR
 * PROCESS DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT INFRINGE PRIVATELY
 * OWNED RIGHTS. REFERENCE HEREIN TO ANY SPECIFIC COMMERCIAL PRODUCT, PROCESS,
 * OR SERVICE BY TRADE NAME, TRADEMARK, MANUFACTURER, OR OTHERWISE, DOES NOT
 * NECESSARILY CONSTITUTE OR IMPLY ITS ENDORSEMENT, RECOMMENDATION, OR FAVORING
 * BY THE UNITED STATES GOVERNMENT OR ANY AGENCY THEREOF. THE VIEW AND OPINIONS
 * OF AUTHORS EXPRESSED HEREIN DO NOT NECESSARILY STATE OR REFLECT THOSE OF THE
 * UNITED STATES GOVERNMENT OR ANY AGENCY THEREOF.
 *
 * Author: Hal Finkel <hfinkel@anl.gov>
 */

#include "bigchunk.h"
#include <stdio.h>

#ifdef _OPENMP
#include <omp.h>
#endif

static void *_bigchunk_ptr = (void *) 0;
static size_t _bigchunk_last_alloc = (size_t) -1;
static size_t _bigchunk_sz = 0;
static size_t _bigchunk_used = 0;
static size_t _bigchunk_total = 0;
static const size_t min_alloc = 32; /* for alignment; must be 2^n */

static size_t _bigchunk_warnings = 0;
static const size_t _bigchunk_max_warnings = 2;

#ifdef _OPENMP
static int _bigchunk_lck_init = 0;
static omp_lock_t _bigchunk_lck;
#define BC_LOCK \
  do { \
    if (_bigchunk_lck_init) omp_set_lock(&_bigchunk_lck); \
  } while (0)

#define BC_UNLOCK \
  do { \
    if (_bigchunk_lck_init) omp_unset_lock(&_bigchunk_lck); \
  } while (0)
#define BC_INIT_LOCK \
  do { \
    omp_init_lock(&_bigchunk_lck); \
    _bigchunk_lck_init = 1; \
  } while (0)
#else
#define BC_LOCK
#define BC_UNLOCK
#define BC_INIT_LOCK
#endif

void *bigchunk_malloc(size_t sz)
{
  BC_LOCK;
  if (sz < min_alloc)
    sz = min_alloc;
  else {
    size_t e = sz - (sz & ~(min_alloc-1));
    if (e != 0) sz += min_alloc - e;
  }

  if (_bigchunk_sz - _bigchunk_used >= sz) {
    /* this fits in the big chunk */
    // casting to an unsigned char so that we can do pointer arithmetic.
    unsigned char* chunkptr = static_cast<unsigned char*>(_bigchunk_ptr);
    void *r = chunkptr + _bigchunk_used;
    _bigchunk_last_alloc = _bigchunk_used;
    _bigchunk_used += sz;
    _bigchunk_total += sz;
    BC_UNLOCK;
    return r;
  } else if (_bigchunk_used == 0 && _bigchunk_sz > 0) {
    /* this is smaller than the big chunk, but nothing
      is currently using the big chunk, so just make
      the big chunk bigger.
    */

    void *new_chuck = realloc(_bigchunk_ptr, sz);
    if (new_chuck) {
      _bigchunk_ptr = new_chuck;
      _bigchunk_last_alloc = 0;
      _bigchunk_sz = sz;
      _bigchunk_used = sz;
      _bigchunk_total += sz;
      void *r = _bigchunk_ptr;
      BC_UNLOCK;
      return r;
    }
        }

  if (_bigchunk_sz > 0) {
    if (++_bigchunk_warnings <= _bigchunk_max_warnings)
      fprintf(stderr, "WARNING: bigchunk: allocation of %zu bytes has been requested,"
                      " only %zu of %zu remain!%s\n",
          sz, _bigchunk_sz - _bigchunk_used, _bigchunk_sz,
                      _bigchunk_warnings == _bigchunk_max_warnings ?
                        " (future warnings suppressed)" : "");
  }

  void *ptr = malloc(sz);
  if (ptr) _bigchunk_total += sz;
  BC_UNLOCK;
  return ptr;
}

void bigchunk_free(void *ptr)
{
  BC_LOCK;
  // casting to an unsigned char so that we can do pointer arithmetic.
  unsigned char* chunkptr = static_cast<unsigned char*>(_bigchunk_ptr);
  if (ptr < _bigchunk_ptr || ptr >= chunkptr + _bigchunk_sz) {
    free(ptr);
  } else if (_bigchunk_last_alloc != (size_t) -1 &&
                   ptr == chunkptr + _bigchunk_last_alloc) {
    /* this is the last allocation, so we can undo that easily... */
    _bigchunk_used = _bigchunk_last_alloc;
    _bigchunk_last_alloc = (size_t) -1;
  }
  BC_UNLOCK;
}

void bigchunk_reset()
{
  BC_LOCK;
  _bigchunk_used = 0;
  _bigchunk_total = 0;
  _bigchunk_last_alloc = (size_t) -1;
  _bigchunk_warnings = 0;
  BC_UNLOCK;
}

void bigchunk_init(size_t sz)
{
  BC_INIT_LOCK;
  BC_LOCK;
  _bigchunk_ptr = malloc(sz);
  if (_bigchunk_ptr) {
    _bigchunk_sz = sz;
    _bigchunk_used = 0;
    _bigchunk_last_alloc = (size_t) -1;
  }
  BC_UNLOCK;
}

void bigchunk_cleanup()
{
  BC_LOCK;
  free(_bigchunk_ptr);
  _bigchunk_ptr = 0;
  _bigchunk_sz = 0;
  _bigchunk_used = 0;
  _bigchunk_total = 0;
  _bigchunk_last_alloc = (size_t) -1;
  _bigchunk_warnings = 0;
  BC_UNLOCK;
}

size_t bigchunk_get_size()
{
  BC_LOCK;
  size_t r = _bigchunk_sz;
  BC_UNLOCK;
  return r;
}


size_t bigchunk_get_total()
{
  BC_LOCK;
  size_t r = _bigchunk_total;
  BC_UNLOCK;
  return r;
}

size_t bigchunk_get_used()
{
  BC_LOCK;
  size_t r = _bigchunk_used;
  BC_UNLOCK;
  return r;
}
