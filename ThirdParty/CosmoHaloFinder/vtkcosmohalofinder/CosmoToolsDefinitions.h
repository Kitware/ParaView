#ifndef COSMOLOGYTOOLSDEFINITIONS_H_
#define COSMOLOGYTOOLSDEFINITIONS_H_

// C/C++ required includes
#include <cinttypes>
#include <stdint.h>
#include <map>
#include <string>

// MPI library
#include <mpi.h>

// Type compatible with HACC
//#ifdef ID_64
  #define TYPE_IDS_64BITS
//#endif

#ifdef USEDIY
  // DIY library
  #include "diy.h"
#endif

/* Explicitly set precision for position/velocity
 * Behavior is controller by the user via a CMAKE build option.
 */
#ifdef TYPE_POSVEL_DOUBLE
  typedef double POSVEL_T;
  #define MPI_POSVEL_T MPI_DOUBLE

#ifdef USEDIY
  #define DIY_POSVEL_T DIY_DOUBLE
#endif

#else
  typedef float POSVEL_T;
  #define MPI_POSVEL_T MPI_FLOAT

#ifdef USEDIY
  #define DIY_POSVEL_T DIY_FLOAT
#endif

#endif

/* Explicitly set precision for potential
 * Behavior is controller by the user via a CMAKE build option.
 */
#ifdef TYPE_POTENTIAL_DOUBLE
  typedef double POTENTIAL_T;
  #define MPI_POTENTIAL_T MPI_DOUBLE

#ifdef USEDIY
  #define DIY_POTENTIAL_T DIY_DOUBLE
#endif

#else
  typedef float POTENTIAL_T;
  #define MPI_POTENTIAL_T MPI_FLOAT

#ifdef USEDIY
  #define DIY_POTENTIAL_T DIY_FLOAT
#endif

#endif

#ifdef TYPE_GRID_DOUBLE
  typedef double GRID_T;
  #define MPI_GRID_T MPI_DOUBLE

#ifdef USEDIY
  #define DIY_GRID_T DIY_DOUBLE
#endif

#else
  typedef float GRID_T;
  #define MPI_GRID_T MPI_FLOAT

#ifdef USEDIY
  #define DIY_GRID_T DIY_FLOAT
#endif

#endif

/* Explicitly set whether to use 64-bit or 32-bit integers for ID types.
 * Behavior is controller by the user via a CMAKE build option.
 */
#ifdef TYPE_IDS_64BITS
  typedef int64_t ID_T;
  #define MPI_ID_T MPI_INT64_T
  #define ID_T_FMT PRId64

#ifndef MPI_INT64_T
#define MPI_INT64_T (sizeof(long) == 8 ? MPI_LONG : MPI_LONG_LONG)
#endif

#ifdef USEDIY
  #define DIY_ID_T DIY_LONG_LONG
#endif

#else
  typedef int32_t ID_T;
  #define MPI_ID_T MPI_INT32_T
  #define ID_T_FMT PRId32

#ifdef USEDIY
  #define DIY_ID_T DIY_INT
#endif

#endif

/* Explicitly set the type for status/mask arrays
 * Behavior is hard-coded in this file.
 */
typedef int32_t STATUS_T;
#define MPI_STATUS_T MPI_INT32_T

#ifdef USEDIY
  #define DIY_STATUS_T DIY_INT
#endif

typedef uint16_t MASK_T;
#define MPI_MASK_T MPI_UINT16_T


// Generic integer/floating point types

/* Set whether default floating type precision is double or single */
#ifdef TYPE_REAL_DOUBLE
  typedef double REAL;
  #define MPI_REAL_T MPI_DOUBLE

#ifdef USEDIY
  #define DIY_REAL_T DIY_DOUBLE
#endif

#else
  typedef float REAL;
  #define MPI_REAL_T MPI_FLOAT

#ifdef USEDIY
  #define DIY_REAL_T DIY_FLOAT
#endif

#endif

/* Set whether to use 64 or 32 bit by default for integer types */
#ifdef TYPE_INT_64BITS
  typedef int64_t INTEGER;
  #define MPI_INTEGER_T MPI_INT64_T

#ifdef USEDIY
  #define DIY_INTEGER_T DIY_LONG_LONG
#endif

#else
  typedef int32_t INTEGER;
  #define MPI_INTEGER_T MPI_INT32_T

#ifdef USEDIY
  #define DIY_INTEGER_T DIY_INT
#endif

#endif

namespace cosmotk {
  /*
   * Define dictionary as key,value pair of strings. Used to store analysis
   * tool parameters
   */
  typedef std::map<std::string,std::string> Dictionary;
  typedef Dictionary::iterator DictionaryIterator;
}

#endif /* COSMOLOGYTOOLSDEFINITIONS_H_ */
