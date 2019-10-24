/*
   Global definitions, etc for the Datamine reader
   project. See dmfile.hxx for full description.

   Revisions:
   99-04-12: Written by Jeremy Maccelari, visualn@iafrica.com
   99-05-03: added byte swapping declaration, JM
   00-06-15: added IAC_DEBUG preprocessor definition, AWD
*/

#ifndef DM_HXX
#define DM_HXX

#ifdef SGI
// Define bool type for SGI
typedef enum { false, true } bool;
#endif

#define NBITS_IN_BYTE 8
#define SIZE_OF_FLOAT 4
#define SIZE_OF_WORD 4
#define SIZE_OF_BUFFER 2048

#define SIZE_OF_DOUBLE 8
#define SIZE_OF_WORD64 8
#define SIZE_OF_BUFF64 4096

// Define debugging constant to get info printed to console.
//#define IAC_DEBUG

#ifdef IAC_DEBUG
#define DEBUG_PRINT(A)                                                                             \
  {                                                                                                \
    fprintf(stdout, "Read DM: %s\n", (A));                                                         \
    fflush(stdout);                                                                                \
  }
#else
#define DEBUG_PRINT(A) /* noop */
#endif

void VISswap_4_byte_ptr(char* ptr);

enum FileTypes
{
  invalid,
  perimeter,
  plot,
  drillhole,
  blockmodel,
  wframetriangle,
  wframepoints,
  sectiondefinition,
  catalogue,
  scheduling,
  results,
  rosette,
  drivestats,
  point,
  dependency,
  plotterpen,
  plotfilter,
  stopesummary,
  validation
};

#endif // of ifdef DM_HXX
