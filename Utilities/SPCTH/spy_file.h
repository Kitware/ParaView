/* Id
 *
 * spy_file.h - include file for spy_file.c, spyplt.c, etc.
 *
 * David Crawford
 * Computational Physics
 * Sandia National Laboratories
 * Albuquerque, New Mexico 87185
 *
 */

#ifndef __spy_file_h__
#define __spy_file_h__

#include <stdio.h>

#include "structured_mesh.h"
#include "stm_types.h"

/* Inhibit C++ name-mangling for functions but not for system calls. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Global variables describing the spy file */
#define MAX_DUMPS 100

typedef struct _SpyFileDump
{
  int Cycle;
  double Time;
  double Offset;
  struct _SpyFileDump *next;
} SpyFileDump;

typedef struct _SpyFile
{
  /* Flag that describes the byte order of doubles, ints, etc. */
  int intel_way; /* 1 = ordering for Intel processors,
                    0 = ordering for everyone else */
  double next_time;
  double last_time;
  double present_dt;
  double present_time;
  int present_cycle;
  Int next_cycle;
  int present_block;

  char **stm_command;
  int num_stm_commands;

  /* Global structure for the structured mesh data */
  Structured_Mesh_Data stm_data;

  /* List of variables to be saved */
  int NumSavedVariables;
  int *SavedVariables;
  char *SavedVariablesValid;
  char *SavedVariablesFileName;
  double *SavedVariablesOffset;

  FILE *in_file;
  FILE *out_file;
  int LNDumps;
  double LastOffset;
  double FirstGroup;
  double LastGroup;
  SpyFileDump *ThisDump;
  SpyFileDump *FirstDump;
  int SpyFileCurrent;  /* This needs to change if the file format changes in
                              such a way as to be incompatible with an earlier version */
  int SpyFileVersion;
  int SpyCompression; /* 0=rle, 1=gzip/gunzip? */
  int ndumps;
  int DumpCycle[MAX_DUMPS];
  double DumpTime[MAX_DUMPS];
  double DumpOffset[MAX_DUMPS];

  char SpyFileTitle[128];
} SpyFile;

/* Initialize and finalize spy file structure */
SpyFile* spy_initialize();
void spy_finalize(SpyFile* spy);

/* Set the filename used internally to a bunch of the functions */
void spy_setfilename(SpyFile* sp, const char *filename);

/* Routine opens a spy visualization file for input and
   reads the header information, returns 1 if error */
int spy_open_file_for_input(SpyFile* sp, const char *filename);

int spy_file_in(SpyFile* spy, double time);
void spy_read_variable_data(SpyFile* spy, int Variable_id);
double *** spy_GetField(Structured_Block_Data *blk, int field_id);

Structured_Block_Data *spy_NextBlock(SpyFile* spy);
Structured_Block_Data *spy_FirstBlock(SpyFile* spy);


#ifdef __cplusplus
}
#endif

#endif /* __spy_file_h__ */
