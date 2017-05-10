/*=========================================================================
 *
 *  Program:   Visualization Toolkit
 *  Module:    cdilib.c
 *
 *  Copyright (c) 2015 Uwe Schulzweida, MPI-M Hamburg
 *  All rights reserved.
 *  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
 *
 *     This software is distributed WITHOUT ANY WARRANTY; without even
 *     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *     PURPOSE.  See the above copyright notice for more information.
 *
 *  =========================================================================*/
// .NAME vtkCDIReader - reads ICON/CDI netCDF data sets
// .SECTION Description
// vtkCDIReader is based on the vtk MPAS netCDF reader developed by
// Christine Ahrens (cahrens@lanl.gov). The plugin reads all ICON/CDI
// netCDF data sets with point and cell variables, both 2D and 3D. It allows
// spherical (standard), as well as equidistant cylindrical and Cassini projection.
// 3D data can be visualized using slices, as well as 3D unstructured mesh. If
// bathymetry information (wet_c) is present in the data, this can be used for
// masking out continents. For more information, also check out our ParaView tutorial:
// https://www.dkrz.de/Nutzerportal-en/doku/vis/sw/paraview
//
// .SECTION Caveats
// The integrated visualization of performance data is not yet fully developed
// and documented. If interested in using it, see the following presentation
// https://www.dkrz.de/about/media/galerie/Vis/performance/perf-vis
// and/or contact Niklas Roeber at roeber@dkrz.de
//
// .SECTION Thanks
// Thanks to Uwe Schulzweida for the CDI code (uwe.schulzweida@mpimet.mpg.de)
// Thanks to Moritz Hanke for the sorting code (hanke@dkrz.de)

#define HAVE_LIBNETCDF

#ifdef _ARCH_PWR6
#pragma options nostrict
#endif

#if defined (HAVE_CONFIG_H)
# include "config.h"
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#if defined(__linux__) || defined(__APPLE__)
  #include <unistd.h>
  #include <stdbool.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <sys/time.h>
  #ifndef _SX
    #include <aio.h>
  #endif
#elif defined(_WIN32)
  #define inline __inline
  #define __func__ __FUNCTION__
  #include <io.h>
  #include <time.h>
  typedef int bool;
  #define false 0
  #define true 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#if defined (HAVE_MMAP)
# include <sys/mman.h>
#endif

#if defined (HAVE_LIBPTHREAD)
# include <pthread.h>
#endif

#if defined (HAVE_LIBSZ)
# include <szlib.h>
#endif

static void tableDefault(void) {}

#ifndef _ERROR_H
#define _ERROR_H

#include <stdarg.h>
#include <stdlib.h>

#ifndef WITH_CALLER_NAME
#define WITH_CALLER_NAME
#endif

#define _FATAL 1
#define _VERBOSE 2
#define _DEBUG 4

extern int _ExitOnError;
extern int _Verbose;
extern int _Debug;

void SysError_(const char *caller, const char *fmt, ...);
void Error_(const char *caller, const char *fmt, ...);
void Warning_(const char *caller, const char *fmt, ...);

void cdiWarning(const char *caller, const char *fmt, va_list ap);
void Message_(const char *caller, const char *fmt, ...);

#if defined WITH_CALLER_NAME
#define SysError(...) SysError_(__func__, __VA_ARGS__)
#define Errorc(...) Error_( caller, __VA_ARGS__)
#define Error(...) Error_(__func__, __VA_ARGS__)
#define Warning(...) Warning_(__func__, __VA_ARGS__)
#define Messagec(...) Message_( caller, __VA_ARGS__)
#define Message(...) Message_(__func__, __VA_ARGS__)
#else
#define SysError(...) SysError_((void *), __VA_ARGS__)
#define Errorc(...) Error_((void *), __VA_ARGS__)
#define Error(...) Error_((void *), __VA_ARGS__)
#define Warning(...) Warning_((void *), __VA_ARGS__)
#define Messagec(...) Message_((void *), __VA_ARGS__)
#define Message(...) Message_((void *), __VA_ARGS__)
#endif


#ifndef __GNUC__
#define __attribute__(x)
#endif

void cdiAbortC(const char *caller, const char *filename,
               const char *functionname, int line,
               const char *errorString, ... )
  __attribute__((noreturn));
#define xabortC(caller,...) cdiAbortC(caller, __FILE__, __func__, __LINE__, __VA_ARGS__ )
#define xabort(...) cdiAbortC(NULL, __FILE__, __func__, __LINE__, __VA_ARGS__ )
#define cdiAbort(file,func,line,...) cdiAbortC(NULL, (file), (func), (line), __VA_ARGS__)

#define xassert(arg) do { \
    if ((arg)) { } else { \
      xabort("assertion `" #arg "` failed");} \
  } while(0)

void
cdiAbortC_serial(const char *caller, const char *filename,
                 const char *functionname, int line,
                 const char *errorString, va_list ap)
  __attribute__((noreturn));

#endif

#ifndef CDI_H_
#define CDI_H_

#include <stdio.h>
#include <sys/types.h>





#define CDI_MAX_NAME 256

#define CDI_UNDEFID -1
#define CDI_GLOBAL -1



#define CDI_BIGENDIAN 0
#define CDI_LITTLEENDIAN 1
#define CDI_PDPENDIAN 2

#define CDI_REAL 1
#define CDI_COMP 2
#define CDI_BOTH 3



#define CDI_NOERR 0
#define CDI_EEOF -1
#define CDI_ESYSTEM -10
#define CDI_EINVAL -20
#define CDI_EUFTYPE -21
#define CDI_ELIBNAVAIL -22
#define CDI_EUFSTRUCT -23
#define CDI_EUNC4 -24
#define CDI_ELIMIT -99



#define FILETYPE_UNDEF -1
#define FILETYPE_GRB 1
#define FILETYPE_GRB2 2
#define FILETYPE_NC 3
#define FILETYPE_NC2 4
#define FILETYPE_NC4 5
#define FILETYPE_NC4C 6
#define FILETYPE_SRV 7
#define FILETYPE_EXT 8
#define FILETYPE_IEG 9



#define COMPRESS_NONE 0
#define COMPRESS_SZIP 1
#define COMPRESS_GZIP 2
#define COMPRESS_BZIP2 3
#define COMPRESS_ZIP 4
#define COMPRESS_JPEG 5



#define DATATYPE_PACK 0
#define DATATYPE_PACK1 1
#define DATATYPE_PACK2 2
#define DATATYPE_PACK3 3
#define DATATYPE_PACK4 4
#define DATATYPE_PACK5 5
#define DATATYPE_PACK6 6
#define DATATYPE_PACK7 7
#define DATATYPE_PACK8 8
#define DATATYPE_PACK9 9
#define DATATYPE_PACK10 10
#define DATATYPE_PACK11 11
#define DATATYPE_PACK12 12
#define DATATYPE_PACK13 13
#define DATATYPE_PACK14 14
#define DATATYPE_PACK15 15
#define DATATYPE_PACK16 16
#define DATATYPE_PACK17 17
#define DATATYPE_PACK18 18
#define DATATYPE_PACK19 19
#define DATATYPE_PACK20 20
#define DATATYPE_PACK21 21
#define DATATYPE_PACK22 22
#define DATATYPE_PACK23 23
#define DATATYPE_PACK24 24
#define DATATYPE_PACK25 25
#define DATATYPE_PACK26 26
#define DATATYPE_PACK27 27
#define DATATYPE_PACK28 28
#define DATATYPE_PACK29 29
#define DATATYPE_PACK30 30
#define DATATYPE_PACK31 31
#define DATATYPE_PACK32 32
#define DATATYPE_CPX32 64
#define DATATYPE_CPX64 128
#define DATATYPE_FLT32 132
#define DATATYPE_FLT64 164
#define DATATYPE_INT8 208
#define DATATYPE_INT16 216
#define DATATYPE_INT32 232
#define DATATYPE_UINT8 308
#define DATATYPE_UINT16 316
#define DATATYPE_UINT32 332


#define DATATYPE_INT 251
#define DATATYPE_FLT 252
#define DATATYPE_TXT 253
#define DATATYPE_CPX 254
#define DATATYPE_UCHAR 255
#define DATATYPE_LONG 256



#define CHUNK_AUTO 1
#define CHUNK_GRID 2
#define CHUNK_LINES 3



#define GRID_GENERIC 1
#define GRID_GAUSSIAN 2
#define GRID_GAUSSIAN_REDUCED 3
#define GRID_LONLAT 4
#define GRID_SPECTRAL 5
#define GRID_FOURIER 6
#define GRID_GME 7
#define GRID_TRAJECTORY 8
#define GRID_UNSTRUCTURED 9
#define GRID_CURVILINEAR 10
#define GRID_LCC 11
#define GRID_LCC2 12
#define GRID_LAEA 13
#define GRID_SINUSOIDAL 14
#define GRID_PROJECTION 15



#define ZAXIS_SURFACE 0
#define ZAXIS_GENERIC 1
#define ZAXIS_HYBRID 2
#define ZAXIS_HYBRID_HALF 3
#define ZAXIS_PRESSURE 4
#define ZAXIS_HEIGHT 5
#define ZAXIS_DEPTH_BELOW_SEA 6
#define ZAXIS_DEPTH_BELOW_LAND 7
#define ZAXIS_ISENTROPIC 8
#define ZAXIS_TRAJECTORY 9
#define ZAXIS_ALTITUDE 10
#define ZAXIS_SIGMA 11
#define ZAXIS_MEANSEA 12
#define ZAXIS_TOA 13
#define ZAXIS_SEA_BOTTOM 14
#define ZAXIS_ATMOSPHERE 15
#define ZAXIS_CLOUD_BASE 16
#define ZAXIS_CLOUD_TOP 17
#define ZAXIS_ISOTHERM_ZERO 18
#define ZAXIS_SNOW 19
#define ZAXIS_LAKE_BOTTOM 20
#define ZAXIS_SEDIMENT_BOTTOM 21
#define ZAXIS_SEDIMENT_BOTTOM_TA 22
#define ZAXIS_SEDIMENT_BOTTOM_TW 23
#define ZAXIS_MIX_LAYER 24
#define ZAXIS_REFERENCE 25



enum {
  SUBTYPE_TILES = 0
};

#define MAX_KV_PAIRS_MATCH 10

typedef struct {
  int nAND;
  int key_value_pairs[2][MAX_KV_PAIRS_MATCH];
} subtype_query_t;





#define TIME_CONSTANT 0
#define TIME_VARIABLE 1



#define TSTEP_CONSTANT 0
#define TSTEP_INSTANT 1
#define TSTEP_AVG 2
#define TSTEP_ACCUM 3
#define TSTEP_MAX 4
#define TSTEP_MIN 5
#define TSTEP_DIFF 6
#define TSTEP_RMS 7
#define TSTEP_SD 8
#define TSTEP_COV 9
#define TSTEP_RATIO 10
#define TSTEP_RANGE 11
#define TSTEP_INSTANT2 12
#define TSTEP_INSTANT3 13



#define TAXIS_ABSOLUTE 1
#define TAXIS_RELATIVE 2
#define TAXIS_FORECAST 3



#define TUNIT_SECOND 1
#define TUNIT_MINUTE 2
#define TUNIT_QUARTER 3
#define TUNIT_30MINUTES 4
#define TUNIT_HOUR 5
#define TUNIT_3HOURS 6
#define TUNIT_6HOURS 7
#define TUNIT_12HOURS 8
#define TUNIT_DAY 9
#define TUNIT_MONTH 10
#define TUNIT_YEAR 11



#define CALENDAR_STANDARD 0
#define CALENDAR_PROLEPTIC 1
#define CALENDAR_360DAYS 2
#define CALENDAR_365DAYS 3
#define CALENDAR_366DAYS 4
#define CALENDAR_NONE 5

#define CDI_UUID_SIZE 16


typedef struct CdiParam { int discipline; int category; int number; } CdiParam;



typedef struct CdiIterator CdiIterator;
typedef struct CdiGribIterator CdiGribIterator;



void cdiReset(void);

const char *cdiStringError(int cdiErrno);

void cdiDebug(int debug);

const char *cdiLibraryVersion(void);
void cdiPrintVersion(void);

int cdiHaveFiletype(int filetype);

void cdiDefMissval(double missval);
double cdiInqMissval(void);
void cdiDefGlobal(const char *string, int val);

int namespaceNew(void);
void namespaceSetActive(int namespaceID);
int namespaceGetActive(void);
void namespaceDelete(int namespaceID);

void cdiParamToString(int param, char *paramstr, int maxlen);

void cdiDecodeParam(int param, int *pnum, int *pcat, int *pdis);
int cdiEncodeParam(int pnum, int pcat, int pdis);

void cdiDecodeDate(int date, int *year, int *month, int *day);
int cdiEncodeDate(int year, int month, int day);

void cdiDecodeTime(int time, int *hour, int *minute, int *second);
int cdiEncodeTime(int hour, int minute, int second);

int cdiGetFiletype(const char *path, int *byteorder);


int streamOpenRead(const char *path);


int streamOpenWrite(const char *path, int filetype);

int streamOpenAppend(const char *path);


void streamClose(int streamID);


void streamSync(int streamID);


void streamDefVlist(int streamID, int vlistID);


int streamInqVlist(int streamID);


int streamInqFiletype(int streamID);


void streamDefByteorder(int streamID, int byteorder);


int streamInqByteorder(int streamID);


void streamDefCompType(int streamID, int comptype);


int streamInqCompType(int streamID);


void streamDefCompLevel(int streamID, int complevel);


int streamInqCompLevel(int streamID);


int streamDefTimestep(int streamID, int tsID);


int streamInqTimestep(int streamID, int tsID);


int streamInqCurTimestepID(int streamID);

const char *streamFilename(int streamID);
const char *streamFilesuffix(int filetype);

off_t streamNvals(int streamID);

int streamInqNvars ( int streamID );




void streamWriteVar(int streamID, int varID, const double data[], int nmiss);
void streamWriteVarF(int streamID, int varID, const float data[], int nmiss);


void streamReadVar(int streamID, int varID, double data[], int *nmiss);
void streamReadVarF(int streamID, int varID, float data[], int *nmiss);


void streamWriteVarSlice(int streamID, int varID, int levelID, const double data[], int nmiss);
void streamWriteVarSliceF(int streamID, int varID, int levelID, const float data[], int nmiss);


void streamReadVarSlice(int streamID, int varID, int levelID, double data[], int *nmiss);
void streamReadVarSliceF(int streamID, int varID, int levelID, float data[], int *nmiss);

void streamWriteVarChunk(int streamID, int varID, const int rect[3][2], const double data[], int nmiss);




void streamDefRecord(int streamID, int varID, int levelID);
void streamInqRecord(int streamID, int *varID, int *levelID);
void streamWriteRecord(int streamID, const double data[], int nmiss);
void streamWriteRecordF(int streamID, const float data[], int nmiss);
void streamReadRecord(int streamID, double data[], int *nmiss);
void streamReadRecordF(int streamID, float data[], int *nmiss);
void streamCopyRecord(int streamIDdest, int streamIDsrc);

void streamInqGRIBinfo(int streamID, int *intnum, float *fltnum, off_t *bignum);





CdiIterator *cdiIterator_new(const char *path);
CdiIterator *cdiIterator_clone(CdiIterator *me);
char *cdiIterator_serialize(CdiIterator *me);
CdiIterator *cdiIterator_deserialize(const char *description);
void cdiIterator_print(CdiIterator *me, FILE *stream);
void cdiIterator_delete(CdiIterator *me);


int cdiIterator_nextField(CdiIterator *me);



char *cdiIterator_inqStartTime(CdiIterator *me);
char *cdiIterator_inqEndTime(CdiIterator *me);
char *cdiIterator_inqVTime(CdiIterator *me);
int cdiIterator_inqLevelType(CdiIterator *me, int levelSelector, char **outName_optional, char **outLongName_optional, char **outStdName_optional, char **outUnit_optional);
int cdiIterator_inqLevel(CdiIterator *me, int levelSelector, double *outValue1_optional, double *outValue2_optional);
int cdiIterator_inqLevelUuid(CdiIterator *me, int *outVgridNumber_optional, int *outLevelCount_optional, unsigned char outUuid_optional[CDI_UUID_SIZE]);
CdiParam cdiIterator_inqParam(CdiIterator *me);
int cdiIterator_inqDatatype(CdiIterator *me);
int cdiIterator_inqTsteptype(CdiIterator *me);
char *cdiIterator_inqVariableName(CdiIterator *me);
int cdiIterator_inqGridId(CdiIterator *me);


void cdiIterator_readField(CdiIterator *me, double data[], size_t *nmiss_optional);
void cdiIterator_readFieldF(CdiIterator *me, float data[], size_t *nmiss_optional);




CdiGribIterator *cdiGribIterator_clone(CdiIterator *me);
void cdiGribIterator_delete(CdiGribIterator *me);


int cdiGribIterator_getLong(CdiGribIterator *me, const char *key, long *value);
int cdiGribIterator_getDouble(CdiGribIterator *me, const char *key, double *value);
int cdiGribIterator_getLength(CdiGribIterator *me, const char *key, size_t *value);
int cdiGribIterator_getString(CdiGribIterator *me, const char *key, char *value, size_t *length);
int cdiGribIterator_getSize(CdiGribIterator *me, const char *key, size_t *value);
int cdiGribIterator_getLongArray(CdiGribIterator *me, const char *key, long *value, size_t *array_size);
int cdiGribIterator_getDoubleArray(CdiGribIterator *me, const char *key, double *value, size_t *array_size);


int cdiGribIterator_inqEdition(CdiGribIterator *me);
long cdiGribIterator_inqLongValue(CdiGribIterator *me, const char *key);
long cdiGribIterator_inqLongDefaultValue(CdiGribIterator *me, const char *key, long defaultValue);
double cdiGribIterator_inqDoubleValue(CdiGribIterator *me, const char *key);
double cdiGribIterator_inqDoubleDefaultValue(CdiGribIterator *me, const char *key, double defaultValue);
char *cdiGribIterator_inqStringValue(CdiGribIterator *me, const char *key);




int vlistCreate(void);


void vlistDestroy(int vlistID);


int vlistDuplicate(int vlistID);


void vlistCopy(int vlistID2, int vlistID1);


void vlistCopyFlag(int vlistID2, int vlistID1);

void vlistClearFlag(int vlistID);


void vlistCat(int vlistID2, int vlistID1);


void vlistMerge(int vlistID2, int vlistID1);

void vlistPrint(int vlistID);


int vlistNumber(int vlistID);


int vlistNvars(int vlistID);


int vlistNgrids(int vlistID);


int vlistNzaxis(int vlistID);


int vlistNsubtypes(int vlistID);

void vlistDefNtsteps(int vlistID, int nts);
int vlistNtsteps(int vlistID);
int vlistGridsizeMax(int vlistID);
int vlistGrid(int vlistID, int index);
int vlistGridIndex(int vlistID, int gridID);
void vlistChangeGridIndex(int vlistID, int index, int gridID);
void vlistChangeGrid(int vlistID, int gridID1, int gridID2);
int vlistZaxis(int vlistID, int index);
int vlistZaxisIndex(int vlistID, int zaxisID);
void vlistChangeZaxisIndex(int vlistID, int index, int zaxisID);
void vlistChangeZaxis(int vlistID, int zaxisID1, int zaxisID2);
int vlistNrecs(int vlistID);
int vlistSubtype(int vlistID, int index);
int vlistSubtypeIndex(int vlistID, int subtypeID);


void vlistDefTaxis(int vlistID, int taxisID);


int vlistInqTaxis(int vlistID);

void vlistDefTable(int vlistID, int tableID);
int vlistInqTable(int vlistID);
void vlistDefInstitut(int vlistID, int instID);
int vlistInqInstitut(int vlistID);
void vlistDefModel(int vlistID, int modelID);
int vlistInqModel(int vlistID);





int vlistDefVarTiles(int vlistID, int gridID, int zaxisID, int tsteptype, int tilesetID);


int vlistDefVar(int vlistID, int gridID, int zaxisID, int tsteptype);

void vlistChangeVarGrid(int vlistID, int varID, int gridID);
void vlistChangeVarZaxis(int vlistID, int varID, int zaxisID);

void vlistInqVar(int vlistID, int varID, int *gridID, int *zaxisID, int *tsteptype);
int vlistInqVarGrid(int vlistID, int varID);
int vlistInqVarZaxis(int vlistID, int varID);


int vlistInqVarID(int vlistID, int code);

void vlistDefVarTsteptype(int vlistID, int varID, int tsteptype);
int vlistInqVarTsteptype(int vlistID, int varID);

void vlistDefVarCompType(int vlistID, int varID, int comptype);
int vlistInqVarCompType(int vlistID, int varID);
void vlistDefVarCompLevel(int vlistID, int varID, int complevel);
int vlistInqVarCompLevel(int vlistID, int varID);


void vlistDefVarParam(int vlistID, int varID, int param);


int vlistInqVarParam(int vlistID, int varID);


void vlistDefVarCode(int vlistID, int varID, int code);


int vlistInqVarCode(int vlistID, int varID);


void vlistDefVarDatatype(int vlistID, int varID, int datatype);


int vlistInqVarDatatype(int vlistID, int varID);

void vlistDefVarChunkType(int vlistID, int varID, int chunktype);
int vlistInqVarChunkType(int vlistID, int varID);

void vlistDefVarXYZ(int vlistID, int varID, int xyz);
int vlistInqVarXYZ(int vlistID, int varID);

int vlistInqVarNumber(int vlistID, int varID);

void vlistDefVarInstitut(int vlistID, int varID, int instID);
int vlistInqVarInstitut(int vlistID, int varID);
void vlistDefVarModel(int vlistID, int varID, int modelID);
int vlistInqVarModel(int vlistID, int varID);
void vlistDefVarTable(int vlistID, int varID, int tableID);
int vlistInqVarTable(int vlistID, int varID);


void vlistDefVarName(int vlistID, int varID, const char *name);


void vlistInqVarName(int vlistID, int varID, char *name);


char *vlistCopyVarName(int vlistId, int varId);


void vlistDefVarStdname(int vlistID, int varID, const char *stdname);


void vlistInqVarStdname(int vlistID, int varID, char *stdname);


void vlistDefVarLongname(int vlistID, int varID, const char *longname);


void vlistInqVarLongname(int vlistID, int varID, char *longname);


void vlistDefVarUnits(int vlistID, int varID, const char *units);


void vlistInqVarUnits(int vlistID, int varID, char *units);


void vlistDefVarMissval(int vlistID, int varID, double missval);


double vlistInqVarMissval(int vlistID, int varID);


void vlistDefVarExtra(int vlistID, int varID, const char *extra);


void vlistInqVarExtra(int vlistID, int varID, char *extra);

void vlistDefVarScalefactor(int vlistID, int varID, double scalefactor);
double vlistInqVarScalefactor(int vlistID, int varID);
void vlistDefVarAddoffset(int vlistID, int varID, double addoffset);
double vlistInqVarAddoffset(int vlistID, int varID);

void vlistDefVarTimave(int vlistID, int varID, int timave);
int vlistInqVarTimave(int vlistID, int varID);
void vlistDefVarTimaccu(int vlistID, int varID, int timaccu);
int vlistInqVarTimaccu(int vlistID, int varID);

void vlistDefVarTypeOfGeneratingProcess(int vlistID, int varID, int typeOfGeneratingProcess);
int vlistInqVarTypeOfGeneratingProcess(int vlistID, int varID);

void vlistDefVarProductDefinitionTemplate(int vlistID, int varID, int productDefinitionTemplate);
int vlistInqVarProductDefinitionTemplate(int vlistID, int varID);

int vlistInqVarSize(int vlistID, int varID);

void vlistDefIndex(int vlistID, int varID, int levID, int index);
int vlistInqIndex(int vlistID, int varID, int levID);
void vlistDefFlag(int vlistID, int varID, int levID, int flag);
int vlistInqFlag(int vlistID, int varID, int levID);
int vlistFindVar(int vlistID, int fvarID);
int vlistFindLevel(int vlistID, int fvarID, int flevelID);
int vlistMergedVar(int vlistID, int varID);
int vlistMergedLevel(int vlistID, int varID, int levelID);


void vlistDefVarEnsemble(int vlistID, int varID, int ensID, int ensCount, int forecast_type);
int vlistInqVarEnsemble(int vlistID, int varID, int *ensID, int *ensCount, int *forecast_type);


void cdiClearAdditionalKeys(void);

void cdiDefAdditionalKey(const char *string);


void vlistDefVarIntKey(int vlistID, int varID, const char *name, int value);

void vlistDefVarDblKey(int vlistID, int varID, const char *name, double value);


int vlistHasVarKey(int vlistID, int varID, const char *name);

double vlistInqVarDblKey(int vlistID, int varID, const char *name);

int vlistInqVarIntKey(int vlistID, int varID, const char *name);





int vlistInqNatts(int vlistID, int varID, int *nattsp);

int vlistInqAtt(int vlistID, int varID, int attrnum, char *name, int *typep, int *lenp);
int vlistDelAtt(int vlistID, int varID, const char *name);


int vlistDefAttInt(int vlistID, int varID, const char *name, int type, int len, const int ip[]);

int vlistDefAttFlt(int vlistID, int varID, const char *name, int type, int len, const double dp[]);

int vlistDefAttTxt(int vlistID, int varID, const char *name, int len, const char *tp_cbuf);


int vlistInqAttInt(int vlistID, int varID, const char *name, int mlen, int ip[]);

int vlistInqAttFlt(int vlistID, int varID, const char *name, int mlen, double dp[]);

int vlistInqAttTxt(int vlistID, int varID, const char *name, int mlen, char *tp_cbuf);




void gridName(int gridtype, char *gridname);
const char *gridNamePtr(int gridtype);

void gridCompress(int gridID);

void gridDefMaskGME(int gridID, const int mask[]);
int gridInqMaskGME(int gridID, int mask[]);

void gridDefMask(int gridID, const int mask[]);
int gridInqMask(int gridID, int mask[]);

void gridPrint(int gridID, int index, int opt);


int gridCreate(int gridtype, int size);


void gridDestroy(int gridID);


int gridDuplicate(int gridID);


int gridInqType(int gridID);


int gridInqSize(int gridID);


void gridDefXsize(int gridID, int xsize);


int gridInqXsize(int gridID);


void gridDefYsize(int gridID, int ysize);


int gridInqYsize(int gridID);


void gridDefNP(int gridID, int np);


int gridInqNP(int gridID);


void gridDefXvals(int gridID, const double xvals[]);


int gridInqXvals(int gridID, double xvals[]);


void gridDefYvals(int gridID, const double yvals[]);


int gridInqYvals(int gridID, double yvals[]);


void gridDefXname(int gridID, const char *xname);


void gridInqXname(int gridID, char *xname);


void gridDefXlongname(int gridID, const char *xlongname);


void gridInqXlongname(int gridID, char *xlongname);


void gridDefXunits(int gridID, const char *xunits);


void gridInqXunits(int gridID, char *xunits);


void gridDefYname(int gridID, const char *yname);


void gridInqYname(int gridID, char *yname);


void gridDefYlongname(int gridID, const char *ylongname);


void gridInqYlongname(int gridID, char *ylongname);


void gridDefYunits(int gridID, const char *yunits);


void gridInqYunits(int gridID, char *yunits);


void gridInqXstdname(int gridID, char *xstdname);


void gridInqYstdname(int gridID, char *ystdname);


void gridDefPrec(int gridID, int prec);


int gridInqPrec(int gridID);


double gridInqXval(int gridID, int index);


double gridInqYval(int gridID, int index);

double gridInqXinc(int gridID);
double gridInqYinc(int gridID);

int gridIsCircular(int gridID);
int gridIsRotated(int gridID);
void gridDefXpole(int gridID, double xpole);
double gridInqXpole(int gridID);
void gridDefYpole(int gridID, double ypole);
double gridInqYpole(int gridID);
void gridDefAngle(int gridID, double angle);
double gridInqAngle(int gridID);
int gridInqTrunc(int gridID);
void gridDefTrunc(int gridID, int trunc);

void gridDefGMEnd(int gridID, int nd);
int gridInqGMEnd(int gridID);
void gridDefGMEni(int gridID, int ni);
int gridInqGMEni(int gridID);
void gridDefGMEni2(int gridID, int ni2);
int gridInqGMEni2(int gridID);
void gridDefGMEni3(int gridID, int ni3);
int gridInqGMEni3(int gridID);




void gridDefNumber(int gridID, int number);


int gridInqNumber(int gridID);


void gridDefPosition(int gridID, int position);


int gridInqPosition(int gridID);


void gridDefReference(int gridID, const char *reference);


int gridInqReference(int gridID, char *reference);


#ifdef __cplusplus
void gridDefUUID(int gridID, const unsigned char *uuid);
#else
void gridDefUUID(int gridID, const unsigned char uuid[CDI_UUID_SIZE]);
#endif


#ifdef __cplusplus
void gridInqUUID(int gridID, unsigned char *uuid);
#else
void gridInqUUID(int gridID, unsigned char uuid[CDI_UUID_SIZE]);
#endif


void gridDefLCC(int gridID, double originLon, double originLat, double lonParY, double lat1, double lat2, double xinc, double yinc, int projflag, int scanflag);
void gridInqLCC(int gridID, double *originLon, double *originLat, double *lonParY, double *lat1, double *lat2, double *xinc, double *yinc, int *projflag, int *scanflag);


void gridDefLcc2(int gridID, double earth_radius, double lon_0, double lat_0, double lat_1, double lat_2);
void gridInqLcc2(int gridID, double *earth_radius, double *lon_0, double *lat_0, double *lat_1, double *lat_2);


void gridDefLaea(int gridID, double earth_radius, double lon_0, double lat_0);
void gridInqLaea(int gridID, double *earth_radius, double *lon_0, double *lat_0);


void gridDefArea(int gridID, const double area[]);
void gridInqArea(int gridID, double area[]);
int gridHasArea(int gridID);


void gridDefNvertex(int gridID, int nvertex);


int gridInqNvertex(int gridID);


void gridDefXbounds(int gridID, const double xbounds[]);


int gridInqXbounds(int gridID, double xbounds[]);


void gridDefYbounds(int gridID, const double ybounds[]);


int gridInqYbounds(int gridID, double ybounds[]);

void gridDefRowlon(int gridID, int nrowlon, const int rowlon[]);
void gridInqRowlon(int gridID, int rowlon[]);
void gridChangeType(int gridID, int gridtype);

void gridDefComplexPacking(int gridID, int lpack);
int gridInqComplexPacking(int gridID);



void zaxisName(int zaxistype, char *zaxisname);


int zaxisCreate(int zaxistype, int size);


void zaxisDestroy(int zaxisID);


int zaxisInqType(int zaxisID);


int zaxisInqSize(int zaxisID);


int zaxisDuplicate(int zaxisID);

void zaxisResize(int zaxisID, int size);

void zaxisPrint(int zaxisID, int index);


void zaxisDefLevels(int zaxisID, const double levels[]);


void zaxisInqLevels(int zaxisID, double levels[]);


void zaxisDefLevel(int zaxisID, int levelID, double levels);


double zaxisInqLevel(int zaxisID, int levelID);


void zaxisDefNlevRef(int gridID, int nhlev);


int zaxisInqNlevRef(int gridID);


void zaxisDefNumber(int gridID, int number);


int zaxisInqNumber(int gridID);


void zaxisDefUUID(int zaxisID, const unsigned char uuid[CDI_UUID_SIZE]);


void zaxisInqUUID(int zaxisID, unsigned char uuid[CDI_UUID_SIZE]);


void zaxisDefName(int zaxisID, const char *name_optional);


void zaxisInqName(int zaxisID, char *name);


void zaxisDefLongname(int zaxisID, const char *longname_optional);


void zaxisInqLongname(int zaxisID, char *longname);


void zaxisDefUnits(int zaxisID, const char *units_optional);


void zaxisInqUnits(int zaxisID, char *units);


void zaxisInqStdname(int zaxisID, char *stdname);


void zaxisDefPsName(int zaxisID, const char *psname_optional);


void zaxisInqPsName(int zaxisID, char *psname);

void zaxisDefPrec(int zaxisID, int prec);
int zaxisInqPrec(int zaxisID);

void zaxisDefPositive(int zaxisID, int positive);
int zaxisInqPositive(int zaxisID);

void zaxisDefScalar(int zaxisID);
int zaxisInqScalar(int zaxisID);

void zaxisDefLtype(int zaxisID, int ltype);
int zaxisInqLtype(int zaxisID);

const double *zaxisInqLevelsPtr(int zaxisID);
void zaxisDefVct(int zaxisID, int size, const double vct[]);
void zaxisInqVct(int zaxisID, double vct[]);
int zaxisInqVctSize(int zaxisID);
const double *zaxisInqVctPtr(int zaxisID);
void zaxisDefLbounds(int zaxisID, const double lbounds[]);
int zaxisInqLbounds(int zaxisID, double lbounds_optional[]);
double zaxisInqLbound(int zaxisID, int index);
void zaxisDefUbounds(int zaxisID, const double ubounds[]);
int zaxisInqUbounds(int zaxisID, double ubounds_optional[]);
double zaxisInqUbound(int zaxisID, int index);
void zaxisDefWeights(int zaxisID, const double weights[]);
int zaxisInqWeights(int zaxisID, double weights_optional[]);
void zaxisChangeType(int zaxisID, int zaxistype);




int taxisCreate(int timetype);


void taxisDestroy(int taxisID);

int taxisDuplicate(int taxisID);

void taxisCopyTimestep(int taxisIDdes, int taxisIDsrc);

void taxisDefType(int taxisID, int type);


void taxisDefVdate(int taxisID, int date);


void taxisDefVtime(int taxisID, int time);


int taxisInqVdate(int taxisID);


int taxisInqVtime(int taxisID);


void taxisDefRdate(int taxisID, int date);


void taxisDefRtime(int taxisID, int time);


int taxisInqRdate(int taxisID);


int taxisInqRtime(int taxisID);


void taxisDefFdate(int taxisID, int date);


void taxisDefFtime(int taxisID, int time);


int taxisInqFdate(int taxisID);


int taxisInqFtime(int taxisID);

int taxisHasBounds(int taxisID);

void taxisDeleteBounds(int taxisID);

void taxisDefVdateBounds(int taxisID, int vdate_lb, int vdate_ub);

void taxisDefVtimeBounds(int taxisID, int vtime_lb, int vtime_ub);

void taxisInqVdateBounds(int taxisID, int *vdate_lb, int *vdate_ub);

void taxisInqVtimeBounds(int taxisID, int *vtime_lb, int *vtime_ub);


void taxisDefCalendar(int taxisID, int calendar);


int taxisInqCalendar(int taxisID);

void taxisDefTunit(int taxisID, int tunit);
int taxisInqTunit(int taxisID);

void taxisDefForecastTunit(int taxisID, int tunit);
int taxisInqForecastTunit(int taxisID);

void taxisDefForecastPeriod(int taxisID, double fc_period);
double taxisInqForecastPeriod(int taxisID);

void taxisDefNumavg(int taxisID, int numavg);

int taxisInqType(int taxisID);

int taxisInqNumavg(int taxisID);

const char *tunitNamePtr(int tunitID);




int institutDef(int center, int subcenter, const char *name, const char *longname);
int institutInq(int center, int subcenter, const char *name, const char *longname);
int institutInqNumber(void);
int institutInqCenter(int instID);
int institutInqSubcenter(int instID);
const char *institutInqNamePtr(int instID);
const char *institutInqLongnamePtr(int instID);



int modelDef(int instID, int modelgribID, const char *name);
int modelInq(int instID, int modelgribID, const char *name);
int modelInqInstitut(int modelID) ;
int modelInqGribID(int modelID);
const char *modelInqNamePtr(int modelID);




void tableWriteC(const char *filename, int tableID);

void tableFWriteC(FILE *ptfp, int tableID);

void tableWrite(const char *filename, int tableID);

int tableRead(const char *tablefile);
int tableDef(int modelID, int tablenum, const char *tablename);

const char *tableInqNamePtr(int tableID);
void tableDefEntry(int tableID, int code, const char *name, const char *longname, const char *units);

int tableInq(int modelID, int tablenum, const char *tablename);
int tableInqNumber(void);

int tableInqNum(int tableID);
int tableInqModel(int tableID);

void tableInqPar(int tableID, int code, char *name, char *longname, char *units);

int tableInqParCode(int tableID, char *name, int *code);
int tableInqParName(int tableID, int code, char *name);
int tableInqParLongname(int tableID, int code, char *longname);
int tableInqParUnits(int tableID, int code, char *units);

const char *tableInqParNamePtr(int tableID, int parID);
const char *tableInqParLongnamePtr(int tableID, int parID);
const char *tableInqParUnitsPtr(int tableID, int parID);



void streamDefHistory(int streamID, int size, const char *history);
int streamInqHistorySize(int streamID);
void streamInqHistoryString(int streamID, char *history);




int subtypeCreate(int subtype);


void subtypePrint(int subtypeID);


int subtypeCompare(int subtypeID1, int subtypeID2);


int subtypeInqSize(int subtypeID);


int subtypeInqActiveIndex(int subtypeID);


void subtypeDefActiveIndex(int subtypeID, int index);


subtype_query_t keyValuePair(const char *key, int value);



subtype_query_t matchAND(subtype_query_t q1, subtype_query_t q2);


int subtypeInqSubEntry(int subtypeID, subtype_query_t criterion);


int subtypeInqTile(int subtypeID, int tileindex, int attribute);


int vlistInqVarSubtype(int vlistID, int varID);


void gribapiLibraryVersion(int *major_version, int *minor_version, int *revision_version);







#endif
#ifndef _BASETIME_H
#define _BASETIME_H


#define MAX_TIMECACHE_SIZE 1024

typedef struct {
  int size;
  int startid;
  int maxvals;
  double cache[MAX_TIMECACHE_SIZE];
}
timecache_t;

typedef struct {
  int ncvarid;
  int ncdimid;
  int ncvarboundsid;
  int leadtimeid;
  int lwrf;
  timecache_t *timevar_cache;
}
basetime_t;

void basetimeInit(basetime_t *basetime);

#endif
#if defined (HAVE_CONFIG_H)
#endif

#include <stdio.h>


#undef UNDEFID
#define UNDEFID CDI_UNDEFID

void basetimeInit(basetime_t *basetime)
{
  if ( basetime == NULL )
    Error("Internal problem! Basetime not allocated.");

  basetime->ncvarid = UNDEFID;
  basetime->ncdimid = UNDEFID;
  basetime->ncvarboundsid = UNDEFID;
  basetime->leadtimeid = UNDEFID;
  basetime->lwrf = 0;
  basetime->timevar_cache = NULL;
}
#ifndef _CALENDAR_H
#define _CALENDAR_H

void encode_caldaysec(int calendar, int year, int month, int day, int hour, int minute, int second,
                      int *julday, int *secofday);
void decode_caldaysec(int calendar, int julday, int secofday,
                      int *year, int *month, int *day, int *hour, int *minute, int *second);

int calendar_dpy(int calendar);
int days_per_year(int calendar, int year);
int days_per_month(int calendar, int year, int month);


#endif
#ifndef _TIMEBASE_H
#define _TIMEBASE_H

#include <inttypes.h>




void decode_julday(int calendar, int julday, int *year, int *mon, int *day);
int encode_julday(int calendar, int year, int month, int day);

int date_to_julday(int calendar, int date);
int julday_to_date(int calendar, int julday);

int time_to_sec(int time);
int sec_to_time(int secofday);

void julday_add_seconds(int64_t seconds, int *julday, int *secofday);
void julday_add(int days, int secs, int *julday, int *secofday);
double julday_sub(int julday1, int secofday1, int julday2, int secofday2, int *days, int *secs);

void encode_juldaysec(int calendar, int year, int month, int day, int hour, int minute, int second, int *julday, int *secofday);
void decode_juldaysec(int calendar, int julday, int secofday, int *year, int *month, int *day, int *hour, int *minute, int *second);

#endif
#include <limits.h>
#include <stdio.h>



static int month_360[12] = {30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30};
static int month_365[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static int month_366[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


int calendar_dpy(int calendar)
{
  int dpy = 0;

  if ( calendar == CALENDAR_360DAYS ) dpy = 360;
  else if ( calendar == CALENDAR_365DAYS ) dpy = 365;
  else if ( calendar == CALENDAR_366DAYS ) dpy = 366;

  return (dpy);
}


int days_per_month(int calendar, int year, int month)
{
  int dayspermonth = 0;
  int *dpm = NULL;
  int dpy;

  dpy = calendar_dpy(calendar);

  if ( dpy == 360 ) dpm = month_360;
  else if ( dpy == 365 ) dpm = month_365;
  else dpm = month_366;

  if ( month >= 1 && month <= 12 )
    dayspermonth = dpm[month-1];




  if ( dpy == 0 && month == 2 )
    {
      if ( (year%4 == 0 && year%100 != 0) || year%400 == 0 )
        dayspermonth = 29;
      else
        dayspermonth = 28;
    }

  return (dayspermonth);
}


int days_per_year(int calendar, int year)
{
  int daysperyear;
  int dpy;

  dpy = calendar_dpy(calendar);

  if ( dpy == 0 )
    {
      if ( calendar == CALENDAR_STANDARD )
        {
          if ( year == 1582 )
            dpy = 355;
          else if ( (year%4 == 0 && year%100 != 0) || year%400 == 0 )
            dpy = 366;
          else
            dpy = 365;
        }
      else
        {
          if ( (year%4 == 0 && year%100 != 0) || year%400 == 0 )
            dpy = 366;
          else
            dpy = 365;
        }
    }

  daysperyear = dpy;

  return (daysperyear);
}


static void decode_day(int dpy, int days, int *year, int *month, int *day)
{
  int i = 0;
  int *dpm = NULL;

  *year = (days-1) / dpy;
  days -= (*year*dpy);

  if ( dpy == 360 ) dpm = month_360;
  else if ( dpy == 365 ) dpm = month_365;
  else if ( dpy == 366 ) dpm = month_366;

  if ( dpm )
    for ( i = 0; i < 12; i++ )
      {
        if ( days > dpm[i] ) days -= dpm[i];
        else break;
      }

  *month = i + 1;
  *day = days;
}


static int encode_day(int dpy, int year, int month, int day)
{
  int i;
  int *dpm = NULL;
  long rval = (long)dpy * year + day;

  if ( dpy == 360 ) dpm = month_360;
  else if ( dpy == 365 ) dpm = month_365;
  else if ( dpy == 366 ) dpm = month_366;

  if ( dpm ) for ( i = 0; i < month-1; i++ ) rval += dpm[i];
  if (rval > INT_MAX || rval < INT_MIN)
    Error("Unhandled date: %ld", rval);

  return (int)rval;
}


void encode_caldaysec(int calendar, int year, int month, int day, int hour, int minute, int second,
                      int *julday, int *secofday)
{
  int dpy;

  dpy = calendar_dpy(calendar);

  if ( dpy == 360 || dpy == 365 || dpy == 366 )
    *julday = encode_day(dpy, year, month, day);
  else
    *julday = encode_julday(calendar, year, month, day);

  *secofday = hour*3600 + minute*60 + second;
}


void decode_caldaysec(int calendar, int julday, int secofday,
                      int *year, int *month, int *day, int *hour, int *minute, int *second)
{
  int dpy;

  dpy = calendar_dpy(calendar);

  if ( dpy == 360 || dpy == 365 || dpy == 366 )
    decode_day(dpy, julday, year, month, day);
  else
    decode_julday(calendar, julday, year, month, day);

  *hour = secofday/3600;
  *minute = secofday/60 - *hour*60;
  *second = secofday - *hour*3600 - *minute*60;
}

#ifndef _CDF_H
#define _CDF_H

void cdfDebug(int debug);

extern int CDF_Debug;

const char *cdfLibraryVersion(void);
const char *hdfLibraryVersion(void);

int cdfOpen(const char *filename, const char *mode);
int cdfOpen64(const char *filename, const char *mode);
int cdf4Open(const char *filename, const char *mode, int *filetype);
void cdfClose(int fileID);

#endif
#ifndef RESOURCE_HANDLE_H
#define RESOURCE_HANDLE_H

#ifdef HAVE_CONFIG_H
#endif

#include <stdio.h>
typedef int cdiResH;


typedef int ( * valCompareFunc )( void *, void * );
typedef void ( * valDestroyFunc )( void * );
typedef void ( * valPrintFunc )( void *, FILE * );
typedef int ( * valGetPackSizeFunc )( void *, void *context );
typedef void ( * valPackFunc )( void *, void *buf, int size, int *pos, void *context );
typedef int ( * valTxCodeFunc )( void );

typedef struct {
  valCompareFunc valCompare;
  valDestroyFunc valDestroy;
  valPrintFunc valPrint;
  valGetPackSizeFunc valGetPackSize;
  valPackFunc valPack;
  valTxCodeFunc valTxCode;
}resOps;

enum {
  RESH_IN_USE_BIT = 1 << 0,
  RESH_SYNC_BIT = 1 << 1,

  RESH_UNUSED = 0,

  RESH_DESYNC_DELETED
    = RESH_SYNC_BIT,

  RESH_IN_USE
    = RESH_IN_USE_BIT,

  RESH_DESYNC_IN_USE
    = RESH_IN_USE_BIT | RESH_SYNC_BIT,
};

void reshListCreate(int namespaceID);
void reshListDestruct(int namespaceID);
int reshPut ( void *, const resOps * );
void reshReplace(cdiResH resH, void *p, const resOps *ops);
void reshRemove ( cdiResH, const resOps * );

void reshDestroy(cdiResH);

unsigned reshCountType(const resOps *resTypeOps);

void * reshGetValue(const char* caller, const char* expressionString, cdiResH id, const resOps* ops);
#define reshGetVal(resH,ops) reshGetValue(__func__, #resH, resH, ops)

void reshGetResHListOfType(unsigned numIDs, int IDs[], const resOps *ops);

enum cdiApplyRet {
  CDI_APPLY_ERROR = -1,
  CDI_APPLY_STOP,
  CDI_APPLY_GO_ON,
};
enum cdiApplyRet
cdiResHApply(enum cdiApplyRet (*func)(int id, void *res, const resOps *p,
                                      void *data), void *data);
enum cdiApplyRet
cdiResHFilterApply(const resOps *p,
                   enum cdiApplyRet (*func)(int id, void *res,
                                            void *data),
                   void *data);

int reshPackBufferCreate(char **packBuf, int *packBufSize, void *context);
void reshPackBufferDestroy ( char ** );
int reshResourceGetPackSize_intern(int resh, const resOps *ops, void *context, const char* caller, const char* expressionString);
#define reshResourceGetPackSize(resh,ops,context) reshResourceGetPackSize_intern(resh, ops, context, __func__, #resh)
void reshPackResource_intern(int resh, const resOps *ops, void *buf, int buf_size, int *position, void *context, const char* caller, const char* expressionString);
#define reshPackResource(resh,ops,buf,buf_size,position,context) reshPackResource_intern(resh, ops, buf, buf_size, position, context, __func__, #resh)

void reshSetStatus ( cdiResH, const resOps *, int );
int reshGetStatus ( cdiResH, const resOps * );

void reshLock ( void );
void reshUnlock ( void );

enum reshListMismatch {
  cdiResHListOccupationMismatch,
  cdiResHListResourceTypeMismatch,
  cdiResHListResourceContentMismatch,
};

int reshListCompare(int nsp0, int nsp1);
void reshListPrint(FILE *fp);

#endif
#ifndef _TAXIS_H
#define _TAXIS_H

#ifndef RESOURCE_HANDLE_H
#endif

typedef struct {


  int self;
  short used;
  short has_bounds;
  int type;
  int vdate;
  int vtime;
  int rdate;
  int rtime;
  int fdate;
  int ftime;
  int calendar;
  int unit;
  int numavg;
  int climatology;
  int vdate_lb;
  int vtime_lb;
  int vdate_ub;
  int vtime_ub;
  int fc_unit;
  double fc_period;
  char* name;
  char* longname;
}
taxis_t;

void ptaxisInit(taxis_t* taxis);
void ptaxisCopy(taxis_t* dest, taxis_t* source);
taxis_t* taxisPtr(int taxisID);
void cdiSetForecastPeriod(double timevalue, taxis_t *taxis);
void cdiDecodeTimeval(double timevalue, taxis_t* taxis, int* date, int* time);
double cdiEncodeTimeval(int date, int time, taxis_t* taxis);
void timeval2vtime(double timevalue, taxis_t* taxis, int* vdate, int* vtime);
double vtime2timeval(int vdate, int vtime, taxis_t *taxis);

void ptaxisDefName(taxis_t *taxisptr, const char *name);
void ptaxisDefLongname(taxis_t *taxisptr, const char *name);
void taxisDestroyKernel(taxis_t *taxisptr);
#if !defined (SX)
extern const resOps taxisOps;
#endif

int
taxisUnpack(char *unpackBuffer, int unpackBufferSize, int *unpackBufferPos,
            int originNamespace, void *context, int checkForSameID);

#endif
#ifndef _CDI_LIMITS_H
#define _CDI_LIMITS_H

#define MAX_GRIDS_PS 128
#define MAX_ZAXES_PS 128
#define MAX_ATTRIBUTES 256
#define MAX_SUBTYPES_PS 128

#endif
#ifndef _SERVICE_H
#define _SERVICE_H


typedef struct {
  int checked;
  int byteswap;
  int header[8];
  int hprec;
  int dprec;
  size_t datasize;
  size_t buffersize;
  void *buffer;
}
srvrec_t;


const char *srvLibraryVersion(void);

void srvDebug(int debug);

int srvCheckFiletype(int fileID, int *swap);

void *srvNew(void);
void srvDelete(void *srv);

int srvRead(int fileID, void *srv);
void srvWrite(int fileID, void *srv);

int srvInqHeader(void *srv, int *header);
int srvInqDataSP(void *srv, float *data);
int srvInqDataDP(void *srv, double *data);

int srvDefHeader(void *srv, const int *header);
int srvDefDataSP(void *srv, const float *data);
int srvDefDataDP(void *srv, const double *data);


#endif

#ifndef _EXTRA_H
#define _EXTRA_H

#define EXT_REAL 1
#define EXT_COMP 2


typedef struct {
  int checked;
  int byteswap;
  int header[4];
  int prec;
  int number;
  size_t datasize;
  size_t buffersize;
  void *buffer;
}
extrec_t;


const char *extLibraryVersion(void);

void extDebug(int debug);

int extCheckFiletype(int fileID, int *swap);

void *extNew(void);
void extDelete(void *ext);

int extRead(int fileID, void *ext);
int extWrite(int fileID, void *ext);

int extInqHeader(void *ext, int *header);
int extInqDataSP(void *ext, float *data);
int extInqDataDP(void *ext, double *data);

int extDefHeader(void *ext, const int *header);
int extDefDataSP(void *ext, const float *data);
int extDefDataDP(void *ext, const double *data);

#endif

#ifndef _IEG_H
#define _IEG_H


#define IEG_LTYPE_SURFACE 1
#define IEG_LTYPE_99 99
#define IEG_LTYPE_ISOBARIC 100
#define IEG_LTYPE_MEANSEA 102
#define IEG_LTYPE_ALTITUDE 103
#define IEG_LTYPE_HEIGHT 105
#define IEG_LTYPE_SIGMA 107
#define IEG_LTYPE_HYBRID 109
#define IEG_LTYPE_HYBRID_LAYER 110
#define IEG_LTYPE_LANDDEPTH 111
#define IEG_LTYPE_LANDDEPTH_LAYER 112
#define IEG_LTYPE_SEADEPTH 160
#define IEG_LTYPE_99_MARGIN 1000




#define IEG_GTYPE_LATLON 0
#define IEG_GTYPE_LATLON_ROT 10

#define IEG_P_CodeTable(x) (x[ 5])
#define IEG_P_Parameter(x) (x[ 6])
#define IEG_P_LevelType(x) (x[ 7])
#define IEG_P_Level1(x) (x[ 8])
#define IEG_P_Level2(x) (x[ 9])
#define IEG_P_Year(x) (x[10])
#define IEG_P_Month(x) (x[11])
#define IEG_P_Day(x) (x[12])
#define IEG_P_Hour(x) (x[13])
#define IEG_P_Minute(x) (x[14])




#define IEG_G_Size(x) (x[ 0])
#define IEG_G_NumVCP(x) (x[3] == 10 ? (x[0]-42)/4 : (x[0]-32)/4)
#define IEG_G_GridType(x) (x[ 3])
#define IEG_G_NumLon(x) (x[ 4])
#define IEG_G_NumLat(x) (x[ 5])
#define IEG_G_FirstLat(x) (x[ 6])
#define IEG_G_FirstLon(x) (x[ 7])
#define IEG_G_ResFlag(x) (x[ 8])
#define IEG_G_LastLat(x) (x[ 9])
#define IEG_G_LastLon(x) (x[10])
#define IEG_G_LonIncr(x) (x[11])
#define IEG_G_LatIncr(x) (x[12])
#define IEG_G_ScanFlag(x) (x[13])
#define IEG_G_LatSP(x) (x[16])
#define IEG_G_LonSP(x) (x[17])
#define IEG_G_ResFac(x) (x[18])


typedef struct {
  int checked;
  int byteswap;
  int dprec;
  int ipdb[37];
  double refval;
  int igdb[22];
  double vct[100];
  size_t datasize;
  size_t buffersize;
  void *buffer;
}
iegrec_t;


const char *iegLibraryVersion(void);

void iegDebug(int debug);
int iegCheckFiletype(int fileID, int *swap);

void *iegNew(void);
void iegDelete(void *ieg);
void iegInitMem(void *ieg);

int iegRead(int fileID, void *ieg);
int iegWrite(int fileID, void *ieg);

void iegCopyMeta(void *dieg, void *sieg);
int iegInqHeader(void *ieg, int *header);
int iegInqDataSP(void *ieg, float *data);
int iegInqDataDP(void *ieg, double *data);

int iegDefHeader(void *ieg, const int *header);
int iegDefDataSP(void *ieg, const float *data);
int iegDefDataDP(void *ieg, const double *data);

#endif
#ifndef _CDI_INT_H
#define _CDI_INT_H

#if defined (HAVE_CONFIG_H)
#endif

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>



#ifndef UNUSED
#define UNUSED(x) (void)x
#endif

#ifndef strdupx
#ifndef strdup
char *strdup(const char *s);
#endif
#define strdupx strdup
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


#ifndef _ERROR_H
#endif
#ifndef _BASETIME_H
#endif
#ifndef _TIMEBASE_H
#endif
#ifndef _TAXIS_H
#endif
#ifndef _CDI_LIMITS_H
#endif
#ifndef _SERVICE_H
#endif
#ifndef _EXTRA_H
#endif
#ifndef _IEG_H
#endif
#ifndef RESOURCE_HANDLE_H
#endif


#define check_parg(arg) if ( arg == 0 ) Warning("Argument '" #arg "' not allocated!")

#if defined (__xlC__)
#ifndef DBL_IS_NAN
#define DBL_IS_NAN(x) ((x) != (x))
#endif
#else
#ifndef DBL_IS_NAN
#if defined (HAVE_DECL_ISNAN)
#define DBL_IS_NAN(x) (isnan(x))
#elif defined (FP_NAN)
#define DBL_IS_NAN(x) (fpclassify(x) == FP_NAN)
#else
#define DBL_IS_NAN(x) ((x) != (x))
#endif
#endif
#endif

#ifndef DBL_IS_EQUAL

#define DBL_IS_EQUAL(x,y) (DBL_IS_NAN(x)||DBL_IS_NAN(y)?(DBL_IS_NAN(x)&&DBL_IS_NAN(y)):!(x < y || y < x))
#endif

#ifndef IS_EQUAL
#define IS_NOT_EQUAL(x,y) (x < y || y < x)
#define IS_EQUAL(x,y) (!IS_NOT_EQUAL(x,y))
#endif

#define FALSE 0
#define TRUE 1

#define TYPE_REC 0
#define TYPE_VAR 1

#define MEMTYPE_DOUBLE 1
#define MEMTYPE_FLOAT 2

typedef struct
{
  void *buffer;
  size_t buffersize;
  off_t position;
  int param;
  int level;
  int date;
  int time;
  int gridID;
  int varID;
  int levelID;
  int prec;
  int sec0[2];
  int sec1[1024];
  int sec2[4096];
  int sec3[2];
  int sec4[512];
  void *exsep;
}
Record;



typedef struct {
  int
    tileindex,
    totalno_of_tileattr_pairs,
    tileClassification,
    numberOfTiles,
    numberOfAttributes,
    attribute;
} var_tile_t;


typedef struct
{
  off_t position;
  size_t size;
  int zip;
  int param;
  int ilevel;
  int ilevel2;
  int ltype;
  int tsteptype;
  short used;
  short varID;
  short levelID;
  char varname[32];
  var_tile_t tiles;
}
record_t;


typedef struct {
  record_t *records;
  int *recIDs;
  int recordSize;
  int nrecs;


  int nallrecs;
  int curRecID;
  long next;
  off_t position;
  taxis_t taxis;
}
tsteps_t;


typedef struct {
  int nlevs;
  int subtypeIndex;
  int *recordID;
  int *lindex;
} sleveltable_t;


typedef struct {
  int ncvarid;
  int subtypeSize;
  sleveltable_t *recordTable;
  int defmiss;

  int isUsed;
  int gridID;
  int zaxisID;
  int tsteptype;
  int subtypeID;
}
svarinfo_t;


typedef struct {
  int ilev;
  int mlev;
  int ilevID;
  int mlevID;
}
VCT;


typedef struct {
  int self;
  int accesstype;
  int accessmode;
  int filetype;
  int byteorder;
  int fileID;
  int filemode;
  int nrecs;
  off_t numvals;
  char *filename;
  Record *record;
  svarinfo_t *vars;
  int nvars;
  int varsAllocated;
  int curTsID;
  int rtsteps;
  long ntsteps;
  tsteps_t *tsteps;
  int tstepsTableSize;
  int tstepsNextID;
  basetime_t basetime;
  int ncmode;
  int vlistID;
  int xdimID[MAX_GRIDS_PS];
  int ydimID[MAX_GRIDS_PS];
  int zaxisID[MAX_ZAXES_PS];
  int nczvarID[MAX_ZAXES_PS];
  int ncxvarID[MAX_GRIDS_PS];
  int ncyvarID[MAX_GRIDS_PS];
  int ncavarID[MAX_GRIDS_PS];
  int historyID;
  int globalatts;
  int localatts;
  VCT vct;
  int unreduced;
  int sortname;
  int have_missval;
  int comptype;
  int complevel;
#if defined (GRIBCONTAINER2D)
  void **gribContainers;
#else
  void *gribContainers;
#endif

  void *gh;
}
stream_t;



#define MAX_OPT_GRIB_ENTRIES 500

enum cdi_convention {CDI_CONVENTION_ECHAM, CDI_CONVENTION_CF};


typedef enum {
  t_double = 0,
  t_int = 1
} key_val_pair_datatype;


typedef struct
{
  char* keyword;
  int update;
  key_val_pair_datatype data_type;
  double dbl_val;
  int int_val;
  int subtype_index;
} opt_key_val_pair_t;




extern int CDI_Debug;
extern int CDI_Recopt;
extern int cdiGribApiDebug;
extern double cdiDefaultMissval;
extern int cdiDefaultInstID;
extern int cdiDefaultModelID;
extern int cdiDefaultTableID;
extern int cdiDefaultLeveltype;
extern int cdiDefaultCalendar;

extern int cdiNcChunksizehint;
extern int cdiChunkType;
extern int cdiSplitLtype105;
extern int cdiDataUnreduced;
extern int cdiSortName;
extern int cdiHaveMissval;
extern int cdiIgnoreAttCoordinates;
extern int cdiIgnoreValidRange;
extern int cdiSkipRecords;
extern int cdiConvention;
extern int cdiInventoryMode;
extern int CDI_Version_Info;
extern int CDI_cmor_mode;
extern size_t CDI_netcdf_hdr_pad;
extern int STREAM_Debug;


extern char *cdiPartabPath;
extern int cdiPartabIntern;
extern const resOps streamOps;

static inline stream_t *
stream_to_pointer(int idx)
{
  return (stream_t *)reshGetVal(idx, &streamOps);
}

static inline void
stream_check_ptr(const char *caller, stream_t *streamptr)
{
  if ( streamptr == NULL )
    Errorc("stream undefined!");
}

int streamInqFileID(int streamID);

void gridDefHasDims(int gridID, int hasdims);
int gridInqHasDims(int gridID);
const char *gridNamePtr(int gridtype);
const char *zaxisNamePtr(int leveltype);
int zaxisInqLevelID(int zaxisID, double level);

void streamCheckID(const char *caller, int streamID);

void streamDefineTaxis(int streamID);

int streamsNewEntry(int filetype);
void streamsInitEntry(int streamID);
void cdiStreamSetupVlist(stream_t *streamptr, int vlistID);

void cdiStreamSetupVlist_(stream_t *streamptr, int vlistID);
int stream_new_var(stream_t *streamptr, int gridID, int zaxisID, int tilesetID);

int tstepsNewEntry(stream_t *streamptr);

const char *strfiletype(int filetype);

void cdi_generate_vars(stream_t *streamptr);

void vlist_check_contents(int vlistID);

void cdi_create_records(stream_t *streamptr, int tsID);

int recordNewEntry(stream_t *streamptr, int tsID);

void cdiCreateTimesteps(stream_t *streamptr);

void recordInitEntry(record_t *record);

void cdiCheckZaxis(int zaxisID);

void cdiPrintDatatypes(void);

void cdiDefAccesstype(int streamID, int type);
int cdiInqAccesstype(int streamID);

int getByteswap(int byteorder);

void cdiStreamGetIndexList(unsigned numIDs, int IDs[]);


void cdiInitialize(void);

void uuid2str(const unsigned char uuid[], char uuidstr[]);
int str2uuid(const char *uuidstr, unsigned char uuid[]);

static inline int cdiUUIDIsNull(const unsigned char uuid[])
{
  int isNull = 1;
  for (size_t i = 0; i < CDI_UUID_SIZE; ++i)
    isNull &= (uuid[i] == 0);
  return isNull;
}


char *cdiEscapeSpaces(const char *string);
char *cdiUnescapeSpaces(const char *string, const char **outStringEnd);

#define CDI_UNIT_PA 1
#define CDI_UNIT_HPA 2
#define CDI_UNIT_MM 3
#define CDI_UNIT_CM 4
#define CDI_UNIT_DM 5
#define CDI_UNIT_M 6

struct streamAssoc
{
  int streamID, vlistID;
};

struct streamAssoc
streamUnpack(char *unpackBuffer, int unpackBufferSize,
             int *unpackBufferPos, int originNamespace, void *context);

int
cdiStreamOpenDefaultDelegate(const char *filename, char filemode,
                             int filetype, stream_t *streamptr,
                             int recordBufIsToBeCreated);

int
streamOpenID(const char *filename, char filemode, int filetype,
             int resH);

void
cdiStreamDefVlist_(int streamID, int vlistID);
void
cdiStreamWriteVar_(int streamID, int varID, int memtype, const void *data,
                   int nmiss);
void
cdiStreamWriteVarChunk_(int streamID, int varID, int memtype,
                        const int rect[][2], const void *data, int nmiss);
void
cdiStreamCloseDefaultDelegate(stream_t *streamptr,
                              int recordBufIsToBeDeleted);

int cdiStreamDefTimestep_(stream_t *streamptr, int tsID);

void cdiStreamSync_(stream_t *streamptr);

const char *cdiUnitNamePtr(int cdi_unit);

void zaxisGetIndexList(int nzaxis, int *zaxisIndexList);

void zaxisDefLtype2(int zaxisID, int ltype2);
int zaxisInqLtype2(int zaxisID);


void cdiDefTableID(int tableID);

#endif
#ifndef _CDF_INT_H
#define _CDF_INT_H

#if defined (HAVE_LIBNETCDF)

#include <stdlib.h>
#include <netcdf.h>

void cdf_create (const char *path, int cmode, int *idp);
int cdf_open (const char *path, int omode, int *idp);
void cdf_close (int ncid);

void cdf_redef(int ncid);
void cdf_enddef(int ncid);
void cdf__enddef(const int ncid, const size_t hdr_pad);
void cdf_sync(int ncid);

void cdf_inq (int ncid, int *ndimsp, int *nvarsp, int *ngattsp, int *unlimdimidp);

void cdf_def_dim (int ncid, const char *name, size_t len, int *idp);
void cdf_inq_dimid (int ncid, const char *name, int *dimidp);
void cdf_inq_dim (int ncid, int dimid, char *name, size_t * lengthp);
void cdf_inq_dimname (int ncid, int dimid, char *name);
void cdf_inq_dimlen (int ncid, int dimid, size_t * lengthp);
void cdf_def_var (int ncid, const char *name, nc_type xtype, int ndims, const int dimids[], int *varidp);
void cdf_def_var_serial(int ncid, const char *name, nc_type xtype, int ndims, const int dimids[], int *varidp);
void cdf_inq_varid(int ncid, const char *name, int *varidp);
void cdf_inq_nvars(int ncid, int *nvarsp);
void cdf_inq_var(int ncid, int varid, char *name, nc_type *xtypep, int *ndimsp, int dimids[], int *nattsp);
void cdf_inq_varname (int ncid, int varid, char *name);
void cdf_inq_vartype (int ncid, int varid, nc_type *xtypep);
void cdf_inq_varndims (int ncid, int varid, int *ndimsp);
void cdf_inq_vardimid (int ncid, int varid, int dimids[]);
void cdf_inq_varnatts (int ncid, int varid, int *nattsp);

void cdf_copy_att (int ncid_in, int varid_in, const char *name, int ncid_out, int varid_out);
void cdf_put_var_text (int ncid, int varid, const char *tp);
void cdf_put_var_uchar (int ncid, int varid, const unsigned char *up);
void cdf_put_var_schar (int ncid, int varid, const signed char *cp);
void cdf_put_var_short (int ncid, int varid, const short *sp);
void cdf_put_var_int (int ncid, int varid, const int *ip);
void cdf_put_var_long (int ncid, int varid, const long *lp);
void cdf_put_var_float (int ncid, int varid, const float *fp);
void cdf_put_var_double (int ncid, int varid, const double *dp);

void cdf_get_var_text (int ncid, int varid, char *tp);
void cdf_get_var_uchar (int ncid, int varid, unsigned char *up);
void cdf_get_var_schar (int ncid, int varid, signed char *cp);
void cdf_get_var_short (int ncid, int varid, short *sp);
void cdf_get_var_int (int ncid, int varid, int *ip);
void cdf_get_var_long (int ncid, int varid, long *lp);
void cdf_get_var_float (int ncid, int varid, float *fp);
void cdf_get_var_double (int ncid, int varid, double *dp);

void cdf_get_var1_text(int ncid, int varid, const size_t index[], char *tp);

void cdf_get_var1_double(int ncid, int varid, const size_t index[], double *dp);
void cdf_put_var1_double(int ncid, int varid, const size_t index[], const double *dp);

void cdf_get_vara_uchar(int ncid, int varid, const size_t start[], const size_t count[], unsigned char *tp);
void cdf_get_vara_text(int ncid, int varid, const size_t start[], const size_t count[], char *tp);

void cdf_get_vara_double(int ncid, int varid, const size_t start[], const size_t count[], double *dp);
void cdf_put_vara_double(int ncid, int varid, const size_t start[], const size_t count[], const double *dp);

void cdf_get_vara_float(int ncid, int varid, const size_t start[], const size_t count[], float *fp);
void cdf_put_vara_float(int ncid, int varid, const size_t start[], const size_t count[], const float *fp);
void cdf_get_vara_int(int ncid, int varid, const size_t start[],
                       const size_t count[], int *dp);

void cdf_put_att_text(int ncid, int varid, const char *name, size_t len, const char *tp);
void cdf_put_att_int(int ncid, int varid, const char *name, nc_type xtype, size_t len, const int *ip);
void cdf_put_att_float(int ncid, int varid, const char *name, nc_type xtype, size_t len, const float *dp);
void cdf_put_att_double(int ncid, int varid, const char *name, nc_type xtype, size_t len, const double *dp);

void cdf_get_att_string(int ncid, int varid, const char *name, char **tp);
void cdf_get_att_text (int ncid, int varid, const char *name, char *tp);
void cdf_get_att_int (int ncid, int varid, const char *name, int *ip);
void cdf_get_att_double(int ncid, int varid, const char *name, double *dp);

void cdf_inq_att (int ncid, int varid, const char *name, nc_type * xtypep, size_t * lenp);
void cdf_inq_atttype(int ncid, int varid, const char *name, nc_type *xtypep);
void cdf_inq_attlen (int ncid, int varid, const char *name, size_t *lenp);
void cdf_inq_attname(int ncid, int varid, int attnum, char *name);
void cdf_inq_attid (int ncid, int varid, const char *name, int *attnump);

typedef int (*cdi_nc__create_funcp)(const char *path, int cmode,
                                    size_t initialsz, size_t *chunksizehintp,
                                    int *ncidp);

typedef void (*cdi_cdf_def_var_funcp)(int ncid, const char *name,
                                      nc_type xtype, int ndims,
                                      const int dimids[], int *varidp);

#endif

#endif
#if defined (HAVE_CONFIG_H)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>



const char *cdfLibraryVersion(void)
{
#if defined (HAVE_LIBNETCDF)
  return (nc_inq_libvers());
#else
  return ("library undefined");
#endif
}

#if defined(HAVE_LIBHDF5)
#if defined(__cplusplus)
extern "C" {
#endif
  int H5get_libversion(unsigned *, unsigned *, unsigned *);
#if defined(__cplusplus)
}
#endif
#endif

const char *hdfLibraryVersion(void)
{
#if defined(HAVE_LIBHDF5)
  static char hdf_libvers[256];
  unsigned majnum, minnum, relnum;

  H5get_libversion(&majnum, &minnum, &relnum);

  sprintf(hdf_libvers, "%u.%u.%u", majnum, minnum, relnum);

  return (hdf_libvers);
#else
  return ("library undefined");
#endif
}


int CDF_Debug = 0;


void cdfDebug(int debug)
{
  CDF_Debug = debug;

  if ( CDF_Debug )
    Message("debug level %d", debug);
}

#if defined (HAVE_LIBNETCDF)
static
void cdfComment(int ncid)
{
  static char comment[256] = "Climate Data Interface version ";
  static int init = 0;

  if ( ! init )
    {
      init = 1;
      const char *libvers = cdiLibraryVersion();
      const char *blank = strchr(libvers, ' ');
      size_t size = blank ? (size_t)(blank - libvers) : 0;

      if ( size == 0 || ! isdigit((int) *libvers) )
        strcat(comment, "??");
      else
        strncat(comment, libvers, size);
      strcat(comment, " (http://mpimet.mpg.de/cdi)");
    }

  cdf_put_att_text(ncid, NC_GLOBAL, "CDI", strlen(comment), comment);
}
#endif

static int cdfOpenFile(const char *filename, const char *mode, int *filetype)
{
  int ncid = -1;
#if defined (HAVE_LIBNETCDF)
  int fmode = tolower(*mode);
  int writemode = NC_CLOBBER;
  int readmode = NC_NOWRITE;
  int status;

  if ( filename == NULL )
    ncid = CDI_EINVAL;
  else
    {
      switch (fmode)
        {
        case 'r':
          status = cdf_open(filename, readmode, &ncid);
          if ( status > 0 && ncid < 0 ) ncid = CDI_ESYSTEM;
#if defined (HAVE_NETCDF4)
          else
            {
              int format;
              (void) nc_inq_format(ncid, &format);
              if ( format == NC_FORMAT_NETCDF4_CLASSIC )
                {
                  *filetype = FILETYPE_NC4C;
                }
            }
#endif
          break;
        case 'w':
#if defined (NC_64BIT_OFFSET)
          if ( *filetype == FILETYPE_NC2 ) writemode |= NC_64BIT_OFFSET;
#endif
#if defined (HAVE_NETCDF4)
          if ( *filetype == FILETYPE_NC4 ) writemode |= NC_NETCDF4;
          else if ( *filetype == FILETYPE_NC4C ) writemode |= NC_NETCDF4 | NC_CLASSIC_MODEL;
#endif
          cdf_create(filename, writemode, &ncid);
          if ( CDI_Version_Info ) cdfComment(ncid);
          cdf_put_att_text(ncid, NC_GLOBAL, "Conventions", 6, "CF-1.4");
          break;
        case 'a':
          cdf_open(filename, NC_WRITE, &ncid);
          break;
        default:
          ncid = CDI_EINVAL;
        }
    }
#endif

  return (ncid);
}


int cdfOpen(const char *filename, const char *mode)
{
  int fileID = 0;
  int filetype = FILETYPE_NC;

  if ( CDF_Debug )
    Message("Open %s with mode %c", filename, *mode);

  fileID = cdfOpenFile(filename, mode, &filetype);

  if ( CDF_Debug )
    Message("File %s opened with id %d", filename, fileID);

  return (fileID);
}


int cdfOpen64(const char *filename, const char *mode)
{
  int fileID = -1;
  int open_file = TRUE;
  int filetype = FILETYPE_NC2;

  if ( CDF_Debug )
    Message("Open %s with mode %c", filename, *mode);

#if defined (HAVE_LIBNETCDF)
#if ! defined (NC_64BIT_OFFSET)
  open_file = FALSE;
#endif
#endif

  if ( open_file )
    {
      fileID = cdfOpenFile(filename, mode, &filetype);

      if ( CDF_Debug )
        Message("File %s opened with id %d", filename, fileID);
    }
  else
    {
      fileID = CDI_ELIBNAVAIL;
    }

  return (fileID);
}


int cdf4Open(const char *filename, const char *mode, int *filetype)
{
  int fileID = -1;
  int open_file = FALSE;

  if ( CDF_Debug )
    Message("Open %s with mode %c", filename, *mode);

#if defined (HAVE_NETCDF4)
  open_file = TRUE;
#endif

  if ( open_file )
    {
      fileID = cdfOpenFile(filename, mode, filetype);

      if ( CDF_Debug )
        Message("File %s opened with id %d", filename, fileID);
    }
  else
    {
      fileID = CDI_ELIBNAVAIL;
    }

  return (fileID);
}


static void cdfCloseFile(int fileID)
{
#if defined (HAVE_LIBNETCDF)
  cdf_close(fileID);
#endif
}

void cdfClose(int fileID)
{
  cdfCloseFile(fileID);
}
#ifndef NAMESPACE_H
#define NAMESPACE_H

#ifdef HAVE_CONFIG_H
#endif


typedef struct {
  int idx;
  int nsp;
} namespaceTuple_t;

enum namespaceSwitch
{
  NSSWITCH_NO_SUCH_SWITCH = -1,
  NSSWITCH_ABORT,
  NSSWITCH_WARNING,
  NSSWITCH_SERIALIZE_GET_SIZE,
  NSSWITCH_SERIALIZE_PACK,
  NSSWITCH_SERIALIZE_UNPACK,
  NSSWITCH_FILE_OPEN,
  NSSWITCH_FILE_WRITE,
  NSSWITCH_FILE_CLOSE,
  NSSWITCH_STREAM_OPEN_BACKEND,
  NSSWITCH_STREAM_DEF_VLIST_,
  NSSWITCH_STREAM_SETUP_VLIST,
  NSSWITCH_STREAM_WRITE_VAR_,
  NSSWITCH_STREAM_WRITE_VAR_CHUNK_,
  NSSWITCH_STREAM_WRITE_VAR_PART_,
  NSSWITCH_STREAM_WRITE_SCATTERED_VAR_PART_,
  NSSWITCH_STREAM_CLOSE_BACKEND,
  NSSWITCH_STREAM_DEF_TIMESTEP_,
  NSSWITCH_STREAM_SYNC,
#ifdef HAVE_LIBNETCDF
  NSSWITCH_NC__CREATE,
  NSSWITCH_CDF_DEF_VAR,
  NSSWITCH_CDF_DEF_TIMESTEP,
  NSSWITCH_CDF_STREAM_SETUP,
#endif
  NUM_NAMESPACE_SWITCH,
};

union namespaceSwitchValue
{
  void *data;
  void (*func)();
};

#define NSSW_FUNC(p) ((union namespaceSwitchValue){ .func = (void (*)())(p) })
#define NSSW_DATA(p) ((union namespaceSwitchValue){ .data = (void *)(p) })



void namespaceCleanup ( void );
int namespaceGetNumber ( void );

int namespaceGetActive ( void );
int namespaceIdxEncode ( namespaceTuple_t );
int namespaceIdxEncode2 ( int, int );
namespaceTuple_t namespaceResHDecode ( int );
int namespaceAdaptKey ( int originResH, int originNamespace);
int namespaceAdaptKey2 ( int );
void namespaceSwitchSet(enum namespaceSwitch sw,
                        union namespaceSwitchValue value);
union namespaceSwitchValue namespaceSwitchGet(enum namespaceSwitch sw);


#endif
#if defined (HAVE_CONFIG_H)
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>


#if defined (HAVE_LIBNETCDF)







void cdf_create(const char *path, int cmode, int *ncidp)
{
  int oldfill;
  size_t initialsz = 0, chunksizehint = 0;
#if defined(__SX__) || defined(ES)
  chunksizehint = 16777216;
#endif

  if ( cdiNcChunksizehint != CDI_UNDEFID )
    chunksizehint = (size_t)cdiNcChunksizehint;

  cdi_nc__create_funcp my_nc__create =
    (cdi_nc__create_funcp)namespaceSwitchGet(NSSWITCH_NC__CREATE).func;
  int status = my_nc__create(path, cmode, initialsz, &chunksizehint, ncidp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d  mode = %d  file = %s", *ncidp, cmode, path);

  if ( CDF_Debug || status != NC_NOERR )
    Message("chunksizehint %d", chunksizehint);

  if ( status != NC_NOERR ) Error("%s: %s", path, nc_strerror(status));

  status = nc_set_fill(*ncidp, NC_NOFILL, &oldfill);

  if ( status != NC_NOERR ) Error("%s: %s", path, nc_strerror(status));
}


int cdf_open(const char *path, int omode, int *ncidp)
{
  int status = 0;
  int dapfile = FALSE;
  struct stat filestat;
  size_t chunksizehint = 0;

#if defined (HAVE_LIBNC_DAP)
  if ( strncmp(path, "http:", 5) == 0 || strncmp(path, "https:", 6) == 0 ) dapfile = TRUE;
#endif

  if ( dapfile )
    {
      status = nc_open(path, omode, ncidp);
    }
  else
    {
      if ( stat(path, &filestat) != 0 ) SysError(path);

#if defined (HAVE_STRUCT_STAT_ST_BLKSIZE)
      chunksizehint = (size_t) filestat.st_blksize * 4;
#endif



      if ( cdiNcChunksizehint != CDI_UNDEFID )
        chunksizehint = (size_t)cdiNcChunksizehint;


      status = nc__open(path, omode, &chunksizehint, ncidp);

      if ( CDF_Debug ) Message("chunksizehint %d", chunksizehint);
    }

  if ( CDF_Debug )
    Message("ncid = %d  mode = %d  file = %s", *ncidp, omode, path);

  if ( CDF_Debug && status != NC_NOERR ) Message("%s", nc_strerror(status));

  return status;
}


void cdf_close(int ncid)
{
  int status = nc_close(ncid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_redef(int ncid)
{
  int status = nc_redef(ncid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_enddef(int ncid)
{
  int status = nc_enddef(ncid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf__enddef(const int ncid, const size_t hdr_pad)
{
  const size_t v_align = 4UL;
  const size_t v_minfree = 0UL;
  const size_t r_align = 4UL;


  int status = nc__enddef(ncid, hdr_pad, v_align, v_minfree, r_align);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_sync(int ncid)
{
  int status = nc_sync(ncid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_inq(int ncid, int *ndimsp, int *nvarsp, int *ngattsp, int *unlimdimidp)
{
  int status = nc_inq(ncid, ndimsp, nvarsp, ngattsp, unlimdimidp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d ndims = %d nvars = %d ngatts = %d unlimid = %d",
            ncid, *ndimsp, *nvarsp, *ngattsp, *unlimdimidp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_def_dim(int ncid, const char *name, size_t len, int *dimidp)
{
  int status = nc_def_dim(ncid, name, len, dimidp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d  name = %s  len = %d", ncid, name, len);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_inq_dimid(int ncid, const char *name, int *dimidp)
{
  int status = nc_inq_dimid(ncid, name, dimidp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d  name = %s  dimid= %d", ncid, name, *dimidp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_inq_dim(int ncid, int dimid, char *name, size_t * lengthp)
{
  int status = nc_inq_dim(ncid, dimid, name, lengthp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d  dimid = %d  length = %d name = %s", ncid, dimid, *lengthp, name);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_inq_dimname(int ncid, int dimid, char *name)
{
  int status = nc_inq_dimname(ncid, dimid, name);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d  dimid = %d  name = %s", ncid, dimid, name);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_inq_dimlen(int ncid, int dimid, size_t * lengthp)
{
  int status = nc_inq_dimlen(ncid, dimid, lengthp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d dimid = %d length = %d", ncid, dimid, *lengthp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_def_var(int ncid, const char *name, nc_type xtype, int ndims,
                 const int dimids[], int *varidp)
{
  cdi_cdf_def_var_funcp my_cdf_def_var
    = (cdi_cdf_def_var_funcp)namespaceSwitchGet(NSSWITCH_CDF_DEF_VAR).func;
  my_cdf_def_var(ncid, name, xtype, ndims, dimids, varidp);
}

void
cdf_def_var_serial(int ncid, const char *name, nc_type xtype, int ndims,
                   const int dimids[], int *varidp)
{
  int status = nc_def_var(ncid, name, xtype, ndims, dimids, varidp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d  name = %s  xtype = %d  ndims = %d  varid = %d",
            ncid, name, xtype, ndims, *varidp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}



void cdf_inq_varid(int ncid, const char *name, int *varidp)
{
  int status = nc_inq_varid(ncid, name, varidp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d  name = %s  varid = %d ", ncid, name, *varidp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_inq_nvars(int ncid, int *nvarsp)
{
  int status = nc_inq_nvars(ncid, nvarsp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d  nvars = %d", ncid, *nvarsp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_inq_var(int ncid, int varid, char *name, nc_type *xtypep, int *ndimsp,
                 int dimids[], int *nattsp)
{
  int status = nc_inq_var(ncid, varid, name, xtypep, ndimsp, dimids, nattsp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d ndims = %d xtype = %d natts = %d name = %s",
            ncid, varid, *ndimsp, *xtypep, *nattsp, name);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_inq_varname(int ncid, int varid, char *name)
{
  int status = nc_inq_varname(ncid, varid, name);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d name = %s", ncid, varid, name);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_inq_vartype(int ncid, int varid, nc_type *xtypep)
{
  int status = nc_inq_vartype(ncid, varid, xtypep);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d xtype = %s", ncid, varid, *xtypep);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_inq_varndims(int ncid, int varid, int *ndimsp)
{
  int status = nc_inq_varndims(ncid, varid, ndimsp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d", ncid, varid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_inq_vardimid(int ncid, int varid, int dimids[])
{
  int status = nc_inq_vardimid(ncid, varid, dimids);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d", ncid, varid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_inq_varnatts(int ncid, int varid, int *nattsp)
{
  int status = nc_inq_varnatts(ncid, varid, nattsp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d nattsp = %d", ncid, varid, *nattsp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_put_var_text(int ncid, int varid, const char *tp)
{
  int status = nc_put_var_text(ncid, varid, tp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("%d %d %s", ncid, varid, tp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_put_var_short(int ncid, int varid, const short *sp)
{
  int status = nc_put_var_short(ncid, varid, sp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("%d %d %hd", ncid, varid, *sp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_put_var_int(int ncid, int varid, const int *ip)
{
  int status = nc_put_var_int(ncid, varid, ip);

  if ( CDF_Debug || status != NC_NOERR )
    Message("%d %d %d", ncid, varid, *ip);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_put_var_long(int ncid, int varid, const long *lp)
{
  int status = nc_put_var_long(ncid, varid, lp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("%d %d %ld", ncid, varid, *lp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_put_var_float(int ncid, int varid, const float *fp)
{
  int status = nc_put_var_float(ncid, varid, fp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("%d %d %f", ncid, varid, *fp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_put_vara_double(int ncid, int varid, const size_t start[],
                         const size_t count[], const double *dp)
{
  int status = nc_put_vara_double(ncid, varid, start, count, dp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d val0 = %f", ncid, varid, *dp);

  if ( status != NC_NOERR )
    {
      char name[256];
      nc_inq_varname(ncid, varid, name);
      Message("varname = %s", name);
    }

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_put_vara_float(int ncid, int varid, const size_t start[],
                         const size_t count[], const float *fp)
{
  int status = nc_put_vara_float(ncid, varid, start, count, fp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d val0 = %f", ncid, varid, *fp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_get_vara_int(int ncid, int varid, const size_t start[],
                       const size_t count[], int *dp)
{
  int status = nc_get_vara_int(ncid, varid, start, count, dp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d", ncid, varid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_get_vara_double(int ncid, int varid, const size_t start[],
                          const size_t count[], double *dp)
{
  int status = nc_get_vara_double(ncid, varid, start, count, dp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d", ncid, varid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_get_vara_float(int ncid, int varid, const size_t start[],
                         const size_t count[], float *fp)
{
  int status = nc_get_vara_float(ncid, varid, start, count, fp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d", ncid, varid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_get_vara_text(int ncid, int varid, const size_t start[],
                        const size_t count[], char *tp)
{
  int status = nc_get_vara_text(ncid, varid, start, count, tp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d", ncid, varid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_get_vara_uchar(int ncid, int varid, const size_t start[], const size_t count[], unsigned char *tp)
{
  int status = nc_get_vara_uchar(ncid, varid, start, count, tp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d", ncid, varid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_put_var_double(int ncid, int varid, const double *dp)
{
  int status = nc_put_var_double(ncid, varid, dp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d val0 = %f", ncid, varid, *dp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_get_var1_text(int ncid, int varid, const size_t index[], char *tp)
{
  int status = nc_get_var1_text(ncid, varid, index, tp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d", ncid, varid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_get_var1_double(int ncid, int varid, const size_t index[], double *dp)
{
  int status = nc_get_var1_double(ncid, varid, index, dp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d", ncid, varid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_put_var1_double(int ncid, int varid, const size_t index[], const double *dp)
{
  int status = nc_put_var1_double(ncid, varid, index, dp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d val = %f", ncid, varid, *dp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_get_var_text(int ncid, int varid, char *tp)
{
  int status = nc_get_var_text(ncid, varid, tp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d", ncid, varid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_get_var_short(int ncid, int varid, short *sp)
{
  int status = nc_get_var_short(ncid, varid, sp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d", ncid, varid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_get_var_int(int ncid, int varid, int *ip)
{
  int status = nc_get_var_int(ncid, varid, ip);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d", ncid, varid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_get_var_long(int ncid, int varid, long *lp)
{
  int status = nc_get_var_long(ncid, varid, lp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d", ncid, varid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_get_var_float(int ncid, int varid, float *fp)
{
  int status = nc_get_var_float(ncid, varid, fp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d", ncid, varid);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_get_var_double(int ncid, int varid, double *dp)
{
  int status = nc_get_var_double(ncid, varid, dp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d val[0] = %f", ncid, varid, *dp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_copy_att(int ncid_in, int varid_in, const char *name, int ncid_out,
                  int varid_out)
{
  int status = nc_copy_att(ncid_in, varid_in, name, ncid_out, varid_out);

  if ( CDF_Debug || status != NC_NOERR )
    Message("%d %d %s %d %d", ncid_in, varid_out, name, ncid_out, varid_out);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_put_att_text(int ncid, int varid, const char *name, size_t len,
                      const char *tp)
{
  int status = nc_put_att_text(ncid, varid, name, len, tp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d att = %s text = %.*s", ncid, varid, name, (int)len, tp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_put_att_int(int ncid, int varid, const char *name, nc_type xtype,
                     size_t len, const int *ip)
{
  int status = nc_put_att_int(ncid, varid, name, xtype, len, ip);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d att = %s val = %d", ncid, varid, name, *ip);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_put_att_float(int ncid, int varid, const char *name, nc_type xtype,
                       size_t len, const float *dp)
{
  int status = nc_put_att_float(ncid, varid, name, xtype, len, dp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d att = %s val = %g", ncid, varid, name, *dp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_put_att_double(int ncid, int varid, const char *name, nc_type xtype,
                        size_t len, const double *dp)
{
  int status = nc_put_att_double(ncid, varid, name, xtype, len, dp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d att = %s val = %g", ncid, varid, name, *dp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_get_att_text(int ncid, int varid, const char *name, char *tp)
{
  int status = nc_get_att_text(ncid, varid, name, tp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d name = %s", ncid, varid, name);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}

void cdf_get_att_string(int ncid, int varid, const char *name, char **tp)
{
#if defined (HAVE_NETCDF4)
  int status = nc_get_att_string(ncid, varid, name, tp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d name = %s", ncid, varid, name);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
#endif
}


void cdf_get_att_int(int ncid, int varid, const char *name, int *ip)
{
  int status = nc_get_att_int(ncid, varid, name, ip);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d att = %s val = %d", ncid, varid, name, *ip);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_get_att_double(int ncid, int varid, const char *name, double *dp)
{
  int status;

  status = nc_get_att_double(ncid, varid, name, dp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d att = %s val = %.9g", ncid, varid, name, *dp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_inq_att(int ncid, int varid, const char *name, nc_type *xtypep,
                 size_t *lenp)
{
  int status = nc_inq_att(ncid, varid, name, xtypep, lenp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d att = %s", ncid, varid, name);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_inq_atttype(int ncid, int varid, const char *name, nc_type * xtypep)
{
  int status = nc_inq_atttype(ncid, varid, name, xtypep);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d att = %s", ncid, varid, name);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_inq_attlen(int ncid, int varid, const char *name, size_t * lenp)
{
  int status = nc_inq_attlen(ncid, varid, name, lenp);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d att = %s len = %d", ncid, varid, name, *lenp);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_inq_attname(int ncid, int varid, int attnum, char *name)
{
  int status = nc_inq_attname(ncid, varid, attnum, name);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d attnum = %d att = %s", ncid, varid, attnum, name);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}


void cdf_inq_attid(int ncid, int varid, const char *name, int *attnump)
{
  int status = nc_inq_attid(ncid, varid, name, attnump);

  if ( CDF_Debug || status != NC_NOERR )
    Message("ncid = %d varid = %d att = %s", ncid, varid, name);

  if ( status != NC_NOERR ) Error("%s", nc_strerror(status));
}

#endif
#ifndef CDI_CKSUM_H_
#define CDI_CKSUM_H_

#include <inttypes.h>

uint32_t cdiCheckSum(int type, int count, const void *data);

#endif
#ifdef HAVE_CONFIG_H
#endif

#include <inttypes.h>
#include <sys/types.h>

void
memcrc_r(uint32_t *state, const unsigned char *block, size_t block_len);

void
memcrc_r_eswap(uint32_t *state, const unsigned char *elems, size_t num_elems,
               size_t elem_size);

uint32_t
memcrc_finish(uint32_t *state, off_t total_size);

uint32_t
memcrc(const unsigned char *b, size_t n);
#ifdef HAVE_CONFIG_H
#endif

#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <string.h>

#ifndef CDI_CKSUM_H_
#endif
#ifndef _ERROR_H
#endif




int serializeGetSize(int count, int datatype, void *context);
void serializePack(const void *data, int count, int datatype,
                   void *buf, int buf_size, int *position, void *context);
void serializeUnpack(const void *buf, int buf_size, int *position,
                     void *data, int count, int datatype, void *context);




static inline int
serializeStrTabGetPackSize(const char **strTab, int numStr,
                           void *context)
{
  xassert(numStr >= 0);
  int packBuffSize = 0;
  for (size_t i = 0; i < (size_t)numStr; ++i)
  {
    size_t len = strlen(strTab[i]);
    packBuffSize +=
      serializeGetSize(1, DATATYPE_INT, context)
      + serializeGetSize((int)len, DATATYPE_TXT, context);
  }
  packBuffSize +=
    serializeGetSize(1, DATATYPE_UINT32, context);
  return packBuffSize;
}

static inline void
serializeStrTabPack(const char **strTab, int numStr,
                    void *buf, int buf_size, int *position, void *context)
{
  uint32_t d = 0;
  xassert(numStr >= 0);
  for (size_t i = 0; i < (size_t)numStr; ++i)
  {
    int len = (int)strlen(strTab[i]);
    serializePack(&len, 1, DATATYPE_INT,
                  buf, buf_size, position, context);
    serializePack(strTab[i], len, DATATYPE_TXT,
                  buf, buf_size, position, context);
    d ^= cdiCheckSum(DATATYPE_TXT, len, strTab[i]);
  }
  serializePack(&d, 1, DATATYPE_UINT32,
                buf, buf_size, position, context);
}

static inline void
serializeStrTabUnpack(const void *buf, int buf_size, int *position,
                      char **strTab, int numStr, void *context)
{
  uint32_t d, d2 = 0;
  xassert(numStr >= 0);
  for (size_t i = 0; i < (size_t)numStr; ++i)
    {
      int len;
      serializeUnpack(buf, buf_size, position,
                      &len, 1, DATATYPE_INT, context);
      serializeUnpack(buf, buf_size, position,
                      strTab[i], len, DATATYPE_TXT, context);
      strTab[i][len] = '\0';
      d2 ^= cdiCheckSum(DATATYPE_TXT, len, strTab[i]);
    }
  serializeUnpack(buf, buf_size, position,
                  &d, 1, DATATYPE_UINT32, context);
  xassert(d == d2);
}




int serializeGetSizeInCore(int count, int datatype, void *context);
void serializePackInCore(const void *data, int count, int datatype,
                          void *buf, int buf_size, int *position, void *context);
void serializeUnpackInCore(const void *buf, int buf_size, int *position,
                           void *data, int count, int datatype, void *context);

#endif
#ifdef HAVE_CONFIG_H
#endif

#include <inttypes.h>
#include <sys/types.h>
#include <stdlib.h>


uint32_t cdiCheckSum(int type, int count, const void *buffer)
{
  uint32_t s = 0U;
  xassert(count >= 0);
  size_t elemSize = (size_t)serializeGetSizeInCore(1, type, NULL);
  memcrc_r_eswap(&s, (const unsigned char *)buffer, (size_t)count, elemSize);
  s = memcrc_finish(&s, (off_t)(elemSize * (size_t)count));
  return s;
}
#if defined (HAVE_CONFIG_H)
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>

const char *cdiStringError(int cdiErrno)
{
  static const char UnknownError[] = "Unknown Error";
  static const char _EUFTYPE[] = "Unsupported file type";
  static const char _ELIBNAVAIL[] = "Unsupported file type (library support not compiled in)";
  static const char _EUFSTRUCT[] = "Unsupported file structure";
  static const char _EUNC4[] = "Unsupported netCDF4 structure";
  static const char _ELIMIT[] = "Internal limits exceeded";

  switch (cdiErrno) {
  case CDI_ESYSTEM:
    {
      const char *cp = strerror(errno);
      if ( cp == NULL ) break;
      return cp;
    }
  case CDI_EUFTYPE: return _EUFTYPE;
  case CDI_ELIBNAVAIL: return _ELIBNAVAIL;
  case CDI_EUFSTRUCT: return _EUFSTRUCT;
  case CDI_EUNC4: return _EUNC4;
  case CDI_ELIMIT: return _ELIMIT;
  }

  return UnknownError;
}
#ifndef _DMEMORY_H
#define _DMEMORY_H

#include <stdio.h>


#define DEBUG_MEMORY

#ifndef WITH_FUNCTION_NAME
#define WITH_FUNCTION_NAME
#endif

extern size_t memTotal(void);
extern void memDebug(int debug);
extern void memExitOnError(void);

#if defined DEBUG_MEMORY

extern void *memRealloc(void *ptr, size_t size, const char *file, const char *functionname, int line);
extern void *memCalloc (size_t nmemb, size_t size, const char *file, const char *functionname, int line);
extern void *memMalloc (size_t size, const char *file, const char *functionname, int line);
extern void memFree (void *ptr, const char *file, const char *functionname, int line);

#if defined WITH_FUNCTION_NAME
#define Realloc(p,s) memRealloc((p), (s), __FILE__, __func__, __LINE__)
#define Calloc(n,s) memCalloc((n), (s), __FILE__, __func__, __LINE__)
#define Malloc(s) memMalloc((s), __FILE__, __func__, __LINE__)
#define Free(p) memFree((p), __FILE__, __func__, __LINE__)
#else
#define Realloc(p,s) memRealloc((p), (s), __FILE__, (void *) NULL, __LINE__)
#define Calloc(n,s) memCalloc((n), (s), __FILE__, (void *) NULL, __LINE__)
#define Malloc(s) memMalloc((s), __FILE__, (void *) NULL, __LINE__)
#define Free(p) memFree((p), __FILE__, (void *) NULL, __LINE__)
#endif

#endif

#endif
#ifndef _FILE_H
#define _FILE_H

#include <stdio.h>
#include <sys/types.h>


#define FILE_UNDEFID -1

#define FILE_TYPE_OPEN 1
#define FILE_TYPE_FOPEN 2


#define FILE_BUFTYPE_STD 1
#define FILE_BUFTYPE_MMAP 2

const
char *fileLibraryVersion(void);

void fileDebug(int debug);

void *filePtr(int fileID);

int fileSetBufferType(int fileID, int type);
void fileSetBufferSize(int fileID, long buffersize);

int fileOpen(const char *filename, const char *mode);
int fileOpen_serial(const char *filename, const char *mode);
int fileClose(int fileID);
int fileClose_serial(int fileID);

char *fileInqName(int fileID);
int fileInqMode(int fileID);

int fileFlush(int fileID);
void fileClearerr(int fileID);
int fileEOF(int fileID);
int filePtrEOF(void *fileptr);
void fileRewind(int fileID);

off_t fileGetPos(int fileID);
int fileSetPos(int fileID, off_t offset, int whence);

int fileGetc(int fileID);
int filePtrGetc(void *fileptr);

size_t filePtrRead(void *fileptr, void *restrict ptr, size_t size);
size_t fileRead(int fileID, void *restrict ptr, size_t size);
size_t fileWrite(int fileID, const void *restrict ptr, size_t size);

#endif
#ifndef _STREAM_CDF_H
#define _STREAM_CDF_H

void cdfDefVars(stream_t *streamptr);
void cdfDefTimestep(stream_t *streamptr, int tsID);
int cdfInqTimestep(stream_t *streamptr, int tsID);
int cdfInqContents(stream_t *streamptr);
void cdfDefHistory(stream_t *streamptr, int size, const char *history);
int cdfInqHistorySize(stream_t *streamptr);
void cdfInqHistoryString(stream_t *streamptr, char *history);

void cdfEndDef(stream_t * streamptr);
void cdfDefRecord(stream_t * streamptr);

void cdfCopyRecord(stream_t *streamptr2, stream_t *streamptr1);

void cdf_read_record(stream_t *streamptr, int memtype, void *data, int *nmiss);
void cdf_write_record(stream_t *streamptr, int memtype, const void *data, int nmiss);

void cdfReadVarDP(stream_t *streamptr, int varID, double *data, int *nmiss);
void cdfReadVarSP(stream_t *streamptr, int varID, float *data, int *nmiss);

void cdf_write_var(stream_t *streamptr, int varID, int memtype, const void *data, int nmiss);

void cdfReadVarSliceDP(stream_t *streamptr, int varID, int levelID, double *data, int *nmiss);
void cdfReadVarSliceSP(stream_t *streamptr, int varID, int levelID, float *data, int *nmiss);
void cdf_write_var_slice(stream_t *streamptr, int varID, int levelID, int memtype, const void *data, int nmiss);

void cdf_write_var_chunk(stream_t *streamptr, int varID, int memtype,
                           const int rect[][2], const void *data, int nmiss);

void cdfDefVarDeflate(int ncid, int ncvarid, int deflate_level);
void cdfDefTime(stream_t* streamptr);

#endif
#if defined (HAVE_CONFIG_H)
#endif

#include <stdarg.h>
#include <ctype.h>

#ifdef HAVE_LIBNETCDF
#endif

#if defined (HAVE_LIBCGRIBEX)
#endif

int cdiDefaultCalendar = CALENDAR_PROLEPTIC;

int cdiDefaultInstID = CDI_UNDEFID;
int cdiDefaultModelID = CDI_UNDEFID;
int cdiDefaultTableID = CDI_UNDEFID;

int cdiNcChunksizehint = CDI_UNDEFID;
int cdiChunkType = CHUNK_GRID;
int cdiSplitLtype105 = CDI_UNDEFID;

int cdiIgnoreAttCoordinates = FALSE;
int cdiIgnoreValidRange = FALSE;
int cdiSkipRecords = 0;
int cdiConvention = CDI_CONVENTION_ECHAM;
int cdiInventoryMode = 1;
int CDI_Version_Info = 1;
int CDI_cmor_mode = 0;
size_t CDI_netcdf_hdr_pad = 0UL;

char *cdiPartabPath = NULL;
int cdiPartabIntern = 1;

double cdiDefaultMissval = -9.E33;

static const char Filetypes[][9] = {
  "UNKNOWN",
  "GRIB",
  "GRIB2",
  "netCDF",
  "netCDF2",
  "netCDF4",
  "netCDF4c",
  "SERVICE",
  "EXTRA",
  "IEG",
  "HDF5",
};

int CDI_Debug = 0;
int CDI_Recopt = 0;

int cdiGribApiDebug = 0;
int cdiDefaultLeveltype = -1;
int cdiDataUnreduced = 0;
int cdiSortName = 0;
int cdiHaveMissval = 0;


static long cdiGetenvInt(const char *envName)
{
  char *envString;
  long envValue = -1;
  long fact = 1;

  envString = getenv(envName);

  if ( envString )
    {
      int loop, len;

      len = (int) strlen(envString);
      for ( loop = 0; loop < len; loop++ )
        {
          if ( ! isdigit((int) envString[loop]) )
            {
              switch ( tolower((int) envString[loop]) )
                {
                case 'k': fact = 1024; break;
                case 'm': fact = 1048576; break;
                case 'g': fact = 1073741824; break;
                default:
                  fact = 0;
                  Message("Invalid number string in %s: %s", envName, envString);
                  Warning("%s must comprise only digits [0-9].",envName);
                  break;
                }
              break;
            }
        }

      if ( fact ) envValue = fact*atol(envString);

      if ( CDI_Debug ) Message("set %s to %ld", envName, envValue);
    }

  return (envValue);
}

static void
cdiPrintDefaults(void)
{
  fprintf(stderr, "default instID     :  %d\n"
          "default modelID    :  %d\n"
          "default tableID    :  %d\n"
          "default missval    :  %g\n", cdiDefaultInstID,
          cdiDefaultModelID, cdiDefaultTableID, cdiDefaultMissval);
}

void cdiPrintVersion(void)
{
  fprintf(stderr, "     CDI library version : %s\n", cdiLibraryVersion());
#if defined (HAVE_LIBCGRIBEX)
  fprintf(stderr, " CGRIBEX library version : %s\n", cgribexLibraryVersion());
#endif
#if defined (HAVE_LIBNETCDF)
  fprintf(stderr, "  netCDF library version : %s\n", cdfLibraryVersion());
#endif
#if defined (HAVE_LIBHDF5)
  fprintf(stderr, "    HDF5 library version : %s\n", hdfLibraryVersion());
#endif
#if defined (HAVE_LIBSERVICE)
  fprintf(stderr, " SERVICE library version : %s\n", srvLibraryVersion());
#endif
#if defined (HAVE_LIBEXTRA)
  fprintf(stderr, "   EXTRA library version : %s\n", extLibraryVersion());
#endif
#if defined (HAVE_LIBIEG)
  fprintf(stderr, "     IEG library version : %s\n", iegLibraryVersion());
#endif
  fprintf(stderr, "    FILE library version : %s\n", fileLibraryVersion());
}

void cdiDebug(int level)
{
  if ( level == 1 || (level & 2) ) CDI_Debug = 1;

  if ( CDI_Debug ) Message("debug level %d", level);

  if ( level == 1 || (level & 4) ) memDebug(1);

  if ( level == 1 || (level & 8) ) fileDebug(1);

  if ( level == 1 || (level & 16) )
    {
#if defined (HAVE_LIBCGRIBEX)
      gribSetDebug(1);
#endif
#if defined (HAVE_LIBNETCDF)
      cdfDebug(1);
#endif
#if defined (HAVE_LIBSERVICE)
      srvDebug(1);
#endif
#if defined (HAVE_LIBEXTRA)
      extDebug(1);
#endif
#if defined (HAVE_LIBIEG)
      iegDebug(1);
#endif
    }

  if ( CDI_Debug )
    {
      cdiPrintDefaults();
      cdiPrintDatatypes();
    }
}


int cdiHaveFiletype(int filetype)
{
  int status = 0;

  switch (filetype)
    {
#if defined (HAVE_LIBSERVICE)
    case FILETYPE_SRV: { status = 1; break; }
#endif
#if defined (HAVE_LIBEXTRA)
    case FILETYPE_EXT: { status = 1; break; }
#endif
#if defined (HAVE_LIBIEG)
    case FILETYPE_IEG: { status = 1; break; }
#endif
#if defined (HAVE_LIBNETCDF)
    case FILETYPE_NC: { status = 1; break; }
#if defined (HAVE_NETCDF2)
    case FILETYPE_NC2: { status = 1; break; }
#endif
#if defined (HAVE_NETCDF4)
    case FILETYPE_NC4: { status = 1; break; }
    case FILETYPE_NC4C: { status = 1; break; }
#endif
#endif
    default: { status = 0; break; }
    }

  return (status);
}

void cdiDefTableID(int tableID)
{
  cdiDefaultTableID = tableID;
  int modelID = cdiDefaultModelID = tableInqModel(tableID);
  cdiDefaultInstID = modelInqInstitut(modelID);
}

static
void cdiSetChunk(const char *chunkAlgo)
{


  int algo = -1;

  if ( strcmp("auto", chunkAlgo) == 0 ) algo = CHUNK_AUTO;
  else if ( strcmp("grid", chunkAlgo) == 0 ) algo = CHUNK_GRID;
  else if ( strcmp("lines", chunkAlgo) == 0 ) algo = CHUNK_LINES;
  else
    Warning("Invalid environment variable CDI_CHUNK_ALGO: %s", chunkAlgo);

  if ( algo != -1 )
    {
      cdiChunkType = algo;
      if ( CDI_Debug ) Message("set ChunkAlgo to %s", chunkAlgo);
    }
}


void cdiInitialize(void)
{
  static int Init_CDI = FALSE;
  char *envstr;
  long value;

  if ( ! Init_CDI )
    {
      Init_CDI = TRUE;

#if defined (HAVE_LIBCGRIBEX)
      gribFixZSE(1);
      gribSetConst(1);
#endif

      value = cdiGetenvInt("CDI_DEBUG");
      if ( value >= 0 ) CDI_Debug = (int) value;

      value = cdiGetenvInt("CDI_GRIBAPI_DEBUG");
      if ( value >= 0 ) cdiGribApiDebug = (int) value;

      value = cdiGetenvInt("CDI_RECOPT");
      if ( value >= 0 ) CDI_Recopt = (int) value;

      value = cdiGetenvInt("CDI_REGULARGRID");
      if ( value >= 0 ) cdiDataUnreduced = (int) value;

      value = cdiGetenvInt("CDI_SORTNAME");
      if ( value >= 0 ) cdiSortName = (int) value;

      value = cdiGetenvInt("CDI_HAVE_MISSVAL");
      if ( value >= 0 ) cdiHaveMissval = (int) value;

      value = cdiGetenvInt("CDI_LEVELTYPE");
      if ( value >= 0 ) cdiDefaultLeveltype = (int) value;

      value = cdiGetenvInt("CDI_NETCDF_HDR_PAD");
      if ( value >= 0 ) CDI_netcdf_hdr_pad = (size_t) value;

      envstr = getenv("CDI_MISSVAL");
      if ( envstr ) cdiDefaultMissval = atof(envstr);




      envstr = getenv("NC_CHUNKSIZEHINT");
      if ( envstr ) cdiNcChunksizehint = atoi(envstr);

      envstr = getenv("CDI_CHUNK_ALGO");
      if ( envstr ) cdiSetChunk(envstr);

      envstr = getenv("SPLIT_LTYPE_105");
      if ( envstr ) cdiSplitLtype105 = atoi(envstr);

      envstr = getenv("IGNORE_ATT_COORDINATES");
      if ( envstr ) cdiIgnoreAttCoordinates = atoi(envstr);

      envstr = getenv("IGNORE_VALID_RANGE");
      if ( envstr ) cdiIgnoreValidRange = atoi(envstr);

      envstr = getenv("CDI_SKIP_RECORDS");
      if ( envstr )
        {
          cdiSkipRecords = atoi(envstr);
          cdiSkipRecords = cdiSkipRecords > 0 ? cdiSkipRecords : 0;
        }

      envstr = getenv("CDI_CONVENTION");
      if ( envstr )
        {
          if ( strcmp(envstr, "CF") == 0 || strcmp(envstr, "cf") == 0 )
            {
              cdiConvention = CDI_CONVENTION_CF;
              if ( CDI_Debug )
                Message("CDI convention was set to CF!");
            }
        }

      envstr = getenv("CDI_INVENTORY_MODE");
      if ( envstr )
        {
          if ( strncmp(envstr, "time", 4) == 0 )
            {
              cdiInventoryMode = 2;
              if ( CDI_Debug )
                Message("Inventory mode was set to timestep!");
            }
        }

      envstr = getenv("CDI_VERSION_INFO");
      if ( envstr )
        {
          int ival = atoi(envstr);
          if ( ival == 0 || ival == 1 )
            {
              CDI_Version_Info = ival;
              if ( CDI_Debug )
                Message("CDI_Version_Info = %s", envstr);
            }
        }


      envstr = getenv("CDI_CALENDAR");
      if ( envstr )
        {
          if ( strncmp(envstr, "standard", 8) == 0 )
            cdiDefaultCalendar = CALENDAR_STANDARD;
          else if ( strncmp(envstr, "proleptic", 9) == 0 )
            cdiDefaultCalendar = CALENDAR_PROLEPTIC;
          else if ( strncmp(envstr, "360days", 7) == 0 )
            cdiDefaultCalendar = CALENDAR_360DAYS;
          else if ( strncmp(envstr, "365days", 7) == 0 )
            cdiDefaultCalendar = CALENDAR_365DAYS;
          else if ( strncmp(envstr, "366days", 7) == 0 )
            cdiDefaultCalendar = CALENDAR_366DAYS;
          else if ( strncmp(envstr, "none", 4) == 0 )
            cdiDefaultCalendar = CALENDAR_NONE;

          if ( CDI_Debug )
            Message("Default calendar set to %s!", envstr);
        }
#if defined (HAVE_LIBCGRIBEX)
      gribSetCalendar(cdiDefaultCalendar);
#endif

      envstr = getenv("PARTAB_INTERN");
      if ( envstr ) cdiPartabIntern = atoi(envstr);

      envstr = getenv("PARTAB_PATH");
      if ( envstr ) cdiPartabPath = strdup(envstr);
    }
}


const char *strfiletype(int filetype)
{
  const char *name;
  int size = (int) (sizeof(Filetypes)/sizeof(char *));

  if ( filetype > 0 && filetype < size )
    name = Filetypes[filetype];
  else
    name = Filetypes[0];

  return (name);
}


void cdiDefGlobal(const char *string, int val)
{
  if ( strcmp(string, "REGULARGRID") == 0 ) cdiDataUnreduced = val;
  else if ( strcmp(string, "GRIBAPI_DEBUG") == 0 ) cdiGribApiDebug = val;
  else if ( strcmp(string, "SORTNAME") == 0 ) cdiSortName = val;
  else if ( strcmp(string, "HAVE_MISSVAL") == 0 ) cdiHaveMissval = val;
  else if ( strcmp(string, "NC_CHUNKSIZEHINT") == 0 ) cdiNcChunksizehint = val;
  else if ( strcmp(string, "CMOR_MODE") == 0 ) CDI_cmor_mode = val;
  else if ( strcmp(string, "NETCDF_HDR_PAD") == 0 ) CDI_netcdf_hdr_pad = (size_t) val;
  else Warning("Unsupported global key: %s", string);
}


void cdiDefMissval(double missval)
{
  cdiInitialize();

  cdiDefaultMissval = missval;
}


double cdiInqMissval(void)
{
  cdiInitialize();

  return (cdiDefaultMissval);
}
#if defined (HAVE_CONFIG_H)
#endif

#include <stdio.h>
#include <stdlib.h>


void cdiDecodeParam(int param, int *pnum, int *pcat, int *pdis)
{
  unsigned uparam = (unsigned)param;
  unsigned upnum;

  *pdis = 0xff & uparam;
  *pcat = 0xff & uparam >> 8;
  upnum = 0xffff & uparam >> 16;
  if ( upnum > 0x7fffU ) upnum = 0x8000U - upnum;
  *pnum = (int)upnum;
}


int cdiEncodeParam(int pnum, int pcat, int pdis)
{
  unsigned uparam, upnum;

  if ( pcat < 0 || pcat > 255 ) pcat = 255;
  if ( pdis < 0 || pdis > 255 ) pdis = 255;

  upnum = (unsigned)pnum;
  if ( pnum < 0 ) upnum = (unsigned)(0x8000 - pnum);

  uparam = upnum << 16 | (unsigned)(pcat << 8) | (unsigned)pdis;

  return ((int)uparam);
}


void cdiDecodeDate(int date, int *year, int *month, int *day)
{

  int iyear = date / 10000;
  *year = iyear;
  int idate = abs(date - iyear * 10000),
    imonth = idate / 100;
  *month = imonth;
  *day = idate - imonth * 100;
}


int cdiEncodeDate(int year, int month, int day)
{
  int iyear = abs(year),
    date = iyear * 10000 + month * 100 + day;
  if ( year < 0 ) date = -date;
  return (date);
}


void cdiDecodeTime(int time, int *hour, int *minute, int *second)
{
  int ihour = time / 10000,
    itime = time - ihour * 10000,
    iminute = itime / 100;
  *hour = ihour;
  *minute = iminute;
  *second = itime - iminute * 100;
}


int cdiEncodeTime(int hour, int minute, int second)
{
  int time = hour*10000 + minute*100 + second;

  return time;
}


void cdiParamToString(int param, char *paramstr, int maxlen)
{
  int dis, cat, num;
  int len;

  cdiDecodeParam(param, &num, &cat, &dis);

  size_t umaxlen = maxlen >= 0 ? (unsigned)maxlen : 0U;
  if ( dis == 255 && (cat == 255 || cat == 0 ) )
    len = snprintf(paramstr, umaxlen, "%d", num);
  else if ( dis == 255 )
    len = snprintf(paramstr, umaxlen, "%d.%d", num, cat);
  else
    len = snprintf(paramstr, umaxlen, "%d.%d.%d", num, cat, dis);

  if ( len >= maxlen || len < 0)
    fprintf(stderr, "Internal problem (%s): size of input string is too small!\n", __func__);
}


const char *cdiUnitNamePtr(int cdi_unit)
{
  const char *cdiUnits[] = {
              "undefined",
              "Pa",
              "hPa",
              "mm",
              "cm",
              "dm",
              "m",
  };
  enum { numUnits = (int) (sizeof(cdiUnits)/sizeof(char *)) };
  const char *name = ( cdi_unit > 0 && cdi_unit < numUnits ) ?
    cdiUnits[cdi_unit] : NULL;
  return name;
}
#ifdef HAVE_CONFIG_H
#endif

#include <inttypes.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef WORDS_BIGENDIAN
#include <limits.h>
#endif


static const uint32_t crctab[] = {
  0x00000000,
  0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
  0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6,
  0x2b4bcb61, 0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
  0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9, 0x5f15adac,
  0x5bd4b01b, 0x569796c2, 0x52568b75, 0x6a1936c8, 0x6ed82b7f,
  0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3, 0x709f7b7a,
  0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
  0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58,
  0xbaea46ef, 0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033,
  0xa4ad16ea, 0xa06c0b5d, 0xd4326d90, 0xd0f37027, 0xddb056fe,
  0xd9714b49, 0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
  0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 0xe13ef6f4,
  0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
  0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5,
  0x2ac12072, 0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
  0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca, 0x7897ab07,
  0x7c56b6b0, 0x71159069, 0x75d48dde, 0x6b93dddb, 0x6f52c06c,
  0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1,
  0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
  0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b,
  0xbb60adfc, 0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698,
  0x832f1041, 0x87ee0df6, 0x99a95df3, 0x9d684044, 0x902b669d,
  0x94ea7b2a, 0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
  0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2, 0xc6bcf05f,
  0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
  0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80,
  0x644fc637, 0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
  0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f, 0x5c007b8a,
  0x58c1663d, 0x558240e4, 0x51435d53, 0x251d3b9e, 0x21dc2629,
  0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5, 0x3f9b762c,
  0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
  0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e,
  0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65,
  0xeba91bbc, 0xef68060b, 0xd727bbb6, 0xd3e6a601, 0xdea580d8,
  0xda649d6f, 0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
  0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7, 0xae3afba2,
  0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
  0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74,
  0x857130c3, 0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
  0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c, 0x7b827d21,
  0x7f436096, 0x7200464f, 0x76c15bf8, 0x68860bfd, 0x6c47164a,
  0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e, 0x18197087,
  0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
  0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d,
  0x2056cd3a, 0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce,
  0xcc2b1d17, 0xc8ea00a0, 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb,
  0xdbee767c, 0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
  0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4, 0x89b8fd09,
  0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
  0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf,
  0xa2f33668, 0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};


uint32_t
memcrc(const unsigned char *b, size_t n)
{






  uint32_t s = 0;

  memcrc_r(&s, b, n);


  while (n != 0) {
    register uint32_t c = n & 0377;
    n >>= 8;
    s = (s << 8) ^ crctab[(s >> 24) ^ c];
  }


  return ~s;
}

void
memcrc_r(uint32_t *state, const unsigned char *block, size_t block_len)
{






  register uint32_t c, s = *state;
  register size_t n = block_len;
  register const unsigned char *b = block;

  for (; n > 0; --n) {
    c = (uint32_t)(*b++);
    s = (s << 8) ^ crctab[(s >> 24) ^ c];
  }

  *state = s;
}

#ifdef WORDS_BIGENDIAN
#define SWAP_CSUM(BITWIDTH,BYTEWIDTH,NACC) \
  do { \
    register const uint##BITWIDTH##_t *b = (uint##BITWIDTH##_t *)elems; \
    for (size_t i = 0; i < num_elems; ++i) { \
      for(size_t aofs = NACC; aofs > 0; --aofs) { \
        uint##BITWIDTH##_t accum = b[i + aofs - 1]; \
        for (size_t j = 0; j < BYTEWIDTH; ++j) { \
          uint32_t c = (uint32_t)(accum & UCHAR_MAX); \
          s = (s << 8) ^ crctab[(s >> 24) ^ c]; \
          accum >>= 8; \
        } \
      } \
    } \
  } while (0)
#endif
void
memcrc_r_eswap(uint32_t *state, const unsigned char *elems, size_t num_elems,
               size_t elem_size)
{
#ifdef WORDS_BIGENDIAN
  register uint32_t s = *state;

  switch (elem_size)
  {
  case 1:
    memcrc_r(state, elems, num_elems * elem_size);
    return;
  case 2:
    SWAP_CSUM(16,2,1);
    break;
  case 4:
    SWAP_CSUM(32,4,1);
    break;
  case 8:
    SWAP_CSUM(64,8,1);
    break;
  case 16:
    SWAP_CSUM(64,8,2);
    break;
  }
  *state = s;
#else
  memcrc_r(state, elems, num_elems * elem_size);
#endif
}


uint32_t
memcrc_finish(uint32_t *state, off_t total_size)
{
  register uint32_t c, s = *state;
  register uint64_t n = (uint64_t)total_size;


  while (n != 0) {
    c = n & 0377;
    n >>= 8;
    s = (s << 8) ^ crctab[(s >> 24) ^ c];
  }

  return ~s;
}
#if defined(HAVE_CONFIG_H)
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>

#if !defined(HAVE_CONFIG_H) && !defined(HAVE_MALLOC_H) && defined(SX)
#define HAVE_MALLOC_H
#endif

#if defined(HAVE_MALLOC_H)
# include <malloc.h>
#endif


enum {MALLOC_FUNC=0, CALLOC_FUNC, REALLOC_FUNC, FREE_FUNC};
static const char *memfunc[] = {"Malloc", "Calloc", "Realloc", "Free"};

#undef MEM_UNDEFID
#define MEM_UNDEFID -1

#define MEM_MAXNAME 32

static int dmemory_ExitOnError = 1;

typedef struct
{
  void *ptr;
  size_t size;
  size_t nobj;
  int item;
  int mtype;
  int line;
  char filename[MEM_MAXNAME];
  char functionname[MEM_MAXNAME];
}
MemTable_t;

static MemTable_t *memTable;
static size_t memTableSize = 0;
static long memAccess = 0;

static size_t MemObjs = 0;
static size_t MaxMemObjs = 0;
static size_t MemUsed = 0;
static size_t MaxMemUsed = 0;

static int MEM_Debug = 0;
static int MEM_Info = 0;

static
const char *get_filename(const char *file)
{
  const char *fnptr = strrchr(file, '/');
  if ( fnptr ) fnptr++;
  else fnptr = (char *) file;

  return fnptr;
}


void memDebug(int debug)
{
  MEM_Debug = debug;
}


#if ! defined __GNUC__ && ! defined __attribute__
#define __attribute__(x)
#endif

static
void memInternalProblem(const char *caller, const char *fmt, ...)
  __attribute__((noreturn));
static
void memError(const char *caller, const char *file, int line, size_t size)
  __attribute__((noreturn));

static
void memInternalProblem(const char *functionname, const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);

  printf("\n");
   fprintf(stderr, "Internal problem (%s) : ", functionname);
  vfprintf(stderr, fmt, args);
   fprintf(stderr, "\n");

  va_end(args);

  exit(EXIT_FAILURE);
}

static
void memError(const char *functionname, const char *file, int line, size_t size)
{
  fputs("\n", stdout);
  fprintf(stderr, "Error (%s) : Allocation of %zu bytes failed. [ line %d file %s ]\n",
          functionname, size, line, get_filename(file));

  if ( errno ) perror("System error message ");

  exit(EXIT_FAILURE);
}

static
void memListPrintEntry(int mtype, int item, size_t size, void *ptr,
                       const char *functionname, const char *file, int line)
{
  fprintf(stderr, "[%-7s ", memfunc[mtype]);

  fprintf(stderr, "memory item %3d ", item);
  fprintf(stderr, "(%6zu byte) ", size);
  fprintf(stderr, "at %p", ptr);
  if ( file != NULL )
    {
      fprintf(stderr, " line %4d", line);
      fprintf(stderr, " file %s", get_filename(file));
    }
  if ( functionname != NULL )
    fprintf(stderr, " (%s)", functionname);
  fprintf(stderr, "]\n");
}

static
void memListPrintTable(void)
{
  if ( MemObjs ) fprintf(stderr, "\nMemory table:\n");

  for ( size_t memID = 0; memID < memTableSize; memID++ )
    {
      if ( memTable[memID].item != MEM_UNDEFID )
        memListPrintEntry(memTable[memID].mtype, memTable[memID].item,
                          memTable[memID].size*memTable[memID].nobj,
                          memTable[memID].ptr, memTable[memID].functionname,
                          memTable[memID].filename, memTable[memID].line);
    }

  if ( MemObjs )
    {
      fprintf(stderr, "  Memory access             : %6u\n", (unsigned) memAccess);
      fprintf(stderr, "  Maximum objects           : %6zu\n", memTableSize);
      fprintf(stderr, "  Objects used              : %6u\n", (unsigned) MaxMemObjs);
      fprintf(stderr, "  Objects in use            : %6u\n", (unsigned) MemObjs);
      fprintf(stderr, "  Memory allocated          : ");
      if (MemUsed > 1024*1024*1024)
        fprintf(stderr, " %5d GB\n", (int) (MemUsed/(1024*1024*1024)));
      else if (MemUsed > 1024*1024)
        fprintf(stderr, " %5d MB\n", (int) (MemUsed/(1024*1024)));
      else if (MemUsed > 1024)
        fprintf(stderr, " %5d KB\n", (int) (MemUsed/(1024)));
      else
        fprintf(stderr, " %5d Byte\n", (int) MemUsed);
    }

  if ( MaxMemUsed )
    {
      fprintf(stderr, "  Maximum memory allocated  : ");
      if (MaxMemUsed > 1024*1024*1024)
        fprintf(stderr, " %5d GB\n", (int) (MaxMemUsed/(1024*1024*1024)));
      else if (MaxMemUsed > 1024*1024)
        fprintf(stderr, " %5d MB\n", (int) (MaxMemUsed/(1024*1024)));
      else if (MaxMemUsed > 1024)
        fprintf(stderr, " %5d KB\n", (int) (MaxMemUsed/(1024)));
      else
        fprintf(stderr, " %5d Byte\n", (int) MaxMemUsed);
    }
}

static
void memGetDebugLevel(void)
{
  const char *envstr;

  envstr = getenv("MEMORY_INFO");
  if ( envstr && isdigit((int) envstr[0]) ) MEM_Info = atoi(envstr);

  envstr = getenv("MEMORY_DEBUG");
  if ( envstr && isdigit((int) envstr[0]) ) MEM_Debug = atoi(envstr);

  if ( MEM_Debug && !MEM_Info ) MEM_Info = 1;

  if ( MEM_Info ) atexit(memListPrintTable);
}

static
void memInit(void)
{
  static int initDebugLevel = 0;

  if ( ! initDebugLevel )
    {
      memGetDebugLevel();
      initDebugLevel = 1;
    }
}

static
int memListDeleteEntry(void *ptr, size_t *size)
{
  int item = MEM_UNDEFID;
  size_t memID;

  for ( memID = 0; memID < memTableSize; memID++ )
    {
      if ( memTable[memID].item == MEM_UNDEFID ) continue;
      if ( memTable[memID].ptr == ptr ) break;
    }

  if ( memID != memTableSize )
    {
      MemObjs--;
      MemUsed -= memTable[memID].size * memTable[memID].nobj;
      *size = memTable[memID].size * memTable[memID].nobj;
      item = memTable[memID].item;
      memTable[memID].item = MEM_UNDEFID;
    }

  return item;
}

static
void memTableInitEntry(size_t memID)
{
  if ( memID >= memTableSize )
    memInternalProblem(__func__, "memID %d undefined!", memID);

  memTable[memID].ptr = NULL;
  memTable[memID].item = MEM_UNDEFID;
  memTable[memID].size = 0;
  memTable[memID].nobj = 0;
  memTable[memID].mtype = MEM_UNDEFID;
  memTable[memID].line = MEM_UNDEFID;
}

static
int memListNewEntry(int mtype, void *ptr, size_t size, size_t nobj,
                    const char *functionname, const char *file, int line)
{
  static int item = 0;
  size_t memSize = 0;
  size_t memID = 0;





  if ( memTableSize == 0 )
    {
      memTableSize = 8;
      memSize = memTableSize * sizeof(MemTable_t);
      memTable = (MemTable_t *) malloc(memSize);
      if( memTable == NULL ) memError(__func__, __FILE__, __LINE__, memSize);

      for ( size_t i = 0; i < memTableSize; i++ )
        memTableInitEntry(i);
    }
  else
    {
      while ( memID < memTableSize )
        {
          if ( memTable[memID].item == MEM_UNDEFID ) break;
          memID++;
        }
    }



  if ( memID == memTableSize )
    {
      memTableSize = 2*memTableSize;
      memSize = memTableSize*sizeof(MemTable_t);
      memTable = (MemTable_t*) realloc(memTable, memSize);
      if ( memTable == NULL ) memError(__func__, __FILE__, __LINE__, memSize);

      for ( size_t i = memID; i < memTableSize; i++ )
        memTableInitEntry(i);
    }

  memTable[memID].item = item;
  memTable[memID].ptr = ptr;
  memTable[memID].size = size;
  memTable[memID].nobj = nobj;
  memTable[memID].mtype = mtype;
  memTable[memID].line = line;

  if ( file )
    {
      const char *filename = get_filename(file);
      size_t len = strlen(filename);
      if ( len > MEM_MAXNAME-1 ) len = MEM_MAXNAME-1;

      (void) memcpy(memTable[memID].filename, filename, len);
      memTable[memID].filename[len] = '\0';
    }
  else
    {
      (void) strcpy(memTable[memID].filename, "unknown");
    }

  if ( functionname )
    {
      size_t len = strlen(functionname);
      if ( len > MEM_MAXNAME-1 ) len = MEM_MAXNAME-1;

      (void) memcpy(memTable[memID].functionname, functionname, len);
      memTable[memID].functionname[len] = '\0';
    }
  else
    {
      (void) strcpy(memTable[memID].functionname, "unknown");
    }

  MaxMemObjs++;
  MemObjs++;
  MemUsed += size*nobj;
  if ( MemUsed > MaxMemUsed ) MaxMemUsed = MemUsed;

  return item++;
}

static
int memListChangeEntry(void *ptrold, void *ptr, size_t size,
                       const char *functionname, const char *file, int line)
{
  int item = MEM_UNDEFID;
  size_t memID = 0;

  while( memID < memTableSize )
    {
      if ( memTable[memID].item != MEM_UNDEFID )
        if ( memTable[memID].ptr == ptrold ) break;
      memID++;
    }

  if ( memID == memTableSize )
    {
      if ( ptrold != NULL )
        memInternalProblem(__func__, "Item at %p not found.", ptrold);
    }
  else
    {
      item = memTable[memID].item;

      size_t sizeold = memTable[memID].size*memTable[memID].nobj;

      memTable[memID].ptr = ptr;
      memTable[memID].size = size;
      memTable[memID].nobj = 1;
      memTable[memID].mtype = REALLOC_FUNC;
      memTable[memID].line = line;

      if ( file )
        {
          const char *filename = get_filename(file);
          size_t len = strlen(filename);
          if ( len > MEM_MAXNAME-1 ) len = MEM_MAXNAME-1;

          (void) memcpy(memTable[memID].filename, filename, len);
          memTable[memID].filename[len] = '\0';
        }
      else
        {
          (void) strcpy(memTable[memID].filename, "unknown");
        }

      if ( functionname )
        {
          size_t len = strlen(functionname);
          if ( len > MEM_MAXNAME-1 ) len = MEM_MAXNAME-1;

          (void) memcpy(memTable[memID].functionname, functionname, len);
          memTable[memID].functionname[len] = '\0';
        }
      else
        {
          (void) strcpy(memTable[memID].functionname, "unknown");
        }

      MemUsed -= sizeold;
      MemUsed += size;
      if ( MemUsed > MaxMemUsed ) MaxMemUsed = MemUsed;
    }

  return item;
}


void *memCalloc(size_t nobjs, size_t size, const char *file, const char *functionname, int line)
{
  void *ptr = NULL;

  memInit();

  if ( nobjs*size > 0 )
    {
      ptr = calloc(nobjs, size);

      if ( MEM_Info )
        {
          memAccess++;

          int item = MEM_UNDEFID;
          if ( ptr ) item = memListNewEntry(CALLOC_FUNC, ptr, size, nobjs, functionname, file, line);

          if ( MEM_Debug ) memListPrintEntry(CALLOC_FUNC, item, size*nobjs, ptr, functionname, file, line);
        }

      if ( ptr == NULL && dmemory_ExitOnError )
        memError(functionname, file, line, size*nobjs);
    }
  else
    fprintf(stderr, "Warning (%s) : Allocation of 0 bytes! [ line %d file %s ]\n", functionname, line, file);

  return ptr;
}


void *memMalloc(size_t size, const char *file, const char *functionname, int line)
{
  void *ptr = NULL;

  memInit();

  if ( size > 0 )
    {
      ptr = malloc(size);

      if ( MEM_Info )
        {
          memAccess++;

          int item = MEM_UNDEFID;
          if ( ptr ) item = memListNewEntry(MALLOC_FUNC, ptr, size, 1, functionname, file, line);

          if ( MEM_Debug ) memListPrintEntry(MALLOC_FUNC, item, size, ptr, functionname, file, line);
        }

      if ( ptr == NULL && dmemory_ExitOnError )
        memError(functionname, file, line, size);
    }
  else
    fprintf(stderr, "Warning (%s) : Allocation of 0 bytes! [ line %d file %s ]\n", functionname, line, file);

  return ptr;
}


void *memRealloc(void *ptrold, size_t size, const char *file, const char *functionname, int line)
{
  void *ptr = NULL;

  memInit();

  if ( size > 0 )
    {
      ptr = realloc(ptrold, size);

      if ( MEM_Info )
        {
          memAccess++;

          int item = MEM_UNDEFID;
          if ( ptr )
            {
              item = memListChangeEntry(ptrold, ptr, size, functionname, file, line);

              if ( item == MEM_UNDEFID ) item = memListNewEntry(REALLOC_FUNC, ptr, size, 1, functionname, file, line);
            }

          if ( MEM_Debug ) memListPrintEntry(REALLOC_FUNC, item, size, ptr, functionname, file, line);
        }

      if ( ptr == NULL && dmemory_ExitOnError )
        memError(functionname, file, line, size);
    }
  else
    fprintf(stderr, "Warning (%s) : Allocation of 0 bytes! [ line %d file %s ]\n", functionname, line, get_filename(file));

  return ptr;
}


void memFree(void *ptr, const char *file, const char *functionname, int line)
{
  memInit();

  if ( MEM_Info )
    {
      int item;
      size_t size;

      if ( (item = memListDeleteEntry(ptr, &size)) >= 0 )
        {
          if ( MEM_Debug ) memListPrintEntry(FREE_FUNC, item, size, ptr, functionname, file, line);
        }
      else
        {
          if ( ptr && MEM_Debug )
            fprintf(stderr, "%s info: memory entry at %p not found. [line %4d file %s (%s)]\n",
                    __func__, ptr, line, get_filename(file), functionname);
        }
    }

  free(ptr);
}


size_t memTotal(void)
{
  size_t memtotal = 0;
#if defined (HAVE_MALLINFO)
  struct mallinfo meminfo = mallinfo();
  if ( MEM_Debug )
    {
      fprintf(stderr, "arena      %8zu (non-mmapped space allocated from system)\n", (size_t)meminfo.arena);
      fprintf(stderr, "ordblks    %8zu (number of free chunks)\n", (size_t)meminfo.ordblks);
      fprintf(stderr, "smblks     %8zu (number of fastbin blocks)\n", (size_t) meminfo.smblks);
      fprintf(stderr, "hblks      %8zu (number of mmapped regions)\n", (size_t) meminfo.hblks);
      fprintf(stderr, "hblkhd     %8zu (space in mmapped regions)\n", (size_t) meminfo.hblkhd);
      fprintf(stderr, "usmblks    %8zu (maximum total allocated space)\n", (size_t) meminfo.usmblks);
      fprintf(stderr, "fsmblks    %8zu (maximum total allocated space)\n", (size_t) meminfo.fsmblks);
      fprintf(stderr, "uordblks   %8zu (total allocated space)\n", (size_t) meminfo.uordblks);
      fprintf(stderr, "fordblks   %8zu (total free space)\n", (size_t) meminfo.fordblks);
      fprintf(stderr, "Memory in use:   %8zu bytes\n", (size_t) meminfo.usmblks + (size_t)meminfo.uordblks);
      fprintf(stderr, "Total heap size: %8zu bytes\n", (size_t) meminfo.arena);


    }
  memtotal = (size_t)meminfo.arena;
#endif

  return memtotal;
}


void memExitOnError(void)
{
  dmemory_ExitOnError = 1;
}
#if defined (HAVE_CONFIG_H)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#if !defined (NAMESPACE_H)
#endif

int _ExitOnError = 1;
int _Verbose = 1;
int _Debug = 0;


#if ! defined __GNUC__ && ! defined __attribute__
#define __attribute__(x)
#endif

void SysError_(const char *caller, const char *fmt, ...)
  __attribute__((noreturn));

void SysError_(const char *caller, const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);

  printf("\n");
   fprintf(stderr, "Error (%s) : ", caller);
  vfprintf(stderr, fmt, args);
   fprintf(stderr, "\n");

  va_end(args);

  if ( errno )
    perror("System error message ");

  exit(EXIT_FAILURE);
}


void Error_(const char *caller, const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);

  printf("\n");
   fprintf(stderr, "Error (%s) : ", caller);
  vfprintf(stderr, fmt, args);
   fprintf(stderr, "\n");

  va_end(args);

  if ( _ExitOnError ) exit(EXIT_FAILURE);
}

typedef void (*cdiAbortCFunc)(const char * caller, const char * filename,
                              const char *functionname, int line,
                              const char * errorString, va_list ap)
#ifdef __GNUC__
  __attribute__((noreturn))
#endif
;

void cdiAbortC(const char * caller, const char * filename,
               const char *functionname, int line,
               const char * errorString, ... )
{
  va_list ap;
  va_start(ap, errorString);
  cdiAbortCFunc cdiAbortC_p
    = (cdiAbortCFunc)namespaceSwitchGet(NSSWITCH_ABORT).func;
  cdiAbortC_p(caller, filename, functionname, line, errorString, ap);
  va_end(ap);
}

void
cdiAbortC_serial(const char *caller, const char *filename,
                 const char *functionname, int line,
                 const char *errorString, va_list ap)
{
  fprintf(stderr, "ERROR, %s, %s, line %d%s%s\nerrorString: \"",
          functionname, filename, line, caller?", called from ":"",
          caller?caller:"");
  vfprintf(stderr, errorString, ap);
  fputs("\"\n", stderr);
  exit(EXIT_FAILURE);
}

typedef void (*cdiWarningFunc)(const char * caller, const char * fmt,
                               va_list ap);

void Warning_(const char *caller, const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);

  if ( _Verbose )
    {
      cdiWarningFunc cdiWarning_p
        = (cdiWarningFunc)namespaceSwitchGet(NSSWITCH_WARNING).func;
      cdiWarning_p(caller, fmt, args);
    }

  va_end(args);
}

void cdiWarning(const char *caller, const char *fmt, va_list ap)
{
  fprintf(stderr, "Warning (%s) : ", caller);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
}


void Message_(const char *caller, const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);

   fprintf(stdout, "%-18s : ", caller);
  vfprintf(stdout, fmt, args);
   fprintf(stdout, "\n");

  va_end(args);
}
#if defined (HAVE_CONFIG_H)
#endif

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


#if ! defined (O_BINARY)
#define O_BINARY 0
#endif

#ifndef strdupx
#ifndef strdup
char *strdup(const char *s);
#endif
#define strdupx strdup
#endif


#if defined (HAVE_MMAP)
# include <sys/mman.h>
#endif


#if ! defined (FALSE)
#define FALSE 0
#endif

#if ! defined (TRUE)
#define TRUE 1
#endif


#define MAX_FILES 4096

static int _file_max = MAX_FILES;

static void file_initialize(void);

static int _file_init = FALSE;

#if defined (HAVE_LIBPTHREAD)
#include <pthread.h>

static pthread_once_t _file_init_thread = PTHREAD_ONCE_INIT;
static pthread_mutex_t _file_mutex;

#define FILE_LOCK() pthread_mutex_lock(&_file_mutex)
#define FILE_UNLOCK() pthread_mutex_unlock(&_file_mutex)
#define FILE_INIT() \
   if ( _file_init == FALSE ) pthread_once(&_file_init_thread, file_initialize)

#else

#define FILE_LOCK()
#define FILE_UNLOCK()
#define FILE_INIT() \
   if ( _file_init == FALSE ) file_initialize()

#endif


typedef struct
{
  int self;
  int flag;
  int eof;
  int fd;
  FILE *fp;
  char *name;
  off_t size;
  off_t position;
  long access;
  off_t byteTrans;
  size_t blockSize;
  int mode;
  short type;
  short bufferType;
  size_t bufferSize;
  size_t mappedSize;
  char *buffer;
  long bufferNumFill;
  char *bufferPtr;
  off_t bufferPos;
  off_t bufferStart;
  off_t bufferEnd;
  size_t bufferCnt;
  double time_in_sec;
}
bfile_t;


enum F_I_L_E_Flags
  {
    FILE_READ = 01,
    FILE_WRITE = 02,
    FILE_UNBUF = 04,
    FILE_EOF = 010,
    FILE_ERROR = 020
  };


static int FileInfo = FALSE;


#if ! defined (MIN_BUF_SIZE)
#define MIN_BUF_SIZE 131072L
#endif


static size_t FileBufferSizeMin = MIN_BUF_SIZE;
static long FileBufferSizeEnv = -1;
static short FileBufferTypeEnv = 0;

static short FileTypeRead = FILE_TYPE_OPEN;
static short FileTypeWrite = FILE_TYPE_FOPEN;
static int FileFlagWrite = 0;

static int FILE_Debug = 0;


static void file_table_print(void);




#undef LIBVERSION
#define LIBVERSION 1.8.2
#define XSTRING(x) #x
#define STRING(x) XSTRING(x)
static const char file_libvers[] = STRING(LIBVERSION) " of " __DATE__ " " __TIME__;
typedef struct _filePtrToIdx {
  int idx;
  bfile_t *ptr;
  struct _filePtrToIdx *next;
} filePtrToIdx;


static filePtrToIdx *_fileList = NULL;
static filePtrToIdx *_fileAvail = NULL;

static
void file_list_new(void)
{
  assert(_fileList == NULL);

  _fileList = (filePtrToIdx *) Malloc((size_t)_file_max * sizeof (filePtrToIdx));
}

static
void file_list_delete(void)
{
  if ( _fileList )
    {
      Free(_fileList);
      _fileList = NULL;
    }
}

static
void file_init_pointer(void)
{
  int i;

  for ( i = 0; i < _file_max; i++ )
    {
      _fileList[i].next = _fileList + i + 1;
      _fileList[i].idx = i;
      _fileList[i].ptr = 0;
    }

  _fileList[_file_max-1].next = 0;

  _fileAvail = _fileList;
}

static
bfile_t *file_to_pointer(int idx)
{
  bfile_t *fileptr = NULL;

  FILE_INIT();

  if ( idx >= 0 && idx < _file_max )
    {
      FILE_LOCK();

      fileptr = _fileList[idx].ptr;

      FILE_UNLOCK();
    }
  else
    Error("file index %d undefined!", idx);

  return (fileptr);
}


static
int file_from_pointer(bfile_t *ptr)
{
  int idx = -1;
  filePtrToIdx *newptr;

  if ( ptr )
    {
      FILE_LOCK();

      if ( _fileAvail )
        {
          newptr = _fileAvail;
          _fileAvail = _fileAvail->next;
          newptr->next = 0;
          idx = newptr->idx;
          newptr->ptr = ptr;

          if ( FILE_Debug )
            Message("Pointer %p has idx %d from file list", ptr, idx);
        }
      else
        Warning("Too many open files (limit is %d)!", _file_max);

      FILE_UNLOCK();
    }
  else
    Error("Internal problem (pointer %p undefined)", ptr);

  return (idx);
}

static
void file_init_entry(bfile_t *fileptr)
{
  fileptr->self = file_from_pointer(fileptr);

  fileptr->flag = 0;
  fileptr->fd = -1;
  fileptr->fp = NULL;
  fileptr->mode = 0;
  fileptr->size = 0;
  fileptr->name = NULL;
  fileptr->access = 0;
  fileptr->position = 0;
  fileptr->byteTrans = 0;
  fileptr->type = 0;
  fileptr->bufferType = 0;
  fileptr->bufferSize = 0;
  fileptr->mappedSize = 0;
  fileptr->buffer = NULL;
  fileptr->bufferNumFill = 0;
  fileptr->bufferStart = 0;
  fileptr->bufferEnd = -1;
  fileptr->bufferPos = 0;
  fileptr->bufferCnt = 0;
  fileptr->bufferPtr = NULL;
  fileptr->time_in_sec = 0.0;
}

static
bfile_t *file_new_entry(void)
{
  bfile_t *fileptr;

  fileptr = (bfile_t *) Malloc(sizeof(bfile_t));

  if ( fileptr ) file_init_entry(fileptr);

  return (fileptr);
}

static
void file_delete_entry(bfile_t *fileptr)
{
  int idx;

  idx = fileptr->self;

  FILE_LOCK();

  Free(fileptr);

  _fileList[idx].next = _fileAvail;
  _fileList[idx].ptr = 0;
  _fileAvail = &_fileList[idx];

  FILE_UNLOCK();

  if ( FILE_Debug )
    Message("Removed idx %d from file list", idx);
}


const char *fileLibraryVersion(void)
{
  return (file_libvers);
}


static
int pagesize(void)
{
#if defined(_SC_PAGESIZE)
  return ((int) sysconf(_SC_PAGESIZE));
#else
#ifndef POSIXIO_DEFAULT_PAGESIZE
#define POSIXIO_DEFAULT_PAGESIZE 4096
#endif
  return ((int) POSIXIO_DEFAULT_PAGESIZE);
#endif
}

static
double file_time()
{
  double tseconds = 0.0;
  struct timeval mytime;
  gettimeofday(&mytime, NULL);
  tseconds = (double) mytime.tv_sec + (double) mytime.tv_usec*1.0e-6;
  return (tseconds);
}

void fileDebug(int debug)
{
  FILE_Debug = debug;

  if ( FILE_Debug )
    Message("Debug level %d", debug);
}


void *filePtr(int fileID)
{
  bfile_t *fileptr;

  fileptr = file_to_pointer(fileID);

  return (fileptr);
}

static
void file_pointer_info(const char *caller, int fileID)
{
  if ( FILE_Debug )
    {
      fprintf(stdout, "%-18s : ", caller);
      fprintf(stdout, "The fileID %d underlying pointer is not valid!", fileID);
      fprintf(stdout, "\n");
    }
}


int fileSetBufferType(int fileID, int type)
{
  int ret = 0;
  bfile_t *fileptr;

  fileptr = file_to_pointer(fileID);

  if ( fileptr )
    {
      switch (type)
        {
        case FILE_BUFTYPE_STD:
        case FILE_BUFTYPE_MMAP:
          fileptr->bufferType = (short)type;
          break;
        default:
          Error("File type %d not implemented!", type);
        }
    }

#if ! defined (HAVE_MMAP)
  if ( type == FILE_BUFTYPE_MMAP ) ret = 1;
#endif

  return (ret);
}

int fileFlush(int fileID)
{
  bfile_t *fileptr;
  int retval = 0;

  fileptr = file_to_pointer(fileID);

  if ( fileptr ) retval = fflush(fileptr->fp);

  return (retval);
}


void fileClearerr(int fileID)
{
  bfile_t *fileptr;

  fileptr = file_to_pointer(fileID);

  if ( fileptr )
    {
      if ( fileptr->mode != 'r' )
        clearerr(fileptr->fp);
    }
}


int filePtrEOF(void *vfileptr)
{
  bfile_t *fileptr = (bfile_t *) vfileptr;
  int retval = 0;

  if ( fileptr ) retval = (fileptr->flag & FILE_EOF) != 0;

  return (retval);
}


int fileEOF(int fileID)
{
  bfile_t *fileptr;
  int retval = 0;

  fileptr = file_to_pointer(fileID);

  if ( fileptr ) retval = (fileptr->flag & FILE_EOF) != 0;

  return (retval);
}

void fileRewind(int fileID)
{
  fileSetPos(fileID, (off_t) 0, SEEK_SET);
  fileClearerr(fileID);
}


off_t fileGetPos(int fileID)
{
  off_t filepos = 0;
  bfile_t *fileptr;

  fileptr = file_to_pointer(fileID);

  if ( fileptr )
    {
      if ( fileptr->mode == 'r' && fileptr->type == FILE_TYPE_OPEN )
        filepos = fileptr->position;
      else
        filepos = ftell(fileptr->fp);
    }

  if ( FILE_Debug ) Message("Position %ld", filepos);

  return (filepos);
}


int fileSetPos(int fileID, off_t offset, int whence)
{
  int status = 0;
  bfile_t *fileptr;

  fileptr = file_to_pointer(fileID);

  if ( FILE_Debug ) Message("Offset %8ld  Whence %3d", (long) offset, whence);

  if ( fileptr == 0 )
    {
      file_pointer_info(__func__, fileID);
      return (1);
    }

  switch (whence)
    {
    case SEEK_SET:
      if ( fileptr->mode == 'r' && fileptr->type == FILE_TYPE_OPEN )
        {
          off_t position = offset;
          fileptr->position = position;
          if ( position < fileptr->bufferStart || position > fileptr->bufferEnd )
            {
              if ( fileptr->bufferType == FILE_BUFTYPE_STD )
                fileptr->bufferPos = position;
              else
                fileptr->bufferPos = position - position % pagesize();

              fileptr->bufferCnt = 0;
              fileptr->bufferPtr = NULL;
            }
          else
            {
              if ( fileptr->bufferPos != fileptr->bufferEnd + 1 )
                {
                  if ( FILE_Debug )
                    Message("Reset buffer pos from %ld to %ld",
                            fileptr->bufferPos, fileptr->bufferEnd + 1);

                  fileptr->bufferPos = fileptr->bufferEnd + 1;
                }
              fileptr->bufferCnt = (size_t)(fileptr->bufferEnd - position) + 1;
              fileptr->bufferPtr = fileptr->buffer + position - fileptr->bufferStart;
            }
        }
      else
        {
          status = fseek(fileptr->fp, offset, whence);
        }
      break;
    case SEEK_CUR:
      if ( fileptr->mode == 'r' && fileptr->type == FILE_TYPE_OPEN )
        {
          fileptr->position += offset;
          off_t position = fileptr->position;
          if ( position < fileptr->bufferStart || position > fileptr->bufferEnd )
            {
              if ( fileptr->bufferType == FILE_BUFTYPE_STD )
                fileptr->bufferPos = position;
              else
                fileptr->bufferPos = position - position % pagesize();

              fileptr->bufferCnt = 0;
              fileptr->bufferPtr = NULL;
            }
          else
            {
              if ( fileptr->bufferPos != fileptr->bufferEnd + 1 )
                {
                  if ( FILE_Debug )
                    Message("Reset buffer pos from %ld to %ld",
                            fileptr->bufferPos, fileptr->bufferEnd + 1);

                  fileptr->bufferPos = fileptr->bufferEnd + 1;
                }
              fileptr->bufferCnt -= (size_t)offset;
              fileptr->bufferPtr += offset;
            }
        }
      else
        {
          status = fseek(fileptr->fp, offset, whence);
        }
      break;
    default:
      Error("Whence = %d not implemented", whence);
    }

  if ( fileptr->position < fileptr->size )
    if ( (fileptr->flag & FILE_EOF) != 0 )
      fileptr->flag -= FILE_EOF;

  return (status);
}

static
void file_table_print(void)
{
  int fileID;
  int lprintHeader = 1;
  bfile_t *fileptr;

  for ( fileID = 0; fileID < _file_max; fileID++ )
    {
      fileptr = file_to_pointer(fileID);

      if ( fileptr )
        {
          if ( lprintHeader )
            {
              fprintf(stderr, "\nFile table:\n");
              fprintf(stderr, "+-----+---------+");
              fprintf(stderr, "----------------------------------------------------+\n");
              fprintf(stderr, "|  ID |  Mode   |");
              fprintf(stderr, "  Name                                              |\n");
              fprintf(stderr, "+-----+---------+");
              fprintf(stderr, "----------------------------------------------------+\n");
              lprintHeader = 0;
            }

          fprintf(stderr, "| %3d | ", fileID);

          switch ( fileptr->mode )
            {
            case 'r':
              fprintf(stderr, "read   ");
              break;
            case 'w':
              fprintf(stderr, "write  ");
              break;
            case 'a':
              fprintf(stderr, "append ");
              break;
            default:
              fprintf(stderr, "unknown");
            }

          fprintf(stderr, " | %-51s|\n", fileptr->name);
        }
    }

  if ( lprintHeader == 0 )
    {
      fprintf(stderr, "+-----+---------+");
      fprintf(stderr, "----------------------------------------------------+\n");
    }
}


char *fileInqName(int fileID)
{
  bfile_t *fileptr;
  char *name = NULL;

  fileptr = file_to_pointer(fileID);

  if ( fileptr ) name = fileptr->name;

  return (name);
}


int fileInqMode(int fileID)
{
  bfile_t *fileptr;
  int mode = 0;

  fileptr = file_to_pointer(fileID);

  if ( fileptr ) mode = fileptr->mode;

  return (mode);
}

static
long file_getenv(const char *envName)
{
  char *envString;
  long envValue = -1;
  long fact = 1;

  envString = getenv(envName);

  if ( envString )
    {
      int loop;

      for ( loop = 0; loop < (int) strlen(envString); loop++ )
        {
          if ( ! isdigit((int) envString[loop]) )
            {
              switch ( tolower((int) envString[loop]) )
                {
                case 'k': fact = 1024; break;
                case 'm': fact = 1048576; break;
                case 'g': fact = 1073741824; break;
                default:
                  fact = 0;
                  Message("Invalid number string in %s: %s", envName, envString);
                  Warning("%s must comprise only digits [0-9].",envName);
                }
              break;
            }
        }

      if ( fact ) envValue = fact*atol(envString);

      if ( FILE_Debug ) Message("Set %s to %ld", envName, envValue);
    }

  return (envValue);
}

static
void file_initialize(void)
{
  long value;
  char *envString;

#if defined (HAVE_LIBPTHREAD)

  pthread_mutex_init(&_file_mutex, NULL);
#endif

  value = file_getenv("FILE_DEBUG");
  if ( value >= 0 ) FILE_Debug = (int) value;

  value = file_getenv("FILE_MAX");
  if ( value >= 0 ) _file_max = (int) value;

  if ( FILE_Debug )
    Message("FILE_MAX = %d", _file_max);

  FileInfo = (int) file_getenv("FILE_INFO");

  value = file_getenv("FILE_BUFSIZE");
  if ( value >= 0 ) FileBufferSizeEnv = value;
  else
    {
      value = file_getenv("GRIB_API_IO_BUFFER_SIZE");
      if ( value >= 0 ) FileBufferSizeEnv = value;
    }

  value = file_getenv("FILE_TYPE_READ");
  if ( value > 0 )
    {
      switch (value)
        {
        case FILE_TYPE_OPEN:
        case FILE_TYPE_FOPEN:
          FileTypeRead = (short)value;
          break;
        default:
          Warning("File type %d not implemented!", value);
        }
    }

  value = file_getenv("FILE_TYPE_WRITE");
  if ( value > 0 )
    {
      switch (value)
        {
        case FILE_TYPE_OPEN:
        case FILE_TYPE_FOPEN:
          FileTypeWrite = (short)value;
          break;
        default:
          Warning("File type %d not implemented!", value);
        }
    }

#if defined (O_NONBLOCK)
  FileFlagWrite = O_NONBLOCK;
#endif
  envString = getenv("FILE_FLAG_WRITE");
  if ( envString )
    {
#if defined (O_NONBLOCK)
      if ( strcmp(envString, "NONBLOCK") == 0 ) FileFlagWrite = O_NONBLOCK;
#endif
    }

  value = file_getenv("FILE_BUFTYPE");
#if ! defined (HAVE_MMAP)
  if ( value == FILE_BUFTYPE_MMAP )
    {
      Warning("MMAP not available!");
      value = 0;
    }
#endif
  if ( value > 0 )
    {
      switch (value)
        {
        case FILE_BUFTYPE_STD:
        case FILE_BUFTYPE_MMAP:
          FileBufferTypeEnv = (short)value;
          break;
        default:
          Warning("File buffer type %d not implemented!", value);
        }
    }

  file_list_new();
  atexit(file_list_delete);

  FILE_LOCK();

  file_init_pointer();

  FILE_UNLOCK();

  if ( FILE_Debug ) atexit(file_table_print);

  _file_init = TRUE;
}

static
void file_set_buffer(bfile_t *fileptr)
{
  size_t buffersize = 0;

  if ( fileptr->mode == 'r' )
    {
      if ( FileBufferTypeEnv )
        fileptr->bufferType = FileBufferTypeEnv;
      else if ( fileptr->bufferType == 0 )
        fileptr->bufferType = FILE_BUFTYPE_STD;

      if ( FileBufferSizeEnv >= 0 )
        buffersize = (size_t) FileBufferSizeEnv;
      else if ( fileptr->bufferSize > 0 )
        buffersize = fileptr->bufferSize;
      else
        {
          buffersize = fileptr->blockSize * 4;
          if ( buffersize < FileBufferSizeMin ) buffersize = FileBufferSizeMin;
        }

      if ( (size_t) fileptr->size < buffersize )
        buffersize = (size_t) fileptr->size;

      if ( fileptr->bufferType == FILE_BUFTYPE_MMAP )
        {
          size_t blocksize = (size_t) pagesize();
          size_t minblocksize = 4 * blocksize;
          buffersize = buffersize - buffersize % minblocksize;

          if ( buffersize < (size_t) fileptr->size && buffersize < minblocksize )
            buffersize = minblocksize;
        }

      if ( buffersize == 0 ) buffersize = 1;
    }
  else
    {
      fileptr->bufferType = FILE_BUFTYPE_STD;

      if ( FileBufferSizeEnv >= 0 )
        buffersize = (size_t) FileBufferSizeEnv;
      else if ( fileptr->bufferSize > 0 )
        buffersize = fileptr->bufferSize;
      else
        {
          buffersize = fileptr->blockSize * 4;
          if ( buffersize < FileBufferSizeMin ) buffersize = FileBufferSizeMin;
        }
    }

  if ( fileptr->bufferType == FILE_BUFTYPE_STD || fileptr->type == FILE_TYPE_FOPEN )
    {
      if ( buffersize > 0 )
        {
          fileptr->buffer = (char *) Malloc(buffersize);
          if ( fileptr->buffer == NULL )
            SysError("Allocation of file buffer failed!");
        }
    }

  if ( fileptr->type == FILE_TYPE_FOPEN )
    if ( setvbuf(fileptr->fp, fileptr->buffer, fileptr->buffer ? _IOFBF : _IONBF, buffersize) )
      SysError("setvbuf failed!");

  fileptr->bufferSize = buffersize;
}

static
int file_fill_buffer(bfile_t *fileptr)
{
  ssize_t nread;
  int fd;
  long offset = 0;
  off_t retseek;

  if ( FILE_Debug )
    Message("file ptr = %p  Cnt = %ld", fileptr, fileptr->bufferCnt);

  if ( (fileptr->flag & FILE_EOF) != 0 ) return (EOF);

  if ( fileptr->buffer == NULL ) file_set_buffer(fileptr);

  if ( fileptr->bufferSize == 0 ) return (EOF);

  fd = fileptr->fd;

#if defined (HAVE_MMAP)
  if ( fileptr->bufferType == FILE_BUFTYPE_MMAP )
    {
      if ( fileptr->bufferPos >= fileptr->size )
        {
          nread = 0;
        }
      else
        {
          xassert(fileptr->bufferSize <= SSIZE_MAX);
          nread = (ssize_t)fileptr->bufferSize;
          if ( (nread + fileptr->bufferPos) > fileptr->size )
            nread = fileptr->size - fileptr->bufferPos;

          if ( fileptr->buffer )
            {
              int ret;
              ret = munmap(fileptr->buffer, fileptr->mappedSize);
              if ( ret == -1 ) SysError("munmap error for read %s", fileptr->name);
              fileptr->buffer = NULL;
            }

          fileptr->mappedSize = (size_t)nread;

          fileptr->buffer = (char*) mmap(NULL, (size_t) nread, PROT_READ, MAP_PRIVATE, fd, fileptr->bufferPos);

          if ( fileptr->buffer == MAP_FAILED ) SysError("mmap error for read %s", fileptr->name);

          offset = fileptr->position - fileptr->bufferPos;
        }
    }
  else
#endif
    {
      retseek = lseek(fileptr->fd, fileptr->bufferPos, SEEK_SET);
      if ( retseek == (off_t)-1 )
        SysError("lseek error at pos %ld file %s", (long) fileptr->bufferPos, fileptr->name);

      nread = read(fd, fileptr->buffer, fileptr->bufferSize);
    }

  if ( nread <= 0 )
    {
      if ( nread == 0 )
        fileptr->flag |= FILE_EOF;
      else
        fileptr->flag |= FILE_ERROR;

      fileptr->bufferCnt = 0;
      return (EOF);
    }

  fileptr->bufferPtr = fileptr->buffer;
  fileptr->bufferCnt = (size_t)nread;

  fileptr->bufferStart = fileptr->bufferPos;
  fileptr->bufferPos += nread;
  fileptr->bufferEnd = fileptr->bufferPos - 1;

  if ( FILE_Debug )
    {
      Message("fileID = %d  Val     = %d", fileptr->self, (int) fileptr->buffer[0]);
      Message("fileID = %d  Start   = %ld", fileptr->self, fileptr->bufferStart);
      Message("fileID = %d  End     = %ld", fileptr->self, fileptr->bufferEnd);
      Message("fileID = %d  nread   = %ld", fileptr->self, nread);
      Message("fileID = %d  offset  = %ld", fileptr->self, offset);
      Message("fileID = %d  Pos     = %ld", fileptr->self, fileptr->bufferPos);
      Message("fileID = %d  postion = %ld", fileptr->self, fileptr->position);
    }

  if ( offset > 0 )
    {
      if ( offset > nread )
        Error("Internal problem with buffer handling. nread = %d offset = %d", nread, offset);

      fileptr->bufferPtr += offset;
      fileptr->bufferCnt -= (size_t)offset;
    }

  fileptr->bufferNumFill++;

  return ((unsigned char) *fileptr->bufferPtr);
}

static
void file_copy_from_buffer(bfile_t *fileptr, void *ptr, size_t size)
{
  if ( FILE_Debug )
    Message("size = %ld  Cnt = %ld", size, fileptr->bufferCnt);

  if ( fileptr->bufferCnt < size )
    Error("Buffer too small. bufferCnt = %d", fileptr->bufferCnt);

  if ( size == 1 )
    {
      ((char *)ptr)[0] = fileptr->bufferPtr[0];

      fileptr->bufferPtr++;
      fileptr->bufferCnt--;
    }
  else
    {
      memcpy(ptr, fileptr->bufferPtr, size);

      fileptr->bufferPtr += size;
      fileptr->bufferCnt -= size;
    }
}

static
size_t file_read_from_buffer(bfile_t *fileptr, void *ptr, size_t size)
{
  size_t nread, rsize;
  size_t offset = 0;

  if ( FILE_Debug )
    Message("size = %ld  Cnt = %ld", size, (long) fileptr->bufferCnt);

  if ( ((long)fileptr->bufferCnt) < 0L )
    Error("Internal problem. bufferCnt = %ld", (long) fileptr->bufferCnt);

  rsize = size;

  while ( fileptr->bufferCnt < rsize )
    {
      nread = fileptr->bufferCnt;



      if ( nread > (size_t) 0 )
        file_copy_from_buffer(fileptr, (char *)ptr+offset, nread);
      offset += nread;
      if ( nread < rsize )
        rsize -= nread;
      else
        rsize = 0;

      if ( file_fill_buffer(fileptr) == EOF ) break;
    }

  nread = size - offset;

  if ( fileptr->bufferCnt < nread ) nread = fileptr->bufferCnt;

  if ( nread > (unsigned) 0 )
    file_copy_from_buffer(fileptr, (char *)ptr+offset, nread);

  return (nread+offset);
}


void fileSetBufferSize(int fileID, long buffersize)
{
  bfile_t *fileptr = file_to_pointer(fileID);
  xassert(buffersize >= 0);
  if ( fileptr ) fileptr->bufferSize = (size_t)buffersize;
}




int fileOpen(const char *filename, const char *mode)
{
  int (*myFileOpen)(const char *filename, const char *mode)
    = (int (*)(const char *, const char *))
    namespaceSwitchGet(NSSWITCH_FILE_OPEN).func;
  return myFileOpen(filename, mode);
}

int fileOpen_serial(const char *filename, const char *mode)
{
  FILE *fp = NULL;
  int fd = -1;
  int fileID = FILE_UNDEFID;
  int fmode = 0;
  struct stat filestat;
  bfile_t *fileptr = NULL;

  FILE_INIT();

  fmode = tolower((int) mode[0]);

  switch ( fmode )
    {
    case 'r':
      if ( FileTypeRead == FILE_TYPE_FOPEN )
        fp = fopen(filename, "rb");
      else
        fd = open(filename, O_RDONLY | O_BINARY);
      break;
    case 'x': fp = fopen(filename, "rb"); break;
    case 'w':
      if ( FileTypeWrite == FILE_TYPE_FOPEN )
        fp = fopen(filename, "wb");
      else
        fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY | FileFlagWrite, 0666);
      break;
    case 'a': fp = fopen(filename, "ab"); break;
    default: Error("Mode %c unexpected!", fmode);
    }

  if ( FILE_Debug )
    if ( fp == NULL && fd == -1 )
      Message("Open failed on %s mode %c errno %d", filename, fmode, errno);

  if ( fp )
    {
      if ( stat(filename, &filestat) != 0 ) return (fileID);

      fileptr = file_new_entry();
      if ( fileptr )
        {
          fileID = fileptr->self;
          fileptr->fp = fp;
        }
    }
  else if ( fd >= 0 )
    {
      if ( fstat(fd, &filestat) != 0 ) return (fileID);

      fileptr = file_new_entry();
      if ( fileptr )
        {
          fileID = fileptr->self;
          fileptr->fd = fd;
        }
    }

  if ( fileID >= 0 )
    {
      fileptr->mode = fmode;
      fileptr->name = strdupx(filename);

#if defined (HAVE_STRUCT_STAT_ST_BLKSIZE)
      fileptr->blockSize = (size_t) filestat.st_blksize;
#else
      fileptr->blockSize = (size_t) 4096;
#endif

      if ( fmode == 'r' )
        fileptr->type = FileTypeRead;
      else if ( fmode == 'w' )
        fileptr->type = FileTypeWrite;
      else
        fileptr->type = FILE_TYPE_FOPEN;

      if ( fmode == 'r' ) fileptr->size = filestat.st_size;

      if ( fileptr->type == FILE_TYPE_FOPEN ) file_set_buffer(fileptr);

      if ( FILE_Debug )
        Message("File %s opened with ID %d", filename, fileID);
    }

  return (fileID);
}




int fileClose(int fileID)
{
  int (*myFileClose)(int fileID)
    = (int (*)(int))namespaceSwitchGet(NSSWITCH_FILE_CLOSE).func;
  return myFileClose(fileID);
}

int fileClose_serial(int fileID)
{
  char *name;
  int ret;
  const char *fbtname[] = {"unknown", "standard", "mmap"};
  const char *ftname[] = {"unknown", "open", "fopen"};
  bfile_t *fileptr = file_to_pointer(fileID);
  double rout = 0;

  if ( fileptr == NULL )
    {
      file_pointer_info(__func__, fileID);
      return (1);
    }

  name = fileptr->name;

  if ( FILE_Debug )
    Message("fileID = %d  filename = %s", fileID, name);

  if ( FileInfo > 0 )
    {
      fprintf(stderr, "____________________________________________\n");
      fprintf(stderr, " file ID          : %d\n", fileID);
      fprintf(stderr, " file name        : %s\n", fileptr->name);
      fprintf(stderr, " file type        : %d (%s)\n", fileptr->type, ftname[fileptr->type]);

      if ( fileptr->type == FILE_TYPE_FOPEN )
        fprintf(stderr, " file pointer     : %p\n", (void *) fileptr->fp);
      else
        {
          fprintf(stderr, " file descriptor  : %d\n", fileptr->fd);
          fprintf(stderr, " file flag        : %d\n", FileFlagWrite);
        }
      fprintf(stderr, " file mode        : %c\n", fileptr->mode);

      if ( sizeof(off_t) > sizeof(long) )
        {
#if defined (_WIN32)
          fprintf(stderr, " file size        : %I64d\n", (long long) fileptr->size);
          if ( fileptr->type == FILE_TYPE_OPEN )
            fprintf(stderr, " file position    : %I64d\n", (long long) fileptr->position);
          fprintf(stderr, " bytes transfered : %I64d\n", (long long) fileptr->byteTrans);
#else
          fprintf(stderr, " file size        : %lld\n", (long long) fileptr->size);
          if ( fileptr->type == FILE_TYPE_OPEN )
            fprintf(stderr, " file position    : %lld\n", (long long) fileptr->position);
          fprintf(stderr, " bytes transfered : %lld\n", (long long) fileptr->byteTrans);
#endif
        }
      else
        {
          fprintf(stderr, " file size        : %ld\n", (long) fileptr->size);
          if ( fileptr->type == FILE_TYPE_OPEN )
            fprintf(stderr, " file position    : %ld\n", (long) fileptr->position);
          fprintf(stderr, " bytes transfered : %ld\n", (long) fileptr->byteTrans);
        }

      if ( fileptr->time_in_sec > 0 )
        {
          rout = (double)fileptr->byteTrans / (1024.*1024.*fileptr->time_in_sec);
        }

      fprintf(stderr, " wall time [s]    : %.2f\n", fileptr->time_in_sec);
      fprintf(stderr, " data rate [MB/s] : %.1f\n", rout);

      fprintf(stderr, " file access      : %ld\n", fileptr->access);
      if ( fileptr->mode == 'r' && fileptr->type == FILE_TYPE_OPEN )
        {
          fprintf(stderr, " buffer type      : %d (%s)\n", fileptr->bufferType, fbtname[fileptr->bufferType]);
          fprintf(stderr, " num buffer fill  : %ld\n", fileptr->bufferNumFill);
        }
      fprintf(stderr, " buffer size      : %lu\n", (unsigned long) fileptr->bufferSize);
      fprintf(stderr, " block size       : %lu\n", (unsigned long) fileptr->blockSize);
      fprintf(stderr, " page size        : %d\n", pagesize());
      fprintf(stderr, "--------------------------------------------\n");
    }

  if ( fileptr->type == FILE_TYPE_FOPEN )
    {
      ret = fclose(fileptr->fp);
      if ( ret == EOF )
        SysError("EOF returned for close of %s!", name);
    }
  else
    {
#if defined (HAVE_MMAP)
      if ( fileptr->buffer && fileptr->mappedSize )
        {
          ret = munmap(fileptr->buffer, fileptr->mappedSize);
          if ( ret == -1 ) SysError("munmap error for close %s", fileptr->name);
          fileptr->buffer = NULL;
        }
#endif
      ret = close(fileptr->fd);
      if ( ret == -1 )
        SysError("EOF returned for close of %s!", name);
    }

  if ( fileptr->name ) Free((void*) fileptr->name);
  if ( fileptr->buffer ) Free((void*) fileptr->buffer);

  file_delete_entry(fileptr);

  return (0);
}


int filePtrGetc(void *vfileptr)
{
  int ivalue = EOF;
  int fillret = 0;
  bfile_t *fileptr = (bfile_t *) vfileptr;

  if ( fileptr )
    {
      if ( fileptr->mode == 'r' && fileptr->type == FILE_TYPE_OPEN )
        {
          if ( fileptr->bufferCnt == 0 ) fillret = file_fill_buffer(fileptr);

          if ( fillret >= 0 )
            {
              ivalue = (unsigned char) *fileptr->bufferPtr++;
              fileptr->bufferCnt--;
              fileptr->position++;

              fileptr->byteTrans++;
              fileptr->access++;
            }
        }
      else
        {
          ivalue = fgetc(fileptr->fp);
          if ( ivalue >= 0 )
            {
              fileptr->byteTrans++;
              fileptr->access++;
            }
          else
            fileptr->flag |= FILE_EOF;
        }
    }

  return (ivalue);
}


int fileGetc(int fileID)
{
  int ivalue;
  bfile_t *fileptr;

  fileptr = file_to_pointer(fileID);

  ivalue = filePtrGetc((void *)fileptr);

  return (ivalue);
}


size_t filePtrRead(void *vfileptr, void *restrict ptr, size_t size)
{
  size_t nread = 0;
  bfile_t *fileptr = (bfile_t *) vfileptr;

  if ( fileptr )
    {
      if ( fileptr->mode == 'r' && fileptr->type == FILE_TYPE_OPEN )
        nread = file_read_from_buffer(fileptr, ptr, size);
      else
        {
          nread = fread(ptr, 1, size, fileptr->fp);
          if ( nread != size )
            {
              if ( nread == 0 )
                fileptr->flag |= FILE_EOF;
              else
                fileptr->flag |= FILE_ERROR;
            }
        }

      fileptr->position += (off_t)nread;
      fileptr->byteTrans += (off_t)nread;
      fileptr->access++;
    }

  if ( FILE_Debug ) Message("size %ld  nread %ld", size, nread);

  return (nread);
}


size_t fileRead(int fileID, void *restrict ptr, size_t size)
{
  size_t nread = 0;
  bfile_t *fileptr;

  fileptr = file_to_pointer(fileID);

  if ( fileptr )
    {
      double t_begin = 0.0;

      if ( FileInfo ) t_begin = file_time();

      if ( fileptr->type == FILE_TYPE_OPEN )
        nread = file_read_from_buffer(fileptr, ptr, size);
      else
        {
          nread = fread(ptr, 1, size, fileptr->fp);
          if ( nread != size )
            {
              if ( nread == 0 )
                fileptr->flag |= FILE_EOF;
              else
                fileptr->flag |= FILE_ERROR;
            }
        }

      if ( FileInfo ) fileptr->time_in_sec += file_time() - t_begin;

      fileptr->position += (off_t)nread;
      fileptr->byteTrans += (off_t)nread;
      fileptr->access++;
    }

  if ( FILE_Debug ) Message("size %ld  nread %ld", size, nread);

  return (nread);
}


size_t fileWrite(int fileID, const void *restrict ptr, size_t size)
{
  size_t nwrite = 0;
  bfile_t *fileptr;

  fileptr = file_to_pointer(fileID);

  if ( fileptr )
    {
      double t_begin = 0.0;



      if ( FileInfo ) t_begin = file_time();

      if ( fileptr->type == FILE_TYPE_FOPEN )
        nwrite = fwrite(ptr, 1, size, fileptr->fp);
      else
        {
          ssize_t temp = write(fileptr->fd, ptr, size);
          if (temp == -1)
            {
              perror("error writing to file");
              nwrite = 0;
            }
          else
            nwrite = (size_t)temp;
        }

      if ( FileInfo ) fileptr->time_in_sec += file_time() - t_begin;

      fileptr->position += (off_t)nwrite;
      fileptr->byteTrans += (off_t)nwrite;
      fileptr->access++;
    }

  return (nwrite);
}
#ifndef _GAUSSGRID_H
#define _GAUSSGRID_H

void gaussaw(double *restrict pa, double *restrict pw, size_t nlat);

#endif
#ifdef HAVE_CONFIG_H
#endif

#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>



#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif


static
void cpledn(size_t kn, size_t kodd, double *pfn, double pdx, int kflag,
            double *pw, double *pdxn, double *pxmod)
{
  double zdlk;
  double zdlldn;
  double zdlx;
  double zdlmod;
  double zdlxn;

  size_t ik;



  zdlx = pdx;
  zdlk = 0.0;
  if (kodd == 0)
    {
      zdlk = 0.5*pfn[0];
    }
  zdlxn = 0.0;
  zdlldn = 0.0;

  ik = 1;

  if (kflag == 0)
    {
      for(size_t jn = 2-kodd; jn <= kn; jn += 2)
        {

          zdlk = zdlk + pfn[ik]*cos((double)(jn)*zdlx);

          zdlldn = zdlldn - pfn[ik]*(double)(jn)*sin((double)(jn)*zdlx);
          ik++;
        }

      zdlmod = -(zdlk/zdlldn);
      zdlxn = zdlx + zdlmod;
      *pdxn = zdlxn;
      *pxmod = zdlmod;
    }



  if (kflag == 1)
    {
      for(size_t jn = 2-kodd; jn <= kn; jn += 2)
        {

          zdlldn = zdlldn - pfn[ik]*(double)(jn)*sin((double)(jn)*zdlx);
          ik++;
        }
      *pw = (double)(2*kn+1)/(zdlldn*zdlldn);
    }

  return;
}

static
void gawl(double *pfn, double *pl, double *pw, size_t kn)
{
  double pmod = 0;
  int iflag;
  int itemax;
  double zw = 0;
  double zdlx;
  double zdlxn = 0;



  iflag = 0;
  itemax = 20;

  size_t iodd = (kn % 2);

  zdlx = *pl;



  for (int jter = 1; jter <= itemax+1; jter++)
    {
      cpledn(kn, iodd, pfn, zdlx, iflag, &zw, &zdlxn, &pmod);
      zdlx = zdlxn;
      if (iflag == 1) break;
      if (fabs(pmod) <= DBL_EPSILON*1000.0) iflag = 1;
    }

  *pl = zdlxn;
  *pw = zw;

  return;
}

static
void gauaw(size_t kn, double *__restrict__ pl, double *__restrict__ pw)
{






  double *zfn, *zfnlat;

  double z, zfnn;

  zfn = (double *) Malloc((kn+1) * (kn+1) * sizeof(double));
  zfnlat = (double *) Malloc((kn/2+1+1)*sizeof(double));

  zfn[0] = M_SQRT2;
  for (size_t jn = 1; jn <= kn; jn++)
    {
      zfnn = zfn[0];
      for (size_t jgl = 1; jgl <= jn; jgl++)
        {
          zfnn *= sqrt(1.0-0.25/((double)(jgl*jgl)));
        }

      zfn[jn*(kn+1)+jn] = zfnn;

      size_t iodd = jn % 2;
      for (size_t jgl = 2; jgl <= jn-iodd; jgl += 2)
        {
          zfn[jn*(kn+1)+jn-jgl] = zfn[jn*(kn+1)+jn-jgl+2]
            *((double)((jgl-1)*(2*jn-jgl+2)))/((double)(jgl*(2*jn-jgl+1)));
        }
    }




  size_t iodd = kn % 2;
  size_t ik = iodd;
  for (size_t jgl = iodd; jgl <= kn; jgl += 2)
    {
      zfnlat[ik] = zfn[kn*(kn+1)+jgl];
      ik++;
    }






  size_t ins2 = kn/2+(kn % 2);

  for (size_t jgl = 1; jgl <= ins2; jgl++)
    {
      z = ((double)(4*jgl-1))*M_PI/((double)(4*kn+2));
      pl[jgl-1] = z+1.0/(tan(z)*((double)(8*kn*kn)));
    }



  for (size_t jgl = ins2; jgl >= 1 ; jgl--)
    {
      size_t jglm1 = jgl-1;
      gawl(zfnlat, &(pl[jglm1]), &(pw[jglm1]), kn);
    }



  for (size_t jgl = 0; jgl < ins2; jgl++)
    {
      pl[jgl] = cos(pl[jgl]);
    }

  for (size_t jgl = 1; jgl <= kn/2; jgl++)
    {
      size_t jglm1 = jgl-1;
      size_t isym = kn-jgl;
      pl[isym] = -pl[jglm1];
      pw[isym] = pw[jglm1];
    }

  Free(zfnlat);
  Free(zfn);

  return;
}

#if 0
static
void gauaw_old(double *pa, double *pw, int nlat)
{







  const int itemax = 20;

  int isym, iter, ins2, jn, j;
  double za, zw, zan;
  double z, zk, zkm1, zkm2, zx, zxn, zldn, zmod;






  ins2 = nlat/2 + nlat%2;

  for ( j = 0; j < ins2; j++ )
    {
      z = (double) (4*(j+1)-1)*M_PI / (double) (4*nlat+2);
      pa[j] = cos(z + 1.0/(tan(z)*(double)(8*nlat*nlat)));
    }

  for ( j = 0; j < ins2; j++ )
    {

      za = pa[j];

      iter = 0;
      do
        {
          iter++;
          zk = 0.0;



          zkm2 = 1.0;
          zkm1 = za;
          zx = za;
          for ( jn = 2; jn <= nlat; jn++ )
            {
              zk = ((double) (2*jn-1)*zx*zkm1-(double)(jn-1)*zkm2) / (double)(jn);
              zkm2 = zkm1;
              zkm1 = zk;
            }
          zkm1 = zkm2;
          zldn = ((double) (nlat)*(zkm1-zx*zk)) / (1.-zx*zx);
          zmod = -zk/zldn;
          zxn = zx+zmod;
          zan = zxn;



          zkm2 = 1.0;
          zkm1 = zxn;
          zx = zxn;
          for ( jn = 2; jn <= nlat; jn++ )
            {
              zk = ((double) (2*jn-1)*zx*zkm1-(double)(jn-1)*zkm2) / (double) (jn);
              zkm2 = zkm1;
              zkm1 = zk;
            }
          zkm1 = zkm2;
          zw = (1.0-zx*zx) / ((double) (nlat*nlat)*zkm1*zkm1);
          za = zan;
        }
      while ( iter <= itemax && fabs(zmod) >= DBL_EPSILON );

      pa[j] = zan;
      pw[j] = 2.0*zw;
    }

#if defined (SX)
#pragma vdir nodep
#endif
  for (j = 0; j < nlat/2; j++)
    {
      isym = nlat-(j+1);
      pa[isym] = -pa[j];
      pw[isym] = pw[j];
    }

  return;
}
#endif

void gaussaw(double *restrict pa, double *restrict pw, size_t nlat)
{

  gauaw(nlat, pa, pw);
}
#ifndef _GRID_H
#define _GRID_H


typedef unsigned char mask_t;

typedef struct {
  int self;
  int type;
  int prec;
  int proj;
  mask_t *mask;
  mask_t *mask_gme;
  double *xvals;
  double *yvals;
  double *area;
  double *xbounds;
  double *ybounds;
  double xfirst, yfirst;
  double xlast, ylast;
  double xinc, yinc;
  double lcc_originLon;
  double lcc_originLat;
  double lcc_lonParY;
  double lcc_lat1;
  double lcc_lat2;
  double lcc_xinc;
  double lcc_yinc;
  int lcc_projflag;
  int lcc_scanflag;
  short lcc_defined;
  short lcc2_defined;
  int laea_defined;
  double lcc2_lon_0;
  double lcc2_lat_0;
  double lcc2_lat_1;
  double lcc2_lat_2;
  double lcc2_a;
  double laea_lon_0;
  double laea_lat_0;
  double laea_a;
  double xpole, ypole, angle;
  short isCyclic;
  short isRotated;
  short xdef;
  short ydef;
  int nd, ni, ni2, ni3;
  int number, position;
  int trunc;
  int nvertex;
  char *reference;
  unsigned char uuid[CDI_UUID_SIZE];
  int *rowlon;
  int nrowlon;
  int size;
  int xsize;
  int ysize;
  int np;
  int lcomplex;
  int hasdims;
  char xname[CDI_MAX_NAME];
  char yname[CDI_MAX_NAME];
  char xlongname[CDI_MAX_NAME];
  char ylongname[CDI_MAX_NAME];
  char xstdname[CDI_MAX_NAME];
  char ystdname[CDI_MAX_NAME];
  char xunits[CDI_MAX_NAME];
  char yunits[CDI_MAX_NAME];
  char *name;
}
grid_t;


void grid_init(grid_t *gridptr);
void grid_free(grid_t *gridptr);

unsigned cdiGridCount(void);

const double *gridInqXvalsPtr(int gridID);
const double *gridInqYvalsPtr(int gridID);

const double *gridInqXboundsPtr(int gridID);
const double *gridInqYboundsPtr(int gridID);
const double *gridInqAreaPtr(int gridID);

int gridCompare(int gridID, const grid_t *grid);
int gridGenerate(const grid_t *grid);

void cdiGridGetIndexList(unsigned, int * );

void
gridUnpack(char * unpackBuffer, int unpackBufferSize,
           int * unpackBufferPos, int originNamespace, void *context,
           int force_id);

int varDefGrid(int vlistID, const grid_t *grid, int mode);


void gridGenXvals(int xsize, double xfirst, double xlast,
                  double xinc, double *xvals);
void gridGenYvals(int gridtype, int ysize, double yfirst, double ylast,
                  double yinc, double *yvals);




#endif
#ifndef RESOURCE_UNPACK_H
#define RESOURCE_UNPACK_H

#ifdef HAVE_CONFIG_H
#endif

enum
{ GRID = 1,
  ZAXIS = 2,
  TAXIS = 3,
  INSTITUTE = 4,
  MODEL = 5,
  STREAM = 6,
  VLIST = 7,
  RESH_DELETE,
  START = 55555555,
  END = 99999999
};

int reshUnpackResources(char * unpackBuffer, int unpackBufferSize,
                        void *context);

#endif
#ifndef _VLIST_H
#define _VLIST_H

#ifdef HAVE_CONFIG_H
#endif

#ifndef _ERROR_H
#endif

#include <stddef.h>

#ifndef _CDI_LIMITS_H
#endif

#define VALIDMISS 1.e+303




typedef struct {
  size_t xsz;
  size_t namesz;
  char *name;
  int indtype;
  int exdtype;




  size_t nelems;
  void *xvalue;
} cdi_att_t;


typedef struct {
  size_t nalloc;
  size_t nelems;
  cdi_att_t value[MAX_ATTRIBUTES];
} cdi_atts_t;


typedef struct
{
  int flag;
  int index;
  int mlevelID;
  int flevelID;
}
levinfo_t;

#define DEFAULT_LEVINFO(levID) \
  (levinfo_t){ 0, -1, levID, levID}




typedef struct
{
  int ens_index;
  int ens_count;
  int forecast_init_type;
}
ensinfo_t;



typedef struct
{
  int flag;
  int isUsed;
  int mvarID;
  int fvarID;
  int param;
  int gridID;
  int zaxisID;
  int tsteptype;
  int datatype;
  int instID;
  int modelID;
  int tableID;
  int timave;
  int timaccu;
  int typeOfGeneratingProcess;
  int productDefinitionTemplate;
  int chunktype;
  int xyz;
  int missvalused;
  int lvalidrange;
  char *name;
  char *longname;
  char *stdname;
  char *units;
  char *extra;
  double missval;
  double scalefactor;
  double addoffset;
  double validrange[2];
  levinfo_t *levinfo;
  int comptype;
  int complevel;
  ensinfo_t *ensdata;
  cdi_atts_t atts;
  int iorank;

  int subtypeID;

  int opt_grib_nentries;
  int opt_grib_kvpair_size;
  opt_key_val_pair_t *opt_grib_kvpair;
}
var_t;


typedef struct
{
  int locked;
  int self;
  int nvars;
  int ngrids;
  int nzaxis;
  int nsubtypes;
  long ntsteps;
  int taxisID;
  int tableID;
  int instID;
  int modelID;
  int varsAllocated;
  int gridIDs[MAX_GRIDS_PS];
  int zaxisIDs[MAX_ZAXES_PS];
  int subtypeIDs[MAX_SUBTYPES_PS];
  var_t *vars;
  cdi_atts_t atts;
}
vlist_t;


vlist_t *vlist_to_pointer(int vlistID);
void vlistCheckVarID(const char *caller, int vlistID, int varID);
const char *vlistInqVarNamePtr(int vlistID, int varID);
const char *vlistInqVarLongnamePtr(int vlistID, int varID);
const char *vlistInqVarStdnamePtr(int vlistID, int varID);
const char *vlistInqVarUnitsPtr(int vlistID, int varID);
void vlistDestroyVarName(int vlistID, int varID);
void vlistDestroyVarLongname(int vlistID, int varID);
void vlistDestroyVarStdname(int vlistID, int varID);
void vlistDestroyVarUnits(int vlistID, int varID);
void vlistDefVarTsteptype(int vlistID, int varID, int tsteptype);
int vlistInqVarMissvalUsed(int vlistID, int varID);
int vlistHasTime(int vlistID);

int vlistDelAtts(int vlistID, int varID);
int vlistCopyVarAtts(int vlistID1, int varID_1, int vlistID2, int varID_2);

void vlistUnpack(char * buffer, int bufferSize, int * pos,
                     int originNamespace, void *context, int force_id);


void vlistDefVarValidrange(int vlistID, int varID, const double *validrange);


int vlistInqVarValidrange(int vlistID, int varID, double *validrange);

void vlistInqVarDimorder(int vlistID, int varID, int (*outDimorder)[3]);

int vlist_att_compare(vlist_t *a, int varIDA, vlist_t *b, int varIDB, int attnum);

void vlist_lock(int vlistID);
void vlist_unlock(int vlistID);

void resize_opt_grib_entries(var_t *var, int nentries);



static inline void
vlistAdd2GridIDs(vlist_t *vlistptr, int gridID)
{
  int index, ngrids = vlistptr->ngrids;
  for ( index = 0; index < ngrids; index++ )
    if (vlistptr->gridIDs[index] == gridID ) break;
  if ( index == ngrids )
    {
      if (ngrids >= MAX_GRIDS_PS)
        Error("Internal limit exceeded: more than %d grids.", MAX_GRIDS_PS);
      ++(vlistptr->ngrids);
      vlistptr->gridIDs[ngrids] = gridID;
    }
}

static inline void
vlistAdd2ZaxisIDs(vlist_t *vlistptr, int zaxisID)
{
  int index, nzaxis = vlistptr->nzaxis;
  for ( index = 0; index < nzaxis; index++ )
    if ( zaxisID == vlistptr->zaxisIDs[index] ) break;

  if ( index == nzaxis )
    {
      if ( nzaxis >= MAX_ZAXES_PS )
        Error("Internal limit exceeded: more than %d zaxis.", MAX_ZAXES_PS);
      vlistptr->zaxisIDs[nzaxis] = zaxisID;
      vlistptr->nzaxis++;
    }
}

static inline void
vlistAdd2SubtypeIDs(vlist_t *vlistptr, int subtypeID)
{
  if ( subtypeID == CDI_UNDEFID ) return;

  int index, nsubs = vlistptr->nsubtypes;
  for ( index = 0; index < nsubs; index++ )
    if (vlistptr->subtypeIDs[index] == subtypeID ) break;
  if ( index == nsubs )
    {
      if (nsubs >= MAX_SUBTYPES_PS)
        Error("Internal limit exceeded: more than %d subs.", MAX_SUBTYPES_PS);
      ++(vlistptr->nsubtypes);
      vlistptr->subtypeIDs[nsubs] = subtypeID;
    }
}

extern
#ifndef __cplusplus
const
#endif
resOps vlistOps;

#endif
#if defined (HAVE_CONFIG_H)
#endif

#include <string.h>
#include <float.h>
#include <limits.h>


#undef UNDEFID
#define UNDEFID -1



static const char Grids[][17] = {
            "undefined",
            "generic",
            "gaussian",
            "gaussian reduced",
            "lonlat",
            "spectral",
            "fourier",
            "gme",
            "trajectory",
            "unstructured",
            "curvilinear",
            "lcc",
            "lcc2",
            "laea",
            "sinusoidal",
            "projection",
};


static int gridCompareP ( void * gridptr1, void * gridptr2 );
static void gridDestroyP ( void * gridptr );
static void gridPrintP ( void * gridptr, FILE * fp );
static int gridGetPackSize ( void * gridptr, void *context);
static void gridPack ( void * gridptr, void * buff, int size,
                                int *position, void *context);
static int gridTxCode ( void );

static const resOps gridOps = {
  gridCompareP,
  gridDestroyP,
  gridPrintP,
  gridGetPackSize,
  gridPack,
  gridTxCode
};

static int GRID_Debug = 0;

#define gridID2Ptr(gridID) (grid_t *)reshGetVal(gridID, &gridOps)

void grid_init(grid_t *gridptr)
{
  gridptr->self = CDI_UNDEFID;
  gridptr->type = CDI_UNDEFID;
  gridptr->proj = CDI_UNDEFID;
  gridptr->mask = NULL;
  gridptr->mask_gme = NULL;
  gridptr->xvals = NULL;
  gridptr->yvals = NULL;
  gridptr->area = NULL;
  gridptr->xbounds = NULL;
  gridptr->ybounds = NULL;
  gridptr->rowlon = NULL;
  gridptr->nrowlon = 0;
  gridptr->xfirst = 0.0;
  gridptr->xlast = 0.0;
  gridptr->xinc = 0.0;
  gridptr->yfirst = 0.0;
  gridptr->ylast = 0.0;
  gridptr->yinc = 0.0;
  gridptr->lcc_originLon = 0.0;
  gridptr->lcc_originLat = 0.0;
  gridptr->lcc_lonParY = 0.0;
  gridptr->lcc_lat1 = 0.0;
  gridptr->lcc_lat2 = 0.0;
  gridptr->lcc_xinc = 0.0;
  gridptr->lcc_yinc = 0.0;
  gridptr->lcc_projflag = 0;
  gridptr->lcc_scanflag = 0;
  gridptr->lcc_defined = FALSE;
  gridptr->lcc2_lon_0 = 0.0;
  gridptr->lcc2_lat_0 = 0.0;
  gridptr->lcc2_lat_1 = 0.0;
  gridptr->lcc2_lat_2 = 0.0;
  gridptr->lcc2_a = 0.0;
  gridptr->lcc2_defined = FALSE;
  gridptr->laea_lon_0 = 0.0;
  gridptr->laea_lat_0 = 0.0;
  gridptr->laea_a = 0.0;
  gridptr->laea_defined = FALSE;
  gridptr->trunc = 0;
  gridptr->nvertex = 0;
  gridptr->nd = 0;
  gridptr->ni = 0;
  gridptr->ni2 = 0;
  gridptr->ni3 = 0;
  gridptr->number = 0;
  gridptr->position = 0;
  gridptr->reference = NULL;
  gridptr->prec = 0;
  gridptr->size = 0;
  gridptr->xsize = 0;
  gridptr->ysize = 0;
  gridptr->np = 0;
  gridptr->xdef = 0;
  gridptr->ydef = 0;
  gridptr->isCyclic = CDI_UNDEFID;
  gridptr->isRotated = FALSE;
  gridptr->xpole = 0.0;
  gridptr->ypole = 0.0;
  gridptr->angle = 0.0;
  gridptr->lcomplex = 0;
  gridptr->hasdims = TRUE;
  gridptr->xname[0] = 0;
  gridptr->yname[0] = 0;
  gridptr->xlongname[0] = 0;
  gridptr->ylongname[0] = 0;
  gridptr->xunits[0] = 0;
  gridptr->yunits[0] = 0;
  gridptr->xstdname[0] = 0;
  gridptr->ystdname[0] = 0;
  memset(gridptr->uuid, 0, CDI_UUID_SIZE);
  gridptr->name = NULL;
}


void grid_free(grid_t *gridptr)
{
  if ( gridptr->mask ) Free(gridptr->mask);
  if ( gridptr->mask_gme ) Free(gridptr->mask_gme);
  if ( gridptr->xvals ) Free(gridptr->xvals);
  if ( gridptr->yvals ) Free(gridptr->yvals);
  if ( gridptr->area ) Free(gridptr->area);
  if ( gridptr->xbounds ) Free(gridptr->xbounds);
  if ( gridptr->ybounds ) Free(gridptr->ybounds);
  if ( gridptr->rowlon ) Free(gridptr->rowlon);
  if ( gridptr->reference ) Free(gridptr->reference);
  if ( gridptr->name ) Free(gridptr->name);

  grid_init(gridptr);
}

static grid_t *
gridNewEntry(cdiResH resH)
{
  grid_t *gridptr = (grid_t*) Malloc(sizeof(grid_t));
  grid_init(gridptr);
  if (resH == CDI_UNDEFID)
    gridptr->self = reshPut(gridptr, &gridOps);
  else
    {
      gridptr->self = resH;
      reshReplace(resH, gridptr, &gridOps);
    }
  return gridptr;
}

static
void gridInit (void)
{
  static int gridInitialized = 0;
  char *env;

  if ( gridInitialized ) return;

  gridInitialized = 1;

  env = getenv("GRID_DEBUG");
  if ( env ) GRID_Debug = atoi(env);
}

static
void grid_copy(grid_t *gridptr2, grid_t *gridptr1)
{
  int gridID2;

  gridID2 = gridptr2->self;
  memcpy(gridptr2, gridptr1, sizeof(grid_t));
  gridptr2->self = gridID2;
}

unsigned cdiGridCount(void)
{
  return reshCountType(&gridOps);
}


void gridGenXvals(int xsize, double xfirst, double xlast, double xinc, double *xvals)
{
  if ( (! (fabs(xinc) > 0)) && xsize > 1 )
    {
      if ( xfirst >= xlast )
        {
          while ( xfirst >= xlast ) xlast += 360;
          xinc = (xlast-xfirst)/(xsize);
        }
      else
        {
          xinc = (xlast-xfirst)/(xsize-1);
        }
    }

  for ( int i = 0; i < xsize; ++i )
    xvals[i] = xfirst + i*xinc;
}

static
void calc_gaussgrid(double *yvals, int ysize, double yfirst, double ylast)
{
  double *restrict yw = (double *) Malloc((size_t)ysize * sizeof(double));
  gaussaw(yvals, yw, (size_t)ysize);
  Free(yw);
  for (int i = 0; i < ysize; i++ )
    yvals[i] = asin(yvals[i])/M_PI*180.0;

  if ( yfirst < ylast && yfirst > -90.0 && ylast < 90.0 )
    {
      int yhsize = ysize/2;
      for (int i = 0; i < yhsize; i++ )
        {
          double ytmp = yvals[i];
          yvals[i] = yvals[ysize-i-1];
          yvals[ysize-i-1] = ytmp;
        }
    }
}


void gridGenYvals(int gridtype, int ysize, double yfirst, double ylast, double yinc, double *yvals)
{
  const double deleps = 0.002;

  if ( gridtype == GRID_GAUSSIAN || gridtype == GRID_GAUSSIAN_REDUCED )
    {
      if ( ysize > 2 )
        {
          calc_gaussgrid(yvals, ysize, yfirst, ylast);

          if ( ! (IS_EQUAL(yfirst, 0) && IS_EQUAL(ylast, 0)) )
            if ( fabs(yvals[0] - yfirst) > deleps || fabs(yvals[ysize-1] - ylast) > deleps )
              {
                double *restrict ytmp = NULL;
                int nstart, lfound = 0;
                int ny = (int) (180./fabs(ylast-yfirst)/(ysize-1) + 0.5);
                ny -= ny%2;
                if ( ny > ysize && ny < 4096 )
                  {
                    ytmp = (double *) Malloc((size_t)ny * sizeof (double));
                    calc_gaussgrid(ytmp, ny, yfirst, ylast);
                    {
                      int i;
                      for ( i = 0; i < (ny-ysize); i++ )
                        if ( fabs(ytmp[i] - yfirst) < deleps ) break;
                      nstart = i;
                    }

                    lfound = (nstart+ysize-1) < ny
                      && fabs(ytmp[nstart+ysize-1] - ylast) < deleps;
                    if ( lfound )
                      {
                        for (int i = 0; i < ysize; i++) yvals[i] = ytmp[i+nstart];
                      }
                  }

                if ( !lfound )
                  {
                    Warning("Cannot calculate gaussian latitudes for lat1 = %g latn = %g!", yfirst, ylast);
                    for (int i = 0; i < ysize; i++ ) yvals[i] = 0;
                    yvals[0] = yfirst;
                    yvals[ysize-1] = ylast;
                  }

                if ( ytmp ) Free(ytmp);
              }
        }
      else
        {
          yvals[0] = yfirst;
          yvals[ysize-1] = ylast;
        }
    }

  else
    {
      if ( (! (fabs(yinc) > 0)) && ysize > 1 )
        {
          if ( IS_EQUAL(yfirst, ylast) && IS_NOT_EQUAL(yfirst, 0) ) ylast *= -1;

          if ( yfirst > ylast )
            yinc = (yfirst-ylast)/(ysize-1);
          else if ( yfirst < ylast )
            yinc = (ylast-yfirst)/(ysize-1);
          else
            {
              if ( ysize%2 != 0 )
                {
                  yinc = 180.0/(ysize-1);
                  yfirst = -90;
                }
              else
                {
                  yinc = 180.0/ysize;
                  yfirst = -90 + yinc/2;
                }
            }
        }

      if ( yfirst > ylast && yinc > 0 ) yinc = -yinc;

      for (int i = 0; i < ysize; i++ )
        yvals[i] = yfirst + i*yinc;
    }




}
int gridCreate(int gridtype, int size)
{
  if ( CDI_Debug ) Message("gridtype=%s  size=%d", gridNamePtr(gridtype), size);

  if ( size < 0 || size > INT_MAX ) Error("Grid size (%d) out of bounds (0 - %d)!", size, INT_MAX);

  gridInit();

  grid_t *gridptr = gridNewEntry(CDI_UNDEFID);
  if ( ! gridptr ) Error("No memory");

  int gridID = gridptr->self;

  if ( CDI_Debug ) Message("gridID: %d", gridID);

  gridptr->type = gridtype;
  gridptr->size = size;


  if ( gridtype == GRID_UNSTRUCTURED ) gridptr->xsize = size;
  if ( gridtype == GRID_CURVILINEAR ) gridptr->nvertex = 4;

  switch (gridtype)
    {
    case GRID_LONLAT:
    case GRID_GAUSSIAN:
    case GRID_GAUSSIAN_REDUCED:
    case GRID_CURVILINEAR:
    case GRID_TRAJECTORY:
      {
        if ( gridtype == GRID_TRAJECTORY )
          {
            gridDefXname(gridID, "tlon");
            gridDefYname(gridID, "tlat");
          }
        else
          {
            gridDefXname(gridID, "lon");
            gridDefYname(gridID, "lat");
          }
        gridDefXlongname(gridID, "longitude");
        gridDefYlongname(gridID, "latitude");
          {
            strcpy(gridptr->xstdname, "longitude");
            strcpy(gridptr->ystdname, "latitude");
            gridDefXunits(gridID, "degrees_east");
            gridDefYunits(gridID, "degrees_north");
          }

        break;
      }
    case GRID_GME:
    case GRID_UNSTRUCTURED:
      {
        gridDefXname(gridID, "lon");
        gridDefYname(gridID, "lat");
        strcpy(gridptr->xstdname, "longitude");
        strcpy(gridptr->ystdname, "latitude");
        gridDefXunits(gridID, "degrees_east");
        gridDefYunits(gridID, "degrees_north");
        break;
      }
    case GRID_GENERIC:
      {
        gridDefXname(gridID, "x");
        gridDefYname(gridID, "y");






        break;
      }
    case GRID_LCC2:
    case GRID_SINUSOIDAL:
    case GRID_LAEA:
      {
        gridDefXname(gridID, "x");
        gridDefYname(gridID, "y");
        strcpy(gridptr->xstdname, "projection_x_coordinate");
        strcpy(gridptr->ystdname, "projection_y_coordinate");
        gridDefXunits(gridID, "m");
        gridDefYunits(gridID, "m");
        break;
      }
    }

  return (gridID);
}

static
void gridDestroyKernel( grid_t * gridptr )
{
  int id;

  xassert ( gridptr );

  id = gridptr->self;

  if ( gridptr->mask ) Free(gridptr->mask);
  if ( gridptr->mask_gme ) Free(gridptr->mask_gme);
  if ( gridptr->xvals ) Free(gridptr->xvals);
  if ( gridptr->yvals ) Free(gridptr->yvals);
  if ( gridptr->area ) Free(gridptr->area);
  if ( gridptr->xbounds ) Free(gridptr->xbounds);
  if ( gridptr->ybounds ) Free(gridptr->ybounds);
  if ( gridptr->rowlon ) Free(gridptr->rowlon);
  if ( gridptr->reference ) Free(gridptr->reference);

  Free( gridptr );

  reshRemove ( id, &gridOps );
}
void gridDestroy(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  gridDestroyKernel ( gridptr );
}

void gridDestroyP ( void * gridptr )
{
  gridDestroyKernel (( grid_t * ) gridptr );
}


const char *gridNamePtr(int gridtype)
{
  int size = (int) (sizeof(Grids)/sizeof(Grids[0]));

  const char *name = gridtype >= 0 && gridtype < size ? Grids[gridtype] : Grids[GRID_GENERIC];

  return (name);
}


void gridName(int gridtype, char *gridname)
{
  strcpy(gridname, gridNamePtr(gridtype));
}
void gridDefXname(int gridID, const char *xname)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if ( xname )
    {
      strncpy(gridptr->xname, xname, CDI_MAX_NAME);
      gridptr->xname[CDI_MAX_NAME - 1] = 0;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}
void gridDefXlongname(int gridID, const char *xlongname)
{
  grid_t *gridptr = gridID2Ptr(gridID);
  if ( xlongname )
    {
      strncpy(gridptr->xlongname, xlongname, CDI_MAX_NAME);
      gridptr->xlongname[CDI_MAX_NAME - 1] = 0;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}
void gridDefXunits(int gridID, const char *xunits)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if ( xunits )
    {
      strncpy(gridptr->xunits, xunits, CDI_MAX_NAME);
      gridptr->xunits[CDI_MAX_NAME - 1] = 0;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}
void gridDefYname(int gridID, const char *yname)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if ( yname )
    {
      strncpy(gridptr->yname, yname, CDI_MAX_NAME);
      gridptr->yname[CDI_MAX_NAME - 1] = 0;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}
void gridDefYlongname(int gridID, const char *ylongname)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if ( ylongname )
    {
      strncpy(gridptr->ylongname, ylongname, CDI_MAX_NAME);
      gridptr->ylongname[CDI_MAX_NAME - 1] = 0;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}
void gridDefYunits(int gridID, const char *yunits)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if ( yunits )
    {
      strncpy(gridptr->yunits, yunits, CDI_MAX_NAME);
      gridptr->yunits[CDI_MAX_NAME - 1] = 0;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}
void gridInqXname(int gridID, char *xname)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  strcpy(xname, gridptr->xname);
}
void gridInqXlongname(int gridID, char *xlongname)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  strcpy(xlongname, gridptr->xlongname);
}
void gridInqXunits(int gridID, char *xunits)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  strcpy(xunits, gridptr->xunits);
}


void gridInqXstdname(int gridID, char *xstdname)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  strcpy(xstdname, gridptr->xstdname);
}
void gridInqYname(int gridID, char *yname)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  strcpy(yname, gridptr->yname);
}
void gridInqYlongname(int gridID, char *ylongname)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  strcpy(ylongname, gridptr->ylongname);
}
void gridInqYunits(int gridID, char *yunits)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  strcpy(yunits, gridptr->yunits);
}

void gridInqYstdname(int gridID, char *ystdname)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  strcpy(ystdname, gridptr->ystdname);
}
int gridInqType(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  return (gridptr->type);
}
int gridInqSize(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  int size = gridptr->size;

  if ( ! size )
    {
      int xsize, ysize;

      xsize = gridptr->xsize;
      ysize = gridptr->ysize;

      if ( ysize )
        size = xsize *ysize;
      else
        size = xsize;

      gridptr->size = size;
    }

  return (size);
}

static
int nsp2trunc(int nsp)
{






  int trunc = (int) (sqrt(nsp*4 + 1.) - 3) / 2;
  return (trunc);
}


int gridInqTrunc(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if ( gridptr->trunc == 0 )
    {
      if ( gridptr->type == GRID_SPECTRAL )
        gridptr->trunc = nsp2trunc(gridptr->size);




    }

  return (gridptr->trunc);
}


void gridDefTrunc(int gridID, int trunc)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if (gridptr->trunc != trunc)
    {
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
      gridptr->trunc = trunc;
    }
}
void gridDefXsize(int gridID, int xsize)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  int gridSize = gridInqSize(gridID);
  if ( xsize > gridSize )
    Error("xsize %d is greater then gridsize %d", xsize, gridSize);

  if ( gridInqType(gridID) == GRID_UNSTRUCTURED && xsize != gridSize )
    Error("xsize %d must be equal to gridsize %d for gridtype: UNSTRUCTURED", xsize, gridSize);

  if (gridptr->xsize != xsize)
    {
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
      gridptr->xsize = xsize;
    }

  if ( gridInqType(gridID) != GRID_UNSTRUCTURED )
    {
      long axisproduct = gridptr->xsize*gridptr->ysize;
      if ( axisproduct > 0 && axisproduct != gridSize )
        Error("Inconsistent grid declaration! (xsize=%d ysize=%d gridsize=%d)",
              gridptr->xsize, gridptr->ysize, gridSize);
    }
}
void gridDefPrec(int gridID, int prec)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if (gridptr->prec != prec)
    {
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
      gridptr->prec = prec;
    }
}
int gridInqPrec(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  return (gridptr->prec);
}
int gridInqXsize(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  return (gridptr->xsize);
}
void gridDefYsize(int gridID, int ysize)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  int gridSize = gridInqSize(gridID);

  if ( ysize > gridSize )
    Error("ysize %d is greater then gridsize %d", ysize, gridSize);

  if ( gridInqType(gridID) == GRID_UNSTRUCTURED && ysize != gridSize )
    Error("ysize %d must be equal gridsize %d for gridtype: UNSTRUCTURED", ysize, gridSize);

  if (gridptr->ysize != ysize)
    {
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
      gridptr->ysize = ysize;
    }

  if ( gridInqType(gridID) != GRID_UNSTRUCTURED )
    {
      long axisproduct = gridptr->xsize*gridptr->ysize;
      if ( axisproduct > 0 && axisproduct != gridSize )
        Error("Inconsistent grid declaration! (xsize=%d ysize=%d gridsize=%d)",
              gridptr->xsize, gridptr->ysize, gridSize);
    }
}
int gridInqYsize(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  return (gridptr->ysize);
}
void gridDefNP(int gridID, int np)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if (gridptr->np != np)
    {
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
      gridptr->np = np;
    }
}
int gridInqNP(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  return (gridptr->np);
}
void gridDefRowlon(int gridID, int nrowlon, const int rowlon[])
{
  grid_t *gridptr = gridID2Ptr(gridID);

  gridptr->rowlon = (int *) Malloc((size_t)nrowlon * sizeof(int));
  gridptr->nrowlon = nrowlon;
  memcpy(gridptr->rowlon, rowlon, (size_t)nrowlon * sizeof(int));
  reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
}
void gridInqRowlon(int gridID, int *rowlon)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if ( gridptr->rowlon == 0 ) Error("undefined pointer!");

  memcpy(rowlon, gridptr->rowlon, (size_t)gridptr->nrowlon * sizeof(int));
}


int gridInqMask(int gridID, int *mask)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  long size = gridptr->size;

  if ( CDI_Debug && size == 0 )
    Warning("Size undefined for gridID = %d", gridID);

  if (mask && gridptr->mask)
    for (long i = 0; i < size; ++i)
      mask[i] = (int)gridptr->mask[i];

  if ( gridptr->mask == NULL ) size = 0;

  return (int)size;
}


void gridDefMask(int gridID, const int *mask)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  long size = gridptr->size;

  if ( size == 0 )
    Error("Size undefined for gridID = %d", gridID);

  if ( mask == NULL )
    {
      if ( gridptr->mask )
        {
          Free(gridptr->mask);
          gridptr->mask = NULL;
        }
    }
  else
    {
      if ( gridptr->mask == NULL )
        gridptr->mask = (mask_t *) Malloc((size_t)size*sizeof(mask_t));
      else if ( CDI_Debug )
        Warning("grid mask already defined!");

      for (long i = 0; i < size; ++i )
        gridptr->mask[i] = (mask_t)(mask[i] != 0);
    }
}


int gridInqMaskGME(int gridID, int *mask)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  long size = gridptr->size;

  if ( CDI_Debug && size == 0 )
    Warning("Size undefined for gridID = %d", gridID);

  if ( mask && gridptr->mask_gme )
    for (long i = 0; i < size; ++i)
      mask[i] = (int)gridptr->mask_gme[i];

  if ( gridptr->mask_gme == NULL ) size = 0;

  return (int)size;
}


void gridDefMaskGME(int gridID, const int *mask)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  long size = gridptr->size;

  if ( size == 0 )
    Error("Size undefined for gridID = %d", gridID);

  if ( gridptr->mask_gme == NULL )
    gridptr->mask_gme = (mask_t *) Malloc((size_t)size * sizeof (mask_t));
  else if ( CDI_Debug )
    Warning("mask already defined!");

  for (long i = 0; i < size; ++i)
    gridptr->mask_gme[i] = (mask_t)(mask[i] != 0);
}
int gridInqXvals(int gridID, double *xvals)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  long size;
  if ( gridptr->type == GRID_CURVILINEAR || gridptr->type == GRID_UNSTRUCTURED )
    size = gridptr->size;
  else if ( gridptr->type == GRID_GAUSSIAN_REDUCED )
    size = 2;
  else
    size = gridptr->xsize;

  if ( CDI_Debug && size == 0 )
    Warning("size undefined for gridID = %d", gridID);

  if ( size && xvals && gridptr->xvals )
    memcpy(xvals, gridptr->xvals, (size_t)size * sizeof (double));

  if ( gridptr->xvals == NULL ) size = 0;

  return (int)size;
}
void gridDefXvals(int gridID, const double *xvals)
{
  grid_t *gridptr = gridID2Ptr(gridID);
  int gridtype = gridptr->type;

  long size;

  if ( gridtype == GRID_UNSTRUCTURED || gridtype == GRID_CURVILINEAR )
    size = gridptr->size;
  else if ( gridtype == GRID_GAUSSIAN_REDUCED )
    size = 2;
  else
    size = gridptr->xsize;

  if ( size == 0 )
    Error("Size undefined for gridID = %d", gridID);

  if (gridptr->xvals && CDI_Debug)
    Warning("values already defined!");
  gridptr->xvals = (double *) Realloc(gridptr->xvals,
                                      (size_t)size * sizeof(double));
  memcpy(gridptr->xvals, xvals, (size_t)size * sizeof (double));
  reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
}
int gridInqYvals(int gridID, double *yvals)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  int gridtype = gridptr->type;
  long size
    = (gridtype == GRID_CURVILINEAR || gridtype == GRID_UNSTRUCTURED)
    ? gridptr->size : gridptr->ysize;

  if ( CDI_Debug && size == 0 )
    Warning("size undefined for gridID = %d!", gridID);

  if ( size && yvals && gridptr->yvals )
    memcpy(yvals, gridptr->yvals, (size_t)size * sizeof (double));

  if ( gridptr->yvals == NULL ) size = 0;

  return (int)size;
}
void gridDefYvals(int gridID, const double *yvals)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  int gridtype = gridptr->type;
  long size
    = (gridtype == GRID_CURVILINEAR || gridtype == GRID_UNSTRUCTURED)
    ? gridptr->size : gridptr->ysize;

  if ( size == 0 )
    Error("Size undefined for gridID = %d!", gridID);

  if (gridptr->yvals && CDI_Debug)
    Warning("Values already defined!");

  gridptr->yvals = (double *) Realloc(gridptr->yvals, (size_t)size * sizeof (double));
  memcpy(gridptr->yvals, yvals, (size_t)size * sizeof (double));
  reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
}


double gridInqXval(int gridID, int index)
{
  double xval = 0;
  grid_t *gridptr = gridID2Ptr(gridID);

  if ( gridptr->xvals ) xval = gridptr->xvals[index];

  return xval;
}
double gridInqYval(int gridID, int index)
{
  double yval = 0;
  grid_t *gridptr = gridID2Ptr(gridID);

  if ( gridptr->yvals ) yval = gridptr->yvals[index];

  return yval;
}
double gridInqXinc(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);
  double xinc = gridptr->xinc;
  const double *restrict xvals = gridptr->xvals;

  if ( (! (fabs(xinc) > 0)) && xvals )
    {
      int xsize = gridptr->xsize;
      if ( xsize > 1 )
        {
          xinc = fabs(xvals[xsize-1] - xvals[0])/(xsize-1);
          for ( size_t i = 2; i < (size_t)xsize; i++ )
            if ( fabs(fabs(xvals[i-1] - xvals[i]) - xinc) > 0.01*xinc )
              {
                xinc = 0;
                break;
              }

          gridptr->xinc = xinc;
        }
    }

  return xinc;
}
double gridInqYinc(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);
  double yinc = gridptr->yinc;
  const double *yvals = gridptr->yvals;

  if ( (! (fabs(yinc) > 0)) && yvals )
    {
      int ysize = gridptr->ysize;
      if ( ysize > 1 )
        {
          yinc = yvals[1] - yvals[0];
          double abs_yinc = fabs(yinc);
          for ( size_t i = 2; i < (size_t)ysize; i++ )
            if ( fabs(fabs(yvals[i] - yvals[i-1]) - abs_yinc) > (0.01*abs_yinc))
              {
                yinc = 0;
                break;
              }

          gridptr->yinc = yinc;
        }
    }

  return yinc;
}
double gridInqXpole(int gridID)
{

  grid_t *gridptr = gridID2Ptr(gridID);

  return gridptr->xpole;
}
void gridDefXpole(int gridID, double xpole)
{

  grid_t *gridptr = gridID2Ptr(gridID);

  if ( memcmp(gridptr->xstdname, "grid", 4) != 0 )
    strcpy(gridptr->xstdname, "grid_longitude");

  if ( gridptr->isRotated != TRUE || IS_NOT_EQUAL(gridptr->xpole, xpole) )
    {
      gridptr->isRotated = TRUE;
      gridptr->xpole = xpole;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}
double gridInqYpole(int gridID)
{

  grid_t *gridptr = gridID2Ptr(gridID);

  return (gridptr->ypole);
}
void gridDefYpole(int gridID, double ypole)
{

  grid_t *gridptr = gridID2Ptr(gridID);

  if ( memcmp(gridptr->ystdname, "grid", 4) != 0 )
    strcpy(gridptr->ystdname, "grid_latitude");

  if ( gridptr->isRotated != TRUE || IS_NOT_EQUAL(gridptr->ypole, ypole) )
    {
      gridptr->isRotated = TRUE;
      gridptr->ypole = ypole;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}
double gridInqAngle(int gridID)
{

  grid_t *gridptr = gridID2Ptr(gridID);

  return (gridptr->angle);
}
void gridDefAngle(int gridID, double angle)
{

  grid_t *gridptr = gridID2Ptr(gridID);

  if ( gridptr->isRotated != TRUE || IS_NOT_EQUAL(gridptr->angle, angle) )
    {
      gridptr->isRotated = TRUE;
      gridptr->angle = angle;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}
int gridInqGMEnd(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  return (gridptr->nd);
}
void gridDefGMEnd(int gridID, int nd)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if (gridptr->nd != nd)
    {
      gridptr->nd = nd;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}
int gridInqGMEni(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  return (gridptr->ni);
}
void gridDefGMEni(int gridID, int ni)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if (gridptr->ni != ni)
    {
      gridptr->ni = ni;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}
int gridInqGMEni2(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  return (gridptr->ni2);
}
void gridDefGMEni2(int gridID, int ni2)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if (gridptr->ni2 != ni2)
    {
      gridptr->ni2 = ni2;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}
int gridInqGMEni3(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  return (gridptr->ni3);
}

void gridDefGMEni3(int gridID, int ni3)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if (gridptr->ni3 != ni3)
    {
      gridptr->ni3 = ni3;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}
void gridChangeType(int gridID, int gridtype)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if ( CDI_Debug )
    Message("Changed grid type from %s to %s", gridNamePtr(gridptr->type), gridNamePtr(gridtype));

  if (gridptr->type != gridtype)
    {
      gridptr->type = gridtype;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}

static
void grid_check_cyclic(grid_t *gridptr)
{
  gridptr->isCyclic = FALSE;

  int xsize = gridptr->xsize,
    ysize = gridptr->ysize;
  const double *xvals = gridptr->xvals,
    *xbounds = gridptr->xbounds;

  if ( gridptr->type == GRID_GAUSSIAN || gridptr->type == GRID_LONLAT )
    {
      if ( xvals && xsize > 1 )
        {
          double xinc = xvals[1] - xvals[0];
          if ( IS_EQUAL(xinc, 0) ) xinc = (xvals[xsize-1] - xvals[0])/(xsize-1);

          double x0 = 2*xvals[xsize-1]-xvals[xsize-2]-360;

          if ( IS_NOT_EQUAL(xvals[0], xvals[xsize-1]) )
            if ( fabs(x0 - xvals[0]) < 0.01*xinc ) gridptr->isCyclic = TRUE;
        }
    }
  else if ( gridptr->type == GRID_CURVILINEAR )
    {
      if ( xvals && xsize > 1 )
        {
          long nc = 0;
          for ( int j = 0; j < ysize; ++j )
            {
              long i1 = j*xsize,
                i2 = j*xsize+1,
                in = j*xsize+(xsize-1);
              double val1 = xvals[i1],
                val2 = xvals[i2],
                valn = xvals[in];

              double xinc = fabs(val2-val1);

              if ( val1 < 1 && valn > 300 ) val1 += 360;
              if ( valn < 1 && val1 > 300 ) valn += 360;
              if ( val1 < -179 && valn > 120 ) val1 += 360;
              if ( valn < -179 && val1 > 120 ) valn += 360;
              if ( fabs(valn-val1) > 180 ) val1 += 360;

              double x0 = valn + copysign(xinc, val1 - valn);

              nc += fabs(x0-val1) < 0.5*xinc;
            }
          gridptr->isCyclic = nc > 0.5*ysize ? TRUE : FALSE;
        }

      if ( xbounds && xsize > 1 )
        {
          gridptr->isCyclic = TRUE;
          for ( int j = 0; j < ysize; ++j )
            {
              long i1 = j*xsize*4,
                i2 = j*xsize*4+(xsize-1)*4;
              long nc = 0;
              for (unsigned k1 = 0; k1 < 4; ++k1 )
                {
                  double val1 = xbounds[i1+k1];
                  for (unsigned k2 = 0; k2 < 4; ++k2 )
                    {
                      double val2 = xbounds[i2+k2];

                      if ( val1 < 1 && val2 > 300 ) val1 += 360;
                      if ( val2 < 1 && val1 > 300 ) val2 += 360;
                      if ( val1 < -179 && val2 > 120 ) val1 += 360;
                      if ( val2 < -179 && val1 > 120 ) val2 += 360;
                      if ( fabs(val2-val1) > 180 ) val1 += 360;

                      if ( fabs(val1-val2) < 0.001 )
                        {
                          nc++;
                          break;
                        }
                    }
                }

              if ( nc < 1 )
                {
                  gridptr->isCyclic = FALSE;
                  break;
                }
            }
        }
    }
}


int gridIsCircular(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if ( gridptr->isCyclic == CDI_UNDEFID ) grid_check_cyclic(gridptr);

  return ( gridptr->isCyclic );
}


int gridIsRotated(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  return ( gridptr->isRotated );
}

static
int compareXYvals(int gridID, long xsize, long ysize, double *xvals0, double *yvals0)
{
  long i;
  int differ = 0;

  if ( !differ && xsize == gridInqXvals(gridID, NULL) )
    {
      double *xvals = (double *) Malloc((size_t)xsize * sizeof (double));

      gridInqXvals(gridID, xvals);

      for ( i = 0; i < xsize; ++i )
        if ( fabs(xvals0[i] - xvals[i]) > 1.e-10 )
          {
            differ = 1;
            break;
          }

      Free(xvals);
    }

  if ( !differ && ysize == gridInqYvals(gridID, NULL) )
    {
      double *yvals = (double *) Malloc((size_t)ysize * sizeof (double));

      gridInqYvals(gridID, yvals);

      for ( i = 0; i < ysize; ++i )
        if ( fabs(yvals0[i] - yvals[i]) > 1.e-10 )
          {
            differ = 1;
            break;
          }

      Free(yvals);
    }

  return (differ);
}

static
int compareXYvals2(int gridID, int gridsize, double *xvals, double *yvals)
{
  int differ = 0;

  if ( !differ && ((xvals == NULL && gridInqXvalsPtr(gridID) != NULL) || (xvals != NULL && gridInqXvalsPtr(gridID) == NULL)) ) differ = 1;
  if ( !differ && ((yvals == NULL && gridInqYvalsPtr(gridID) != NULL) || (yvals != NULL && gridInqYvalsPtr(gridID) == NULL)) ) differ = 1;

  if ( !differ && xvals && gridInqXvalsPtr(gridID) )
    {
      if ( fabs(xvals[0] - gridInqXval(gridID, 0)) > 1.e-9 ||
           fabs(xvals[gridsize-1] - gridInqXval(gridID, gridsize-1)) > 1.e-9 )
        differ = 1;
    }

  if ( !differ && yvals && gridInqYvalsPtr(gridID) )
    {
      if ( fabs(yvals[0] - gridInqYval(gridID, 0)) > 1.e-9 ||
           fabs(yvals[gridsize-1] - gridInqYval(gridID, gridsize-1)) > 1.e-9 )
        differ = 1;
    }

  return differ;
}


int gridCompare(int gridID, const grid_t *grid)
{
  int differ = 1;

  if ( grid->type == gridInqType(gridID) || grid->type == GRID_GENERIC )
    {
      if ( grid->size == gridInqSize(gridID) )
        {
          differ = 0;
          if ( grid->type == GRID_LONLAT )
            {
              if ( grid->xsize == gridInqXsize(gridID) && grid->ysize == gridInqYsize(gridID) )
                {
                  if ( grid->xdef == 2 && grid->ydef == 2 )
                    {
                      if ( ! (IS_EQUAL(grid->xfirst, 0) && IS_EQUAL(grid->xlast, 0) && IS_EQUAL(grid->xinc, 0)) &&
                           ! (IS_EQUAL(grid->yfirst, 0) && IS_EQUAL(grid->ylast, 0) && IS_EQUAL(grid->yinc, 0)) &&
                           IS_NOT_EQUAL(grid->xfirst, grid->xlast) && IS_NOT_EQUAL(grid->yfirst, grid->ylast) )
                        {
                          if ( IS_NOT_EQUAL(grid->xfirst, gridInqXval(gridID, 0)) ||
                               IS_NOT_EQUAL(grid->yfirst, gridInqYval(gridID, 0)))
                            {
                              differ = 1;
                            }
                          if ( !differ && fabs(grid->xinc) > 0 &&
                               fabs(fabs(grid->xinc) - fabs(gridInqXinc(gridID))) > fabs(grid->xinc/1000))
                            {
                              differ = 1;
                            }
                          if ( !differ && fabs(grid->yinc) > 0 &&
                               fabs(fabs(grid->yinc) - fabs(gridInqYinc(gridID))) > fabs(grid->yinc/1000))
                            {
                              differ = 1;
                            }
                        }
                    }
                  else
                    {
                      if ( grid->xvals && grid->yvals )
                        differ = compareXYvals(gridID, grid->xsize, grid->ysize, grid->xvals, grid->yvals);
                    }
                }
              else
                differ = 1;
            }
          else if ( grid->type == GRID_GENERIC )
            {
              if ( grid->xsize == gridInqXsize(gridID) && grid->ysize == gridInqYsize(gridID) )
                {
                  if ( grid->xdef == 1 && grid->ydef == 1 )
                    {
                      if ( grid->xvals && grid->yvals )
                        differ = compareXYvals(gridID, grid->xsize, grid->ysize, grid->xvals, grid->yvals);
                    }
                }
              else if ( (grid->ysize == 0 || grid->ysize == 1) &&
                        grid->xsize == gridInqXsize(gridID)*gridInqYsize(gridID) )
                {
                }
              else
                differ = 1;
            }
          else if ( grid->type == GRID_GAUSSIAN )
            {
              if ( grid->xsize == gridInqXsize(gridID) && grid->ysize == gridInqYsize(gridID) )
                {
                  if ( grid->xdef == 2 && grid->ydef == 2 )
                    {
                      if ( ! (IS_EQUAL(grid->xfirst, 0) && IS_EQUAL(grid->xlast, 0) && IS_EQUAL(grid->xinc, 0)) &&
                           ! (IS_EQUAL(grid->yfirst, 0) && IS_EQUAL(grid->ylast, 0)) )
                        if ( fabs(grid->xfirst - gridInqXval(gridID, 0)) > 0.0015 ||
                             fabs(grid->yfirst - gridInqYval(gridID, 0)) > 0.0015 ||
                             (fabs(grid->xinc)>0 && fabs(fabs(grid->xinc) - fabs(gridInqXinc(gridID))) > fabs(grid->xinc/1000)) )
                          {
                            differ = 1;
                          }
                    }
                  else
                    {
                      if ( grid->xvals && grid->yvals )
                        differ = compareXYvals(gridID, grid->xsize, grid->ysize, grid->xvals, grid->yvals);
                    }
                }
              else
                differ = 1;
            }
          else if ( grid->type == GRID_CURVILINEAR )
            {
              if ( grid->xsize == gridInqXsize(gridID) && grid->ysize == gridInqYsize(gridID) )
                differ = compareXYvals2(gridID, grid->size, grid->xvals, grid->yvals);
            }
          else if ( grid->type == GRID_UNSTRUCTURED )
            {
              unsigned char uuidOfHGrid[CDI_UUID_SIZE];
              gridInqUUID(gridID, uuidOfHGrid);

              if ( !differ && uuidOfHGrid[0] && grid->uuid[0] && memcmp(uuidOfHGrid, grid->uuid, CDI_UUID_SIZE) != 0 ) differ = 1;

              if ( !differ &&
                   ((grid->xvals == NULL && gridInqXvalsPtr(gridID) != NULL) || (grid->xvals != NULL && gridInqXvalsPtr(gridID) == NULL)) &&
                   ((grid->yvals == NULL && gridInqYvalsPtr(gridID) != NULL) || (grid->yvals != NULL && gridInqYvalsPtr(gridID) == NULL)) )
                {
                  if ( !differ && grid->nvertex && gridInqNvertex(gridID) && grid->nvertex != gridInqNvertex(gridID) ) differ = 1;
                  if ( !differ && grid->number && gridInqNumber(gridID) && grid->number != gridInqNumber(gridID) ) differ = 1;
                  if ( !differ && grid->number && gridInqNumber(gridID) && grid->position != gridInqPosition(gridID) ) differ = 1;
                }
              else
                {
                  if ( !differ && grid->nvertex != gridInqNvertex(gridID) ) differ = 1;
                  if ( !differ && grid->number != gridInqNumber(gridID) ) differ = 1;
                  if ( !differ && grid->number > 0 && grid->position != gridInqPosition(gridID) ) differ = 1;
                  if ( !differ )
                    differ = compareXYvals2(gridID, grid->size, grid->xvals, grid->yvals);
                }
              }
        }
    }

  return differ;
}


int gridCompareP ( void * gridptr1, void * gridptr2 )
{
  grid_t * g1 = ( grid_t * ) gridptr1;
  grid_t * g2 = ( grid_t * ) gridptr2;
  enum { equal = 0,
         differ = -1 };
  int i, size;

  xassert ( g1 );
  xassert ( g2 );

  if ( g1->type != g2->type ) return differ;
  if ( g1->prec != g2->prec ) return differ;
  if ( g1->lcc_projflag != g2->lcc_projflag ) return differ;
  if ( g1->lcc_scanflag != g2->lcc_scanflag ) return differ;
  if ( g1->lcc_defined != g2->lcc_defined ) return differ;
  if ( g1->lcc2_defined != g2->lcc2_defined ) return differ;
  if ( g1->laea_defined != g2->laea_defined ) return differ;
  if ( g1->isCyclic != g2->isCyclic ) return differ;
  if ( g1->isRotated != g2->isRotated ) return differ;
  if ( g1->xdef != g2->xdef ) return differ;
  if ( g1->ydef != g2->ydef ) return differ;
  if ( g1->nd != g2->nd ) return differ;
  if ( g1->ni != g2->ni ) return differ;
  if ( g1->ni2 != g2->ni2 ) return differ;
  if ( g1->ni3 != g2->ni3 ) return differ;
  if ( g1->number != g2->number ) return differ;
  if ( g1->position != g2->position ) return differ;
  if ( g1->trunc != g2->trunc ) return differ;
  if ( g1->nvertex != g2->nvertex ) return differ;
  if ( g1->nrowlon != g2->nrowlon ) return differ;
  if ( g1->size != g2->size ) return differ;
  if ( g1->xsize != g2->xsize ) return differ;
  if ( g1->ysize != g2->ysize ) return differ;
  if ( g1->lcomplex != g2->lcomplex ) return differ;

  if ( g1->rowlon )
    {
      for ( i = 0; i < g1->nrowlon; i++ )
        if ( g1->rowlon[i] != g2->rowlon[i] ) return differ;
    }
  else if ( g2->rowlon )
    return differ;

  if ( IS_NOT_EQUAL(g1->xfirst , g2->xfirst) ) return differ;
  if ( IS_NOT_EQUAL(g1->yfirst , g2->yfirst) ) return differ;
  if ( IS_NOT_EQUAL(g1->xlast , g2->xlast) ) return differ;
  if ( IS_NOT_EQUAL(g1->ylast , g2->ylast) ) return differ;
  if ( IS_NOT_EQUAL(g1->xinc , g2->xinc) ) return differ;
  if ( IS_NOT_EQUAL(g1->yinc , g2->yinc) ) return differ;
  if ( IS_NOT_EQUAL(g1->lcc_originLon , g2->lcc_originLon) ) return differ;
  if ( IS_NOT_EQUAL(g1->lcc_originLat , g2->lcc_originLat) ) return differ;
  if ( IS_NOT_EQUAL(g1->lcc_lonParY , g2->lcc_lonParY) ) return differ;
  if ( IS_NOT_EQUAL(g1->lcc_lat1 , g2->lcc_lat1) ) return differ;
  if ( IS_NOT_EQUAL(g1->lcc_lat2 , g2->lcc_lat2) ) return differ;
  if ( IS_NOT_EQUAL(g1->lcc_xinc , g2->lcc_xinc) ) return differ;
  if ( IS_NOT_EQUAL(g1->lcc_yinc , g2->lcc_yinc) ) return differ;
  if ( IS_NOT_EQUAL(g1->lcc2_lon_0 , g2->lcc2_lon_0) ) return differ;
  if ( IS_NOT_EQUAL(g1->lcc2_lat_0 , g2->lcc2_lat_0) ) return differ;
  if ( IS_NOT_EQUAL(g1->lcc2_lat_1 , g2->lcc2_lat_1) ) return differ;
  if ( IS_NOT_EQUAL(g1->lcc2_lat_2 , g2->lcc2_lat_2) ) return differ;
  if ( IS_NOT_EQUAL(g1->lcc2_a , g2->lcc2_a) ) return differ;
  if ( IS_NOT_EQUAL(g1->laea_lon_0 , g2->laea_lon_0) ) return differ;
  if ( IS_NOT_EQUAL(g1->laea_lat_0 , g2->laea_lat_0) ) return differ;
  if ( IS_NOT_EQUAL(g1->laea_a , g2->laea_a) ) return differ;
  if ( IS_NOT_EQUAL(g1->xpole , g2->xpole) ) return differ;
  if ( IS_NOT_EQUAL(g1->ypole , g2->ypole) ) return differ;
  if ( IS_NOT_EQUAL(g1->angle , g2->angle) ) return differ;

  if ( g1->xvals )
    {
      if ( g1->type == GRID_UNSTRUCTURED || g1->type == GRID_CURVILINEAR )
        size = g1->size;
      else
        size = g1->xsize;
      xassert ( size );

      if ( !g2->xvals ) return differ;

      for ( i = 0; i < size; i++ )
        if ( IS_NOT_EQUAL(g1->xvals[i], g2->xvals[i]) ) return differ;
    }
  else if ( g2->xvals )
    return differ;

  if ( g1->yvals )
    {
      if ( g1->type == GRID_UNSTRUCTURED || g1->type == GRID_CURVILINEAR )
        size = g1->size;
      else
        size = g1->ysize;
      xassert ( size );

      if ( !g2->yvals ) return differ;

      for ( i = 0; i < size; i++ )
        if ( IS_NOT_EQUAL(g1->yvals[i], g2->yvals[i]) ) return differ;
    }
  else if ( g2->yvals )
    return differ;

  if ( g1->area )
    {
      xassert ( g1->size );

      if ( !g2->area ) return differ;

      for ( i = 0; i < g1->size; i++ )
        if ( IS_NOT_EQUAL(g1->area[i], g2->area[i]) ) return differ;
    }
  else if ( g2->area )
    return differ;

  if ( g1->xbounds )
    {
      xassert ( g1->nvertex );
      if ( g1->type == GRID_CURVILINEAR || g1->type == GRID_UNSTRUCTURED )
        size = g1->nvertex * g1->size;
      else
        size = g1->nvertex * g1->xsize;
      xassert ( size );

      if ( !g2->xbounds ) return differ;

      for ( i = 0; i < size; i++ )
        if ( IS_NOT_EQUAL(g1->xbounds[i], g2->xbounds[i]) ) return differ;
    }
  else if ( g2->xbounds )
    return differ;

  if ( g1->ybounds )
    {
      xassert ( g1->nvertex );
      if ( g1->type == GRID_CURVILINEAR || g1->type == GRID_UNSTRUCTURED )
        size = g1->nvertex * g1->size;
      else
        size = g1->nvertex * g1->ysize;
      xassert ( size );

      if ( !g2->ybounds ) return differ;

      for ( i = 0; i < size; i++ )
        if ( IS_NOT_EQUAL(g1->ybounds[i], g2->ybounds[i]) ) return differ;
    }
  else if ( g2->ybounds )
    return differ;

  if (strcmp(g1->xname, g2->xname)) return differ;
  if (strcmp(g1->yname, g2->yname)) return differ;
  if (strcmp(g1->xlongname, g2->xlongname)) return differ;
  if (strcmp(g1->ylongname, g2->ylongname)) return differ;
  if (strcmp(g1->xstdname, g2->xstdname)) return differ;
  if (strcmp(g1->ystdname, g2->ystdname)) return differ;
  if (strcmp(g1->xunits, g2->xunits)) return differ;
  if (strcmp(g1->yunits, g2->yunits)) return differ;

  if ( g1->reference )
    {
      if ( !g2->reference ) return differ;
      if ( strcmp(g1->reference, g2->reference) ) return differ;
    }
  else if ( g2->reference )
    return differ;

  if ( g1->mask )
    {
      xassert ( g1->size );
      if ( !g2->mask ) return differ;
      if ( memcmp ( g1->mask, g2->mask, (size_t)g1->size * sizeof(mask_t)) ) return differ;
    }
  else if ( g2->mask )
    return differ;

  if ( g1->mask_gme )
    {
      xassert ( g1->size );
      if ( !g2->mask_gme ) return differ;
      if ( memcmp ( g1->mask_gme, g2->mask_gme, (size_t)g1->size * sizeof(mask_t)) ) return differ;
    }
  else if ( g2->mask_gme )
    return differ;

  if (memcmp(g1->uuid, g2->uuid, CDI_UUID_SIZE))
    return differ;

  return equal;
}


int gridGenerate(const grid_t *grid)
{
  int gridID = gridCreate(grid->type, grid->size);

  grid_t *gridptr = gridID2Ptr(gridID);

  gridDefPrec(gridID, grid->prec);

  switch (grid->type)
    {
    case GRID_LONLAT:
    case GRID_GAUSSIAN:
    case GRID_UNSTRUCTURED:
    case GRID_CURVILINEAR:
    case GRID_GENERIC:
    case GRID_LCC:
    case GRID_LCC2:
    case GRID_SINUSOIDAL:
    case GRID_LAEA:
    case GRID_PROJECTION:
      {
        if ( grid->xsize > 0 ) gridDefXsize(gridID, grid->xsize);
        if ( grid->ysize > 0 ) gridDefYsize(gridID, grid->ysize);

        if ( grid->type == GRID_GAUSSIAN ) gridDefNP(gridID, grid->np);

        if ( grid->nvertex > 0 )
          gridDefNvertex(gridID, grid->nvertex);

        if ( grid->xdef == 1 )
          {
            gridDefXvals(gridID, grid->xvals);
            if ( grid->xbounds )
              gridDefXbounds(gridID, grid->xbounds);
          }
        else if ( grid->xdef == 2 )
          {
            double *xvals
              = (double *) Malloc((size_t)grid->xsize * sizeof (double));
            gridGenXvals(grid->xsize, grid->xfirst, grid->xlast, grid->xinc, xvals);
            gridDefXvals(gridID, xvals);
            Free(xvals);



          }

        if ( grid->ydef == 1 )
          {
            gridDefYvals(gridID, grid->yvals);
            if ( grid->ybounds && grid->nvertex )
              gridDefYbounds(gridID, grid->ybounds);
          }
        else if ( grid->ydef == 2 )
          {
            double *yvals
              = (double *) Malloc((size_t)grid->ysize * sizeof (double));
            gridGenYvals(grid->type, grid->ysize, grid->yfirst, grid->ylast, grid->yinc, yvals);
            gridDefYvals(gridID, yvals);
            Free(yvals);



          }

        if ( grid->isRotated )
          {
            gridDefXname(gridID, "rlon");
            gridDefYname(gridID, "rlat");
            gridDefXlongname(gridID, "longitude in rotated pole grid");
            gridDefYlongname(gridID, "latitude in rotated pole grid");
            strcpy(gridptr->xstdname, "grid_longitude");
            strcpy(gridptr->ystdname, "grid_latitude");
            gridDefXunits(gridID, "degrees");
            gridDefYunits(gridID, "degrees");

            gridDefXpole(gridID, grid->xpole);
            gridDefYpole(gridID, grid->ypole);
            gridDefAngle(gridID, grid->angle);
          }

        if ( grid->area )
          {
            gridDefArea(gridID, grid->area);
          }

        if ( grid->type == GRID_LAEA )
          gridDefLaea(gridID, grid->laea_a, grid->laea_lon_0, grid->laea_lat_0);

        if ( grid->type == GRID_LCC2 )
          gridDefLcc2(gridID, grid->lcc2_a, grid->lcc2_lon_0, grid->lcc2_lat_0, grid->lcc2_lat_1, grid->lcc2_lat_2);

        if ( grid->type == GRID_LCC )
          gridDefLCC(gridID, grid->lcc_originLon, grid->lcc_originLat, grid->lcc_lonParY,
                     grid->lcc_lat1, grid->lcc_lat2, grid->lcc_xinc, grid->lcc_yinc,
                     grid->lcc_projflag, grid->lcc_scanflag);

        if ( grid->type == GRID_UNSTRUCTURED )
          {
            int number = grid->number;
            int position = grid->position;
            if ( position < 0 ) position = 0;
            if ( number > 0 )
              {
                gridDefNumber(gridID, number);
                gridDefPosition(gridID, position);
              }
            gridDefUUID(gridID, grid->uuid);
            if ( grid->reference ) gridDefReference(gridID, grid->reference);
          }

        if ( grid->type == GRID_PROJECTION )
          {
            gridptr->name = strdup(grid->name);
          }

        break;
      }
    case GRID_GAUSSIAN_REDUCED:
      {
        gridDefNP(gridID, grid->np);
        gridDefYsize(gridID, grid->ysize);
        gridDefRowlon(gridID, grid->ysize, grid->rowlon);

        if ( grid->xdef == 2 )
          {
            double xvals[2];
            xvals[0] = grid->xfirst;
            xvals[1] = grid->xlast;
            gridDefXvals(gridID, xvals);
          }

        if ( grid->ydef == 1 )
          {
            gridDefYvals(gridID, grid->yvals);
            if ( grid->ybounds && grid->nvertex )
              gridDefYbounds(gridID, grid->ybounds);
          }
        else if ( grid->ydef == 2 )
          {
            double *yvals
              = (double *) Malloc((size_t)grid->ysize * sizeof (double));
            gridGenYvals(grid->type, grid->ysize, grid->yfirst, grid->ylast, grid->yinc, yvals);
            gridDefYvals(gridID, yvals);
            Free(yvals);



          }
        break;
      }
    case GRID_SPECTRAL:
      {
        gridDefTrunc(gridID, grid->trunc);
        if ( grid->lcomplex ) gridDefComplexPacking(gridID, 1);
        break;
      }
    case GRID_FOURIER:
      {
        gridDefTrunc(gridID, grid->trunc);
        break;
      }
    case GRID_GME:
      {
        gridDefGMEnd(gridID, grid->nd);
        gridDefGMEni(gridID, grid->ni);
        gridDefGMEni2(gridID, grid->ni2);
        gridDefGMEni3(gridID, grid->ni3);
        break;
      }
    case GRID_TRAJECTORY:
      {
        gridDefXsize(gridID, 1);
        gridDefYsize(gridID, 1);
        break;
      }
    default:
      {
        Error("Gridtype %s unsupported!", gridNamePtr(grid->type));
        break;
      }
    }

  if ( grid->xname[0] ) gridDefXname(gridID, grid->xname);
  if ( grid->xlongname[0] ) gridDefXlongname(gridID, grid->xlongname);
  if ( grid->xunits[0] ) gridDefXunits(gridID, grid->xunits);
  if ( grid->yname[0] ) gridDefYname(gridID, grid->yname);
  if ( grid->ylongname[0] ) gridDefYlongname(gridID, grid->ylongname);
  if ( grid->yunits[0] ) gridDefYunits(gridID, grid->yunits);

  return (gridID);
}
int gridDuplicate(int gridID)
{
  grid_t *gridptr = (grid_t *)reshGetVal(gridID, &gridOps);

  int gridtype = gridInqType(gridID);
  int gridsize = gridInqSize(gridID);

  int gridIDnew = gridCreate(gridtype, gridsize);
  grid_t *gridptrnew = (grid_t *)reshGetVal(gridIDnew, &gridOps);

  grid_copy(gridptrnew, gridptr);

  strcpy(gridptrnew->xname, gridptr->xname);
  strcpy(gridptrnew->yname, gridptr->yname);
  strcpy(gridptrnew->xlongname, gridptr->xlongname);
  strcpy(gridptrnew->ylongname, gridptr->ylongname);
  strcpy(gridptrnew->xunits, gridptr->xunits);
  strcpy(gridptrnew->yunits, gridptr->yunits);
  strcpy(gridptrnew->xstdname, gridptr->xstdname);
  strcpy(gridptrnew->ystdname, gridptr->ystdname);

  if (gridptr->reference)
    gridptrnew->reference = strdupx(gridptr->reference);

  size_t nrowlon = (size_t)gridptr->nrowlon;
  int irregular = gridtype == GRID_CURVILINEAR || gridtype == GRID_UNSTRUCTURED;
  if ( nrowlon )
    {
      gridptrnew->rowlon = (int *) Malloc(nrowlon * sizeof (int));
      memcpy(gridptrnew->rowlon, gridptr->rowlon, nrowlon * sizeof(int));
    }

  if ( gridptr->xvals != NULL )
    {
      size_t size = (size_t)(irregular ? gridsize : gridptr->xsize);

      gridptrnew->xvals = (double *) Malloc(size * sizeof (double));
      memcpy(gridptrnew->xvals, gridptr->xvals, size * sizeof (double));
    }

  if ( gridptr->yvals != NULL )
    {
      size_t size = (size_t)(irregular ? gridsize : gridptr->ysize);

      gridptrnew->yvals = (double *) Malloc(size * sizeof (double));
      memcpy(gridptrnew->yvals, gridptr->yvals, size * sizeof (double));
    }

  if ( gridptr->xbounds != NULL )
    {
      size_t size = (size_t)(irregular ? gridsize : gridptr->xsize)
        * (size_t)gridptr->nvertex;

      gridptrnew->xbounds = (double *) Malloc(size * sizeof (double));
      memcpy(gridptrnew->xbounds, gridptr->xbounds, size * sizeof (double));
    }

  if ( gridptr->ybounds != NULL )
    {
      size_t size = (size_t)(irregular ? gridsize : gridptr->ysize)
        * (size_t)gridptr->nvertex;

      gridptrnew->ybounds = (double *) Malloc(size * sizeof (double));
      memcpy(gridptrnew->ybounds, gridptr->ybounds, size * sizeof (double));
    }

  if ( gridptr->area != NULL )
    {
      size_t size = (size_t)gridsize;

      gridptrnew->area = (double *) Malloc(size * sizeof (double));
      memcpy(gridptrnew->area, gridptr->area, size * sizeof (double));
    }

  if ( gridptr->mask != NULL )
    {
      size_t size = (size_t)gridsize;

      gridptrnew->mask = (mask_t *) Malloc(size * sizeof(mask_t));
      memcpy(gridptrnew->mask, gridptr->mask, size * sizeof (mask_t));
    }

  if ( gridptr->mask_gme != NULL )
    {
      size_t size = (size_t)gridsize;

      gridptrnew->mask_gme = (mask_t *) Malloc(size * sizeof (mask_t));
      memcpy(gridptrnew->mask_gme, gridptr->mask_gme, size * sizeof(mask_t));
    }

  return (gridIDnew);
}


void gridCompress(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  int gridtype = gridInqType(gridID);
  if ( gridtype == GRID_UNSTRUCTURED )
    {
      if ( gridptr->mask_gme != NULL )
        {
          size_t gridsize = (size_t)gridInqSize(gridID);
          size_t nv = (size_t)gridptr->nvertex;

          size_t j = 0;
          double *area = gridptr->area,
            *xvals = gridptr->xvals,
            *yvals = gridptr->yvals,
            *xbounds = gridptr->xbounds,
            *ybounds = gridptr->ybounds;
          mask_t *mask_gme = gridptr->mask_gme;
          for (size_t i = 0; i < gridsize; i++ )
            {
              if (mask_gme[i])
                {
                  if (xvals) xvals[j] = xvals[i];
                  if (yvals) yvals[j] = yvals[i];
                  if (area) area[j] = area[i];
                  if (xbounds != NULL)
                    for (size_t iv = 0; iv < nv; iv++)
                      xbounds[j * nv + iv] = xbounds[i * nv + iv];
                  if (ybounds != NULL)
                    for (size_t iv = 0; iv < nv; iv++)
                      ybounds[j * nv + iv] = ybounds[i * nv + iv];

                  j++;
                }
            }


          gridsize = j;
          gridptr->size = (int)gridsize;
          gridptr->xsize = (int)gridsize;
          gridptr->ysize = (int)gridsize;

          if ( gridptr->xvals )
            gridptr->xvals = (double *) Realloc(gridptr->xvals, gridsize*sizeof(double));

          if ( gridptr->yvals )
            gridptr->yvals = (double *) Realloc(gridptr->yvals, gridsize*sizeof(double));

          if ( gridptr->area )
            gridptr->area = (double *) Realloc(gridptr->area, gridsize*sizeof(double));

          if ( gridptr->xbounds )
            gridptr->xbounds = (double *) Realloc(gridptr->xbounds, nv*gridsize*sizeof(double));

          if ( gridptr->ybounds )
            gridptr->ybounds = (double *) Realloc(gridptr->ybounds, nv*gridsize*sizeof(double));

          Free(gridptr->mask_gme);
          gridptr->mask_gme = NULL;
          reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
        }
    }
  else
    Warning("Unsupported grid type: %s", gridNamePtr(gridtype));
}


void gridDefArea(int gridID, const double *area)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  size_t size = (size_t)gridptr->size;

  if ( size == 0 )
    Error("size undefined for gridID = %d", gridID);

  if ( gridptr->area == NULL )
    gridptr->area = (double *) Malloc(size*sizeof(double));
  else if ( CDI_Debug )
    Warning("values already defined!");

  memcpy(gridptr->area, area, size * sizeof(double));
  reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
}


void gridInqArea(int gridID, double *area)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if (gridptr->area)
    memcpy(area, gridptr->area, (size_t)gridptr->size * sizeof (double));
}


int gridHasArea(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  int hasArea = (gridptr->area != NULL);

  return (hasArea);
}


const double *gridInqAreaPtr(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  return (gridptr->area);
}


void gridDefNvertex(int gridID, int nvertex)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if (gridptr->nvertex != nvertex)
    {
      gridptr->nvertex = nvertex;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}


int gridInqNvertex(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  return (gridptr->nvertex);
}
void gridDefXbounds(int gridID, const double *xbounds)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  size_t nvertex = (size_t)gridptr->nvertex;
  if ( nvertex == 0 )
    {
      Warning("nvertex undefined for gridID = %d. Cannot define bounds!", gridID);
      return;
    }

  int irregular = gridptr->type == GRID_CURVILINEAR
    || gridptr->type == GRID_UNSTRUCTURED;
  size_t size = nvertex
    * (size_t)(irregular ? gridptr->size : gridptr->xsize);
  if ( size == 0 )
    Error("size undefined for gridID = %d", gridID);

  if (gridptr->xbounds == NULL)
    gridptr->xbounds = (double *) Malloc(size * sizeof (double));
  else if ( CDI_Debug )
    Warning("values already defined!");

  memcpy(gridptr->xbounds, xbounds, size * sizeof (double));
  reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
}
int gridInqXbounds(int gridID, double *xbounds)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  size_t nvertex = (size_t)gridptr->nvertex;

  int irregular = gridptr->type == GRID_CURVILINEAR
    || gridptr->type == GRID_UNSTRUCTURED;
  size_t size = nvertex * (size_t)(irregular ? gridptr->size : gridptr->xsize);

  if ( size && xbounds && gridptr->xbounds )
    memcpy(xbounds, gridptr->xbounds, size * sizeof (double));

  if ( gridptr->xbounds == NULL ) size = 0;

  return ((int)size);
}


const double *gridInqXboundsPtr(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  return (gridptr->xbounds);
}
void gridDefYbounds(int gridID, const double *ybounds)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  size_t nvertex = (size_t)gridptr->nvertex;
  if ( nvertex == 0 )
    {
      Warning("nvertex undefined for gridID = %d. Cannot define bounds!", gridID);
      return;
    }

  int irregular = gridptr->type == GRID_CURVILINEAR
    || gridptr->type == GRID_UNSTRUCTURED;
  size_t size = nvertex * (size_t)(irregular ? gridptr->size : gridptr->ysize);

  if ( size == 0 )
    Error("size undefined for gridID = %d", gridID);

  if ( gridptr->ybounds == NULL )
    gridptr->ybounds = (double *) Malloc(size * sizeof (double));
  else if ( CDI_Debug )
    Warning("values already defined!");

  memcpy(gridptr->ybounds, ybounds, size * sizeof (double));
  reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
}
int gridInqYbounds(int gridID, double *ybounds)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  size_t nvertex = (size_t)gridptr->nvertex;

  int irregular = gridptr->type == GRID_CURVILINEAR
    || gridptr->type == GRID_UNSTRUCTURED;
  size_t size = nvertex * (size_t)(irregular ? gridptr->size : gridptr->ysize);

  if ( size && ybounds && gridptr->ybounds )
    memcpy(ybounds, gridptr->ybounds, size * sizeof (double));

  if ( gridptr->ybounds == NULL ) size = 0;

  return ((int)size);
}


const double *gridInqYboundsPtr(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  return (gridptr->ybounds);
}


static void gridPrintKernel(grid_t * gridptr, int index, int opt, FILE *fp)
{
  int xdim, ydim;
  int nbyte;
  int i, iv;
  unsigned char uuidOfHGrid[CDI_UUID_SIZE];
  int gridID = gridptr->self;
  const double *area = gridInqAreaPtr(gridID);
  const double *xvals = gridInqXvalsPtr(gridID);
  const double *yvals = gridInqYvalsPtr(gridID);
  const double *xbounds = gridInqXboundsPtr(gridID);
  const double *ybounds = gridInqYboundsPtr(gridID);

  int type = gridInqType(gridID);
  int trunc = gridInqTrunc(gridID);
  int gridsize = gridInqSize(gridID);
  int xsize = gridInqXsize(gridID);
  int ysize = gridInqYsize(gridID);
  int nvertex = gridInqNvertex(gridID);

  int nbyte0 = 0;
  fprintf(fp, "#\n");
  fprintf(fp, "# gridID %d\n", index);
  fprintf(fp, "#\n");
  fprintf(fp, "gridtype  = %s\n", gridNamePtr(type));
  fprintf(fp, "gridsize  = %d\n", gridsize);

  if ( type != GRID_GME )
    {
      if ( xvals )
        {
          if ( gridptr->xname[0] ) fprintf(fp, "xname     = %s\n", gridptr->xname);
          if ( gridptr->xlongname[0] ) fprintf(fp, "xlongname = %s\n", gridptr->xlongname);
          if ( gridptr->xunits[0] ) fprintf(fp, "xunits    = %s\n", gridptr->xunits);
        }
      if ( yvals )
        {
          if ( gridptr->yname[0] ) fprintf(fp, "yname     = %s\n", gridptr->yname);
          if ( gridptr->ylongname[0] ) fprintf(fp, "ylongname = %s\n", gridptr->ylongname);
          if ( gridptr->yunits[0] ) fprintf(fp, "yunits    = %s\n", gridptr->yunits);
        }
      if ( type == GRID_UNSTRUCTURED && nvertex > 0 ) fprintf(fp, "nvertex   = %d\n", nvertex);
    }

  switch (type)
    {
    case GRID_LONLAT:
    case GRID_GAUSSIAN:
    case GRID_GAUSSIAN_REDUCED:
    case GRID_GENERIC:
    case GRID_LCC2:
    case GRID_SINUSOIDAL:
    case GRID_LAEA:
    case GRID_CURVILINEAR:
    case GRID_UNSTRUCTURED:
      {
        if ( type == GRID_GAUSSIAN || type == GRID_GAUSSIAN_REDUCED ) fprintf(fp, "np        = %d\n", gridptr->np);

        if ( type == GRID_CURVILINEAR || type == GRID_UNSTRUCTURED )
          {
            xdim = gridsize;
            ydim = gridsize;
          }
        else if ( type == GRID_GAUSSIAN_REDUCED )
          {
            xdim = 2;
            ydim = ysize;
          }
        else
          {
            xdim = xsize;
            ydim = ysize;
          }

        if ( type != GRID_UNSTRUCTURED )
          {
            if ( xsize > 0 ) fprintf(fp, "xsize     = %d\n", xsize);
            if ( ysize > 0 ) fprintf(fp, "ysize     = %d\n", ysize);
          }

        if ( type == GRID_UNSTRUCTURED )
          {
            int number = gridInqNumber(gridID);
            int position = gridInqPosition(gridID);

            if ( number > 0 )
              {
                fprintf(fp, "number    = %d\n", number);
                if ( position >= 0 ) fprintf(fp, "position  = %d\n", position);
              }







            if ( gridInqReference(gridID, NULL) )
              {
                char reference_link[8192];
                gridInqReference(gridID, reference_link);
                fprintf(fp, "uri       = %s\n", reference_link);
              }
          }

        if ( type == GRID_LAEA )
          {
            double a = 0, lon_0 = 0, lat_0 = 0;
            gridInqLaea(gridID, &a, &lon_0, &lat_0);
            fprintf(fp, "a         = %g\n", a);
            fprintf(fp, "lon_0     = %g\n", lon_0);
            fprintf(fp, "lat_0     = %g\n", lat_0);
          }

        if ( type == GRID_LCC2 )
          {
            double a = 0, lon_0 = 0, lat_0 = 0, lat_1 = 0, lat_2 = 0;
            gridInqLcc2(gridID, &a, &lon_0, &lat_0, &lat_1, &lat_2);
            fprintf(fp, "a         = %g\n", a);
            fprintf(fp, "lon_0     = %g\n", lon_0);
            fprintf(fp, "lat_0     = %g\n", lat_0);
            fprintf(fp, "lat_1     = %g\n", lat_1);
            fprintf(fp, "lat_2     = %g\n", lat_2);
          }

        if ( gridptr->isRotated )
          {
            if ( xsize > 0 ) fprintf(fp, "xnpole    = %g\n", gridptr->xpole);
            if ( ysize > 0 ) fprintf(fp, "ynpole    = %g\n", gridptr->ypole);
            if ( IS_NOT_EQUAL(gridptr->angle, 0) ) fprintf(fp, "angle     = %g\n", gridptr->angle);
          }

        if ( xvals )
          {
            double xfirst = 0.0, xinc = 0.0;

            if ( type == GRID_LONLAT || type == GRID_GAUSSIAN ||
                 type == GRID_GENERIC || type == GRID_LCC2 ||
                 type == GRID_SINUSOIDAL || type == GRID_LAEA )
              {
                xfirst = gridInqXval(gridID, 0);
                xinc = gridInqXinc(gridID);
              }

            if ( IS_NOT_EQUAL(xinc, 0) && opt )
              {
                fprintf(fp, "xfirst    = %g\n", xfirst);
                fprintf(fp, "xinc      = %g\n", xinc);
              }
            else
              {
                nbyte0 = fprintf(fp, "xvals     = ");
                nbyte = nbyte0;
                for ( i = 0; i < xdim; i++ )
                  {
                    if ( nbyte > 80 )
                      {
                        fprintf(fp, "\n");
                        fprintf(fp, "%*s", nbyte0, "");
                        nbyte = nbyte0;
                      }
                    nbyte += fprintf(fp, "%.9g ", xvals[i]);
                  }
                fprintf(fp, "\n");
              }
          }

        if ( xbounds )
          {
            nbyte0 = fprintf(fp, "xbounds   = ");
            for ( i = 0; i < xdim; i++ )
              {
                if ( i ) fprintf(fp, "%*s", nbyte0, "");

                for ( iv = 0; iv < nvertex; iv++ )
                  fprintf(fp, "%.9g ", xbounds[i*nvertex+iv]);
                fprintf(fp, "\n");
              }
          }

        if ( yvals )
          {
            double yfirst = 0.0, yinc = 0.0;

            if ( type == GRID_LONLAT || type == GRID_GENERIC || type == GRID_LCC2 ||
                 type == GRID_SINUSOIDAL || type == GRID_LAEA )
              {
                yfirst = gridInqYval(gridID, 0);
                yinc = gridInqYinc(gridID);
              }

            if ( IS_NOT_EQUAL(yinc, 0) && opt )
              {
                fprintf(fp, "yfirst    = %g\n", yfirst);
                fprintf(fp, "yinc      = %g\n", yinc);
              }
            else
              {
                nbyte0 = fprintf(fp, "yvals     = ");
                nbyte = nbyte0;
                for ( i = 0; i < ydim; i++ )
                  {
                    if ( nbyte > 80 )
                      {
                        fprintf(fp, "\n");
                        fprintf(fp, "%*s", nbyte0, "");
                        nbyte = nbyte0;
                      }
                    nbyte += fprintf(fp, "%.9g ", yvals[i]);
                  }
                fprintf(fp, "\n");
              }
          }

        if ( ybounds )
          {
            nbyte0 = fprintf(fp, "ybounds   = ");
            for ( i = 0; i < ydim; i++ )
              {
                if ( i ) fprintf(fp, "%*s", nbyte0, "");

                for ( iv = 0; iv < nvertex; iv++ )
                  fprintf(fp, "%.9g ", ybounds[i*nvertex+iv]);
                fprintf(fp, "\n");
              }
          }

        if ( area )
          {
            nbyte0 = fprintf(fp, "area      = ");
            nbyte = nbyte0;
            for ( i = 0; i < gridsize; i++ )
              {
                if ( nbyte > 80 )
                  {
                    fprintf(fp, "\n");
                    fprintf(fp, "%*s", nbyte0, "");
                    nbyte = nbyte0;
                  }
                nbyte += fprintf(fp, "%.9g ", area[i]);
              }
            fprintf(fp, "\n");
          }

        if ( type == GRID_GAUSSIAN_REDUCED )
          {
            int *rowlon;
            nbyte0 = fprintf(fp, "rowlon    = ");
            nbyte = nbyte0;
            rowlon = (int *) Malloc((size_t)ysize*sizeof(int));
            gridInqRowlon(gridID, rowlon);
            for ( i = 0; i < ysize; i++ )
              {
                if ( nbyte > 80 )
                  {
                    fprintf(fp, "\n");
                    fprintf(fp, "%*s", nbyte0, "");
                    nbyte = nbyte0;
                  }
                nbyte += fprintf(fp, "%d ", rowlon[i]);
              }
            fprintf(fp, "\n");
            Free(rowlon);
          }

        break;
      }
    case GRID_LCC:
      {
        double originLon = 0, originLat = 0, lonParY = 0, lat1 = 0, lat2 = 0, xincm = 0, yincm = 0;
        int projflag = 0, scanflag = 0;
        gridInqLCC(gridID, &originLon, &originLat, &lonParY, &lat1, &lat2, &xincm, &yincm,
                   &projflag, &scanflag);

        fprintf(fp, "xsize     = %d\n", xsize);
        fprintf(fp, "ysize     = %d\n", ysize);

        fprintf(fp, "originLon = %g\n", originLon);
        fprintf(fp, "originLat = %g\n", originLat);
        fprintf(fp, "lonParY   = %g\n", lonParY);
        fprintf(fp, "lat1      = %g\n", lat1);
        fprintf(fp, "lat2      = %g\n", lat2);
        fprintf(fp, "xinc      = %g\n", xincm);
        fprintf(fp, "yinc      = %g\n", yincm);
        if ( (projflag & 128) == 0 )
          fprintf(fp, "projection = northpole\n");
        else
          fprintf(fp, "projection = southpole\n");

        break;
      }
    case GRID_SPECTRAL:
      {
        fprintf(fp, "truncation = %d\n", trunc);
        fprintf(fp, "complexpacking = %d\n", gridptr->lcomplex );
        break;
      }
    case GRID_FOURIER:
      {
        fprintf(fp, "truncation = %d\n", trunc);
        break;
      }
    case GRID_GME:
      {
        fprintf(fp, "ni        = %d\n", gridptr->ni );
        break;
      }
   default:
      {
        fprintf(stderr, "Unsupported grid type: %s\n", gridNamePtr(type));
        break;
      }
    }

  gridInqUUID(gridID, uuidOfHGrid);
  if ( !cdiUUIDIsNull(uuidOfHGrid) )
    {
      char uuidOfHGridStr[37];
      uuid2str(uuidOfHGrid, uuidOfHGridStr);
      if ( uuidOfHGridStr[0] != 0 && strlen(uuidOfHGridStr) == 36 )
        fprintf(fp, "uuid      = %s\n", uuidOfHGridStr);
    }

  if ( gridptr->mask )
    {
      nbyte0 = fprintf(fp, "mask      = ");
      nbyte = nbyte0;
      for ( i = 0; i < gridsize; i++ )
        {
          if ( nbyte > 80 )
            {
              fprintf(fp, "\n");
              fprintf(fp, "%*s", nbyte0, "");
              nbyte = nbyte0;
            }
          nbyte += fprintf(fp, "%d ", (int) gridptr->mask[i]);
        }
      fprintf(fp, "\n");
    }
}

void gridPrint ( int gridID, int index, int opt )
{
  grid_t *gridptr = gridID2Ptr(gridID);

  gridPrintKernel ( gridptr, index, opt, stdout );
}



void gridPrintP ( void * voidptr, FILE * fp )
{
  grid_t * gridptr = ( grid_t * ) voidptr;
  int nbyte0, nbyte, i;

  xassert ( gridptr );

  gridPrintKernel ( gridptr , gridptr->self, 0, fp );

  fprintf ( fp, "precision = %d\n", gridptr->prec);
  fprintf ( fp, "nd        = %d\n", gridptr->nd );
  fprintf ( fp, "ni        = %d\n", gridptr->ni );
  fprintf ( fp, "ni2       = %d\n", gridptr->ni2 );
  fprintf ( fp, "ni3       = %d\n", gridptr->ni3 );
  fprintf ( fp, "number    = %d\n", gridptr->number );
  fprintf ( fp, "position  = %d\n", gridptr->position );
  fprintf ( fp, "trunc     = %d\n", gridptr->trunc );
  fprintf ( fp, "lcomplex  = %d\n", gridptr->lcomplex );
  fprintf ( fp, "nrowlon   = %d\n", gridptr->nrowlon );

  if ( gridptr->rowlon )
    {
      nbyte0 = fprintf(fp, "rowlon    = ");
      nbyte = nbyte0;
      for ( i = 0; i < gridptr->nrowlon; i++ )
        {
          if ( nbyte > 80 )
            {
              fprintf(fp, "\n");
              fprintf(fp, "%*s", nbyte0, "");
              nbyte = nbyte0;
            }
          nbyte += fprintf(fp, "%d ", gridptr->rowlon[i]);
        }
      fprintf(fp, "\n");
    }

  if ( gridptr->mask_gme )
    {
      nbyte0 = fprintf(fp, "mask_gme  = ");
      nbyte = nbyte0;
      for ( i = 0; i < gridptr->size; i++ )
        {
          if ( nbyte > 80 )
            {
              fprintf(fp, "\n");
              fprintf(fp, "%*s", nbyte0, "");
              nbyte = nbyte0;
            }
          nbyte += fprintf(fp, "%d ", (int) gridptr->mask_gme[i]);
        }
      fprintf(fp, "\n");
    }
}


const double *gridInqXvalsPtr(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  return ( gridptr->xvals );
}


const double *gridInqYvalsPtr(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  return ( gridptr->yvals );
}
void gridDefLCC(int gridID, double originLon, double originLat, double lonParY,
                double lat1, double lat2, double xinc, double yinc,
                int projflag, int scanflag)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if ( gridptr->type != GRID_LCC )
    Warning("Definition of LCC grid for %s grid not allowed!",
            gridNamePtr(gridptr->type));
  else
    {
      gridptr->lcc_originLon = originLon;
      gridptr->lcc_originLat = originLat;
      gridptr->lcc_lonParY = lonParY;
      gridptr->lcc_lat1 = lat1;
      gridptr->lcc_lat2 = lat2;
      gridptr->lcc_xinc = xinc;
      gridptr->lcc_yinc = yinc;
      gridptr->lcc_projflag = projflag;
      gridptr->lcc_scanflag = scanflag;
      gridptr->lcc_defined = TRUE;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}
void gridInqLCC(int gridID, double *originLon, double *originLat, double *lonParY,
                double *lat1, double *lat2, double *xinc, double *yinc,
                int *projflag, int *scanflag)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if ( gridptr->type != GRID_LCC )
    Warning("Inquire of LCC grid definition for %s grid not allowed!",
            gridNamePtr(gridptr->type));
  else
    {
      if ( gridptr->lcc_defined )
        {
          *originLon = gridptr->lcc_originLon;
          *originLat = gridptr->lcc_originLat;
          *lonParY = gridptr->lcc_lonParY;
          *lat1 = gridptr->lcc_lat1;
          *lat2 = gridptr->lcc_lat2;
          *xinc = gridptr->lcc_xinc;
          *yinc = gridptr->lcc_yinc;
          *projflag = gridptr->lcc_projflag;
          *scanflag = gridptr->lcc_scanflag;
        }
      else
        Warning("Lambert Conformal grid undefined (gridID = %d)", gridID);
    }
}

void gridDefLcc2(int gridID, double earth_radius, double lon_0, double lat_0, double lat_1, double lat_2)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if ( gridptr->type != GRID_LCC2 )
    Warning("Definition of LCC2 grid for %s grid not allowed!",
            gridNamePtr(gridptr->type));
  else
    {
      gridptr->lcc2_a = earth_radius;
      gridptr->lcc2_lon_0 = lon_0;
      gridptr->lcc2_lat_0 = lat_0;
      gridptr->lcc2_lat_1 = lat_1;
      gridptr->lcc2_lat_2 = lat_2;
      gridptr->lcc2_defined = TRUE;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}


void gridInqLcc2(int gridID, double *earth_radius, double *lon_0, double *lat_0, double *lat_1, double *lat_2)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if ( gridptr->type != GRID_LCC2 )
    Warning("Inquire of LCC2 grid definition for %s grid not allowed!",
            gridNamePtr(gridptr->type));
  else
    {
      if ( gridptr->lcc2_defined )
        {
          *earth_radius = gridptr->lcc2_a;
          *lon_0 = gridptr->lcc2_lon_0;
          *lat_0 = gridptr->lcc2_lat_0;
          *lat_1 = gridptr->lcc2_lat_1;
          *lat_2 = gridptr->lcc2_lat_2;
        }
      else
        Warning("LCC2 grid undefined (gridID = %d)", gridID);
    }
}

void gridDefLaea(int gridID, double earth_radius, double lon_0, double lat_0)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if ( gridptr->type != GRID_LAEA )
    Warning("Definition of LAEA grid for %s grid not allowed!",
            gridNamePtr(gridptr->type));
  else
    {
      gridptr->laea_a = earth_radius;
      gridptr->laea_lon_0 = lon_0;
      gridptr->laea_lat_0 = lat_0;
      gridptr->laea_defined = TRUE;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}


void gridInqLaea(int gridID, double *earth_radius, double *lon_0, double *lat_0)
{
  grid_t* gridptr = gridID2Ptr(gridID);

  if ( gridptr->type != GRID_LAEA )
    Warning("Inquire of LAEA grid definition for %s grid not allowed!",
            gridNamePtr(gridptr->type));
  else
    {
      if ( gridptr->laea_defined )
        {
          *earth_radius = gridptr->laea_a;
          *lon_0 = gridptr->laea_lon_0;
          *lat_0 = gridptr->laea_lat_0;
        }
      else
        Warning("LAEA grid undefined (gridID = %d)", gridID);
    }
}


void gridDefComplexPacking(int gridID, int lcomplex)
{
  grid_t *gridptr = gridID2Ptr(gridID);


  if (gridptr->lcomplex != lcomplex)
    {
      gridptr->lcomplex = lcomplex;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}


int gridInqComplexPacking(int gridID)
{
  grid_t* gridptr = gridID2Ptr(gridID);

  return (gridptr->lcomplex);
}


void gridDefHasDims(int gridID, int hasdims)
{
  grid_t* gridptr = gridID2Ptr(gridID);

  if (gridptr->hasdims != hasdims)
    {
      gridptr->hasdims = hasdims;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}


int gridInqHasDims(int gridID)
{
  grid_t* gridptr = gridID2Ptr(gridID);

  return (gridptr->hasdims);
}
void gridDefNumber(int gridID, const int number)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  if (gridptr->number != number)
    {
      gridptr->number = number;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}
int gridInqNumber(int gridID)
{
  grid_t* gridptr = gridID2Ptr(gridID);

  return (gridptr->number);
}
void gridDefPosition(int gridID, int position)
{
  grid_t* gridptr = gridID2Ptr(gridID);

  if (gridptr->position != position)
    {
      gridptr->position = position;
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}
int gridInqPosition(int gridID)
{
  grid_t *gridptr = gridID2Ptr(gridID);

  return (gridptr->position);
}
void gridDefReference(int gridID, const char *reference)
{
  grid_t* gridptr = gridID2Ptr(gridID);

  if ( reference )
    {
      if ( gridptr->reference )
        {
          Free(gridptr->reference);
          gridptr->reference = NULL;
        }

      gridptr->reference = strdupx(reference);
      reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
    }
}
int gridInqReference(int gridID, char *reference)
{
  int len = 0;
  grid_t* gridptr = gridID2Ptr(gridID);

  if ( gridptr->reference && reference )
    {
      strcpy(reference, gridptr->reference);
    }

  return (len);
}
void gridDefUUID(int gridID, const unsigned char uuid[CDI_UUID_SIZE])
{
  grid_t* gridptr = gridID2Ptr(gridID);

  memcpy(gridptr->uuid, uuid, CDI_UUID_SIZE);
  reshSetStatus(gridID, &gridOps, RESH_DESYNC_IN_USE);
}
void gridInqUUID(int gridID, unsigned char uuid[CDI_UUID_SIZE])
{
  grid_t *gridptr = gridID2Ptr(gridID);

  memcpy(uuid, gridptr->uuid, CDI_UUID_SIZE);
}


void cdiGridGetIndexList(unsigned ngrids, int * gridIndexList)
{
  reshGetResHListOfType(ngrids, gridIndexList, &gridOps);
}


static int
gridTxCode ()
{
  return GRID;
}

enum { gridNint = 26,
       gridNdouble = 24,
       gridHasMaskFlag = 1 << 0,
       gridHasGMEMaskFlag = 1 << 1,
       gridHasXValsFlag = 1 << 2,
       gridHasYValsFlag = 1 << 3,
       gridHasAreaFlag = 1 << 4,
       gridHasXBoundsFlag = 1 << 5,
       gridHasYBoundsFlag = 1 << 6,
       gridHasReferenceFlag = 1 << 7,
       gridHasRowLonFlag = 1 << 8,
       gridHasUUIDFlag = 1 << 9,
};


static int gridGetComponentFlags(const grid_t * gridP)
{
  int flags = (gridHasMaskFlag & (int)((unsigned)(gridP->mask == NULL) - 1U))
    | (gridHasGMEMaskFlag & (int)((unsigned)(gridP->mask_gme == NULL) - 1U))
    | (gridHasXValsFlag & (int)((unsigned)(gridP->xvals == NULL) - 1U))
    | (gridHasYValsFlag & (int)((unsigned)(gridP->yvals == NULL) - 1U))
    | (gridHasAreaFlag & (int)((unsigned)(gridP->area == NULL) - 1U))
    | (gridHasXBoundsFlag & (int)((unsigned)(gridP->xbounds == NULL) - 1U))
    | (gridHasYBoundsFlag & (int)((unsigned)(gridP->ybounds == NULL) - 1U))
    | (gridHasReferenceFlag & (int)((unsigned)(gridP->reference == NULL) - 1U))
    | (gridHasRowLonFlag & (int)((unsigned)(gridP->rowlon == NULL) - 1U))
    | (gridHasUUIDFlag & (int)((unsigned)cdiUUIDIsNull(gridP->uuid) - 1U));
  return flags;
}


#define GRID_STR_SERIALIZE { gridP->xname, gridP->yname, \
    gridP->xlongname, gridP->ylongname, \
    gridP->xstdname, gridP->ystdname, \
    gridP->xunits, gridP->yunits }

static int
gridGetPackSize(void * voidP, void *context)
{
  grid_t * gridP = ( grid_t * ) voidP;
  int packBuffSize = 0, count;

  packBuffSize += serializeGetSize(gridNint, DATATYPE_INT, context)
    + serializeGetSize(1, DATATYPE_UINT32, context);

  if (gridP->rowlon)
    {
      xassert(gridP->nrowlon);
      packBuffSize += serializeGetSize(gridP->nrowlon, DATATYPE_INT, context)
        + serializeGetSize( 1, DATATYPE_UINT32, context);
    }

  packBuffSize += serializeGetSize(gridNdouble, DATATYPE_FLT64, context);

  if (gridP->xvals)
    {
      if (gridP->type == GRID_UNSTRUCTURED || gridP->type == GRID_CURVILINEAR)
        count = gridP->size;
      else
        count = gridP->xsize;
      xassert(count);
      packBuffSize += serializeGetSize(count, DATATYPE_FLT64, context)
        + serializeGetSize(1, DATATYPE_UINT32, context);
    }

  if (gridP->yvals)
    {
      if (gridP->type == GRID_UNSTRUCTURED || gridP->type == GRID_CURVILINEAR)
        count = gridP->size;
      else
        count = gridP->ysize;
      xassert(count);
      packBuffSize += serializeGetSize(count, DATATYPE_FLT64, context)
        + serializeGetSize(1, DATATYPE_UINT32, context);
    }

  if (gridP->area)
    {
      xassert(gridP->size);
      packBuffSize +=
        serializeGetSize(gridP->size, DATATYPE_FLT64, context)
        + serializeGetSize(1, DATATYPE_UINT32, context);
    }

  if (gridP->xbounds)
    {
      xassert(gridP->nvertex);
      if (gridP->type == GRID_CURVILINEAR || gridP->type == GRID_UNSTRUCTURED)
        count = gridP->size;
      else
        count = gridP->xsize;
      xassert(count);
      packBuffSize
        += (serializeGetSize(gridP->nvertex * count, DATATYPE_FLT64, context)
            + serializeGetSize(1, DATATYPE_UINT32, context));
    }

  if (gridP->ybounds)
    {
      xassert(gridP->nvertex);
      if (gridP->type == GRID_CURVILINEAR || gridP->type == GRID_UNSTRUCTURED)
        count = gridP->size;
      else
        count = gridP->ysize;
      xassert(count);
      packBuffSize
        += (serializeGetSize(gridP->nvertex * count, DATATYPE_FLT64, context)
            + serializeGetSize(1, DATATYPE_UINT32, context));
    }

  {
    const char *strTab[] = GRID_STR_SERIALIZE;
    int numStr = (int)(sizeof (strTab) / sizeof (strTab[0]));
    packBuffSize
      += serializeStrTabGetPackSize(strTab, numStr, context);
  }

  if (gridP->reference)
    {
      size_t len = strlen(gridP->reference);
      packBuffSize += serializeGetSize(1, DATATYPE_INT, context)
        + serializeGetSize((int)len + 1, DATATYPE_TXT, context)
        + serializeGetSize(1, DATATYPE_UINT32, context);
    }

  if (gridP->mask)
    {
      xassert(gridP->size);
      packBuffSize
        += serializeGetSize(gridP->size, DATATYPE_UCHAR, context)
        + serializeGetSize(1, DATATYPE_UINT32, context);
    }

  if (gridP->mask_gme)
    {
      xassert(gridP->size);
      packBuffSize += serializeGetSize(gridP->size, DATATYPE_UCHAR, context)
        + serializeGetSize(1, DATATYPE_UINT32, context);
    }

  if (!cdiUUIDIsNull(gridP->uuid))
    packBuffSize += serializeGetSize(CDI_UUID_SIZE, DATATYPE_UCHAR, context);

  return packBuffSize;
}

void
gridUnpack(char * unpackBuffer, int unpackBufferSize,
           int * unpackBufferPos, int originNamespace, void *context,
           int force_id)
{
  grid_t * gridP;
  uint32_t d;
  int memberMask, size;

  gridInit();

  {
    int intBuffer[gridNint];
    serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                    intBuffer, gridNint, DATATYPE_INT, context);
    serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                    &d, 1, DATATYPE_UINT32, context);

    xassert(cdiCheckSum(DATATYPE_INT, gridNint, intBuffer) == d);
    int targetID = namespaceAdaptKey(intBuffer[0], originNamespace);
    gridP = gridNewEntry(force_id?targetID:CDI_UNDEFID);

    xassert(!force_id || targetID == gridP->self);

    gridP->type = intBuffer[1];
    gridP->prec = intBuffer[2];
    gridP->lcc_projflag = intBuffer[3];
    gridP->lcc_scanflag = intBuffer[4];
    gridP->lcc_defined = (short)intBuffer[5];
    gridP->lcc2_defined = (short)intBuffer[6];
    gridP->laea_defined = intBuffer[7];
    gridP->isCyclic = (short)intBuffer[8];
    gridP->isRotated = (short)intBuffer[9];
    gridP->xdef = (short)intBuffer[10];
    gridP->ydef = (short)intBuffer[11];
    gridP->nd = intBuffer[12];
    gridP->ni = intBuffer[13];
    gridP->ni2 = intBuffer[14];
    gridP->ni3 = intBuffer[15];
    gridP->number = intBuffer[16];
    gridP->position = intBuffer[17];
    gridP->trunc = intBuffer[18];
    gridP->nvertex = intBuffer[19];
    gridP->nrowlon = intBuffer[20];
    gridP->size = intBuffer[21];
    gridP->xsize = intBuffer[22];
    gridP->ysize = intBuffer[23];
    gridP->lcomplex = intBuffer[24];
    memberMask = intBuffer[25];
  }

  if (memberMask & gridHasRowLonFlag)
    {
      xassert(gridP->nrowlon);
      gridP->rowlon = (int *) Malloc((size_t)gridP->nrowlon * sizeof (int));
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      gridP->rowlon, gridP->nrowlon , DATATYPE_INT, context);
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      &d, 1, DATATYPE_UINT32, context);
      xassert(cdiCheckSum(DATATYPE_INT, gridP->nrowlon, gridP->rowlon) == d);
    }

  {
    double doubleBuffer[gridNdouble];
    serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                    doubleBuffer, gridNdouble, DATATYPE_FLT64, context);
    serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                    &d, 1, DATATYPE_UINT32, context);
    xassert(d == cdiCheckSum(DATATYPE_FLT, gridNdouble, doubleBuffer));

    gridP->xfirst = doubleBuffer[0];
    gridP->yfirst = doubleBuffer[1];
    gridP->xlast = doubleBuffer[2];
    gridP->ylast = doubleBuffer[3];
    gridP->xinc = doubleBuffer[4];
    gridP->yinc = doubleBuffer[5];
    gridP->lcc_originLon = doubleBuffer[6];
    gridP->lcc_originLat = doubleBuffer[7];
    gridP->lcc_lonParY = doubleBuffer[8];
    gridP->lcc_lat1 = doubleBuffer[9];
    gridP->lcc_lat2 = doubleBuffer[10];
    gridP->lcc_xinc = doubleBuffer[11];
    gridP->lcc_yinc = doubleBuffer[12];
    gridP->lcc2_lon_0 = doubleBuffer[13];
    gridP->lcc2_lat_0 = doubleBuffer[14];
    gridP->lcc2_lat_1 = doubleBuffer[15];
    gridP->lcc2_lat_2 = doubleBuffer[16];
    gridP->lcc2_a = doubleBuffer[17];
    gridP->laea_lon_0 = doubleBuffer[18];
    gridP->laea_lat_0 = doubleBuffer[19];
    gridP->laea_a = doubleBuffer[20];
    gridP->xpole = doubleBuffer[21];
    gridP->ypole = doubleBuffer[22];
    gridP->angle = doubleBuffer[23];
  }

  int irregular = gridP->type == GRID_UNSTRUCTURED
    || gridP->type == GRID_CURVILINEAR;
  if (memberMask & gridHasXValsFlag)
    {
      size = irregular ? gridP->size : gridP->xsize;

      gridP->xvals = (double *) Malloc((size_t)size * sizeof (double));
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      gridP->xvals, size, DATATYPE_FLT64, context);
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      &d, 1, DATATYPE_UINT32, context);
      xassert(cdiCheckSum(DATATYPE_FLT, size, gridP->xvals) == d );
    }

  if (memberMask & gridHasYValsFlag)
    {
      size = irregular ? gridP->size : gridP->ysize;

      gridP->yvals = (double *) Malloc((size_t)size * sizeof (double));
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      gridP->yvals, size, DATATYPE_FLT64, context);
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      &d, 1, DATATYPE_UINT32, context);
      xassert(cdiCheckSum(DATATYPE_FLT, size, gridP->yvals) == d);
    }

  if (memberMask & gridHasAreaFlag)
    {
      size = gridP->size;
      xassert(size);
      gridP->area = (double *) Malloc((size_t)size * sizeof (double));
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      gridP->area, size, DATATYPE_FLT64, context);
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      &d, 1, DATATYPE_UINT32, context);
      xassert(cdiCheckSum(DATATYPE_FLT, size, gridP->area) == d);
    }

  if (memberMask & gridHasXBoundsFlag)
    {
      size = gridP->nvertex * (irregular ? gridP->size : gridP->xsize);
      xassert(size);

      gridP->xbounds = (double *) Malloc((size_t)size * sizeof (double));
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      gridP->xbounds, size, DATATYPE_FLT64, context);
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      &d, 1, DATATYPE_UINT32, context);
      xassert(cdiCheckSum(DATATYPE_FLT, size, gridP->xbounds) == d);
    }

  if (memberMask & gridHasYBoundsFlag)
    {
      size = gridP->nvertex * (irregular ? gridP->size : gridP->ysize);
      xassert(size);

      gridP->ybounds = (double *) Malloc((size_t)size * sizeof (double));
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                          gridP->ybounds, size, DATATYPE_FLT64, context);
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      &d, 1, DATATYPE_UINT32, context);
      xassert(cdiCheckSum(DATATYPE_FLT, size, gridP->ybounds) == d);
    }

  {
    char *strTab[] = GRID_STR_SERIALIZE;
    int numStr = sizeof (strTab) / sizeof (strTab[0]);
    serializeStrTabUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                          strTab, numStr, context);
  }

  if (memberMask & gridHasReferenceFlag)
    {
      int referenceSize;
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      &referenceSize, 1, DATATYPE_INT, context);
      gridP->reference = (char *) Malloc((size_t)referenceSize);
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      gridP->reference, referenceSize, DATATYPE_TXT, context);
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      &d, 1, DATATYPE_UINT32, context);
      xassert(cdiCheckSum(DATATYPE_TXT, referenceSize, gridP->reference) == d);
    }

  if (memberMask & gridHasMaskFlag)
    {
      xassert((size = gridP->size));
      gridP->mask = (mask_t *) Malloc((size_t)size * sizeof (mask_t));
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      gridP->mask, gridP->size, DATATYPE_UCHAR, context);
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      &d, 1, DATATYPE_UINT32, context);
      xassert(cdiCheckSum(DATATYPE_UCHAR, gridP->size, gridP->mask) == d);
    }

  if (memberMask & gridHasGMEMaskFlag)
    {
      xassert((size = gridP->size));
      gridP->mask_gme = (mask_t *) Malloc((size_t)size * sizeof (mask_t));
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      gridP->mask_gme, gridP->size, DATATYPE_UCHAR, context);
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      &d, 1, DATATYPE_UINT32, context);
      xassert(cdiCheckSum(DATATYPE_UCHAR, gridP->size, gridP->mask_gme) == d);
    }
  if (memberMask & gridHasUUIDFlag)
    {
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      gridP->uuid, CDI_UUID_SIZE, DATATYPE_UCHAR, context);
    }

  reshSetStatus(gridP->self, &gridOps,
                reshGetStatus(gridP->self, &gridOps) & ~RESH_SYNC_BIT);
}


static void
gridPack(void * voidP, void * packBuffer, int packBufferSize,
         int * packBufferPos, void *context)
{
  grid_t * gridP = ( grid_t * ) voidP;
  int size;
  uint32_t d;
  int memberMask;

  {
    int intBuffer[gridNint];

    intBuffer[0] = gridP->self;
    intBuffer[1] = gridP->type;
    intBuffer[2] = gridP->prec;
    intBuffer[3] = gridP->lcc_projflag;
    intBuffer[4] = gridP->lcc_scanflag;
    intBuffer[5] = gridP->lcc_defined;
    intBuffer[6] = gridP->lcc2_defined;
    intBuffer[7] = gridP->laea_defined;
    intBuffer[8] = gridP->isCyclic;
    intBuffer[9] = gridP->isRotated;
    intBuffer[10] = gridP->xdef;
    intBuffer[11] = gridP->ydef;
    intBuffer[12] = gridP->nd;
    intBuffer[13] = gridP->ni;
    intBuffer[14] = gridP->ni2;
    intBuffer[15] = gridP->ni3;
    intBuffer[16] = gridP->number;
    intBuffer[17] = gridP->position;
    intBuffer[18] = gridP->trunc;
    intBuffer[19] = gridP->nvertex;
    intBuffer[20] = gridP->nrowlon;
    intBuffer[21] = gridP->size;
    intBuffer[22] = gridP->xsize;
    intBuffer[23] = gridP->ysize;
    intBuffer[24] = gridP->lcomplex;
    intBuffer[25] = memberMask = gridGetComponentFlags(gridP);

    serializePack(intBuffer, gridNint, DATATYPE_INT,
                  packBuffer, packBufferSize, packBufferPos, context);
    d = cdiCheckSum(DATATYPE_INT, gridNint, intBuffer);
    serializePack(&d, 1, DATATYPE_UINT32,
                  packBuffer, packBufferSize, packBufferPos, context);
  }

  if (memberMask & gridHasRowLonFlag)
    {
      size = gridP->nrowlon;
      xassert(size > 0);
      serializePack(gridP->rowlon, size, DATATYPE_INT,
                    packBuffer, packBufferSize, packBufferPos, context);
      d = cdiCheckSum(DATATYPE_INT , size, gridP->rowlon);
      serializePack(&d, 1, DATATYPE_UINT32,
                    packBuffer, packBufferSize, packBufferPos, context);
    }

  {
    double doubleBuffer[gridNdouble];

    doubleBuffer[0] = gridP->xfirst;
    doubleBuffer[1] = gridP->yfirst;
    doubleBuffer[2] = gridP->xlast;
    doubleBuffer[3] = gridP->ylast;
    doubleBuffer[4] = gridP->xinc;
    doubleBuffer[5] = gridP->yinc;
    doubleBuffer[6] = gridP->lcc_originLon;
    doubleBuffer[7] = gridP->lcc_originLat;
    doubleBuffer[8] = gridP->lcc_lonParY;
    doubleBuffer[9] = gridP->lcc_lat1;
    doubleBuffer[10] = gridP->lcc_lat2;
    doubleBuffer[11] = gridP->lcc_xinc;
    doubleBuffer[12] = gridP->lcc_yinc;
    doubleBuffer[13] = gridP->lcc2_lon_0;
    doubleBuffer[14] = gridP->lcc2_lat_0;
    doubleBuffer[15] = gridP->lcc2_lat_1;
    doubleBuffer[16] = gridP->lcc2_lat_2;
    doubleBuffer[17] = gridP->lcc2_a;
    doubleBuffer[18] = gridP->laea_lon_0;
    doubleBuffer[19] = gridP->laea_lat_0;
    doubleBuffer[20] = gridP->laea_a;
    doubleBuffer[21] = gridP->xpole;
    doubleBuffer[22] = gridP->ypole;
    doubleBuffer[23] = gridP->angle;

    serializePack(doubleBuffer, gridNdouble, DATATYPE_FLT64,
                  packBuffer, packBufferSize, packBufferPos, context);
    d = cdiCheckSum(DATATYPE_FLT, gridNdouble, doubleBuffer);
    serializePack(&d, 1, DATATYPE_UINT32,
                  packBuffer, packBufferSize, packBufferPos, context);
  }

  if (memberMask & gridHasXValsFlag)
    {
      if (gridP->type == GRID_UNSTRUCTURED || gridP->type == GRID_CURVILINEAR)
        size = gridP->size;
      else
        size = gridP->xsize;
      xassert(size);

      serializePack(gridP->xvals, size, DATATYPE_FLT64,
                    packBuffer, packBufferSize, packBufferPos, context);
      d = cdiCheckSum(DATATYPE_FLT, size, gridP->xvals);
      serializePack(&d, 1, DATATYPE_UINT32,
                    packBuffer, packBufferSize, packBufferPos, context);
    }

  if (memberMask & gridHasYValsFlag)
    {
      if (gridP->type == GRID_UNSTRUCTURED || gridP->type == GRID_CURVILINEAR )
        size = gridP->size;
      else
        size = gridP->ysize;
      xassert(size);
      serializePack(gridP->yvals, size, DATATYPE_FLT64,
                    packBuffer, packBufferSize, packBufferPos, context);
      d = cdiCheckSum(DATATYPE_FLT, size, gridP->yvals);
      serializePack(&d, 1, DATATYPE_UINT32,
                    packBuffer, packBufferSize, packBufferPos, context);
    }

  if (memberMask & gridHasAreaFlag)
    {
      xassert(gridP->size);

      serializePack(gridP->area, gridP->size, DATATYPE_FLT64,
                    packBuffer, packBufferSize, packBufferPos, context);
      d = cdiCheckSum(DATATYPE_FLT, gridP->size, gridP->area);
      serializePack(&d, 1, DATATYPE_UINT32,
                    packBuffer, packBufferSize, packBufferPos, context);
    }

  if (memberMask & gridHasXBoundsFlag)
    {
      xassert ( gridP->nvertex );
      if (gridP->type == GRID_CURVILINEAR || gridP->type == GRID_UNSTRUCTURED)
        size = gridP->nvertex * gridP->size;
      else
        size = gridP->nvertex * gridP->xsize;
      xassert ( size );

      serializePack(gridP->xbounds, size, DATATYPE_FLT64,
                    packBuffer, packBufferSize, packBufferPos, context);
      d = cdiCheckSum(DATATYPE_FLT, size, gridP->xbounds);
      serializePack(&d, 1, DATATYPE_UINT32,
                    packBuffer, packBufferSize, packBufferPos, context);
    }

  if (memberMask & gridHasYBoundsFlag)
    {
      xassert(gridP->nvertex);
      if (gridP->type == GRID_CURVILINEAR || gridP->type == GRID_UNSTRUCTURED)
        size = gridP->nvertex * gridP->size;
      else
        size = gridP->nvertex * gridP->ysize;
      xassert ( size );

      serializePack(gridP->ybounds, size, DATATYPE_FLT64,
                    packBuffer, packBufferSize, packBufferPos, context);
      d = cdiCheckSum(DATATYPE_FLT, size, gridP->ybounds);
      serializePack(&d, 1, DATATYPE_UINT32,
                    packBuffer, packBufferSize, packBufferPos, context);
    }

  {
    const char *strTab[] = GRID_STR_SERIALIZE;
    int numStr = sizeof (strTab) / sizeof (strTab[0]);
    serializeStrTabPack(strTab, numStr,
                        packBuffer, packBufferSize, packBufferPos, context);
  }

  if (memberMask & gridHasReferenceFlag)
    {
      size = (int)strlen(gridP->reference) + 1;
      serializePack(&size, 1, DATATYPE_INT,
                    packBuffer, packBufferSize, packBufferPos, context);
      serializePack(gridP->reference, size, DATATYPE_TXT,
                    packBuffer, packBufferSize, packBufferPos, context);
      d = cdiCheckSum(DATATYPE_TXT, size, gridP->reference);
      serializePack(&d, 1, DATATYPE_UINT32,
                    packBuffer, packBufferSize, packBufferPos, context);
    }

  if (memberMask & gridHasMaskFlag)
    {
      xassert((size = gridP->size));
      serializePack(gridP->mask, size, DATATYPE_UCHAR,
                    packBuffer, packBufferSize, packBufferPos, context);
      d = cdiCheckSum(DATATYPE_UCHAR, size, gridP->mask);
      serializePack(&d, 1, DATATYPE_UINT32,
                    packBuffer, packBufferSize, packBufferPos, context);
    }

  if (memberMask & gridHasGMEMaskFlag)
    {
      xassert((size = gridP->size));

      serializePack(gridP->mask_gme, size, DATATYPE_UCHAR,
                    packBuffer, packBufferSize, packBufferPos, context);
      d = cdiCheckSum(DATATYPE_UCHAR, size, gridP->mask_gme);
      serializePack(&d, 1, DATATYPE_UINT32,
                    packBuffer, packBufferSize, packBufferPos, context);
    }

  if (memberMask & gridHasUUIDFlag)
    serializePack(gridP->uuid, CDI_UUID_SIZE, DATATYPE_UCHAR,
                  packBuffer, packBufferSize, packBufferPos, context);
}

#undef GRID_STR_SERIALIZE


struct varDefGridSearchState
{
  int resIDValue;
  const grid_t *queryKey;
};

static enum cdiApplyRet
varDefGridSearch(int id, void *res, void *data)
{
  struct varDefGridSearchState *state = (struct varDefGridSearchState*)data;
  (void)res;
  if (gridCompare(id, state->queryKey) == 0)
    {
      state->resIDValue = id;
      return CDI_APPLY_STOP;
    }
  else
    return CDI_APPLY_GO_ON;
}

int varDefGrid(int vlistID, const grid_t *grid, int mode)
{




  int gridglobdefined = FALSE;
  int griddefined;
  int gridID = CDI_UNDEFID;
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  griddefined = FALSE;
  unsigned ngrids = (unsigned)vlistptr->ngrids;

  if ( mode == 0 )
    for (unsigned index = 0; index < ngrids; index++ )
      {
        gridID = vlistptr->gridIDs[index];
        if ( gridID == UNDEFID )
          Error("Internal problem: undefined gridID %d!", gridID);

        if ( gridCompare(gridID, grid) == 0 )
          {
            griddefined = TRUE;
            break;
          }
      }

  if ( ! griddefined )
    {
      struct varDefGridSearchState query;
      query.queryKey = grid;
      if ((gridglobdefined
           = (cdiResHFilterApply(&gridOps, varDefGridSearch, &query)
              == CDI_APPLY_STOP)))
        gridID = query.resIDValue;

      if ( mode == 1 && gridglobdefined)
        for (unsigned index = 0; index < ngrids; index++ )
          if ( vlistptr->gridIDs[index] == gridID )
            {
              gridglobdefined = FALSE;
              break;
            }
    }

  if ( ! griddefined )
    {
      if ( ! gridglobdefined ) gridID = gridGenerate(grid);
      ngrids = (unsigned)vlistptr->ngrids;
      vlistptr->gridIDs[ngrids] = gridID;
      vlistptr->ngrids++;
    }

  return (gridID);
}
#ifndef INSTITUTION_H
#define INSTITUTION_H

int
instituteUnpack(void *buf, int size, int *position, int originNamespace,
                void *context, int force_id);

void instituteDefaultEntries(void);

#endif
#if defined (HAVE_CONFIG_H)
#endif

#include <assert.h>
#include <limits.h>


#undef UNDEFID
#define UNDEFID -1

static int ECMWF = UNDEFID,
  MPIMET = UNDEFID,
  MCH = UNDEFID;

typedef struct
{
  int self;
  int used;
  int center;
  int subcenter;
  char *name;
  char *longname;
}
institute_t;


static int instituteCompareKernel(institute_t *ip1, institute_t *ip2);
static void instituteDestroyP(institute_t *instituteptr);
static void institutePrintP(institute_t *instituteptr, FILE * fp);
static int instituteGetPackSize(institute_t *instituteptr, void *context);
static void institutePackP ( void * instituteptr, void *buf, int size, int *position, void *context );
static int instituteTxCode ( void );

static const resOps instituteOps = {
  (int (*)(void *, void *))instituteCompareKernel,
  (void (*)(void *))instituteDestroyP,
  (void (*)(void *, FILE *))institutePrintP,
  (int (*)(void *, void *))instituteGetPackSize,
  institutePackP,
  instituteTxCode
};

static
void instituteDefaultValue ( institute_t * instituteptr )
{
  instituteptr->self = UNDEFID;
  instituteptr->used = 0;
  instituteptr->center = UNDEFID;
  instituteptr->subcenter = UNDEFID;
  instituteptr->name = NULL;
  instituteptr->longname = NULL;
}

void instituteDefaultEntries ( void )
{
  cdiResH resH[]
    = { ECMWF = institutDef( 98, 0, "ECMWF", "European Centre for Medium-Range Weather Forecasts"),
        MPIMET = institutDef( 98, 232, "MPIMET", "Max-Planck-Institute for Meteorology"),
        institutDef( 98, 255, "MPIMET", "Max-Planck-Institute for Meteorology"),
        institutDef( 98, 232, "MPIMET", "Max-Planck Institute for Meteorology"),
        institutDef( 78, 0, "DWD", "Deutscher Wetterdienst"),
        institutDef( 78, 255, "DWD", "Deutscher Wetterdienst"),
        MCH = institutDef(215, 255, "MCH", "MeteoSwiss"),
        institutDef( 7, 0, "NCEP", "National Centers for Environmental Prediction"),
        institutDef( 7, 1, "NCEP", "National Centers for Environmental Prediction"),
        institutDef( 60, 0, "NCAR", "National Center for Atmospheric Research"),
        institutDef( 74, 0, "METOFFICE", "U.K. Met Office"),
        institutDef( 97, 0, "ESA", "European Space Agency"),
        institutDef( 99, 0, "KNMI", "Royal Netherlands Meteorological Institute"),
  };


  size_t n = sizeof(resH)/sizeof(*resH);

  for (size_t i = 0; i < n ; i++ )
    reshSetStatus(resH[i], &instituteOps, RESH_IN_USE);
}


static int
instituteCompareKernel(institute_t * ip1, institute_t * ip2)
{
  int differ = 0;
  size_t len1, len2;

  if ( ip1->name )
    {
      if ( ip1->center > 0 && ip2->center != ip1->center ) differ = 1;
      if ( ip1->subcenter > 0 && ip2->subcenter != ip1->subcenter ) differ = 1;

      if ( !differ )
        {
          if ( ip2->name )
            {
              len1 = strlen(ip1->name);
              len2 = strlen(ip2->name);
              if ( (len1 != len2) || memcmp(ip2->name, ip1->name, len2) ) differ = 1;
            }
        }
    }
  else if ( ip1->longname )
    {
      if ( ip2->longname )
        {
          len1 = strlen(ip1->longname);
          len2 = strlen(ip2->longname);
          if ( (len1 < len2) || memcmp(ip2->longname, ip1->longname, len2) ) differ = 1;
        }
    }
  else
    {
      if ( !( ip2->center == ip1->center &&
              ip2->subcenter == ip1->subcenter )) differ = 1;
    }

  return differ;
}


struct instLoc
{
  institute_t *ip;
  int id;
};

static enum cdiApplyRet
findInstitute(int id, void *res, void *data)
{
  institute_t * ip1 = ((struct instLoc *)data)->ip;
  institute_t * ip2 = (institute_t*) res;
  if (ip2->used && !instituteCompareKernel(ip1, ip2))
    {
      ((struct instLoc *)data)->id = id;
      return CDI_APPLY_STOP;
    }
  else
    return CDI_APPLY_GO_ON;
}


int institutInq(int center, int subcenter, const char *name, const char *longname)
{
  institute_t * ip_ref = (institute_t *) Malloc(sizeof (*ip_ref));
  ip_ref->self = UNDEFID;
  ip_ref->used = 0;
  ip_ref->center = center;
  ip_ref->subcenter = subcenter;
  ip_ref->name = name && name[0] ? (char *)name : NULL;
  ip_ref->longname = longname && longname[0] ? (char *)longname : NULL;

  struct instLoc state = { .ip = ip_ref, .id = UNDEFID };
  cdiResHFilterApply(&instituteOps, findInstitute, &state);

  Free(ip_ref);

  return state.id;
}

static
institute_t *instituteNewEntry(cdiResH resH, int center, int subcenter,
                               const char *name, const char *longname)
{
  institute_t *instituteptr = (institute_t*) Malloc(sizeof(institute_t));
  instituteDefaultValue(instituteptr);
  if (resH == CDI_UNDEFID)
    instituteptr->self = reshPut(instituteptr, &instituteOps);
  else
    {
      instituteptr->self = resH;
      reshReplace(resH, instituteptr, &instituteOps);
    }
  instituteptr->used = 1;
  instituteptr->center = center;
  instituteptr->subcenter = subcenter;
  if ( name && *name )
    instituteptr->name = strdupx(name);
  if (longname && *longname)
    instituteptr->longname = strdupx(longname);
  return instituteptr;
}


int institutDef(int center, int subcenter, const char *name, const char *longname)
{
  institute_t * instituteptr
    = instituteNewEntry(CDI_UNDEFID, center, subcenter, name, longname);
  return instituteptr->self;
}


int institutInqCenter(int instID)
{
  institute_t * instituteptr = NULL;

  if ( instID != UNDEFID )
    instituteptr = ( institute_t * ) reshGetVal ( instID, &instituteOps );

  return instituteptr ? instituteptr->center : UNDEFID;
}


int institutInqSubcenter(int instID)
{
  institute_t * instituteptr = NULL;

  if ( instID != UNDEFID )
    instituteptr = ( institute_t * ) reshGetVal ( instID, &instituteOps );

  return instituteptr ? instituteptr->subcenter: UNDEFID;
}


const char *institutInqNamePtr(int instID)
{
  institute_t * instituteptr = NULL;

  if ( instID != UNDEFID )
    instituteptr = ( institute_t * ) reshGetVal ( instID, &instituteOps );

  return instituteptr ? instituteptr->name : NULL;
}


const char *institutInqLongnamePtr(int instID)
{
  institute_t * instituteptr = NULL;

  if ( instID != UNDEFID )
    instituteptr = ( institute_t * ) reshGetVal ( instID, &instituteOps );

  return instituteptr ? instituteptr->longname : NULL;
}

static enum cdiApplyRet
activeInstitutes(int id, void *res, void *data)
{
  (void)id;
  if (res && ((institute_t *)res)->used)
    ++(*(int *)data);
  return CDI_APPLY_GO_ON;
}

int institutInqNumber(void)
{
  int instNum = 0;

  cdiResHFilterApply(&instituteOps, activeInstitutes, &instNum);
  return instNum;
}


static void
instituteDestroyP(institute_t *instituteptr)
{
  xassert(instituteptr);

  int instituteID = instituteptr->self;
  Free(instituteptr->name);
  Free(instituteptr->longname);
  reshRemove(instituteID, &instituteOps);
  Free(instituteptr);
}


static void institutePrintP(institute_t *ip, FILE * fp )
{
  if (ip)
    fprintf(fp, "#\n"
            "# instituteID %d\n"
            "#\n"
            "self          = %d\n"
            "used          = %d\n"
            "center        = %d\n"
            "subcenter     = %d\n"
            "name          = %s\n"
            "longname      = %s\n",
            ip->self, ip->self, ip->used, ip->center, ip->subcenter,
            ip->name ? ip->name : "NN",
            ip->longname ? ip->longname : "NN");
}


static int
instituteTxCode ( void )
{
  return INSTITUTE;
}

enum {
  institute_nints = 5,
};

static int instituteGetPackSize(institute_t *ip, void *context)
{
  size_t namelen = strlen(ip->name), longnamelen = strlen(ip->longname);
  xassert(namelen < INT_MAX && longnamelen < INT_MAX);
  size_t txsize = (size_t)serializeGetSize(institute_nints, DATATYPE_INT, context)
    + (size_t)serializeGetSize((int)namelen + 1, DATATYPE_TXT, context)
    + (size_t)serializeGetSize((int)longnamelen + 1, DATATYPE_TXT, context);
  xassert(txsize <= INT_MAX);
  return (int)txsize;
}

static void institutePackP(void * instituteptr, void *buf, int size, int *position, void *context)
{
  institute_t *p = (institute_t*) instituteptr;
  int tempbuf[institute_nints];
  tempbuf[0] = p->self;
  tempbuf[1] = p->center;
  tempbuf[2] = p->subcenter;
  tempbuf[3] = (int)strlen(p->name) + 1;
  tempbuf[4] = (int)strlen(p->longname) + 1;
  serializePack(tempbuf, institute_nints, DATATYPE_INT, buf, size, position, context);
  serializePack(p->name, tempbuf[3], DATATYPE_TXT, buf, size, position, context);
  serializePack(p->longname, tempbuf[4], DATATYPE_TXT, buf, size, position, context);
}

int instituteUnpack(void *buf, int size, int *position, int originNamespace,
                    void *context, int force_id)
{
  int tempbuf[institute_nints];
  int instituteID;
  char *name, *longname;
  serializeUnpack(buf, size, position, tempbuf, institute_nints, DATATYPE_INT, context);
  name = (char *) Malloc((size_t)tempbuf[3] + (size_t)tempbuf[4]);
  longname = name + tempbuf[3];
  serializeUnpack(buf, size, position, name, tempbuf[3], DATATYPE_TXT, context);
  serializeUnpack(buf, size, position, longname, tempbuf[4], DATATYPE_TXT, context);
  int targetID = namespaceAdaptKey(tempbuf[0], originNamespace);
  institute_t *ip = instituteNewEntry(force_id?targetID:CDI_UNDEFID,
                                      tempbuf[1], tempbuf[2], name, longname);
  instituteID = ip->self;
  xassert(!force_id || instituteID == targetID);
  Free(name);
  reshSetStatus(instituteID, &instituteOps,
                reshGetStatus(instituteID, &instituteOps) & ~RESH_SYNC_BIT);
  return instituteID;
}
#ifndef MODEL_H
#define MODEL_H

int
modelUnpack(void *buf, int size, int *position,
            int originNamespace, void *context, int force_id);

void modelDefaultEntries(void);

#endif
#if defined (HAVE_CONFIG_H)
#endif

#include <limits.h>


#undef UNDEFID
#define UNDEFID -1

static int ECHAM4 = UNDEFID,
  ECHAM5 = UNDEFID,
  COSMO = UNDEFID;

typedef struct
{
  int self;
  int used;
  int instID;
  int modelgribID;
  char *name;
}
model_t;


static int MODEL_Debug = 0;

static void modelInit(void);


static int modelCompareP(void *modelptr1, void *modelptr2);
static void modelDestroyP ( void * modelptr );
static void modelPrintP ( void * modelptr, FILE * fp );
static int modelGetSizeP ( void * modelptr, void *context);
static void modelPackP ( void * modelptr, void * buff, int size,
                              int *position, void *context);
static int modelTxCode ( void );

static const resOps modelOps = {
  modelCompareP,
  modelDestroyP,
  modelPrintP,
  modelGetSizeP,
  modelPackP,
  modelTxCode
};

static
void modelDefaultValue ( model_t *modelptr )
{
  modelptr->self = UNDEFID;
  modelptr->used = 0;
  modelptr->instID = UNDEFID;
  modelptr->modelgribID = UNDEFID;
  modelptr->name = NULL;
}

static model_t *
modelNewEntry(cdiResH resH, int instID, int modelgribID, const char *name)
{
  model_t *modelptr;

  modelptr = (model_t *) Malloc(sizeof(model_t));
  modelDefaultValue ( modelptr );
  if (resH == CDI_UNDEFID)
    modelptr->self = reshPut(modelptr, &modelOps);
  else
    {
      modelptr->self = resH;
      reshReplace(resH, modelptr, &modelOps);
    }
  modelptr->used = 1;
  modelptr->instID = instID;
  modelptr->modelgribID = modelgribID;
  if ( name && *name ) modelptr->name = strdupx(name);

  return (modelptr);
}

void modelDefaultEntries ( void )
{
  int instID, i;
  enum { nDefModels = 10 };
  cdiResH resH[nDefModels];

  instID = institutInq( 0, 0, "ECMWF", NULL);


  instID = institutInq( 0, 0, "MPIMET", NULL);

  resH[0] = ECHAM5 = modelDef(instID, 64, "ECHAM5.4");
  resH[1] = modelDef(instID, 63, "ECHAM5.3");
  resH[2] = modelDef(instID, 62, "ECHAM5.2");
  resH[3] = modelDef(instID, 61, "ECHAM5.1");

  instID = institutInq( 98, 255, "MPIMET", NULL);
  resH[4] = modelDef(instID, 60, "ECHAM5.0");
  resH[5] = ECHAM4 = modelDef(instID, 50, "ECHAM4");
  resH[6] = modelDef(instID, 110, "MPIOM1");

  instID = institutInq( 0, 0, "DWD", NULL);
  resH[7] = modelDef(instID, 149, "GME");

  instID = institutInq( 0, 0, "MCH", NULL);

  resH[8] = COSMO = modelDef(instID, 255, "COSMO");

  instID = institutInq( 0, 1, "NCEP", NULL);
  resH[9] = modelDef(instID, 80, "T62L28MRF");


  for ( i = 0; i < nDefModels ; i++ )
    reshSetStatus(resH[i], &modelOps, RESH_IN_USE);
}

static
void modelInit(void)
{
  static int modelInitialized = 0;

  if (modelInitialized) return;

  modelInitialized = 1;
  char *env = getenv("MODEL_DEBUG");
  if ( env ) MODEL_Debug = atoi(env);
}

struct modelLoc
{
  const char *name;
  int instID, modelgribID, resID;
};

static enum cdiApplyRet
findModelByID(int resID, void *res, void *data)
{
  model_t *modelptr = (model_t*) res;
  struct modelLoc *ret = (struct modelLoc*) data;
  int instID = ret->instID, modelgribID = ret->modelgribID;
  if (modelptr->used
      && modelptr->instID == instID
      && modelptr->modelgribID == modelgribID)
    {
      ret->resID = resID;
      return CDI_APPLY_STOP;
    }
  else
    return CDI_APPLY_GO_ON;
}

static enum cdiApplyRet
findModelByName(int resID, void *res, void *data)
{
  model_t *modelptr = (model_t*) res;
  struct modelLoc *ret = (struct modelLoc*) data;
  int instID = ret->instID, modelgribID = ret->modelgribID;
  const char *name = ret->name;
  if (modelptr->used
      && (instID == -1 || modelptr->instID == instID)
      && (modelgribID == 0 || modelptr->modelgribID == modelgribID)
      && modelptr->name)
    {
      const char *p = name, *q = modelptr->name;
      while (*p != '\0' && *p == *q)
        ++p, ++q;
      if (*p == '\0' || *q == '\0')
        {
          ret->resID = resID;
          return CDI_APPLY_STOP;
        }
    }
  return CDI_APPLY_GO_ON;
}

int modelInq(int instID, int modelgribID, const char *name)
{
  modelInit ();

  struct modelLoc searchState = { .name = name, .instID = instID,
                                  .modelgribID = modelgribID,
                                  .resID = UNDEFID };
  if (name && *name)
    cdiResHFilterApply(&modelOps, findModelByName, &searchState);
  else
    cdiResHFilterApply(&modelOps, findModelByID, &searchState);
  return searchState.resID;
}


int modelDef(int instID, int modelgribID, const char *name)
{
  model_t *modelptr;

  modelInit ();

  modelptr = modelNewEntry(CDI_UNDEFID, instID, modelgribID, name);

  return modelptr->self;
}


int modelInqInstitut(int modelID)
{
  model_t *modelptr = NULL;

  modelInit ();

  if ( modelID != UNDEFID )
    modelptr = ( model_t * ) reshGetVal ( modelID, &modelOps );

  return modelptr ? modelptr->instID : UNDEFID;
}


int modelInqGribID(int modelID)
{
  model_t *modelptr = NULL;

  modelInit ();

  if ( modelID != UNDEFID )
    modelptr = ( model_t * ) reshGetVal ( modelID, &modelOps );

  return modelptr ? modelptr->modelgribID : UNDEFID;
}


const char *modelInqNamePtr(int modelID)
{
  model_t *modelptr = NULL;

  modelInit ();

  if ( modelID != UNDEFID )
    modelptr = ( model_t * ) reshGetVal ( modelID, &modelOps );

  return modelptr ? modelptr->name : NULL;
}


static int
modelCompareP(void *modelptr1, void *modelptr2)
{
  model_t *model1 = (model_t *)modelptr1, *model2 = (model_t *)modelptr2;
  int diff = (namespaceResHDecode(model1->instID).idx
              != namespaceResHDecode(model2->instID).idx)
    | (model1->modelgribID != model2->modelgribID)
    | (strcmp(model1->name, model2->name) != 0);
  return diff;
}


void modelDestroyP ( void * modelptr )
{
  model_t *mp = (model_t*) modelptr;
  if (mp->name)
    Free(mp->name);
  Free(mp);
}


void modelPrintP ( void * modelptr, FILE * fp )
{
  model_t *mp = (model_t*) modelptr;

  if ( !mp ) return;

  fprintf ( fp, "#\n");
  fprintf ( fp, "# modelID %d\n", mp->self);
  fprintf ( fp, "#\n");
  fprintf ( fp, "self          = %d\n", mp->self );
  fprintf ( fp, "used          = %d\n", mp->used );
  fprintf ( fp, "instID        = %d\n", mp->instID );
  fprintf ( fp, "modelgribID   = %d\n", mp->modelgribID );
  fprintf ( fp, "name          = %s\n", mp->name ? mp->name : "NN" );
}


static int
modelTxCode ( void )
{
  return MODEL;
}

enum {
  model_nints = 4,
};


static int modelGetSizeP(void * modelptr, void *context)
{
  model_t *p = (model_t*)modelptr;
  size_t txsize = (size_t)serializeGetSize(model_nints, DATATYPE_INT, context)
    + (size_t)serializeGetSize(p->name?(int)strlen(p->name) + 1:0, DATATYPE_TXT, context);
  xassert(txsize <= INT_MAX);
  return (int)txsize;
}


static void modelPackP(void * modelptr, void * buf, int size, int *position, void *context)
{
  model_t *p = (model_t*) modelptr;
  int tempbuf[model_nints];
  tempbuf[0] = p->self;
  tempbuf[1] = p->instID;
  tempbuf[2] = p->modelgribID;
  tempbuf[3] = p->name ? (int)strlen(p->name) + 1 : 0;
  serializePack(tempbuf, model_nints, DATATYPE_INT, buf, size, position, context);
  if (p->name)
    serializePack(p->name, tempbuf[3], DATATYPE_TXT, buf, size, position, context);
}

int
modelUnpack(void *buf, int size, int *position, int originNamespace, void *context,
            int force_id)
{
  int tempbuf[model_nints];
  char *name;
  serializeUnpack(buf, size, position, tempbuf, model_nints, DATATYPE_INT, context);
  if (tempbuf[3] != 0)
    {
      name = (char *) Malloc((size_t)tempbuf[3]);
      serializeUnpack(buf, size, position,
                      name, tempbuf[3], DATATYPE_TXT, context);
    }
  else
    {
      name = (char*)"";
    }
  int targetID = namespaceAdaptKey(tempbuf[0], originNamespace);
  model_t *mp = modelNewEntry(force_id?targetID:CDI_UNDEFID,
                              namespaceAdaptKey(tempbuf[1], originNamespace),
                              tempbuf[2], name);
  if (tempbuf[3] != 0)
    Free(name);
  xassert(!force_id
          || (mp->self == namespaceAdaptKey(tempbuf[0], originNamespace)));
  reshSetStatus(mp->self, &modelOps,
                reshGetStatus(mp->self, &modelOps) & ~RESH_SYNC_BIT);
  return mp->self;
}
#if defined (HAVE_CONFIG_H)
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>


static unsigned nNamespaces = 1;
static int activeNamespace = 0;

#ifdef HAVE_LIBNETCDF
#define CDI_NETCDF_SWITCHES \
  { .func = (void (*)()) nc__create }, \
  { .func = (void (*)()) cdf_def_var_serial }, \
  { .func = (void (*)()) cdfDefTimestep }, \
  { .func = (void (*)()) cdfDefVars }

#else
#define CDI_NETCDF_SWITCHES
#endif

#define defaultSwitches { \
    { .func = (void (*)()) cdiAbortC_serial }, \
    { .func = (void (*)()) cdiWarning }, \
    { .func = (void (*)()) serializeGetSizeInCore }, \
    { .func = (void (*)()) serializePackInCore }, \
    { .func = (void (*)()) serializeUnpackInCore }, \
    { .func = (void (*)()) fileOpen_serial }, \
    { .func = (void (*)()) fileWrite }, \
    { .func = (void (*)()) fileClose_serial }, \
    { .func = (void (*)()) cdiStreamOpenDefaultDelegate }, \
    { .func = (void (*)()) cdiStreamDefVlist_ }, \
    { .func = (void (*)()) cdiStreamSetupVlist_ }, \
    { .func = (void (*)()) cdiStreamWriteVar_ }, \
    { .func = (void (*)()) cdiStreamWriteVarChunk_ }, \
    { .func = (void (*)()) 0 }, \
    { .func = (void (*)()) 0 }, \
    { .func = (void (*)()) cdiStreamCloseDefaultDelegate }, \
    { .func = (void (*)()) cdiStreamDefTimestep_ }, \
    { .func = (void (*)()) cdiStreamSync_ }, \
    CDI_NETCDF_SWITCHES \
    }

#if defined (SX) || defined (__cplusplus)
static const union namespaceSwitchValue
  defaultSwitches_[NUM_NAMESPACE_SWITCH] = defaultSwitches;
#endif

enum namespaceStatus {
  NAMESPACE_STATUS_INUSE,
  NAMESPACE_STATUS_UNUSED,
};

static struct Namespace
{
  enum namespaceStatus resStage;
  union namespaceSwitchValue switches[NUM_NAMESPACE_SWITCH];
} initialNamespace = {
  .resStage = NAMESPACE_STATUS_INUSE,
  .switches = defaultSwitches
};

static struct Namespace *namespaces = &initialNamespace;

static unsigned namespacesSize = 1;

#if defined (HAVE_LIBPTHREAD)
# include <pthread.h>

static pthread_once_t namespaceOnce = PTHREAD_ONCE_INIT;
static pthread_mutex_t namespaceMutex;

static void
namespaceInitialize(void)
{
  pthread_mutexattr_t ma;
  pthread_mutexattr_init(&ma);
  pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&namespaceMutex, &ma);
  pthread_mutexattr_destroy(&ma);
}

#define NAMESPACE_LOCK() pthread_mutex_lock(&namespaceMutex)
#define NAMESPACE_UNLOCK() pthread_mutex_unlock(&namespaceMutex)
#define NAMESPACE_INIT() pthread_once(&namespaceOnce, \
                                                namespaceInitialize)


#else

#define NAMESPACE_INIT() do { } while (0)
#define NAMESPACE_LOCK()
#define NAMESPACE_UNLOCK()

#endif


enum {
  intbits = sizeof(int) * CHAR_BIT,
  nspbits = 4,
  idxbits = intbits - nspbits,
  nspmask = (int)((( (unsigned)1 << nspbits ) - 1) << idxbits),
  idxmask = ( 1 << idxbits ) - 1,
};

enum {
  NUM_NAMESPACES = 1 << nspbits,
  NUM_IDX = 1 << idxbits,
};


int namespaceIdxEncode ( namespaceTuple_t tin )
{
  xassert ( tin.nsp < NUM_NAMESPACES && tin.idx < NUM_IDX);
  return ( tin.nsp << idxbits ) + tin.idx;
}

int namespaceIdxEncode2 ( int nsp, int idx )
{
  xassert(nsp < NUM_NAMESPACES && idx < NUM_IDX);
  return ( nsp << idxbits ) + idx;
}


namespaceTuple_t namespaceResHDecode ( int resH )
{
  namespaceTuple_t tin;

  tin.idx = resH & idxmask;
  tin.nsp = (int)(((unsigned)( resH & nspmask )) >> idxbits);

  return tin;
}

int
namespaceNew()
{
  int newNamespaceID = -1;
  NAMESPACE_INIT();
  NAMESPACE_LOCK();
  if (namespacesSize > nNamespaces)
    {

      for (unsigned i = 0; i < namespacesSize; ++i)
        if (namespaces[i].resStage == NAMESPACE_STATUS_UNUSED)
          {
            newNamespaceID = (int)i;
            break;
          }
    }
  else if (namespacesSize == 1)
    {

      struct Namespace *newNameSpaces
        = (struct Namespace *) Malloc(((size_t)namespacesSize + 1) * sizeof (namespaces[0]));
      memcpy(newNameSpaces, namespaces, sizeof (namespaces[0]));
      namespaces = newNameSpaces;
      ++namespacesSize;
      newNamespaceID = 1;
    }
  else if (namespacesSize < NUM_NAMESPACES)
    {

      newNamespaceID = (int)namespacesSize;
      namespaces
        = (struct Namespace *) Realloc(namespaces, ((size_t)namespacesSize + 1) * sizeof (namespaces[0]));
      ++namespacesSize;
    }
  else
    {
      NAMESPACE_UNLOCK();
      return -1;
    }
  xassert(newNamespaceID >= 0 && newNamespaceID < NUM_NAMESPACES);
  ++nNamespaces;
  namespaces[newNamespaceID].resStage = NAMESPACE_STATUS_INUSE;
#if defined (SX) || defined (__cplusplus)
  memcpy(namespaces[newNamespaceID].switches,
         defaultSwitches_,
         sizeof (namespaces[newNamespaceID].switches));
#else
    memcpy(namespaces[newNamespaceID].switches,
           (union namespaceSwitchValue[NUM_NAMESPACE_SWITCH])defaultSwitches,
           sizeof (namespaces[newNamespaceID].switches));
#endif
  reshListCreate(newNamespaceID);
  NAMESPACE_UNLOCK();
  return newNamespaceID;
}

void
namespaceDelete(int namespaceID)
{
  NAMESPACE_INIT();
  NAMESPACE_LOCK();
  xassert(namespaceID >= 0 && (unsigned)namespaceID < namespacesSize
          && nNamespaces);
  reshListDestruct(namespaceID);
  namespaces[namespaceID].resStage = NAMESPACE_STATUS_UNUSED;
  --nNamespaces;
  NAMESPACE_UNLOCK();
}

int namespaceGetNumber ()
{
  return (int)nNamespaces;
}


void namespaceSetActive ( int nId )
{
  xassert((unsigned)nId < namespacesSize
          && namespaces[nId].resStage != NAMESPACE_STATUS_UNUSED);
  activeNamespace = nId;
}


int namespaceGetActive ()
{
  return activeNamespace;
}

int namespaceAdaptKey ( int originResH, int originNamespace )
{
  namespaceTuple_t tin;
  int nsp;

  if ( originResH == CDI_UNDEFID ) return CDI_UNDEFID;

  tin.idx = originResH & idxmask;
  tin.nsp = (int)(((unsigned)( originResH & nspmask )) >> idxbits);

  xassert ( tin.nsp == originNamespace );

  nsp = namespaceGetActive ();

  return namespaceIdxEncode2 ( nsp, tin.idx );
}


int namespaceAdaptKey2 ( int originResH )
{
  namespaceTuple_t tin;
  int nsp;

  if ( originResH == CDI_UNDEFID ) return CDI_UNDEFID;

  tin.idx = originResH & idxmask;
  tin.nsp = (int)(((unsigned)( originResH & nspmask )) >> idxbits);

  nsp = namespaceGetActive ();

  return namespaceIdxEncode2 ( nsp, tin.idx );
}

void namespaceSwitchSet(enum namespaceSwitch sw, union namespaceSwitchValue value)
{
  xassert(sw > NSSWITCH_NO_SUCH_SWITCH && sw < NUM_NAMESPACE_SWITCH);
  int nsp = namespaceGetActive();
  namespaces[nsp].switches[sw] = value;
}

union namespaceSwitchValue namespaceSwitchGet(enum namespaceSwitch sw)
{
  xassert(sw > NSSWITCH_NO_SUCH_SWITCH && sw < NUM_NAMESPACE_SWITCH);
  int nsp = namespaceGetActive();
  return namespaces[nsp].switches[sw];
}

void cdiReset(void)
{
  NAMESPACE_INIT();
  NAMESPACE_LOCK();
  for (unsigned namespaceID = 0; namespaceID < namespacesSize; ++namespaceID)
    if (namespaces[namespaceID].resStage != NAMESPACE_STATUS_UNUSED)
      namespaceDelete((int)namespaceID);
  if (namespaces != &initialNamespace)
    {
      Free(namespaces);
      namespaces = &initialNamespace;
      namespaces[0].resStage = NAMESPACE_STATUS_UNUSED;
    }
  namespacesSize = 1;
  nNamespaces = 0;
  NAMESPACE_UNLOCK();
}
#ifndef INCLUDE_GUARD_CDI_REFERENCE_COUNTING
#define INCLUDE_GUARD_CDI_REFERENCE_COUNTING


#include <sys/types.h>
#include <stdlib.h>
typedef struct CdiReferencedObject CdiReferencedObject;
struct CdiReferencedObject {

    void (*destructor)(CdiReferencedObject* me);


    size_t refCount;
};

void cdiRefObject_construct(CdiReferencedObject* me);
void cdiRefObject_retain(CdiReferencedObject* me);
void cdiRefObject_release(CdiReferencedObject* me);
void cdiRefObject_destruct(CdiReferencedObject* me);

#endif
void cdiRefObject_construct(CdiReferencedObject* me)
{
  me->destructor = cdiRefObject_destruct;
  me->refCount = 1;
}

void cdiRefObject_retain(CdiReferencedObject* me)
{
  size_t oldCount = me->refCount++;
  xassert(oldCount && "A reference counted object was used after it was destructed.");
}

void cdiRefObject_release(CdiReferencedObject* me)
{
  size_t oldCount = me->refCount--;
  xassert(oldCount && "A reference counted object was released too often.");
  if(oldCount == 1)
    {
      me->destructor(me);
      Free(me);
    }
}

void cdiRefObject_destruct(CdiReferencedObject* me)
{
  (void)me;

}
#if defined (HAVE_CONFIG_H)
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#if defined (HAVE_EXECINFO_H)
#include <execinfo.h>
#endif

static
void show_stackframe()
{
#if defined HAVE_EXECINFO_H && defined backtrace_size_t && defined HAVE_BACKTRACE
  void *trace[16];
  backtrace_size_t trace_size = backtrace(trace, 16);
  char **messages = backtrace_symbols(trace, trace_size);

  fprintf(stderr, "[bt] Execution path:\n");
  if ( messages ) {
    for ( backtrace_size_t i = 0; i < trace_size; ++i )
      fprintf(stderr, "[bt] %s\n", messages[i]);
    free(messages);
  }
#endif
}


enum { MIN_LIST_SIZE = 128 };

static void listInitialize(void);

typedef struct listElem {
  union
  {

    struct
    {
      int next, prev;
    } free;

    struct
    {
      const resOps *ops;
      void *val;
    } v;
  } res;
  int status;
} listElem_t;

struct resHList_t
{
  int size, freeHead, hasDefaultRes;
  listElem_t *resources;
};

static struct resHList_t *resHList;

static int resHListSize = 0;

#if defined (HAVE_LIBPTHREAD)
# include <pthread.h>

static pthread_once_t listInitThread = PTHREAD_ONCE_INIT;
static pthread_mutex_t listMutex;

#define LIST_LOCK() pthread_mutex_lock(&listMutex)
#define LIST_UNLOCK() pthread_mutex_unlock(&listMutex)
#define LIST_INIT(init0) do { \
    pthread_once(&listInitThread, listInitialize); \
    pthread_mutex_lock(&listMutex); \
    if ((init0) && (!resHList || !resHList[0].resources)) \
      reshListCreate(0); \
    pthread_mutex_unlock(&listMutex); \
  } while (0)



#else

static int listInit = 0;

#define LIST_LOCK()
#define LIST_UNLOCK()
#define LIST_INIT(init0) do { \
  if ( !listInit ) \
    { \
      listInitialize(); \
      if ((init0) && (!resHList || !resHList[0].resources)) \
        reshListCreate(0); \
      listInit = 1; \
    } \
  } while(0)

#endif



static void
listInitResources(int nsp)
{
  xassert(nsp < resHListSize && nsp >= 0);
  int size = resHList[nsp].size = MIN_LIST_SIZE;
  xassert(resHList[nsp].resources == NULL);
  resHList[nsp].resources = (listElem_t*) Calloc(MIN_LIST_SIZE, sizeof(listElem_t));
  listElem_t *p = resHList[nsp].resources;

  for (int i = 0; i < size; i++ )
    {
      p[i].res.free.next = i + 1;
      p[i].res.free.prev = i - 1;
      p[i].status = RESH_UNUSED;
    }

  p[size-1].res.free.next = -1;
  resHList[nsp].freeHead = 0;
  int oldNsp = namespaceGetActive();
  namespaceSetActive(nsp);
  instituteDefaultEntries();
  modelDefaultEntries();
  namespaceSetActive(oldNsp);
}

static inline void
reshListClearEntry(int i)
{
  resHList[i].size = 0;
  resHList[i].resources = NULL;
  resHList[i].freeHead = -1;
}

void
reshListCreate(int namespaceID)
{
  LIST_INIT(namespaceID != 0);
  LIST_LOCK();
  if (resHListSize <= namespaceID)
    {
      resHList = (struct resHList_t *) Realloc(resHList, (size_t)(namespaceID + 1) * sizeof (resHList[0]));
      for (int i = resHListSize; i <= namespaceID; ++i)
        reshListClearEntry(i);
      resHListSize = namespaceID + 1;
    }
  listInitResources(namespaceID);
  LIST_UNLOCK();
}




void
reshListDestruct(int namespaceID)
{
  LIST_LOCK();
  xassert(resHList && namespaceID >= 0 && namespaceID < resHListSize);
  int callerNamespaceID = namespaceGetActive();
  namespaceSetActive(namespaceID);
  if (resHList[namespaceID].resources)
    {
      for ( int j = 0; j < resHList[namespaceID].size; j++ )
        {
          listElem_t *listElem = resHList[namespaceID].resources + j;
          if (listElem->status & RESH_IN_USE_BIT)
            listElem->res.v.ops->valDestroy(listElem->res.v.val);
        }
      Free(resHList[namespaceID].resources);
      resHList[namespaceID].resources = NULL;
      reshListClearEntry(namespaceID);
    }
  if (resHList[callerNamespaceID].resources)
    namespaceSetActive(callerNamespaceID);
  LIST_UNLOCK();
}


static void listDestroy ( void )
{
  LIST_LOCK();
  for (int i = resHListSize; i > 0; --i)
    if (resHList[i-1].resources)
      namespaceDelete(i-1);
  resHListSize = 0;
  Free(resHList);
  resHList = NULL;
  cdiReset();
  LIST_UNLOCK();
}



static
void listInitialize ( void )
{
#if defined (HAVE_LIBPTHREAD)
  pthread_mutexattr_t ma;
  pthread_mutexattr_init(&ma);
  pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);

  pthread_mutex_init ( &listMutex, &ma);
  pthread_mutexattr_destroy(&ma);
#endif


  {
    int null_id;
    null_id = fileOpen_serial("/dev/null", "r");
    if (null_id != -1)
      fileClose_serial(null_id);
  }
  atexit ( listDestroy );
}



static
void listSizeExtend()
{
  int nsp = namespaceGetActive ();
  int oldSize = resHList[nsp].size;
  size_t newListSize = (size_t)oldSize + MIN_LIST_SIZE;

  resHList[nsp].resources = (listElem_t*) Realloc(resHList[nsp].resources,
                                                   newListSize * sizeof(listElem_t));

  listElem_t *r = resHList[nsp].resources;
  for (size_t i = (size_t)oldSize; i < newListSize; ++i)
    {
      r[i].res.free.next = (int)i + 1;
      r[i].res.free.prev = (int)i - 1;
      r[i].status = RESH_UNUSED;
    }

  if (resHList[nsp].freeHead != -1)
    r[resHList[nsp].freeHead].res.free.prev = (int)newListSize - 1;
  r[newListSize-1].res.free.next = resHList[nsp].freeHead;
  r[oldSize].res.free.prev = -1;
  resHList[nsp].freeHead = oldSize;
  resHList[nsp].size = (int)newListSize;
}



static void
reshPut_(int nsp, int entry, void *p, const resOps *ops)
{
  listElem_t *newListElem = resHList[nsp].resources + entry;
  int next = newListElem->res.free.next,
    prev = newListElem->res.free.prev;
  if (next != -1)
    resHList[nsp].resources[next].res.free.prev = prev;
  if (prev != -1)
    resHList[nsp].resources[prev].res.free.next = next;
  else
    resHList[nsp].freeHead = next;
  newListElem->res.v.val = p;
  newListElem->res.v.ops = ops;
  newListElem->status = RESH_DESYNC_IN_USE;
}

int reshPut ( void *p, const resOps *ops )
{
  xassert ( p && ops );

  LIST_INIT(1);

  LIST_LOCK();

  int nsp = namespaceGetActive ();

  if ( resHList[nsp].freeHead == -1) listSizeExtend();
  int entry = resHList[nsp].freeHead;
  cdiResH resH = namespaceIdxEncode2(nsp, entry);
  reshPut_(nsp, entry, p, ops);

  LIST_UNLOCK();

  return resH;
}



static void
reshRemove_(int nsp, int idx)
{
  int curFree = resHList[nsp].freeHead;
  listElem_t *r = resHList[nsp].resources;
  r[idx].res.free.next = curFree;
  r[idx].res.free.prev = -1;
  if (curFree != -1)
    r[curFree].res.free.prev = idx;
  r[idx].status = RESH_DESYNC_DELETED;
  resHList[nsp].freeHead = idx;
}

void reshDestroy(cdiResH resH)
{
  int nsp;
  namespaceTuple_t nspT;

  LIST_LOCK();

  nsp = namespaceGetActive ();

  nspT = namespaceResHDecode ( resH );

  xassert ( nspT.nsp == nsp
            && nspT.idx >= 0
            && nspT.idx < resHList[nsp].size
            && resHList[nsp].resources[nspT.idx].res.v.ops);

  if (resHList[nsp].resources[nspT.idx].status & RESH_IN_USE_BIT)
    reshRemove_(nsp, nspT.idx);

  LIST_UNLOCK();
}

void reshRemove ( cdiResH resH, const resOps * ops )
{
  int nsp;
  namespaceTuple_t nspT;

  LIST_LOCK();

  nsp = namespaceGetActive ();

  nspT = namespaceResHDecode ( resH );

  xassert ( nspT.nsp == nsp
            && nspT.idx >= 0
            && nspT.idx < resHList[nsp].size
            && (resHList[nsp].resources[nspT.idx].status & RESH_IN_USE_BIT)
            && resHList[nsp].resources[nspT.idx].res.v.ops
            && resHList[nsp].resources[nspT.idx].res.v.ops == ops );

  reshRemove_(nsp, nspT.idx);

  LIST_UNLOCK();
}



void reshReplace(cdiResH resH, void *p, const resOps *ops)
{
  xassert(p && ops);
  LIST_INIT(1);
  LIST_LOCK();
  int nsp = namespaceGetActive();
  namespaceTuple_t nspT = namespaceResHDecode(resH);
  while (resHList[nsp].size <= nspT.idx)
    listSizeExtend();
  listElem_t *q = resHList[nsp].resources + nspT.idx;
  if (q->status & RESH_IN_USE_BIT)
    {
      q->res.v.ops->valDestroy(q->res.v.val);
      reshRemove_(nsp, nspT.idx);
    }
  reshPut_(nsp, nspT.idx, p, ops);
  LIST_UNLOCK();
}


static listElem_t *
reshGetElem(const char *caller, const char* expressionString, cdiResH resH, const resOps *ops)
{
  listElem_t *listElem;
  int nsp;
  namespaceTuple_t nspT;
  xassert ( ops );

  LIST_INIT(1);

  LIST_LOCK();

  nsp = namespaceGetActive ();

  nspT = namespaceResHDecode ( resH );
  assert(nspT.idx >= 0);

  if (nspT.nsp == nsp &&
      nspT.idx < resHList[nsp].size)
    {
      listElem = resHList[nsp].resources + nspT.idx;
      LIST_UNLOCK();
    }
  else
    {
      LIST_UNLOCK();
      show_stackframe();

      if ( resH == CDI_UNDEFID )
        {
          xabortC(caller, "Error while trying to resolve the ID \"%s\" in `%s()`: the value is CDI_UNDEFID (= %d).\n\tThis is most likely the result of a failed earlier call. Please check the IDs returned by CDI.", expressionString, caller, resH);
        }
      else
        {
          xabortC(caller, "Error while trying to resolve the ID \"%s\" in `%s()`: the value is garbage (= %d, which resolves to namespace = %d, index = %d).\n\tThis is either the result of using an uninitialized variable,\n\tof using a value as an ID that is not an ID,\n\tor of using an ID after it has been invalidated.", expressionString, caller, resH, nspT.nsp, nspT.idx);
        }
    }

  if ( !(listElem && listElem->res.v.ops == ops) )
    {
      show_stackframe();

      xabortC(caller, "Error while trying to resolve the ID \"%s\" in `%s()`: list element not found. The failed ID is %d", expressionString, caller, (int)resH);
    }

  return listElem;
}

void *reshGetValue(const char * caller, const char* expressionString, cdiResH resH, const resOps * ops)
{
  return reshGetElem(caller, expressionString, resH, ops)->res.v.val;
}



void reshGetResHListOfType(unsigned numIDs, int resHs[], const resOps *ops)
{
  xassert ( resHs && ops );

  LIST_INIT(1);

  LIST_LOCK();

  int nsp = namespaceGetActive();
  unsigned j = 0;
  for (int i = 0; i < resHList[nsp].size && j < numIDs; i++ )
    if ((resHList[nsp].resources[i].status & RESH_IN_USE_BIT)
        && resHList[nsp].resources[i].res.v.ops == ops)
      resHs[j++] = namespaceIdxEncode2(nsp, i);

  LIST_UNLOCK();
}

enum cdiApplyRet
cdiResHApply(enum cdiApplyRet (*func)(int id, void *res, const resOps *p,
                                      void *data), void *data)
{
  xassert(func);

  LIST_INIT(1);

  LIST_LOCK();

  int nsp = namespaceGetActive ();
  enum cdiApplyRet ret = CDI_APPLY_GO_ON;
  for (int i = 0; i < resHList[nsp].size && ret > 0; ++i)
    if (resHList[nsp].resources[i].status & RESH_IN_USE_BIT)
      ret = func(namespaceIdxEncode2(nsp, i),
                 resHList[nsp].resources[i].res.v.val,
                 resHList[nsp].resources[i].res.v.ops, data);
  LIST_UNLOCK();
  return ret;
}


enum cdiApplyRet
cdiResHFilterApply(const resOps *p,
                   enum cdiApplyRet (*func)(int id, void *res, void *data),
                   void *data)
{
  xassert(p && func);

  LIST_INIT(1);

  LIST_LOCK();

  int nsp = namespaceGetActive ();
  enum cdiApplyRet ret = CDI_APPLY_GO_ON;
  listElem_t *r = resHList[nsp].resources;
  for (int i = 0; i < resHList[nsp].size && ret > 0; ++i)
    if ((r[i].status & RESH_IN_USE_BIT) && r[i].res.v.ops == p)
      ret = func(namespaceIdxEncode2(nsp, i), r[i].res.v.val,
                 data);
  LIST_UNLOCK();
  return ret;
}






unsigned reshCountType(const resOps *ops)
{
  unsigned countType = 0;

  xassert(ops);

  LIST_INIT(1);

  LIST_LOCK();

  int nsp = namespaceGetActive ();

  listElem_t *r = resHList[nsp].resources;
  size_t len = (size_t)resHList[nsp].size;
  for (size_t i = 0; i < len; i++ )
    countType += ((r[i].status & RESH_IN_USE_BIT) && r[i].res.v.ops == ops);

  LIST_UNLOCK();

  return countType;
}



int
reshResourceGetPackSize_intern(int resH, const resOps *ops, void *context, const char* caller, const char* expressionString)
{
  listElem_t *curr = reshGetElem(caller, expressionString, resH, ops);
  return curr->res.v.ops->valGetPackSize(curr->res.v.val, context);
}

void
reshPackResource_intern(int resH, const resOps *ops, void *buf, int buf_size, int *position, void *context,
                        const char* caller, const char* expressionString)
{
  listElem_t *curr = reshGetElem(caller, expressionString, resH, ops);
  curr->res.v.ops->valPack(curr->res.v.val, buf, buf_size, position, context);
}

enum {
  resHPackHeaderNInt = 2,
  resHDeleteNInt = 2,
};

static int getPackBufferSize(void *context)
{
  int intpacksize, packBufferSize = 0;

  int nsp = namespaceGetActive ();


  packBufferSize += resHPackHeaderNInt * (intpacksize = serializeGetSize(1, DATATYPE_INT, context));


  listElem_t *r = resHList[nsp].resources;
  for ( int i = 0; i < resHList[nsp].size; i++)
    if (r[i].status & RESH_SYNC_BIT)
      {
        if (r[i].status == RESH_DESYNC_DELETED)
          {
            packBufferSize += resHDeleteNInt * intpacksize;
          }
        else if (r[i].status == RESH_DESYNC_IN_USE)
          {
            xassert ( r[i].res.v.ops );

            packBufferSize +=
              r[i].res.v.ops->valGetPackSize(r[i].res.v.val, context)
              + intpacksize;
          }
      }

  packBufferSize += intpacksize;

  return packBufferSize;
}



void reshPackBufferDestroy ( char ** buffer )
{
  if ( buffer ) free ( *buffer );
}



int reshPackBufferCreate(char **packBuffer, int *packBufferSize, void *context)
{
  int i, packBufferPos = 0;
  int end = END;

  xassert ( packBuffer );

  LIST_LOCK();

  int nsp = namespaceGetActive ();

  int pBSize = *packBufferSize = getPackBufferSize(context);
  char *pB = *packBuffer = (char *) Malloc((size_t)pBSize);

  {
    int header[resHPackHeaderNInt] = { START, nsp };
    serializePack(header, resHPackHeaderNInt, DATATYPE_INT, pB, pBSize, &packBufferPos, context);
  }

  listElem_t *r = resHList[nsp].resources;
  for ( i = 0; i < resHList[nsp].size; i++ )
    if (r[i].status & RESH_SYNC_BIT)
      {
        if (r[i].status == RESH_DESYNC_DELETED)
          {
            int temp[resHDeleteNInt]
              = { RESH_DELETE, namespaceIdxEncode2(nsp, i) };
            serializePack(temp, resHDeleteNInt, DATATYPE_INT,
                          pB, pBSize, &packBufferPos, context);
          }
        else
          {
            listElem_t * curr = r + i;
            xassert ( curr->res.v.ops );
            int type = curr->res.v.ops->valTxCode();
            if ( ! type ) continue;
            serializePack(&type, 1, DATATYPE_INT, pB,
                          pBSize, &packBufferPos, context);
            curr->res.v.ops->valPack(curr->res.v.val,
                                     pB, pBSize, &packBufferPos, context);
          }
        r[i].status &= ~RESH_SYNC_BIT;
      }

  LIST_UNLOCK();

  serializePack(&end, 1, DATATYPE_INT, pB, pBSize, &packBufferPos, context);
  return packBufferPos;
}





void reshSetStatus ( cdiResH resH, const resOps * ops, int status )
{
  int nsp;
  namespaceTuple_t nspT;
  listElem_t * listElem;

  xassert((ops != NULL) ^ !(status & RESH_IN_USE_BIT));

  LIST_INIT(1);

  LIST_LOCK();

  nsp = namespaceGetActive ();

  nspT = namespaceResHDecode ( resH );

  xassert ( nspT.nsp == nsp &&
            nspT.idx >= 0 &&
            nspT.idx < resHList[nsp].size );

  xassert ( resHList[nsp].resources );
  listElem = resHList[nsp].resources + nspT.idx;

  xassert((!ops || (listElem->res.v.ops == ops))
          && (listElem->status & RESH_IN_USE_BIT) == (status & RESH_IN_USE_BIT));

  listElem->status = status;

  LIST_UNLOCK();
}



int reshGetStatus ( cdiResH resH, const resOps * ops )
{
  int nsp;
  namespaceTuple_t nspT;

  LIST_INIT(1);

  LIST_LOCK();

  nsp = namespaceGetActive ();

  nspT = namespaceResHDecode ( resH );

  xassert ( nspT.nsp == nsp &&
            nspT.idx >= 0 &&
            nspT.idx < resHList[nsp].size );

  listElem_t *listElem = resHList[nsp].resources + nspT.idx;

  const resOps *elemOps = listElem->res.v.ops;

  LIST_UNLOCK();

  xassert(listElem && (!(listElem->status & RESH_IN_USE_BIT) || elemOps == ops));

  return listElem->status;
}



void reshLock ()
{
  LIST_LOCK();
}



void reshUnlock ()
{
  LIST_UNLOCK();
}



int reshListCompare ( int nsp0, int nsp1 )
{
  LIST_INIT(1);
  LIST_LOCK();

  xassert(resHListSize > nsp0 && resHListSize > nsp1 &&
          nsp0 >= 0 && nsp1 >= 0);

  int valCompare = 0;
  int i, listSizeMin = (resHList[nsp0].size <= resHList[nsp1].size)
    ? resHList[nsp0].size : resHList[nsp1].size;
  listElem_t *resources0 = resHList[nsp0].resources,
    *resources1 = resHList[nsp1].resources;
  for (i = 0; i < listSizeMin; i++)
    {
      int occupied0 = (resources0[i].status & RESH_IN_USE_BIT) != 0,
        occupied1 = (resources1[i].status & RESH_IN_USE_BIT) != 0;

      int diff = occupied0 ^ occupied1;
      valCompare |= (diff << cdiResHListOccupationMismatch);
      if (!diff && occupied0)
        {

          diff = (resources0[i].res.v.ops != resources1[i].res.v.ops
                  || resources0[i].res.v.ops == NULL);
          valCompare |= (diff << cdiResHListResourceTypeMismatch);
          if (!diff)
            {

              diff
                = resources0[i].res.v.ops->valCompare(resources0[i].res.v.val,
                                                      resources1[i].res.v.val);
              valCompare |= (diff << cdiResHListResourceContentMismatch);
            }
        }
    }

  for (int j = listSizeMin; j < resHList[nsp0].size; ++j)
    valCompare |= (((resources0[j].status & RESH_IN_USE_BIT) != 0)
                   << cdiResHListOccupationMismatch);

  for (; i < resHList[nsp1].size; ++i)
    valCompare |= (((resources1[i].status & RESH_IN_USE_BIT) != 0)
                   << cdiResHListOccupationMismatch);

  LIST_UNLOCK();

  return valCompare;
}



void reshListPrint(FILE *fp)
{
  int i, j, temp;
  listElem_t * curr;

  LIST_INIT(1);


  temp = namespaceGetActive ();

  fprintf ( fp, "\n\n##########################################\n#\n#  print " \
            "global resource list \n#\n" );

  for ( i = 0; i < namespaceGetNumber (); i++ )
    {
      namespaceSetActive ( i );

      fprintf ( fp, "\n" );
      fprintf ( fp, "##################################\n" );
      fprintf ( fp, "#\n" );
      fprintf ( fp, "# namespace=%d\n", i );
      fprintf ( fp, "#\n" );
      fprintf ( fp, "##################################\n\n" );

      fprintf ( fp, "resHList[%d].size=%d\n", i, resHList[i].size );

      for ( j = 0; j < resHList[i].size; j++ )
        {
          curr = resHList[i].resources + j;
          if (!(curr->status & RESH_IN_USE_BIT))
            {
              curr->res.v.ops->valPrint(curr->res.v.val, fp);
              fprintf ( fp, "\n" );
            }
        }
    }
  fprintf ( fp, "#\n#  end global resource list" \
            "\n#\n##########################################\n\n" );

  namespaceSetActive ( temp );
}
#include <inttypes.h>
#include <limits.h>
#include <string.h>


int
serializeGetSize(int count, int datatype, void *context)
{
  int (*serialize_get_size_p)(int count, int datatype, void *context)
    = (int (*)(int, int, void *))
    namespaceSwitchGet(NSSWITCH_SERIALIZE_GET_SIZE).func;
  return serialize_get_size_p(count, datatype, context);
}

void serializePack(const void *data, int count, int datatype,
                   void *buf, int buf_size, int *position, void *context)
{
  void (*serialize_pack_p)(const void *data, int count, int datatype,
                           void *buf, int buf_size, int *position, void *context)
    = (void (*)(const void *, int, int, void *, int, int *, void *))
    namespaceSwitchGet(NSSWITCH_SERIALIZE_PACK).func;
  serialize_pack_p(data, count, datatype, buf, buf_size, position, context);
}

void serializeUnpack(const void *buf, int buf_size, int *position,
                     void *data, int count, int datatype, void *context)
{
  void (*serialize_unpack_p)(const void *buf, int buf_size, int *position,
                             void *data, int count, int datatype, void *context)
    = (void (*)(const void *, int, int *, void *, int, int, void *))
    namespaceSwitchGet(NSSWITCH_SERIALIZE_UNPACK).func;
  serialize_unpack_p(buf, buf_size, position, data, count, datatype, context);
}



int
serializeGetSizeInCore(int count, int datatype, void *context)
{
  int elemSize;
  (void)context;
  switch (datatype)
  {
  case DATATYPE_INT8:
    elemSize = sizeof (int8_t);
    break;
  case DATATYPE_INT16:
    elemSize = sizeof (int16_t);
    break;
  case DATATYPE_UINT32:
    elemSize = sizeof (uint32_t);
    break;
  case DATATYPE_INT:
    elemSize = sizeof (int);
    break;
  case DATATYPE_FLT:
  case DATATYPE_FLT64:
    elemSize = sizeof (double);
    break;
  case DATATYPE_TXT:
  case DATATYPE_UCHAR:
    elemSize = 1;
    break;
  case DATATYPE_LONG:
    elemSize = sizeof (long);
    break;
  default:
    xabort("Unexpected datatype");
  }
  return count * elemSize;
}

void serializePackInCore(const void *data, int count, int datatype,
                         void *buf, int buf_size, int *position, void *context)
{
  int size = serializeGetSize(count, datatype, context);
  int pos = *position;
  xassert(INT_MAX - pos >= size && buf_size - pos >= size);
  memcpy((unsigned char *)buf + pos, data, (size_t)size);
  pos += size;
  *position = pos;
}

void serializeUnpackInCore(const void *buf, int buf_size, int *position,
                           void *data, int count, int datatype, void *context)
{
  int size = serializeGetSize(count, datatype, context);
  int pos = *position;
  xassert(INT_MAX - pos >= size && buf_size - pos >= size);
  memcpy(data, (unsigned char *)buf + pos, (size_t)size);
  pos += size;
  *position = pos;
}
#ifndef _DTYPES_H
#define _DTYPES_H

#include <stdio.h>
#include <limits.h>



#if ! defined (INT_MAX)
# error INT_MAX undefined
#endif

#undef INT32
#if INT_MAX == 2147483647L
#define INT32 int
#elif LONG_MAX == 2147483647L
#define INT32 long
#endif



#if ! defined (LONG_MAX)
# error LONG_MAX undefined
#endif

#undef INT64
#if LONG_MAX > 2147483647L
#define INT64 long
#else
#define INT64 long long
#endif



#undef FLT32
#define FLT32 float



#undef FLT64
#define FLT64 double



#define UINT32 unsigned INT32
#define UINT64 unsigned INT64

#endif
#ifndef _BINARY_H
#define _BINARY_H

#ifdef HAVE_CONFIG_H
#endif

#include <inttypes.h>
#include <stdio.h>

#ifndef _DTYPES_H
#endif

#ifndef HOST_ENDIANNESS
#ifdef __cplusplus
static const uint32_t HOST_ENDIANNESS_temp[1] = { UINT32_C(0x00030201) };
#define HOST_ENDIANNESS (((const unsigned char *)HOST_ENDIANNESS_temp)[0])
#else
#define HOST_ENDIANNESS (((const unsigned char *)&(const uint32_t[1]){UINT32_C(0x00030201)})[0])
#endif
#endif


UINT32 get_UINT32(unsigned char *x);
UINT32 get_SUINT32(unsigned char *x);
UINT64 get_UINT64(unsigned char *x);
UINT64 get_SUINT64(unsigned char *x);


size_t binReadF77Block(int fileID, int byteswap);
void binWriteF77Block(int fileID, int byteswap, size_t blocksize);

int binReadInt32(int fileID, int byteswap, size_t size, INT32 *ptr);
int binReadInt64(int fileID, int byteswap, size_t size, INT64 *ptr);

int binWriteInt32(int fileID, int byteswap, size_t size, INT32 *ptr);
int binWriteInt64(int fileID, int byteswap, size_t size, INT64 *ptr);

int binReadFlt32(int fileID, int byteswap, size_t size, FLT32 *ptr);
int binReadFlt64(int fileID, int byteswap, size_t size, FLT64 *ptr);

int binWriteFlt32(int fileID, int byteswap, size_t size, FLT32 *ptr);
int binWriteFlt64(int fileID, int byteswap, size_t size, FLT64 *ptr);

#endif
#ifndef _STREAM_GRB_H
#define _STREAM_GRB_H

int grbBitsPerValue(int datatype);

int grbInqContents(stream_t * streamptr);
int grbInqTimestep(stream_t * streamptr, int tsID);

int grbInqRecord(stream_t * streamptr, int *varID, int *levelID);
void grbDefRecord(stream_t * streamptr);
void grbReadRecord(stream_t * streamptr, double *data, int *nmiss);
void grb_write_record(stream_t * streamptr, int memtype, const void *data, int nmiss);
void grbCopyRecord(stream_t * streamptr2, stream_t * streamptr1);

void grbReadVarDP(stream_t * streamptr, int varID, double *data, int *nmiss);
void grb_write_var(stream_t * streamptr, int varID, int memtype, const void *data, int nmiss);

void grbReadVarSliceDP(stream_t * streamptr, int varID, int levelID, double *data, int *nmiss);
void grb_write_var_slice(stream_t *streamptr, int varID, int levelID, int memtype, const void *data, int nmiss);

int grib1ltypeToZaxisType(int grib_ltype);
int grib2ltypeToZaxisType(int grib_ltype);

int zaxisTypeToGrib1ltype(int zaxistype);
int zaxisTypeToGrib2ltype(int zaxistype);

#endif
#ifndef _STREAM_SRV_H
#define _STREAM_SRV_H

#ifndef _SERVICE_H
#endif

int srvInqContents(stream_t *streamptr);
int srvInqTimestep(stream_t *streamptr, int tsID);

int srvInqRecord(stream_t *streamptr, int *varID, int *levelID);
void srvDefRecord(stream_t *streamptr);
void srvCopyRecord(stream_t *streamptr2, stream_t *streamptr1);
void srvReadRecord(stream_t *streamptr, double *data, int *nmiss);
void srvWriteRecord(stream_t *streamptr, const double *data);

void srvReadVarDP (stream_t *streamptr, int varID, double *data, int *nmiss);
void srvWriteVarDP(stream_t *streamptr, int varID, const double *data);

void srvReadVarSliceDP (stream_t *streamptr, int varID, int levelID, double *data, int *nmiss);
void srvWriteVarSliceDP(stream_t *streamptr, int varID, int levelID, const double *data);

#endif
#ifndef _STREAM_EXT_H
#define _STREAM_EXT_H

#ifndef _EXTRA_H
#endif

int extInqContents(stream_t *streamptr);
int extInqTimestep(stream_t *streamptr, int tsID);

int extInqRecord(stream_t *streamptr, int *varID, int *levelID);
void extDefRecord(stream_t *streamptr);
void extCopyRecord(stream_t *streamptr2, stream_t *streamptr1);
void extReadRecord(stream_t *streamptr, double *data, int *nmiss);
void extWriteRecord(stream_t *streamptr, const double *data);

void extReadVarDP (stream_t *streamptr, int varID, double *data, int *nmiss);
void extWriteVarDP(stream_t *streamptr, int varID, const double *data);

void extReadVarSliceDP (stream_t *streamptr, int varID, int levelID, double *data, int *nmiss);
void extWriteVarSliceDP(stream_t *streamptr, int varID, int levelID, const double *data);

#endif
#ifndef _STREAM_IEG_H
#define _STREAM_IEG_H

#ifndef _IEG_H
#endif

int iegInqContents(stream_t *streamptr);
int iegInqTimestep(stream_t *streamptr, int tsID);

int iegInqRecord(stream_t *streamptr, int *varID, int *levelID);
void iegDefRecord(stream_t *streamptr);
void iegCopyRecord(stream_t *streamptr2, stream_t *streamptr1);
void iegReadRecord(stream_t *streamptr, double *data, int *nmiss);
void iegWriteRecord(stream_t *streamptr, const double *data);

void iegReadVarDP (stream_t *streamptr, int varID, double *data, int *nmiss);
void iegWriteVarDP(stream_t *streamptr, int varID, const double *data);

void iegReadVarSliceDP (stream_t *streamptr, int varID, int levelID, double *data, int *nmiss);
void iegWriteVarSliceDP(stream_t *streamptr, int varID, int levelID, const double *data);

#endif
#if defined (HAVE_CONFIG_H)
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#include <ctype.h>



static stream_t *stream_new_entry(int resH);
static void stream_delete_entry(stream_t *streamptr);
static int streamCompareP(void * streamptr1, void * streamptr2);
static void streamDestroyP(void * streamptr);
static void streamPrintP(void * streamptr, FILE * fp);
static int streamGetPackSize(void * streamptr, void *context);
static void streamPack(void * streamptr, void * buff, int size, int * position, void *context);
static int streamTxCode(void);

const resOps streamOps = {
  streamCompareP,
  streamDestroyP,
  streamPrintP,
  streamGetPackSize,
  streamPack,
  streamTxCode
};



static
int getByteorder(int byteswap)
{
  int byteorder = -1;

  switch (HOST_ENDIANNESS)
    {
    case CDI_BIGENDIAN:
      byteorder = byteswap ? CDI_LITTLEENDIAN : CDI_BIGENDIAN;
      break;
    case CDI_LITTLEENDIAN:
      byteorder = byteswap ? CDI_BIGENDIAN : CDI_LITTLEENDIAN;
      break;

    case CDI_PDPENDIAN:
    default:
      Error("unhandled endianness");
    }
  return byteorder;
}


int cdiGetFiletype(const char *filename, int *byteorder)
{
  int filetype = CDI_EUFTYPE;
  int swap = 0;
  int version;
#if defined (HAVE_LIBCGRIBEX)
  long recpos;
#endif
  char buffer[8];

  int fileID = fileOpen(filename, "r");

  if ( fileID == CDI_UNDEFID )
    {
      if ( strncmp(filename, "http:", 5) == 0 || strncmp(filename, "https:", 6) == 0 )
        return (FILETYPE_NC);
      else
        return (CDI_ESYSTEM);
    }

  if ( fileRead(fileID, buffer, 8) != 8 ) return (CDI_EUFTYPE);

  fileRewind(fileID);

  if ( memcmp(buffer, "GRIB", 4) == 0 )
    {
      version = buffer[7];
      if ( version <= 1 )
        {
          filetype = FILETYPE_GRB;
          if ( CDI_Debug ) Message("found GRIB file = %s, version %d", filename, version);
        }
      else if ( version == 2 )
        {
          filetype = FILETYPE_GRB2;
          if ( CDI_Debug ) Message("found GRIB2 file = %s", filename);
        }
    }
  else if ( memcmp(buffer, "CDF\001", 4) == 0 )
    {
      filetype = FILETYPE_NC;
      if ( CDI_Debug ) Message("found CDF1 file = %s", filename);
    }
  else if ( memcmp(buffer, "CDF\002", 4) == 0 )
    {
      filetype = FILETYPE_NC2;
      if ( CDI_Debug ) Message("found CDF2 file = %s", filename);
    }
  else if ( memcmp(buffer+1, "HDF", 3) == 0 )
    {
      filetype = FILETYPE_NC4;
      if ( CDI_Debug ) Message("found HDF file = %s", filename);
    }
#if defined (HAVE_LIBSERVICE)
  else if ( srvCheckFiletype(fileID, &swap) )
    {
      filetype = FILETYPE_SRV;
      if ( CDI_Debug ) Message("found SRV file = %s", filename);
    }
#endif
#if defined (HAVE_LIBEXTRA)
  else if ( extCheckFiletype(fileID, &swap) )
    {
      filetype = FILETYPE_EXT;
      if ( CDI_Debug ) Message("found EXT file = %s", filename);
    }
#endif
#if defined (HAVE_LIBIEG)
  else if ( iegCheckFiletype(fileID, &swap) )
    {
      filetype = FILETYPE_IEG;
      if ( CDI_Debug ) Message("found IEG file = %s", filename);
    }
#endif
#if defined (HAVE_LIBCGRIBEX)
  else if ( gribCheckSeek(fileID, &recpos, &version) == 0 )
    {
      if ( version <= 1 )
        {
          filetype = FILETYPE_GRB;
          if ( CDI_Debug ) Message("found seeked GRIB file = %s", filename);
        }
      else if ( version == 2 )
        {
          filetype = FILETYPE_GRB2;
          if ( CDI_Debug ) Message("found seeked GRIB2 file = %s", filename);
        }
    }
#endif

  fileClose(fileID);

  *byteorder = getByteorder(swap);

  return (filetype);
}
int streamInqFiletype(int streamID)
{
  stream_t *streamptr = stream_to_pointer(streamID);
  return (streamptr->filetype);
}


int getByteswap(int byteorder)
{
  int byteswap = -1;

  switch (byteorder)
    {
    case CDI_BIGENDIAN:
    case CDI_LITTLEENDIAN:
    case CDI_PDPENDIAN:
      byteswap = (HOST_ENDIANNESS != byteorder);
      break;
    case -1:
      break;
    default:
      Error("unexpected byteorder %d query!", byteorder);
    }

  return (byteswap);
}
void streamDefByteorder(int streamID, int byteorder)
{
  stream_t *streamptr = stream_to_pointer(streamID);
  streamptr->byteorder = byteorder;
  int filetype = streamptr->filetype;

  switch (filetype)
    {
#if defined (HAVE_LIBSERVICE)
    case FILETYPE_SRV:
      {
        srvrec_t *srvp = (srvrec_t*) streamptr->record->exsep;
        srvp->byteswap = getByteswap(byteorder);

        break;
      }
#endif
#if defined (HAVE_LIBEXTRA)
    case FILETYPE_EXT:
      {
        extrec_t *extp = (extrec_t*) streamptr->record->exsep;
        extp->byteswap = getByteswap(byteorder);

        break;
      }
#endif
#if defined (HAVE_LIBIEG)
    case FILETYPE_IEG:
      {
        iegrec_t *iegp = (iegrec_t*) streamptr->record->exsep;
        iegp->byteswap = getByteswap(byteorder);

        break;
      }
#endif
    }
  reshSetStatus(streamID, &streamOps, RESH_DESYNC_IN_USE);
}
int streamInqByteorder(int streamID)
{
  stream_t *streamptr = stream_to_pointer(streamID);
  return (streamptr->byteorder);
}


const char *streamFilesuffix(int filetype)
{




  static const char fileSuffix[][5] = {"", ".grb", ".grb", ".nc", ".nc", ".nc", ".nc", ".srv", ".ext", ".ieg"};
  int size = (int)(sizeof(fileSuffix)/sizeof(fileSuffix[0]));

  if ( filetype > 0 && filetype < size )
    return (fileSuffix[filetype]);
  else
    return (fileSuffix[0]);
}


const char *streamFilename(int streamID)
{
  stream_t *streamptr = stream_to_pointer(streamID);
  return (streamptr->filename);
}

static
long cdiInqTimeSize(int streamID)
{
  int tsID = 0, nrecs;
  stream_t *streamptr = stream_to_pointer(streamID);
  long ntsteps = streamptr->ntsteps;

  if ( ntsteps == (long)CDI_UNDEFID )
    while ( (nrecs = streamInqTimestep(streamID, tsID++)) )
      ntsteps = streamptr->ntsteps;

  return (ntsteps);
}

static
int cdiInqContents(stream_t * streamptr)
{
  int status = 0;

  int filetype = streamptr->filetype;

  switch (filetype)
    {
#if defined (HAVE_LIBGRIB)
    case FILETYPE_GRB:
    case FILETYPE_GRB2:
      {
        status = grbInqContents(streamptr);
        break;
      }
#endif
#if defined (HAVE_LIBSERVICE)
    case FILETYPE_SRV:
      {
        status = srvInqContents(streamptr);
        break;
      }
#endif
#if defined (HAVE_LIBEXTRA)
    case FILETYPE_EXT:
      {
        status = extInqContents(streamptr);
        break;
      }
#endif
#if defined (HAVE_LIBIEG)
    case FILETYPE_IEG:
      {
        status = iegInqContents(streamptr);
        break;
      }
#endif
#if defined (HAVE_LIBNETCDF)
    case FILETYPE_NC:
    case FILETYPE_NC2:
    case FILETYPE_NC4:
    case FILETYPE_NC4C:
      {
        status = cdfInqContents(streamptr);
        break;
      }
#endif
    default:
      {
        if ( CDI_Debug )
          Message("%s support not compiled in!", strfiletype(filetype));

        status = CDI_ELIBNAVAIL;
        break;
      }
    }

  if ( status == 0 )
    {
      int vlistID = streamptr->vlistID;
      int taxisID = vlistInqTaxis(vlistID);
      if ( taxisID != CDI_UNDEFID )
        {
          taxis_t *taxisptr1 = &streamptr->tsteps[0].taxis;
          taxis_t *taxisptr2 = taxisPtr(taxisID);
          ptaxisCopy(taxisptr2, taxisptr1);
        }
    }

  return (status);
}

int cdiStreamOpenDefaultDelegate(const char *filename, char filemode,
                                 int filetype, stream_t *streamptr,
                                 int recordBufIsToBeCreated)
{
  int fileID;
  switch (filetype)
    {
#if defined (HAVE_LIBGRIB)
    case FILETYPE_GRB:
    case FILETYPE_GRB2:
      {
        fileID = gribOpen(filename, (char [2]){filemode, 0});
        if ( fileID < 0 ) fileID = CDI_ESYSTEM;
        if (recordBufIsToBeCreated)
          {
            streamptr->record = (Record *) Malloc(sizeof(Record));
            streamptr->record->buffer = NULL;
          }
        break;
      }
#endif
#if defined (HAVE_LIBSERVICE)
    case FILETYPE_SRV:
      {
        fileID = fileOpen(filename, (char [2]){filemode, 0});
        if ( fileID < 0 ) fileID = CDI_ESYSTEM;
        if (recordBufIsToBeCreated)
          {
            streamptr->record = (Record *) Malloc(sizeof(Record));
            streamptr->record->buffer = NULL;
            streamptr->record->exsep = srvNew();
          }
        break;
      }
#endif
#if defined (HAVE_LIBEXTRA)
    case FILETYPE_EXT:
      {
        fileID = fileOpen(filename, (char [2]){filemode, 0});
        if ( fileID < 0 ) fileID = CDI_ESYSTEM;
        if (recordBufIsToBeCreated)
          {
            streamptr->record = (Record *) Malloc(sizeof(Record));
            streamptr->record->buffer = NULL;
            streamptr->record->exsep = extNew();
          }
        break;
      }
#endif
#if defined (HAVE_LIBIEG)
    case FILETYPE_IEG:
      {
        fileID = fileOpen(filename, (char [2]){filemode, 0});
        if ( fileID < 0 ) fileID = CDI_ESYSTEM;
        if (recordBufIsToBeCreated)
          {
            streamptr->record = (Record *) Malloc(sizeof(Record));
            streamptr->record->buffer = NULL;
            streamptr->record->exsep = iegNew();
          }
        break;
      }
#endif
#if defined (HAVE_LIBNETCDF)
    case FILETYPE_NC:
      {
        fileID = cdfOpen(filename, (char [2]){filemode, 0});
        break;
      }
    case FILETYPE_NC2:
      {
        fileID = cdfOpen64(filename, (char [2]){filemode, 0});
        break;
      }
    case FILETYPE_NC4:
    case FILETYPE_NC4C:
      {
        fileID = cdf4Open(filename, (char [2]){filemode, 0}, &filetype);
        break;
      }
#endif
    default:
      {
        if ( CDI_Debug ) Message("%s support not compiled in!", strfiletype(filetype));
        return (CDI_ELIBNAVAIL);
      }
    }

  streamptr->filetype = filetype;

  return fileID;
}


int
streamOpenID(const char *filename, char filemode, int filetype,
             int resH)
{
  int fileID = CDI_UNDEFID;
  int status;

  if ( CDI_Debug )
    Message("Open %s mode %c file %s", strfiletype(filetype), filemode,
            filename?filename:"(NUL)");

  if ( ! filename || filetype < 0 ) return (CDI_EINVAL);

  stream_t *streamptr = stream_new_entry(resH);
  int streamID = CDI_ESYSTEM;

  {
    int (*streamOpenDelegate)(const char *filename, char filemode,
                              int filetype, stream_t *streamptr, int recordBufIsToBeCreated)
      = (int (*)(const char *, char, int, stream_t *, int))
      namespaceSwitchGet(NSSWITCH_STREAM_OPEN_BACKEND).func;

    fileID = streamOpenDelegate(filename, filemode, filetype, streamptr, 1);
  }

  if (fileID < 0)
    {
      Free(streamptr->record);
      stream_delete_entry(streamptr);
      streamID = fileID;
    }
  else
    {
      streamID = streamptr->self;

      if ( streamID < 0 ) return (CDI_ELIMIT);

      streamptr->filemode = filemode;
      streamptr->filename = strdupx(filename);
      streamptr->fileID = fileID;

      if ( filemode == 'r' )
        {
          int vlistID = vlistCreate();
          if ( vlistID < 0 ) return(CDI_ELIMIT);

          streamptr->vlistID = vlistID;

          status = cdiInqContents(streamptr);
          if ( status < 0 ) return (status);
          vlist_t *vlistptr = vlist_to_pointer(streamptr->vlistID);
          vlistptr->ntsteps = streamptr->ntsteps;
        }
    }

  return (streamID);
}

static int streamOpen(const char *filename, const char *filemode, int filetype)
{
  if (!filemode || strlen(filemode) != 1) return CDI_EINVAL;
  return streamOpenID(filename, (char)tolower(filemode[0]),
                      filetype, CDI_UNDEFID);
}

static int streamOpenA(const char *filename, const char *filemode, int filetype)
{
  int fileID = CDI_UNDEFID;
  int streamID = CDI_ESYSTEM;
  int status;
  stream_t *streamptr = stream_new_entry(CDI_UNDEFID);
  vlist_t *vlistptr;

  if ( CDI_Debug )
    Message("Open %s file (mode=%c); filename: %s", strfiletype(filetype), (int) *filemode, filename);
  if ( CDI_Debug ) printf("streamOpenA: %s\n", filename);

  if ( ! filename || ! filemode || filetype < 0 ) return (CDI_EINVAL);

  {
    int (*streamOpenDelegate)(const char *filename, char filemode,
                              int filetype, stream_t *streamptr, int recordBufIsToBeCreated)
      = (int (*)(const char *, char, int, stream_t *, int))
      namespaceSwitchGet(NSSWITCH_STREAM_OPEN_BACKEND).func;

    fileID = streamOpenDelegate(filename, 'r', filetype, streamptr, 1);
  }

  if ( fileID == CDI_UNDEFID || fileID == CDI_ELIBNAVAIL || fileID == CDI_ESYSTEM ) return (fileID);

  streamID = streamptr->self;

  streamptr->filemode = tolower(*filemode);
  streamptr->filename = strdupx(filename);
  streamptr->fileID = fileID;

  streamptr->vlistID = vlistCreate();

  status = cdiInqContents(streamptr);
  if ( status < 0 ) return (status);
  vlistptr = vlist_to_pointer(streamptr->vlistID);
  vlistptr->ntsteps = (int)cdiInqTimeSize(streamID);

  {
    void (*streamCloseDelegate)(stream_t *streamptr, int recordBufIsToBeDeleted)
      = (void (*)(stream_t *, int))
      namespaceSwitchGet(NSSWITCH_STREAM_CLOSE_BACKEND).func;

    streamCloseDelegate(streamptr, 0);
  }

  switch (filetype)
    {
#if defined (HAVE_LIBGRIB)
    case FILETYPE_GRB:
    case FILETYPE_GRB2:
      {
        fileID = gribOpen(filename, filemode);
        break;
      }
#endif
#if defined (HAVE_LIBSERVICE)
    case FILETYPE_SRV:
      {
        fileID = fileOpen(filename, filemode);
        break;
      }
#endif
#if defined (HAVE_LIBEXTRA)
    case FILETYPE_EXT:
      {
        fileID = fileOpen(filename, filemode);
        break;
      }
#endif
#if defined (HAVE_LIBIEG)
    case FILETYPE_IEG:
      {
        fileID = fileOpen(filename, filemode);
        break;
      }
#endif
#if defined (HAVE_LIBNETCDF)
    case FILETYPE_NC:
      {
        fileID = cdfOpen(filename, filemode);
        streamptr->ncmode = 2;
        break;
      }
    case FILETYPE_NC2:
      {
        fileID = cdfOpen64(filename, filemode);
        streamptr->ncmode = 2;
        break;
      }
    case FILETYPE_NC4:
    case FILETYPE_NC4C:
      {
        fileID = cdf4Open(filename, filemode, &filetype);
        streamptr->ncmode = 2;
        break;
      }
#endif
    default:
      {
        if ( CDI_Debug ) Message("%s support not compiled in!", strfiletype(filetype));
        return (CDI_ELIBNAVAIL);
      }
    }

  if ( fileID == CDI_UNDEFID )
    streamID = CDI_UNDEFID;
  else
    streamptr->fileID = fileID;

  return (streamID);
}
int streamOpenRead(const char *filename)
{
  cdiInitialize();

  int byteorder = 0;
  int filetype = cdiGetFiletype(filename, &byteorder);

  if ( filetype < 0 ) return (filetype);

  int streamID = streamOpen(filename, "r", filetype);

  if ( streamID >= 0 )
    {
      stream_t *streamptr = stream_to_pointer(streamID);
      streamptr->byteorder = byteorder;
    }

  return (streamID);
}


int streamOpenAppend(const char *filename)
{
  cdiInitialize();

  int byteorder = 0;
  int filetype = cdiGetFiletype(filename, &byteorder);

  if ( filetype < 0 ) return (filetype);

  int streamID = streamOpenA(filename, "a", filetype);

  if ( streamID >= 0 )
    {
      stream_t *streamptr = stream_to_pointer(streamID);
      streamptr->byteorder = byteorder;
    }

  return (streamID);
}
int streamOpenWrite(const char *filename, int filetype)
{
  cdiInitialize();

  return (streamOpen(filename, "w", filetype));
}

static
void streamDefaultValue ( stream_t * streamptr )
{
  streamptr->self = CDI_UNDEFID;
  streamptr->accesstype = CDI_UNDEFID;
  streamptr->accessmode = 0;
  streamptr->filetype = FILETYPE_UNDEF;
  streamptr->byteorder = CDI_UNDEFID;
  streamptr->fileID = 0;
  streamptr->filemode = 0;
  streamptr->numvals = 0;
  streamptr->filename = NULL;
  streamptr->record = NULL;
  streamptr->varsAllocated = 0;
  streamptr->nrecs = 0;
  streamptr->nvars = 0;
  streamptr->vars = NULL;
  streamptr->ncmode = 0;
  streamptr->curTsID = CDI_UNDEFID;
  streamptr->rtsteps = 0;
  streamptr->ntsteps = CDI_UNDEFID;
  streamptr->tsteps = NULL;
  streamptr->tstepsTableSize = 0;
  streamptr->tstepsNextID = 0;
  streamptr->historyID = CDI_UNDEFID;
  streamptr->vlistID = CDI_UNDEFID;
  streamptr->globalatts = 0;
  streamptr->localatts = 0;
  streamptr->vct.ilev = 0;
  streamptr->vct.mlev = 0;
  streamptr->vct.ilevID = CDI_UNDEFID;
  streamptr->vct.mlevID = CDI_UNDEFID;
  streamptr->unreduced = cdiDataUnreduced;
  streamptr->sortname = cdiSortName;
  streamptr->have_missval = cdiHaveMissval;
  streamptr->comptype = COMPRESS_NONE;
  streamptr->complevel = 0;

  basetimeInit(&streamptr->basetime);

  int i;
  for ( i = 0; i < MAX_GRIDS_PS; i++ ) streamptr->xdimID[i] = CDI_UNDEFID;
  for ( i = 0; i < MAX_GRIDS_PS; i++ ) streamptr->ydimID[i] = CDI_UNDEFID;
  for ( i = 0; i < MAX_ZAXES_PS; i++ ) streamptr->zaxisID[i] = CDI_UNDEFID;
  for ( i = 0; i < MAX_ZAXES_PS; i++ ) streamptr->nczvarID[i] = CDI_UNDEFID;
  for ( i = 0; i < MAX_GRIDS_PS; i++ ) streamptr->ncxvarID[i] = CDI_UNDEFID;
  for ( i = 0; i < MAX_GRIDS_PS; i++ ) streamptr->ncyvarID[i] = CDI_UNDEFID;
  for ( i = 0; i < MAX_GRIDS_PS; i++ ) streamptr->ncavarID[i] = CDI_UNDEFID;

  streamptr->gribContainers = NULL;
}


static stream_t *stream_new_entry(int resH)
{
  stream_t *streamptr;

  cdiInitialize();

  streamptr = (stream_t *) Malloc(sizeof(stream_t));
  streamDefaultValue ( streamptr );
  if (resH == CDI_UNDEFID)
    streamptr->self = reshPut(streamptr, &streamOps);
  else
    {
      streamptr->self = resH;
      reshReplace(resH, streamptr, &streamOps);
    }

  return streamptr;
}


void
cdiStreamCloseDefaultDelegate(stream_t *streamptr, int recordBufIsToBeDeleted)
{
  int fileID = streamptr->fileID;
  int filetype = streamptr->filetype;
  if ( fileID == CDI_UNDEFID )
    Warning("File %s not open!", streamptr->filename);
  else
    switch (filetype)
      {
#if defined (HAVE_LIBGRIB)
      case FILETYPE_GRB:
      case FILETYPE_GRB2:
        {
          gribClose(fileID);
          if (recordBufIsToBeDeleted)
            gribContainersDelete(streamptr);
          break;
        }
#endif
#if defined (HAVE_LIBSERVICE)
      case FILETYPE_SRV:
        {
          fileClose(fileID);
          if (recordBufIsToBeDeleted)
            srvDelete(streamptr->record->exsep);
          break;
        }
#endif
#if defined (HAVE_LIBEXTRA)
      case FILETYPE_EXT:
        {
          fileClose(fileID);
          if (recordBufIsToBeDeleted)
            extDelete(streamptr->record->exsep);
          break;
        }
#endif
#if defined (HAVE_LIBIEG)
      case FILETYPE_IEG:
        {
          fileClose(fileID);
          if (recordBufIsToBeDeleted)
            iegDelete(streamptr->record->exsep);
          break;
        }
#endif
#if defined (HAVE_LIBNETCDF)
      case FILETYPE_NC:
      case FILETYPE_NC2:
      case FILETYPE_NC4:
      case FILETYPE_NC4C:
        {
          cdfClose(fileID);
          break;
        }
#endif
      default:
        {
          Error("%s support not compiled in (fileID = %d)!", strfiletype(filetype), fileID);
          break;
        }
      }
}


static void deallocate_sleveltable_t(sleveltable_t *entry)
{
  if (entry->recordID) Free(entry->recordID);
  if (entry->lindex) Free(entry->lindex);
  entry->recordID = NULL;
  entry->lindex = NULL;
}
void streamClose(int streamID)
{
  int index;
  stream_t *streamptr = stream_to_pointer(streamID);

  if ( CDI_Debug )
    Message("streamID = %d filename = %s", streamID, streamptr->filename);

  int vlistID = streamptr->vlistID;

  void (*streamCloseDelegate)(stream_t *streamptr, int recordBufIsToBeDeleted)
    = (void (*)(stream_t *, int))
    namespaceSwitchGet(NSSWITCH_STREAM_CLOSE_BACKEND).func;

  if ( streamptr->filetype != -1 ) streamCloseDelegate(streamptr, 1);

  if ( streamptr->record )
    {
      if ( streamptr->record->buffer )
        Free(streamptr->record->buffer);

      Free(streamptr->record);
    }

  streamptr->filetype = 0;
  if ( streamptr->filename ) Free(streamptr->filename);

  for ( index = 0; index < streamptr->nvars; index++ )
    {
      sleveltable_t *pslev = streamptr->vars[index].recordTable;
      unsigned nsub = streamptr->vars[index].subtypeSize >= 0
        ? (unsigned)streamptr->vars[index].subtypeSize : 0U;
      for (size_t isub=0; isub < nsub; isub++)
        {
          deallocate_sleveltable_t(pslev + isub);
        }
      if (pslev) Free(pslev);
    }
  Free(streamptr->vars);
  streamptr->vars = NULL;

  for ( index = 0; index < streamptr->ntsteps; ++index )
    {
      if ( streamptr->tsteps[index].records )
        Free(streamptr->tsteps[index].records);
      if ( streamptr->tsteps[index].recIDs )
        Free(streamptr->tsteps[index].recIDs);
      taxisDestroyKernel(&streamptr->tsteps[index].taxis);
    }

  if ( streamptr->tsteps ) Free(streamptr->tsteps);

  if ( streamptr->basetime.timevar_cache ) Free(streamptr->basetime.timevar_cache);

  if ( vlistID != -1 )
    {
      if ( streamptr->filemode != 'w' )
        if ( vlistInqTaxis(vlistID) != -1 )
          {
            taxisDestroy(vlistInqTaxis(vlistID));
          }

      vlist_unlock(vlistID);
      vlistDestroy(vlistID);
    }

  stream_delete_entry(streamptr);
}

static void stream_delete_entry(stream_t *streamptr)
{
  int idx;

  xassert ( streamptr );

  idx = streamptr->self;
  Free( streamptr );
  reshRemove ( idx, &streamOps );

  if ( CDI_Debug )
    Message("Removed idx %d from stream list", idx);
}


void cdiStreamSync_(stream_t *streamptr)
{
  int fileID = streamptr->fileID;
  int filetype = streamptr->filetype;
  int vlistID = streamptr->vlistID;
  int nvars = vlistNvars(vlistID);

  if ( fileID == CDI_UNDEFID )
    Warning("File %s not open!", streamptr->filename);
  else if ( vlistID == CDI_UNDEFID )
    Warning("Vlist undefined for file %s!", streamptr->filename);
  else if ( nvars == 0 )
    Warning("No variables defined!");
  else
    {
      if ( streamptr->filemode == 'w' || streamptr->filemode == 'a' )
        {
          switch (filetype)
            {
#if defined (HAVE_LIBNETCDF)
            case FILETYPE_NC:
            case FILETYPE_NC2:
            case FILETYPE_NC4:
            case FILETYPE_NC4C:
              {
                void cdf_sync(int ncid);
                if ( streamptr->ncmode == 2 ) cdf_sync(fileID);
                break;
              }
#endif
            default:
              {
                fileFlush(fileID);
                break;
              }
            }
        }
    }
}
void streamSync(int streamID)
{
  stream_t *streamptr = stream_to_pointer(streamID);

  void (*myStreamSync_)(stream_t *streamptr)
    = (void (*)(stream_t *))namespaceSwitchGet(NSSWITCH_STREAM_SYNC).func;
  myStreamSync_(streamptr);
}


int cdiStreamDefTimestep_(stream_t *streamptr, int tsID)
{
  int taxisID = 0;

  stream_check_ptr(__func__, streamptr);

  if ( CDI_Debug )
    Message("streamID = %d  tsID = %d", streamptr->self, tsID);

  int vlistID = streamptr->vlistID;

  int time_is_varying = vlistHasTime(vlistID);

  if ( time_is_varying )
    {
      taxisID = vlistInqTaxis(vlistID);
      if ( taxisID == CDI_UNDEFID )
        {
          Warning("taxisID undefined for fileID = %d! Using absolute time axis.", streamptr->self);
          taxisID = taxisCreate(TAXIS_ABSOLUTE);
          vlistDefTaxis(vlistID, taxisID);
        }
    }

  int newtsID = tstepsNewEntry(streamptr);

  if ( tsID != newtsID )
    Error("Internal problem: tsID = %d newtsID = %d", tsID, newtsID);

  streamptr->curTsID = tsID;

  if ( time_is_varying )
    {
      taxis_t *taxisptr1 = taxisPtr(taxisID);
      taxis_t *taxisptr2 = &streamptr->tsteps[tsID].taxis;
      ptaxisCopy(taxisptr2, taxisptr1);
    }

  streamptr->ntsteps = tsID + 1;

#ifdef HAVE_LIBNETCDF
  if ((streamptr->filetype == FILETYPE_NC ||
       streamptr->filetype == FILETYPE_NC2 ||
       streamptr->filetype == FILETYPE_NC4 ||
       streamptr->filetype == FILETYPE_NC4C)
      && time_is_varying)
    {
      void (*myCdfDefTimestep)(stream_t *streamptr, int tsID)
        = (void (*)(stream_t *, int))
        namespaceSwitchGet(NSSWITCH_CDF_DEF_TIMESTEP).func;
      myCdfDefTimestep(streamptr, tsID);
    }
#endif

  cdi_create_records(streamptr, tsID);

  return (int)streamptr->ntsteps;
}
int streamDefTimestep(int streamID, int tsID)
{
  stream_t *streamptr = stream_to_pointer(streamID);
  int (*myStreamDefTimestep_)(stream_t *streamptr, int tsID)
    = (int (*)(stream_t *, int))
    namespaceSwitchGet(NSSWITCH_STREAM_DEF_TIMESTEP_).func;
  return myStreamDefTimestep_(streamptr, tsID);
}

int streamInqCurTimestepID(int streamID)
{
  stream_t *streamptr = stream_to_pointer(streamID);
  return streamptr->curTsID;
}
int streamInqTimestep(int streamID, int tsID)
{
  int nrecs = 0;
  int taxisID;
  stream_t *streamptr = stream_to_pointer(streamID);
  int vlistID = streamptr->vlistID;

  if ( tsID < streamptr->rtsteps )
    {
      streamptr->curTsID = tsID;
      nrecs = streamptr->tsteps[tsID].nrecs;
      streamptr->tsteps[tsID].curRecID = CDI_UNDEFID;
      taxisID = vlistInqTaxis(vlistID);
      if ( taxisID == -1 )
        Error("Timestep undefined for fileID = %d", streamID);
      ptaxisCopy(taxisPtr(taxisID), &streamptr->tsteps[tsID].taxis);

      return (nrecs);
    }

  if ( tsID >= streamptr->ntsteps && streamptr->ntsteps != CDI_UNDEFID )
    {
      return (0);
    }

  int filetype = streamptr->filetype;

  if ( CDI_Debug )
    Message("streamID = %d  tsID = %d  filetype = %d", streamID, tsID, filetype);

  switch (filetype)
    {
#if defined (HAVE_LIBGRIB)
    case FILETYPE_GRB:
    case FILETYPE_GRB2:
      {
        nrecs = grbInqTimestep(streamptr, tsID);
        break;
      }
#endif
#if defined (HAVE_LIBSERVICE)
    case FILETYPE_SRV:
      {
        nrecs = srvInqTimestep(streamptr, tsID);
        break;
      }
#endif
#if defined (HAVE_LIBEXTRA)
    case FILETYPE_EXT:
      {
        nrecs = extInqTimestep(streamptr, tsID);
        break;
      }
#endif
#if defined (HAVE_LIBIEG)
    case FILETYPE_IEG:
      {
        nrecs = iegInqTimestep(streamptr, tsID);
        break;
      }
#endif
#if defined (HAVE_LIBNETCDF)
    case FILETYPE_NC:
    case FILETYPE_NC2:
    case FILETYPE_NC4:
    case FILETYPE_NC4C:
      {
        nrecs = cdfInqTimestep(streamptr, tsID);
        break;
      }
#endif
    default:
      {
        Error("%s support not compiled in!", strfiletype(filetype));
        break;
      }
    }

  taxisID = vlistInqTaxis(vlistID);
  if ( taxisID == -1 )
    Error("Timestep undefined for fileID = %d", streamID);

  ptaxisCopy(taxisPtr(taxisID), &streamptr->tsteps[tsID].taxis);

  return (nrecs);
}

#if 0
void streamWriteContents(int streamID, char *cname)
{
  stream_t *streamptr = stream_to_pointer(streamID);

  int vlistID = streamptr->vlistID;

  FILE *cnp = fopen(cname, "w");

  if ( cnp == NULL ) SysError(cname);

  fprintf(cnp, "#CDI library version %s\n", cdiLibraryVersion());
  fprintf(cnp, "#\n");

  fprintf(cnp, "filename: %s\n", streamptr->filename);
  int filetype = streamptr->filetype;
  fprintf(cnp, "filetype: %s\n", strfiletype(filetype));

  fprintf(cnp, "#\n");
  fprintf(cnp, "#grids:\n");

  int ngrids = vlistNgrids(vlistID);
  for ( int i = 0; i < ngrids; i++ )
    {
      int gridID = vlistGrid(vlistID, i);
      int gridtype = gridInqType(gridID);
      int xsize = gridInqXsize(gridID);
      int ysize = gridInqYsize(gridID);
      fprintf(cnp, "%4d:%4d:%4d:%4d\n", i+1, gridtype, xsize, ysize);
    }

  fprintf(cnp, "#\n");

  fprintf(cnp, "varID:code:gridID:zaxisID:tsteptype:datatype\n");

  int nvars = vlistNvars(vlistID);
  for ( int varID = 0; varID < nvars; varID++ )
    {
      int code = vlistInqVarCode(vlistID, varID);
      int gridID = vlistInqVarGrid(vlistID, varID);
      int zaxisID = vlistInqVarZaxis(vlistID, varID);
      int tsteptype = vlistInqVarTsteptype(vlistID, varID);
      int datatype = vlistInqVarDatatype(vlistID, varID);
      fprintf(cnp, "%4d:%4d:%4d:%4d:%4d:%4d:\n",
              varID+1, code, gridID, zaxisID, tsteptype, datatype);
    }

  fprintf(cnp, "#\n");

  fprintf(cnp, "tsID:nrecs:date:time\n");

  int tsID = 0;
  while (1)
    {
      int nrecs = streamptr->tsteps[tsID].nallrecs;
      int date = streamptr->tsteps[tsID].taxis.vdate;
      int time = streamptr->tsteps[tsID].taxis.vtime;
      off_t position = streamptr->tsteps[tsID].position;

      fprintf(cnp, "%4d:%4d:%4d:%4d:%ld\n",
              tsID, nrecs, date, time, (long) position);

      if ( streamptr->tsteps[tsID].next )
        tsID++;
      else
        break;
    }

  fprintf(cnp, "#\n");

  fprintf(cnp, "tsID:recID:varID:levID:size:pos\n");

  tsID = 0;
  while (1)
    {
      int nrecs = streamptr->tsteps[tsID].nallrecs;
      for ( int recID = 0; recID < nrecs; recID++ )
        {
          int varID = streamptr->tsteps[tsID].records[recID].varID;
          int levelID = streamptr->tsteps[tsID].records[recID].levelID;
          off_t recpos = streamptr->tsteps[tsID].records[recID].position;
          long recsize = (long)streamptr->tsteps[tsID].records[recID].size;
          fprintf(cnp, "%4d:%4d:%4d:%4d:%4ld:%ld\n",
                  tsID, recID, varID, levelID, recsize, (long) recpos);
        }

      if ( streamptr->tsteps[tsID].next )
        tsID++;
      else
        break;
    }

  fclose(cnp);
}
#endif


off_t streamNvals(int streamID)
{
  stream_t *streamptr = stream_to_pointer(streamID);

  return (streamptr->numvals);
}
void streamDefVlist(int streamID, int vlistID)
{
  void (*myStreamDefVlist)(int streamID, int vlistID)
    = (void (*)(int, int))namespaceSwitchGet(NSSWITCH_STREAM_DEF_VLIST_).func;
  myStreamDefVlist(streamID, vlistID);
}


void
cdiStreamDefVlist_(int streamID, int vlistID)
{
  stream_t *streamptr = stream_to_pointer(streamID);

  if ( streamptr->vlistID == CDI_UNDEFID )
    cdiStreamSetupVlist(streamptr, vlistDuplicate(vlistID));
  else
    Warning("vlist already defined for %s!", streamptr->filename);
}
int streamInqVlist(int streamID)
{
  stream_t *streamptr = stream_to_pointer(streamID);
  return (streamptr->vlistID);
}


void streamDefCompType(int streamID, int comptype)
{
  stream_t *streamptr = stream_to_pointer(streamID);
  if (streamptr->comptype != comptype)
    {
      streamptr->comptype = comptype;
      reshSetStatus(streamID, &streamOps, RESH_DESYNC_IN_USE);
    }
}


void streamDefCompLevel(int streamID, int complevel)
{
  stream_t *streamptr = stream_to_pointer(streamID);
  if (streamptr->complevel != complevel)
    {
      streamptr->complevel = complevel;
      reshSetStatus(streamID, &streamOps, RESH_DESYNC_IN_USE);
    }
}


int streamInqCompType(int streamID)
{
  stream_t *streamptr = stream_to_pointer(streamID);
  return (streamptr->comptype);
}


int streamInqCompLevel(int streamID)
{
  stream_t *streamptr = stream_to_pointer(streamID);
  return (streamptr->complevel);
}

int streamInqFileID(int streamID)
{
  stream_t *streamptr;

  streamptr = ( stream_t *) reshGetVal ( streamID, &streamOps );

  return (streamptr->fileID);
}


void cdiDefAccesstype(int streamID, int type)
{
  stream_t *streamptr = (stream_t *)reshGetVal(streamID, &streamOps);

  if ( streamptr->accesstype == CDI_UNDEFID )
    {
      streamptr->accesstype = type;
    }
  else if ( streamptr->accesstype != type )
    Error("Changing access type from %s not allowed!",
          streamptr->accesstype == TYPE_REC ? "REC to VAR" : "VAR to REC");
}


int cdiInqAccesstype(int streamID)
{
  stream_t *streamptr = (stream_t *) reshGetVal ( streamID, &streamOps );

  return (streamptr->accesstype);
}

static
int streamTxCode(void)
{
  return STREAM;
}

void
cdiStreamSetupVlist(stream_t *streamptr, int vlistID)
{
  void (*myStreamSetupVlist)(stream_t *streamptr, int vlistID)
    = (void (*)(stream_t *, int))
    namespaceSwitchGet(NSSWITCH_STREAM_SETUP_VLIST).func;
  myStreamSetupVlist(streamptr, vlistID);
}

void
cdiStreamSetupVlist_(stream_t *streamptr, int vlistID)
{
  vlist_lock(vlistID);
  int nvars = vlistNvars(vlistID);
  streamptr->vlistID = vlistID;
  for (int varID = 0; varID < nvars; varID++ )
    {
      int gridID = vlistInqVarGrid(vlistID, varID);
      int zaxisID = vlistInqVarZaxis(vlistID, varID);
      int tilesetID = vlistInqVarSubtype(vlistID, varID);
      stream_new_var(streamptr, gridID, zaxisID, tilesetID);
      if ( streamptr->have_missval )
        vlistDefVarMissval(vlistID, varID,
                           vlistInqVarMissval(vlistID, varID));
    }

  if (streamptr->filemode == 'w')
    switch (streamptr->filetype)
      {
#ifdef HAVE_LIBNETCDF
      case FILETYPE_NC:
      case FILETYPE_NC2:
      case FILETYPE_NC4:
      case FILETYPE_NC4C:
        {
          void (*myCdfDefVars)(stream_t *streamptr)
            = (void (*)(stream_t *))
            namespaceSwitchGet(NSSWITCH_CDF_STREAM_SETUP).func;
          myCdfDefVars(streamptr);
        }
        break;
#endif
#ifdef HAVE_LIBGRIB
      case FILETYPE_GRB:
      case FILETYPE_GRB2:
        gribContainersNew(streamptr);
        break;
#endif
      default:
        ;
      }
}


void cdiStreamGetIndexList(unsigned numIDs, int *IDs)
{
  reshGetResHListOfType(numIDs, IDs, &streamOps);
}

int streamInqNvars ( int streamID )
{
  stream_t *streamptr = (stream_t *)reshGetVal(streamID, &streamOps);
  return streamptr->nvars;
}


static int streamCompareP(void * streamptr1, void * streamptr2)
{
  stream_t * s1 = ( stream_t * ) streamptr1;
  stream_t * s2 = ( stream_t * ) streamptr2;
  enum {
    differ = -1,
    equal = 0,
  };

  xassert ( s1 );
  xassert ( s2 );

  if ( s1->filetype != s2->filetype ) return differ;
  if ( s1->byteorder != s2->byteorder ) return differ;
  if ( s1->comptype != s2->comptype ) return differ;
  if ( s1->complevel != s2->complevel ) return differ;

  if ( s1->filename )
    {
      if (strcmp(s1->filename, s2->filename))
        return differ;
    }
  else if ( s2->filename )
    return differ;

  return equal;
}


void streamDestroyP ( void * streamptr )
{
  stream_t * sp = ( stream_t * ) streamptr;

  xassert ( sp );

  int id = sp->self;
  streamClose ( id );
}


void streamPrintP ( void * streamptr, FILE * fp )
{
  stream_t * sp = ( stream_t * ) streamptr;

  if ( !sp ) return;

  fprintf(fp, "#\n"
          "# streamID %d\n"
          "#\n"
          "self          = %d\n"
          "accesstype    = %d\n"
          "accessmode    = %d\n"
          "filetype      = %d\n"
          "byteorder     = %d\n"
          "fileID        = %d\n"
          "filemode      = %d\n"
          "filename      = %s\n"
          "nrecs         = %d\n"
          "nvars         = %d\n"
          "varsAllocated = %d\n"
          "curTsID       = %d\n"
          "rtsteps       = %d\n"
          "ntsteps       = %ld\n"
          "tstepsTableSize= %d\n"
          "tstepsNextID  = %d\n"
          "ncmode        = %d\n"
          "vlistID       = %d\n"
          "historyID     = %d\n"
          "globalatts    = %d\n"
          "localatts     = %d\n"
          "unreduced     = %d\n"
          "sortname      = %d\n"
          "have_missval  = %d\n"
          "ztype         = %d\n"
          "zlevel        = %d\n",
          sp->self, sp->self, sp->accesstype, sp->accessmode,
          sp->filetype, sp->byteorder, sp->fileID, sp->filemode,
          sp->filename, sp->nrecs, sp->nvars, sp->varsAllocated,
          sp->curTsID, sp->rtsteps, sp->ntsteps, sp->tstepsTableSize,
          sp->tstepsNextID, sp->ncmode, sp->vlistID, sp->historyID,
          sp->globalatts, sp->localatts, sp->unreduced, sp->sortname,
          sp->have_missval, sp->comptype, sp->complevel);
}

enum {
  streamNint = 10,
};

static int
streamGetPackSize(void * voidP, void *context)
{
  stream_t * streamP = ( stream_t * ) voidP;
  int packBufferSize
    = serializeGetSize(streamNint, DATATYPE_INT, context)
    + serializeGetSize(2, DATATYPE_UINT32, context)
    + serializeGetSize((int)strlen(streamP->filename) + 1,
                       DATATYPE_TXT, context)
    + serializeGetSize(1, DATATYPE_FLT64, context);
  return packBufferSize;
}


static void
streamPack(void * streamptr, void * packBuffer, int packBufferSize,
           int * packBufferPos, void *context)
{
  stream_t * streamP = ( stream_t * ) streamptr;
  int intBuffer[streamNint];

  intBuffer[0] = streamP->self;
  intBuffer[1] = streamP->filetype;
  intBuffer[2] = (int)strlen(streamP->filename) + 1;
  intBuffer[3] = streamP->vlistID;
  intBuffer[4] = streamP->byteorder;
  intBuffer[5] = streamP->comptype;
  intBuffer[6] = streamP->complevel;
  intBuffer[7] = streamP->unreduced;
  intBuffer[8] = streamP->sortname;
  intBuffer[9] = streamP->have_missval;

  serializePack(intBuffer, streamNint, DATATYPE_INT, packBuffer, packBufferSize, packBufferPos, context);
  uint32_t d = cdiCheckSum(DATATYPE_INT, streamNint, intBuffer);
  serializePack(&d, 1, DATATYPE_UINT32, packBuffer, packBufferSize, packBufferPos, context);

  serializePack(&cdiDefaultMissval, 1, DATATYPE_FLT64, packBuffer, packBufferSize, packBufferPos, context);
  serializePack(streamP->filename, intBuffer[2], DATATYPE_TXT, packBuffer, packBufferSize, packBufferPos, context);
  d = cdiCheckSum(DATATYPE_TXT, intBuffer[2], streamP->filename);
  serializePack(&d, 1, DATATYPE_UINT32, packBuffer, packBufferSize, packBufferPos, context);
}

struct streamAssoc
streamUnpack(char * unpackBuffer, int unpackBufferSize,
             int * unpackBufferPos, int originNamespace, void *context)
{
  int intBuffer[streamNint];
  uint32_t d;
  char filename[CDI_MAX_NAME];

  serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                  intBuffer, streamNint, DATATYPE_INT, context);
  serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                  &d, 1, DATATYPE_UINT32, context);
  xassert(cdiCheckSum(DATATYPE_INT, streamNint, intBuffer) == d);

  serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                  &cdiDefaultMissval, 1, DATATYPE_FLT64, context);
  serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                  &filename, intBuffer[2], DATATYPE_TXT, context);
  serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                  &d, 1, DATATYPE_UINT32, context);
  xassert(d == cdiCheckSum(DATATYPE_TXT, intBuffer[2], filename));
  int targetStreamID = namespaceAdaptKey(intBuffer[0], originNamespace),
    streamID = streamOpenID(filename, 'w', intBuffer[1], targetStreamID);
  xassert(streamID >= 0 && targetStreamID == streamID);
  streamDefByteorder(streamID, intBuffer[4]);
  streamDefCompType(streamID, intBuffer[5]);
  streamDefCompLevel(streamID, intBuffer[6]);
  stream_t *streamptr = stream_to_pointer(streamID);
  streamptr->unreduced = intBuffer[7];
  streamptr->sortname = intBuffer[8];
  streamptr->have_missval = intBuffer[9];
  struct streamAssoc retval = { streamID, intBuffer[3] };
  return retval;
}
#if defined (HAVE_CONFIG_H)
#endif




void cdiStreamWriteVar_(int streamID, int varID, int memtype, const void *data, int nmiss)
{
  if ( CDI_Debug ) Message("streamID = %d varID = %d", streamID, varID);

  check_parg(data);

  stream_t *streamptr = stream_to_pointer(streamID);
  if (subtypeInqActiveIndex(streamptr->vars[varID].subtypeID) != 0)
    Error("Writing of non-trivial subtypes not yet implemented!");


  if ( streamptr->curTsID == CDI_UNDEFID ) streamDefTimestep(streamID, 0);

  int filetype = streamptr->filetype;

  switch (filetype)
    {
#if defined (HAVE_LIBGRIB)
    case FILETYPE_GRB:
    case FILETYPE_GRB2:
      {
        grb_write_var(streamptr, varID, memtype, data, nmiss);
        break;
      }
#endif
#if defined (HAVE_LIBSERVICE)
    case FILETYPE_SRV:
      {
        if ( memtype == MEMTYPE_FLOAT ) Error("srvWriteVar not implemented for memtype float!");
        srvWriteVarDP(streamptr, varID, (double *)data);
        break;
      }
#endif
#if defined (HAVE_LIBEXTRA)
    case FILETYPE_EXT:
      {
        if ( memtype == MEMTYPE_FLOAT ) Error("extWriteVar not implemented for memtype float!");
        extWriteVarDP(streamptr, varID, (double *)data);
        break;
      }
#endif
#if defined (HAVE_LIBIEG)
    case FILETYPE_IEG:
      {
        if ( memtype == MEMTYPE_FLOAT ) Error("iegWriteVar not implemented for memtype float!");
        iegWriteVarDP(streamptr, varID, (double *)data);
        break;
      }
#endif
#if defined (HAVE_LIBNETCDF)
    case FILETYPE_NC:
    case FILETYPE_NC2:
    case FILETYPE_NC4:
    case FILETYPE_NC4C:
      {
        if ( streamptr->accessmode == 0 ) cdfEndDef(streamptr);

        break;
      }
#endif
    default:
      {
        Error("%s support not compiled in!", strfiletype(filetype));
        break;
      }
    }
}
void streamWriteVar(int streamID, int varID, const double *data, int nmiss)
{
  void (*myCdiStreamWriteVar_)(int streamID, int varID, int memtype,
                               const void *data, int nmiss)
    = (void (*)(int, int, int, const void *, int))
    namespaceSwitchGet(NSSWITCH_STREAM_WRITE_VAR_).func;
  myCdiStreamWriteVar_(streamID, varID, MEMTYPE_DOUBLE, data, nmiss);
}
void streamWriteVarF(int streamID, int varID, const float *data, int nmiss)
{
  void (*myCdiStreamWriteVar_)(int streamID, int varID, int memtype,
                               const void *data, int nmiss)
    = (void (*)(int, int, int, const void *, int))
    namespaceSwitchGet(NSSWITCH_STREAM_WRITE_VAR_).func;
  myCdiStreamWriteVar_(streamID, varID, MEMTYPE_FLOAT, data, nmiss);
}

static
void cdiStreamWriteVarSlice(int streamID, int varID, int levelID, int memtype, const void *data, int nmiss)
{
  if ( CDI_Debug ) Message("streamID = %d varID = %d", streamID, varID);

  check_parg(data);

  stream_t *streamptr = stream_to_pointer(streamID);
  if (subtypeInqActiveIndex(streamptr->vars[varID].subtypeID) != 0)
    Error("Writing of non-trivial subtypes not yet implemented!");


  if ( streamptr->curTsID == CDI_UNDEFID ) streamDefTimestep(streamID, 0);

  int filetype = streamptr->filetype;

  switch (filetype)
    {
#if defined (HAVE_LIBGRIB)
    case FILETYPE_GRB:
    case FILETYPE_GRB2:
      {
        grb_write_var_slice(streamptr, varID, levelID, memtype, data, nmiss);
        break;
      }
#endif
#if defined (HAVE_LIBSERVICE)
    case FILETYPE_SRV:
      {
        if ( memtype == MEMTYPE_FLOAT ) Error("srvWriteVarSlice not implemented for memtype float!");
        srvWriteVarSliceDP(streamptr, varID, levelID, (double *)data);
        break;
      }
#endif
#if defined (HAVE_LIBEXTRA)
    case FILETYPE_EXT:
      {
        if ( memtype == MEMTYPE_FLOAT ) Error("extWriteVarSlice not implemented for memtype float!");
        extWriteVarSliceDP(streamptr, varID, levelID, (double *)data);
        break;
      }
#endif
#if defined (HAVE_LIBIEG)
    case FILETYPE_IEG:
      {
        if ( memtype == MEMTYPE_FLOAT ) Error("iegWriteVarSlice not implemented for memtype float!");
        iegWriteVarSliceDP(streamptr, varID, levelID, (double *)data);
        break;
      }
#endif
#if defined (HAVE_LIBNETCDF)
    case FILETYPE_NC:
    case FILETYPE_NC2:
    case FILETYPE_NC4:
    case FILETYPE_NC4C:
      if ( streamptr->accessmode == 0 ) cdfEndDef(streamptr);

      break;
#endif
    default:
      {
        Error("%s support not compiled in!", strfiletype(filetype));
        break;
      }
    }
}
void streamWriteVarSlice(int streamID, int varID, int levelID, const double *data, int nmiss)
{
  cdiStreamWriteVarSlice(streamID, varID, levelID, MEMTYPE_DOUBLE, data, nmiss);
}
void streamWriteVarSliceF(int streamID, int varID, int levelID, const float *data, int nmiss)
{
  cdiStreamWriteVarSlice(streamID, varID, levelID, MEMTYPE_FLOAT, data, nmiss);
}


void
streamWriteVarChunk(int streamID, int varID,
                    const int rect[][2], const double *data, int nmiss)
{
  void (*myCdiStreamWriteVarChunk_)(int streamID, int varID, int memtype,
                                    const int rect[][2], const void *data,
                                    int nmiss)
    = (void (*)(int, int, int, const int [][2], const void *, int))
    namespaceSwitchGet(NSSWITCH_STREAM_WRITE_VAR_CHUNK_).func;
  myCdiStreamWriteVarChunk_(streamID, varID, MEMTYPE_DOUBLE, rect, data, nmiss);
}


void
cdiStreamWriteVarChunk_(int streamID, int varID, int memtype,
                        const int rect[][2], const void *data, int nmiss)
{
  if ( CDI_Debug ) Message("streamID = %d varID = %d", streamID, varID);

  stream_t *streamptr = stream_to_pointer(streamID);



  int filetype = streamptr->filetype;

  switch (filetype)
    {
#if defined (HAVE_LIBGRIB)
    case FILETYPE_GRB:
    case FILETYPE_GRB2:
#endif
#if defined (HAVE_LIBSERVICE)
    case FILETYPE_SRV:
#endif
#if defined (HAVE_LIBEXTRA)
    case FILETYPE_EXT:
#endif
#if defined (HAVE_LIBIEG)
    case FILETYPE_IEG:
#endif
#if defined (HAVE_LIBGRIB) || defined (HAVE_LIBSERVICE) \
  || defined (HAVE_LIBEXTRA) || defined (HAVE_LIBIEG)
      xabort("streamWriteVarChunk not implemented for filetype %s!",
             strfiletype(filetype));
      break;
#endif
#if defined (HAVE_LIBNETCDF)
    case FILETYPE_NC:
    case FILETYPE_NC2:
    case FILETYPE_NC4:
    case FILETYPE_NC4C:
      if ( streamptr->accessmode == 0 ) cdfEndDef(streamptr);

      break;
#endif
    default:
      Error("%s support not compiled in!", strfiletype(filetype));
      break;
    }
}

static
void stream_write_record(int streamID, int memtype, const void *data, int nmiss)
{
  check_parg(data);

  stream_t *streamptr = stream_to_pointer(streamID);

  switch (streamptr->filetype)
    {
#if defined (HAVE_LIBGRIB)
    case FILETYPE_GRB:
    case FILETYPE_GRB2:
      grb_write_record(streamptr, memtype, data, nmiss);
      break;
#endif
#if defined (HAVE_LIBSERVICE)
    case FILETYPE_SRV:
      if ( memtype == MEMTYPE_FLOAT ) Error("srvWriteRecord not implemented for memtype float!");
      srvWriteRecord(streamptr, (const double *)data);
      break;
#endif
#if defined (HAVE_LIBEXTRA)
    case FILETYPE_EXT:
      if ( memtype == MEMTYPE_FLOAT ) Error("extWriteRecord not implemented for memtype float!");
      extWriteRecord(streamptr, (const double *)data);
      break;
#endif
#if defined (HAVE_LIBIEG)
    case FILETYPE_IEG:
      if ( memtype == MEMTYPE_FLOAT ) Error("iegWriteRecord not implemented for memtype float!");
      iegWriteRecord(streamptr, (const double *)data);
      break;
#endif
#if defined (HAVE_LIBNETCDF)
    case FILETYPE_NC:
    case FILETYPE_NC2:
    case FILETYPE_NC4:
    case FILETYPE_NC4C:
      {

        break;
      }
#endif
    default:
      {
        Error("%s support not compiled in!", strfiletype(streamptr->filetype));
        break;
      }
    }
}
void streamWriteRecord(int streamID, const double *data, int nmiss)
{
  stream_write_record(streamID, MEMTYPE_DOUBLE, (const void *) data, nmiss);
}

void streamWriteRecordF(int streamID, const float *data, int nmiss)
{
  stream_write_record(streamID, MEMTYPE_FLOAT, (const void *) data, nmiss);
}

#if defined (HAVE_CONFIG_H)
#endif




static
void cdiStreamReadVar(int streamID, int varID, int memtype, void *data, int *nmiss)
{
  if ( CDI_Debug ) Message("streamID = %d  varID = %d", streamID, varID);

  check_parg(data);
  check_parg(nmiss);

  stream_t *streamptr = stream_to_pointer(streamID);
  int filetype = streamptr->filetype;

  *nmiss = 0;

  switch (filetype)
    {
#if defined (HAVE_LIBGRIB)
    case FILETYPE_GRB:
    case FILETYPE_GRB2:
      {
        if ( memtype == MEMTYPE_FLOAT ) Error("grbReadVar not implemented for memtype float!");
        grbReadVarDP(streamptr, varID, (double *)data, nmiss);
        break;
      }
#endif
#if defined (HAVE_LIBSERVICE)
    case FILETYPE_SRV:
      {
        if ( memtype == MEMTYPE_FLOAT ) Error("srvReadVar not implemented for memtype float!");
        srvReadVarDP(streamptr, varID, (double *)data, nmiss);
        break;
      }
#endif
#if defined (HAVE_LIBEXTRA)
    case FILETYPE_EXT:
      {
        if ( memtype == MEMTYPE_FLOAT ) Error("extReadVar not implemented for memtype float!");
        extReadVarDP(streamptr, varID, (double *)data, nmiss);
        break;
      }
#endif
#if defined (HAVE_LIBIEG)
    case FILETYPE_IEG:
      {
        if ( memtype == MEMTYPE_FLOAT ) Error("iegReadVar not implemented for memtype float!");
        iegReadVarDP(streamptr, varID, (double *)data, nmiss);
        break;
      }
#endif
#if defined (HAVE_LIBNETCDF)
    case FILETYPE_NC:
    case FILETYPE_NC2:
    case FILETYPE_NC4:
    case FILETYPE_NC4C:
      {
        if ( memtype == MEMTYPE_FLOAT )
          cdfReadVarSP(streamptr, varID, (float *)data, nmiss);
        else
          cdfReadVarDP(streamptr, varID, (double *)data, nmiss);

        break;
      }
#endif
    default:
      {
        Error("%s support not compiled in!", strfiletype(filetype));
        break;
      }
    }
}
void streamReadVar(int streamID, int varID, double *data, int *nmiss)
{
  cdiStreamReadVar(streamID, varID, MEMTYPE_DOUBLE, data, nmiss);
}
void streamReadVarF(int streamID, int varID, float *data, int *nmiss)
{
  cdiStreamReadVar(streamID, varID, MEMTYPE_FLOAT, data, nmiss);
}


static
int cdiStreamReadVarSlice(int streamID, int varID, int levelID, int memtype, void *data, int *nmiss)
{


  int status = 0;

  if ( CDI_Debug ) Message("streamID = %d  varID = %d", streamID, varID);

  check_parg(data);
  check_parg(nmiss);

  stream_t *streamptr = stream_to_pointer(streamID);
  int filetype = streamptr->filetype;

  *nmiss = 0;

  switch (filetype)
    {
#if defined (HAVE_LIBGRIB)
    case FILETYPE_GRB:
    case FILETYPE_GRB2:
      {
        if ( memtype == MEMTYPE_FLOAT ) return 1;
        grbReadVarSliceDP(streamptr, varID, levelID, (double *)data, nmiss);
        break;
      }
#endif
#if defined (HAVE_LIBSERVICE)
    case FILETYPE_SRV:
      {
        if ( memtype == MEMTYPE_FLOAT ) return 1;
        srvReadVarSliceDP(streamptr, varID, levelID, (double *)data, nmiss);
        break;
      }
#endif
#if defined (HAVE_LIBEXTRA)
    case FILETYPE_EXT:
      {
        if ( memtype == MEMTYPE_FLOAT ) return 1;
        extReadVarSliceDP(streamptr, varID, levelID, (double *)data, nmiss);
        break;
      }
#endif
#if defined (HAVE_LIBIEG)
    case FILETYPE_IEG:
      {
        if ( memtype == MEMTYPE_FLOAT ) return 1;
        iegReadVarSliceDP(streamptr, varID, levelID, (double *)data, nmiss);
        break;
      }
#endif
#if defined (HAVE_LIBNETCDF)
    case FILETYPE_NC:
    case FILETYPE_NC2:
    case FILETYPE_NC4:
    case FILETYPE_NC4C:
      {
        if ( memtype == MEMTYPE_FLOAT )
          cdfReadVarSliceSP(streamptr, varID, levelID, (float *)data, nmiss);
        else
          cdfReadVarSliceDP(streamptr, varID, levelID, (double *)data, nmiss);
        break;
      }
#endif
    default:
      {
        Error("%s support not compiled in!", strfiletype(filetype));
        status = 2;
        break;
      }
    }

  return status;
}
void streamReadVarSlice(int streamID, int varID, int levelID, double *data, int *nmiss)
{
  if ( cdiStreamReadVarSlice(streamID, varID, levelID, MEMTYPE_DOUBLE, data, nmiss) )
    {
      Warning("Unexpected error returned from cdiStreamReadVarSlice()!");
      size_t elementCount = (size_t)gridInqSize(vlistInqVarGrid(streamInqVlist(streamID), varID));
      memset(data, 0, elementCount * sizeof(*data));
    }
}
void streamReadVarSliceF(int streamID, int varID, int levelID, float *data, int *nmiss)
{
  if ( cdiStreamReadVarSlice(streamID, varID, levelID, MEMTYPE_FLOAT, data, nmiss) )
    {


      size_t elementCount = (size_t)gridInqSize(vlistInqVarGrid(streamInqVlist(streamID), varID));
      double *conversionBuffer = (double *) Malloc(elementCount * sizeof(*conversionBuffer));
      streamReadVarSlice(streamID, varID, levelID, conversionBuffer, nmiss);
      for (size_t i = elementCount; i--; ) data[i] = (float)conversionBuffer[i];
      Free(conversionBuffer);
    }
}

static
void stream_read_record(int streamID, int memtype, void *data, int *nmiss)
{
  check_parg(data);
  check_parg(nmiss);

  stream_t *streamptr = stream_to_pointer(streamID);

  *nmiss = 0;

  switch (streamptr->filetype)
    {
#if defined (HAVE_LIBGRIB)
    case FILETYPE_GRB:
    case FILETYPE_GRB2:
      if ( memtype == MEMTYPE_FLOAT ) Error("grbReadRecord not implemented for memtype float!");
      grbReadRecord(streamptr, data, nmiss);
      break;
#endif
#if defined (HAVE_LIBSERVICE)
    case FILETYPE_SRV:
      if ( memtype == MEMTYPE_FLOAT ) Error("srvReadRecord not implemented for memtype float!");
      srvReadRecord(streamptr, data, nmiss);
      break;
#endif
#if defined (HAVE_LIBEXTRA)
    case FILETYPE_EXT:
      if ( memtype == MEMTYPE_FLOAT ) Error("extReadRecord not implemented for memtype float!");
      extReadRecord(streamptr, data, nmiss);
      break;
#endif
#if defined (HAVE_LIBIEG)
    case FILETYPE_IEG:
      if ( memtype == MEMTYPE_FLOAT ) Error("iegReadRecord not implemented for memtype float!");
      iegReadRecord(streamptr, data, nmiss);
      break;
#endif
#if defined (HAVE_LIBNETCDF)
    case FILETYPE_NC:
    case FILETYPE_NC2:
    case FILETYPE_NC4:
    case FILETYPE_NC4C:
      cdf_read_record(streamptr, memtype, data, nmiss);
      break;
#endif
    default:
      {
        Error("%s support not compiled in!", strfiletype(streamptr->filetype));
        break;
      }
    }
}


void streamReadRecord(int streamID, double *data, int *nmiss)
{
  stream_read_record(streamID, MEMTYPE_DOUBLE, (void *) data, nmiss);
}


void streamReadRecordF(int streamID, float *data, int *nmiss)
{
  stream_read_record(streamID, MEMTYPE_FLOAT, (void *) data, nmiss);
}
#ifndef _VARSCAN_H
#define _VARSCAN_H

#ifndef _GRID_H
#endif


void varAddRecord(int recID, int param, int gridID, int zaxistype, int lbounds,
                  int level1, int level2, int level_sf, int level_unit, int prec,
                  int *pvarID, int *plevelID, int tsteptype, int numavg, int ltype1, int ltype2,
                  const char *name, const char *stdname, const char *longname, const char *units,
                  const var_tile_t *tiles, int *tile_index);

void varDefVCT(size_t vctsize, double *vctptr);
void varDefZAxisReference(int nlev, int nvgrid, unsigned char uuid[CDI_UUID_SIZE]);

int varDefZaxis(int vlistID, int zaxistype, int nlevels, double *levels, int lbounds,
                 double *levels1, double *levels2, int vctsize, double *vct, char *name,
                 char *longname, const char *units, int prec, int mode, int ltype);

void varDefMissval(int varID, double missval);
void varDefCompType(int varID, int comptype);
void varDefCompLevel(int varID, int complevel);
void varDefInst(int varID, int instID);
int varInqInst(int varID);
void varDefModel(int varID, int modelID);
int varInqModel(int varID);
void varDefTable(int varID, int tableID);
int varInqTable(int varID);
void varDefEnsembleInfo(int varID, int ens_idx, int ens_count, int forecast_type);

void varDefTypeOfGeneratingProcess(int varID, int typeOfGeneratingProcess);
void varDefProductDefinitionTemplate(int varID, int productDefinitionTemplate);


void varDefOptGribInt(int varID, int tile_index, long lval, const char *keyword);
void varDefOptGribDbl(int varID, int tile_index, double dval, const char *keyword);
int varOptGribNentries(int varID);

int zaxisCompare(int zaxisID, int zaxistype, int nlevels, int lbounds, const double *levels, const char *longname, const char *units, int ltype);

#endif
#if defined (HAVE_CONFIG_H)
#endif

#ifdef HAVE_LIBNETCDF



#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>

#include <netcdf.h>



extern int CDI_cmor_mode;

#undef UNDEFID
#define UNDEFID CDI_UNDEFID

#define BNDS_NAME "bnds"

#define X_AXIS 1
#define Y_AXIS 2
#define Z_AXIS 3
#define T_AXIS 4

#define POSITIVE_UP 1
#define POSITIVE_DOWN 2

typedef struct {
  int ncvarid;
  int dimtype;
  size_t len;
  char name[CDI_MAX_NAME];
}
ncdim_t;
#define MAX_COORDVARS 4
#define MAX_AUXVARS 4

typedef struct {
  int ncid;
  int ignore;
  short isvar;
  short islon;
  int islat;
  int islev;
  int istime;
  int warn;
  int tsteptype;
  int param;
  int code;
  int tabnum;
  int climatology;
  int bounds;
  int lformula;
  int lformulaterms;
  int gridID;
  int zaxisID;
  int gridtype;
  int zaxistype;
  int xdim;
  int ydim;
  int zdim;
  int xvarid;
  int yvarid;
  int zvarid;
  int tvarid;
  int psvarid;
  int ncoordvars;
  int coordvarids[MAX_COORDVARS];
  int nauxvars;
  int auxvarids[MAX_AUXVARS];
  int cellarea;
  int calendar;
  int tableID;
  int truncation;
  int position;
  int defmissval;
  int deffillval;
  int xtype;
  int ndims;
  int gmapid;
  int positive;
  int dimids[8];
  int dimtype[8];
  int chunks[8];
  int chunked;
  int chunktype;
  int natts;
  int deflate;
  int lunsigned;
  int lvalidrange;
  int *atts;
  size_t vctsize;
  double *vct;
  double missval;
  double fillval;
  double addoffset;
  double scalefactor;
  double validrange[2];
  char name[CDI_MAX_NAME];
  char longname[CDI_MAX_NAME];
  char stdname[CDI_MAX_NAME];
  char units[CDI_MAX_NAME];
  char extra[CDI_MAX_NAME];
  ensinfo_t *ensdata;
}
ncvar_t;

static
void strtolower(char *str)
{
  if ( str )
    for (size_t i = 0; str[i]; ++i)
      str[i] = (char)tolower((int)str[i]);
}

static
int get_timeunit(size_t len, const char *ptu)
{
  int timeunit = -1;

  if ( len > 2 )
    {
      if ( memcmp(ptu, "sec", 3) == 0 ) timeunit = TUNIT_SECOND;
      else if ( memcmp(ptu, "minute", 6) == 0 ) timeunit = TUNIT_MINUTE;
      else if ( memcmp(ptu, "hour", 4) == 0 ) timeunit = TUNIT_HOUR;
      else if ( memcmp(ptu, "day", 3) == 0 ) timeunit = TUNIT_DAY;
      else if ( memcmp(ptu, "month", 5) == 0 ) timeunit = TUNIT_MONTH;
      else if ( memcmp(ptu, "calendar_month", 14) == 0 ) timeunit = TUNIT_MONTH;
      else if ( memcmp(ptu, "year", 4) == 0 ) timeunit = TUNIT_YEAR;
    }
  else if ( len == 1 )
    {
      if ( ptu[0] == 's' ) timeunit = TUNIT_SECOND;
    }

  return (timeunit);
}

static
int isTimeUnits(const char *timeunits)
{
  int status = 0;

  if ( strncmp(timeunits, "sec", 3) == 0 ||
       strncmp(timeunits, "minute", 6) == 0 ||
       strncmp(timeunits, "hour", 4) == 0 ||
       strncmp(timeunits, "day", 3) == 0 ||
       strncmp(timeunits, "month", 5) == 0 ) status = 1;

  return (status);
}

static
int isTimeAxisUnits(const char *timeunits)
{
  char *ptu, *tu;
  int timetype = -1;
  int timeunit;
  int status = FALSE;

  size_t len = strlen(timeunits);
  tu = (char *) Malloc((len+1)*sizeof(char));
  memcpy(tu, timeunits, (len+1) * sizeof(char));
  ptu = tu;

  for (size_t i = 0; i < len; i++ ) ptu[i] = (char)tolower((int)ptu[i]);

  timeunit = get_timeunit(len, ptu);
  if ( timeunit != -1 )
    {

      while ( ! isspace(*ptu) && *ptu != 0 ) ptu++;
      if ( *ptu )
        {
          while ( isspace(*ptu) ) ptu++;

          if ( memcmp(ptu, "as", 2) == 0 )
            timetype = TAXIS_ABSOLUTE;
          else if ( memcmp(ptu, "since", 5) == 0 )
            timetype = TAXIS_RELATIVE;

          if ( timetype != -1 ) status = TRUE;
        }
    }

  Free(tu);

  return (status);
}

static
void scanTimeString(const char *ptu, int *rdate, int *rtime)
{
  int year = 1, month = 1, day = 1;
  int hour = 0, minute = 0, second = 0;
  int v1 = 1, v2 = 1, v3 = 1;

  *rdate = 0;
  *rtime = 0;

  if ( *ptu )
    {
      v1 = atoi(ptu);
      if ( v1 < 0 ) ptu++;
      while ( isdigit((int) *ptu) ) ptu++;
      if ( *ptu )
        {
          v2 = atoi(++ptu);
          while ( isdigit((int) *ptu) ) ptu++;
          if ( *ptu )
            {
              v3 = atoi(++ptu);
              while ( isdigit((int) *ptu) ) ptu++;
            }
        }
    }

  if ( v3 > 999 && v1 < 32 )
    { year = v3; month = v2; day = v1; }
  else
    { year = v1; month = v2; day = v3; }

  while ( isspace((int) *ptu) ) ptu++;

  if ( *ptu )
    {
      while ( ! isdigit((int) *ptu) ) ptu++;

      hour = atoi(ptu);
      while ( isdigit((int) *ptu) ) ptu++;
      if ( *ptu == ':' )
        {
          ptu++;
          minute = atoi(ptu);
          while ( isdigit((int) *ptu) ) ptu++;
          if ( *ptu == ':' )
            {
              ptu++;
              second = atoi(ptu);
            }
        }
    }

  *rdate = cdiEncodeDate(year, month, day);
  *rtime = cdiEncodeTime(hour, minute, second);
}

static
int scanTimeUnit(const char *unitstr)
{
  int timeunit = -1;
  size_t len = strlen(unitstr);
  timeunit = get_timeunit(len, unitstr);
  if ( timeunit == -1 )
    Message("Unsupported TIMEUNIT: %s!", unitstr);

  return (timeunit);
}

static
void setForecastTime(const char *timestr, taxis_t *taxis)
{
  int len;

  (*taxis).fdate = 0;
  (*taxis).ftime = 0;

  len = (int) strlen(timestr);
  if ( len == 0 ) return;

  int fdate = 0, ftime = 0;
  scanTimeString(timestr, &fdate, &ftime);

  (*taxis).fdate = fdate;
  (*taxis).ftime = ftime;
}

static
int setBaseTime(const char *timeunits, taxis_t *taxis)
{
  char *ptu, *tu;
  int timetype = TAXIS_ABSOLUTE;
  int rdate = -1, rtime = -1;
  int timeunit;

  size_t len = strlen(timeunits);
  tu = (char *) Malloc((len+1) * sizeof (char));
  memcpy(tu, timeunits, (len+1) * sizeof (char));
  ptu = tu;

  for ( size_t i = 0; i < len; i++ ) ptu[i] = (char)tolower((int) ptu[i]);

  timeunit = get_timeunit(len, ptu);
  if ( timeunit == -1 )
    {
      Message("Unsupported TIMEUNIT: %s!", timeunits);
      return (1);
    }

  while ( ! isspace(*ptu) && *ptu != 0 ) ptu++;
  if ( *ptu )
    {
      while ( isspace(*ptu) ) ptu++;

      if ( memcmp(ptu, "as", 2) == 0 )
        timetype = TAXIS_ABSOLUTE;
      else if ( memcmp(ptu, "since", 5) == 0 )
        timetype = TAXIS_RELATIVE;

      while ( ! isspace(*ptu) && *ptu != 0 ) ptu++;
      if ( *ptu )
        {
          while ( isspace(*ptu) ) ptu++;

          if ( timetype == TAXIS_ABSOLUTE )
            {
              if ( memcmp(ptu, "%y%m%d.%f", 9) != 0 && timeunit == TUNIT_DAY )
                {
                  Message("Unsupported format %s for TIMEUNIT day!", ptu);
                  timeunit = -1;
                }
              else if ( memcmp(ptu, "%y%m.%f", 7) != 0 && timeunit == TUNIT_MONTH )
                {
                  Message("Unsupported format %s for TIMEUNIT month!", ptu);
                  timeunit = -1;
                }
            }
          else if ( timetype == TAXIS_RELATIVE )
            {
              scanTimeString(ptu, &rdate, &rtime);

              (*taxis).rdate = rdate;
              (*taxis).rtime = rtime;

              if ( CDI_Debug )
                Message("rdate = %d  rtime = %d", rdate, rtime);
            }
        }
    }

  (*taxis).type = timetype;
  (*taxis).unit = timeunit;

  Free(tu);

  if ( CDI_Debug )
    Message("timetype = %d  unit = %d", timetype, timeunit);

  return (0);
}

static
void cdfGetAttInt(int fileID, int ncvarid, const char *attname, int attlen, int *attint)
{
  nc_type atttype;
  size_t nc_attlen;

  *attint = 0;

  cdf_inq_atttype(fileID, ncvarid, attname, &atttype);
  cdf_inq_attlen(fileID, ncvarid, attname, &nc_attlen);

  if ( atttype != NC_CHAR )
    {
      int *pintatt = NULL;

      if ( (int)nc_attlen > attlen )
        pintatt = (int *) Malloc(nc_attlen * sizeof (int));
      else
        pintatt = attint;

      cdf_get_att_int(fileID, ncvarid, attname, pintatt);

      if ( (int)nc_attlen > attlen )
        {
          memcpy(attint, pintatt, (size_t)attlen * sizeof (int));
          Free(pintatt);
        }
    }
}

static
void cdfGetAttDouble(int fileID, int ncvarid, char *attname, int attlen, double *attdouble)
{
  nc_type atttype;
  size_t nc_attlen;

  *attdouble = 0;

  cdf_inq_atttype(fileID, ncvarid, attname, &atttype);
  cdf_inq_attlen(fileID, ncvarid, attname, &nc_attlen);

  if ( atttype != NC_CHAR )
    {
      double *pdoubleatt = NULL;

      if ( (int)nc_attlen > attlen )
        pdoubleatt = (double *) Malloc(nc_attlen * sizeof (double));
      else
        pdoubleatt = attdouble;

      cdf_get_att_double(fileID, ncvarid, attname, pdoubleatt);

      if ( (int)nc_attlen > attlen )
        {
          memcpy(attdouble, pdoubleatt, (size_t)attlen * sizeof (double));
          Free(pdoubleatt);
        }
    }
}

static
void cdfGetAttText(int fileID, int ncvarid,const char *attname, int attlen, char *atttext)
{
  nc_type atttype;
  size_t nc_attlen;

  cdf_inq_atttype(fileID, ncvarid, attname, &atttype);
  cdf_inq_attlen(fileID, ncvarid, attname, &nc_attlen);

  if ( atttype == NC_CHAR )
    {
      char attbuf[65636];
      if ( nc_attlen < sizeof(attbuf) )
        {
          cdf_get_att_text(fileID, ncvarid, attname, attbuf);

          if ( (int) nc_attlen > (attlen-1) ) nc_attlen = (size_t)(attlen-1);

          attbuf[nc_attlen++] = 0;
          memcpy(atttext, attbuf, nc_attlen);
        }
      else
        {
          atttext[0] = 0;
        }
    }
#if defined (HAVE_NETCDF4)
  else if ( atttype == NC_STRING )
    {
      if ( nc_attlen == 1 )
        {
          char *attbuf = NULL;
          cdf_get_att_string(fileID, ncvarid, attname, &attbuf);

          size_t ssize = strlen(attbuf) + 1;

          if ( ssize > (size_t)attlen ) ssize = (size_t)attlen;
          memcpy(atttext, attbuf, ssize);
          atttext[ssize - 1] = 0;
          Free(attbuf);
        }
      else
        {
          atttext[0] = 0;
        }
    }
#endif
}

static
int xtypeIsText(int xtype)
{
  int isText = FALSE;

  if ( xtype == NC_CHAR )
    isText = TRUE;
#if defined (HAVE_NETCDF4)
  else if ( xtype == NC_STRING )
    isText = TRUE;
#endif

  return isText;
}

static
int xtypeIsFloat(int xtype)
{
  int isFloat = FALSE;

  if ( xtype == NC_FLOAT || xtype == NC_DOUBLE ) isFloat = TRUE;

  return isFloat;
}

static
int cdfInqDatatype(int xtype, int lunsigned)
{
  int datatype = -1;

#if defined (HAVE_NETCDF4)
  if ( xtype == NC_BYTE && lunsigned ) xtype = NC_UBYTE;
#endif

  if ( xtype == NC_BYTE ) datatype = DATATYPE_INT8;

  else if ( xtype == NC_SHORT ) datatype = DATATYPE_INT16;
  else if ( xtype == NC_INT ) datatype = DATATYPE_INT32;
  else if ( xtype == NC_FLOAT ) datatype = DATATYPE_FLT32;
  else if ( xtype == NC_DOUBLE ) datatype = DATATYPE_FLT64;
#if defined (HAVE_NETCDF4)
  else if ( xtype == NC_UBYTE ) datatype = DATATYPE_UINT8;
  else if ( xtype == NC_LONG ) datatype = DATATYPE_INT32;
  else if ( xtype == NC_USHORT ) datatype = DATATYPE_UINT16;
  else if ( xtype == NC_UINT ) datatype = DATATYPE_UINT32;
  else if ( xtype == NC_INT64 ) datatype = DATATYPE_FLT64;
  else if ( xtype == NC_UINT64 ) datatype = DATATYPE_FLT64;
#endif

  return (datatype);
}


void cdfCopyRecord(stream_t *streamptr2, stream_t *streamptr1)
{
  int memtype = MEMTYPE_DOUBLE;
  int vlistID1 = streamptr1->vlistID;
  int tsID = streamptr1->curTsID;
  int vrecID = streamptr1->tsteps[tsID].curRecID;
  int recID = streamptr1->tsteps[tsID].recIDs[vrecID];
  int ivarID = streamptr1->tsteps[tsID].records[recID].varID;
  int gridID = vlistInqVarGrid(vlistID1, ivarID);
  int datasize = gridInqSize(gridID);

  double *data = (double *) Malloc((size_t)datasize * sizeof (double));

  int nmiss;
  cdf_read_record(streamptr1, memtype, data, &nmiss);


  Free(data);
}
void cdfDefRecord(stream_t *streamptr)
{
  (void)streamptr;
}

#if defined(NC_SZIP_NN_OPTION_MASK)
static
void cdfDefVarSzip(int ncid, int ncvarid)
{
  int retval;

  int options_mask = NC_SZIP_NN_OPTION_MASK;
  int bits_per_pixel = 16;

  if ((retval = nc_def_var_szip(ncid, ncvarid, options_mask, bits_per_pixel)))
    {
      if ( retval == NC_EINVAL )
        {
          static int lwarn = TRUE;

          if ( lwarn )
            {
              lwarn = FALSE;
              Warning("netCDF4/Szip compression not compiled in!");
            }
        }
      else
        Error("nc_def_var_szip failed, status = %d", retval);
    }
}
#endif

static
void cdfDefTimeValue(stream_t *streamptr, int tsID)
{
  int fileID = streamptr->fileID;

  if ( CDI_Debug )
    Message("streamID = %d, fileID = %d", streamptr->self, fileID);

  taxis_t *taxis = &streamptr->tsteps[tsID].taxis;

  if ( streamptr->ncmode == 1 )
    {
      cdf_enddef(fileID);
      streamptr->ncmode = 2;
    }

  size_t index = (size_t)tsID;

  double timevalue = cdiEncodeTimeval(taxis->vdate, taxis->vtime, &streamptr->tsteps[0].taxis);
  if ( CDI_Debug ) Message("tsID = %d  timevalue = %f", tsID, timevalue);

  int ncvarid = streamptr->basetime.ncvarid;
  cdf_put_var1_double(fileID, ncvarid, &index, &timevalue);

  if ( taxis->has_bounds )
    {
      size_t start[2], count[2];

      ncvarid = streamptr->basetime.ncvarboundsid;

      timevalue = cdiEncodeTimeval(taxis->vdate_lb, taxis->vtime_lb, &streamptr->tsteps[0].taxis);
      start[0] = (size_t)tsID; count[0] = 1; start[1] = 0; count[1] = 1;
      cdf_put_vara_double(fileID, ncvarid, start, count, &timevalue);

      timevalue = cdiEncodeTimeval(taxis->vdate_ub, taxis->vtime_ub, &streamptr->tsteps[0].taxis);
      start[0] = (size_t)tsID; count[0] = 1; start[1] = 1; count[1] = 1;
      cdf_put_vara_double(fileID, ncvarid, start, count, &timevalue);
    }

  ncvarid = streamptr->basetime.leadtimeid;
  if ( taxis->type == TAXIS_FORECAST && ncvarid != UNDEFID )
    {
      timevalue = taxis->fc_period;
      cdf_put_var1_double(fileID, ncvarid, &index, &timevalue);
    }




}

static
int cdfDefTimeBounds(int fileID, int nctimevarid, int nctimedimid, char* taxis_name, taxis_t* taxis)
{
  int time_bndsid = -1;
  int dims[2];
  char tmpstr[CDI_MAX_NAME];

  dims[0] = nctimedimid;



  if ( nc_inq_dimid(fileID, BNDS_NAME, &dims[1]) != NC_NOERR )
    cdf_def_dim(fileID, BNDS_NAME, 2, &dims[1]);

  if ( taxis->climatology )
    {
      strcpy(tmpstr, "climatology_");
      strcat(tmpstr, BNDS_NAME);
      cdf_def_var(fileID, tmpstr, NC_DOUBLE, 2, dims, &time_bndsid);

      cdf_put_att_text(fileID, nctimevarid, "climatology", strlen(tmpstr), tmpstr);
    }
  else
    {
      strcpy(tmpstr, taxis_name);
      strcat(tmpstr, "_");
      strcat(tmpstr, BNDS_NAME);
      cdf_def_var(fileID, tmpstr, NC_DOUBLE, 2, dims, &time_bndsid);

      cdf_put_att_text(fileID, nctimevarid, "bounds", strlen(tmpstr), tmpstr);
    }

  return (time_bndsid);
}

static
void cdfDefTimeUnits(char *unitstr, taxis_t* taxis0, taxis_t* taxis)
{
  unitstr[0] = 0;

  if ( taxis0->type == TAXIS_ABSOLUTE )
    {
      if ( taxis0->unit == TUNIT_YEAR )
        sprintf(unitstr, "year as %s", "%Y.%f");
      else if ( taxis0->unit == TUNIT_MONTH )
        sprintf(unitstr, "month as %s", "%Y%m.%f");
      else
        sprintf(unitstr, "day as %s", "%Y%m%d.%f");
    }
  else
    {
      int timeunit = taxis->unit;
      if ( timeunit == -1 ) timeunit = TUNIT_HOUR;
      int rdate = taxis->rdate;
      int rtime = taxis->rtime;
      if ( rdate == -1 )
        {
          rdate = taxis->vdate;
          rtime = taxis->vtime;
        }

      int year, month, day, hour, minute, second;
      cdiDecodeDate(rdate, &year, &month, &day);
      cdiDecodeTime(rtime, &hour, &minute, &second);

      if ( timeunit == TUNIT_QUARTER ) timeunit = TUNIT_MINUTE;
      if ( timeunit == TUNIT_30MINUTES ) timeunit = TUNIT_MINUTE;
      if ( timeunit == TUNIT_3HOURS ||
           timeunit == TUNIT_6HOURS ||
           timeunit == TUNIT_12HOURS ) timeunit = TUNIT_HOUR;

      sprintf(unitstr, "%s since %d-%d-%d %02d:%02d:%02d",
              tunitNamePtr(timeunit), year, month, day, hour, minute, second);
    }
}

static
void cdfDefForecastTimeUnits(char *unitstr, int timeunit)
{
  unitstr[0] = 0;

  if ( timeunit == -1 ) timeunit = TUNIT_HOUR;

  if ( timeunit == TUNIT_QUARTER ) timeunit = TUNIT_MINUTE;
  if ( timeunit == TUNIT_30MINUTES ) timeunit = TUNIT_MINUTE;
  if ( timeunit == TUNIT_3HOURS ||
       timeunit == TUNIT_6HOURS ||
       timeunit == TUNIT_12HOURS ) timeunit = TUNIT_HOUR;

  sprintf(unitstr, "%s", tunitNamePtr(timeunit));
}

static
void cdfDefCalendar(int fileID, int ncvarid, int calendar)
{
  size_t len;
  char calstr[80];

  calstr[0] = 0;

  if ( calendar == CALENDAR_STANDARD ) strcpy(calstr, "standard");
  else if ( calendar == CALENDAR_PROLEPTIC ) strcpy(calstr, "proleptic_gregorian");
  else if ( calendar == CALENDAR_NONE ) strcpy(calstr, "none");
  else if ( calendar == CALENDAR_360DAYS ) strcpy(calstr, "360_day");
  else if ( calendar == CALENDAR_365DAYS ) strcpy(calstr, "365_day");
  else if ( calendar == CALENDAR_366DAYS ) strcpy(calstr, "366_day");

  len = strlen(calstr);

  if ( len ) cdf_put_att_text(fileID, ncvarid, "calendar", len, calstr);
}


void cdfDefTime(stream_t* streamptr)
{
  int time_varid;
  int time_dimid;
  int time_bndsid = -1;
  char unitstr[CDI_MAX_NAME];
  char tmpstr[CDI_MAX_NAME];
  char default_name[] = "time";
  char* taxis_name = default_name;

  if ( streamptr->basetime.ncvarid != UNDEFID ) return;

  int fileID = streamptr->fileID;

  if ( streamptr->ncmode == 0 ) streamptr->ncmode = 1;
  if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

  taxis_t *taxis = &streamptr->tsteps[0].taxis;

  if ( taxis->name && taxis->name[0] ) taxis_name = taxis->name;

  cdf_def_dim(fileID, taxis_name, NC_UNLIMITED, &time_dimid);
  streamptr->basetime.ncdimid = time_dimid;

  cdf_def_var(fileID, taxis_name, NC_DOUBLE, 1, &time_dimid, &time_varid);

  streamptr->basetime.ncvarid = time_varid;

  strcpy(tmpstr, "time");
  cdf_put_att_text(fileID, time_varid, "standard_name", strlen(tmpstr), tmpstr);

  if ( taxis->longname && taxis->longname[0] )
    cdf_put_att_text(fileID, time_varid, "long_name", strlen(taxis->longname), taxis->longname);

  if ( taxis->has_bounds )
    {
      time_bndsid = cdfDefTimeBounds(fileID, time_varid, time_dimid, taxis_name, taxis);
      streamptr->basetime.ncvarboundsid = time_bndsid;
    }

  cdfDefTimeUnits(unitstr, &streamptr->tsteps[0].taxis, taxis);

  size_t len = strlen(unitstr);
  if ( len )
    {
      cdf_put_att_text(fileID, time_varid, "units", len, unitstr);




    }

  if ( taxis->calendar != -1 )
    {
      cdfDefCalendar(fileID, time_varid, taxis->calendar);




    }

  if ( taxis->type == TAXIS_FORECAST )
    {
      int leadtimeid;

      cdf_def_var(fileID, "leadtime", NC_DOUBLE, 1, &time_dimid, &leadtimeid);

      streamptr->basetime.leadtimeid = leadtimeid;

      strcpy(tmpstr, "forecast_period");
      cdf_put_att_text(fileID, leadtimeid, "standard_name", strlen(tmpstr), tmpstr);

      strcpy(tmpstr, "Time elapsed since the start of the forecast");
      cdf_put_att_text(fileID, leadtimeid, "long_name", strlen(tmpstr), tmpstr);

      cdfDefForecastTimeUnits(unitstr, taxis->fc_unit);

      len = strlen(unitstr);
      if ( len ) cdf_put_att_text(fileID, leadtimeid, "units", len, unitstr);
    }

  cdf_put_att_text(fileID, time_varid, "axis", 1, "T");

  if ( streamptr->ncmode == 2 ) cdf_enddef(fileID);
}


void cdfDefTimestep(stream_t *streamptr, int tsID)
{
  int vlistID = streamptr->vlistID;

  if ( vlistHasTime(vlistID) ) cdfDefTime(streamptr);

  cdfDefTimeValue(streamptr, tsID);
}

static
void cdfDefComplex(stream_t *streamptr, int gridID)
{
  char axisname[] = "nc2";
  int dimID = UNDEFID;
  int gridID0, gridtype0;
  int vlistID = streamptr->vlistID;
  int fileID = streamptr->fileID;

  int ngrids = vlistNgrids(vlistID);

  for ( int index = 0; index < ngrids; index++ )
    {
      if ( streamptr->xdimID[index] != UNDEFID )
        {
          gridID0 = vlistGrid(vlistID, index);
          gridtype0 = gridInqType(gridID0);
          if ( gridtype0 == GRID_SPECTRAL || gridtype0 == GRID_FOURIER )
            {
              dimID = streamptr->xdimID[index];
              break;
            }
        }
    }

  if ( dimID == UNDEFID )
    {
      size_t dimlen = 2;

      if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

      cdf_def_dim(fileID, axisname, dimlen, &dimID);

      cdf_enddef(fileID);
      streamptr->ncmode = 2;
    }

  int gridindex = vlistGridIndex(vlistID, gridID);
  streamptr->xdimID[gridindex] = dimID;
}


static
void cdfDefSP(stream_t *streamptr, int gridID)
{



  char axisname[5] = "nspX";
  int index, iz = 0;
  int gridID0, gridtype0, gridindex;
  int dimID = UNDEFID;
  int ngrids;
  int fileID;
  int vlistID;

  vlistID = streamptr->vlistID;
  fileID = streamptr->fileID;

  ngrids = vlistNgrids(vlistID);

  size_t dimlen = (size_t)gridInqSize(gridID)/2;

  for ( index = 0; index < ngrids; index++ )
    {
      if ( streamptr->ydimID[index] != UNDEFID )
        {
          gridID0 = vlistGrid(vlistID, index);
          gridtype0 = gridInqType(gridID0);
          if ( gridtype0 == GRID_SPECTRAL )
            {
              size_t dimlen0 = (size_t)gridInqSize(gridID0)/2;
              if ( dimlen == dimlen0 )
                {
                  dimID = streamptr->ydimID[index];
                  break;
                }
              else
                iz++;
            }
        }
    }

  if ( dimID == UNDEFID )
    {
      if ( iz == 0 ) axisname[3] = '\0';
      else sprintf(&axisname[3], "%1d", iz+1);

      if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

      cdf_def_dim(fileID, axisname, dimlen, &dimID);

      cdf_enddef(fileID);
      streamptr->ncmode = 2;
    }

  gridindex = vlistGridIndex(vlistID, gridID);
  streamptr->ydimID[gridindex] = dimID;
}


static
void cdfDefFC(stream_t *streamptr, int gridID)
{
  char axisname[5] = "nfcX";
  int index, iz = 0;
  int gridID0, gridtype0, gridindex;
  int dimID = UNDEFID;
  int ngrids;
  int fileID;
  int vlistID;

  vlistID = streamptr->vlistID;
  fileID = streamptr->fileID;

  ngrids = vlistNgrids(vlistID);

  size_t dimlen = (size_t)gridInqSize(gridID)/2;

  for ( index = 0; index < ngrids; index++ )
    {
      if ( streamptr->ydimID[index] != UNDEFID )
        {
          gridID0 = vlistGrid(vlistID, index);
          gridtype0 = gridInqType(gridID0);
          if ( gridtype0 == GRID_FOURIER )
            {
              size_t dimlen0 = (size_t)gridInqSize(gridID0)/2;
              if ( dimlen == dimlen0 )
                {
                  dimID = streamptr->ydimID[index];
                  break;
                }
              else
                iz++;
            }
        }
    }

  if ( dimID == UNDEFID )
    {
      if ( iz == 0 ) axisname[3] = '\0';
      else sprintf(&axisname[3], "%1d", iz+1);

      if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

      cdf_def_dim(fileID, axisname, dimlen, &dimID);

      cdf_enddef(fileID);
      streamptr->ncmode = 2;
    }

  gridindex = vlistGridIndex(vlistID, gridID);
  streamptr->ydimID[gridindex] = dimID;
}


static
void cdfDefTrajLon(stream_t *streamptr, int gridID)
{
  char units[CDI_MAX_NAME];
  char longname[CDI_MAX_NAME];
  char stdname[CDI_MAX_NAME];
  char axisname[CDI_MAX_NAME];
  int gridtype, gridindex;
  int dimID = UNDEFID;
  int fileID;
  int dimlen;
  size_t len;
  int ncvarid;
  int vlistID;
  int xtype = NC_DOUBLE;

  if ( gridInqPrec(gridID) == DATATYPE_FLT32 ) xtype = NC_FLOAT;

  vlistID = streamptr->vlistID;
  fileID = streamptr->fileID;

  gridtype = gridInqType(gridID);
  dimlen = gridInqXsize(gridID);
  if ( dimlen != 1 ) Error("Xsize isn't 1 for %s grid!", gridNamePtr(gridtype));

  gridindex = vlistGridIndex(vlistID, gridID);
  ncvarid = streamptr->xdimID[gridindex];

  gridInqXname(gridID, axisname);
  gridInqXlongname(gridID, longname);
  gridInqXstdname(gridID, stdname);
  gridInqXunits(gridID, units);

  if ( ncvarid == UNDEFID )
    {
      dimID = streamptr->basetime.ncvarid;

      if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

      cdf_def_var(fileID, axisname, (nc_type) xtype, 1, &dimID, &ncvarid);

      if ( (len = strlen(stdname)) )
        cdf_put_att_text(fileID, ncvarid, "standard_name", len, stdname);
      if ( (len = strlen(longname)) )
        cdf_put_att_text(fileID, ncvarid, "long_name", len, longname);
      if ( (len = strlen(units)) )
        cdf_put_att_text(fileID, ncvarid, "units", len, units);

      cdf_enddef(fileID);
      streamptr->ncmode = 2;
    }

  streamptr->xdimID[gridindex] = ncvarid;
}


static
void cdfDefTrajLat(stream_t *streamptr, int gridID)
{
  char units[] = "degrees_north";
  char longname[] = "latitude";
  char stdname[] = "latitude";
  char axisname[] = "tlat";
  int gridtype, gridindex;
  int dimID = UNDEFID;
  int fileID;
  int dimlen;
  size_t len;
  int ncvarid;
  int vlistID;
  int xtype = NC_DOUBLE;

  if ( gridInqPrec(gridID) == DATATYPE_FLT32 ) xtype = NC_FLOAT;

  vlistID = streamptr->vlistID;
  fileID = streamptr->fileID;

  gridtype = gridInqType(gridID);
  dimlen = gridInqYsize(gridID);
  if ( dimlen != 1 ) Error("Ysize isn't 1 for %s grid!", gridNamePtr(gridtype));

  gridindex = vlistGridIndex(vlistID, gridID);
  ncvarid = streamptr->ydimID[gridindex];

  gridInqYname(gridID, axisname);
  gridInqYlongname(gridID, longname);
  gridInqYstdname(gridID, stdname);
  gridInqYunits(gridID, units);

  if ( ncvarid == UNDEFID )
    {
      dimID = streamptr->basetime.ncvarid;

      if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

      cdf_def_var(fileID, axisname, (nc_type) xtype, 1, &dimID, &ncvarid);

      if ( (len = strlen(stdname)) )
        cdf_put_att_text(fileID, ncvarid, "standard_name", len, stdname);
      if ( (len = strlen(longname)) )
        cdf_put_att_text(fileID, ncvarid, "long_name", len, longname);
      if ( (len = strlen(units)) )
        cdf_put_att_text(fileID, ncvarid, "units", len, units);

      cdf_enddef(fileID);
      streamptr->ncmode = 2;
    }

  streamptr->ydimID[gridindex] = ncvarid;
}


static
int checkGridName(int type, char *axisname, int fileID, int vlistID, int gridID, int ngrids, int mode)
{
  int iz, index;
  int gridID0;
  int ncdimid;
  char axisname0[CDI_MAX_NAME];
  char axisname2[CDI_MAX_NAME];
  int checkname;
  int status;


  checkname = TRUE;
  iz = 0;

  do
    {
      strcpy(axisname2, axisname);
      if ( iz ) sprintf(&axisname2[strlen(axisname2)], "_%d", iz+1);


      if ( type == 'V' )
        status = nc_inq_varid(fileID, axisname2, &ncdimid);
      else
        status = nc_inq_dimid(fileID, axisname2, &ncdimid);

      if ( status != NC_NOERR )
        {
          if ( iz )
            {

              for ( index = 0; index < ngrids; index++ )
                {
                  gridID0 = vlistGrid(vlistID, index);
                  if ( gridID != gridID0 )
                    {
                      if ( mode == 'X' )
                        gridInqXname(gridID0, axisname0);
                      else
                        gridInqYname(gridID0, axisname0);

                      if ( strcmp(axisname0, axisname2) == 0 ) break;
                    }
                }
              if ( index == ngrids ) checkname = FALSE;
            }
          else
            {
              checkname = FALSE;
            }
        }

      if ( checkname ) iz++;
    }
  while (checkname && iz <= 99);


  if ( iz ) sprintf(&axisname[strlen(axisname)], "_%d", iz+1);

  return (iz);
}


static
void cdfDefXaxis(stream_t *streamptr, int gridID, int ndims)
{
  char units[CDI_MAX_NAME];
  char longname[CDI_MAX_NAME];
  char stdname[CDI_MAX_NAME];
  char axisname[CDI_MAX_NAME];
  int index;

  int gridID0, gridtype0, gridindex;
  int dimID = UNDEFID;
  int dimIDs[2];
  int ngrids = 0;
  int fileID;
  size_t len;
  int ncvarid = UNDEFID, ncbvarid = UNDEFID;
  int nvdimID = UNDEFID;
  int vlistID;
  int xtype = NC_DOUBLE;

  if ( gridInqPrec(gridID) == DATATYPE_FLT32 ) xtype = NC_FLOAT;

  vlistID = streamptr->vlistID;
  fileID = streamptr->fileID;

  if ( ndims ) ngrids = vlistNgrids(vlistID);

  size_t dimlen = (size_t)gridInqXsize(gridID);
  gridindex = vlistGridIndex(vlistID, gridID);

  gridInqXname(gridID, axisname);
  gridInqXlongname(gridID, longname);
  gridInqXstdname(gridID, stdname);
  gridInqXunits(gridID, units);

  if ( axisname[0] == 0 ) Error("axis name undefined!");

  for ( index = 0; index < ngrids; index++ )
    {
      if ( streamptr->xdimID[index] != UNDEFID )
        {
          gridID0 = vlistGrid(vlistID, index);
          gridtype0 = gridInqType(gridID0);
          if ( gridtype0 == GRID_GAUSSIAN ||
               gridtype0 == GRID_LONLAT ||
               gridtype0 == GRID_CURVILINEAR ||
               gridtype0 == GRID_GENERIC )
            {
              size_t dimlen0 = (size_t)gridInqXsize(gridID0);
              if ( dimlen == dimlen0 )
                if ( IS_EQUAL(gridInqXval(gridID0, 0), gridInqXval(gridID, 0)) &&
                     IS_EQUAL(gridInqXval(gridID0, (int)dimlen-1), gridInqXval(gridID, (int)dimlen-1)) )
                  {
                    dimID = streamptr->xdimID[index];
                    break;
                  }






            }
        }
    }

  if ( dimID == UNDEFID )
    {
      int status;
      status = checkGridName('V', axisname, fileID, vlistID, gridID, ngrids, 'X');
      if ( status == 0 && ndims )
        status = checkGridName('D', axisname, fileID, vlistID, gridID, ngrids, 'X');

      if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

      if ( ndims )
        {
          cdf_def_dim(fileID, axisname, dimlen, &dimID);

          if ( gridInqXboundsPtr(gridID) || gridInqYboundsPtr(gridID) )
            {
              size_t nvertex = 2;
              if ( nc_inq_dimid(fileID, BNDS_NAME, &nvdimID) != NC_NOERR )
                cdf_def_dim(fileID, BNDS_NAME, nvertex, &nvdimID);
            }
        }

      if ( gridInqXvalsPtr(gridID) )
        {
          cdf_def_var(fileID, axisname, (nc_type) xtype, ndims, &dimID, &ncvarid);

          if ( (len = strlen(stdname)) )
            cdf_put_att_text(fileID, ncvarid, "standard_name", len, stdname);
          if ( (len = strlen(longname)) )
            cdf_put_att_text(fileID, ncvarid, "long_name", len, longname);
          if ( (len = strlen(units)) )
            cdf_put_att_text(fileID, ncvarid, "units", len, units);

          cdf_put_att_text(fileID, ncvarid, "axis", 1, "X");

          if ( gridInqXboundsPtr(gridID) && nvdimID != UNDEFID )
            {
              strcat(axisname, "_");
              strcat(axisname, BNDS_NAME);
              dimIDs[0] = dimID;
              dimIDs[1] = nvdimID;
              cdf_def_var(fileID, axisname, (nc_type) xtype, 2, dimIDs, &ncbvarid);
              cdf_put_att_text(fileID, ncvarid, "bounds", strlen(axisname), axisname);
            }







        }

      cdf_enddef(fileID);
      streamptr->ncmode = 2;

      if ( ncvarid != UNDEFID ) cdf_put_var_double(fileID, ncvarid, gridInqXvalsPtr(gridID));
      if ( ncbvarid != UNDEFID ) cdf_put_var_double(fileID, ncbvarid, gridInqXboundsPtr(gridID));

      if ( ndims == 0 ) streamptr->ncxvarID[gridindex] = ncvarid;
    }

  streamptr->xdimID[gridindex] = dimID;
}


static
void cdfDefYaxis(stream_t *streamptr, int gridID, int ndims)
{
  char units[CDI_MAX_NAME];
  char longname[CDI_MAX_NAME];
  char stdname[CDI_MAX_NAME];
  char axisname[CDI_MAX_NAME];
  int index;

  int gridID0, gridtype0, gridindex;
  int dimID = UNDEFID;
  int dimIDs[2];
  int ngrids = 0;
  int fileID;
  size_t len;
  int ncvarid = UNDEFID, ncbvarid = UNDEFID;
  int nvdimID = UNDEFID;
  int vlistID;
  int xtype = NC_DOUBLE;

  if ( gridInqPrec(gridID) == DATATYPE_FLT32 ) xtype = NC_FLOAT;

  vlistID = streamptr->vlistID;
  fileID = streamptr->fileID;

  if ( ndims ) ngrids = vlistNgrids(vlistID);

  size_t dimlen = (size_t)gridInqYsize(gridID);
  gridindex = vlistGridIndex(vlistID, gridID);

  gridInqYname(gridID, axisname);
  gridInqYlongname(gridID, longname);
  gridInqYstdname(gridID, stdname);
  gridInqYunits(gridID, units);

  if ( axisname[0] == 0 ) Error("axis name undefined!");

  for ( index = 0; index < ngrids; index++ )
    {
      if ( streamptr->ydimID[index] != UNDEFID )
        {
          gridID0 = vlistGrid(vlistID, index);
          gridtype0 = gridInqType(gridID0);
          if ( gridtype0 == GRID_GAUSSIAN ||
               gridtype0 == GRID_LONLAT ||
               gridtype0 == GRID_CURVILINEAR ||
               gridtype0 == GRID_GENERIC )
            {
              size_t dimlen0 = (size_t)gridInqYsize(gridID0);
              if ( dimlen == dimlen0 )
                if ( IS_EQUAL(gridInqYval(gridID0, 0), gridInqYval(gridID, 0)) &&
                     IS_EQUAL(gridInqYval(gridID0, (int)dimlen-1), gridInqYval(gridID, (int)dimlen-1)) )
                  {
                    dimID = streamptr->ydimID[index];
                    break;
                  }






            }
        }
    }

  if ( dimID == UNDEFID )
    {
      int status;
      status = checkGridName('V', axisname, fileID, vlistID, gridID, ngrids, 'Y');
      if ( status == 0 && ndims )
        status = checkGridName('D', axisname, fileID, vlistID, gridID, ngrids, 'Y');

      if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

      if ( ndims )
        {
          cdf_def_dim(fileID, axisname, dimlen, &dimID);

          if ( gridInqXboundsPtr(gridID) || gridInqYboundsPtr(gridID) )
            {
              size_t nvertex = 2;
              if ( nc_inq_dimid(fileID, BNDS_NAME, &nvdimID) != NC_NOERR )
                cdf_def_dim(fileID, BNDS_NAME, nvertex, &nvdimID);
            }
        }

      if ( gridInqYvalsPtr(gridID) )
        {
          cdf_def_var(fileID, axisname, (nc_type) xtype, ndims, &dimID, &ncvarid);

          if ( (len = strlen(stdname)) )
            cdf_put_att_text(fileID, ncvarid, "standard_name", len, stdname);
          if ( (len = strlen(longname)) )
            cdf_put_att_text(fileID, ncvarid, "long_name", len, longname);
          if ( (len = strlen(units)) )
            cdf_put_att_text(fileID, ncvarid, "units", len, units);

          cdf_put_att_text(fileID, ncvarid, "axis", 1, "Y");

          if ( gridInqYboundsPtr(gridID) && nvdimID != UNDEFID )
            {
              strcat(axisname, "_");
              strcat(axisname, BNDS_NAME);
              dimIDs[0] = dimID;
              dimIDs[1] = nvdimID;
              cdf_def_var(fileID, axisname, (nc_type) xtype, 2, dimIDs, &ncbvarid);
              cdf_put_att_text(fileID, ncvarid, "bounds", strlen(axisname), axisname);
            }







        }

      cdf_enddef(fileID);
      streamptr->ncmode = 2;

      if ( ncvarid != UNDEFID ) cdf_put_var_double(fileID, ncvarid, gridInqYvalsPtr(gridID));
      if ( ncbvarid != UNDEFID ) cdf_put_var_double(fileID, ncbvarid, gridInqYboundsPtr(gridID));

      if ( ndims == 0 ) streamptr->ncyvarID[gridindex] = ncvarid;
    }

  streamptr->ydimID[gridindex] = dimID;
}

static
void cdfGridCompress(int fileID, int ncvarid, int gridsize, int filetype, int comptype)
{
#if defined (HAVE_NETCDF4)
  if ( gridsize > 1 && comptype == COMPRESS_ZIP && (filetype == FILETYPE_NC4 || filetype == FILETYPE_NC4C) )
    {
      nc_def_var_chunking(fileID, ncvarid, NC_CHUNKED, NULL);
      cdfDefVarDeflate(fileID, ncvarid, 1);
    }
#endif
}


static
void cdfDefCurvilinear(stream_t *streamptr, int gridID)
{
  char xunits[CDI_MAX_NAME];
  char xlongname[CDI_MAX_NAME];
  char xstdname[CDI_MAX_NAME];
  char yunits[CDI_MAX_NAME];
  char ylongname[CDI_MAX_NAME];
  char ystdname[CDI_MAX_NAME];
  char xaxisname[CDI_MAX_NAME];
  char yaxisname[CDI_MAX_NAME];
  char xdimname[4] = "x";
  char ydimname[4] = "y";
  int index;
  int gridID0, gridtype0, gridindex;
  int xdimID = UNDEFID;
  int ydimID = UNDEFID;
  int dimIDs[3];
  int ngrids;
  int fileID;
  size_t len;
  int ncxvarid = UNDEFID, ncyvarid = UNDEFID;
  int ncbxvarid = UNDEFID, ncbyvarid = UNDEFID, ncavarid = UNDEFID;
  int nvdimID = UNDEFID;
  int vlistID;
  int xtype = NC_DOUBLE;

  if ( gridInqPrec(gridID) == DATATYPE_FLT32 ) xtype = NC_FLOAT;

  vlistID = streamptr->vlistID;
  fileID = streamptr->fileID;

  ngrids = vlistNgrids(vlistID);

  size_t xdimlen = (size_t)gridInqXsize(gridID);
  size_t ydimlen = (size_t)gridInqYsize(gridID);
  gridindex = vlistGridIndex(vlistID, gridID);

  gridInqXname(gridID, xaxisname);
  gridInqXlongname(gridID, xlongname);
  gridInqXstdname(gridID, xstdname);
  gridInqXunits(gridID, xunits);
  gridInqYname(gridID, yaxisname);
  gridInqYlongname(gridID, ylongname);
  gridInqYstdname(gridID, ystdname);
  gridInqYunits(gridID, yunits);

  for ( index = 0; index < ngrids; index++ )
    {
      if ( streamptr->xdimID[index] != UNDEFID )
        {
          gridID0 = vlistGrid(vlistID, index);
          gridtype0 = gridInqType(gridID0);
          if ( gridtype0 == GRID_GAUSSIAN ||
               gridtype0 == GRID_LONLAT ||
               gridtype0 == GRID_CURVILINEAR ||
               gridtype0 == GRID_GENERIC )
            {
              size_t dimlen0 = (size_t)gridInqXsize(gridID0);
              if ( xdimlen == dimlen0 )
                if ( IS_EQUAL(gridInqXval(gridID0, 0), gridInqXval(gridID, 0)) &&
                     IS_EQUAL(gridInqXval(gridID0, (int)xdimlen-1), gridInqXval(gridID, (int)xdimlen-1)) )
                  {
                    xdimID = streamptr->xdimID[index];
                    ncxvarid = streamptr->ncxvarID[index];
                    break;
                  }
              dimlen0 = (size_t)gridInqYsize(gridID0);
              if ( ydimlen == dimlen0 )
                if ( IS_EQUAL(gridInqYval(gridID0, 0), gridInqYval(gridID, 0)) &&
                     IS_EQUAL(gridInqYval(gridID0, (int)xdimlen-1), gridInqYval(gridID, (int)xdimlen-1)) )
                  {
                    ydimID = streamptr->ydimID[index];
                    ncyvarid = streamptr->ncyvarID[index];
                    break;
                  }
            }
        }
    }

  if ( xdimID == UNDEFID || ydimID == UNDEFID )
    {
      checkGridName('V', xaxisname, fileID, vlistID, gridID, ngrids, 'X');
      checkGridName('V', yaxisname, fileID, vlistID, gridID, ngrids, 'Y');
      checkGridName('D', xdimname, fileID, vlistID, gridID, ngrids, 'X');
      checkGridName('D', ydimname, fileID, vlistID, gridID, ngrids, 'Y');

      if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

      cdf_def_dim(fileID, xdimname, xdimlen, &xdimID);
      cdf_def_dim(fileID, ydimname, ydimlen, &ydimID);

      if ( gridInqXboundsPtr(gridID) || gridInqYboundsPtr(gridID) )
        {
          size_t nvertex = 4;
          if ( nc_inq_dimid(fileID, "nv4", &nvdimID) != NC_NOERR )
            cdf_def_dim(fileID, "nv4", nvertex, &nvdimID);
        }

      dimIDs[0] = ydimID;
      dimIDs[1] = xdimID;

      if ( gridInqXvalsPtr(gridID) )
        {
          cdf_def_var(fileID, xaxisname, (nc_type) xtype, 2, dimIDs, &ncxvarid);
          cdfGridCompress(fileID, ncxvarid, (int)(xdimlen*ydimlen), streamptr->filetype, streamptr->comptype);

          if ( (len = strlen(xstdname)) )
            cdf_put_att_text(fileID, ncxvarid, "standard_name", len, xstdname);
          if ( (len = strlen(xlongname)) )
            cdf_put_att_text(fileID, ncxvarid, "long_name", len, xlongname);
          if ( (len = strlen(xunits)) )
            cdf_put_att_text(fileID, ncxvarid, "units", len, xunits);


          cdf_put_att_text(fileID, ncxvarid, "_CoordinateAxisType", 3, "Lon");

          if ( gridInqXboundsPtr(gridID) && nvdimID != UNDEFID )
            {
              strcat(xaxisname, "_");
              strcat(xaxisname, BNDS_NAME);
              dimIDs[0] = ydimID;
              dimIDs[1] = xdimID;
              dimIDs[2] = nvdimID;
              cdf_def_var(fileID, xaxisname, (nc_type) xtype, 3, dimIDs, &ncbxvarid);
              cdfGridCompress(fileID, ncbxvarid, (int)(xdimlen*ydimlen), streamptr->filetype, streamptr->comptype);

              cdf_put_att_text(fileID, ncxvarid, "bounds", strlen(xaxisname), xaxisname);
            }
        }

      if ( gridInqYvalsPtr(gridID) )
        {
          cdf_def_var(fileID, yaxisname, (nc_type) xtype, 2, dimIDs, &ncyvarid);
          cdfGridCompress(fileID, ncyvarid, (int)(xdimlen*ydimlen), streamptr->filetype, streamptr->comptype);

          if ( (len = strlen(ystdname)) )
            cdf_put_att_text(fileID, ncyvarid, "standard_name", len, ystdname);
          if ( (len = strlen(ylongname)) )
            cdf_put_att_text(fileID, ncyvarid, "long_name", len, ylongname);
          if ( (len = strlen(yunits)) )
            cdf_put_att_text(fileID, ncyvarid, "units", len, yunits);


          cdf_put_att_text(fileID, ncyvarid, "_CoordinateAxisType", 3, "Lat");

          if ( gridInqYboundsPtr(gridID) && nvdimID != UNDEFID )
            {
              strcat(yaxisname, "_");
              strcat(yaxisname, BNDS_NAME);
              dimIDs[0] = ydimID;
              dimIDs[1] = xdimID;
              dimIDs[2] = nvdimID;
              cdf_def_var(fileID, yaxisname, (nc_type) xtype, 3, dimIDs, &ncbyvarid);
              cdfGridCompress(fileID, ncbyvarid, (int)(xdimlen*ydimlen), streamptr->filetype, streamptr->comptype);

              cdf_put_att_text(fileID, ncyvarid, "bounds", strlen(yaxisname), yaxisname);
            }
        }

      if ( gridInqAreaPtr(gridID) )
        {
          static const char yaxisname_[] = "cell_area";
          static const char units[] = "m2";
          static const char longname[] = "area of grid cell";
          static const char stdname[] = "cell_area";

          cdf_def_var(fileID, yaxisname_, (nc_type) xtype, 2, dimIDs, &ncavarid);

          cdf_put_att_text(fileID, ncavarid, "standard_name", sizeof (stdname) - 1, stdname);
          cdf_put_att_text(fileID, ncavarid, "long_name", sizeof (longname) - 1, longname);
          cdf_put_att_text(fileID, ncavarid, "units", sizeof (units) - 1, units);
        }

      cdf_enddef(fileID);
      streamptr->ncmode = 2;

      if ( ncxvarid != UNDEFID ) cdf_put_var_double(fileID, ncxvarid, gridInqXvalsPtr(gridID));
      if ( ncbxvarid != UNDEFID ) cdf_put_var_double(fileID, ncbxvarid, gridInqXboundsPtr(gridID));
      if ( ncyvarid != UNDEFID ) cdf_put_var_double(fileID, ncyvarid, gridInqYvalsPtr(gridID));
      if ( ncbyvarid != UNDEFID ) cdf_put_var_double(fileID, ncbyvarid, gridInqYboundsPtr(gridID));
      if ( ncavarid != UNDEFID ) cdf_put_var_double(fileID, ncavarid, gridInqAreaPtr(gridID));
    }

  streamptr->xdimID[gridindex] = xdimID;
  streamptr->ydimID[gridindex] = ydimID;
  streamptr->ncxvarID[gridindex] = ncxvarid;
  streamptr->ncyvarID[gridindex] = ncyvarid;
  streamptr->ncavarID[gridindex] = ncavarid;
}


static
void cdfDefRgrid(stream_t *streamptr, int gridID)
{
  char axisname[7] = "rgridX";
  int index, iz = 0;
  int gridID0, gridtype0, gridindex;
  int dimID = UNDEFID;
  int ngrids;
  int fileID;
  int vlistID;
  int lwarn = TRUE;

  vlistID = streamptr->vlistID;
  fileID = streamptr->fileID;

  ngrids = vlistNgrids(vlistID);

  size_t dimlen = (size_t)gridInqSize(gridID);

  for ( index = 0; index < ngrids; index++ )
    {
      if ( streamptr->xdimID[index] != UNDEFID )
        {
          gridID0 = vlistGrid(vlistID, index);
          gridtype0 = gridInqType(gridID0);
          if ( gridtype0 == GRID_GAUSSIAN_REDUCED )
            {
              size_t dimlen0 = (size_t)gridInqSize(gridID0);

              if ( dimlen == dimlen0 )
                {
                  dimID = streamptr->xdimID[index];
                  break;
                }
              else
                iz++;
            }
        }
    }

  if ( dimID == UNDEFID )
    {
      if ( lwarn )
        {
          Warning("Creating a netCDF file with data on a gaussian reduced grid.");
          Warning("The further processing of the resulting file is unsupported!");
          lwarn = FALSE;
        }

      if ( iz == 0 ) axisname[5] = '\0';
      else sprintf(&axisname[5], "%1d", iz+1);

      if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

      cdf_def_dim(fileID, axisname, dimlen, &dimID);

      cdf_enddef(fileID);
      streamptr->ncmode = 2;
    }

  gridindex = vlistGridIndex(vlistID, gridID);
  streamptr->xdimID[gridindex] = dimID;
}


static
void cdfDefGdim(stream_t *streamptr, int gridID)
{
  int index, iz = 0;
  int gridID0, gridtype0, gridindex;
  int dimID = UNDEFID;
  int ngrids;
  int fileID;
  int vlistID;

  vlistID = streamptr->vlistID;
  fileID = streamptr->fileID;

  ngrids = vlistNgrids(vlistID);

  size_t dimlen = (size_t)gridInqSize(gridID);

  if ( gridInqYsize(gridID) == 0 )
    for ( index = 0; index < ngrids; index++ )
      {
        if ( streamptr->xdimID[index] != UNDEFID )
          {
            gridID0 = vlistGrid(vlistID, index);
            gridtype0 = gridInqType(gridID0);
            if ( gridtype0 == GRID_GENERIC )
              {
                size_t dimlen0 = (size_t)gridInqSize(gridID0);
                if ( dimlen == dimlen0 )
                  {
                    dimID = streamptr->xdimID[index];
                    break;
                  }
                else
                  iz++;
              }
          }
      }

  if ( gridInqXsize(gridID) == 0 )
    for ( index = 0; index < ngrids; index++ )
      {
        if ( streamptr->ydimID[index] != UNDEFID )
          {
            gridID0 = vlistGrid(vlistID, index);
            gridtype0 = gridInqType(gridID0);
            if ( gridtype0 == GRID_GENERIC )
              {
                size_t dimlen0 = (size_t)gridInqSize(gridID0);
                if ( dimlen == dimlen0 )
                  {
                    dimID = streamptr->ydimID[index];
                    break;
                  }
                else
                  iz++;
              }
          }
      }

  if ( dimID == UNDEFID )
    {
      char axisname[CDI_MAX_NAME];
      strcpy(axisname, "gsize");

      checkGridName('D', axisname, fileID, vlistID, gridID, ngrids, 'X');




      if ( streamptr->ncmode == 2 ) cdf_redef(fileID);


      cdf_def_dim(fileID, axisname, dimlen, &dimID);

      cdf_enddef(fileID);
      streamptr->ncmode = 2;
    }

  gridindex = vlistGridIndex(vlistID, gridID);
  streamptr->xdimID[gridindex] = dimID;
}


static
void cdfDefGridReference(stream_t *streamptr, int gridID)
{
  int fileID = streamptr->fileID;
  int number = gridInqNumber(gridID);

  if ( number > 0 )
    {
      cdf_put_att_int(fileID, NC_GLOBAL, "number_of_grid_used", NC_INT, 1, &number);
    }

  if ( gridInqReference(gridID, NULL) )
    {
      char gridfile[8912];
      gridInqReference(gridID, gridfile);

      if ( gridfile[0] != 0 )
        cdf_put_att_text(fileID, NC_GLOBAL, "grid_file_uri", strlen(gridfile), gridfile);
    }
}

static
void cdfDefGridUUID(stream_t *streamptr, int gridID)
{
  unsigned char uuidOfHGrid[CDI_UUID_SIZE];

  gridInqUUID(gridID, uuidOfHGrid);
  if ( !cdiUUIDIsNull(uuidOfHGrid) )
    {
      char uuidOfHGridStr[37];
      uuid2str(uuidOfHGrid, uuidOfHGridStr);
      if ( uuidOfHGridStr[0] != 0 && strlen(uuidOfHGridStr) == 36 )
        {
          int fileID = streamptr->fileID;

          cdf_put_att_text(fileID, NC_GLOBAL, "uuidOfHGrid", 36, uuidOfHGridStr);

        }
    }
}

static
void cdfDefZaxisUUID(stream_t *streamptr, int zaxisID)
{
  unsigned char uuidOfVGrid[CDI_UUID_SIZE];
  zaxisInqUUID(zaxisID, uuidOfVGrid);

  if ( uuidOfVGrid[0] != 0 )
    {
      char uuidOfVGridStr[37];
      uuid2str(uuidOfVGrid, uuidOfVGridStr);
      if ( uuidOfVGridStr[0] != 0 && strlen(uuidOfVGridStr) == 36 )
        {
          int fileID = streamptr->fileID;
          if ( streamptr->ncmode == 2 ) cdf_redef(fileID);
          cdf_put_att_text(fileID, NC_GLOBAL, "uuidOfVGrid", 36, uuidOfVGridStr);
          if ( streamptr->ncmode == 2 ) cdf_enddef(fileID);
        }
    }
}

static
void cdfDefUnstructured(stream_t *streamptr, int gridID)
{
  char xunits[CDI_MAX_NAME];
  char xlongname[CDI_MAX_NAME];
  char xstdname[CDI_MAX_NAME];
  char yunits[CDI_MAX_NAME];
  char ylongname[CDI_MAX_NAME];
  char ystdname[CDI_MAX_NAME];
  char xaxisname[CDI_MAX_NAME];
  char yaxisname[CDI_MAX_NAME];
  int index;
  int gridID0, gridtype0, gridindex;
  int dimID = UNDEFID;
  int ngrids;
  int fileID;
  size_t len;
  int ncxvarid = UNDEFID, ncyvarid = UNDEFID;
  int ncbxvarid = UNDEFID, ncbyvarid = UNDEFID, ncavarid = UNDEFID;
  int nvdimID = UNDEFID;
  int vlistID;
  int xtype = NC_DOUBLE;

  if ( gridInqPrec(gridID) == DATATYPE_FLT32 ) xtype = NC_FLOAT;

  vlistID = streamptr->vlistID;
  fileID = streamptr->fileID;

  ngrids = vlistNgrids(vlistID);

  size_t dimlen = (size_t)gridInqSize(gridID);
  gridindex = vlistGridIndex(vlistID, gridID);

  gridInqXname(gridID, xaxisname);
  gridInqXlongname(gridID, xlongname);
  gridInqXstdname(gridID, xstdname);
  gridInqXunits(gridID, xunits);
  gridInqYname(gridID, yaxisname);
  gridInqYlongname(gridID, ylongname);
  gridInqYstdname(gridID, ystdname);
  gridInqYunits(gridID, yunits);

  for ( index = 0; index < ngrids; index++ )
    {
      if ( streamptr->xdimID[index] != UNDEFID )
        {
          gridID0 = vlistGrid(vlistID, index);
          gridtype0 = gridInqType(gridID0);
          if ( gridtype0 == GRID_UNSTRUCTURED )
            {
              size_t dimlen0 = (size_t)gridInqSize(gridID0);
              if ( dimlen == dimlen0 )
                if ( gridInqNvertex(gridID0) == gridInqNvertex(gridID) &&
                     IS_EQUAL(gridInqXval(gridID0, 0), gridInqXval(gridID, 0)) &&
                     IS_EQUAL(gridInqXval(gridID0, (int)dimlen-1), gridInqXval(gridID, (int)dimlen-1)) )
                  {
                    dimID = streamptr->xdimID[index];
                    ncxvarid = streamptr->ncxvarID[index];
                    ncyvarid = streamptr->ncyvarID[index];
                    ncavarid = streamptr->ncavarID[index];
                    break;
                  }
            }
        }
    }

  if ( dimID == UNDEFID )
    {
      char axisname[CDI_MAX_NAME];
      char vertname[CDI_MAX_NAME];
      strcpy(axisname, "ncells");
      strcpy(vertname, "vertices");

      checkGridName('V', xaxisname, fileID, vlistID, gridID, ngrids, 'X');
      checkGridName('V', yaxisname, fileID, vlistID, gridID, ngrids, 'Y');
      checkGridName('D', axisname, fileID, vlistID, gridID, ngrids, 'X');
      checkGridName('D', vertname, fileID, vlistID, gridID, ngrids, 'X');

      if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

      cdf_def_dim(fileID, axisname, dimlen, &dimID);

      size_t nvertex = (size_t)gridInqNvertex(gridID);
      if ( nvertex > 0 ) cdf_def_dim(fileID, vertname, nvertex, &nvdimID);

      cdfDefGridReference(streamptr, gridID);

      cdfDefGridUUID(streamptr, gridID);

      if ( gridInqXvalsPtr(gridID) )
        {
          cdf_def_var(fileID, xaxisname, (nc_type) xtype, 1, &dimID, &ncxvarid);
          cdfGridCompress(fileID, ncxvarid, (int)dimlen, streamptr->filetype, streamptr->comptype);

          if ( (len = strlen(xstdname)) )
            cdf_put_att_text(fileID, ncxvarid, "standard_name", len, xstdname);
          if ( (len = strlen(xlongname)) )
            cdf_put_att_text(fileID, ncxvarid, "long_name", len, xlongname);
          if ( (len = strlen(xunits)) )
            cdf_put_att_text(fileID, ncxvarid, "units", len, xunits);

          if ( gridInqXboundsPtr(gridID) && nvdimID != UNDEFID )
            {
              int dimIDs[2];
              dimIDs[0] = dimID;
              dimIDs[1] = nvdimID;
              strcat(xaxisname, "_");
              strcat(xaxisname, BNDS_NAME);
              cdf_def_var(fileID, xaxisname, (nc_type) xtype, 2, dimIDs, &ncbxvarid);
              cdfGridCompress(fileID, ncbxvarid, (int)dimlen, streamptr->filetype, streamptr->comptype);

              cdf_put_att_text(fileID, ncxvarid, "bounds", strlen(xaxisname), xaxisname);
            }
        }

      if ( gridInqYvalsPtr(gridID) )
        {
          cdf_def_var(fileID, yaxisname, (nc_type) xtype, 1, &dimID, &ncyvarid);
          cdfGridCompress(fileID, ncyvarid, (int)dimlen, streamptr->filetype, streamptr->comptype);

          if ( (len = strlen(ystdname)) )
            cdf_put_att_text(fileID, ncyvarid, "standard_name", len, ystdname);
          if ( (len = strlen(ylongname)) )
            cdf_put_att_text(fileID, ncyvarid, "long_name", len, ylongname);
          if ( (len = strlen(yunits)) )
            cdf_put_att_text(fileID, ncyvarid, "units", len, yunits);

          if ( gridInqYboundsPtr(gridID) && nvdimID != UNDEFID )
            {
              int dimIDs[2];
              dimIDs[0] = dimID;
              dimIDs[1] = nvdimID;
              strcat(yaxisname, "_");
              strcat(yaxisname, BNDS_NAME);
              cdf_def_var(fileID, yaxisname, (nc_type) xtype, 2, dimIDs, &ncbyvarid);
              cdfGridCompress(fileID, ncbyvarid, (int)dimlen, streamptr->filetype, streamptr->comptype);

              cdf_put_att_text(fileID, ncyvarid, "bounds", strlen(yaxisname), yaxisname);
            }
        }

      if ( gridInqAreaPtr(gridID) )
        {
          static const char yaxisname_[] = "cell_area";
          static const char units[] = "m2";
          static const char longname[] = "area of grid cell";
          static const char stdname[] = "cell_area";

          cdf_def_var(fileID, yaxisname_, (nc_type) xtype, 1, &dimID, &ncavarid);

          cdf_put_att_text(fileID, ncavarid, "standard_name", sizeof (stdname) - 1, stdname);
          cdf_put_att_text(fileID, ncavarid, "long_name", sizeof (longname) - 1, longname);
          cdf_put_att_text(fileID, ncavarid, "units", sizeof (units) - 1, units);
        }

      cdf_enddef(fileID);
      streamptr->ncmode = 2;

      if ( ncxvarid != UNDEFID ) cdf_put_var_double(fileID, ncxvarid, gridInqXvalsPtr(gridID));
      if ( ncbxvarid != UNDEFID ) cdf_put_var_double(fileID, ncbxvarid, gridInqXboundsPtr(gridID));
      if ( ncyvarid != UNDEFID ) cdf_put_var_double(fileID, ncyvarid, gridInqYvalsPtr(gridID));
      if ( ncbyvarid != UNDEFID ) cdf_put_var_double(fileID, ncbyvarid, gridInqYboundsPtr(gridID));
      if ( ncavarid != UNDEFID ) cdf_put_var_double(fileID, ncavarid, gridInqAreaPtr(gridID));
    }

  streamptr->xdimID[gridindex] = dimID;
  streamptr->ncxvarID[gridindex] = ncxvarid;
  streamptr->ncyvarID[gridindex] = ncyvarid;
  streamptr->ncavarID[gridindex] = ncavarid;
}

static
void cdf_def_vct_echam(stream_t *streamptr, int zaxisID)
{
  int type = zaxisInqType(zaxisID);

  if ( type == ZAXIS_HYBRID || type == ZAXIS_HYBRID_HALF )
    {
      int ilev = zaxisInqVctSize(zaxisID)/2;
      int mlev = ilev - 1;
      size_t start;
      size_t count = 1;
      int ncdimid, ncdimid2;
      int hyaiid, hybiid, hyamid, hybmid;
      double mval;
      char tmpname[CDI_MAX_NAME];

      if ( streamptr->vct.ilev > 0 )
        {
          if ( streamptr->vct.ilev != ilev )
            Error("more than one VCT for each file unsupported!");
          return;
        }

      if ( ilev == 0 )
        {
          Warning("VCT missing");
          return;
        }

      int fileID = streamptr->fileID;

      if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

      cdf_def_dim(fileID, "nhym", (size_t)mlev, &ncdimid);
      cdf_def_dim(fileID, "nhyi", (size_t)ilev, &ncdimid2);

      streamptr->vct.mlev = mlev;
      streamptr->vct.ilev = ilev;
      streamptr->vct.mlevID = ncdimid;
      streamptr->vct.ilevID = ncdimid2;

      cdf_def_var(fileID, "hyai", NC_DOUBLE, 1, &ncdimid2, &hyaiid);
      cdf_def_var(fileID, "hybi", NC_DOUBLE, 1, &ncdimid2, &hybiid);
      cdf_def_var(fileID, "hyam", NC_DOUBLE, 1, &ncdimid, &hyamid);
      cdf_def_var(fileID, "hybm", NC_DOUBLE, 1, &ncdimid, &hybmid);

      strcpy(tmpname, "hybrid A coefficient at layer interfaces");
      cdf_put_att_text(fileID, hyaiid, "long_name", strlen(tmpname), tmpname);
      strcpy(tmpname, "Pa");
      cdf_put_att_text(fileID, hyaiid, "units", strlen(tmpname), tmpname);
      strcpy(tmpname, "hybrid B coefficient at layer interfaces");
      cdf_put_att_text(fileID, hybiid, "long_name", strlen(tmpname), tmpname);
      strcpy(tmpname, "1");
      cdf_put_att_text(fileID, hybiid, "units", strlen(tmpname), tmpname);
      strcpy(tmpname, "hybrid A coefficient at layer midpoints");
      cdf_put_att_text(fileID, hyamid, "long_name", strlen(tmpname), tmpname);
      strcpy(tmpname, "Pa");
      cdf_put_att_text(fileID, hyamid, "units", strlen(tmpname), tmpname);
      strcpy(tmpname, "hybrid B coefficient at layer midpoints");
      cdf_put_att_text(fileID, hybmid, "long_name", strlen(tmpname), tmpname);
      strcpy(tmpname, "1");
      cdf_put_att_text(fileID, hybmid, "units", strlen(tmpname), tmpname);

      cdf_enddef(fileID);
      streamptr->ncmode = 2;

      const double *vctptr = zaxisInqVctPtr(zaxisID);

      cdf_put_var_double(fileID, hyaiid, vctptr);
      cdf_put_var_double(fileID, hybiid, vctptr+ilev);

      for ( int i = 0; i < mlev; i++ )
        {
          start = (size_t)i;
          mval = (vctptr[i] + vctptr[i+1]) * 0.5;
          cdf_put_vara_double(fileID, hyamid, &start, &count, &mval);
          mval = (vctptr[ilev+i] + vctptr[ilev+i+1]) * 0.5;
          cdf_put_vara_double(fileID, hybmid, &start, &count, &mval);
        }
    }
}

static
void cdf_def_vct_cf(stream_t *streamptr, int zaxisID, int nclevID, int ncbndsID)
{
  int type = zaxisInqType(zaxisID);

  if ( type == ZAXIS_HYBRID || type == ZAXIS_HYBRID_HALF )
    {
      int ilev = zaxisInqVctSize(zaxisID)/2;
      int mlev = ilev - 1;
      int hyaiid = 0, hybiid = 0, hyamid, hybmid;
      char tmpname[CDI_MAX_NAME];

      if ( streamptr->vct.ilev > 0 )
        {
          if ( streamptr->vct.ilev != ilev )
            Error("more than one VCT for each file unsupported!");
          return;
        }

      if ( ilev == 0 )
        {
          Warning("VCT missing");
          return;
        }

      int fileID = streamptr->fileID;

      if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

      int dimIDs[2];
      dimIDs[0] = nclevID;
      dimIDs[1] = ncbndsID;

      streamptr->vct.mlev = mlev;
      streamptr->vct.ilev = ilev;
      streamptr->vct.mlevID = nclevID;
      streamptr->vct.ilevID = nclevID;

      cdf_def_var(fileID, "ap", NC_DOUBLE, 1, dimIDs, &hyamid);
      cdf_def_var(fileID, "b", NC_DOUBLE, 1, dimIDs, &hybmid);

      strcpy(tmpname, "vertical coordinate formula term: ap(k)");
      cdf_put_att_text(fileID, hyamid, "long_name", strlen(tmpname), tmpname);
      strcpy(tmpname, "Pa");
      cdf_put_att_text(fileID, hyamid, "units", strlen(tmpname), tmpname);
      strcpy(tmpname, "vertical coordinate formula term: b(k)");
      cdf_put_att_text(fileID, hybmid, "long_name", strlen(tmpname), tmpname);
      strcpy(tmpname, "1");
      cdf_put_att_text(fileID, hybmid, "units", strlen(tmpname), tmpname);

      if ( ncbndsID != -1 )
        {
          cdf_def_var(fileID, "ap_bnds", NC_DOUBLE, 2, dimIDs, &hyaiid);
          cdf_def_var(fileID, "b_bnds", NC_DOUBLE, 2, dimIDs, &hybiid);

          strcpy(tmpname, "vertical coordinate formula term: ap(k+1/2)");
          cdf_put_att_text(fileID, hyaiid, "long_name", strlen(tmpname), tmpname);
          strcpy(tmpname, "Pa");
          cdf_put_att_text(fileID, hyaiid, "units", strlen(tmpname), tmpname);
          strcpy(tmpname, "vertical coordinate formula term: b(k+1/2)");
          cdf_put_att_text(fileID, hybiid, "long_name", strlen(tmpname), tmpname);
          strcpy(tmpname, "1");
          cdf_put_att_text(fileID, hybiid, "units", strlen(tmpname), tmpname);
        }

      cdf_enddef(fileID);
      streamptr->ncmode = 2;

      const double *vctptr = zaxisInqVctPtr(zaxisID);
      double tarray[ilev*2];

      if ( ncbndsID != -1 )
        {
          for ( int i = 0; i < mlev; ++i )
            {
              tarray[2*i ] = vctptr[i];
              tarray[2*i+1] = vctptr[i+1];
            }
          cdf_put_var_double(fileID, hyaiid, tarray);

          for ( int i = 0; i < mlev; ++i )
            {
              tarray[2*i ] = vctptr[ilev+i];
              tarray[2*i+1] = vctptr[ilev+i+1];
            }
          cdf_put_var_double(fileID, hybiid, tarray);
        }

      for ( int i = 0; i < mlev; ++i )
        tarray[i] = (vctptr[i] + vctptr[i+1]) * 0.5;
      cdf_put_var_double(fileID, hyamid, tarray);

      for ( int i = 0; i < mlev; ++i )
        tarray[i] = (vctptr[ilev+i] + vctptr[ilev+i+1]) * 0.5;
      cdf_put_var_double(fileID, hybmid, tarray);
    }
}

static
void cdf_def_zaxis_hybrid_echam(stream_t *streamptr, int type, int ncvarid, int zaxisID, int zaxisindex, int xtype, size_t dimlen, int *dimID, char *axisname)
{
  char tmpname[CDI_MAX_NAME];
  int fileID = streamptr->fileID;

  if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

  cdf_def_dim(fileID, axisname, dimlen, dimID);
  cdf_def_var(fileID, axisname, (nc_type) xtype, 1, dimID, &ncvarid);

  strcpy(tmpname, "hybrid_sigma_pressure");
  cdf_put_att_text(fileID, ncvarid, "standard_name", strlen(tmpname), tmpname);

  if ( type == ZAXIS_HYBRID )
    {
      strcpy(tmpname, "hybrid level at layer midpoints");
      cdf_put_att_text(fileID, ncvarid, "long_name", strlen(tmpname), tmpname);
      strcpy(tmpname, "hyam hybm (mlev=hyam+hybm*aps)");
      cdf_put_att_text(fileID, ncvarid, "formula", strlen(tmpname), tmpname);
      strcpy(tmpname, "ap: hyam b: hybm ps: aps");
      cdf_put_att_text(fileID, ncvarid, "formula_terms", strlen(tmpname), tmpname);
    }
  else
    {
      strcpy(tmpname, "hybrid level at layer interfaces");
      cdf_put_att_text(fileID, ncvarid, "long_name", strlen(tmpname), tmpname);
      strcpy(tmpname, "hyai hybi (ilev=hyai+hybi*aps)");
      cdf_put_att_text(fileID, ncvarid, "formula", strlen(tmpname), tmpname);
      strcpy(tmpname, "ap: hyai b: hybi ps: aps");
      cdf_put_att_text(fileID, ncvarid, "formula_terms", strlen(tmpname), tmpname);
    }

  strcpy(tmpname, "level");
  cdf_put_att_text(fileID, ncvarid, "units", strlen(tmpname), tmpname);
  strcpy(tmpname, "down");
  cdf_put_att_text(fileID, ncvarid, "positive", strlen(tmpname), tmpname);

  cdf_enddef(fileID);
  streamptr->ncmode = 2;

  cdf_put_var_double(fileID, ncvarid, zaxisInqLevelsPtr(zaxisID));

  cdf_def_vct_echam(streamptr, zaxisID);

  if ( *dimID == UNDEFID )
    {
      if ( type == ZAXIS_HYBRID )
        streamptr->zaxisID[zaxisindex] = streamptr->vct.mlevID;
      else
        streamptr->zaxisID[zaxisindex] = streamptr->vct.ilevID;
    }
}

static
void cdf_def_zaxis_hybrid_cf(stream_t *streamptr, int type, int ncvarid, int zaxisID, int zaxisindex, int xtype, size_t dimlen, int *dimID, char *axisname)
{
  char psname[CDI_MAX_NAME];
  psname[0] = 0;
  zaxisInqPsName(zaxisID, psname);
  if ( psname[0] == 0 ) strcpy(psname, "ps");

  int fileID = streamptr->fileID;
  if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

  strcpy(axisname, "lev");

  cdf_def_dim(fileID, axisname, dimlen, dimID);
  cdf_def_var(fileID, axisname, (nc_type) xtype, 1, dimID, &ncvarid);

  char tmpname[CDI_MAX_NAME];
  strcpy(tmpname, "atmosphere_hybrid_sigma_pressure_coordinate");
  cdf_put_att_text(fileID, ncvarid, "standard_name", strlen(tmpname), tmpname);

  strcpy(tmpname, "hybrid sigma pressure coordinate");
  cdf_put_att_text(fileID, ncvarid, "long_name", strlen(tmpname), tmpname);
  strcpy(tmpname, "p = ap + b*ps");
  cdf_put_att_text(fileID, ncvarid, "formula", strlen(tmpname), tmpname);
  strcpy(tmpname, "ap: ap b: b ps: ");
  strcat(tmpname, psname);
  cdf_put_att_text(fileID, ncvarid, "formula_terms", strlen(tmpname), tmpname);

  strcpy(tmpname, "1");
  cdf_put_att_text(fileID, ncvarid, "units", strlen(tmpname), tmpname);
  strcpy(tmpname, "Z");
  cdf_put_att_text(fileID, ncvarid, "axis", strlen(tmpname), tmpname);
  strcpy(tmpname, "down");
  cdf_put_att_text(fileID, ncvarid, "positive", strlen(tmpname), tmpname);

  int ncbvarid = UNDEFID;
  int nvdimID = UNDEFID;

  double lbounds[dimlen], ubounds[dimlen], levels[dimlen];

  zaxisInqLevels(zaxisID, levels);

  if ( zaxisInqLbounds(zaxisID, NULL) && zaxisInqUbounds(zaxisID, NULL) )
    {
      zaxisInqLbounds(zaxisID, lbounds);
      zaxisInqUbounds(zaxisID, ubounds);
    }
  else
    {
      for ( size_t i = 0; i < dimlen; ++i ) lbounds[i] = levels[i];
      for ( size_t i = 0; i < dimlen-1; ++i ) ubounds[i] = levels[i+1];
      ubounds[dimlen-1] = levels[dimlen-1] + 1;
    }


    {
      int dimIDs[2];
      size_t nvertex = 2;
      if ( nc_inq_dimid(fileID, BNDS_NAME, &nvdimID) != NC_NOERR )
        cdf_def_dim(fileID, BNDS_NAME, nvertex, &nvdimID);

      if ( nvdimID != UNDEFID )
        {
          strcat(axisname, "_");
          strcat(axisname, BNDS_NAME);
          dimIDs[0] = *dimID;
          dimIDs[1] = nvdimID;
          cdf_def_var(fileID, axisname, (nc_type) xtype, 2, dimIDs, &ncbvarid);
          cdf_put_att_text(fileID, ncvarid, "bounds", strlen(axisname), axisname);

          strcpy(tmpname, "atmosphere_hybrid_sigma_pressure_coordinate");
          cdf_put_att_text(fileID, ncbvarid, "standard_name", strlen(tmpname), tmpname);

          strcpy(tmpname, "p = ap + b*ps");
          cdf_put_att_text(fileID, ncbvarid, "formula", strlen(tmpname), tmpname);
          strcpy(tmpname, "ap: ap_bnds b: b_bnds ps: ");
          strcat(tmpname, psname);
          cdf_put_att_text(fileID, ncbvarid, "formula_terms", strlen(tmpname), tmpname);

          strcpy(tmpname, "1");
          cdf_put_att_text(fileID, ncbvarid, "units", strlen(tmpname), tmpname);
        }
    }

  cdf_enddef(fileID);
  streamptr->ncmode = 2;

  cdf_put_var_double(fileID, ncvarid, levels);

  if ( ncbvarid != UNDEFID )
    {
      double zbounds[2*dimlen];
      for ( size_t i = 0; i < dimlen; ++i )
        {
          zbounds[2*i ] = lbounds[i];
          zbounds[2*i+1] = ubounds[i];
        }
      cdf_put_var_double(fileID, ncbvarid, zbounds);
    }

  cdf_def_vct_cf(streamptr, zaxisID, *dimID, nvdimID);

  if ( *dimID == UNDEFID )
    {
      if ( type == ZAXIS_HYBRID )
        streamptr->zaxisID[zaxisindex] = streamptr->vct.mlevID;
      else
        streamptr->zaxisID[zaxisindex] = streamptr->vct.ilevID;
    }
}

static
void cdf_def_zaxis_hybrid(stream_t *streamptr, int type, int ncvarid, int zaxisID, int zaxisindex, int xtype, size_t dimlen, int *dimID, char *axisname)
{
  if ( (!CDI_cmor_mode && cdiConvention == CDI_CONVENTION_ECHAM) || type == ZAXIS_HYBRID_HALF )
    cdf_def_zaxis_hybrid_echam(streamptr, type, ncvarid, zaxisID, zaxisindex, xtype, dimlen, dimID, axisname);
  else
    cdf_def_zaxis_hybrid_cf(streamptr, type, ncvarid, zaxisID, zaxisindex, xtype, dimlen, dimID, axisname);
}

static
void cdfDefZaxis(stream_t *streamptr, int zaxisID)
{

  char axisname[CDI_MAX_NAME];
  char stdname[CDI_MAX_NAME];
  char longname[CDI_MAX_NAME];
  char units[CDI_MAX_NAME];
  char tmpname[CDI_MAX_NAME];
  int index;
  int zaxisID0;
  int dimID = UNDEFID;
  int dimIDs[2];
  size_t len;
  int ncvarid = UNDEFID, ncbvarid = UNDEFID;
  int nvdimID = UNDEFID;
  int ilevel = 0;
  int xtype = NC_DOUBLE;
  int positive;

  if ( zaxisInqPrec(zaxisID) == DATATYPE_FLT32 ) xtype = NC_FLOAT;

  int vlistID = streamptr->vlistID;
  int fileID = streamptr->fileID;

  int zaxisindex = vlistZaxisIndex(vlistID, zaxisID);

  int nzaxis = vlistNzaxis(vlistID);

  size_t dimlen = (size_t)zaxisInqSize(zaxisID);
  int type = zaxisInqType(zaxisID);

  int is_scalar = FALSE;
  if ( dimlen == 1 )
    {
      is_scalar = zaxisInqScalar(zaxisID);
      if ( !is_scalar && CDI_cmor_mode )
        {
          is_scalar = TRUE;
          zaxisDefScalar(zaxisID);
        }
    }

  int ndims = 1;
  if ( is_scalar ) ndims = 0;

  if ( dimlen == 1 )
    switch (type)
      {
      case ZAXIS_SURFACE:
      case ZAXIS_CLOUD_BASE:
      case ZAXIS_CLOUD_TOP:
      case ZAXIS_ISOTHERM_ZERO:
      case ZAXIS_TOA:
      case ZAXIS_SEA_BOTTOM:
      case ZAXIS_ATMOSPHERE:
      case ZAXIS_MEANSEA:
      case ZAXIS_LAKE_BOTTOM:
      case ZAXIS_SEDIMENT_BOTTOM:
      case ZAXIS_SEDIMENT_BOTTOM_TA:
      case ZAXIS_SEDIMENT_BOTTOM_TW:
      case ZAXIS_MIX_LAYER:
        return;
      }

  zaxisInqName(zaxisID, axisname);
  if ( dimID == UNDEFID )
    {
      char axisname0[CDI_MAX_NAME];
      char axisname2[CDI_MAX_NAME];
      int checkname = FALSE;
      int status;


      checkname = TRUE;
      ilevel = 0;

      while ( checkname )
        {
          strcpy(axisname2, axisname);
          if ( ilevel ) sprintf(&axisname2[strlen(axisname2)], "_%d", ilevel+1);

          status = nc_inq_varid(fileID, axisname2, &ncvarid);
          if ( status != NC_NOERR )
            {
              if ( ilevel )
                {

                  for ( index = 0; index < nzaxis; index++ )
                    {
                      zaxisID0 = vlistZaxis(vlistID, index);
                      if ( zaxisID != zaxisID0 )
                        {
                          zaxisInqName(zaxisID0, axisname0);
                          if ( strcmp(axisname0, axisname2) == 0 ) break;
                        }
                    }
                  if ( index == nzaxis ) checkname = FALSE;
                }
              else
                {
                  checkname = FALSE;
                }
            }

          if ( checkname ) ilevel++;

          if ( ilevel > 99 ) break;
        }

      if ( ilevel ) sprintf(&axisname[strlen(axisname)], "_%1d", ilevel+1);

      if ( type == ZAXIS_REFERENCE )
        cdfDefZaxisUUID(streamptr, zaxisID);

      if ( type == ZAXIS_HYBRID || type == ZAXIS_HYBRID_HALF )
        {
          cdf_def_zaxis_hybrid(streamptr, type, ncvarid, zaxisID, zaxisindex, xtype, dimlen, &dimID, axisname);
        }
      else
        {
          if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

          if ( ndims ) cdf_def_dim(fileID, axisname, dimlen, &dimID);

          zaxisInqLongname(zaxisID, longname);
          zaxisInqUnits(zaxisID, units);
          zaxisInqStdname(zaxisID, stdname);

          cdf_def_var(fileID, axisname, (nc_type) xtype, ndims, &dimID, &ncvarid);

          if ( (len = strlen(stdname)) )
            cdf_put_att_text(fileID, ncvarid, "standard_name", len, stdname);
          if ( (len = strlen(longname)) )
            cdf_put_att_text(fileID, ncvarid, "long_name", len, longname);
          if ( (len = strlen(units)) )
            cdf_put_att_text(fileID, ncvarid, "units", len, units);

          positive = zaxisInqPositive(zaxisID);
          if ( positive == POSITIVE_UP )
            {
              strcpy(tmpname, "up");
              cdf_put_att_text(fileID, ncvarid, "positive", strlen(tmpname), tmpname);
            }
          else if ( positive == POSITIVE_DOWN )
            {
              strcpy(tmpname, "down");
              cdf_put_att_text(fileID, ncvarid, "positive", strlen(tmpname), tmpname);
            }

          cdf_put_att_text(fileID, ncvarid, "axis", 1, "Z");

          if ( zaxisInqLbounds(zaxisID, NULL) && zaxisInqUbounds(zaxisID, NULL) )
            {
              size_t nvertex = 2;
              if ( nc_inq_dimid(fileID, BNDS_NAME, &nvdimID) != NC_NOERR )
                cdf_def_dim(fileID, BNDS_NAME, nvertex, &nvdimID);

              if ( nvdimID != UNDEFID )
                {
                  strcat(axisname, "_");
                  strcat(axisname, BNDS_NAME);
                  if ( ndims ) dimIDs[0] = dimID;
                  dimIDs[ndims] = nvdimID;
                  cdf_def_var(fileID, axisname, (nc_type) xtype, ndims+1, dimIDs, &ncbvarid);
                  cdf_put_att_text(fileID, ncvarid, "bounds", strlen(axisname), axisname);
                }
            }

          cdf_enddef(fileID);
          streamptr->ncmode = 2;

          cdf_put_var_double(fileID, ncvarid, zaxisInqLevelsPtr(zaxisID));

          if ( ncbvarid != UNDEFID )
            {
              double lbounds[dimlen], ubounds[dimlen], zbounds[2*dimlen];
              zaxisInqLbounds(zaxisID, lbounds);
              zaxisInqUbounds(zaxisID, ubounds);
              for ( size_t i = 0; i < dimlen; ++i )
                {
                  zbounds[2*i ] = lbounds[i];
                  zbounds[2*i+1] = ubounds[i];
                }

              cdf_put_var_double(fileID, ncbvarid, zbounds);
            }

          if ( ndims == 0 ) streamptr->nczvarID[zaxisindex] = ncvarid;
        }
    }

  if ( dimID != UNDEFID )
    streamptr->zaxisID[zaxisindex] = dimID;
}

static
void cdfDefPole(stream_t *streamptr, int gridID)
{
  int ncvarid = UNDEFID;
  char varname[] = "rotated_pole";
  char mapname[] = "rotated_latitude_longitude";

  int fileID = streamptr->fileID;

  double ypole = gridInqYpole(gridID);
  double xpole = gridInqXpole(gridID);
  double angle = gridInqAngle(gridID);

  cdf_redef(fileID);

  int ncerrcode = nc_def_var(fileID, varname, (nc_type) NC_CHAR, 0, NULL, &ncvarid);
  if ( ncerrcode == NC_NOERR )
    {
      cdf_put_att_text(fileID, ncvarid, "grid_mapping_name", strlen(mapname), mapname);
      cdf_put_att_double(fileID, ncvarid, "grid_north_pole_latitude", NC_DOUBLE, 1, &ypole);
      cdf_put_att_double(fileID, ncvarid, "grid_north_pole_longitude", NC_DOUBLE, 1, &xpole);
      if ( IS_NOT_EQUAL(angle, 0) )
        cdf_put_att_double(fileID, ncvarid, "north_pole_grid_longitude", NC_DOUBLE, 1, &angle);
    }

  cdf_enddef(fileID);
}


static
void cdfDefMapping(stream_t *streamptr, int gridID)
{
  int ncvarid = UNDEFID;
  int fileID = streamptr->fileID;

  if ( gridInqType(gridID) == GRID_SINUSOIDAL )
    {
      char varname[] = "sinusoidal";
      char mapname[] = "sinusoidal";

      cdf_redef(fileID);

      int ncerrcode = nc_def_var(fileID, varname, (nc_type) NC_CHAR, 0, NULL, &ncvarid);
      if ( ncerrcode == NC_NOERR )
        {
          cdf_put_att_text(fileID, ncvarid, "grid_mapping_name", strlen(mapname), mapname);




        }

      cdf_enddef(fileID);
    }
  else if ( gridInqType(gridID) == GRID_LAEA )
    {
      char varname[] = "laea";
      char mapname[] = "lambert_azimuthal_equal_area";

      cdf_redef(fileID);

      int ncerrcode = nc_def_var(fileID, varname, (nc_type) NC_CHAR, 0, NULL, &ncvarid);
      if ( ncerrcode == NC_NOERR )
        {
          double a, lon_0, lat_0;

          gridInqLaea(gridID, &a, &lon_0, &lat_0);

          cdf_put_att_text(fileID, ncvarid, "grid_mapping_name", strlen(mapname), mapname);
          cdf_put_att_double(fileID, ncvarid, "earth_radius", NC_DOUBLE, 1, &a);
          cdf_put_att_double(fileID, ncvarid, "longitude_of_projection_origin", NC_DOUBLE, 1, &lon_0);
          cdf_put_att_double(fileID, ncvarid, "latitude_of_projection_origin", NC_DOUBLE, 1, &lat_0);
        }

      cdf_enddef(fileID);
    }
  else if ( gridInqType(gridID) == GRID_LCC2 )
    {
      char varname[] = "Lambert_Conformal";
      char mapname[] = "lambert_conformal_conic";

      cdf_redef(fileID);

      int ncerrcode = nc_def_var(fileID, varname, (nc_type) NC_CHAR, 0, NULL, &ncvarid);
      if ( ncerrcode == NC_NOERR )
        {
          double radius, lon_0, lat_0, lat_1, lat_2;

          gridInqLcc2(gridID, &radius, &lon_0, &lat_0, &lat_1, &lat_2);

          cdf_put_att_text(fileID, ncvarid, "grid_mapping_name", strlen(mapname), mapname);
          if ( radius > 0 )
            cdf_put_att_double(fileID, ncvarid, "earth_radius", NC_DOUBLE, 1, &radius);
          cdf_put_att_double(fileID, ncvarid, "longitude_of_central_meridian", NC_DOUBLE, 1, &lon_0);
          cdf_put_att_double(fileID, ncvarid, "latitude_of_projection_origin", NC_DOUBLE, 1, &lat_0);
          if ( IS_EQUAL(lat_1, lat_2) )
            cdf_put_att_double(fileID, ncvarid, "standard_parallel", NC_DOUBLE, 1, &lat_1);
          else
            {
              double lat_1_2[2];
              lat_1_2[0] = lat_1;
              lat_1_2[1] = lat_2;
              cdf_put_att_double(fileID, ncvarid, "standard_parallel", NC_DOUBLE, 2, lat_1_2);
            }
        }

      cdf_enddef(fileID);
    }
}


static
void cdfDefGrid(stream_t *streamptr, int gridID)
{
  int vlistID = streamptr->vlistID;
  int gridindex = vlistGridIndex(vlistID, gridID);
  if ( streamptr->xdimID[gridindex] != UNDEFID ) return;

  int gridtype = gridInqType(gridID);
  int size = gridInqSize(gridID);

  if ( CDI_Debug )
    Message("gridtype = %d  size = %d", gridtype, size);

  if ( gridtype == GRID_GAUSSIAN ||
       gridtype == GRID_LONLAT ||
       gridtype == GRID_GENERIC )
    {
      if ( gridtype == GRID_GENERIC )
        {
          if ( size == 1 && gridInqXsize(gridID) == 0 && gridInqYsize(gridID) == 0 )
            {

            }
          else
            {
              int lx = 0, ly = 0;
              if ( gridInqXsize(gridID) > 0 )
                {
                  cdfDefXaxis(streamptr, gridID, 1);
                  lx = 1;
                }

              if ( gridInqYsize(gridID) > 0 )
                {
                  cdfDefYaxis(streamptr, gridID, 1);
                  ly = 1;
                }

              if ( lx == 0 && ly == 0 ) cdfDefGdim(streamptr, gridID);
            }
        }
      else
        {
          int ndims = 1;
          if ( gridtype == GRID_LONLAT && size == 1 && gridInqHasDims(gridID) == FALSE )
            ndims = 0;

          if ( gridInqXsize(gridID) > 0 ) cdfDefXaxis(streamptr, gridID, ndims);
          if ( gridInqYsize(gridID) > 0 ) cdfDefYaxis(streamptr, gridID, ndims);
        }

      if ( gridIsRotated(gridID) ) cdfDefPole(streamptr, gridID);
    }
  else if ( gridtype == GRID_CURVILINEAR )
    {
      cdfDefCurvilinear(streamptr, gridID);
    }
  else if ( gridtype == GRID_UNSTRUCTURED )
    {
      cdfDefUnstructured(streamptr, gridID);
    }
  else if ( gridtype == GRID_GAUSSIAN_REDUCED )
    {
      cdfDefRgrid(streamptr, gridID);
    }
  else if ( gridtype == GRID_SPECTRAL )
    {
      cdfDefComplex(streamptr, gridID);
      cdfDefSP(streamptr, gridID);
    }
  else if ( gridtype == GRID_FOURIER )
    {
      cdfDefComplex(streamptr, gridID);
      cdfDefFC(streamptr, gridID);
    }
  else if ( gridtype == GRID_TRAJECTORY )
    {
      cdfDefTrajLon(streamptr, gridID);
      cdfDefTrajLat(streamptr, gridID);
    }
  else if ( gridtype == GRID_SINUSOIDAL || gridtype == GRID_LAEA || gridtype == GRID_LCC2 )
    {
      cdfDefXaxis(streamptr, gridID, 1);
      cdfDefYaxis(streamptr, gridID, 1);

      cdfDefMapping(streamptr, gridID);
    }






  else
    {
      Error("Unsupported grid type: %s", gridNamePtr(gridtype));
    }
}

static
void scale_add(size_t size, double *data, double addoffset, double scalefactor)
{
  int laddoffset;
  int lscalefactor;

  laddoffset = IS_NOT_EQUAL(addoffset, 0);
  lscalefactor = IS_NOT_EQUAL(scalefactor, 1);

  if ( laddoffset || lscalefactor )
    {
      for (size_t i = 0; i < size; ++i )
        {
          if ( lscalefactor ) data[i] *= scalefactor;
          if ( laddoffset ) data[i] += addoffset;
        }
    }
}

static
void cdfCreateRecords(stream_t *streamptr, int tsID)
{
  int varID, levelID, recID, vrecID, zaxisID;
  int nlev, nvrecs;

  int vlistID = streamptr->vlistID;

  if ( tsID < 0 || (tsID >= streamptr->ntsteps && tsID > 0) ) return;

  if ( streamptr->tsteps[tsID].nallrecs > 0 ) return;

  tsteps_t* sourceTstep = streamptr->tsteps;
  tsteps_t* destTstep = sourceTstep + tsID;

  int nvars = vlistNvars(vlistID);
  int nrecs = vlistNrecs(vlistID);

  if ( nrecs <= 0 ) return;

  if ( tsID == 0 )
    {
      nvrecs = nrecs;

      streamptr->nrecs += nrecs;

      destTstep->records = (record_t *) Malloc((size_t)nrecs*sizeof(record_t));
      destTstep->nrecs = nrecs;
      destTstep->nallrecs = nrecs;
      destTstep->recordSize = nrecs;
      destTstep->curRecID = UNDEFID;
      destTstep->recIDs = (int *) Malloc((size_t)nvrecs*sizeof (int));;
      for ( recID = 0; recID < nvrecs; recID++ ) destTstep->recIDs[recID] = recID;

      record_t *records = destTstep->records;

      recID = 0;
      for ( varID = 0; varID < nvars; varID++ )
        {
          zaxisID = vlistInqVarZaxis(vlistID, varID);
          nlev = zaxisInqSize(zaxisID);
          for ( levelID = 0; levelID < nlev; levelID++ )
            {
              recordInitEntry(&records[recID]);
              records[recID].varID = (short)varID;
              records[recID].levelID = (short)levelID;
              recID++;
            }
        }
    }
  else if ( tsID == 1 )
    {
      nvrecs = 0;
      for ( varID = 0; varID < nvars; varID++ )
        {
          if ( vlistInqVarTsteptype(vlistID, varID) != TSTEP_CONSTANT )
            {
              zaxisID = vlistInqVarZaxis(vlistID, varID);
              nvrecs += zaxisInqSize(zaxisID);
            }
        }

      streamptr->nrecs += nvrecs;

      destTstep->records = (record_t *) Malloc((size_t)nrecs*sizeof(record_t));
      destTstep->nrecs = nvrecs;
      destTstep->nallrecs = nrecs;
      destTstep->recordSize = nrecs;
      destTstep->curRecID = UNDEFID;

      memcpy(destTstep->records, sourceTstep->records, (size_t)nrecs*sizeof(record_t));

      if ( nvrecs )
        {
          destTstep->recIDs = (int *) Malloc((size_t)nvrecs * sizeof (int));
          vrecID = 0;
          for ( recID = 0; recID < nrecs; recID++ )
            {
              varID = destTstep->records[recID].varID;
              if ( vlistInqVarTsteptype(vlistID, varID) != TSTEP_CONSTANT )
                {
                  destTstep->recIDs[vrecID++] = recID;
                }
            }
        }
    }
  else
    {
      if ( streamptr->tsteps[1].records == 0 ) cdfCreateRecords(streamptr, 1);

      nvrecs = streamptr->tsteps[1].nrecs;

      streamptr->nrecs += nvrecs;

      destTstep->records = (record_t *) Malloc((size_t)nrecs*sizeof(record_t));
      destTstep->nrecs = nvrecs;
      destTstep->nallrecs = nrecs;
      destTstep->recordSize = nrecs;
      destTstep->curRecID = UNDEFID;

      memcpy(destTstep->records, sourceTstep->records, (size_t)nrecs*sizeof(record_t));

      destTstep->recIDs = (int *) Malloc((size_t)nvrecs * sizeof(int));

      memcpy(destTstep->recIDs, streamptr->tsteps[1].recIDs, (size_t)nvrecs*sizeof(int));
    }
}


static
int cdfTimeDimID(int fileID, int ndims, int nvars)
{
  int dimid = UNDEFID;
  int timedimid = UNDEFID;
  char dimname[80];
  char timeunits[CDI_MAX_NAME];
  char attname[CDI_MAX_NAME];
  char name[CDI_MAX_NAME];
  nc_type xtype;
  int nvdims, nvatts;
  int dimids[9];
  int varid, iatt;

  for ( dimid = 0; dimid < ndims; dimid++ )
    {
      cdf_inq_dimname(fileID, dimid, dimname);
      if ( memcmp(dimname, "time", 4) == 0 )
        {
          timedimid = dimid;
          break;
        }
    }

  if ( timedimid == UNDEFID )
    {
      for ( varid = 0; varid < nvars; varid++ )
        {
          cdf_inq_var(fileID, varid, name, &xtype, &nvdims, dimids, &nvatts);
          if ( nvdims == 1 )
            {
              for ( iatt = 0; iatt < nvatts; iatt++ )
                {
                  cdf_inq_attname(fileID, varid, iatt, attname);
                  if ( strncmp(attname, "units", 5) == 0 )
                    {
                      cdfGetAttText(fileID, varid, "units", sizeof(timeunits), timeunits);
                      strtolower(timeunits);

                      if ( isTimeUnits(timeunits) )
                        {
                          timedimid = dimids[0];
                          break;
                        }
                    }
                }
            }
        }
    }

  return (timedimid);
}

static
void init_ncdims(long ndims, ncdim_t *ncdims)
{
  for ( long ncdimid = 0; ncdimid < ndims; ncdimid++ )
    {
      ncdims[ncdimid].ncvarid = UNDEFID;
      ncdims[ncdimid].dimtype = UNDEFID;
      ncdims[ncdimid].len = 0;
      ncdims[ncdimid].name[0] = 0;
    }
}

static
void init_ncvars(long nvars, ncvar_t *ncvars)
{
  for ( long ncvarid = 0; ncvarid < nvars; ++ncvarid )
    {
      ncvars[ncvarid].ncid = UNDEFID;
      ncvars[ncvarid].ignore = FALSE;
      ncvars[ncvarid].isvar = UNDEFID;
      ncvars[ncvarid].islon = FALSE;
      ncvars[ncvarid].islat = FALSE;
      ncvars[ncvarid].islev = FALSE;
      ncvars[ncvarid].istime = FALSE;
      ncvars[ncvarid].warn = FALSE;
      ncvars[ncvarid].tsteptype = TSTEP_CONSTANT;
      ncvars[ncvarid].param = UNDEFID;
      ncvars[ncvarid].code = UNDEFID;
      ncvars[ncvarid].tabnum = 0;
      ncvars[ncvarid].calendar = FALSE;
      ncvars[ncvarid].climatology = FALSE;
      ncvars[ncvarid].bounds = UNDEFID;
      ncvars[ncvarid].lformula = FALSE;
      ncvars[ncvarid].lformulaterms = FALSE;
      ncvars[ncvarid].gridID = UNDEFID;
      ncvars[ncvarid].zaxisID = UNDEFID;
      ncvars[ncvarid].gridtype = UNDEFID;
      ncvars[ncvarid].zaxistype = UNDEFID;
      ncvars[ncvarid].xdim = UNDEFID;
      ncvars[ncvarid].ydim = UNDEFID;
      ncvars[ncvarid].zdim = UNDEFID;
      ncvars[ncvarid].xvarid = UNDEFID;
      ncvars[ncvarid].yvarid = UNDEFID;
      ncvars[ncvarid].zvarid = UNDEFID;
      ncvars[ncvarid].tvarid = UNDEFID;
      ncvars[ncvarid].psvarid = UNDEFID;
      ncvars[ncvarid].ncoordvars = 0;
      for ( int i = 0; i < MAX_COORDVARS; ++i )
        ncvars[ncvarid].coordvarids[i] = UNDEFID;
      ncvars[ncvarid].nauxvars = 0;
      for ( int i = 0; i < MAX_AUXVARS; ++i )
        ncvars[ncvarid].auxvarids[i] = UNDEFID;
      ncvars[ncvarid].cellarea = UNDEFID;
      ncvars[ncvarid].tableID = UNDEFID;
      ncvars[ncvarid].xtype = 0;
      ncvars[ncvarid].ndims = 0;
      ncvars[ncvarid].gmapid = UNDEFID;
      ncvars[ncvarid].vctsize = 0;
      ncvars[ncvarid].vct = NULL;
      ncvars[ncvarid].truncation = 0;
      ncvars[ncvarid].position = 0;
      ncvars[ncvarid].positive = 0;
      ncvars[ncvarid].chunked = 0;
      ncvars[ncvarid].chunktype = UNDEFID;
      ncvars[ncvarid].defmissval = 0;
      ncvars[ncvarid].deffillval = 0;
      ncvars[ncvarid].missval = 0;
      ncvars[ncvarid].fillval = 0;
      ncvars[ncvarid].addoffset = 0;
      ncvars[ncvarid].scalefactor = 1;
      ncvars[ncvarid].name[0] = 0;
      ncvars[ncvarid].longname[0] = 0;
      ncvars[ncvarid].stdname[0] = 0;
      ncvars[ncvarid].units[0] = 0;
      ncvars[ncvarid].extra[0] = 0;
      ncvars[ncvarid].natts = 0;
      ncvars[ncvarid].atts = NULL;
      ncvars[ncvarid].deflate = 0;
      ncvars[ncvarid].lunsigned = 0;
      ncvars[ncvarid].lvalidrange = 0;
      ncvars[ncvarid].validrange[0] = VALIDMISS;
      ncvars[ncvarid].validrange[1] = VALIDMISS;
      ncvars[ncvarid].ensdata = NULL;
    }
}

static
void cdfSetVar(ncvar_t *ncvars, int ncvarid, short isvar)
{
  if ( ncvars[ncvarid].isvar != UNDEFID &&
       ncvars[ncvarid].isvar != isvar &&
       ncvars[ncvarid].warn == FALSE )
    {
      if ( ! ncvars[ncvarid].ignore )
        Warning("Inconsistent variable definition for %s!", ncvars[ncvarid].name);

      ncvars[ncvarid].warn = TRUE;
      isvar = FALSE;
    }

  ncvars[ncvarid].isvar = isvar;
}

static
void cdfSetDim(ncvar_t *ncvars, int ncvarid, int dimid, int dimtype)
{
  if ( ncvars[ncvarid].dimtype[dimid] != UNDEFID &&
       ncvars[ncvarid].dimtype[dimid] != dimtype )
    {
      Warning("Inconsistent dimension definition for %s! dimid = %d;  type = %d;  newtype = %d",
              ncvars[ncvarid].name, dimid, ncvars[ncvarid].dimtype[dimid], dimtype);
    }

  ncvars[ncvarid].dimtype[dimid] = dimtype;
}

static
int isLonAxis(const char *units, const char *stdname)
{
  int status = FALSE;
  char lc_units[16];

  memcpy(lc_units, units, 15);
  lc_units[15] = 0;
  strtolower(lc_units);

  if ( ((memcmp(lc_units, "degree", 6) == 0 || memcmp(lc_units, "radian", 6) == 0) &&
        (memcmp(stdname, "grid_longitude", 14) == 0 || memcmp(stdname, "longitude", 9) == 0)) )
    {
      status = TRUE;
    }

  if ( status == FALSE &&
       memcmp(stdname, "grid_latitude", 13) && memcmp(stdname, "latitude", 8) &&
       memcmp(lc_units, "degree", 6) == 0 )
    {
      int ioff = 6;
      if ( lc_units[ioff] == 's' ) ioff++;
      if ( lc_units[ioff] == '_' ) ioff++;
      if ( lc_units[ioff] == 'e' ) status = TRUE;
    }

  return (status);
}

static
int isLatAxis(const char *units, const char *stdname)
{
  int status = FALSE;
  char lc_units[16];

  memcpy(lc_units, units, 15);
  lc_units[15] = 0;
  strtolower(lc_units);

  if ( ((memcmp(lc_units, "degree", 6) == 0 || memcmp(lc_units, "radian", 6) == 0) &&
        (memcmp(stdname, "grid_latitude", 13) == 0 || memcmp(stdname, "latitude", 8) == 0)) )
    {
      status = TRUE;
    }

  if ( status == FALSE &&
       memcmp(stdname, "grid_longitude", 14) && memcmp(stdname, "longitude", 9) &&
       memcmp(lc_units, "degree", 6) == 0 )
    {
      int ioff = 6;
      if ( lc_units[ioff] == 's' ) ioff++;
      if ( lc_units[ioff] == '_' ) ioff++;
      if ( lc_units[ioff] == 'n' || lc_units[ioff] == 's' ) status = TRUE;
    }

  return (status);
}

static
int isDBLAxis( const char *longname)
{
  int status = FALSE;

  if ( strcmp(longname, "depth below land") == 0 ||
       strcmp(longname, "depth_below_land") == 0 ||
       strcmp(longname, "levels below the surface") == 0 )
    {





        status = TRUE;
    }

  return (status);
}

static
int unitsIsMeter(const char *units)
{
  return (units[0] == 'm' && (!units[1] || strncmp(units, "meter", 5) == 0));
}

static
int isDepthAxis(const char *stdname, const char *longname)
{
  int status = FALSE;

  if ( strcmp(stdname, "depth") == 0 ) status = TRUE;

  if ( status == FALSE )
    if ( strcmp(longname, "depth_below_sea") == 0 ||
         strcmp(longname, "depth below sea") == 0 )
      {
        status = TRUE;
      }

  return (status);
}

static
int isHeightAxis(const char *stdname, const char *longname)
{
  int status = FALSE;

  if ( strcmp(stdname, "height") == 0 ) status = TRUE;

  if ( status == FALSE )
    if ( strcmp(longname, "height") == 0 ||
         strcmp(longname, "height above the surface") == 0 )
      {
        status = TRUE;
      }

  return (status);
}

static
int unitsIsPressure(const char *units)
{
  int status = FALSE;

  if ( memcmp(units, "millibar", 8) == 0 ||
       memcmp(units, "mb", 2) == 0 ||
       memcmp(units, "hectopas", 8) == 0 ||
       memcmp(units, "hPa", 3) == 0 ||
       memcmp(units, "Pa", 2) == 0 )
    {
      status = TRUE;
    }

  return status;
}

static
void scan_hybrid_formula(int ncid, int ncfvarid, int *apvarid, int *bvarid, int *psvarid)
{
  *apvarid = -1;
  *bvarid = -1;
  *psvarid = -1;
  const int attstringlen = 8192; char attstring[8192];
  cdfGetAttText(ncid, ncfvarid, "formula", attstringlen, attstring);
  if ( strcmp(attstring, "p = ap + b*ps") == 0 )
    {
      int lstop = FALSE;
      int dimvarid;
      cdfGetAttText(ncid, ncfvarid, "formula_terms", attstringlen, attstring);
      char *pstring = attstring;

      for ( int i = 0; i < 3; i++ )
        {
          while ( isspace((int) *pstring) ) pstring++;
          if ( *pstring == 0 ) break;
          char *tagname = pstring;
          while ( !isspace((int) *pstring) && *pstring != 0 ) pstring++;
          if ( *pstring == 0 ) lstop = TRUE;
          *pstring++ = 0;

          while ( isspace((int) *pstring) ) pstring++;
          if ( *pstring == 0 ) break;
          char *varname = pstring;
          while ( !isspace((int) *pstring) && *pstring != 0 ) pstring++;
          if ( *pstring == 0 ) lstop = TRUE;
          *pstring++ = 0;

          int status = nc_inq_varid(ncid, varname, &dimvarid);
          if ( status == NC_NOERR )
            {
              if ( strcmp(tagname, "ap:") == 0 ) *apvarid = dimvarid;
              else if ( strcmp(tagname, "b:") == 0 ) *bvarid = dimvarid;
              else if ( strcmp(tagname, "ps:") == 0 ) *psvarid = dimvarid;
            }
          else if ( strcmp(tagname, "ps:") != 0 )
            {
              Warning("%s - %s", nc_strerror(status), varname);
            }

          if ( lstop ) break;
        }
    }
}

static
int isHybridSigmaPressureCoordinate(int ncid, int ncvarid, ncvar_t *ncvars, const ncdim_t *ncdims)
{
  int status = FALSE;
  int ncfvarid = ncvarid;
  ncvar_t *ncvar = &ncvars[ncvarid];

  if ( strcmp(ncvar->stdname, "atmosphere_hybrid_sigma_pressure_coordinate") == 0 )
    {
      cdiConvention = CDI_CONVENTION_CF;

      status = TRUE;
      ncvar->zaxistype = ZAXIS_HYBRID;
      int dimid = ncvar->dimids[0];
      size_t dimlen = ncdims[dimid].len;

      int apvarid1 = -1, bvarid1 = -1, psvarid1 = -1;
      if ( ncvars[ncfvarid].lformula && ncvars[ncfvarid].lformulaterms )
        scan_hybrid_formula(ncid, ncfvarid, &apvarid1, &bvarid1, &psvarid1);
      if ( apvarid1 != -1 ) ncvars[apvarid1].isvar = FALSE;
      if ( bvarid1 != -1 ) ncvars[bvarid1].isvar = FALSE;
      if ( psvarid1 != -1 ) ncvar->psvarid = psvarid1;

      if ( ncvar->bounds != UNDEFID && ncvars[ncvar->bounds].lformula && ncvars[ncvar->bounds].lformulaterms )
        {
          ncfvarid = ncvar->bounds;
          int apvarid2 = -1, bvarid2 = -1, psvarid2 = -1;
          if ( ncvars[ncfvarid].lformula && ncvars[ncfvarid].lformulaterms )
            scan_hybrid_formula(ncid, ncfvarid, &apvarid2, &bvarid2, &psvarid2);
          if ( apvarid2 != -1 && bvarid2 != -1 )
            {
              ncvars[apvarid2].isvar = FALSE;
              ncvars[bvarid2].isvar = FALSE;

              if ( dimid == ncvars[apvarid2].dimids[0] && ncdims[ncvars[apvarid2].dimids[1]].len == 2 )
                {
                  double abuf[dimlen*2], bbuf[dimlen*2];
                  cdf_get_var_double(ncid, apvarid2, abuf);
                  cdf_get_var_double(ncid, bvarid2, bbuf);




                  size_t vctsize = (dimlen+1)*2;
                  double *vct = (double *) Malloc(vctsize * sizeof(double));
                  for ( size_t i = 0; i < dimlen; ++i )
                    {
                      vct[i] = abuf[i*2];
                      vct[i+dimlen+1] = bbuf[i*2];
                    }
                  vct[dimlen] = abuf[dimlen*2-1];
                  vct[dimlen*2+1] = bbuf[dimlen*2-1];

                  ncvar->vct = vct;
                  ncvar->vctsize = vctsize;
                }
            }
        }
    }

  return status;
}


static
int isGaussGrid(size_t ysize, double yinc, const double *yvals)
{
  int lgauss = FALSE;
  double *yv, *yw;

  if ( IS_EQUAL(yinc, 0) && ysize > 2 )
    {
      size_t i;
      yv = (double *) Malloc(ysize*sizeof(double));
      yw = (double *) Malloc(ysize*sizeof(double));
      gaussaw(yv, yw, ysize);
      Free(yw);
      for ( i = 0; i < ysize; i++ )
        yv[i] = asin(yv[i])/M_PI*180.0;

      for ( i = 0; i < ysize; i++ )
        if ( fabs(yv[i] - yvals[i]) >
             ((yv[0] - yv[1])/500) ) break;

      if ( i == ysize ) lgauss = TRUE;


      if ( lgauss == FALSE )
        {
          for ( i = 0; i < ysize; i++ )
            if ( fabs(yv[i] - yvals[ysize-i-1]) >
                 ((yv[0] - yv[1])/500) ) break;

          if ( i == ysize ) lgauss = TRUE;
        }

      Free(yv);
    }

  return (lgauss);
}

static
void printNCvars(const ncvar_t *ncvars, int nvars, const char *oname)
{
  char axis[7];
  int ncvarid, i;
  int ndim;
  static const char iaxis[] = {'t', 'z', 'y', 'x'};

  fprintf(stderr, "%s:\n", oname);

  for ( ncvarid = 0; ncvarid < nvars; ncvarid++ )
    {
      ndim = 0;
      if ( ncvars[ncvarid].isvar )
        {
          axis[ndim++] = 'v';
          axis[ndim++] = ':';
          for ( i = 0; i < ncvars[ncvarid].ndims; i++ )
            {






              if ( ncvars[ncvarid].dimtype[i] == T_AXIS ) axis[ndim++] = iaxis[0];
              else if ( ncvars[ncvarid].dimtype[i] == Z_AXIS ) axis[ndim++] = iaxis[1];
              else if ( ncvars[ncvarid].dimtype[i] == Y_AXIS ) axis[ndim++] = iaxis[2];
              else if ( ncvars[ncvarid].dimtype[i] == X_AXIS ) axis[ndim++] = iaxis[3];
              else axis[ndim++] = '?';
            }
        }
      else
        {
          axis[ndim++] = 'c';
          axis[ndim++] = ':';
          if ( ncvars[ncvarid].istime ) axis[ndim++] = iaxis[0];
          else if ( ncvars[ncvarid].islev ) axis[ndim++] = iaxis[1];
          else if ( ncvars[ncvarid].islat ) axis[ndim++] = iaxis[2];
          else if ( ncvars[ncvarid].islon ) axis[ndim++] = iaxis[3];
          else axis[ndim++] = '?';
        }

      axis[ndim++] = 0;

      fprintf(stderr, "%3d %3d  %-6s %s\n", ncvarid, ndim-3, axis, ncvars[ncvarid].name);
    }
}

static
void cdfScanVarAttributes(int nvars, ncvar_t *ncvars, ncdim_t *ncdims,
                          int timedimid, int modelID, int format)
{
  int ncid;
  int ncdimid;
  int nvdims, nvatts;
  int *dimidsp;
  int iatt;
  nc_type xtype, atttype;
  size_t attlen;
  char name[CDI_MAX_NAME];
  char attname[CDI_MAX_NAME];
  const int attstringlen = 8192; char attstring[8192];

  int nchecked_vars = 0;
  enum { max_check_vars = 9 };
  char *checked_vars[max_check_vars];
  for ( int i = 0; i < max_check_vars; ++i ) checked_vars[i] = NULL;

  for ( int ncvarid = 0; ncvarid < nvars; ncvarid++ )
    {
      ncid = ncvars[ncvarid].ncid;
      dimidsp = ncvars[ncvarid].dimids;

      cdf_inq_var(ncid, ncvarid, name, &xtype, &nvdims, dimidsp, &nvatts);
      strcpy(ncvars[ncvarid].name, name);

      for ( ncdimid = 0; ncdimid < nvdims; ncdimid++ )
        ncvars[ncvarid].dimtype[ncdimid] = -1;

      ncvars[ncvarid].xtype = xtype;
      ncvars[ncvarid].ndims = nvdims;

#if defined (HAVE_NETCDF4)
      if ( format == NC_FORMAT_NETCDF4_CLASSIC || format == NC_FORMAT_NETCDF4 )
        {
          char buf[CDI_MAX_NAME];
          int shuffle, deflate, deflate_level;
          size_t chunks[nvdims];
          int storage_in;
          nc_inq_var_deflate(ncid, ncvarid, &shuffle, &deflate, &deflate_level);
          if ( deflate > 0 ) ncvars[ncvarid].deflate = 1;

          if ( nc_inq_var_chunking(ncid, ncvarid, &storage_in, chunks) == NC_NOERR )
            {
              if ( storage_in == NC_CHUNKED )
                {
                  ncvars[ncvarid].chunked = 1;
                  for ( int i = 0; i < nvdims; ++i ) ncvars[ncvarid].chunks[i] = (int)chunks[i];
                  if ( CDI_Debug )
                    {
                      fprintf(stderr, "%s: chunking %d %d %d  chunks ", name, storage_in, NC_CONTIGUOUS, NC_CHUNKED);
                      for ( int i = 0; i < nvdims; ++i ) fprintf(stderr, "%ld ", chunks[i]);
                      fprintf(stderr, "\n");
                    }
                  strcat(ncvars[ncvarid].extra, "chunks=");
                  for ( int i = nvdims-1; i >= 0; --i )
                    {
                      sprintf(buf, "%ld", (long) chunks[i]);
                      strcat(ncvars[ncvarid].extra, buf);
                      if ( i > 0 ) strcat(ncvars[ncvarid].extra, "x");
                    }
                  strcat(ncvars[ncvarid].extra, " ");
                }
            }
        }
#endif

      if ( nvdims > 0 )
        {
          if ( timedimid == dimidsp[0] )
            {
              ncvars[ncvarid].tsteptype = TSTEP_INSTANT;
              cdfSetDim(ncvars, ncvarid, 0, T_AXIS);
            }
          else
            {
              for ( ncdimid = 1; ncdimid < nvdims; ncdimid++ )
                {
                  if ( timedimid == dimidsp[ncdimid] )
                    {
                      Warning("Time must be the first dimension! Unsupported array structure, skipped variable %s!", ncvars[ncvarid].name);
                      ncvars[ncvarid].isvar = FALSE;
                    }
                }
            }
        }

      for ( iatt = 0; iatt < nvatts; iatt++ )
        {
          cdf_inq_attname(ncid, ncvarid, iatt, attname);
          cdf_inq_atttype(ncid, ncvarid, attname, &atttype);
          cdf_inq_attlen(ncid, ncvarid, attname, &attlen);

          if ( strcmp(attname, "long_name") == 0 && xtypeIsText(atttype) )
            {
              cdfGetAttText(ncid, ncvarid, attname, CDI_MAX_NAME, ncvars[ncvarid].longname);
            }
          else if ( strcmp(attname, "standard_name") == 0 && xtypeIsText(atttype) )
            {
              cdfGetAttText(ncid, ncvarid, attname, CDI_MAX_NAME, ncvars[ncvarid].stdname);
            }
          else if ( strcmp(attname, "units") == 0 && xtypeIsText(atttype) )
            {
              cdfGetAttText(ncid, ncvarid, attname, CDI_MAX_NAME, ncvars[ncvarid].units);
            }
          else if ( strcmp(attname, "calendar") == 0 )
            {
              ncvars[ncvarid].calendar = TRUE;
            }
          else if ( strcmp(attname, "param") == 0 && xtypeIsText(atttype) )
            {
              char paramstr[32];
              int pnum = 0, pcat = 255, pdis = 255;
              cdfGetAttText(ncid, ncvarid, attname, sizeof(paramstr), paramstr);
              sscanf(paramstr, "%d.%d.%d", &pnum, &pcat, &pdis);
              ncvars[ncvarid].param = cdiEncodeParam(pnum, pcat, pdis);
              cdfSetVar(ncvars, ncvarid, TRUE);
            }
          else if ( strcmp(attname, "code") == 0 && !xtypeIsText(atttype) )
            {
              cdfGetAttInt(ncid, ncvarid, attname, 1, &ncvars[ncvarid].code);
              cdfSetVar(ncvars, ncvarid, TRUE);
            }
          else if ( strcmp(attname, "table") == 0 && !xtypeIsText(atttype) )
            {
              int tablenum;
              cdfGetAttInt(ncid, ncvarid, attname, 1, &tablenum);
              if ( tablenum > 0 )
                {
                  ncvars[ncvarid].tabnum = tablenum;
                  ncvars[ncvarid].tableID = tableInq(modelID, tablenum, NULL);
                  if ( ncvars[ncvarid].tableID == CDI_UNDEFID )
                    ncvars[ncvarid].tableID = tableDef(modelID, tablenum, NULL);
                }
              cdfSetVar(ncvars, ncvarid, TRUE);
            }
          else if ( strcmp(attname, "trunc_type") == 0 && xtypeIsText(atttype) )
            {
              cdfGetAttText(ncid, ncvarid, attname, attstringlen, attstring);
              if ( memcmp(attstring, "Triangular", attlen) == 0 )
                ncvars[ncvarid].gridtype = GRID_SPECTRAL;
            }
          else if ( strcmp(attname, "grid_type") == 0 && xtypeIsText(atttype) )
            {
              cdfGetAttText(ncid, ncvarid, attname, attstringlen, attstring);
              strtolower(attstring);

              if ( strcmp(attstring, "gaussian reduced") == 0 )
                ncvars[ncvarid].gridtype = GRID_GAUSSIAN_REDUCED;
              else if ( strcmp(attstring, "gaussian") == 0 )
                ncvars[ncvarid].gridtype = GRID_GAUSSIAN;
              else if ( strncmp(attstring, "spectral", 8) == 0 )
                ncvars[ncvarid].gridtype = GRID_SPECTRAL;
              else if ( strncmp(attstring, "fourier", 7) == 0 )
                ncvars[ncvarid].gridtype = GRID_FOURIER;
              else if ( strcmp(attstring, "trajectory") == 0 )
                ncvars[ncvarid].gridtype = GRID_TRAJECTORY;
              else if ( strcmp(attstring, "generic") == 0 )
                ncvars[ncvarid].gridtype = GRID_GENERIC;
              else if ( strcmp(attstring, "cell") == 0 )
                ncvars[ncvarid].gridtype = GRID_UNSTRUCTURED;
              else if ( strcmp(attstring, "unstructured") == 0 )
                ncvars[ncvarid].gridtype = GRID_UNSTRUCTURED;
              else if ( strcmp(attstring, "curvilinear") == 0 )
                ncvars[ncvarid].gridtype = GRID_CURVILINEAR;
              else if ( strcmp(attstring, "sinusoidal") == 0 )
                ;
              else if ( strcmp(attstring, "laea") == 0 )
                ;
              else if ( strcmp(attstring, "lcc2") == 0 )
                ;
              else if ( strcmp(attstring, "linear") == 0 )
                ;
              else
                {
                  static int warn = TRUE;
                  if ( warn )
                    {
                      warn = FALSE;
                      Warning("netCDF attribute grid_type='%s' unsupported!", attstring);
                    }
                }

              cdfSetVar(ncvars, ncvarid, TRUE);
            }
          else if ( strcmp(attname, "level_type") == 0 && xtypeIsText(atttype) )
            {
              cdfGetAttText(ncid, ncvarid, attname, attstringlen, attstring);
              strtolower(attstring);

              if ( strcmp(attstring, "toa") == 0 )
                ncvars[ncvarid].zaxistype = ZAXIS_TOA;
              else if ( strcmp(attstring, "cloudbase") == 0 )
                ncvars[ncvarid].zaxistype = ZAXIS_CLOUD_BASE;
              else if ( strcmp(attstring, "cloudtop") == 0 )
                ncvars[ncvarid].zaxistype = ZAXIS_CLOUD_TOP;
              else if ( strcmp(attstring, "isotherm0") == 0 )
                ncvars[ncvarid].zaxistype = ZAXIS_ISOTHERM_ZERO;
              else if ( strcmp(attstring, "seabottom") == 0 )
                ncvars[ncvarid].zaxistype = ZAXIS_SEA_BOTTOM;
              else if ( strcmp(attstring, "lakebottom") == 0 )
                ncvars[ncvarid].zaxistype = ZAXIS_LAKE_BOTTOM;
              else if ( strcmp(attstring, "sedimentbottom") == 0 )
                ncvars[ncvarid].zaxistype = ZAXIS_SEDIMENT_BOTTOM;
              else if ( strcmp(attstring, "sedimentbottomta") == 0 )
                ncvars[ncvarid].zaxistype = ZAXIS_SEDIMENT_BOTTOM_TA;
              else if ( strcmp(attstring, "sedimentbottomtw") == 0 )
                ncvars[ncvarid].zaxistype = ZAXIS_SEDIMENT_BOTTOM_TW;
              else if ( strcmp(attstring, "mixlayer") == 0 )
                ncvars[ncvarid].zaxistype = ZAXIS_MIX_LAYER;
              else if ( strcmp(attstring, "atmosphere") == 0 )
                ncvars[ncvarid].zaxistype = ZAXIS_ATMOSPHERE;
              else
                {
                  static int warn = TRUE;
                  if ( warn )
                    {
                      warn = FALSE;
                      Warning("netCDF attribute level_type='%s' unsupported!", attstring);
                    }
                }

              cdfSetVar(ncvars, ncvarid, TRUE);
            }
          else if ( strcmp(attname, "trunc_count") == 0 && !xtypeIsText(atttype) )
            {
              cdfGetAttInt(ncid, ncvarid, attname, 1, &ncvars[ncvarid].truncation);
            }
          else if ( strcmp(attname, "truncation") == 0 && !xtypeIsText(atttype) )
            {
              cdfGetAttInt(ncid, ncvarid, attname, 1, &ncvars[ncvarid].truncation);
            }
          else if ( strcmp(attname, "number_of_grid_in_reference") == 0 && !xtypeIsText(atttype) )
            {
              cdfGetAttInt(ncid, ncvarid, attname, 1, &ncvars[ncvarid].position);
            }
          else if ( strcmp(attname, "add_offset") == 0 && !xtypeIsText(atttype) )
            {
              cdfGetAttDouble(ncid, ncvarid, attname, 1, &ncvars[ncvarid].addoffset);






            }
          else if ( strcmp(attname, "scale_factor") == 0 && !xtypeIsText(atttype) )
            {
              cdfGetAttDouble(ncid, ncvarid, attname, 1, &ncvars[ncvarid].scalefactor);






            }
          else if ( strcmp(attname, "climatology") == 0 && xtypeIsText(atttype) )
            {
              cdfGetAttText(ncid, ncvarid, attname, attstringlen, attstring);
              int ncboundsid;
              int status = nc_inq_varid(ncid, attstring, &ncboundsid);
              if ( status == NC_NOERR )
                {
                  ncvars[ncvarid].climatology = TRUE;
                  ncvars[ncvarid].bounds = ncboundsid;
                  cdfSetVar(ncvars, ncvars[ncvarid].bounds, FALSE);
                  cdfSetVar(ncvars, ncvarid, FALSE);
                }
              else
                Warning("%s - %s", nc_strerror(status), attstring);
            }
          else if ( xtypeIsText(atttype) && strcmp(attname, "bounds") == 0 )
            {
              cdfGetAttText(ncid, ncvarid, attname, attstringlen, attstring);
              int ncboundsid;
              int status = nc_inq_varid(ncid, attstring, &ncboundsid);
              if ( status == NC_NOERR )
                {
                  ncvars[ncvarid].bounds = ncboundsid;
                  cdfSetVar(ncvars, ncvars[ncvarid].bounds, FALSE);
                  cdfSetVar(ncvars, ncvarid, FALSE);
                }
              else
                Warning("%s - %s", nc_strerror(status), attstring);
            }
          else if ( xtypeIsText(atttype) && strcmp(attname, "formula_terms") == 0 )
            {
              ncvars[ncvarid].lformulaterms = TRUE;
            }
          else if ( xtypeIsText(atttype) && strcmp(attname, "formula") == 0 )
            {
              ncvars[ncvarid].lformula = TRUE;
            }
          else if ( strcmp(attname, "cell_measures") == 0 && xtypeIsText(atttype) )
            {
              char *cell_measures = NULL, *cell_var = NULL;

              cdfGetAttText(ncid, ncvarid, attname, attstringlen, attstring);
              char *pstring = attstring;

              while ( isspace((int) *pstring) ) pstring++;
              cell_measures = pstring;
              while ( isalnum((int) *pstring) ) pstring++;
              *pstring++ = 0;
              while ( isspace((int) *pstring) ) pstring++;
              cell_var = pstring;
              while ( ! isspace((int) *pstring) && *pstring != 0 ) pstring++;
              *pstring++ = 0;




              if ( memcmp(cell_measures, "area", 4) == 0 )
                {
                  int nc_cell_id;
                  int status = nc_inq_varid(ncid, cell_var, &nc_cell_id);
                  if ( status == NC_NOERR )
                    {
                      ncvars[ncvarid].cellarea = nc_cell_id;

                      cdfSetVar(ncvars, nc_cell_id, FALSE);
                    }
                  else
                    Warning("%s - %s", nc_strerror(status), cell_var);
                }
              else
                {
                  Warning("%s has an unexpected contents: %s", attname, cell_measures);
                }
              cdfSetVar(ncvars, ncvarid, TRUE);
            }
          else if ( (strcmp(attname, "associate") == 0 || strcmp(attname, "coordinates") == 0) && xtypeIsText(atttype) )
            {
              int status;
              char *varname = NULL;
              int lstop = FALSE;
              int dimvarid;

              cdfGetAttText(ncid, ncvarid, attname, attstringlen, attstring);
              char *pstring = attstring;

              for ( int i = 0; i < MAX_COORDVARS; i++ )
                {
                  while ( isspace((int) *pstring) ) pstring++;
                  if ( *pstring == 0 ) break;
                  varname = pstring;
                  while ( !isspace((int) *pstring) && *pstring != 0 ) pstring++;
                  if ( *pstring == 0 ) lstop = TRUE;
                  *pstring++ = 0;

                  status = nc_inq_varid(ncid, varname, &dimvarid);
                  if ( status == NC_NOERR )
                    {
                      cdfSetVar(ncvars, dimvarid, FALSE);
                      if ( cdiIgnoreAttCoordinates == FALSE )
                        {
                          ncvars[ncvarid].coordvarids[i] = dimvarid;
                          ncvars[ncvarid].ncoordvars++;
                        }
                    }
                  else
                    {
                      int k;
                      for ( k = 0; k < nchecked_vars; ++k )
                        if ( strcmp(checked_vars[k], varname) == 0 ) break;

                      if ( k == nchecked_vars )
                        {
                          if ( nchecked_vars < max_check_vars ) checked_vars[nchecked_vars++] = strdup(varname);
                          Warning("%s - %s", nc_strerror(status), varname);
                        }
                    }

                  if ( lstop ) break;
                }

              cdfSetVar(ncvars, ncvarid, TRUE);
            }
          else if ( (strcmp(attname, "auxiliary_variable") == 0) && xtypeIsText(atttype) )
            {
              int status;
              char *varname = NULL;
              int lstop = FALSE;
              int dimvarid;

              cdfGetAttText(ncid, ncvarid, attname, attstringlen, attstring);
              char *pstring = attstring;

              for ( int i = 0; i < MAX_AUXVARS; i++ )
                {
                  while ( isspace((int) *pstring) ) pstring++;
                  if ( *pstring == 0 ) break;
                  varname = pstring;
                  while ( !isspace((int) *pstring) && *pstring != 0 ) pstring++;
                  if ( *pstring == 0 ) lstop = TRUE;
                  *pstring++ = 0;

                  status = nc_inq_varid(ncid, varname, &dimvarid);
                  if ( status == NC_NOERR )
                    {
                      cdfSetVar(ncvars, dimvarid, FALSE);

                        {
                          ncvars[ncvarid].auxvarids[i] = dimvarid;
                          ncvars[ncvarid].nauxvars++;
                        }
                    }
                  else
                    Warning("%s - %s", nc_strerror(status), varname);

                  if ( lstop ) break;
                }

              cdfSetVar(ncvars, ncvarid, TRUE);
            }
          else if ( strcmp(attname, "grid_mapping") == 0 && xtypeIsText(atttype) )
            {
              cdfGetAttText(ncid, ncvarid, attname, attstringlen, attstring);
              int nc_gmap_id;
              int status = nc_inq_varid(ncid, attstring, &nc_gmap_id);
              if ( status == NC_NOERR )
                {
                  ncvars[ncvarid].gmapid = nc_gmap_id;
                  cdfSetVar(ncvars, ncvars[ncvarid].gmapid, FALSE);
                }
              else
                Warning("%s - %s", nc_strerror(status), attstring);

              cdfSetVar(ncvars, ncvarid, TRUE);
            }
          else if ( strcmp(attname, "positive") == 0 && xtypeIsText(atttype) )
            {
              cdfGetAttText(ncid, ncvarid, attname, attstringlen, attstring);
              strtolower(attstring);

              if ( memcmp(attstring, "down", 4) == 0 ) ncvars[ncvarid].positive = POSITIVE_DOWN;
              else if ( memcmp(attstring, "up", 2) == 0 ) ncvars[ncvarid].positive = POSITIVE_UP;

              if ( ncvars[ncvarid].ndims == 1 )
                {
                  cdfSetVar(ncvars, ncvarid, FALSE);
                  cdfSetDim(ncvars, ncvarid, 0, Z_AXIS);
                  ncdims[ncvars[ncvarid].dimids[0]].dimtype = Z_AXIS;
                }
            }
          else if ( strcmp(attname, "_FillValue") == 0 && !xtypeIsText(atttype) )
            {
              cdfGetAttDouble(ncid, ncvarid, attname, 1, &ncvars[ncvarid].fillval);
              ncvars[ncvarid].deffillval = TRUE;

            }
          else if ( strcmp(attname, "missing_value") == 0 && !xtypeIsText(atttype) )
            {
              cdfGetAttDouble(ncid, ncvarid, attname, 1, &ncvars[ncvarid].missval);
              ncvars[ncvarid].defmissval = TRUE;

            }
          else if ( strcmp(attname, "valid_range") == 0 && attlen == 2 )
            {
              if ( ncvars[ncvarid].lvalidrange == FALSE )
                {
                  extern int cdiIgnoreValidRange;
                  int lignore = FALSE;
                  if ( xtypeIsFloat(atttype) != xtypeIsFloat(xtype) ) lignore = TRUE;
                  if ( cdiIgnoreValidRange == FALSE && lignore == FALSE )
                    {
                      cdfGetAttDouble(ncid, ncvarid, attname, 2, ncvars[ncvarid].validrange);
                      ncvars[ncvarid].lvalidrange = TRUE;
                      if ( ((int)ncvars[ncvarid].validrange[0]) == 0 && ((int)ncvars[ncvarid].validrange[1]) == 255 )
                        ncvars[ncvarid].lunsigned = TRUE;

                    }
                  else if ( lignore )
                    {
                      Warning("Inconsistent data type for attribute %s:valid_range, ignored!", name);
                    }
                }
            }
          else if ( strcmp(attname, "valid_min") == 0 && attlen == 1 )
            {
              if ( ncvars[ncvarid].lvalidrange == FALSE )
                {
                  extern int cdiIgnoreValidRange;
                  int lignore = FALSE;
                  if ( xtypeIsFloat(atttype) != xtypeIsFloat(xtype) ) lignore = TRUE;
                  if ( cdiIgnoreValidRange == FALSE && lignore == FALSE )
                    {
                      cdfGetAttDouble(ncid, ncvarid, attname, 1, &(ncvars[ncvarid].validrange)[0]);
                      ncvars[ncvarid].lvalidrange = TRUE;
                    }
                  else if ( lignore )
                    {
                      Warning("Inconsistent data type for attribute %s:valid_min, ignored!", name);
                    }
                }
            }
          else if ( strcmp(attname, "valid_max") == 0 && attlen == 1 )
            {
              if ( ncvars[ncvarid].lvalidrange == FALSE )
                {
                  extern int cdiIgnoreValidRange;
                  int lignore = FALSE;
                  if ( xtypeIsFloat(atttype) != xtypeIsFloat(xtype) ) lignore = TRUE;
                  if ( cdiIgnoreValidRange == FALSE && lignore == FALSE )
                    {
                      cdfGetAttDouble(ncid, ncvarid, attname, 1, &(ncvars[ncvarid].validrange)[1]);
                      ncvars[ncvarid].lvalidrange = TRUE;
                    }
                  else if ( lignore )
                    {
                      Warning("Inconsistent data type for attribute %s:valid_max, ignored!", name);
                    }
                }
            }
          else if ( strcmp(attname, "_Unsigned") == 0 && xtypeIsText(atttype) )
            {
              cdfGetAttText(ncid, ncvarid, attname, attstringlen, attstring);
              strtolower(attstring);

              if ( memcmp(attstring, "true", 4) == 0 )
                {
                  ncvars[ncvarid].lunsigned = TRUE;





                }

            }
          else if ( strcmp(attname, "cdi") == 0 && xtypeIsText(atttype) )
            {
              cdfGetAttText(ncid, ncvarid, attname, attstringlen, attstring);
              strtolower(attstring);

              if ( memcmp(attstring, "ignore", 6) == 0 )
                {
                  ncvars[ncvarid].ignore = TRUE;
                  cdfSetVar(ncvars, ncvarid, FALSE);
                }
            }
          else if ( strcmp(attname, "axis") == 0 && xtypeIsText(atttype) )
            {
              cdfGetAttText(ncid, ncvarid, attname, attstringlen, attstring);
              attlen = strlen(attstring);

              if ( (int) attlen > nvdims && nvdims > 0 && attlen > 1 )
                {
                    Warning("Unexpected axis attribute length for %s, ignored!", name);
                }
              else if ( nvdims == 0 && attlen == 1 )
                {
                  if ( attstring[0] == 'z' || attstring[0] == 'Z' )
                    {
                      cdfSetVar(ncvars, ncvarid, FALSE);
                      ncvars[ncvarid].islev = TRUE;
                    }
                }
              else
                {
                  strtolower(attstring);
                  int i;
                  for ( i = 0; i < (int)attlen; ++i )
                    {
                      if ( attstring[i] != '-' && attstring[i] != 't' && attstring[i] != 'z' &&
                           attstring[i] != 'y' && attstring[i] != 'x' )
                        {
                          Warning("Unexpected character in axis attribute for %s, ignored!", name);
                          break;
                        }
                    }

                  if ( i == (int) attlen && (int) attlen == nvdims )
                    {
                      while ( attlen-- )
                        {
                          if ( (int) attstring[attlen] == 't' )
                            {
                              if ( attlen != 0 ) Warning("axis attribute 't' not on first position");
                              cdfSetDim(ncvars, ncvarid, (int)attlen, T_AXIS);
                            }
                          else if ( (int) attstring[attlen] == 'z' )
                            {
                              ncvars[ncvarid].zdim = dimidsp[attlen];
                              cdfSetDim(ncvars, ncvarid, (int)attlen, Z_AXIS);

                              if ( ncvars[ncvarid].ndims == 1 )
                                {
                                  cdfSetVar(ncvars, ncvarid, FALSE);
                                  ncdims[ncvars[ncvarid].dimids[0]].dimtype = Z_AXIS;
                                }
                            }
                          else if ( (int) attstring[attlen] == 'y' )
                            {
                              ncvars[ncvarid].ydim = dimidsp[attlen];
                              cdfSetDim(ncvars, ncvarid, (int)attlen, Y_AXIS);

                              if ( ncvars[ncvarid].ndims == 1 )
                                {
                                  cdfSetVar(ncvars, ncvarid, FALSE);
                                  ncdims[ncvars[ncvarid].dimids[0]].dimtype = Y_AXIS;
                                }
                            }
                          else if ( (int) attstring[attlen] == 'x' )
                            {
                              ncvars[ncvarid].xdim = dimidsp[attlen];
                              cdfSetDim(ncvars, ncvarid, (int)attlen, X_AXIS);

                              if ( ncvars[ncvarid].ndims == 1 )
                                {
                                  cdfSetVar(ncvars, ncvarid, FALSE);
                                  ncdims[ncvars[ncvarid].dimids[0]].dimtype = X_AXIS;
                                }
                            }
                        }
                    }
                }
            }
          else if ( ( strcmp(attname, "realization") == 0 ) ||
                    ( strcmp(attname, "ensemble_members") == 0 ) ||
                    ( strcmp(attname, "forecast_init_type") == 0 ) )
            {
              int temp;

              if( ncvars[ncvarid].ensdata == NULL )
                ncvars[ncvarid].ensdata = (ensinfo_t *) Malloc( sizeof( ensinfo_t ) );

              cdfGetAttInt(ncid, ncvarid, attname, 1, &temp);

              if( strcmp(attname, "realization") == 0 )
                ncvars[ncvarid].ensdata->ens_index = temp;
              else if( strcmp(attname, "ensemble_members") == 0 )
                ncvars[ncvarid].ensdata->ens_count = temp;
              else if( strcmp(attname, "forecast_init_type") == 0 )
                ncvars[ncvarid].ensdata->forecast_init_type = temp;

              cdfSetVar(ncvars, ncvarid, TRUE);
            }
          else
            {
              if ( ncvars[ncvarid].natts == 0 )
                ncvars[ncvarid].atts
                  = (int *) Malloc((size_t)nvatts * sizeof (int));

              ncvars[ncvarid].atts[ncvars[ncvarid].natts++] = iatt;
            }
        }
    }

  for ( int i = 0; i < max_check_vars; ++i ) if ( checked_vars[i] ) Free(checked_vars[i]);
}

static
void setDimType(int nvars, ncvar_t *ncvars, ncdim_t *ncdims)
{
  int ndims;
  int ncvarid, ncdimid;
  int i;

  for ( ncvarid = 0; ncvarid < nvars; ncvarid++ )
    {
      if ( ncvars[ncvarid].isvar == TRUE )
        {
          int lxdim = 0, lydim = 0, lzdim = 0 ;
          ndims = ncvars[ncvarid].ndims;
          for ( i = 0; i < ndims; i++ )
            {
              ncdimid = ncvars[ncvarid].dimids[i];
              if ( ncdims[ncdimid].dimtype == X_AXIS ) cdfSetDim(ncvars, ncvarid, i, X_AXIS);
              else if ( ncdims[ncdimid].dimtype == Y_AXIS ) cdfSetDim(ncvars, ncvarid, i, Y_AXIS);
              else if ( ncdims[ncdimid].dimtype == Z_AXIS ) cdfSetDim(ncvars, ncvarid, i, Z_AXIS);
              else if ( ncdims[ncdimid].dimtype == T_AXIS ) cdfSetDim(ncvars, ncvarid, i, T_AXIS);
            }

          if ( CDI_Debug )
            {
              Message("var %d %s", ncvarid, ncvars[ncvarid].name);
              for ( i = 0; i < ndims; i++ )
                printf("  dim%d type=%d  ", i, ncvars[ncvarid].dimtype[i]);
              printf("\n");
            }

          for ( i = 0; i < ndims; i++ )
            {
              if ( ncvars[ncvarid].dimtype[i] == X_AXIS ) lxdim = TRUE;
              else if ( ncvars[ncvarid].dimtype[i] == Y_AXIS ) lydim = TRUE;
              else if ( ncvars[ncvarid].dimtype[i] == Z_AXIS ) lzdim = TRUE;

            }

          if ( lxdim == FALSE && ncvars[ncvarid].xvarid != UNDEFID )
            {
              if ( ncvars[ncvars[ncvarid].xvarid].ndims == 0 ) lxdim = TRUE;
            }

          if ( lydim == FALSE && ncvars[ncvarid].yvarid != UNDEFID )
            {
              if ( ncvars[ncvars[ncvarid].yvarid].ndims == 0 ) lydim = TRUE;
            }


            for ( i = ndims-1; i >= 0; i-- )
              {
                if ( ncvars[ncvarid].dimtype[i] == -1 )
                  {
                    if ( lxdim == FALSE )
                      {
                        cdfSetDim(ncvars, ncvarid, i, X_AXIS);
                        lxdim = TRUE;
                      }
                    else if ( lydim == FALSE && ncvars[ncvarid].gridtype != GRID_UNSTRUCTURED )
                      {
                        cdfSetDim(ncvars, ncvarid, i, Y_AXIS);
                        lydim = TRUE;
                      }
                    else if ( lzdim == FALSE )
                      {
                        cdfSetDim(ncvars, ncvarid, i, Z_AXIS);
                        lzdim = TRUE;
                      }
                  }
              }
        }
    }
}


static
void verify_coordinate_vars_1(int ncid, int ndims, ncdim_t *ncdims, ncvar_t *ncvars, int timedimid)
{
  int ncdimid, ncvarid;

  for ( ncdimid = 0; ncdimid < ndims; ncdimid++ )
    {
      ncvarid = ncdims[ncdimid].ncvarid;
      if ( ncvarid != -1 )
        {
          if ( ncvars[ncvarid].dimids[0] == timedimid )
            {
              ncvars[ncvarid].istime = TRUE;
              ncdims[ncdimid].dimtype = T_AXIS;
              continue;
            }

          if ( isHybridSigmaPressureCoordinate(ncid, ncvarid, ncvars, ncdims) ) continue;

          if ( ncvars[ncvarid].units[0] != 0 )
            {
              if ( isLonAxis(ncvars[ncvarid].units, ncvars[ncvarid].stdname) )
                {
                  ncvars[ncvarid].islon = TRUE;
                  cdfSetVar(ncvars, ncvarid, FALSE);
                  cdfSetDim(ncvars, ncvarid, 0, X_AXIS);
                  ncdims[ncdimid].dimtype = X_AXIS;
                }
              else if ( isLatAxis(ncvars[ncvarid].units, ncvars[ncvarid].stdname) )
                {
                  ncvars[ncvarid].islat = TRUE;
                  cdfSetVar(ncvars, ncvarid, FALSE);
                  cdfSetDim(ncvars, ncvarid, 0, Y_AXIS);
                  ncdims[ncdimid].dimtype = Y_AXIS;
                }
              else if ( unitsIsPressure(ncvars[ncvarid].units) )
                {
                  ncvars[ncvarid].zaxistype = ZAXIS_PRESSURE;
                }
              else if ( strcmp(ncvars[ncvarid].units, "level") == 0 || strcmp(ncvars[ncvarid].units, "1") == 0 )
                {
                  if ( strcmp(ncvars[ncvarid].longname, "hybrid level at layer midpoints") == 0 )
                    ncvars[ncvarid].zaxistype = ZAXIS_HYBRID;
                  else if ( strncmp(ncvars[ncvarid].longname, "hybrid level at midpoints", 25) == 0 )
                    ncvars[ncvarid].zaxistype = ZAXIS_HYBRID;
                  else if ( strcmp(ncvars[ncvarid].longname, "hybrid level at layer interfaces") == 0 )
                    ncvars[ncvarid].zaxistype = ZAXIS_HYBRID_HALF;
                  else if ( strncmp(ncvars[ncvarid].longname, "hybrid level at interfaces", 26) == 0 )
                    ncvars[ncvarid].zaxistype = ZAXIS_HYBRID_HALF;
                  else if ( strcmp(ncvars[ncvarid].units, "level") == 0 )
                    ncvars[ncvarid].zaxistype = ZAXIS_GENERIC;
                }
              else if ( isDBLAxis(ncvars[ncvarid].longname) )
                {
                  ncvars[ncvarid].zaxistype = ZAXIS_DEPTH_BELOW_LAND;
                }
              else if ( unitsIsMeter(ncvars[ncvarid].units) )
                {
                  if ( isDepthAxis(ncvars[ncvarid].stdname, ncvars[ncvarid].longname) )
                    ncvars[ncvarid].zaxistype = ZAXIS_DEPTH_BELOW_SEA;
                  else if ( isHeightAxis(ncvars[ncvarid].stdname, ncvars[ncvarid].longname) )
                    ncvars[ncvarid].zaxistype = ZAXIS_HEIGHT;
                }
            }
          else
            {
              if ( (strcmp(ncvars[ncvarid].longname, "generalized_height") == 0 ||
                    strcmp(ncvars[ncvarid].longname, "generalized height") == 0) &&
                   strcmp(ncvars[ncvarid].stdname, "height") == 0 )
                  ncvars[ncvarid].zaxistype = ZAXIS_REFERENCE;
            }

          if ( ncvars[ncvarid].islon == FALSE && ncvars[ncvarid].longname[0] != 0 &&
               ncvars[ncvarid].islat == FALSE && ncvars[ncvarid].longname[1] != 0 )
            {
              if ( memcmp(ncvars[ncvarid].longname+1, "ongitude", 8) == 0 )
                {
                  ncvars[ncvarid].islon = TRUE;
                  cdfSetVar(ncvars, ncvarid, FALSE);
                  cdfSetDim(ncvars, ncvarid, 0, X_AXIS);
                  ncdims[ncdimid].dimtype = X_AXIS;
                  continue;
                }
              else if ( memcmp(ncvars[ncvarid].longname+1, "atitude", 7) == 0 )
                {
                  ncvars[ncvarid].islat = TRUE;
                  cdfSetVar(ncvars, ncvarid, FALSE);
                  cdfSetDim(ncvars, ncvarid, 0, Y_AXIS);
                  ncdims[ncdimid].dimtype = Y_AXIS;
                  continue;
                }
            }

          if ( ncvars[ncvarid].zaxistype != UNDEFID )
            {
              ncvars[ncvarid].islev = TRUE;
              cdfSetVar(ncvars, ncvarid, FALSE);
              cdfSetDim(ncvars, ncvarid, 0, Z_AXIS);
              ncdims[ncdimid].dimtype = Z_AXIS;
            }
        }
    }
}


static
void verify_coordinate_vars_2(int nvars, ncvar_t *ncvars)
{
  int ncvarid;

  for ( ncvarid = 0; ncvarid < nvars; ncvarid++ )
    {
      if ( ncvars[ncvarid].isvar == 0 )
        {
          if ( ncvars[ncvarid].units[0] != 0 )
            {
              if ( isLonAxis(ncvars[ncvarid].units, ncvars[ncvarid].stdname) )
                {
                  ncvars[ncvarid].islon = TRUE;
                  continue;
                }
              else if ( isLatAxis(ncvars[ncvarid].units, ncvars[ncvarid].stdname) )
                {
                  ncvars[ncvarid].islat = TRUE;
                  continue;
                }
              else if ( unitsIsPressure(ncvars[ncvarid].units) )
                {
                  ncvars[ncvarid].zaxistype = ZAXIS_PRESSURE;
                  continue;
                }
              else if ( strcmp(ncvars[ncvarid].units, "level") == 0 || strcmp(ncvars[ncvarid].units, "1") == 0 )
                {
                  if ( strcmp(ncvars[ncvarid].longname, "hybrid level at layer midpoints") == 0 )
                    ncvars[ncvarid].zaxistype = ZAXIS_HYBRID;
                  else if ( strncmp(ncvars[ncvarid].longname, "hybrid level at midpoints", 25) == 0 )
                    ncvars[ncvarid].zaxistype = ZAXIS_HYBRID;
                  else if ( strcmp(ncvars[ncvarid].longname, "hybrid level at layer interfaces") == 0 )
                    ncvars[ncvarid].zaxistype = ZAXIS_HYBRID_HALF;
                  else if ( strncmp(ncvars[ncvarid].longname, "hybrid level at interfaces", 26) == 0 )
                    ncvars[ncvarid].zaxistype = ZAXIS_HYBRID_HALF;
                  else if ( strcmp(ncvars[ncvarid].units, "level") == 0 )
                    ncvars[ncvarid].zaxistype = ZAXIS_GENERIC;
                  continue;
                }
              else if ( isDBLAxis(ncvars[ncvarid].longname) )
                {
                  ncvars[ncvarid].zaxistype = ZAXIS_DEPTH_BELOW_LAND;
                  continue;
                }
              else if ( unitsIsMeter(ncvars[ncvarid].units) )
                {
                  if ( isDepthAxis(ncvars[ncvarid].stdname, ncvars[ncvarid].longname) )
                    ncvars[ncvarid].zaxistype = ZAXIS_DEPTH_BELOW_SEA;
                  else if ( isHeightAxis(ncvars[ncvarid].stdname, ncvars[ncvarid].longname) )
                    ncvars[ncvarid].zaxistype = ZAXIS_HEIGHT;
                  continue;
                }
            }


          if ( ncvars[ncvarid].islon == FALSE && ncvars[ncvarid].longname[0] != 0 &&
               ncvars[ncvarid].islat == FALSE && ncvars[ncvarid].longname[1] != 0 )
            {
              if ( memcmp(ncvars[ncvarid].longname+1, "ongitude", 8) == 0 )
                {
                  ncvars[ncvarid].islon = TRUE;
                  continue;
                }
              else if ( memcmp(ncvars[ncvarid].longname+1, "atitude", 7) == 0 )
                {
                  ncvars[ncvarid].islat = TRUE;
                  continue;
                }
            }
        }
    }
}

static
void grid_set_chunktype(grid_t *grid, ncvar_t *ncvar)
{
  if ( ncvar->chunked )
    {
      int ndims = ncvar->ndims;

      if ( grid->type == GRID_UNSTRUCTURED )
        {
          if ( ncvar->chunks[ndims-1] == grid->size )
            ncvar->chunktype = CHUNK_GRID;
          else
            ncvar->chunktype = CHUNK_AUTO;
        }
      else
        {
          if ( grid->xsize > 1 && grid->ysize > 1 && ndims > 1 &&
               grid->xsize == ncvar->chunks[ndims-1] &&
               grid->ysize == ncvar->chunks[ndims-2] )
            ncvar->chunktype = CHUNK_GRID;
          else if ( grid->xsize > 1 && grid->xsize == ncvar->chunks[ndims-1] )
            ncvar->chunktype = CHUNK_LINES;
          else
            ncvar->chunktype = CHUNK_AUTO;
        }
    }
}


static
void define_all_grids(stream_t *streamptr, int vlistID, ncdim_t *ncdims, int nvars, ncvar_t *ncvars, int timedimid, unsigned char *uuidOfHGrid, char *gridfile, int number_of_grid_used)
{
  int ncvarid, ncvarid2;
  int nbdims;
  int i;
  int nvatts;
  int skipvar;
  size_t nvertex;
  grid_t grid;
  grid_t proj;
  int gridindex;
  char name[CDI_MAX_NAME];
  int iatt;
  int ltwarn = TRUE;
  size_t attlen;
  char attname[CDI_MAX_NAME];
  double datt;

  for ( ncvarid = 0; ncvarid < nvars; ++ncvarid )
    {
      if ( ncvars[ncvarid].isvar && ncvars[ncvarid].gridID == UNDEFID )
        {
          int xdimids[2] = {-1,-1}, ydimids[2] = {-1,-1};
          int xdimid = -1, ydimid = -1;
          int xvarid = -1, yvarid = -1;
          int islon = 0, islat = 0;
          int nxdims = 0, nydims = 0;
          size_t size = 0, np = 0;
          size_t xsize = 0, ysize = 0;
          double xinc = 0, yinc = 0;

          int ndims = ncvars[ncvarid].ndims;
          for ( i = 0; i < ndims; i++ )
            {
              if ( ncvars[ncvarid].dimtype[i] == X_AXIS && nxdims < 2 )
                {
                  xdimids[nxdims] = ncvars[ncvarid].dimids[i];
                  nxdims++;
                }
              else if ( ncvars[ncvarid].dimtype[i] == Y_AXIS && nydims < 2 )
                {
                  ydimids[nydims] = ncvars[ncvarid].dimids[i];
                  nydims++;
                }
            }

          if ( nxdims == 2 )
            {
              xdimid = xdimids[1];
              ydimid = xdimids[0];
            }
          else if ( nydims == 2 )
            {
              xdimid = ydimids[1];
              ydimid = ydimids[0];
            }
          else
            {
              xdimid = xdimids[0];
              ydimid = ydimids[0];
            }

          if ( ncvars[ncvarid].xvarid != UNDEFID )
            xvarid = ncvars[ncvarid].xvarid;
          else if ( xdimid != UNDEFID )
            xvarid = ncdims[xdimid].ncvarid;

          if ( ncvars[ncvarid].yvarid != UNDEFID )
            yvarid = ncvars[ncvarid].yvarid;
          else if ( ydimid != UNDEFID )
            yvarid = ncdims[ydimid].ncvarid;
          if ( xdimid != UNDEFID ) xsize = ncdims[xdimid].len;
          if ( ydimid != UNDEFID ) ysize = ncdims[ydimid].len;

          if ( ydimid == UNDEFID && yvarid != UNDEFID )
            {
              if ( ncvars[yvarid].ndims == 1 )
                {
                  ydimid = ncvars[yvarid].dimids[0];
                  ysize = ncdims[ydimid].len;
                }
            }

          if ( ncvars[ncvarid].gridtype == UNDEFID || ncvars[ncvarid].gridtype == GRID_GENERIC )
            if ( xdimid != UNDEFID && xdimid == ydimid && nydims == 0 ) ncvars[ncvarid].gridtype = GRID_UNSTRUCTURED;

          grid_init(&grid);
          grid_init(&proj);

          grid.prec = DATATYPE_FLT64;
          grid.trunc = ncvars[ncvarid].truncation;

          if ( ncvars[ncvarid].gridtype == GRID_TRAJECTORY )
            {
              if ( ncvars[ncvarid].xvarid == UNDEFID )
                Error("Longitude coordinate undefined for %s!", name);
              if ( ncvars[ncvarid].yvarid == UNDEFID )
                Error("Latitude coordinate undefined for %s!", name);
            }
          else
            {
              size_t start[3], count[3];
              int ltgrid = FALSE;

              if ( xvarid != UNDEFID && yvarid != UNDEFID )
                {
                  if ( ncvars[xvarid].ndims != ncvars[yvarid].ndims )
                    {
                      Warning("Inconsistent grid structure for variable %s!", ncvars[ncvarid].name);
                      ncvars[ncvarid].xvarid = UNDEFID;
                      ncvars[ncvarid].yvarid = UNDEFID;
                      xvarid = UNDEFID;
                      yvarid = UNDEFID;
                    }

                  if ( ncvars[xvarid].ndims > 2 || ncvars[yvarid].ndims > 2 )
                    {
                      if ( ncvars[xvarid].ndims == 3 && ncvars[xvarid].dimids[0] == timedimid &&
                           ncvars[yvarid].ndims == 3 && ncvars[yvarid].dimids[0] == timedimid )
                        {
                          if ( ltwarn )
                            Warning("Time varying grids unsupported, using grid at time step 1!");
                          ltgrid = TRUE;
                          ltwarn = FALSE;
                          start[0] = start[1] = start[2] = 0;
                          count[0] = 1; count[1] = ysize; count[2] = xsize;
                        }
                      else
                        {
                          Warning("Unsupported grid structure for variable %s (grid dims > 2)!", ncvars[ncvarid].name);
                          ncvars[ncvarid].xvarid = UNDEFID;
                          ncvars[ncvarid].yvarid = UNDEFID;
                          xvarid = UNDEFID;
                          yvarid = UNDEFID;
                        }
                    }
                }

              if ( xvarid != UNDEFID )
                {
                  if ( ncvars[xvarid].ndims > 3 || (ncvars[xvarid].ndims == 3 && ltgrid == FALSE) )
                    {
                      Warning("Coordinate variable %s has to many dimensions (%d), skipped!", ncvars[xvarid].name, ncvars[xvarid].ndims);

                      xvarid = UNDEFID;
                    }
                }

              if ( yvarid != UNDEFID )
                {
                  if ( ncvars[yvarid].ndims > 3 || (ncvars[yvarid].ndims == 3 && ltgrid == FALSE) )
                    {
                      Warning("Coordinate variable %s has to many dimensions (%d), skipped!", ncvars[yvarid].name, ncvars[yvarid].ndims);

                      yvarid = UNDEFID;
                    }
                }

              if ( xvarid != UNDEFID )
                {
                  skipvar = TRUE;
                  islon = ncvars[xvarid].islon;
                  ndims = ncvars[xvarid].ndims;
                  if ( ndims == 2 || ndims == 3 )
                    {
                      ncvars[ncvarid].gridtype = GRID_CURVILINEAR;
                      size = xsize*ysize;

                      {
                        int dimid = ncvars[xvarid].dimids[ndims-2];
                        size_t dimsize1 = ncdims[dimid].len;
                        dimid = ncvars[xvarid].dimids[ndims-1];
                        size_t dimsize2 = ncdims[dimid].len;
                        if ( dimsize1*dimsize2 == size ) skipvar = FALSE;
                      }
                    }
                  else if ( ndims == 1 )
                    {
                      size = xsize;

                      {
                        int dimid = ncvars[xvarid].dimids[0];
                        size_t dimsize = ncdims[dimid].len;
                        if ( dimsize == size ) skipvar = FALSE;
                      }
                    }
                  else if ( ndims == 0 && xsize == 0 )
                    {
                      xsize = 1;
                      size = xsize;
                      skipvar = FALSE;
                    }

                  if ( skipvar )
                    {
                      Warning("Unsupported array structure, skipped variable %s!", ncvars[ncvarid].name);
                      ncvars[ncvarid].isvar = -1;
                      continue;
                    }

                  if ( ncvars[xvarid].xtype == NC_FLOAT ) grid.prec = DATATYPE_FLT32;
                  grid.xvals = (double *) Malloc(size*sizeof(double));

                  if ( ltgrid )
                    cdf_get_vara_double(ncvars[xvarid].ncid, xvarid, start, count, grid.xvals);
                  else
                    cdf_get_var_double(ncvars[xvarid].ncid, xvarid, grid.xvals);

                  scale_add(size, grid.xvals, ncvars[xvarid].addoffset, ncvars[xvarid].scalefactor);

                  strcpy(grid.xname, ncvars[xvarid].name);
                  strcpy(grid.xlongname, ncvars[xvarid].longname);
                  strcpy(grid.xunits, ncvars[xvarid].units);






                  if ( islon && xsize > 1 )
                    {
                      xinc = fabs(grid.xvals[0] - grid.xvals[1]);
                      for ( i = 2; i < (int) xsize; i++ )
                        if ( (fabs(grid.xvals[i-1] - grid.xvals[i]) - xinc) > (xinc/1000) ) break;

                      if ( i < (int) xsize ) xinc = 0;
                    }
                }

              if ( yvarid != UNDEFID )
                {
                  skipvar = TRUE;
                  islat = ncvars[yvarid].islat;
                  ndims = ncvars[yvarid].ndims;
                  if ( ndims == 2 || ndims == 3 )
                    {
                      ncvars[ncvarid].gridtype = GRID_CURVILINEAR;
                      size = xsize*ysize;

                      {
                        int dimid;
                        size_t dimsize1, dimsize2;
                        dimid = ncvars[yvarid].dimids[ndims-2];
                        dimsize1 = ncdims[dimid].len;
                        dimid = ncvars[yvarid].dimids[ndims-1];
                        dimsize2 = ncdims[dimid].len;
                        if ( dimsize1*dimsize2 == size ) skipvar = FALSE;
                      }
                    }
                  else if ( ndims == 1 )
                    {
                      if ( (int) ysize == 0 ) size = xsize;
                      else size = ysize;


                      {
                        int dimid;
                        size_t dimsize;
                        dimid = ncvars[yvarid].dimids[0];
                        dimsize = ncdims[dimid].len;
                        if ( dimsize == size ) skipvar = FALSE;
                      }
                    }
                  else if ( ndims == 0 && ysize == 0 )
                    {
                      ysize = 1;
                      size = ysize;
                      skipvar = FALSE;
                    }

                  if ( skipvar )
                    {
                      Warning("Unsupported array structure, skipped variable %s!", ncvars[ncvarid].name);
                      ncvars[ncvarid].isvar = -1;
                      continue;
                    }

                  if ( ncvars[yvarid].xtype == NC_FLOAT ) grid.prec = DATATYPE_FLT32;
                  grid.yvals = (double *) Malloc(size*sizeof(double));

                  if ( ltgrid )
                    cdf_get_vara_double(ncvars[yvarid].ncid, yvarid, start, count, grid.yvals);
                  else
                    cdf_get_var_double(ncvars[yvarid].ncid, yvarid, grid.yvals);

                  scale_add(size, grid.yvals, ncvars[yvarid].addoffset, ncvars[yvarid].scalefactor);

                  strcpy(grid.yname, ncvars[yvarid].name);
                  strcpy(grid.ylongname, ncvars[yvarid].longname);
                  strcpy(grid.yunits, ncvars[yvarid].units);






                  if ( islon && (int) ysize > 1 )
                    {
                      yinc = fabs(grid.yvals[0] - grid.yvals[1]);
                      for ( i = 2; i < (int) ysize; i++ )
                        if ( (fabs(grid.yvals[i-1] - grid.yvals[i]) - yinc) > (yinc/1000) ) break;

                      if ( i < (int) ysize ) yinc = 0;
                    }
                }

              if ( (int) ysize == 0 ) size = xsize;
              else if ( (int) xsize == 0 ) size = ysize;
              else if ( ncvars[ncvarid].gridtype == GRID_UNSTRUCTURED ) size = xsize;
              else size = xsize*ysize;
            }

          if ( ncvars[ncvarid].gridtype == UNDEFID ||
               ncvars[ncvarid].gridtype == GRID_GENERIC )
            {
              if ( islat && islon )
                {
                  if ( isGaussGrid(ysize, yinc, grid.yvals) )
                    {
                      ncvars[ncvarid].gridtype = GRID_GAUSSIAN;
                      np = ysize/2;
                    }
                  else
                    ncvars[ncvarid].gridtype = GRID_LONLAT;
                }
              else if ( islat && !islon && xsize == 0 )
                {
                  if ( isGaussGrid(ysize, yinc, grid.yvals) )
                    {
                      ncvars[ncvarid].gridtype = GRID_GAUSSIAN;
                      np = ysize/2;
                    }
                  else
                    ncvars[ncvarid].gridtype = GRID_LONLAT;
                }
              else if ( islon && !islat && ysize == 0 )
                {
                  ncvars[ncvarid].gridtype = GRID_LONLAT;
                }
              else
                ncvars[ncvarid].gridtype = GRID_GENERIC;
            }

          switch (ncvars[ncvarid].gridtype)
            {
            case GRID_GENERIC:
            case GRID_LONLAT:
            case GRID_GAUSSIAN:
            case GRID_UNSTRUCTURED:
            case GRID_CURVILINEAR:
              {
                grid.size = (int)size;
                grid.xsize = (int)xsize;
                grid.ysize = (int)ysize;
                grid.np = (int)np;
                if ( xvarid != UNDEFID )
                  {
                    grid.xdef = 1;
                    if ( ncvars[xvarid].bounds != UNDEFID )
                      {
                        nbdims = ncvars[ncvars[xvarid].bounds].ndims;
                        if ( nbdims == 2 || nbdims == 3 )
                          {
                            nvertex = ncdims[ncvars[ncvars[xvarid].bounds].dimids[nbdims-1]].len;
                            grid.nvertex = (int) nvertex;
                            grid.xbounds = (double *) Malloc(nvertex*size*sizeof(double));
                            cdf_get_var_double(ncvars[xvarid].ncid, ncvars[xvarid].bounds, grid.xbounds);
                          }
                      }
                  }
                if ( yvarid != UNDEFID )
                  {
                    grid.ydef = 1;
                    if ( ncvars[yvarid].bounds != UNDEFID )
                      {
                        nbdims = ncvars[ncvars[yvarid].bounds].ndims;
                        if ( nbdims == 2 || nbdims == 3 )
                          {
                            nvertex = ncdims[ncvars[ncvars[yvarid].bounds].dimids[nbdims-1]].len;





                            grid.ybounds = (double *) Malloc(nvertex*size*sizeof(double));
                            cdf_get_var_double(ncvars[yvarid].ncid, ncvars[yvarid].bounds, grid.ybounds);
                          }
                      }
                  }

                if ( ncvars[ncvarid].cellarea != UNDEFID )
                  {
                    grid.area = (double *) Malloc(size*sizeof(double));
                    cdf_get_var_double(ncvars[ncvarid].ncid, ncvars[ncvarid].cellarea, grid.area);
                  }

                break;
              }
            case GRID_SPECTRAL:
              {
                grid.size = (int)size;
                grid.lcomplex = 1;
                break;
              }
            case GRID_FOURIER:
              {
                grid.size = (int)size;
                break;
              }
            case GRID_TRAJECTORY:
              {
                grid.size = 1;
                break;
              }
            }

          grid.type = ncvars[ncvarid].gridtype;

          if ( grid.size == 0 )
            {
              if ( (ncvars[ncvarid].ndims == 1 && ncvars[ncvarid].dimtype[0] == T_AXIS) ||
                   (ncvars[ncvarid].ndims == 1 && ncvars[ncvarid].dimtype[0] == Z_AXIS) ||
                   (ncvars[ncvarid].ndims == 2 && ncvars[ncvarid].dimtype[0] == T_AXIS && ncvars[ncvarid].dimtype[1] == Z_AXIS) )
                {
                  grid.type = GRID_GENERIC;
                  grid.size = 1;
                  grid.xsize = 0;
                  grid.ysize = 0;
                }
              else
                {
                  Warning("Variable %s has an unsupported grid, skipped!", ncvars[ncvarid].name);
                  ncvars[ncvarid].isvar = -1;
                  continue;
                }
            }

          if ( number_of_grid_used != UNDEFID && (grid.type == UNDEFID || grid.type == GRID_GENERIC) )
            grid.type = GRID_UNSTRUCTURED;

          if ( number_of_grid_used != UNDEFID && grid.type == GRID_UNSTRUCTURED )
            grid.number = number_of_grid_used;

          if ( ncvars[ncvarid].gmapid >= 0 && ncvars[ncvarid].gridtype != GRID_CURVILINEAR )
            {
              cdf_inq_varnatts(ncvars[ncvarid].ncid, ncvars[ncvarid].gmapid, &nvatts);

              for ( iatt = 0; iatt < nvatts; iatt++ )
                {
                  cdf_inq_attname(ncvars[ncvarid].ncid, ncvars[ncvarid].gmapid, iatt, attname);
                  cdf_inq_attlen(ncvars[ncvarid].ncid, ncvars[ncvarid].gmapid, attname, &attlen);

                  if ( strcmp(attname, "grid_mapping_name") == 0 )
                    {
                      enum {
                        attstringlen = 8192,
                      };
                      char attstring[attstringlen];

                      cdfGetAttText(ncvars[ncvarid].ncid, ncvars[ncvarid].gmapid, attname, attstringlen, attstring);
                      strtolower(attstring);

                      if ( strcmp(attstring, "rotated_latitude_longitude") == 0 )
                        grid.isRotated = TRUE;
                      else if ( strcmp(attstring, "sinusoidal") == 0 )
                        grid.type = GRID_SINUSOIDAL;
                      else if ( strcmp(attstring, "lambert_azimuthal_equal_area") == 0 )
                        grid.type = GRID_LAEA;
                      else if ( strcmp(attstring, "lambert_conformal_conic") == 0 )
                        grid.type = GRID_LCC2;
                      else if ( strcmp(attstring, "lambert_cylindrical_equal_area") == 0 )
                        {
                          proj.type = GRID_PROJECTION;
                          proj.name = strdup(attstring);
                        }
                    }
                  else if ( strcmp(attname, "earth_radius") == 0 )
                    {
                      cdfGetAttDouble(ncvars[ncvarid].ncid, ncvars[ncvarid].gmapid, attname, 1, &datt);
                      grid.laea_a = datt;
                      grid.lcc2_a = datt;
                    }
                  else if ( strcmp(attname, "longitude_of_projection_origin") == 0 )
                    {
                      cdfGetAttDouble(ncvars[ncvarid].ncid, ncvars[ncvarid].gmapid, attname, 1, &grid.laea_lon_0);
                    }
                  else if ( strcmp(attname, "longitude_of_central_meridian") == 0 )
                    {
                      cdfGetAttDouble(ncvars[ncvarid].ncid, ncvars[ncvarid].gmapid, attname, 1, &grid.lcc2_lon_0);
                    }
                  else if ( strcmp(attname, "latitude_of_projection_origin") == 0 )
                    {
                      cdfGetAttDouble(ncvars[ncvarid].ncid, ncvars[ncvarid].gmapid, attname, 1, &datt);
                      grid.laea_lat_0 = datt;
                      grid.lcc2_lat_0 = datt;
                    }
                  else if ( strcmp(attname, "standard_parallel") == 0 )
                    {
                      if ( attlen == 1 )
                        {
                          cdfGetAttDouble(ncvars[ncvarid].ncid, ncvars[ncvarid].gmapid, attname, 1, &datt);
                          grid.lcc2_lat_1 = datt;
                          grid.lcc2_lat_2 = datt;
                        }
                      else
                        {
                          double datt2[2];
                          cdfGetAttDouble(ncvars[ncvarid].ncid, ncvars[ncvarid].gmapid, attname, 2, datt2);
                          grid.lcc2_lat_1 = datt2[0];
                          grid.lcc2_lat_2 = datt2[1];
                        }
                    }
                  else if ( strcmp(attname, "grid_north_pole_latitude") == 0 )
                    {
                      cdfGetAttDouble(ncvars[ncvarid].ncid, ncvars[ncvarid].gmapid, attname, 1, &grid.ypole);
                    }
                  else if ( strcmp(attname, "grid_north_pole_longitude") == 0 )
                    {
                      cdfGetAttDouble(ncvars[ncvarid].ncid, ncvars[ncvarid].gmapid, attname, 1, &grid.xpole);
                    }
                  else if ( strcmp(attname, "north_pole_grid_longitude") == 0 )
                    {
                      cdfGetAttDouble(ncvars[ncvarid].ncid, ncvars[ncvarid].gmapid, attname, 1, &grid.angle);
                    }
                }
            }

          if ( grid.type == GRID_UNSTRUCTURED )
            {
              int zdimid = UNDEFID;
              int xdimidx = -1, ydimidx = -1;

              for ( i = 0; i < ndims; i++ )
                {
                  if ( ncvars[ncvarid].dimtype[i] == X_AXIS ) xdimidx = i;
                  else if ( ncvars[ncvarid].dimtype[i] == Y_AXIS ) ydimidx = i;
                  else if ( ncvars[ncvarid].dimtype[i] == Z_AXIS ) zdimid = ncvars[ncvarid].dimids[i];
                }

              if ( xdimid != UNDEFID && ydimid != UNDEFID && zdimid == UNDEFID )
                {
                  if ( grid.xsize > grid.ysize && grid.ysize < 1000 )
                    {
                      ncvars[ncvarid].dimtype[ydimidx] = Z_AXIS;
                      ydimid = UNDEFID;
                      grid.size = grid.xsize;
                      grid.ysize = 0;
                    }
                  else if ( grid.ysize > grid.xsize && grid.xsize < 1000 )
                    {
                      ncvars[ncvarid].dimtype[xdimidx] = Z_AXIS;
                      xdimid = ydimid;
                      ydimid = UNDEFID;
                      grid.size = grid.ysize;
                      grid.xsize = grid.ysize;
                      grid.ysize = 0;
                    }
                }

              if ( grid.size != grid.xsize )
                {
                  Warning("Unsupported array structure, skipped variable %s!", ncvars[ncvarid].name);
                  ncvars[ncvarid].isvar = -1;
                  continue;
                }

              if ( ncvars[ncvarid].position > 0 ) grid.position = ncvars[ncvarid].position;
              if ( uuidOfHGrid[0] != 0 ) memcpy(grid.uuid, uuidOfHGrid, 16);
            }

#if defined (PROJECTION_TEST)

#endif

          if ( CDI_Debug )
            {
              Message("grid: type = %d, size = %d, nx = %d, ny %d",
                      grid.type, grid.size, grid.xsize, grid.ysize);
              Message("proj: type = %d, size = %d, nx = %d, ny %d",
                      proj.type, proj.size, proj.xsize, proj.ysize);
            }

#if defined (PROJECTION_TEST)

#endif
            ncvars[ncvarid].gridID = varDefGrid(vlistID, &grid, 1);

          if ( grid.type == GRID_UNSTRUCTURED )
            {
              if ( gridfile[0] != 0 ) gridDefReference(ncvars[ncvarid].gridID, gridfile);
            }

          if ( ncvars[ncvarid].chunked ) grid_set_chunktype(&grid, &ncvars[ncvarid]);

          gridindex = vlistGridIndex(vlistID, ncvars[ncvarid].gridID);
          streamptr->xdimID[gridindex] = xdimid;
          streamptr->ydimID[gridindex] = ydimid;
          if ( xdimid == -1 && ydimid == -1 && grid.size == 1 )
            gridDefHasDims(ncvars[ncvarid].gridID, FALSE);

          if ( CDI_Debug )
            Message("gridID %d %d %s", ncvars[ncvarid].gridID, ncvarid, ncvars[ncvarid].name);

          for ( ncvarid2 = ncvarid+1; ncvarid2 < nvars; ncvarid2++ )
            if ( ncvars[ncvarid2].isvar == TRUE && ncvars[ncvarid2].gridID == UNDEFID )
              {
                int xdimid2 = UNDEFID, ydimid2 = UNDEFID, zdimid2 = UNDEFID;
                int xdimidx = -1, ydimidx = -1;
                int ndims2 = ncvars[ncvarid2].ndims;

                for ( i = 0; i < ndims2; i++ )
                  {
                    if ( ncvars[ncvarid2].dimtype[i] == X_AXIS )
                      { xdimid2 = ncvars[ncvarid2].dimids[i]; xdimidx = i; }
                    else if ( ncvars[ncvarid2].dimtype[i] == Y_AXIS )
                      { ydimid2 = ncvars[ncvarid2].dimids[i]; ydimidx = i; }
                    else if ( ncvars[ncvarid2].dimtype[i] == Z_AXIS )
                      { zdimid2 = ncvars[ncvarid2].dimids[i]; }
                  }

                if ( ncvars[ncvarid2].gridtype == UNDEFID && grid.type == GRID_UNSTRUCTURED )
                  {
                    if ( xdimid == xdimid2 && ydimid2 != UNDEFID && zdimid2 == UNDEFID )
                      {
                        ncvars[ncvarid2].dimtype[ydimidx] = Z_AXIS;
                        ydimid2 = UNDEFID;
                      }

                    if ( xdimid == ydimid2 && xdimid2 != UNDEFID && zdimid2 == UNDEFID )
                      {
                        ncvars[ncvarid2].dimtype[xdimidx] = Z_AXIS;
                        xdimid2 = ydimid2;
                        ydimid2 = UNDEFID;
                      }
                  }

                if ( xdimid == xdimid2 &&
                    (ydimid == ydimid2 || (xdimid == ydimid && ydimid2 == UNDEFID)) )
                  {
                    int same_grid = TRUE;







                    if ( ncvars[ncvarid].xvarid != ncvars[ncvarid2].xvarid ) same_grid = FALSE;
                    if ( ncvars[ncvarid].yvarid != ncvars[ncvarid2].yvarid ) same_grid = FALSE;

                    if ( ncvars[ncvarid].position != ncvars[ncvarid2].position ) same_grid = FALSE;

                    if ( same_grid )
                      {
                        if ( CDI_Debug )
                          Message("Same gridID %d %d %s", ncvars[ncvarid].gridID, ncvarid2, ncvars[ncvarid2].name);
                        ncvars[ncvarid2].gridID = ncvars[ncvarid].gridID;
                        ncvars[ncvarid2].chunktype = ncvars[ncvarid].chunktype;
                      }
                  }
              }

          grid_free(&grid);
          grid_free(&proj);
        }
    }
}


static
void define_all_zaxes(stream_t *streamptr, int vlistID, ncdim_t *ncdims, int nvars, ncvar_t *ncvars,
                      size_t vctsize_echam, double *vct_echam, unsigned char *uuidOfVGrid)
{
  int ncvarid, ncvarid2;
  int i, ilev;
  int zaxisindex;
  int nbdims, nvertex, nlevel;
  int psvarid = -1;
  char *pname, *plongname, *punits;
  size_t vctsize = vctsize_echam;
  double *vct = vct_echam;

  for ( ncvarid = 0; ncvarid < nvars; ncvarid++ )
    {
      if ( ncvars[ncvarid].isvar == TRUE && ncvars[ncvarid].zaxisID == UNDEFID )
        {
          int is_scalar = FALSE;
          int with_bounds = FALSE;
          int zdimid = UNDEFID;
          int zvarid = UNDEFID;
          int zsize = 1;
          double *lbounds = NULL;
          double *ubounds = NULL;

          int positive = 0;
          int ndims = ncvars[ncvarid].ndims;

          if ( ncvars[ncvarid].zvarid != -1 && ncvars[ncvars[ncvarid].zvarid].ndims == 0 )
            {
              zvarid = ncvars[ncvarid].zvarid;
              is_scalar = TRUE;
            }
          else
            {
              for ( i = 0; i < ndims; i++ )
                {
                  if ( ncvars[ncvarid].dimtype[i] == Z_AXIS )
                    zdimid = ncvars[ncvarid].dimids[i];
                }

              if ( zdimid != UNDEFID )
                {
                  zvarid = ncdims[zdimid].ncvarid;
                  zsize = (int)ncdims[zdimid].len;
                }
            }

          if ( CDI_Debug ) Message("nlevs = %d", zsize);

          double *zvar = (double *) Malloc((size_t)zsize * sizeof (double));

          int zaxisType = UNDEFID;
          if ( zvarid != UNDEFID ) zaxisType = ncvars[zvarid].zaxistype;
          if ( zaxisType == UNDEFID ) zaxisType = ZAXIS_GENERIC;

          int zprec = DATATYPE_FLT64;

          if ( zvarid != UNDEFID )
            {
              positive = ncvars[zvarid].positive;
              pname = ncvars[zvarid].name;
              plongname = ncvars[zvarid].longname;
              punits = ncvars[zvarid].units;
              if ( ncvars[zvarid].xtype == NC_FLOAT ) zprec = DATATYPE_FLT32;






              psvarid = -1;
              if ( zaxisType == ZAXIS_HYBRID && ncvars[zvarid].vct )
                {
                  vct = ncvars[zvarid].vct;
                  vctsize = ncvars[zvarid].vctsize;

                  if ( ncvars[zvarid].psvarid != -1 ) psvarid = ncvars[zvarid].psvarid;
                }

              cdf_get_var_double(ncvars[zvarid].ncid, zvarid, zvar);

              if ( ncvars[zvarid].bounds != UNDEFID )
                {
                  nbdims = ncvars[ncvars[zvarid].bounds].ndims;
                  if ( nbdims == 2 )
                    {
                      nlevel = (int)ncdims[ncvars[ncvars[zvarid].bounds].dimids[0]].len;
                      nvertex = (int)ncdims[ncvars[ncvars[zvarid].bounds].dimids[1]].len;
                      if ( nlevel == zsize && nvertex == 2 )
                        {
                          with_bounds = TRUE;
                          lbounds = (double *) Malloc((size_t)nlevel*sizeof(double));
                          ubounds = (double *) Malloc((size_t)nlevel*sizeof(double));
                          double zbounds[2*nlevel];
                          cdf_get_var_double(ncvars[zvarid].ncid, ncvars[zvarid].bounds, zbounds);
                          for ( i = 0; i < nlevel; ++i )
                            {
                              lbounds[i] = zbounds[i*2];
                              ubounds[i] = zbounds[i*2+1];
                            }
                        }
                    }
                }
            }
          else
            {
              pname = NULL;
              plongname = NULL;
              punits = NULL;

              if ( zsize == 1 )
                {
                  if ( ncvars[ncvarid].zaxistype != UNDEFID )
                    zaxisType = ncvars[ncvarid].zaxistype;
                  else
                    zaxisType = ZAXIS_SURFACE;

                  zvar[0] = 0;






                }
              else
                {
                  for ( ilev = 0; ilev < zsize; ilev++ ) zvar[ilev] = ilev + 1;
                }
            }

          ncvars[ncvarid].zaxisID = varDefZaxis(vlistID, zaxisType, (int) zsize, zvar, with_bounds, lbounds, ubounds,
                                                (int)vctsize, vct, pname, plongname, punits, zprec, 1, 0);

          if ( uuidOfVGrid[0] != 0 )
            {

              zaxisDefUUID(ncvars[ncvarid].zaxisID, uuidOfVGrid);
            }

          if ( zaxisType == ZAXIS_HYBRID && psvarid != -1 ) zaxisDefPsName(ncvars[ncvarid].zaxisID, ncvars[psvarid].name);

          if ( positive > 0 ) zaxisDefPositive(ncvars[ncvarid].zaxisID, positive);
          if ( is_scalar ) zaxisDefScalar(ncvars[ncvarid].zaxisID);

          Free(zvar);
          Free(lbounds);
          Free(ubounds);

          zaxisindex = vlistZaxisIndex(vlistID, ncvars[ncvarid].zaxisID);
          streamptr->zaxisID[zaxisindex] = zdimid;

          if ( CDI_Debug )
            Message("zaxisID %d %d %s", ncvars[ncvarid].zaxisID, ncvarid, ncvars[ncvarid].name);

          for ( ncvarid2 = ncvarid+1; ncvarid2 < nvars; ncvarid2++ )
            if ( ncvars[ncvarid2].isvar == TRUE && ncvars[ncvarid2].zaxisID == UNDEFID )
              {
                int zvarid2 = UNDEFID;
                if ( ncvars[ncvarid2].zvarid != UNDEFID && ncvars[ncvars[ncvarid2].zvarid].ndims == 0 )
                  zvarid2 = ncvars[ncvarid2].zvarid;

                int zdimid2 = UNDEFID;
                ndims = ncvars[ncvarid2].ndims;
                for ( i = 0; i < ndims; i++ )
                  {
                    if ( ncvars[ncvarid2].dimtype[i] == Z_AXIS )
                      zdimid2 = ncvars[ncvarid2].dimids[i];
                  }

                if ( zdimid == zdimid2 )
                  {
                    if ( (zdimid != UNDEFID && ncvars[ncvarid2].zaxistype == UNDEFID) ||
                         (zdimid == UNDEFID && zvarid != UNDEFID && zvarid == zvarid2) ||
                         (zdimid == UNDEFID && zaxisType == ncvars[ncvarid2].zaxistype) ||
                         (zdimid == UNDEFID && zvarid2 == UNDEFID && ncvars[ncvarid2].zaxistype == UNDEFID) )
                      {
                        if ( CDI_Debug )
                          Message("zaxisID %d %d %s", ncvars[ncvarid].zaxisID, ncvarid2, ncvars[ncvarid2].name);
                        ncvars[ncvarid2].zaxisID = ncvars[ncvarid].zaxisID;
                      }
                  }
              }
        }
    }
}

struct varinfo
{
  int ncvarid;
  const char *name;
};

static
int cmpvarname(const void *s1, const void *s2)
{
  const struct varinfo *x = (const struct varinfo *)s1,
    *y = (const struct varinfo *)s2;
  return (strcmp(x->name, y->name));
}


static
void define_all_vars(stream_t *streamptr, int vlistID, int instID, int modelID, int *varids, int nvars, int num_ncvars, ncvar_t *ncvars)
{
  if ( streamptr->sortname )
    {
      struct varinfo *varInfo
        = (struct varinfo *) Malloc((size_t)nvars * sizeof (struct varinfo));

      for ( int varID = 0; varID < nvars; varID++ )
        {
          int ncvarid = varids[varID];
          varInfo[varID].ncvarid = ncvarid;
          varInfo[varID].name = ncvars[ncvarid].name;
        }
      qsort(varInfo, (size_t)nvars, sizeof(varInfo[0]), cmpvarname);
      for ( int varID = 0; varID < nvars; varID++ )
        {
          varids[varID] = varInfo[varID].ncvarid;
        }
      Free(varInfo);
    }

  for ( int varID1 = 0; varID1 < nvars; varID1++ )
    {
      int ncvarid = varids[varID1];
      int gridID = ncvars[ncvarid].gridID;
      int zaxisID = ncvars[ncvarid].zaxisID;

      stream_new_var(streamptr, gridID, zaxisID, CDI_UNDEFID);
      int varID = vlistDefVar(vlistID, gridID, zaxisID, ncvars[ncvarid].tsteptype);

#if defined (HAVE_NETCDF4)
      if ( ncvars[ncvarid].deflate )
        vlistDefVarCompType(vlistID, varID, COMPRESS_ZIP);

      if ( ncvars[ncvarid].chunked && ncvars[ncvarid].chunktype != UNDEFID )
        vlistDefVarChunkType(vlistID, varID, ncvars[ncvarid].chunktype);
#endif

      streamptr->vars[varID1].defmiss = 0;
      streamptr->vars[varID1].ncvarid = ncvarid;

      vlistDefVarName(vlistID, varID, ncvars[ncvarid].name);
      if ( ncvars[ncvarid].param != UNDEFID ) vlistDefVarParam(vlistID, varID, ncvars[ncvarid].param);
      if ( ncvars[ncvarid].code != UNDEFID ) vlistDefVarCode(vlistID, varID, ncvars[ncvarid].code);
      if ( ncvars[ncvarid].code != UNDEFID )
        {
          int param = cdiEncodeParam(ncvars[ncvarid].code, ncvars[ncvarid].tabnum, 255);
          vlistDefVarParam(vlistID, varID, param);
        }
      if ( ncvars[ncvarid].longname[0] ) vlistDefVarLongname(vlistID, varID, ncvars[ncvarid].longname);
      if ( ncvars[ncvarid].stdname[0] ) vlistDefVarStdname(vlistID, varID, ncvars[ncvarid].stdname);
      if ( ncvars[ncvarid].units[0] ) vlistDefVarUnits(vlistID, varID, ncvars[ncvarid].units);

      if ( ncvars[ncvarid].lvalidrange )
        vlistDefVarValidrange(vlistID, varID, ncvars[ncvarid].validrange);

      if ( IS_NOT_EQUAL(ncvars[ncvarid].addoffset, 0) )
        vlistDefVarAddoffset(vlistID, varID, ncvars[ncvarid].addoffset);
      if ( IS_NOT_EQUAL(ncvars[ncvarid].scalefactor, 1) )
        vlistDefVarScalefactor(vlistID, varID, ncvars[ncvarid].scalefactor);

      vlistDefVarDatatype(vlistID, varID, cdfInqDatatype(ncvars[ncvarid].xtype, ncvars[ncvarid].lunsigned));

      vlistDefVarInstitut(vlistID, varID, instID);
      vlistDefVarModel(vlistID, varID, modelID);
      if ( ncvars[ncvarid].tableID != UNDEFID )
        vlistDefVarTable(vlistID, varID, ncvars[ncvarid].tableID);

      if ( ncvars[ncvarid].deffillval == FALSE && ncvars[ncvarid].defmissval == TRUE )
        {
          ncvars[ncvarid].deffillval = TRUE;
          ncvars[ncvarid].fillval = ncvars[ncvarid].missval;
        }

      if ( ncvars[ncvarid].deffillval == TRUE )
        vlistDefVarMissval(vlistID, varID, ncvars[ncvarid].fillval);

      if ( CDI_Debug )
        Message("varID = %d  gridID = %d  zaxisID = %d", varID,
                vlistInqVarGrid(vlistID, varID), vlistInqVarZaxis(vlistID, varID));

      int gridindex = vlistGridIndex(vlistID, gridID);
      int xdimid = streamptr->xdimID[gridindex];
      int ydimid = streamptr->ydimID[gridindex];

      int zaxisindex = vlistZaxisIndex(vlistID, zaxisID);
      int zdimid = streamptr->zaxisID[zaxisindex];

      int ndims = ncvars[ncvarid].ndims;
      int iodim = 0;
      int ixyz = 0;
      int ipow10[4] = {1, 10, 100, 1000};

      if ( ncvars[ncvarid].tsteptype != TSTEP_CONSTANT ) iodim++;

      if ( gridInqType(gridID) == GRID_UNSTRUCTURED && ndims-iodim <= 2 && ydimid == xdimid )
        {
          if ( xdimid == ncvars[ncvarid].dimids[ndims-1] )
            {
              ixyz = 321;
            }
          else
            {
              ixyz = 213;
            }
        }
      else
        {
          for ( int idim = iodim; idim < ndims; idim++ )
            {
              if ( xdimid == ncvars[ncvarid].dimids[idim] )
                ixyz += 1*ipow10[ndims-idim-1];
              else if ( ydimid == ncvars[ncvarid].dimids[idim] )
                ixyz += 2*ipow10[ndims-idim-1];
              else if ( zdimid == ncvars[ncvarid].dimids[idim] )
                ixyz += 3*ipow10[ndims-idim-1];
            }
        }

      vlistDefVarXYZ(vlistID, varID, ixyz);







      if ( ncvars[ncvarid].ensdata != NULL )
        {
          vlistDefVarEnsemble( vlistID, varID, ncvars[ncvarid].ensdata->ens_index,
                               ncvars[ncvarid].ensdata->ens_count,
                               ncvars[ncvarid].ensdata->forecast_init_type );
          Free(ncvars[ncvarid].ensdata);
          ncvars[ncvarid].ensdata = NULL;
        }

      if ( ncvars[ncvarid].extra[0] != 0 )
        {
          vlistDefVarExtra(vlistID, varID, ncvars[ncvarid].extra);
        }
    }

  for ( int varID = 0; varID < nvars; varID++ )
    {
      int ncvarid = varids[varID];
      int ncid = ncvars[ncvarid].ncid;

      if ( ncvars[ncvarid].natts )
        {
          int attnum;
          int iatt;
          nc_type attrtype;
          size_t attlen;
          char attname[CDI_MAX_NAME];
          const int attstringlen = 8192; char attstring[8192];
          int nvatts = ncvars[ncvarid].natts;

          for ( iatt = 0; iatt < nvatts; iatt++ )
            {
              attnum = ncvars[ncvarid].atts[iatt];
              cdf_inq_attname(ncid, ncvarid, attnum, attname);
              cdf_inq_attlen(ncid, ncvarid, attname, &attlen);
              cdf_inq_atttype(ncid, ncvarid, attname, &attrtype);

              if ( attrtype == NC_SHORT || attrtype == NC_INT )
                {
                  int attint[attlen];
                  cdfGetAttInt(ncid, ncvarid, attname, (int)attlen, attint);
                  if ( attrtype == NC_SHORT )
                    vlistDefAttInt(vlistID, varID, attname, DATATYPE_INT16, (int)attlen, attint);
                  else
                    vlistDefAttInt(vlistID, varID, attname, DATATYPE_INT32, (int)attlen, attint);
                }
              else if ( attrtype == NC_FLOAT || attrtype == NC_DOUBLE )
                {
                  double attflt[attlen];
                  cdfGetAttDouble(ncid, ncvarid, attname, (int)attlen, attflt);
                  if ( attrtype == NC_FLOAT )
                    vlistDefAttFlt(vlistID, varID, attname, DATATYPE_FLT32, (int)attlen, attflt);
                  else
                    vlistDefAttFlt(vlistID, varID, attname, DATATYPE_FLT64, (int)attlen, attflt);
                }
              else if ( xtypeIsText(attrtype) )
                {
                  cdfGetAttText(ncid, ncvarid, attname, attstringlen, attstring);
                  vlistDefAttTxt(vlistID, varID, attname, (int)attlen, attstring);
                }
              else
                {
                  if ( CDI_Debug ) printf("att: %s.%s = unknown\n", ncvars[ncvarid].name, attname);
                }
            }

          if (ncvars[ncvarid].vct) Free(ncvars[ncvarid].vct);
          if (ncvars[ncvarid].atts) Free(ncvars[ncvarid].atts);
          ncvars[ncvarid].vct = NULL;
          ncvars[ncvarid].atts = NULL;
        }
    }


  for ( int ncvarid = 0; ncvarid < num_ncvars; ncvarid++ )
    if ( ncvars[ncvarid].atts ) Free(ncvars[ncvarid].atts);

  if ( varids ) Free(varids);

  for ( int varID = 0; varID < nvars; varID++ )
    {
      if ( vlistInqVarCode(vlistID, varID) == -varID-1 )
        {
          const char *pname = vlistInqVarNamePtr(vlistID, varID);
          size_t len = strlen(pname);
          if ( len > 3 && isdigit((int) pname[3]) )
            {
              if ( memcmp("var", pname, 3) == 0 )
                {
                  vlistDefVarCode(vlistID, varID, atoi(pname+3));

                }
            }
          else if ( len > 4 && isdigit((int) pname[4]) )
            {
              if ( memcmp("code", pname, 4) == 0 )
                {
                  vlistDefVarCode(vlistID, varID, atoi(pname+4));

                }
            }
          else if ( len > 5 && isdigit((int) pname[5]) )
            {
              if ( memcmp("param", pname, 5) == 0 )
                {
                  int pnum = -1, pcat = 255, pdis = 255;
                  sscanf(pname+5, "%d.%d.%d", &pnum, &pcat, &pdis);
                  vlistDefVarParam(vlistID, varID, cdiEncodeParam(pnum, pcat, pdis));

                }
            }
        }
    }

  for ( int varID = 0; varID < nvars; varID++ )
    {
      int varInstID = vlistInqVarInstitut(vlistID, varID);
      int varModelID = vlistInqVarModel(vlistID, varID);
      int varTableID = vlistInqVarTable(vlistID, varID);
      int code = vlistInqVarCode(vlistID, varID);
      if ( cdiDefaultTableID != UNDEFID )
        {
          if ( tableInqParNamePtr(cdiDefaultTableID, code) )
            {
              vlistDestroyVarName(vlistID, varID);
              vlistDestroyVarLongname(vlistID, varID);
              vlistDestroyVarUnits(vlistID, varID);

              if ( varTableID != UNDEFID )
                {
                  vlistDefVarName(vlistID, varID, tableInqParNamePtr(cdiDefaultTableID, code));
                  if ( tableInqParLongnamePtr(cdiDefaultTableID, code) )
                    vlistDefVarLongname(vlistID, varID, tableInqParLongnamePtr(cdiDefaultTableID, code));
                  if ( tableInqParUnitsPtr(cdiDefaultTableID, code) )
                    vlistDefVarUnits(vlistID, varID, tableInqParUnitsPtr(cdiDefaultTableID, code));
                }
              else
                {
                  varTableID = cdiDefaultTableID;
                }
            }

          if ( cdiDefaultModelID != UNDEFID ) varModelID = cdiDefaultModelID;
          if ( cdiDefaultInstID != UNDEFID ) varInstID = cdiDefaultInstID;
        }
      if ( varInstID != UNDEFID ) vlistDefVarInstitut(vlistID, varID, varInstID);
      if ( varModelID != UNDEFID ) vlistDefVarModel(vlistID, varID, varModelID);
      if ( varTableID != UNDEFID ) vlistDefVarTable(vlistID, varID, varTableID);
    }
}

static
void scan_global_attributes(int fileID, int vlistID, stream_t *streamptr, int ngatts, int *instID, int *modelID, int *ucla_les, unsigned char *uuidOfHGrid, unsigned char *uuidOfVGrid, char *gridfile, int *number_of_grid_used)
{
  nc_type xtype;
  size_t attlen;
  char attname[CDI_MAX_NAME];
  enum { attstringlen = 65636 };
  char attstring[attstringlen];
  int iatt;

  for ( iatt = 0; iatt < ngatts; iatt++ )
    {
      cdf_inq_attname(fileID, NC_GLOBAL, iatt, attname);
      cdf_inq_atttype(fileID, NC_GLOBAL, attname, &xtype);
      cdf_inq_attlen(fileID, NC_GLOBAL, attname, &attlen);

      if ( xtypeIsText(xtype) )
        {
          cdfGetAttText(fileID, NC_GLOBAL, attname, attstringlen, attstring);

          size_t attstrlen = strlen(attstring);

          if ( attlen > 0 && attstring[0] != 0 )
            {
              if ( strcmp(attname, "history") == 0 )
                {
                  streamptr->historyID = iatt;
                }
              else if ( strcmp(attname, "institution") == 0 )
                {
                  *instID = institutInq(0, 0, NULL, attstring);
                  if ( *instID == UNDEFID )
                    *instID = institutDef(0, 0, NULL, attstring);
                }
              else if ( strcmp(attname, "source") == 0 )
                {
                  *modelID = modelInq(-1, 0, attstring);
                  if ( *modelID == UNDEFID )
                    *modelID = modelDef(-1, 0, attstring);
                }
              else if ( strcmp(attname, "Source") == 0 )
                {
                  if ( strncmp(attstring, "UCLA-LES", 8) == 0 )
                    *ucla_les = TRUE;
                }





              else if ( strcmp(attname, "CDI") == 0 )
                {
                }
              else if ( strcmp(attname, "CDO") == 0 )
                {
                }






              else if ( strcmp(attname, "grid_file_uri") == 0 )
                {
                  memcpy(gridfile, attstring, attstrlen+1);
                }
              else if ( strcmp(attname, "uuidOfHGrid") == 0 && attstrlen == 36 )
                {
                  attstring[36] = 0;
                  str2uuid(attstring, uuidOfHGrid);

                }
              else if ( strcmp(attname, "uuidOfVGrid") == 0 && attstrlen == 36 )
                {
                  attstring[36] = 0;
                  str2uuid(attstring, uuidOfVGrid);
                }
              else
                {
                  if ( strcmp(attname, "ICON_grid_file_uri") == 0 && gridfile[0] == 0 )
                    {
                      memcpy(gridfile, attstring, attstrlen+1);
                    }

                  vlistDefAttTxt(vlistID, CDI_GLOBAL, attname, (int)attstrlen, attstring);
                }
            }
        }
      else if ( xtype == NC_SHORT || xtype == NC_INT )
        {
          if ( strcmp(attname, "number_of_grid_used") == 0 )
            {
              (*number_of_grid_used) = UNDEFID;
              cdfGetAttInt(fileID, NC_GLOBAL, attname, 1, number_of_grid_used);
            }
          else
            {
              int attint[attlen];
              cdfGetAttInt(fileID, NC_GLOBAL, attname, (int)attlen, attint);
              if ( xtype == NC_SHORT )
                vlistDefAttInt(vlistID, CDI_GLOBAL, attname, DATATYPE_INT16, (int)attlen, attint);
              else
                vlistDefAttInt(vlistID, CDI_GLOBAL, attname, DATATYPE_INT32, (int)attlen, attint);
            }
        }
      else if ( xtype == NC_FLOAT || xtype == NC_DOUBLE )
        {
          double attflt[attlen];
          cdfGetAttDouble(fileID, NC_GLOBAL, attname, (int)attlen, attflt);
          if ( xtype == NC_FLOAT )
            vlistDefAttFlt(vlistID, CDI_GLOBAL, attname, DATATYPE_FLT32, (int)attlen, attflt);
          else
            vlistDefAttFlt(vlistID, CDI_GLOBAL, attname, DATATYPE_FLT64, (int)attlen, attflt);
        }
    }
}

static
int find_leadtime(int nvars, ncvar_t *ncvars)
{
  int leadtime_id = UNDEFID;

  for ( int ncvarid = 0; ncvarid < nvars; ncvarid++ )
    {
      if ( ncvars[ncvarid].stdname[0] )
        {
          if ( strcmp(ncvars[ncvarid].stdname, "forecast_period") == 0 )
            {
              leadtime_id = ncvarid;
              break;
            }
        }
    }

  return (leadtime_id);
}

static
void find_time_vars(int nvars, ncvar_t *ncvars, ncdim_t *ncdims, int timedimid, stream_t *streamptr,
                    int *time_has_units, int *time_has_bounds, int *time_climatology)
{
  int ncvarid;

  if ( timedimid == UNDEFID )
    {
      char timeunits[CDI_MAX_NAME];

      for ( ncvarid = 0; ncvarid < nvars; ncvarid++ )
        {
          if ( ncvars[ncvarid].ndims == 0 && strcmp(ncvars[ncvarid].name, "time") == 0 )
            {
              if ( ncvars[ncvarid].units[0] )
                {
                  strcpy(timeunits, ncvars[ncvarid].units);
                  strtolower(timeunits);

                  if ( isTimeUnits(timeunits) )
                    {
                      streamptr->basetime.ncvarid = ncvarid;
                      break;
                    }
                }
            }
        }
    }
  else
    {
      int ltimevar = FALSE;

      if ( ncdims[timedimid].ncvarid != UNDEFID )
        {
          streamptr->basetime.ncvarid = ncdims[timedimid].ncvarid;
          ltimevar = TRUE;
        }

      for ( ncvarid = 0; ncvarid < nvars; ncvarid++ )
        if ( ncvarid != streamptr->basetime.ncvarid &&
             ncvars[ncvarid].ndims == 1 &&
             timedimid == ncvars[ncvarid].dimids[0] &&
             !xtypeIsText(ncvars[ncvarid].xtype) &&
             isTimeAxisUnits(ncvars[ncvarid].units) )
          {
            ncvars[ncvarid].isvar = FALSE;

            if ( !ltimevar )
              {
                streamptr->basetime.ncvarid = ncvarid;
                ltimevar = TRUE;
                if ( CDI_Debug )
                  fprintf(stderr, "timevar %s\n", ncvars[ncvarid].name);
              }
            else
              {
                Warning("Found more than one time variable, skipped variable %s!", ncvars[ncvarid].name);
              }
          }

      if ( ltimevar == FALSE )
        {
          for ( ncvarid = 0; ncvarid < nvars; ncvarid++ )
            if ( ncvarid != streamptr->basetime.ncvarid &&
                 ncvars[ncvarid].ndims == 2 &&
                 timedimid == ncvars[ncvarid].dimids[0] &&
                 xtypeIsText(ncvars[ncvarid].xtype) &&
                 ncdims[ncvars[ncvarid].dimids[1]].len == 19 )
              {
                streamptr->basetime.ncvarid = ncvarid;
                streamptr->basetime.lwrf = TRUE;
                break;
              }
        }


      ncvarid = streamptr->basetime.ncvarid;

      if ( ncvarid == UNDEFID )
        {
          Warning("Time variable >%s< not found!", ncdims[timedimid].name);
        }
    }


  ncvarid = streamptr->basetime.ncvarid;

  if ( ncvarid != UNDEFID && streamptr->basetime.lwrf == FALSE )
    {
      if ( ncvars[ncvarid].units[0] != 0 ) *time_has_units = TRUE;

      if ( ncvars[ncvarid].bounds != UNDEFID )
        {
          int nbdims = ncvars[ncvars[ncvarid].bounds].ndims;
          if ( nbdims == 2 )
            {
              int len = (int) ncdims[ncvars[ncvars[ncvarid].bounds].dimids[nbdims-1]].len;
              if ( len == 2 && timedimid == ncvars[ncvars[ncvarid].bounds].dimids[0] )
                {
                  *time_has_bounds = TRUE;
                  streamptr->basetime.ncvarboundsid = ncvars[ncvarid].bounds;
                  if ( ncvars[ncvarid].climatology ) *time_climatology = TRUE;
                }
            }
        }
    }
}

static
void read_vct_echam(int fileID, int nvars, ncvar_t *ncvars, ncdim_t *ncdims, double **vct, size_t *pvctsize)
{

  int nvcth_id = UNDEFID, vcta_id = UNDEFID, vctb_id = UNDEFID;

  for ( int ncvarid = 0; ncvarid < nvars; ncvarid++ )
    {
      if ( ncvars[ncvarid].ndims == 1 )
        {
          size_t len = strlen(ncvars[ncvarid].name);
          if ( len == 4 && ncvars[ncvarid].name[0] == 'h' && ncvars[ncvarid].name[1] == 'y' )
            {
              if ( ncvars[ncvarid].name[2] == 'a' && ncvars[ncvarid].name[3] == 'i' )
                {
                  vcta_id = ncvarid;
                  nvcth_id = ncvars[ncvarid].dimids[0];
                  ncvars[ncvarid].isvar = FALSE;
                }
              else if ( ncvars[ncvarid].name[2] == 'b' && ncvars[ncvarid].name[3] == 'i' )
                {
                  vctb_id = ncvarid;
                  nvcth_id = ncvars[ncvarid].dimids[0];
                  ncvars[ncvarid].isvar = FALSE;
                }
              else if ( (ncvars[ncvarid].name[2] == 'a' || ncvars[ncvarid].name[2] == 'b') && ncvars[ncvarid].name[3] == 'm' )
                {
                  ncvars[ncvarid].isvar = FALSE;
                }
            }
        }
    }


  if ( nvcth_id != UNDEFID && vcta_id != UNDEFID && vctb_id != UNDEFID )
    {
      size_t vctsize = ncdims[nvcth_id].len;
      vctsize *= 2;
      *vct = (double *) Malloc(vctsize*sizeof(double));
      cdf_get_var_double(fileID, vcta_id, *vct);
      cdf_get_var_double(fileID, vctb_id, *vct+vctsize/2);
      *pvctsize = vctsize;
    }
}


int cdfInqContents(stream_t *streamptr)
{
  int ndims, nvars, ngatts, unlimdimid;
  int ncvarid;
  int ncdimid;
  size_t ntsteps;
  int timedimid = -1;
  int *varids;
  int nvarids;
  int time_has_units = FALSE;
  int time_has_bounds = FALSE;
  int time_climatology = FALSE;
  int leadtime_id = UNDEFID;
  int nvars_data;
  int instID = UNDEFID;
  int modelID = UNDEFID;
  int taxisID;
  int i;
  int calendar = UNDEFID;
  ncdim_t *ncdims;
  ncvar_t *ncvars = NULL;
  int format = 0;
  int ucla_les = FALSE;
  unsigned char uuidOfHGrid[CDI_UUID_SIZE];
  unsigned char uuidOfVGrid[CDI_UUID_SIZE];
  char gridfile[8912];
  char fcreftime[CDI_MAX_NAME];
  int number_of_grid_used = UNDEFID;

  memset(uuidOfHGrid, 0, CDI_UUID_SIZE);
  memset(uuidOfVGrid, 0, CDI_UUID_SIZE);
  gridfile[0] = 0;
  fcreftime[0] = 0;

  int vlistID = streamptr->vlistID;
  int fileID = streamptr->fileID;

  if ( CDI_Debug ) Message("streamID = %d, fileID = %d", streamptr->self, fileID);

#if defined (HAVE_NETCDF4)
  nc_inq_format(fileID, &format);
#endif

  cdf_inq(fileID, &ndims , &nvars, &ngatts, &unlimdimid);

  if ( CDI_Debug )
    Message("root: ndims %d, nvars %d, ngatts %d", ndims, nvars, ngatts);

  if ( ndims == 0 )
    {
      Warning("ndims = %d", ndims);
      return (CDI_EUFSTRUCT);
    }


  ncdims = (ncdim_t *) Malloc((size_t)ndims * sizeof (ncdim_t));
  init_ncdims(ndims, ncdims);

  if ( nvars > 0 )
    {

      ncvars = (ncvar_t *) Malloc((size_t)nvars * sizeof (ncvar_t));
      init_ncvars(nvars, ncvars);

      for ( ncvarid = 0; ncvarid < nvars; ++ncvarid )
        ncvars[ncvarid].ncid = fileID;
    }

#if defined (TEST_GROUPS)
#if defined (HAVE_NETCDF4)
  if ( format == NC_FORMAT_NETCDF4 )
    {
      int ncid;
      int numgrps;
      int ncids[NC_MAX_VARS];
      char name1[CDI_MAX_NAME];
      int gndims, gnvars, gngatts, gunlimdimid;
      nc_inq_grps(fileID, &numgrps, ncids);
      for ( int i = 0; i < numgrps; ++i )
        {
          ncid = ncids[i];
          nc_inq_grpname (ncid, name1);
          cdf_inq(ncid, &gndims , &gnvars, &gngatts, &gunlimdimid);

          if ( CDI_Debug )
            Message("%s: ndims %d, nvars %d, ngatts %d", name1, gndims, gnvars, gngatts);

          if ( gndims == 0 )
            {
            }
        }
    }
#endif
#endif

  if ( nvars == 0 )
    {
      Warning("nvars = %d", nvars);
      return (CDI_EUFSTRUCT);
    }


  scan_global_attributes(fileID, vlistID, streamptr, ngatts, &instID, &modelID, &ucla_les,
                         uuidOfHGrid, uuidOfVGrid, gridfile, &number_of_grid_used);


  if ( unlimdimid >= 0 )
    timedimid = unlimdimid;
  else
    timedimid = cdfTimeDimID(fileID, ndims, nvars);

  streamptr->basetime.ncdimid = timedimid;

  if ( timedimid != UNDEFID )
    cdf_inq_dimlen(fileID, timedimid, &ntsteps);
  else
    ntsteps = 0;

  if ( CDI_Debug ) Message("Number of timesteps = %d", ntsteps);
  if ( CDI_Debug ) Message("Time dimid = %d", streamptr->basetime.ncdimid);


  for ( ncdimid = 0; ncdimid < ndims; ncdimid++ )
    {
      cdf_inq_dimlen(fileID, ncdimid, &ncdims[ncdimid].len);
      cdf_inq_dimname(fileID, ncdimid, ncdims[ncdimid].name);
      if ( timedimid == ncdimid )
        ncdims[ncdimid].dimtype = T_AXIS;
    }

  if ( CDI_Debug ) printNCvars(ncvars, nvars, "cdfScanVarAttributes");


  cdfScanVarAttributes(nvars, ncvars, ncdims, timedimid, modelID, format);


  if ( CDI_Debug ) printNCvars(ncvars, nvars, "find coordinate vars");


  for ( ncdimid = 0; ncdimid < ndims; ncdimid++ )
    {
      for ( ncvarid = 0; ncvarid < nvars; ncvarid++ )
        {
          if ( ncvars[ncvarid].ndims == 1 )
            {
              if ( timedimid != UNDEFID && timedimid == ncvars[ncvarid].dimids[0] )
                {
                  if ( ncvars[ncvarid].isvar != FALSE ) cdfSetVar(ncvars, ncvarid, TRUE);
                }
              else
                {

                }


              if ( ncdimid == ncvars[ncvarid].dimids[0] && ncdims[ncdimid].ncvarid == UNDEFID )
                if ( strcmp(ncvars[ncvarid].name, ncdims[ncdimid].name) == 0 )
                  {
                    ncdims[ncdimid].ncvarid = ncvarid;
                    ncvars[ncvarid].isvar = FALSE;
                  }
            }
        }
    }


  find_time_vars(nvars, ncvars, ncdims, timedimid, streamptr, &time_has_units, &time_has_bounds, &time_climatology);

  leadtime_id = find_leadtime(nvars, ncvars);
  if ( leadtime_id != UNDEFID ) ncvars[leadtime_id].isvar = FALSE;


  for ( ncvarid = 0; ncvarid < nvars; ncvarid++ )
    {
      if ( timedimid != UNDEFID )
        if ( ncvars[ncvarid].isvar == -1 &&
             ncvars[ncvarid].ndims > 1 &&
             timedimid == ncvars[ncvarid].dimids[0] )
          cdfSetVar(ncvars, ncvarid, TRUE);

      if ( ncvars[ncvarid].isvar == -1 && ncvars[ncvarid].ndims == 0 )
        cdfSetVar(ncvars, ncvarid, FALSE);


      if ( ncvars[ncvarid].isvar == -1 && ncvars[ncvarid].ndims >= 1 )
        cdfSetVar(ncvars, ncvarid, TRUE);

      if ( ncvars[ncvarid].isvar == -1 )
        {
          ncvars[ncvarid].isvar = 0;
          Warning("Variable %s has an unknown type, skipped!", ncvars[ncvarid].name);
          continue;
        }

      if ( ncvars[ncvarid].ndims > 4 )
        {
          ncvars[ncvarid].isvar = 0;
          Warning("%d dimensional variables are not supported, skipped variable %s!",
                ncvars[ncvarid].ndims, ncvars[ncvarid].name);
          continue;
        }

      if ( ncvars[ncvarid].ndims == 4 && timedimid == UNDEFID )
        {
          ncvars[ncvarid].isvar = 0;
          Warning("%d dimensional variables without time dimension are not supported, skipped variable %s!",
                ncvars[ncvarid].ndims, ncvars[ncvarid].name);
          continue;
        }

      if ( xtypeIsText(ncvars[ncvarid].xtype) )
        {
          ncvars[ncvarid].isvar = 0;
          continue;
        }

      if ( cdfInqDatatype(ncvars[ncvarid].xtype, ncvars[ncvarid].lunsigned) == -1 )
        {
          ncvars[ncvarid].isvar = 0;
          Warning("Variable %s has an unsupported data type, skipped!", ncvars[ncvarid].name);
          continue;
        }

      if ( timedimid != UNDEFID && ntsteps == 0 && ncvars[ncvarid].ndims > 0 )
        {
          if ( timedimid == ncvars[ncvarid].dimids[0] )
            {
              ncvars[ncvarid].isvar = 0;
              Warning("Number of time steps undefined, skipped variable %s!", ncvars[ncvarid].name);
              continue;
            }
        }
    }


  verify_coordinate_vars_1(fileID, ndims, ncdims, ncvars, timedimid);


  verify_coordinate_vars_2(nvars, ncvars);

  if ( CDI_Debug ) printNCvars(ncvars, nvars, "verify_coordinate_vars");

  if ( ucla_les == TRUE )
    {
      for ( ncdimid = 0; ncdimid < ndims; ncdimid++ )
        {
          ncvarid = ncdims[ncdimid].ncvarid;
          if ( ncvarid != -1 )
            {
              if ( ncdims[ncdimid].dimtype == UNDEFID && ncvars[ncvarid].units[0] == 'm' )
                {
                  if ( ncvars[ncvarid].name[0] == 'x' ) ncdims[ncdimid].dimtype = X_AXIS;
                  else if ( ncvars[ncvarid].name[0] == 'y' ) ncdims[ncdimid].dimtype = Y_AXIS;
                  else if ( ncvars[ncvarid].name[0] == 'z' ) ncdims[ncdimid].dimtype = Z_AXIS;
                }
            }
        }
    }
  for ( ncvarid = 0; ncvarid < nvars; ncvarid++ )
    {
      if ( ncvars[ncvarid].isvar == TRUE && ncvars[ncvarid].ncoordvars )
        {

          ndims = ncvars[ncvarid].ncoordvars;
          for ( i = 0; i < ndims; i++ )
            {
              if ( ncvars[ncvars[ncvarid].coordvarids[i]].islon )
                ncvars[ncvarid].xvarid = ncvars[ncvarid].coordvarids[i];
              else if ( ncvars[ncvars[ncvarid].coordvarids[i]].islat )
                ncvars[ncvarid].yvarid = ncvars[ncvarid].coordvarids[i];
              else if ( ncvars[ncvars[ncvarid].coordvarids[i]].islev )
                ncvars[ncvarid].zvarid = ncvars[ncvarid].coordvarids[i];
            }
        }
    }


  setDimType(nvars, ncvars, ncdims);


  size_t vctsize = 0;
  double *vct = NULL;
  read_vct_echam(fileID, nvars, ncvars, ncdims, &vct, &vctsize);


  if ( CDI_Debug ) printNCvars(ncvars, nvars, "define_all_grids");


  define_all_grids(streamptr, vlistID, ncdims, nvars, ncvars, timedimid, uuidOfHGrid, gridfile, number_of_grid_used);



  define_all_zaxes(streamptr, vlistID, ncdims, nvars, ncvars, vctsize, vct, uuidOfVGrid);
  if ( vct ) Free(vct);



  varids = (int *) Malloc((size_t)nvars * sizeof (int));
  nvarids = 0;
  for ( ncvarid = 0; ncvarid < nvars; ncvarid++ )
    if ( ncvars[ncvarid].isvar == TRUE ) varids[nvarids++] = ncvarid;

  nvars_data = nvarids;

  if ( CDI_Debug ) Message("time varid = %d", streamptr->basetime.ncvarid);
  if ( CDI_Debug ) Message("ntsteps = %d", ntsteps);
  if ( CDI_Debug ) Message("nvars_data = %d", nvars_data);


  if ( nvars_data == 0 )
    {
      streamptr->ntsteps = 0;
      return (CDI_EUFSTRUCT);
    }

  if ( ntsteps == 0 && streamptr->basetime.ncdimid == UNDEFID && streamptr->basetime.ncvarid != UNDEFID )
    ntsteps = 1;

  streamptr->ntsteps = (long)ntsteps;


  define_all_vars(streamptr, vlistID, instID, modelID, varids, nvars_data, nvars, ncvars);


  cdiCreateTimesteps(streamptr);


  int nctimevarid = streamptr->basetime.ncvarid;

  if ( time_has_units )
    {
      taxis_t *taxis = &streamptr->tsteps[0].taxis;

      if ( setBaseTime(ncvars[nctimevarid].units, taxis) == 1 )
        {
          nctimevarid = UNDEFID;
          streamptr->basetime.ncvarid = UNDEFID;
        }

      if ( leadtime_id != UNDEFID && taxis->type == TAXIS_RELATIVE )
        {
          streamptr->basetime.leadtimeid = leadtime_id;
          taxis->type = TAXIS_FORECAST;

          int timeunit = -1;
          if ( ncvars[leadtime_id].units[0] != 0 ) timeunit = scanTimeUnit(ncvars[leadtime_id].units);
          if ( timeunit == -1 ) timeunit = taxis->unit;
          taxis->fc_unit = timeunit;

          setForecastTime(fcreftime, taxis);
        }
    }

  if ( time_has_bounds )
    {
      streamptr->tsteps[0].taxis.has_bounds = TRUE;
      if ( time_climatology ) streamptr->tsteps[0].taxis.climatology = TRUE;
    }

  if ( nctimevarid != UNDEFID )
    {
      taxis_t *taxis = &streamptr->tsteps[0].taxis;
      ptaxisDefName(taxis, ncvars[nctimevarid].name);
      if ( ncvars[nctimevarid].longname[0] )
        ptaxisDefLongname(taxis, ncvars[nctimevarid].longname);
    }

  if ( nctimevarid != UNDEFID )
    if ( ncvars[nctimevarid].calendar == TRUE )
      {
        enum {attstringlen = 8192};
        char attstring[attstringlen];

        cdfGetAttText(fileID, nctimevarid, "calendar", attstringlen, attstring);
        strtolower(attstring);

        if ( memcmp(attstring, "standard", 8) == 0 ||
             memcmp(attstring, "gregorian", 9) == 0 )
          calendar = CALENDAR_STANDARD;
        else if ( memcmp(attstring, "none", 4) == 0 )
          calendar = CALENDAR_NONE;
        else if ( memcmp(attstring, "proleptic", 9) == 0 )
          calendar = CALENDAR_PROLEPTIC;
        else if ( memcmp(attstring, "360", 3) == 0 )
          calendar = CALENDAR_360DAYS;
        else if ( memcmp(attstring, "365", 3) == 0 ||
                  memcmp(attstring, "noleap", 6) == 0 )
          calendar = CALENDAR_365DAYS;
        else if ( memcmp(attstring, "366", 3) == 0 ||
                  memcmp(attstring, "all_leap", 8) == 0 )
          calendar = CALENDAR_366DAYS;
        else
          Warning("calendar >%s< unsupported!", attstring);
      }

  if ( streamptr->tsteps[0].taxis.type == TAXIS_FORECAST )
    {
      taxisID = taxisCreate(TAXIS_FORECAST);
    }
  else if ( streamptr->tsteps[0].taxis.type == TAXIS_RELATIVE )
    {
      taxisID = taxisCreate(TAXIS_RELATIVE);
    }
  else
    {
      taxisID = taxisCreate(TAXIS_ABSOLUTE);
      if ( !time_has_units )
        {
          taxisDefTunit(taxisID, TUNIT_DAY);
          streamptr->tsteps[0].taxis.unit = TUNIT_DAY;
        }
    }


  if ( calendar == UNDEFID && streamptr->tsteps[0].taxis.type != TAXIS_ABSOLUTE )
    {
      calendar = CALENDAR_STANDARD;
    }

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5)
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wstrict-overflow"
#endif
  if ( calendar != UNDEFID )
    {
      taxis_t *taxis = &streamptr->tsteps[0].taxis;
      taxis->calendar = calendar;
      taxisDefCalendar(taxisID, calendar);
    }
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5)
#pragma GCC diagnostic pop
#endif

  vlistDefTaxis(vlistID, taxisID);

  streamptr->curTsID = 0;
  streamptr->rtsteps = 1;

  (void) cdfInqTimestep(streamptr, 0);

  cdfCreateRecords(streamptr, 0);


  Free(ncdims);


  Free(ncvars);

  return (0);
}

static
void wrf_read_timestep(int fileID, int nctimevarid, int tsID, taxis_t *taxis)
{
  size_t start[2], count[2];
  char stvalue[32];
  start[0] = (size_t) tsID; start[1] = 0;
  count[0] = 1; count[1] = 19;
  stvalue[0] = 0;
  cdf_get_vara_text(fileID, nctimevarid, start, count, stvalue);
  stvalue[19] = 0;
  {
    int year = 1, month = 1, day = 1 , hour = 0, minute = 0, second = 0;
    if ( strlen(stvalue) == 19 )
      sscanf(stvalue, "%d-%d-%d_%d:%d:%d", &year, &month, &day, &hour, &minute, &second);
    taxis->vdate = cdiEncodeDate(year, month, day);
    taxis->vtime = cdiEncodeTime(hour, minute, second);
    taxis->type = TAXIS_ABSOLUTE;
  }
}

static
double get_timevalue(int fileID, int nctimevarid, int tsID, timecache_t *tcache)
{
  double timevalue = 0;
  size_t index = (size_t) tsID;

  if ( tcache )
    {
      if ( tcache->size == 0 || (tsID < tcache->startid || tsID > (tcache->startid+tcache->size-1)) )
        {
          int maxvals = MAX_TIMECACHE_SIZE;
          tcache->startid = (tsID/MAX_TIMECACHE_SIZE)*MAX_TIMECACHE_SIZE;
          if ( (tcache->startid + maxvals) > tcache->maxvals ) maxvals = (tcache->maxvals)%MAX_TIMECACHE_SIZE;
          tcache->size = maxvals;
          index = (size_t) tcache->startid;

          for ( int ival = 0; ival < maxvals; ++ival )
            {
              cdf_get_var1_double(fileID, nctimevarid, &index, &timevalue);
              if ( timevalue >= NC_FILL_DOUBLE || timevalue < -NC_FILL_DOUBLE ) timevalue = 0;
              tcache->cache[ival] = timevalue;
              index++;
            }
        }

      timevalue = tcache->cache[tsID%MAX_TIMECACHE_SIZE];
    }
  else
    {
      cdf_get_var1_double(fileID, nctimevarid, &index, &timevalue);
      if ( timevalue >= NC_FILL_DOUBLE || timevalue < -NC_FILL_DOUBLE ) timevalue = 0;
    }

  return timevalue;
}


int cdfInqTimestep(stream_t * streamptr, int tsID)
{
  long nrecs = 0;
  double timevalue;
  int fileID;
  taxis_t *taxis;

  if ( CDI_Debug ) Message("streamID = %d  tsID = %d", streamptr->self, tsID);

  if ( tsID < 0 ) Error("unexpected tsID = %d", tsID);

  if ( tsID < streamptr->ntsteps && streamptr->ntsteps > 0 )
    {
      cdfCreateRecords(streamptr, tsID);

      taxis = &streamptr->tsteps[tsID].taxis;
      if ( tsID > 0 )
        ptaxisCopy(taxis, &streamptr->tsteps[0].taxis);

      timevalue = tsID;

      int nctimevarid = streamptr->basetime.ncvarid;
      if ( nctimevarid != UNDEFID )
        {
          fileID = streamptr->fileID;
          size_t index = (size_t)tsID;

          if ( streamptr->basetime.lwrf )
            {
              wrf_read_timestep(fileID, nctimevarid, tsID, taxis);
            }
          else
            {
#if defined (USE_TIMECACHE)
              if ( streamptr->basetime.timevar_cache == NULL )
                {
                  streamptr->basetime.timevar_cache = (timecache_t *) Malloc(MAX_TIMECACHE_SIZE*sizeof(timecache_t));
                  streamptr->basetime.timevar_cache->size = 0;
                  streamptr->basetime.timevar_cache->maxvals = streamptr->ntsteps;
                }
#endif
              timevalue = get_timevalue(fileID, nctimevarid, tsID, streamptr->basetime.timevar_cache);
              cdiDecodeTimeval(timevalue, taxis, &taxis->vdate, &taxis->vtime);
            }

          int nctimeboundsid = streamptr->basetime.ncvarboundsid;
          if ( nctimeboundsid != UNDEFID )
            {
              size_t start[2], count[2];
              start[0] = index; count[0] = 1; start[1] = 0; count[1] = 1;
              cdf_get_vara_double(fileID, nctimeboundsid, start, count, &timevalue);
              if ( timevalue >= NC_FILL_DOUBLE || timevalue < -NC_FILL_DOUBLE ) timevalue = 0;

              cdiDecodeTimeval(timevalue, taxis, &taxis->vdate_lb, &taxis->vtime_lb);

              start[0] = index; count[0] = 1; start[1] = 1; count[1] = 1;
              cdf_get_vara_double(fileID, nctimeboundsid, start, count, &timevalue);
              if ( timevalue >= NC_FILL_DOUBLE || timevalue < -NC_FILL_DOUBLE ) timevalue = 0;

              cdiDecodeTimeval(timevalue, taxis, &taxis->vdate_ub, &taxis->vtime_ub);
            }

          int leadtimeid = streamptr->basetime.leadtimeid;
          if ( leadtimeid != UNDEFID )
            {
              timevalue = get_timevalue(fileID, leadtimeid, tsID, NULL);
              cdiSetForecastPeriod(timevalue, taxis);
            }
        }
    }

  streamptr->curTsID = tsID;
  nrecs = streamptr->tsteps[tsID].nrecs;

  return ((int) nrecs);
}


void cdfDefHistory(stream_t *streamptr, int size, const char *history)
{
  int ncid = streamptr->fileID;
  cdf_put_att_text(ncid, NC_GLOBAL, "history", (size_t) size, history);
}


int cdfInqHistorySize(stream_t *streamptr)
{
  size_t size = 0;
  int ncid = streamptr->fileID;
  if ( streamptr->historyID != UNDEFID )
    cdf_inq_attlen(ncid, NC_GLOBAL, "history", &size);

  return ((int) size);
}


void cdfInqHistoryString(stream_t *streamptr, char *history)
{
  int ncid = streamptr->fileID;
  if ( streamptr->historyID != UNDEFID )
    cdf_get_att_text(ncid, NC_GLOBAL, "history", history);
}


void cdfDefVars(stream_t *streamptr)
{
  int vlistID = streamptr->vlistID;
  if ( vlistID == UNDEFID )
    Error("Internal problem! vlist undefined for streamptr %p", streamptr);

  int ngrids = vlistNgrids(vlistID);
  int nzaxis = vlistNzaxis(vlistID);



  if ( ngrids > 0 )
    for ( int index = 0; index < ngrids; index++ )
      {
        int gridID = vlistGrid(vlistID, index);
        cdfDefGrid(streamptr, gridID);
      }

  if ( nzaxis > 0 )
    for ( int index = 0; index < nzaxis; index++ )
      {
        int zaxisID = vlistZaxis(vlistID, index);
        if ( streamptr->zaxisID[index] == UNDEFID ) cdfDefZaxis(streamptr, zaxisID);
      }
}
#endif
#if defined (HAVE_CONFIG_H)
#endif



void streamDefHistory(int streamID, int length, const char *history)
{
#ifdef HAVE_LIBNETCDF
  stream_t *streamptr = stream_to_pointer(streamID);

  if ( streamptr->filetype == FILETYPE_NC ||
       streamptr->filetype == FILETYPE_NC2 ||
       streamptr->filetype == FILETYPE_NC4 ||
       streamptr->filetype == FILETYPE_NC4C )
    {
      char *histstring;
      size_t len;
      if ( history )
        {
          len = strlen(history);
          if ( len )
            {


              histstring = strdupx(history);
              cdfDefHistory(streamptr, length, histstring);
              Free(histstring);
            }
        }
    }
#else
  (void)streamID; (void)length; (void)history;
#endif
}


int streamInqHistorySize(int streamID)
{
  int size = 0;
#ifdef HAVE_LIBNETCDF
  stream_t *streamptr = stream_to_pointer(streamID);

  if ( streamptr->filetype == FILETYPE_NC ||
       streamptr->filetype == FILETYPE_NC2 ||
       streamptr->filetype == FILETYPE_NC4 ||
       streamptr->filetype == FILETYPE_NC4C )
    {
      size = cdfInqHistorySize(streamptr);
    }
#else
  (void)streamID;
#endif
  return (size);
}


void streamInqHistoryString(int streamID, char *history)
{
#ifdef HAVE_LIBNETCDF
  stream_t *streamptr = stream_to_pointer(streamID);

  if ( streamptr->filetype == FILETYPE_NC ||
       streamptr->filetype == FILETYPE_NC2 ||
       streamptr->filetype == FILETYPE_NC4 ||
       streamptr->filetype == FILETYPE_NC4C )
    {
      cdfInqHistoryString(streamptr, history);
    }
#else
  (void)streamID; (void)history;
#endif
}
#if defined (HAVE_CONFIG_H)
#endif

#include <limits.h>
#include <stdio.h>
#include <string.h>




void recordInitEntry(record_t *record)
{
  record->position = CDI_UNDEFID;
  record->size = 0;
  record->param = 0;
  record->ilevel = CDI_UNDEFID;
  record->used = FALSE;
  record->varID = CDI_UNDEFID;
  record->levelID = CDI_UNDEFID;
  memset(record->varname, 0, sizeof(record->varname));
  memset(&record->tiles, 0, sizeof(record->tiles));
}


int recordNewEntry(stream_t *streamptr, int tsID)
{
  size_t recordID = 0;
  size_t recordSize = (size_t)streamptr->tsteps[tsID].recordSize;
  record_t *records = streamptr->tsteps[tsID].records;




  if ( ! recordSize )
    {
      recordSize = 1;
      records = (record_t *) Malloc(recordSize * sizeof (record_t));

      for ( size_t i = 0; i < recordSize; i++ )
        records[i].used = CDI_UNDEFID;
    }
  else
    {
      while ( recordID < recordSize
              && records[recordID].used != CDI_UNDEFID )
        ++recordID;
    }



  if ( recordID == recordSize )
    {
      if (recordSize <= INT_MAX / 2)
        recordSize *= 2;
      else if (recordSize < INT_MAX)
        recordSize = INT_MAX;
      else
        Error("Cannot handle this many records!\n");
      records = (record_t *) Realloc(records,
                                     recordSize * sizeof (record_t));

      for ( size_t i = recordID; i < recordSize; i++ )
        records[i].used = CDI_UNDEFID;
    }

  recordInitEntry(&records[recordID]);

  records[recordID].used = 1;

  streamptr->tsteps[tsID].recordSize = (int)recordSize;
  streamptr->tsteps[tsID].records = records;

  return (int)recordID;
}

static
void cdiInitRecord(stream_t *streamptr)
{
  streamptr->record = (Record *) Malloc(sizeof(Record));

  streamptr->record->param = 0;
  streamptr->record->level = 0;
  streamptr->record->date = 0;
  streamptr->record->time = 0;
  streamptr->record->gridID = 0;
  streamptr->record->buffer = NULL;
  streamptr->record->buffersize = 0;
  streamptr->record->position = 0;
  streamptr->record->varID = 0;
  streamptr->record->levelID = CDI_UNDEFID;
}


void streamInqRecord(int streamID, int *varID, int *levelID)
{
  check_parg(varID);
  check_parg(levelID);

  stream_t *streamptr = stream_to_pointer(streamID);

  cdiDefAccesstype(streamID, TYPE_REC);

  if ( ! streamptr->record ) cdiInitRecord(streamptr);

  int tsID = streamptr->curTsID;
  int rindex = streamptr->tsteps[tsID].curRecID + 1;

  if ( rindex >= streamptr->tsteps[tsID].nrecs )
    Error("record %d not available at timestep %d", rindex+1, tsID+1);

  int recID = streamptr->tsteps[tsID].recIDs[rindex];

  if ( recID == -1 || recID >= streamptr->tsteps[tsID].nallrecs )
    Error("Internal problem! tsID = %d recID = %d", tsID, recID);

  *varID = streamptr->tsteps[tsID].records[recID].varID;
  int lindex = streamptr->tsteps[tsID].records[recID].levelID;

  int isub = subtypeInqActiveIndex(streamptr->vars[*varID].subtypeID);
  *levelID = streamptr->vars[*varID].recordTable[isub].lindex[lindex];

  if ( CDI_Debug )
    Message("tsID = %d, recID = %d, varID = %d, levelID = %d\n", tsID, recID, *varID, *levelID);

  streamptr->curTsID = tsID;
  streamptr->tsteps[tsID].curRecID = rindex;
}
void streamDefRecord(int streamID, int varID, int levelID)
{
  stream_t *streamptr = stream_to_pointer(streamID);

  int tsID = streamptr->curTsID;

  if ( tsID == CDI_UNDEFID )
    {
      tsID++;
      streamDefTimestep(streamID, tsID);
    }

  if ( ! streamptr->record ) cdiInitRecord(streamptr);

  int vlistID = streamptr->vlistID;
  int gridID = vlistInqVarGrid(vlistID, varID);
  int zaxisID = vlistInqVarZaxis(vlistID, varID);
  int param = vlistInqVarParam(vlistID, varID);
  int level = (int)(zaxisInqLevel(zaxisID, levelID));

  streamptr->record->varID = varID;
  streamptr->record->levelID = levelID;
  streamptr->record->param = param;
  streamptr->record->level = level;
  streamptr->record->date = streamptr->tsteps[tsID].taxis.vdate;
  streamptr->record->time = streamptr->tsteps[tsID].taxis.vtime;
  streamptr->record->gridID = gridID;
  streamptr->record->prec = vlistInqVarDatatype(vlistID, varID);

  switch (streamptr->filetype)
    {
#if defined (HAVE_LIBGRIB)
    case FILETYPE_GRB:
    case FILETYPE_GRB2:
      grbDefRecord(streamptr);
      break;
#endif
#if defined (HAVE_LIBSERVICE)
    case FILETYPE_SRV:
      srvDefRecord(streamptr);
      break;
#endif
#if defined (HAVE_LIBEXTRA)
    case FILETYPE_EXT:
      extDefRecord(streamptr);
      break;
#endif
#if defined (HAVE_LIBIEG)
    case FILETYPE_IEG:
      iegDefRecord(streamptr);
      break;
#endif
#if defined (HAVE_LIBNETCDF)
    case FILETYPE_NC:
    case FILETYPE_NC2:
    case FILETYPE_NC4:
    case FILETYPE_NC4C:
      if ( streamptr->accessmode == 0 ) cdfEndDef(streamptr);
      cdfDefRecord(streamptr);
      break;
#endif
    default:
      Error("%s support not compiled in!", strfiletype(streamptr->filetype));
      break;
    }
}


void streamCopyRecord(int streamID2, int streamID1)
{
  stream_t *streamptr1 = stream_to_pointer(streamID1),
    *streamptr2 = stream_to_pointer(streamID2);
  int filetype1 = streamptr1->filetype,
    filetype2 = streamptr2->filetype,
    filetype = FILETYPE_UNDEF;

  if ( filetype1 == filetype2 ) filetype = filetype2;
  else
    {
      switch (filetype1)
        {
        case FILETYPE_NC:
        case FILETYPE_NC2:
        case FILETYPE_NC4:
        case FILETYPE_NC4C:
          switch (filetype2)
            {
            case FILETYPE_NC:
            case FILETYPE_NC2:
            case FILETYPE_NC4:
            case FILETYPE_NC4C:
              Warning("Streams have different file types (%s -> %s)!", strfiletype(filetype1), strfiletype(filetype2));
              filetype = filetype2;
              break;
            }
          break;
        }
    }

  if ( filetype == FILETYPE_UNDEF )
    Error("Streams have different file types (%s -> %s)!", strfiletype(filetype1), strfiletype(filetype2));

  switch (filetype)
    {
#if defined (HAVE_LIBGRIB)
    case FILETYPE_GRB:
    case FILETYPE_GRB2:
      grbCopyRecord(streamptr2, streamptr1);
      break;
#endif
#if defined (HAVE_LIBSERVICE)
    case FILETYPE_SRV:
      srvCopyRecord(streamptr2, streamptr1);
      break;
#endif
#if defined (HAVE_LIBEXTRA)
    case FILETYPE_EXT:
      extCopyRecord(streamptr2, streamptr1);
      break;
#endif
#if defined (HAVE_LIBIEG)
    case FILETYPE_IEG:
      iegCopyRecord(streamptr2, streamptr1);
      break;
#endif
#if defined (HAVE_LIBNETCDF)
    case FILETYPE_NC:
    case FILETYPE_NC2:
    case FILETYPE_NC4:
    case FILETYPE_NC4C:
      cdfCopyRecord(streamptr2, streamptr1);
      break;
#endif
    default:
      {
        Error("%s support not compiled in!", strfiletype(filetype));
        break;
      }
    }
}


void cdi_create_records(stream_t *streamptr, int tsID)
{
  unsigned nrecords, maxrecords;
  record_t *records;

  tsteps_t *sourceTstep = streamptr->tsteps;
  tsteps_t *destTstep = sourceTstep + tsID;

  if ( destTstep->records ) return;

  int vlistID = streamptr->vlistID;

  if ( tsID == 0 )
    {
      maxrecords = 0;
      int nvars = streamptr->nvars;
      for ( int varID = 0; varID < nvars; varID++)
        for (int isub=0; isub<streamptr->vars[varID].subtypeSize; isub++)
          maxrecords += (unsigned)streamptr->vars[varID].recordTable[isub].nlevs;
    }
  else
    {
      maxrecords = (unsigned)sourceTstep->recordSize;
    }

  if ( tsID == 0 )
    {
      nrecords = maxrecords;
    }
  else if ( tsID == 1 )
    {
      nrecords = 0;
      maxrecords = (unsigned)sourceTstep->recordSize;
      for ( unsigned recID = 0; recID < maxrecords; recID++ )
        {
          int varID = sourceTstep->records[recID].varID;
          nrecords += (varID == CDI_UNDEFID
                       || vlistInqVarTsteptype(vlistID, varID) != TSTEP_CONSTANT);

        }
    }
  else
    {
      nrecords = (unsigned)streamptr->tsteps[1].nallrecs;
    }


  if ( maxrecords > 0 )
    records = (record_t *) Malloc(maxrecords*sizeof(record_t));
  else
    records = NULL;

  destTstep->records = records;
  destTstep->recordSize = (int)maxrecords;
  destTstep->nallrecs = (int)nrecords;

  if ( tsID == 0 )
    {
      for ( unsigned recID = 0; recID < maxrecords; recID++ )
        recordInitEntry(&destTstep->records[recID]);
    }
  else
    {
      memcpy(destTstep->records, sourceTstep->records, (size_t)maxrecords*sizeof(record_t));

      for ( unsigned recID = 0; recID < maxrecords; recID++ )
        {
          record_t *curRecord = &sourceTstep->records[recID];
          destTstep->records[recID].used = curRecord->used;
          if ( curRecord->used != CDI_UNDEFID && curRecord->varID != -1 )
            {
              if ( vlistInqVarTsteptype(vlistID, curRecord->varID) != TSTEP_CONSTANT )
                {
                  destTstep->records[recID].position = CDI_UNDEFID;
                  destTstep->records[recID].size = 0;
                  destTstep->records[recID].used = FALSE;
                }
            }
        }
    }
}
#if defined (HAVE_CONFIG_H)
#endif

#include <string.h>




static void streamvar_init_recordtable(stream_t *streamptr, int varID, int isub)
{
  streamptr->vars[varID].recordTable[isub].nlevs = 0;
  streamptr->vars[varID].recordTable[isub].recordID = NULL;
  streamptr->vars[varID].recordTable[isub].lindex = NULL;
}


static
void streamvar_init_entry(stream_t *streamptr, int varID)
{
  streamptr->vars[varID].ncvarid = CDI_UNDEFID;
  streamptr->vars[varID].defmiss = 0;

  streamptr->vars[varID].subtypeSize = 0;
  streamptr->vars[varID].recordTable = NULL;

  streamptr->vars[varID].gridID = CDI_UNDEFID;
  streamptr->vars[varID].zaxisID = CDI_UNDEFID;
  streamptr->vars[varID].tsteptype = CDI_UNDEFID;
  streamptr->vars[varID].subtypeID = CDI_UNDEFID;
}

static
int streamvar_new_entry(stream_t *streamptr)
{
  int varID = 0;
  int streamvarSize;
  svarinfo_t *streamvar;

  streamvarSize = streamptr->varsAllocated;
  streamvar = streamptr->vars;




  if ( ! streamvarSize )
    {
      int i;

      streamvarSize = 2;
      streamvar
        = (svarinfo_t *) Malloc((size_t)streamvarSize * sizeof(svarinfo_t));
      if ( streamvar == NULL )
        {
          Message("streamvarSize = %d", streamvarSize);
          SysError("Allocation of svarinfo_t failed");
        }

      for ( i = 0; i < streamvarSize; i++ )
        streamvar[i].isUsed = FALSE;
    }
  else
    {
      while ( varID < streamvarSize )
        {
          if ( ! streamvar[varID].isUsed ) break;
          varID++;
        }
    }



  if ( varID == streamvarSize )
    {
      int i;

      streamvarSize = 2*streamvarSize;
      streamvar
        = (svarinfo_t *) Realloc(streamvar,
                                 (size_t)streamvarSize * sizeof (svarinfo_t));
      if ( streamvar == NULL )
        {
          Message("streamvarSize = %d", streamvarSize);
          SysError("Reallocation of svarinfo_t failed");
        }
      varID = streamvarSize/2;

      for ( i = varID; i < streamvarSize; i++ )
        streamvar[i].isUsed = FALSE;
    }

  streamptr->varsAllocated = streamvarSize;
  streamptr->vars = streamvar;

  streamvar_init_entry(streamptr, varID);

  streamptr->vars[varID].isUsed = TRUE;
  return (varID);
}


static void
allocate_record_table_entry(stream_t *streamptr, int varID, int subID, int nlevs)
{
  int *level = (int *) Malloc((size_t)nlevs * sizeof (int));
  int *lindex = (int *) Malloc((size_t)nlevs * sizeof (int));

  for (int levID = 0; levID < nlevs; levID++ )
    {
      level[levID] = CDI_UNDEFID;
      lindex[levID] = levID;
    }

  streamptr->vars[varID].recordTable[subID].nlevs = nlevs;
  streamptr->vars[varID].recordTable[subID].recordID = level;
  streamptr->vars[varID].recordTable[subID].lindex = lindex;
}


int stream_new_var(stream_t *streamptr, int gridID, int zaxisID, int tilesetID)
{
  if ( CDI_Debug )
    Message("gridID = %d  zaxisID = %d", gridID, zaxisID);

  int varID = streamvar_new_entry(streamptr);
  int nlevs = zaxisInqSize(zaxisID);

  streamptr->nvars++;

  streamptr->vars[varID].gridID = gridID;
  streamptr->vars[varID].zaxisID = zaxisID;

  int nsub = 1;
  if (tilesetID != CDI_UNDEFID)
    nsub = subtypeInqSize(tilesetID);
  if ( CDI_Debug )
    Message("varID %d: create %d tiles with %d level(s), zaxisID=%d", varID, nsub, nlevs,zaxisID);
  streamptr->vars[varID].recordTable = (sleveltable_t *) Malloc((size_t)nsub * sizeof (sleveltable_t));
  if( streamptr->vars[varID].recordTable == NULL )
    SysError("Allocation of leveltable failed!");
  streamptr->vars[varID].subtypeSize = nsub;

  for (int isub=0; isub<nsub; isub++) {
    streamvar_init_recordtable(streamptr, varID, isub);
    allocate_record_table_entry(streamptr, varID, isub, nlevs);
    if ( CDI_Debug )
      Message("streamptr->vars[varID].recordTable[isub].recordID[0]=%d",
              streamptr->vars[varID].recordTable[isub].recordID[0]);
  }

  streamptr->vars[varID].subtypeID = tilesetID;

  return (varID);
}
#if defined (HAVE_CONFIG_H)
#endif

#ifdef HAVE_LIBNETCDF



#undef UNDEFID
#define UNDEFID CDI_UNDEFID


void cdfDefVarDeflate(int ncid, int ncvarid, int deflate_level)
{
#if defined (HAVE_NETCDF4)
  int retval;

  int shuffle = 1;
  int deflate = 1;

  if ( deflate_level < 1 || deflate_level > 9 ) deflate_level = 1;

  if ((retval = nc_def_var_deflate(ncid, ncvarid, shuffle, deflate, deflate_level)))
    {
      Error("nc_def_var_deflate failed, status = %d", retval);
    }
#else

  static int lwarn = TRUE;
  if ( lwarn )
    {
      lwarn = FALSE;
      Warning("Deflate compression failed, netCDF4 not available!");
    }
#endif
}

static
int cdfDefDatatype(int datatype, int filetype)
{
  int xtype = NC_FLOAT;

  if ( datatype == DATATYPE_CPX32 || datatype == DATATYPE_CPX64 )
    Error("CDI/netCDF library does not support complex numbers!");

  if ( filetype == FILETYPE_NC4 )
    {
      if ( datatype == DATATYPE_INT8 ) xtype = NC_BYTE;
      else if ( datatype == DATATYPE_INT16 ) xtype = NC_SHORT;
      else if ( datatype == DATATYPE_INT32 ) xtype = NC_INT;
#if defined (HAVE_NETCDF4)
      else if ( datatype == DATATYPE_UINT8 ) xtype = NC_UBYTE;
      else if ( datatype == DATATYPE_UINT16 ) xtype = NC_USHORT;
      else if ( datatype == DATATYPE_UINT32 ) xtype = NC_UINT;
#else
      else if ( datatype == DATATYPE_UINT8 ) xtype = NC_SHORT;
      else if ( datatype == DATATYPE_UINT16 ) xtype = NC_INT;
      else if ( datatype == DATATYPE_UINT32 ) xtype = NC_INT;
#endif
      else if ( datatype == DATATYPE_FLT64 ) xtype = NC_DOUBLE;
      else xtype = NC_FLOAT;
    }
  else
    {
      if ( datatype == DATATYPE_INT8 ) xtype = NC_BYTE;
      else if ( datatype == DATATYPE_INT16 ) xtype = NC_SHORT;
      else if ( datatype == DATATYPE_INT32 ) xtype = NC_INT;
      else if ( datatype == DATATYPE_UINT8 ) xtype = NC_SHORT;
      else if ( datatype == DATATYPE_UINT16 ) xtype = NC_INT;
      else if ( datatype == DATATYPE_UINT32 ) xtype = NC_INT;
      else if ( datatype == DATATYPE_FLT64 ) xtype = NC_DOUBLE;
      else xtype = NC_FLOAT;
    }

  return xtype;
}

static
void cdfDefVarMissval(stream_t *streamptr, int varID, int dtype, int lcheck)
{
  if ( streamptr->vars[varID].defmiss == FALSE )
    {
      int vlistID = streamptr->vlistID;
      int fileID = streamptr->fileID;
      int ncvarid = streamptr->vars[varID].ncvarid;
      double missval = vlistInqVarMissval(vlistID, varID);

      if ( lcheck && streamptr->ncmode == 2 ) cdf_redef(fileID);

      int xtype = cdfDefDatatype(dtype, streamptr->filetype);

      if ( xtype == NC_BYTE && missval > 127 && missval < 256 ) xtype = NC_INT;

      cdf_put_att_double(fileID, ncvarid, "_FillValue", (nc_type) xtype, 1, &missval);
      cdf_put_att_double(fileID, ncvarid, "missing_value", (nc_type) xtype, 1, &missval);

      if ( lcheck && streamptr->ncmode == 2 ) cdf_enddef(fileID);

      streamptr->vars[varID].defmiss = TRUE;
    }
}

static
void cdfDefInstitut(stream_t *streamptr)
{
  int vlistID = streamptr->vlistID;
  int fileID = streamptr->fileID;
  int instID = vlistInqInstitut(vlistID);

  if ( instID != UNDEFID )
    {
      const char *longname = institutInqLongnamePtr(instID);
      if ( longname )
        {
          size_t len = strlen(longname);
          if ( len > 0 )
            {
              if ( streamptr->ncmode == 2 ) cdf_redef(fileID);
              cdf_put_att_text(fileID, NC_GLOBAL, "institution", len, longname);
              if ( streamptr->ncmode == 2 ) cdf_enddef(fileID);
            }
        }
    }
}

static
void cdfDefSource(stream_t *streamptr)
{
  int vlistID = streamptr->vlistID;
  int fileID = streamptr->fileID;
  int modelID = vlistInqModel(vlistID);

  if ( modelID != UNDEFID )
    {
      const char *longname = modelInqNamePtr(modelID);
      if ( longname )
        {
          size_t len = strlen(longname);
          if ( len > 0 )
            {
              if ( streamptr->ncmode == 2 ) cdf_redef(fileID);
              cdf_put_att_text(fileID, NC_GLOBAL, "source", len, longname);
              if ( streamptr->ncmode == 2 ) cdf_enddef(fileID);
            }
        }
    }
}

static inline
void *resizeBuf(void **buf, size_t *bufSize, size_t reqSize)
{
  if (reqSize > *bufSize)
    {
      *buf = Realloc(*buf, reqSize);
      *bufSize = reqSize;
    }
  return *buf;
}

static
void defineAttributes(int vlistID, int varID, int fileID, int ncvarID)
{
  int atttype, attlen;
  size_t len;
  char attname[CDI_MAX_NAME+1];
  void *attBuf = NULL;
  size_t attBufSize = 0;

  int natts;
  vlistInqNatts(vlistID, varID, &natts);

  for ( int iatt = 0; iatt < natts; iatt++ )
    {
      vlistInqAtt(vlistID, varID, iatt, attname, &atttype, &attlen);

      if ( attlen == 0 ) continue;

      if ( atttype == DATATYPE_TXT )
        {
          size_t attSize = (size_t)attlen*sizeof(char);
          char *atttxt = (char *)resizeBuf(&attBuf, &attBufSize, attSize);
          vlistInqAttTxt(vlistID, varID, attname, attlen, atttxt);
          len = (size_t)attlen;
          cdf_put_att_text(fileID, ncvarID, attname, len, atttxt);
        }
      else if ( atttype == DATATYPE_INT16 || atttype == DATATYPE_INT32 )
        {
          size_t attSize = (size_t)attlen*sizeof(int);
          int *attint = (int *)resizeBuf(&attBuf, &attBufSize, attSize);
          vlistInqAttInt(vlistID, varID, attname, attlen, &attint[0]);
          len = (size_t)attlen;
          cdf_put_att_int(fileID, ncvarID, attname, atttype == DATATYPE_INT16 ? NC_SHORT : NC_INT, len, attint);
        }
      else if ( atttype == DATATYPE_FLT32 || atttype == DATATYPE_FLT64 )
        {
          size_t attSize = (size_t)attlen * sizeof(double);
          double *attflt = (double *)resizeBuf(&attBuf, &attBufSize, attSize);
          vlistInqAttFlt(vlistID, varID, attname, attlen, attflt);
          len = (size_t)attlen;
          if ( atttype == DATATYPE_FLT32 )
            {
              float attflt_sp[len];
              for ( size_t i = 0; i < len; ++i ) attflt_sp[i] = (float)attflt[i];
              cdf_put_att_float(fileID, ncvarID, attname, NC_FLOAT, len, attflt_sp);
            }
          else
            cdf_put_att_double(fileID, ncvarID, attname, NC_DOUBLE, len, attflt);
        }
    }

  Free(attBuf);
}

static
void cdfDefGlobalAtts(stream_t *streamptr)
{
  if ( streamptr->globalatts ) return;

  int vlistID = streamptr->vlistID;
  int fileID = streamptr->fileID;

  cdfDefSource(streamptr);
  cdfDefInstitut(streamptr);

  int natts;
  vlistInqNatts(vlistID, CDI_GLOBAL, &natts);

  if ( natts > 0 && streamptr->ncmode == 2 ) cdf_redef(fileID);

  defineAttributes(vlistID, CDI_GLOBAL, fileID, NC_GLOBAL);

  if ( natts > 0 && streamptr->ncmode == 2 ) cdf_enddef(fileID);

  streamptr->globalatts = 1;
}

static
void cdfDefLocalAtts(stream_t *streamptr)
{
  int vlistID = streamptr->vlistID;
  int fileID = streamptr->fileID;

  if ( streamptr->localatts ) return;
  if ( vlistInqInstitut(vlistID) != UNDEFID ) return;

  streamptr->localatts = 1;

  if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

  for ( int varID = 0; varID < streamptr->nvars; varID++ )
    {
      int instID = vlistInqVarInstitut(vlistID, varID);
      if ( instID != UNDEFID )
        {
          int ncvarid = streamptr->vars[varID].ncvarid;
          const char *name = institutInqNamePtr(instID);
          if ( name )
            {
              size_t len = strlen(name);
              cdf_put_att_text(fileID, ncvarid, "institution", len, name);
            }
        }
      }

  if ( streamptr->ncmode == 2 ) cdf_enddef(fileID);
}

static
int cdfDefVar(stream_t *streamptr, int varID)
{
  int ncvarid = -1;
  int xid = UNDEFID, yid = UNDEFID;
  size_t xsize = 0, ysize = 0;
  char varname[CDI_MAX_NAME];
  int dims[4];
  int lchunk = FALSE;
  size_t chunks[4] = {0,0,0,0};
  int ndims = 0;
  int tablenum;
  int dimorder[3];
  size_t iax = 0;
  char axis[5];
  int ensID, ensCount, forecast_type;
#if defined (HAVE_NETCDF4)
  int retval;
#endif

  int fileID = streamptr->fileID;

  if ( CDI_Debug )
    Message("streamID = %d, fileID = %d, varID = %d", streamptr->self, fileID, varID);

  if ( streamptr->vars[varID].ncvarid != UNDEFID )
    return streamptr->vars[varID].ncvarid;

  int vlistID = streamptr->vlistID;
  int gridID = vlistInqVarGrid(vlistID, varID);
  int zaxisID = vlistInqVarZaxis(vlistID, varID);
  int tsteptype = vlistInqVarTsteptype(vlistID, varID);
  int code = vlistInqVarCode(vlistID, varID);
  int param = vlistInqVarParam(vlistID, varID);
  int pnum, pcat, pdis;
  cdiDecodeParam(param, &pnum, &pcat, &pdis);

  int chunktype = vlistInqVarChunkType(vlistID, varID);

  vlistInqVarDimorder(vlistID, varID, &dimorder);

  int gridsize = gridInqSize(gridID);
  if ( gridsize > 1 ) lchunk = TRUE;
  int gridtype = gridInqType(gridID);
  int gridindex = vlistGridIndex(vlistID, gridID);
  if ( gridtype != GRID_TRAJECTORY )
    {
      xid = streamptr->xdimID[gridindex];
      yid = streamptr->ydimID[gridindex];
      if ( xid != UNDEFID ) cdf_inq_dimlen(fileID, xid, &xsize);
      if ( yid != UNDEFID ) cdf_inq_dimlen(fileID, yid, &ysize);
    }

  int zaxisindex = vlistZaxisIndex(vlistID, zaxisID);
  int zid = streamptr->zaxisID[zaxisindex];
  int zaxis_is_scalar = FALSE;
  if ( zid == UNDEFID ) zaxis_is_scalar = zaxisInqScalar(zaxisID);

  if ( dimorder[0] != 3 ) lchunk = FALSE;

  if ( ((dimorder[0]>0)+(dimorder[1]>0)+(dimorder[2]>0)) < ((xid!=UNDEFID)+(yid!=UNDEFID)+(zid!=UNDEFID)) )
    {
      printf("zid=%d  yid=%d  xid=%d\n", zid, yid, xid);
      Error("Internal problem, dimension order missing!");
    }

  int tid = streamptr->basetime.ncdimid;

  if ( tsteptype != TSTEP_CONSTANT )
    {
      if ( tid == UNDEFID ) Error("Internal problem, time undefined!");
      chunks[ndims] = 1;
      dims[ndims++] = tid;
      axis[iax++] = 'T';
    }
  for ( int id = 0; id < 3; ++id )
    {
      if ( dimorder[id] == 3 && zid != UNDEFID )
        {
          axis[iax++] = 'Z';
          chunks[ndims] = 1;
          dims[ndims] = zid;
          ndims++;
        }
      else if ( dimorder[id] == 2 && yid != UNDEFID )
        {
          if ( chunktype == CHUNK_LINES )
            chunks[ndims] = 1;
          else
            chunks[ndims] = ysize;
          dims[ndims] = yid;
          ndims++;
        }
      else if ( dimorder[id] == 1 && xid != UNDEFID )
        {
          chunks[ndims] = xsize;
          dims[ndims] = xid;
          ndims++;
        }
    }

  if ( CDI_Debug )
    fprintf(stderr, "chunktype %d  chunks %d %d %d %d\n", chunktype, (int)chunks[0], (int)chunks[1], (int)chunks[2], (int)chunks[3]);

  int tableID = vlistInqVarTable(vlistID, varID);

  const char *name = vlistInqVarNamePtr(vlistID, varID);
  const char *longname = vlistInqVarLongnamePtr(vlistID, varID);
  const char *stdname = vlistInqVarStdnamePtr(vlistID, varID);
  const char *units = vlistInqVarUnitsPtr(vlistID, varID);

  if ( name == NULL ) name = tableInqParNamePtr(tableID, code);
  if ( longname == NULL ) longname = tableInqParLongnamePtr(tableID, code);
  if ( units == NULL ) units = tableInqParUnitsPtr(tableID, code);
  if ( name )
    {
      int checkname;
      int iz;
      int status;

      sprintf(varname, "%s", name);

      checkname = TRUE;
      iz = 0;

      while ( checkname )
        {
          if ( iz ) sprintf(varname, "%s_%d", name, iz+1);

          status = nc_inq_varid(fileID, varname, &ncvarid);
          if ( status != NC_NOERR )
            {
              checkname = FALSE;
            }

          if ( checkname ) iz++;

          if ( iz >= CDI_MAX_NAME ) Error("Double entry of variable name '%s'!", name);
        }

      if ( strcmp(name, varname) != 0 )
        {
          if ( iz == 1 )
            Warning("Changed double entry of variable name '%s' to '%s'!", name, varname);
          else
            Warning("Changed multiple entry of variable name '%s' to '%s'!", name, varname);
        }

      name = varname;
    }
  else
    {
      if ( code < 0 ) code = -code;
      if ( pnum < 0 ) pnum = -pnum;

      if ( pdis == 255 )
        sprintf(varname, "var%d", code);
      else
        sprintf(varname, "param%d.%d.%d", pnum, pcat, pdis);

      char *varname2 = varname+strlen(varname);

      int checkname = TRUE;
      int iz = 0;

      while ( checkname )
        {
          if ( iz ) sprintf(varname2, "_%d", iz+1);

          int status = nc_inq_varid(fileID, varname, &ncvarid);
          if ( status != NC_NOERR ) checkname = FALSE;

          if ( checkname ) iz++;

          if ( iz >= CDI_MAX_NAME ) break;
        }

      name = varname;
      code = 0;
      pdis = 255;
    }



  int dtype = vlistInqVarDatatype(vlistID, varID);
  int xtype = cdfDefDatatype(dtype, streamptr->filetype);

  cdf_def_var(fileID, name, (nc_type) xtype, ndims, dims, &ncvarid);

#if defined (HAVE_NETCDF4)
  if ( lchunk && (streamptr->filetype == FILETYPE_NC4 || streamptr->filetype == FILETYPE_NC4C) )
    {
      if ( chunktype == CHUNK_AUTO )
        retval = nc_def_var_chunking(fileID, ncvarid, NC_CHUNKED, NULL);
      else
        retval = nc_def_var_chunking(fileID, ncvarid, NC_CHUNKED, chunks);

      if ( retval ) Error("nc_def_var_chunking failed, status = %d", retval);
    }
#endif

  if ( streamptr->comptype == COMPRESS_ZIP )
    {
      if ( lchunk && (streamptr->filetype == FILETYPE_NC4 || streamptr->filetype == FILETYPE_NC4C) )
        {
          cdfDefVarDeflate(fileID, ncvarid, streamptr->complevel);
        }
      else
        {
          if ( lchunk )
            {
              static int lwarn = TRUE;

              if ( lwarn )
                {
                  lwarn = FALSE;
                  Warning("Deflate compression is only available for netCDF4!");
                }
            }
        }
    }

  if ( streamptr->comptype == COMPRESS_SZIP )
    {
      if ( lchunk && (streamptr->filetype == FILETYPE_NC4 || streamptr->filetype == FILETYPE_NC4C) )
        {
#if defined (NC_SZIP_NN_OPTION_MASK)
          cdfDefVarSzip(fileID, ncvarid);
#else
          static int lwarn = TRUE;

          if ( lwarn )
            {
              lwarn = FALSE;
              Warning("netCDF4/SZIP compression not available!");
            }
#endif
        }
      else
        {
          static int lwarn = TRUE;

          if ( lwarn )
            {
              lwarn = FALSE;
              Warning("SZIP compression is only available for netCDF4!");
            }
        }
    }

  if ( stdname && *stdname )
    cdf_put_att_text(fileID, ncvarid, "standard_name", strlen(stdname), stdname);

  if ( longname && *longname )
    cdf_put_att_text(fileID, ncvarid, "long_name", strlen(longname), longname);

  if ( units && *units )
    cdf_put_att_text(fileID, ncvarid, "units", strlen(units), units);

  if ( code > 0 && pdis == 255 )
    cdf_put_att_int(fileID, ncvarid, "code", NC_INT, 1, &code);

  if ( pdis != 255 )
    {
      char paramstr[32];
      cdiParamToString(param, paramstr, sizeof(paramstr));
      cdf_put_att_text(fileID, ncvarid, "param", strlen(paramstr), paramstr);
    }

  if ( tableID != UNDEFID )
    {
      tablenum = tableInqNum(tableID);
      if ( tablenum > 0 )
        cdf_put_att_int(fileID, ncvarid, "table", NC_INT, 1, &tablenum);
    }

  char coordinates[CDI_MAX_NAME];
  coordinates[0] = 0;

  if ( zaxis_is_scalar )
    {
      int nczvarID = streamptr->nczvarID[zaxisindex];
      if ( nczvarID != CDI_UNDEFID )
        {
          size_t len = strlen(coordinates);
          if ( len ) coordinates[len++] = ' ';
          cdf_inq_varname(fileID, nczvarID, coordinates+len);
        }
    }

  if ( gridtype != GRID_GENERIC && gridtype != GRID_LONLAT && gridtype != GRID_CURVILINEAR )
    {
      size_t len = strlen(gridNamePtr(gridtype));
      if ( len > 0 )
        cdf_put_att_text(fileID, ncvarid, "grid_type", len, gridNamePtr(gridtype));
    }

  if ( gridIsRotated(gridID) )
    {
      char mapping[] = "rotated_pole";
      cdf_put_att_text(fileID, ncvarid, "grid_mapping", strlen(mapping), mapping);
    }

  if ( gridtype == GRID_SINUSOIDAL )
    {
      char mapping[] = "sinusoidal";
      cdf_put_att_text(fileID, ncvarid, "grid_mapping", strlen(mapping), mapping);
    }
  else if ( gridtype == GRID_LAEA )
    {
      char mapping[] = "laea";
      cdf_put_att_text(fileID, ncvarid, "grid_mapping", strlen(mapping), mapping);
    }
  else if ( gridtype == GRID_LCC2 )
    {
      char mapping[] = "Lambert_Conformal";
      cdf_put_att_text(fileID, ncvarid, "grid_mapping", strlen(mapping), mapping);
    }
  else if ( gridtype == GRID_TRAJECTORY )
    {
      cdf_put_att_text(fileID, ncvarid, "coordinates", 9, "tlon tlat" );
    }
  else if ( gridtype == GRID_LONLAT && xid == UNDEFID && yid == UNDEFID && gridsize == 1 )
    {
      int ncxvarID = streamptr->ncxvarID[gridindex];
      int ncyvarID = streamptr->ncyvarID[gridindex];
      if ( ncyvarID != CDI_UNDEFID )
        {
          size_t len = strlen(coordinates);
          if ( len ) coordinates[len++] = ' ';
          cdf_inq_varname(fileID, ncyvarID, coordinates+len);
        }
      if ( ncxvarID != CDI_UNDEFID )
        {
          size_t len = strlen(coordinates);
          if ( len ) coordinates[len++] = ' ';
          cdf_inq_varname(fileID, ncxvarID, coordinates+len);
        }
    }
  else if ( gridtype == GRID_UNSTRUCTURED || gridtype == GRID_CURVILINEAR )
    {
      char cellarea[CDI_MAX_NAME] = "area: ";
      int ncxvarID = streamptr->ncxvarID[gridindex];
      int ncyvarID = streamptr->ncyvarID[gridindex];
      int ncavarID = streamptr->ncavarID[gridindex];
      if ( ncyvarID != CDI_UNDEFID )
        {
          size_t len = strlen(coordinates);
          if ( len ) coordinates[len++] = ' ';
          cdf_inq_varname(fileID, ncyvarID, coordinates+len);
        }
      if ( ncxvarID != CDI_UNDEFID )
        {
          size_t len = strlen(coordinates);
          if ( len ) coordinates[len++] = ' ';
          cdf_inq_varname(fileID, ncxvarID, coordinates+len);
        }

      if ( ncavarID != CDI_UNDEFID )
        {
          size_t len = strlen(cellarea);
          cdf_inq_varname(fileID, ncavarID, cellarea+len);
          len = strlen(cellarea);
          cdf_put_att_text(fileID, ncvarid, "cell_measures", len, cellarea);
        }

      if ( gridtype == GRID_UNSTRUCTURED )
        {
          int position = gridInqPosition(gridID);
          if ( position > 0 )
            cdf_put_att_int(fileID, ncvarid, "number_of_grid_in_reference", NC_INT, 1, &position);
        }
    }
  else if ( gridtype == GRID_SPECTRAL || gridtype == GRID_FOURIER )
    {
      int gridTruncation = gridInqTrunc(gridID);
      axis[iax++] = '-';
      axis[iax++] = '-';
      cdf_put_att_text(fileID, ncvarid, "axis", iax, axis);
      cdf_put_att_int(fileID, ncvarid, "truncation", NC_INT, 1, &gridTruncation);
    }

  size_t len = strlen(coordinates);
  if ( len ) cdf_put_att_text(fileID, ncvarid, "coordinates", len, coordinates);


    {
      int laddoffset, lscalefactor;
      double addoffset, scalefactor;
      int astype = NC_DOUBLE;

      addoffset = vlistInqVarAddoffset(vlistID, varID);
      scalefactor = vlistInqVarScalefactor(vlistID, varID);
      laddoffset = IS_NOT_EQUAL(addoffset, 0);
      lscalefactor = IS_NOT_EQUAL(scalefactor, 1);

      if ( laddoffset || lscalefactor )
        {
          if ( IS_EQUAL(addoffset, (double) ((float) addoffset)) &&
               IS_EQUAL(scalefactor, (double) ((float) scalefactor)) )
            {
              astype = NC_FLOAT;
            }

          if ( xtype == (int) NC_FLOAT ) astype = NC_FLOAT;

          cdf_put_att_double(fileID, ncvarid, "add_offset", (nc_type) astype, 1, &addoffset);
          cdf_put_att_double(fileID, ncvarid, "scale_factor", (nc_type) astype, 1, &scalefactor);
        }
    }

  if ( dtype == DATATYPE_UINT8 && xtype == NC_BYTE )
    {
      int validrange[2] = {0, 255};
      cdf_put_att_int(fileID, ncvarid, "valid_range", NC_SHORT, 2, validrange);
      cdf_put_att_text(fileID, ncvarid, "_Unsigned", 4, "true");
    }

  streamptr->vars[varID].ncvarid = ncvarid;

  if ( vlistInqVarMissvalUsed(vlistID, varID) )
    cdfDefVarMissval(streamptr, varID, vlistInqVarDatatype(vlistID, varID), 0);

  if ( zid == -1 )
    {
      if ( zaxisInqType(zaxisID) == ZAXIS_CLOUD_BASE ||
           zaxisInqType(zaxisID) == ZAXIS_CLOUD_TOP ||
           zaxisInqType(zaxisID) == ZAXIS_ISOTHERM_ZERO ||
           zaxisInqType(zaxisID) == ZAXIS_TOA ||
           zaxisInqType(zaxisID) == ZAXIS_SEA_BOTTOM ||
           zaxisInqType(zaxisID) == ZAXIS_LAKE_BOTTOM ||
           zaxisInqType(zaxisID) == ZAXIS_SEDIMENT_BOTTOM ||
           zaxisInqType(zaxisID) == ZAXIS_SEDIMENT_BOTTOM_TA ||
           zaxisInqType(zaxisID) == ZAXIS_SEDIMENT_BOTTOM_TW ||
           zaxisInqType(zaxisID) == ZAXIS_MIX_LAYER ||
           zaxisInqType(zaxisID) == ZAXIS_ATMOSPHERE )
        {
          zaxisInqName(zaxisID, varname);
          cdf_put_att_text(fileID, ncvarid, "level_type", strlen(varname), varname);
        }
    }

  if ( vlistInqVarEnsemble( vlistID, varID, &ensID, &ensCount, &forecast_type ) )
    {



        cdf_put_att_int(fileID, ncvarid, "realization", NC_INT, 1, &ensID);
        cdf_put_att_int(fileID, ncvarid, "ensemble_members", NC_INT, 1, &ensCount);
        cdf_put_att_int(fileID, ncvarid, "forecast_init_type", NC_INT, 1, &forecast_type);

#ifdef DBG
        if( DBG )
          {
            fprintf( stderr, "cdfDefVar :\n EnsID  %d\n Enscount %d\n Forecast init type %d\n", ensID,
                     ensCount, forecast_type );
          }
#endif
    }


  defineAttributes(vlistID, varID, fileID, ncvarid);



  return ncvarid;
}


void cdfEndDef(stream_t *streamptr)
{
  cdfDefGlobalAtts(streamptr);
  cdfDefLocalAtts(streamptr);

  if ( streamptr->accessmode == 0 )
    {
      int fileID = streamptr->fileID;
      if ( streamptr->ncmode == 2 ) cdf_redef(fileID);

      int nvars = streamptr->nvars;
      for ( int varID = 0; varID < nvars; varID++ )
        cdfDefVar(streamptr, varID);

      if ( streamptr->ncmode == 2 )
        {
          if ( CDI_netcdf_hdr_pad == 0UL )
            cdf_enddef(fileID);
          else
            cdf__enddef(fileID, CDI_netcdf_hdr_pad);
        }

      streamptr->accessmode = 1;
    }
}

#endif
#if defined (HAVE_CONFIG_H)
#endif

#ifdef HAVE_LIBNETCDF

#include <limits.h>
#include <float.h>



#undef UNDEFID
#define UNDEFID CDI_UNDEFID


static
void cdfReadGridTraj(stream_t *streamptr, int gridID)
{
  int vlistID = streamptr->vlistID;
  int fileID = streamptr->fileID;

  int gridindex = vlistGridIndex(vlistID, gridID);
  int lonID = streamptr->xdimID[gridindex];
  int latID = streamptr->ydimID[gridindex];

  int tsID = streamptr->curTsID;
  size_t index = (size_t)tsID;

  double xlon, xlat;
  cdf_get_var1_double(fileID, lonID, &index, &xlon);
  cdf_get_var1_double(fileID, latID, &index, &xlat);

  gridDefXvals(gridID, &xlon);
  gridDefYvals(gridID, &xlat);
}

static
void cdfGetSlapDescription(stream_t *streamptr, int varID, size_t (*start)[4], size_t (*count)[4])
{
  int vlistID = streamptr->vlistID;
  int tsID = streamptr->curTsID;
  int gridID = vlistInqVarGrid(vlistID, varID);
  int zaxisID = vlistInqVarZaxis(vlistID, varID);
  int tsteptype = vlistInqVarTsteptype(vlistID, varID);
  int gridindex = vlistGridIndex(vlistID, gridID);

  if ( CDI_Debug ) Message("tsID = %d", tsID);

  int xid = UNDEFID, yid = UNDEFID;
  if ( gridInqType(gridID) == GRID_TRAJECTORY )
    {
      cdfReadGridTraj(streamptr, gridID);
    }
  else
    {
      xid = streamptr->xdimID[gridindex];
      yid = streamptr->ydimID[gridindex];
    }
  int zaxisindex = vlistZaxisIndex(vlistID, zaxisID);
  int zid = streamptr->zaxisID[zaxisindex];

  int ndims = 0;
#define addDimension(startCoord,length) do \
    { \
      (*start)[ndims] = startCoord; \
      (*count)[ndims] = length; \
      ndims++; \
    } while(0)
  if ( tsteptype != TSTEP_CONSTANT ) addDimension((size_t)tsID, 1);
  if ( zid != UNDEFID ) addDimension(0, (size_t)zaxisInqSize(zaxisID));
  if ( yid != UNDEFID ) addDimension(0, (size_t)gridInqYsize(gridID));
  if ( xid != UNDEFID ) addDimension(0, (size_t)gridInqXsize(gridID));
#undef addDimension

  assert(ndims <= (int)(sizeof(*start)/sizeof(**start)));
  assert(ndims <= (int)(sizeof(*count)/sizeof(**count)));

  if ( CDI_Debug )
    for (int idim = 0; idim < ndims; idim++)
      Message("dim = %d  start = %d  count = %d", idim, start[idim], count[idim]);
}



static
size_t cdfDoInputDataTransformationDP(size_t valueCount, double *data, bool haveMissVal, double missVal, double scaleFactor, double offset, double validMin, double validMax)
 {
  const bool haveOffset = IS_NOT_EQUAL(offset, 0);
  const bool haveScaleFactor = IS_NOT_EQUAL(scaleFactor, 1);
  size_t missValCount = 0;

  if ( IS_EQUAL(validMin, VALIDMISS) ) validMin = DBL_MIN;
  if ( IS_EQUAL(validMax, VALIDMISS) ) validMax = DBL_MAX;

  bool haveRangeCheck = (IS_NOT_EQUAL(validMax, DBL_MAX)) | (IS_NOT_EQUAL(validMin,DBL_MIN));
  assert(!haveRangeCheck || haveMissVal);

  switch ((int)haveMissVal | ((int)haveScaleFactor << 1)
          | ((int)haveOffset << 2) | ((int)haveRangeCheck << 3))
    {
    case 15:
      for ( size_t i = 0; i < valueCount; i++ )
        {
          int outOfRange = data[i] < validMin || data[i] > validMax;
          int isMissVal = DBL_IS_EQUAL(data[i], missVal);
          missValCount += (size_t)(outOfRange | isMissVal);
          data[i] = outOfRange ? missVal
            : isMissVal ? data[i] : data[i] * scaleFactor + offset;
        }
      break;
    case 13:
      for ( size_t i = 0; i < valueCount; i++ )
        {
          int outOfRange = data[i] < validMin || data[i] > validMax;
          int isMissVal = DBL_IS_EQUAL(data[i], missVal);
          missValCount += (size_t)(outOfRange | isMissVal);
          data[i] = outOfRange ? missVal
            : isMissVal ? data[i] : data[i] + offset;
        }
      break;
    case 11:
      for ( size_t i = 0; i < valueCount; i++ )
        {
          int outOfRange = data[i] < validMin || data[i] > validMax;
          int isMissVal = DBL_IS_EQUAL(data[i], missVal);
          missValCount += (size_t)(outOfRange | isMissVal);
          data[i] = outOfRange ? missVal
            : isMissVal ? data[i] : data[i] * scaleFactor;
        }
      break;
    case 9:
      for ( size_t i = 0; i < valueCount; i++ )
        {
          int outOfRange = data[i] < validMin || data[i] > validMax;
          int isMissVal = DBL_IS_EQUAL(data[i], missVal);
          missValCount += (size_t)(outOfRange | isMissVal);
          data[i] = outOfRange ? missVal : data[i];
        }
      break;
    case 7:
      for ( size_t i = 0; i < valueCount; i++ )
        if ( DBL_IS_EQUAL(data[i], missVal) )
          missValCount++;
        else
          data[i] = data[i] * scaleFactor + offset;
      break;
    case 6:
      for ( size_t i = 0; i < valueCount; i++ )
        data[i] = data[i] * scaleFactor + offset;
      break;
    case 5:
      for ( size_t i = 0; i < valueCount; i++ )
        if ( DBL_IS_EQUAL(data[i], missVal) )
          missValCount++;
        else
          data[i] += offset;
      break;
    case 4:
      for ( size_t i = 0; i < valueCount; i++ )
        data[i] += offset;
      break;
    case 3:
      for ( size_t i = 0; i < valueCount; i++ )
        if ( DBL_IS_EQUAL(data[i], missVal) )
          missValCount++;
        else
          data[i] *= scaleFactor;
      break;
    case 2:
      for ( size_t i = 0; i < valueCount; i++ )
        data[i] *= scaleFactor;
      break;
    case 1:
      for ( size_t i = 0; i < valueCount; i++ )
        missValCount += (unsigned)DBL_IS_EQUAL(data[i], missVal);
      break;
    }

  return missValCount;
}

static
size_t cdfDoInputDataTransformationSP(size_t valueCount, float *data, bool haveMissVal, double missVal, double scaleFactor, double offset, double validMin, double validMax)
 {
  const bool haveOffset = IS_NOT_EQUAL(offset, 0);
  const bool haveScaleFactor = IS_NOT_EQUAL(scaleFactor, 1);
  size_t missValCount = 0;

  if ( IS_EQUAL(validMin, VALIDMISS) ) validMin = DBL_MIN;
  if ( IS_EQUAL(validMax, VALIDMISS) ) validMax = DBL_MAX;

  bool haveRangeCheck = (IS_NOT_EQUAL(validMax, DBL_MAX)) | (IS_NOT_EQUAL(validMin,DBL_MIN));
  assert(!haveRangeCheck || haveMissVal);

  switch ((int)haveMissVal | ((int)haveScaleFactor << 1)
          | ((int)haveOffset << 2) | ((int)haveRangeCheck << 3))
    {
    case 15:
      for ( size_t i = 0; i < valueCount; i++ )
        {
          int outOfRange = data[i] < validMin || data[i] > validMax;
          int isMissVal = DBL_IS_EQUAL(data[i], missVal);
          missValCount += (size_t)(outOfRange | isMissVal);
          data[i] = outOfRange ? (float)missVal
            : isMissVal ? data[i] : (float)(data[i] * scaleFactor + offset);
        }
      break;
    case 13:
      for ( size_t i = 0; i < valueCount; i++ )
        {
          int outOfRange = data[i] < validMin || data[i] > validMax;
          int isMissVal = DBL_IS_EQUAL(data[i], missVal);
          missValCount += (size_t)(outOfRange | isMissVal);
          data[i] = outOfRange ? (float)missVal
            : isMissVal ? data[i] : (float)(data[i] + offset);
        }
      break;
    case 11:
      for ( size_t i = 0; i < valueCount; i++ )
        {
          int outOfRange = data[i] < validMin || data[i] > validMax;
          int isMissVal = DBL_IS_EQUAL(data[i], missVal);
          missValCount += (size_t)(outOfRange | isMissVal);
          data[i] = outOfRange ? (float)missVal
            : isMissVal ? data[i] : (float)(data[i] * scaleFactor);
        }
      break;
    case 9:
      for ( size_t i = 0; i < valueCount; i++ )
        {
          int outOfRange = data[i] < validMin || data[i] > validMax;
          int isMissVal = DBL_IS_EQUAL(data[i], missVal);
          missValCount += (size_t)(outOfRange | isMissVal);
          data[i] = outOfRange ? (float)missVal : data[i];
        }
      break;
    case 7:
      for ( size_t i = 0; i < valueCount; i++ )
        if ( DBL_IS_EQUAL(data[i], missVal) )
          missValCount++;
        else
          data[i] = (float)(data[i] * scaleFactor + offset);
      break;
    case 6:
      for ( size_t i = 0; i < valueCount; i++ )
        data[i] = (float)(data[i] * scaleFactor + offset);
      break;
    case 5:
      for ( size_t i = 0; i < valueCount; i++ )
        if ( DBL_IS_EQUAL(data[i], missVal) )
          missValCount++;
        else
          data[i] = (float)(data[i] + offset);
      break;
    case 4:
      for ( size_t i = 0; i < valueCount; i++ )
        data[i] = (float)(data[i] + offset);
      break;
    case 3:
      for ( size_t i = 0; i < valueCount; i++ )
        if ( DBL_IS_EQUAL(data[i], missVal) )
          missValCount++;
        else
          data[i] = (float)(data[i] * scaleFactor);
      break;
    case 2:
      for ( size_t i = 0; i < valueCount; i++ )
        data[i] = (float)(data[i] * scaleFactor);
      break;
    case 1:
      for ( size_t i = 0; i < valueCount; i++ )
        missValCount += (unsigned)DBL_IS_EQUAL(data[i], missVal);
      break;
    }

  return missValCount;
}

static
size_t min_size(size_t a, size_t b)
{
  return a < b ? a : b;
}

static
void transpose2dArrayDP(size_t inWidth, size_t inHeight, double *data)
{
  const size_t cacheBlockSize = 256;

#ifdef __cplusplus
  double *out[inHeight];
  double *temp[inWidth];
  temp[0] = (double *) Malloc(inHeight*inWidth*sizeof(double));
  memcpy(temp[0], data, inHeight*inWidth*sizeof(double));
  for(int i = 0; i < inHeight; i++) out[i] = data + (inWidth*i);
  for(int i = 1; i < inWidth; i++) temp[i] = temp[0] + (inHeight*i);
#else
  double (*out)[inHeight] = (double (*)[inHeight])data;
  double (*temp)[inWidth] = (double (*)[inWidth]) Malloc(inHeight*sizeof(*temp));
  memcpy(temp, data, inHeight*sizeof(*temp));
#endif







  for ( size_t yBlock = 0; yBlock < inHeight; yBlock += cacheBlockSize )
    {
      for ( size_t xBlock = 0; xBlock < inWidth; xBlock += cacheBlockSize )
        {
          for ( size_t y = yBlock, yEnd = min_size(yBlock + cacheBlockSize, inHeight); y < yEnd; y++ )
            {
              for ( size_t x = xBlock, xEnd = min_size(xBlock + cacheBlockSize, inWidth); x < xEnd; x++ )
                {
                  out[x][y] = temp[y][x];
                }
            }
        }
    }

  Free(temp[0]);
}

static
void transpose2dArraySP(size_t inWidth, size_t inHeight, float *data)
{
  const size_t cacheBlockSize = 256;

#ifdef __cplusplus
  float *out[inHeight];
  float *temp[inWidth];
  temp[0] = (float *) Malloc(inHeight*inWidth*sizeof(float));
  memcpy(temp[0], data, inHeight*inWidth*sizeof(float));
  for(int i = 0; i < inHeight; i++) out[i] = data + (inWidth*i);
  for(int i = 1; i < inWidth; i++) temp[i] = temp[0] + (inHeight*i);
#else
  float (*out)[inHeight] = (float (*)[inHeight])data;
  float (*temp)[inWidth] = (float (*)[inWidth]) Malloc(inHeight*sizeof(*temp));
  memcpy(temp, data, inHeight*sizeof(*temp));
#endif







  for ( size_t yBlock = 0; yBlock < inHeight; yBlock += cacheBlockSize )
    {
      for ( size_t xBlock = 0; xBlock < inWidth; xBlock += cacheBlockSize )
        {
          for ( size_t y = yBlock, yEnd = min_size(yBlock + cacheBlockSize, inHeight); y < yEnd; y++ )
            {
              for ( size_t x = xBlock, xEnd = min_size(xBlock + cacheBlockSize, inWidth); x < xEnd; x++ )
                {
                  out[x][y] = temp[y][x];
                }
            }
        }
    }

  Free(temp);
}

static
void cdfInqDimIds(stream_t *streamptr, int varId, int (*outDimIds)[3])
{
  int gridId = vlistInqVarGrid(streamptr->vlistID, varId);
  int gridindex = vlistGridIndex(streamptr->vlistID, gridId);

  (*outDimIds)[0] = (*outDimIds)[1] = (*outDimIds)[2] = UNDEFID;
  switch ( gridInqType(gridId) )
    {
      case GRID_TRAJECTORY:
        cdfReadGridTraj(streamptr, gridId);
        break;

      case GRID_UNSTRUCTURED:
        (*outDimIds)[0] = streamptr->xdimID[gridindex];
        break;

      default:
        (*outDimIds)[0] = streamptr->xdimID[gridindex];
        (*outDimIds)[1] = streamptr->ydimID[gridindex];
        break;
    }

  int zaxisID = vlistInqVarZaxis(streamptr->vlistID, varId);
  int zaxisindex = vlistZaxisIndex(streamptr->vlistID, zaxisID);
  (*outDimIds)[2] = streamptr->zaxisID[zaxisindex];
}

static
int cdfGetSkipDim(int fileId, int ncvarid, int (*dimIds)[3])
{
  if((*dimIds)[0] != UNDEFID) return 0;
  if((*dimIds)[1] != UNDEFID) return 0;
  int nvdims;
  cdf_inq_varndims(fileId, ncvarid, &nvdims);
  if(nvdims != 3) return 0;

  int varDimIds[3];
  cdf_inq_vardimid(fileId, ncvarid, varDimIds);
  size_t size = 0;
  if ( (*dimIds)[2] == varDimIds[2] )
    {
      cdf_inq_dimlen(fileId, varDimIds[1], &size);
      if ( size == 1 ) return 1;
    }
  else if ( (*dimIds)[2] == varDimIds[1] )
    {
      cdf_inq_dimlen(fileId, varDimIds[2], &size);
      if ( size == 1 ) return 2;
    }
  return 0;
}

static
void cdfGetSliceSlapDescription(stream_t *streamptr, int varId, int levelId, bool *outSwapXY, size_t (*start)[4], size_t (*count)[4])
{
  int tsID = streamptr->curTsID;
  if ( CDI_Debug ) Message("tsID = %d", tsID);

  int fileId = streamptr->fileID;
  int vlistId = streamptr->vlistID;
  int ncvarid = streamptr->vars[varId].ncvarid;

  int gridId = vlistInqVarGrid(vlistId, varId);
  int tsteptype = vlistInqVarTsteptype(vlistId, varId);
  int gridsize = gridInqSize(gridId);

  streamptr->numvals += gridsize;

  int dimIds[3];
  cdfInqDimIds(streamptr, varId, &dimIds);

  int skipdim = cdfGetSkipDim(fileId, ncvarid, &dimIds);

  int dimorder[3];
  vlistInqVarDimorder(vlistId, varId, &dimorder);

  *outSwapXY = (dimorder[2] == 2 || dimorder[0] == 1) && dimIds[0] != UNDEFID && dimIds[1] != UNDEFID ;

  int ndims = 0;

#define addDimension(startIndex,extent) do { \
      (*start)[ndims] = startIndex; \
      (*count)[ndims] = extent; \
      ndims++; \
  } while(0)

  if ( tsteptype != TSTEP_CONSTANT ) addDimension((size_t)tsID, 1);
  if ( skipdim == 1 ) addDimension(0, 1);

  for ( int id = 0; id < 3; ++id )
    {
      size_t size;
      int curDimId = dimIds[dimorder[id]-1];
      if ( curDimId == UNDEFID ) continue;
      switch ( dimorder[id] )
        {
          Error("Internal errror: Malformed dimension order encountered. Please report this bug.\n");
          case 1:
          case 2:
            cdf_inq_dimlen(fileId, curDimId, &size);
            addDimension(0, size);
            break;

          case 3:
            addDimension((size_t)levelId, 1);
            break;

          default:
            Error("Internal errror: Malformed dimension order encountered. Please report this bug.\n");
        }
    }

  if ( skipdim == 2 ) addDimension(0, 1);

  assert(ndims <= (int)(sizeof(*start)/sizeof(**start)));
  assert(ndims <= (int)(sizeof(*count)/sizeof(**count)));

#undef addDimension





  int nvdims;
  cdf_inq_varndims(fileId, ncvarid, &nvdims);

  if ( nvdims != ndims )
    Error("Internal error, variable %s has an unsupported array structure!", vlistInqVarNamePtr(vlistId, varId));
}


void cdfReadVarDP(stream_t *streamptr, int varID, double *data, int *nmiss)
{
  if ( CDI_Debug ) Message("streamID = %d  varID = %d", streamptr->self, varID);

  int vlistID = streamptr->vlistID;
  int fileID = streamptr->fileID;

  int ncvarid = streamptr->vars[varID].ncvarid;

  int gridID = vlistInqVarGrid(vlistID, varID);
  int zaxisID = vlistInqVarZaxis(vlistID, varID);

  size_t start[4];
  size_t count[4];
  cdfGetSlapDescription(streamptr, varID, &start, &count);

  cdf_get_vara_double(fileID, ncvarid, start, count, data);

  size_t size = (size_t)gridInqSize(gridID)*(size_t)zaxisInqSize(zaxisID);
  double missval = vlistInqVarMissval(vlistID, varID);
  const bool haveMissVal = vlistInqVarMissvalUsed(vlistID, varID);
  double validRange[2];
  if (!(haveMissVal && vlistInqVarValidrange(vlistID, varID, validRange)))
    validRange[0] = DBL_MIN, validRange[1] = DBL_MAX;
  double addoffset = vlistInqVarAddoffset(vlistID, varID);
  double scalefactor = vlistInqVarScalefactor(vlistID, varID);
  size_t nmiss_ = cdfDoInputDataTransformationDP(size, data, haveMissVal, missval, scalefactor, addoffset, validRange[0], validRange[1]);
  assert(nmiss_ <= INT_MAX);
  *nmiss = (int)nmiss_;
}


void cdfReadVarSP(stream_t *streamptr, int varID, float *data, int *nmiss)
{
  if ( CDI_Debug ) Message("streamID = %d  varID = %d", streamptr->self, varID);

  int vlistID = streamptr->vlistID;
  int fileID = streamptr->fileID;

  int ncvarid = streamptr->vars[varID].ncvarid;

  int gridID = vlistInqVarGrid(vlistID, varID);
  int zaxisID = vlistInqVarZaxis(vlistID, varID);

  size_t start[4];
  size_t count[4];
  cdfGetSlapDescription(streamptr, varID, &start, &count);

  cdf_get_vara_float(fileID, ncvarid, start, count, data);

  size_t size = (size_t)gridInqSize(gridID) * (size_t)zaxisInqSize(zaxisID);
  double missval = vlistInqVarMissval(vlistID, varID);
  const bool haveMissVal = vlistInqVarMissvalUsed(vlistID, varID);
  double validRange[2];
  if (!(haveMissVal && vlistInqVarValidrange(vlistID, varID, validRange)))
    validRange[0] = DBL_MIN, validRange[1] = DBL_MAX;
  double addoffset = vlistInqVarAddoffset(vlistID, varID);
  double scalefactor = vlistInqVarScalefactor(vlistID, varID);
  size_t nmiss_ = cdfDoInputDataTransformationSP(size, data, haveMissVal, missval, scalefactor, addoffset, validRange[0], validRange[1]);
  assert(nmiss_ <= INT_MAX);
  *nmiss = (int)nmiss_;
}


void cdfReadVarSliceDP(stream_t *streamptr, int varID, int levelID, double *data, int *nmiss)
{
  size_t start[4];
  size_t count[4];

  if ( CDI_Debug )
    Message("streamID = %d  varID = %d  levelID = %d", streamptr->self, varID, levelID);

  int vlistID = streamptr->vlistID;
  int fileID = streamptr->fileID;

  bool swapxy;
  cdfGetSliceSlapDescription(streamptr, varID, levelID, &swapxy, &start, &count);

  int ncvarid = streamptr->vars[varID].ncvarid;
  int gridId = vlistInqVarGrid(vlistID, varID);
  size_t gridsize = (size_t)gridInqSize(gridId);
  size_t xsize = (size_t)gridInqXsize(gridId);
  size_t ysize = (size_t)gridInqYsize(gridId);

  if ( vlistInqVarDatatype(vlistID, varID) == DATATYPE_FLT32 )
    {
      float *data_fp = (float *) Malloc(gridsize*sizeof(*data_fp));
      cdf_get_vara_float(fileID, ncvarid, start, count, data_fp);
      for ( size_t i = 0; i < gridsize; i++ )
        data[i] = (double) data_fp[i];
      Free(data_fp);
    }
  else if ( vlistInqVarDatatype(vlistID, varID) == DATATYPE_UINT8 )
    {
      nc_type xtype;
      cdf_inq_vartype(fileID, ncvarid, &xtype);
      if ( xtype == NC_BYTE )
        {
          for ( size_t i = 0; i < gridsize; i++ )
            if ( data[i] < 0 ) data[i] += 256;
        }
    }
  else
    {
      cdf_get_vara_double(fileID, ncvarid, start, count, data);
    }

  if ( swapxy ) transpose2dArrayDP(ysize, xsize, data);

  double missval = vlistInqVarMissval(vlistID, varID);
  const bool haveMissVal = vlistInqVarMissvalUsed(vlistID, varID);
  double validRange[2];
  if (!(haveMissVal && vlistInqVarValidrange(vlistID, varID, validRange)))
    validRange[0] = DBL_MIN, validRange[1] = DBL_MAX;
  double addoffset = vlistInqVarAddoffset(vlistID, varID);
  double scalefactor = vlistInqVarScalefactor(vlistID, varID);
  size_t nmiss_ = cdfDoInputDataTransformationDP(gridsize, data, haveMissVal, missval, scalefactor, addoffset, validRange[0], validRange[1]);
  assert(nmiss_ <= INT_MAX);
  *nmiss = (int)nmiss_;
}


void cdfReadVarSliceSP(stream_t *streamptr, int varID, int levelID, float *data, int *nmiss)
{
  size_t start[4];
  size_t count[4];

  if ( CDI_Debug )
    Message("streamID = %d  varID = %d  levelID = %d", streamptr->self, varID, levelID);

  int vlistID = streamptr->vlistID;
  int fileID = streamptr->fileID;

  bool swapxy;
  cdfGetSliceSlapDescription(streamptr, varID, levelID, &swapxy, &start, &count);

  int ncvarid = streamptr->vars[varID].ncvarid;
  int gridId = vlistInqVarGrid(vlistID, varID);
  size_t gridsize = (size_t)gridInqSize(gridId);
  size_t xsize = (size_t)gridInqXsize(gridId);
  size_t ysize = (size_t)gridInqYsize(gridId);

  if ( vlistInqVarDatatype(vlistID, varID) == DATATYPE_FLT64 )
    {
      double *data_dp = (double *) Malloc(gridsize*sizeof(*data_dp));
      cdf_get_vara_double(fileID, ncvarid, start, count, data_dp);
      for ( size_t i = 0; i < gridsize; i++ )
        data[i] = (float) data_dp[i];
      Free(data_dp);
    }
  else if ( vlistInqVarDatatype(vlistID, varID) == DATATYPE_UINT8 )
    {
      nc_type xtype;
      cdf_inq_vartype(fileID, ncvarid, &xtype);
      if ( xtype == NC_BYTE )
        {
          for ( size_t i = 0; i < gridsize; i++ )
            if ( data[i] < 0 ) data[i] += 256;
        }
    }
  else
    {
      cdf_get_vara_float(fileID, ncvarid, start, count, data);
    }

  if ( swapxy ) transpose2dArraySP(ysize, xsize, data);

  double missval = vlistInqVarMissval(vlistID, varID);
  bool haveMissVal = vlistInqVarMissvalUsed(vlistID, varID);
  double validRange[2];
  if (!(haveMissVal && vlistInqVarValidrange(vlistID, varID, validRange)))
    validRange[0] = DBL_MIN, validRange[1] = DBL_MAX;
  double addoffset = vlistInqVarAddoffset(vlistID, varID);
  double scalefactor = vlistInqVarScalefactor(vlistID, varID);
  size_t nmiss_ = cdfDoInputDataTransformationSP(gridsize, data, haveMissVal, missval, scalefactor, addoffset, validRange[0], validRange[1]);
  assert(nmiss_ <= INT_MAX);
  *nmiss = (int)nmiss_;
}


void cdf_read_record(stream_t *streamptr, int memtype, void *data, int *nmiss)
{
  if ( CDI_Debug ) Message("streamID = %d", streamptr->self);

  int tsID = streamptr->curTsID;
  int vrecID = streamptr->tsteps[tsID].curRecID;
  int recID = streamptr->tsteps[tsID].recIDs[vrecID];
  int varID = streamptr->tsteps[tsID].records[recID].varID;
  int levelID = streamptr->tsteps[tsID].records[recID].levelID;

  if ( memtype == MEMTYPE_DOUBLE )
    cdfReadVarSliceDP(streamptr, varID, levelID, data, nmiss);
  else
    cdfReadVarSliceSP(streamptr, varID, levelID, data, nmiss);
}

#endif
#ifndef _SUBTYPE_H
#define _SUBTYPE_H


enum {

  SUBTYPE_ATT_TILEINDEX = 0,
  SUBTYPE_ATT_TOTALNO_OF_TILEATTR_PAIRS = 1,
  SUBTYPE_ATT_TILE_CLASSIFICATION = 2,
  SUBTYPE_ATT_NUMBER_OF_TILES = 3,
  SUBTYPE_ATT_NUMBER_OF_ATTR = 4,
  SUBTYPE_ATT_TILEATTRIBUTE = 5,


  nSubtypeAttributes
};




extern const char * const cdiSubtypeAttributeName[];





struct subtype_attr_t {
  int key, val;
  struct subtype_attr_t* next;
};




struct subtype_entry_t {
  int self;
  struct subtype_entry_t *next;


  struct subtype_attr_t *atts;
};





typedef struct {
  int self;
  int subtype;
  int nentries;

  struct subtype_entry_t globals;


  struct subtype_entry_t *entries;


  int active_subtype_index;
} subtype_t;





void subtypeAllocate(subtype_t **subtype_ptr2, int subtype);
int subtypePush(subtype_t *subtype_ptr);
void subtypeDestroyPtr(void *ptr);
void subtypeDuplicate(subtype_t *subtype_ptr, subtype_t **dst);
struct subtype_entry_t* subtypeEntryInsert(subtype_t* head);


void subtypePrint(int subtypeID);
void subtypePrintPtr(subtype_t* subtype_ptr);
void subtypeDefGlobalDataP(subtype_t *subtype_ptr, int key, int val);
void subtypeDefGlobalData(int subtypeID, int key, int val);
int subtypeGetGlobalData(int subtypeID, int key);
int subtypeGetGlobalDataP(subtype_t *subtype_ptr, int key);
int subtypeComparePtr(int s1_ID, subtype_t *s2);


void subtypeDefEntryDataP(struct subtype_entry_t *subtype_entry_ptr, int key, int val);



void tilesetInsertP(subtype_t *s1, subtype_t *s2);



int vlistDefTileSubtype(int vlistID, subtype_t *tiles);


int vlistInsertTrivialTileSubtype(int vlistID);


#endif
#if defined (HAVE_CONFIG_H)
#endif




static const char* subtypeName[] = {
  "tileset"
};

const char * const cdiSubtypeAttributeName[] = {
  "tileIndex",
  "totalNumberOfTileAttributePairs",
  "tileClassification",
  "numberOfTiles",
  "numberOfTileAttributes",
  "tileAttribute"
};



static int subtypeCompareP (subtype_t *z1, subtype_t *z2);
static void subtypeDestroyP ( void * subtype_ptr );
static void subtypePrintP ( void * subtype_ptr, FILE * fp );
static int subtypeGetPackSize ( void * subtype_ptr, void *context);
static void subtypePack ( void * subtype_ptr, void * buffer, int size, int *pos, void *context);
static int subtypeTxCode ( void );

static const resOps subtypeOps = {
  (int (*) (void *, void *)) subtypeCompareP,
  (void (*)(void *)) subtypeDestroyP,
  (void (*)(void *, FILE *)) subtypePrintP,
  (int (*) (void *, void *)) subtypeGetPackSize,
                             subtypePack,
                             subtypeTxCode
};

enum {
  atts_differ = 1,
};
static int attribute_to_index(const char *key)
{
  if (key == NULL) Error("Internal error!");
  for (int i=0; i<nSubtypeAttributes; i++)
    if ( strcmp(key, cdiSubtypeAttributeName[i]) == 0 ) return i;
  return -1;
}
static struct subtype_attr_t* subtypeAttrNewList(struct subtype_entry_t* head, int key, int val)
{
  if (head == NULL) Error("Internal error!");
  struct subtype_attr_t *ptr = (struct subtype_attr_t*) Malloc(sizeof(struct subtype_attr_t));
  if(NULL == ptr) Error("Node creation failed");
  ptr->key = key;
  ptr->val = val;
  ptr->next = NULL;

  head->atts = ptr;
  return ptr;
}
static struct subtype_attr_t* subtypeAttrInsert(struct subtype_entry_t* head, int key, int val)
{
  if (head == NULL) Error("Internal error!");
  if (head->atts == NULL) return (subtypeAttrNewList(head, key, val));


  struct subtype_attr_t* ptr = (struct subtype_attr_t*) Malloc(sizeof(struct subtype_attr_t));
  if(NULL == ptr) Error("Node creation failed");

  ptr->key = key;
  ptr->val = val;
  ptr->next = NULL;


  if (head->atts->key >= key) {

    ptr->next = head->atts;
    head->atts = ptr;
  } else {
    struct subtype_attr_t** predec = &head->atts;
    while (((*predec)->next != NULL) && ((*predec)->next->key < key)) {
      predec = &((*predec)->next);
    }
    ptr->next = (*predec)->next;
    (*predec)->next = ptr;
  }
  return ptr;
}



static void subtypeAttrDestroy(struct subtype_attr_t* head)
{
  if (head == NULL) return;
  subtypeAttrDestroy(head->next);
  Free(head);
  head = NULL;
}




static struct subtype_attr_t* subtypeAttrFind(struct subtype_attr_t* head, int key)
{
  if (head == NULL)
    return NULL;
  else if (head->key == key)
    return head;
  else
    return subtypeAttrFind(head->next, key);
}





static int subtypeAttsCompare(struct subtype_attr_t *a1, struct subtype_attr_t *a2)
{
  if ((a1 == NULL) && (a2 == NULL))
    return 0;
  else if ((a1 == NULL) && (a2 != NULL))
    {
      return atts_differ;
    }
  else if ((a1 != NULL) && (a2 == NULL))
    {
      return atts_differ;
    }

  if (a1->key != a2->key)
    {
      return atts_differ;
    }
  if (a1->val != a2->val)
    return atts_differ;

  return subtypeAttsCompare(a1->next, a2->next);
}



static void subtypeAttsDuplicate(struct subtype_attr_t *a1, struct subtype_entry_t* dst)
{
  if (a1 == NULL) return;

  subtypeAttsDuplicate(a1->next, dst);
  (void) subtypeAttrInsert(dst, a1->key, a1->val);
}
static struct subtype_entry_t* subtypeEntryNewList(subtype_t* head)
{
  struct subtype_entry_t *ptr = (struct subtype_entry_t*) Malloc(sizeof(struct subtype_entry_t));
  if(NULL == ptr) Error("Node creation failed");
  ptr->atts = NULL;
  ptr->next = NULL;
  head->entries = ptr;
  head->nentries = 0;
  ptr->self = head->nentries++;
  return ptr;
}
struct subtype_entry_t* subtypeEntryInsert(subtype_t* head)
{
  if (head == NULL) Error("Internal error!");
  if (head->entries == NULL) return (subtypeEntryNewList(head));


  struct subtype_entry_t* ptr = (struct subtype_entry_t*) Malloc(sizeof(struct subtype_entry_t));
  if(NULL == ptr) Error("Node creation failed");

  ptr->atts = NULL;
  ptr->self = head->nentries++;


  if (head->entries->self >= ptr->self) {

    ptr->next = head->entries;
    head->entries = ptr;
  } else {
    struct subtype_entry_t** predec = &head->entries;
    while (((*predec)->next != NULL) && ((*predec)->next->self < ptr->self)) {
      predec = &((*predec)->next);
    }
    ptr->next = (*predec)->next;
    (*predec)->next = ptr;
  }
  return ptr;
}
static struct subtype_entry_t* subtypeEntryAppend(subtype_t* head)
{
  if (head == NULL) Error("Internal error!");
  if (head->entries == NULL) return (subtypeEntryNewList(head));


  struct subtype_entry_t* ptr = (struct subtype_entry_t*) Malloc(sizeof(struct subtype_entry_t));
  if(NULL == ptr) Error("Node creation failed");

  ptr->atts = NULL;
  ptr->next = NULL;
  ptr->self = head->nentries++;


  struct subtype_entry_t* prec_ptr = head->entries;
  while (prec_ptr->next != NULL)
    prec_ptr = prec_ptr->next;

  prec_ptr->next = ptr;
  return ptr;
}



static void subtypeEntryDestroy(struct subtype_entry_t *entry)
{
  if (entry == NULL) return;
  subtypeEntryDestroy(entry->next);
  subtypeAttrDestroy(entry->atts);
  Free(entry);
  entry = NULL;
}



static int subtypeEntryCompare(struct subtype_entry_t *e1, struct subtype_entry_t *e2)
{
  if (e1 == NULL) Error("Internal error!");
  if (e2 == NULL) Error("Internal error!");
  return
    (e1->self == e2->self) &&
    subtypeAttsCompare(e1->atts, e2->atts);
}



static void subtypeEntryDuplicate(struct subtype_entry_t *a1, subtype_t* dst)
{
  if (a1 == NULL) return;

  struct subtype_entry_t *ptr = subtypeEntryAppend(dst);

  subtypeAttsDuplicate(a1->atts, ptr);
  ptr->self = a1->self;

  subtypeEntryDuplicate(a1->next, dst);
}
static void subtypePrintKernel(subtype_t *subtype_ptr, FILE *fp)
{
  if (subtype_ptr == NULL) Error("Internal error!");
  fprintf(fp, "# %s (subtype ID %d)\n", subtypeName[subtype_ptr->subtype], subtype_ptr->self);

  struct subtype_attr_t* ptr = subtype_ptr->globals.atts;
  if (ptr != NULL) fprintf(fp, "#\n# global attributes:\n");
  while (ptr != NULL) {
    fprintf(fp, "#   %-40s   (%2d) : %d\n", cdiSubtypeAttributeName[ptr->key], ptr->key, ptr->val);
    ptr = ptr->next;
  }

  fprintf(fp, "# %d local entries:\n", subtype_ptr->nentries);
  struct subtype_entry_t *entry = subtype_ptr->entries;
  while (entry != NULL) {
    fprintf(fp, "# subtype entry %d\n", entry->self);
    ptr = entry->atts;
    if (ptr != NULL) fprintf(fp, "#   attributes:\n");
    while (ptr != NULL) {
      fprintf(fp, "#     %-40s (%2d) : %d\n", cdiSubtypeAttributeName[ptr->key], ptr->key, ptr->val);
      ptr = ptr->next;
    }
    entry = entry->next;
  }
  fprintf(fp, "\n");
}




static int subtypeCompareP(subtype_t *s1, subtype_t *s2)
{
  xassert(s1 && s2);
  if (s1->subtype != s2->subtype) return atts_differ;
  if (subtypeEntryCompare(&s1->globals, &s2->globals) != 0) return atts_differ;

  struct subtype_entry_t *entry1 = s1->entries;
  struct subtype_entry_t *entry2 = s2->entries;
  while ((entry1 != NULL) && (entry2 != NULL)) {
    if (subtypeEntryCompare(entry1, entry2) != 0) return atts_differ;
    entry1 = entry1->next;
    entry2 = entry2->next;
  }

  if ((entry1 != NULL) || (entry2 != NULL)) return atts_differ;
  return 0;
}



static void subtypeDestroyP(void *ptr)
{
  subtype_t *subtype_ptr = (subtype_t*) ptr;

  subtypeAttrDestroy(subtype_ptr->globals.atts);

  subtypeEntryDestroy(subtype_ptr->entries);
  subtype_ptr->entries = NULL;
  Free(subtype_ptr);
  subtype_ptr = NULL;
}



void subtypeDestroyPtr(void *ptr)
{
  subtypeDestroyP(ptr);
}



int subtypeComparePtr(int s1_ID, subtype_t *s2)
{
  subtype_t *subtype_ptr = (subtype_t *)reshGetVal(s1_ID, &subtypeOps);
  if (subtype_ptr == NULL) Error("Internal error");
  return subtypeCompareP(subtype_ptr,s2);
}




static void subtypePrintP(void * subtype_ptr, FILE * fp)
{ subtypePrintKernel((subtype_t *)subtype_ptr, fp); }




void subtypePrintPtr(subtype_t* subtype_ptr)
{
  subtypePrintKernel(subtype_ptr, stdout);
}



static void subtypeDefaultValue(subtype_t *subtype_ptr)
{
  if (subtype_ptr == NULL) Error("Internal error!");
  subtype_ptr->self = CDI_UNDEFID;
  subtype_ptr->nentries = 0;
  subtype_ptr->entries = NULL;
  subtype_ptr->globals.atts = NULL;
  subtype_ptr->globals.next = NULL;
  subtype_ptr->globals.self = -1;
  subtype_ptr->active_subtype_index = 0;
}


void subtypeAllocate(subtype_t **subtype_ptr2, int subtype)
{

  (*subtype_ptr2) = (subtype_t *) Malloc(sizeof(subtype_t));
  subtype_t* subtype_ptr = *subtype_ptr2;
  subtypeDefaultValue(subtype_ptr);
  subtype_ptr->subtype = subtype;
  subtype_ptr->self = CDI_UNDEFID;
}



void subtypeDuplicate(subtype_t *subtype_ptr, subtype_t **dst_ptr)
{
  if (subtype_ptr == NULL) Error("Internal error!");
  subtypeAllocate(dst_ptr, subtype_ptr->subtype);
  subtype_t *dst = (*dst_ptr);

  subtypeAttsDuplicate(subtype_ptr->globals.atts, &dst->globals);
  dst->globals.self = subtype_ptr->globals.self;

  subtypeEntryDuplicate( subtype_ptr->entries, dst);
}



int subtypePush(subtype_t *subtype_ptr)
{
  if (subtype_ptr == NULL) Error("Internal error!");
  subtype_ptr->self = reshPut(subtype_ptr, &subtypeOps);
  return subtype_ptr->self;
}






void subtypeDefGlobalDataP(subtype_t *subtype_ptr, int key, int val)
{
  if (subtype_ptr == NULL) Error("Internal error!");

  struct subtype_attr_t* att_ptr = subtypeAttrFind(subtype_ptr->globals.atts, key);
  if (att_ptr == NULL)
    subtypeAttrInsert(&subtype_ptr->globals, key, val);
  else
    att_ptr->val = val;
}





void subtypeDefGlobalData(int subtypeID, int key, int val)
{
  subtype_t *subtype_ptr = (subtype_t *)reshGetVal(subtypeID, &subtypeOps);
  subtypeDefGlobalDataP(subtype_ptr, key, val);
}




int subtypeGetGlobalDataP(subtype_t *subtype_ptr, int key)
{
  if (subtype_ptr == NULL) Error("Internal error!");

  struct subtype_attr_t* att_ptr = subtypeAttrFind(subtype_ptr->globals.atts, key);
  if (att_ptr == NULL)
    return -1;
  else
    return att_ptr->val;
}




int subtypeGetGlobalData(int subtypeID, int key)
{
  subtype_t *subtype_ptr = (subtype_t *)reshGetVal(subtypeID, &subtypeOps);
  return subtypeGetGlobalDataP(subtype_ptr, key);
}





void subtypeDefEntryDataP(struct subtype_entry_t *subtype_entry_ptr, int key, int val)
{
  if (subtype_entry_ptr == NULL) Error("Internal error!");

  struct subtype_attr_t* att_ptr = subtypeAttrFind(subtype_entry_ptr->atts, key);
  if (att_ptr == NULL)
    subtypeAttrInsert(subtype_entry_ptr, key, val);
  else
    att_ptr->val = val;
}
subtype_query_t keyValuePair(const char* key, int value)
{
  subtype_query_t result;
  result.nAND = 1;
  result.key_value_pairs[0][0] = attribute_to_index(key);
  result.key_value_pairs[1][0] = value;
  if (CDI_Debug) {
    Message("key  %s matches %d", key, result.key_value_pairs[0][0]);
    Message("%d --?-- %d", result.key_value_pairs[0][0], result.key_value_pairs[1][0]);
  }
  return result;
}




subtype_query_t matchAND(subtype_query_t q1, subtype_query_t q2)
{
  if ((q1.nAND + q2.nAND) > MAX_KV_PAIRS_MATCH) Error("Internal error");
  subtype_query_t result;
  result.nAND = q1.nAND;
  for (int i=0; i<q1.nAND; i++)
    {
      result.key_value_pairs[0][i] = q1.key_value_pairs[0][i];
      result.key_value_pairs[1][i] = q1.key_value_pairs[1][i];
    }
  for (int i=0; i<q2.nAND; i++)
    {
      result.key_value_pairs[0][result.nAND] = q2.key_value_pairs[0][i];
      result.key_value_pairs[1][result.nAND] = q2.key_value_pairs[1][i];
      result.nAND++;
    }

  if (CDI_Debug) {
    Message("combined criterion:");
    for (int i=0; i<result.nAND; i++)
      Message("%d --?-- %d", result.key_value_pairs[0][i], result.key_value_pairs[1][i]);
  }
  return result;
}
void tilesetInsertP(subtype_t *s1, subtype_t *s2)
{
  if (s1 == NULL) Error("Internal error!");
  if (s2 == NULL) Error("Internal error!");
  struct subtype_entry_t
    *entry1 = s1->entries,
    *entry2 = s2->entries;
  struct subtype_attr_t *att_ptr2;



  if (subtypeAttsCompare(s1->globals.atts, s2->globals.atts) != atts_differ)
    {
      while (entry1 != NULL) {
        int found = 1;
        entry2 = s2->entries;
        while (entry2 != NULL) {
          found &= (subtypeAttsCompare(entry1->atts, entry2->atts) != atts_differ);
          entry2 = entry2->next;
        }
        if (found)
          {
            return;
          }
        entry1 = entry1->next;
      }

      entry2 = s2->entries;
      while (entry2 != NULL) {
        entry1 = subtypeEntryInsert(s1);

        att_ptr2 = entry2->atts;
        while (att_ptr2 != NULL) {
          (void) subtypeAttrInsert(entry1, att_ptr2->key, att_ptr2->val);
          att_ptr2 = att_ptr2->next;
        }
        entry2 = entry2->next;
      }
    }
  else
    {
      fprintf(stderr, "\n# SUBTYPE A:\n");
      subtypePrintKernel(s1, stderr);
      fprintf(stderr, "\n# SUBTYPE B:\n");
      subtypePrintKernel(s2, stderr);
      Error("Attempting to insert subtype entry into subtype with different global attributes!");
    }
}
int subtypeCreate(int subtype)
{
  if ( CDI_Debug ) Message("subtype: %d ", subtype);
  Message("subtype: %d ", subtype);


  subtype_t *subtype_ptr;
  subtypeAllocate(&subtype_ptr, subtype);

  return subtypePush(subtype_ptr);
}



void subtypePrint(int subtypeID)
{
  subtype_t *subtype_ptr = (subtype_t *)reshGetVal(subtypeID, &subtypeOps);
  subtypePrintKernel(subtype_ptr, stdout);
}



int subtypeCompare(int subtypeID1, int subtypeID2)
{
  subtype_t *subtype_ptr1 = (subtype_t *)reshGetVal(subtypeID1, &subtypeOps);
  subtype_t *subtype_ptr2 = (subtype_t *)reshGetVal(subtypeID2, &subtypeOps);
  return subtypeCompareP(subtype_ptr1,subtype_ptr2);
}



int subtypeInqSize(int subtypeID)
{
  if ( subtypeID == CDI_UNDEFID )
    {
      return 0;
    }
  else
    {
      subtype_t *subtype_ptr = (subtype_t *)reshGetVal(subtypeID, &subtypeOps);
      return subtype_ptr->nentries;
    }
}



int subtypeInqActiveIndex(int subtypeID)
{
  if (subtypeID == CDI_UNDEFID) return 0;
  subtype_t *subtype_ptr = (subtype_t *)reshGetVal(subtypeID, &subtypeOps);
  return subtype_ptr->active_subtype_index;
}



void subtypeDefActiveIndex(int subtypeID, int index)
{
  subtype_t *subtype_ptr = (subtype_t *)reshGetVal(subtypeID, &subtypeOps);
  subtype_ptr->active_subtype_index = index;
}




int subtypeInqSubEntry(int subtypeID, subtype_query_t criterion)
{
  subtype_t *subtype_ptr = (subtype_t *)reshGetVal(subtypeID, &subtypeOps);
  struct subtype_entry_t *entry = subtype_ptr->entries;

  while (entry != NULL) {
    {
      int match = 1;

      for (int j=0; (j<criterion.nAND) && (match); j++)
        {
          if (CDI_Debug) Message("check criterion %d :  %d --?-- %d", j,
                                  criterion.key_value_pairs[0][j], criterion.key_value_pairs[1][j]);
          struct subtype_attr_t* att_ptr =
            subtypeAttrFind(entry->atts, criterion.key_value_pairs[0][j]);
          if (att_ptr == NULL)
            {
              match = 0;
              if (CDI_Debug) Message("did not find %d", criterion.key_value_pairs[0][j]);
            }
          else
            {
              if (CDI_Debug) Message("found %d", criterion.key_value_pairs[0][j]);
              match &= (att_ptr->val == criterion.key_value_pairs[1][j]);
            }
        }
      if (match) return entry->self;
    }
    entry = entry->next;
  }
  return CDI_UNDEFID;
}


int subtypeInqTile(int subtypeID, int tileindex, int attribute)
{
  return subtypeInqSubEntry(subtypeID,
                            matchAND(keyValuePair(cdiSubtypeAttributeName[SUBTYPE_ATT_TILEINDEX], tileindex),
                                     keyValuePair(cdiSubtypeAttributeName[SUBTYPE_ATT_TILEATTRIBUTE], attribute)));
}
int vlistDefTileSubtype(int vlistID, subtype_t *tiles)
{
  int subtypeID = CDI_UNDEFID;


  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  int tileset_defined = 0;
  for (int isub=0; isub<vlistptr->nsubtypes; isub++)
    {

      subtypeID = vlistptr->subtypeIDs[isub];
      if (subtypeComparePtr(subtypeID, tiles) == 0)
        {
          tileset_defined = 1;
          break;
        }
    }


  if (tileset_defined == 0) {
    subtype_t *tiles_duplicate = NULL;
    subtypeDuplicate(tiles, &tiles_duplicate);
    subtypeID = vlistptr->subtypeIDs[vlistptr->nsubtypes++] = subtypePush(tiles_duplicate);
  }

  return subtypeID;
}



int vlistInsertTrivialTileSubtype(int vlistID)
{

  subtype_t *subtype_ptr;
  subtypeAllocate(&subtype_ptr, SUBTYPE_TILES);


  (void) subtypeEntryInsert(subtype_ptr);


  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  int subtypeID = vlistptr->subtypeIDs[vlistptr->nsubtypes++] = subtypePush(subtype_ptr);
  return subtypeID;
}
static int subtypeGetPackSize( void * subtype_ptr, void *context)
{
  (void)subtype_ptr; (void)context;
  Error("Not yet implemented for subtypes!");
  return 0;
}

static void subtypePack( void * subtype_ptr, void * buffer, int size, int *pos, void *context)
{
  (void)subtype_ptr; (void)buffer; (void)size; (void)pos; (void)context;
  Error("Not yet implemented for subtypes!");
}

static int subtypeTxCode( void )
{ Error("Not yet implemented for subtypes!"); return 0; }
#ifndef _TABLEPAR_H
#define _TABLEPAR_H

enum {
  TABLE_DUP_NAME = 1 << 0,
  TABLE_DUP_LONGNAME = 1 << 1,
  TABLE_DUP_UNITS = 1 << 2,
};

typedef struct
{
  int id;
  int dupflags;
  const char *name;
  const char *longname;
  const char *units;
}
PAR;


int tableDef(int modelID, int tablegribID, const char *tablename);

#endif
#if defined (HAVE_CONFIG_H)
#endif

#include <ctype.h>
#include <stddef.h>
#include <string.h>


#undef UNDEFID
#define UNDEFID -1





#define MAX_TABLE 256
#define MAX_PARS 1024

typedef struct
{
  int used;
  int npars;
  PAR *pars;
  int modelID;
  int number;
  char *name;
}
PARTAB;

static PARTAB parTable[MAX_TABLE];
static int parTableSize = MAX_TABLE;
static int parTableNum = 0;
static int ParTableInit = 0;

static char *tablePath = NULL;

static void tableDefModelID(int tableID, int modelID);
static void tableDefNum(int tableID, int tablenum);


void tableDefEntry(int tableID, int id, const char *name,
                   const char *longname, const char *units)
{
  int item;

  if ( tableID >= 0 && tableID < MAX_TABLE && parTable[tableID].used) { } else
    Error("Invalid table ID %d", tableID);
  item = parTable[tableID].npars++;
  parTable[tableID].pars[item].id = id;
  parTable[tableID].pars[item].dupflags = 0;
  parTable[tableID].pars[item].name = NULL;
  parTable[tableID].pars[item].longname = NULL;
  parTable[tableID].pars[item].units = NULL;

  if ( name && name[0] )
    {
      parTable[tableID].pars[item].name = strdupx(name);
      parTable[tableID].pars[item].dupflags |= TABLE_DUP_NAME;
    }
  if ( longname && longname[0] )
    {
      parTable[tableID].pars[item].longname = strdupx(longname);
      parTable[tableID].pars[item].dupflags |= TABLE_DUP_LONGNAME;
    }
  if ( units && units[0] )
    {
      parTable[tableID].pars[item].units = strdupx(units);
      parTable[tableID].pars[item].dupflags |= TABLE_DUP_UNITS;
    }
}

static void parTableInitEntry(int tableID)
{
  parTable[tableID].used = 0;
  parTable[tableID].pars = NULL;
  parTable[tableID].npars = 0;
  parTable[tableID].modelID = UNDEFID;
  parTable[tableID].number = UNDEFID;
  parTable[tableID].name = NULL;
}

static void tableGetPath(void)
{
  char *path;

  path = getenv("TABLEPATH");

  if ( path ) tablePath = strdupx(path);



}

static void parTableFinalize(void)
{
  for (int tableID = 0; tableID < MAX_TABLE; ++tableID)
    if (parTable[tableID].used)
      {
        int npars = parTable[tableID].npars;
        for (int item = 0; item < npars; ++item)
          {
            if (parTable[tableID].pars[item].dupflags & TABLE_DUP_NAME)
              Free((void *)parTable[tableID].pars[item].name);
            if (parTable[tableID].pars[item].dupflags & TABLE_DUP_LONGNAME)
              Free((void *)parTable[tableID].pars[item].longname);
            if (parTable[tableID].pars[item].dupflags & TABLE_DUP_UNITS)
              Free((void *)parTable[tableID].pars[item].units);
          }
        Free(parTable[tableID].pars);
        Free(parTable[tableID].name);
      }
}

static void parTableInit(void)
{
  ParTableInit = 1;

  atexit(parTableFinalize);
  if ( cdiPartabIntern )
    tableDefault();

  tableGetPath();
}

static int tableNewEntry()
{
  int tableID = 0;
  static int init = 0;

  if ( ! init )
    {
      for ( tableID = 0; tableID < parTableSize; tableID++ )
        parTableInitEntry(tableID);
      init = 1;
    }




  for ( tableID = 0; tableID < parTableSize; tableID++ )
    {
      if ( ! parTable[tableID].used ) break;
    }

  if ( tableID == parTableSize )
    Error("no more entries!");

  parTable[tableID].used = 1;
  parTableNum++;

  return (tableID);
}

static int
decodeForm1(char *pline, char *name, char *longname, char *units)
{
  char *pstart, *pend;


                     strtol(pline, &pline, 10);
  while ( isspace((int) *pline) ) pline++;

  pstart = pline;
  while ( ! (isspace((int) *pline) || *pline == 0) ) pline++;
  size_t len = (size_t)(pline - pstart);
  if ( len > 0 )
    {
      memcpy(name, pstart, len);
      name[len] = 0;
    }
  else
    return (0);

  if ( pline[0] == 0 ) return (0);



                      strtod(pline, &pline);

                      strtod(pline, &pline);

  while ( isspace((int) *pline) ) pline++;

  len = strlen(pline);
  if ( len > 0 )
    {
      pstart = pline;
      pend = strrchr(pline, '[');
      if ( pend == pstart )
        len = 0;
      else
        {
          if ( pend )
            pend--;
          else
            pend = pstart + len;
          while ( isspace((int) *pend) ) pend--;
          len = (size_t)(pend - pstart + 1);
        }
      if ( len > 0 )
        {
          memcpy(longname, pstart, len);
          longname[len] = 0;
        }
      pstart = strrchr(pline, '[');
      if ( pstart )
        {
          pstart++;
          while ( isspace((int) *pstart) ) pstart++;
          pend = strchr(pstart, ']');
          if ( ! pend ) return (0);
          pend--;
          while ( isspace((int) *pend) ) pend--;
          len = (size_t)(pend - pstart + 1);
          if ( len > 0 )
            {
              memcpy(units, pstart, len);
              units[len] = 0;
            }
        }
    }

  return (0);
}

static int
decodeForm2(char *pline, char *name, char *longname, char *units)
{

  char *pend;

  pline = strchr(pline, '|');
  pline++;

  while ( isspace((int) *pline) ) pline++;
  if (*pline != '|')
    {
      pend = strchr(pline, '|');
      if ( ! pend )
        {
          pend = pline;
          while ( ! isspace((int) *pend) ) pend++;
          size_t len = (size_t)(pend - pline);
          if ( len > 0 )
            {
              memcpy(name, pline, len);
              name[len] = 0;
            }
          return (0);
        }
      else
        {
          pend--;
          while ( isspace((int) *pend) ) pend--;
          size_t len = (size_t)(pend - pline + 1);
          if ( len > 0 )
            {
              memcpy(name, pline, len);
              name[len] = 0;
            }
        }
    }
  else
    name[0] = '\0';

  pline = strchr(pline, '|');
  pline++;
  while ( isspace((int) *pline) ) pline++;
  pend = strchr(pline, '|');
  if ( !pend ) pend = strchr(pline, 0);
  pend--;
  while ( isspace((int) *pend) ) pend--;
  {
    size_t len = (size_t)(pend - pline + 1);
    if ( len > 0 )
      {
        memcpy(longname, pline, len);
        longname[len] = 0;
      }
  }

  pline = strchr(pline, '|');
  if ( pline )
    {
      pline++;
      while ( isspace((int) *pline) ) pline++;
      pend = strchr(pline, '|');
      if ( !pend ) pend = strchr(pline, 0);
      pend--;
      while ( isspace((int) *pend) ) pend--;
      ptrdiff_t len = pend - pline + 1;
      if ( len < 0 ) len = 0;
      memcpy(units, pline, (size_t)len);
      units[len] = 0;
    }

  return (0);
}

int tableRead(const char *tablefile)
{
  char line[1024], *pline;
  int lnr = 0;
  int id;
  char name[256], longname[256], units[256];
  int tableID = UNDEFID;
  int err;
  char *tablename;
  FILE *tablefp;

  tablefp = fopen(tablefile, "r");
  if ( tablefp == NULL ) return (tableID);

  tablename = (char* )strrchr(tablefile, '/');
  if ( tablename == 0 ) tablename = (char *) tablefile;
  else tablename++;

  tableID = tableDef(-1, 0, tablename);

  while ( fgets(line, 1023, tablefp) )
    {
      size_t len = strlen(line);
      if ( line[len-1] == '\n' ) line[len-1] = '\0';
      lnr++;
      id = CDI_UNDEFID;
      name[0] = 0;
      longname[0] = 0;
      units[0] = 0;
      if ( line[0] == '#' ) continue;
      pline = line;

      len = strlen(pline);
      if ( len < 4 ) continue;
      while ( isspace((int) *pline) ) pline++;
      id = atoi(pline);



      if ( id == 0 ) continue;

      while ( isdigit((int) *pline) ) pline++;

      if ( strchr(pline, '|') )
        err = decodeForm2(pline, name, longname, units);
      else
        err = decodeForm1(pline, name, longname, units);

      if ( err ) continue;

      if ( name[0] ) sprintf(name, "var%d", id);

      tableDefEntry(tableID, id, name, longname, units);
    }

  return (tableID);
}

static int tableFromEnv(int modelID, int tablenum)
{
  int tableID = UNDEFID;
  char tablename[256] = {'\0'};
  int tablenamefound = 0;

  const char *modelName;
  if ( (modelName = modelInqNamePtr(modelID)) )
    {
      strcpy(tablename, modelName);
      if ( tablenum )
        {
          size_t len = strlen(tablename);
          sprintf(tablename+len, "_%03d", tablenum);
        }
      tablenamefound = 1;
    }
  else
    {
      int instID = modelInqInstitut(modelID);
      if ( instID != UNDEFID )
        {
          const char *instName;
          if ( (instName = institutInqNamePtr(instID)) )
            {
              strcpy(tablename, instName);
              if ( tablenum )
                {
                  size_t len = strlen(tablename);
                  sprintf(tablename+len, "_%03d", tablenum);
                }
              tablenamefound = 1;
            }
        }
    }

  if ( tablenamefound )
    {
      size_t lenp = 0, lenf;
      char *tablefile = NULL;
      if ( tablePath )
        lenp = strlen(tablePath);
      lenf = strlen(tablename);


      tablefile = (char *) Malloc(lenp+lenf+3);
      if ( tablePath )
        {
          strcpy(tablefile, tablePath);
          strcat(tablefile, "/");
        }
      else
        tablefile[0] = '\0';
      strcat(tablefile, tablename);


      tableID = tableRead(tablefile);
      if ( tableID != UNDEFID )
        {
          tableDefModelID(tableID, modelID);
          tableDefNum(tableID, tablenum);
        }


      Free(tablefile);
    }

  return (tableID);
}

int tableInq(int modelID, int tablenum, const char *tablename)
{
  int tableID = UNDEFID;
  int modelID2 = UNDEFID;
  char tablefile[256] = {'\0'};

  if ( ! ParTableInit ) parTableInit();

  if ( tablename )
    {
      size_t len;
      strcpy(tablefile, tablename);




      for ( tableID = 0; tableID < MAX_TABLE; tableID++ )
        {
          if ( parTable[tableID].used && parTable[tableID].name )
            {

              len = strlen(tablename);
              if ( memcmp(parTable[tableID].name, tablename, len) == 0 ) break;
            }
        }
      if ( tableID == MAX_TABLE ) tableID = UNDEFID;
      if ( CDI_Debug )
        Message("tableID = %d tablename = %s", tableID, tablename);
    }
  else
    {
      for ( tableID = 0; tableID < MAX_TABLE; tableID++ )
        {
          if ( parTable[tableID].used )
            {
              if ( parTable[tableID].modelID == modelID &&
                   parTable[tableID].number == tablenum ) break;
            }
        }

      if ( tableID == MAX_TABLE ) tableID = UNDEFID;

      if ( tableID == UNDEFID )
        {
          if ( modelID != UNDEFID )
            {
              const char *modelName;
              if ( (modelName = modelInqNamePtr(modelID)) )
                {
                  strcpy(tablefile, modelName);
                  size_t len = strlen(tablefile);
                  for ( size_t i = 0; i < len; i++)
                    if ( tablefile[i] == '.' ) tablefile[i] = '\0';
                  modelID2 = modelInq(-1, 0, tablefile);
                }
            }
          if ( modelID2 != UNDEFID )
            for ( tableID = 0; tableID < MAX_TABLE; tableID++ )
              {
                if ( parTable[tableID].used )
                  {
                    if ( parTable[tableID].modelID == modelID2 &&
                         parTable[tableID].number == tablenum ) break;
                  }
              }
        }

      if ( tableID == MAX_TABLE ) tableID = UNDEFID;

      if ( tableID == UNDEFID && modelID != UNDEFID )
        tableID = tableFromEnv(modelID, tablenum);

      if ( CDI_Debug )
        if ( tablename )
          Message("tableID = %d tablename = %s", tableID, tablename);
    }

  return (tableID);
}

int tableDef(int modelID, int tablenum, const char *tablename)
{
  int tableID = UNDEFID;

  if ( ! ParTableInit ) parTableInit();




  if ( tableID == UNDEFID )
    {
      tableID = tableNewEntry();

      parTable[tableID].modelID = modelID;
      parTable[tableID].number = tablenum;
      if ( tablename )
        parTable[tableID].name = strdupx(tablename);

      parTable[tableID].pars = (PAR *) Malloc(MAX_PARS * sizeof(PAR));
    }

  return (tableID);
}

static void tableDefModelID(int tableID, int modelID)
{
  parTable[tableID].modelID = modelID;
}

static void tableDefNum(int tableID, int tablenum)
{
  parTable[tableID].number = tablenum;
}

int tableInqNum(int tableID)
{
  int number = 0;

  if ( tableID >= 0 && tableID < MAX_TABLE )
    number = parTable[tableID].number;

  return (number);
}

int tableInqModel(int tableID)
{
  int modelID = -1;

  if ( tableID >= 0 && tableID < MAX_TABLE )
    modelID = parTable[tableID].modelID;

  return (modelID);
}

static void partabCheckID(int item)
{
  if ( item < 0 || item >= parTableSize )
    Error("item %d undefined!", item);

  if ( ! parTable[item].name )
    Error("item %d name undefined!", item);
}

const char *tableInqNamePtr(int tableID)
{
  const char *tablename = NULL;

  if ( CDI_Debug )
    Message("tableID = %d", tableID);

  if ( ! ParTableInit ) parTableInit();

  if ( tableID >= 0 && tableID < parTableSize )
    if ( parTable[tableID].name )
      tablename = parTable[tableID].name;

  return (tablename);
}

void tableWrite(const char *ptfile, int tableID)
{
  int item, npars;
  size_t maxname = 4, maxlname = 10, maxunits = 2;
  FILE *ptfp;
  int tablenum, modelID, instID = CDI_UNDEFID;
  int center = 0, subcenter = 0;
  const char *instnameptr = NULL, *modelnameptr = NULL;

  if ( CDI_Debug )
    Message("write parameter table %d to %s", tableID, ptfile);

  if ( tableID == UNDEFID )
    {
      Warning("parameter table ID undefined");
      return;
    }

  partabCheckID(tableID);

  ptfp = fopen(ptfile, "w");

  npars = parTable[tableID].npars;

  for ( item = 0; item < npars; item++)
    {
      if ( parTable[tableID].pars[item].name )
        {
          size_t lenname = strlen(parTable[tableID].pars[item].name);
          if ( lenname > maxname ) maxname = lenname;
        }

      if ( parTable[tableID].pars[item].longname )
        {
          size_t lenlname = strlen(parTable[tableID].pars[item].longname);
          if ( lenlname > maxlname ) maxlname = lenlname;
        }

      if ( parTable[tableID].pars[item].units )
        {
          size_t lenunits = strlen(parTable[tableID].pars[item].units);
          if ( lenunits > maxunits ) maxunits = lenunits;
        }
    }

  tablenum = tableInqNum(tableID);
  modelID = parTable[tableID].modelID;
  if ( modelID != CDI_UNDEFID )
    {
      modelnameptr = modelInqNamePtr(modelID);
      instID = modelInqInstitut(modelID);
    }
  if ( instID != CDI_UNDEFID )
    {
      center = institutInqCenter(instID);
      subcenter = institutInqSubcenter(instID);
      instnameptr = institutInqNamePtr(instID);
    }

  fprintf(ptfp, "# Parameter table\n");
  fprintf(ptfp, "#\n");
  if ( tablenum )
    fprintf(ptfp, "# TABLE_ID=%d\n", tablenum);
  fprintf(ptfp, "# TABLE_NAME=%s\n", parTable[tableID].name);
  if ( modelnameptr )
    fprintf(ptfp, "# TABLE_MODEL=%s\n", modelnameptr);
  if ( instnameptr )
    fprintf(ptfp, "# TABLE_INSTITUT=%s\n", instnameptr);
  if ( center )
    fprintf(ptfp, "# TABLE_CENTER=%d\n", center);
  if ( subcenter )
    fprintf(ptfp, "# TABLE_SUBCENTER=%d\n", subcenter);
  fprintf(ptfp, "#\n");
  fprintf(ptfp, "#\n");
  fprintf(ptfp, "# id       = parameter ID\n");
  fprintf(ptfp, "# name     = variable name\n");
  fprintf(ptfp, "# title    = long name (description)\n");
  fprintf(ptfp, "# units    = variable units\n");
  fprintf(ptfp, "#\n");
  fprintf(ptfp, "# The format of each record is:\n");
  fprintf(ptfp, "#\n");
  fprintf(ptfp, "# id | %-*s | %-*s | %-*s\n",
          (int)maxname, "name",
          (int)maxlname, "title",
          (int)maxunits, "units");

  for ( item = 0; item < npars; item++)
    {
      const char *name = parTable[tableID].pars[item].name,
        *longname = parTable[tableID].pars[item].longname,
        *units = parTable[tableID].pars[item].units;
      if ( name == NULL ) name = " ";
      if ( longname == NULL ) longname = " ";
      if ( units == NULL ) units = " ";
      fprintf(ptfp, "%4d | %-*s | %-*s | %-*s\n",
              parTable[tableID].pars[item].id,
              (int)maxname, name,
              (int)maxlname, longname,
              (int)maxunits, units);
    }

  fclose(ptfp);
}


void tableWriteC(const char *filename, int tableID)
{
  FILE *ptfp = fopen(filename, "w");
  if (!ptfp)
    Error("failed to open file \"%s\"!", filename);
  if ( CDI_Debug )
    Message("write parameter table %d to %s", tableID, filename);
  tableFWriteC(ptfp, tableID);
  fclose(ptfp);
}

void tableFWriteC(FILE *ptfp, int tableID)
{
  const char chelp[] = "";
  int item, npars;
  size_t maxname = 0, maxlname = 0, maxunits = 0;
  char tablename[256];


  if ( tableID == UNDEFID )
    {
      Warning("parameter table ID undefined");
      return;
    }

  partabCheckID(tableID);

  npars = parTable[tableID].npars;

  for ( item = 0; item < npars; item++)
    {
      if ( parTable[tableID].pars[item].name )
        {
          size_t lenname = strlen(parTable[tableID].pars[item].name);
          if ( lenname > maxname ) maxname = lenname;
        }

      if ( parTable[tableID].pars[item].longname )
        {
          size_t lenlname = strlen(parTable[tableID].pars[item].longname);
          if ( lenlname > maxlname ) maxlname = lenlname;
        }

      if ( parTable[tableID].pars[item].units )
        {
          size_t lenunits = strlen(parTable[tableID].pars[item].units);
          if ( lenunits > maxunits ) maxunits = lenunits;
        }
    }

  strncpy(tablename, parTable[tableID].name, sizeof (tablename));
  tablename[sizeof (tablename) - 1] = '\0';
  {
    size_t len = strlen(tablename);
    for (size_t i = 0; i < len; i++ )
      if ( tablename[i] == '.' ) tablename[i] = '_';
  }
  fprintf(ptfp, "static const PAR %s[] = {\n", tablename);

  for ( item = 0; item < npars; item++ )
    {
      size_t len = strlen(parTable[tableID].pars[item].name),
        llen = parTable[tableID].pars[item].longname
        ? strlen(parTable[tableID].pars[item].longname) : 0,
        ulen = parTable[tableID].pars[item].units
        ? strlen(parTable[tableID].pars[item].units) : 0;
      fprintf(ptfp, "  {%4d, 0, \"%s\", %-*s%c%s%s, %-*s%c%s%s %-*s},\n",
              parTable[tableID].pars[item].id,
              parTable[tableID].pars[item].name, (int)(maxname-len), chelp,
              llen?'"':' ',
              llen?parTable[tableID].pars[item].longname:"NULL",
              llen?"\"":"",
              (int)(maxlname-(llen?llen:3)), chelp,
              ulen?'"':' ',
              ulen?parTable[tableID].pars[item].units:"NULL",
              ulen?"\"":"",
              (int)(maxunits-(ulen?ulen:3)), chelp);
    }

  fprintf(ptfp, "};\n\n");
}


int tableInqParCode(int tableID, char *varname, int *code)
{
  int err = 1;

  if ( tableID != UNDEFID && varname != NULL )
    {
      int npars = parTable[tableID].npars;
      for ( int item = 0; item < npars; item++ )
        {
          if ( parTable[tableID].pars[item].name
               && strcmp(parTable[tableID].pars[item].name, varname) == 0 )
            {
              *code = parTable[tableID].pars[item].id;
              err = 0;
              break;
            }
        }
    }

  return (err);
}


int tableInqParName(int tableID, int code, char *varname)
{
  int err = 1;

  if ( tableID >= 0 && tableID < MAX_TABLE )
    {
      int npars = parTable[tableID].npars;
      for ( int item = 0; item < npars; item++ )
        {
          if ( parTable[tableID].pars[item].id == code )
            {
              if ( parTable[tableID].pars[item].name )
                strcpy(varname, parTable[tableID].pars[item].name);
              err = 0;
              break;
            }
        }
    }
  else if ( tableID == UNDEFID )
    { }
  else
    Error("Invalid table ID %d", tableID);

  return (err);
}


const char *tableInqParNamePtr(int tableID, int code)
{
  const char *name = NULL;

  if ( tableID != UNDEFID )
    {
      int npars = parTable[tableID].npars;
      for ( int item = 0; item < npars; item++ )
        {
          if ( parTable[tableID].pars[item].id == code )
            {
              name = parTable[tableID].pars[item].name;
              break;
            }
        }
    }

  return (name);
}


const char *tableInqParLongnamePtr(int tableID, int code)
{
  const char *longname = NULL;

  if ( tableID != UNDEFID )
    {
      int npars = parTable[tableID].npars;
      for ( int item = 0; item < npars; item++ )
        {
          if ( parTable[tableID].pars[item].id == code )
            {
              longname = parTable[tableID].pars[item].longname;
              break;
            }
        }
    }

  return (longname);
}


const char *tableInqParUnitsPtr(int tableID, int code)
{
  const char *units = NULL;

  if ( tableID != UNDEFID )
    {
      int npars = parTable[tableID].npars;
      for ( int item = 0; item < npars; item++ )
        {
          if ( parTable[tableID].pars[item].id == code )
            {
              units = parTable[tableID].pars[item].units;
              break;
            }
        }
    }

  return (units);
}


int tableInqParLongname(int tableID, int code, char *longname)
{
  if ( ((tableID >= 0) & (tableID < MAX_TABLE)) | (tableID == UNDEFID) ) { } else
    Error("Invalid table ID %d", tableID);

  int err = 1;

  if ( tableID != UNDEFID )
    {
      int npars = parTable[tableID].npars;
      for ( int item = 0; item < npars; item++ )
        {
          if ( parTable[tableID].pars[item].id == code )
            {
              if ( parTable[tableID].pars[item].longname )
                strcpy(longname, parTable[tableID].pars[item].longname);
              err = 0;
              break;
            }
        }
    }

  return (err);
}


int tableInqParUnits(int tableID, int code, char *units)
{

  if ( ((tableID >= 0) & (tableID < MAX_TABLE)) | (tableID == UNDEFID) ) { } else
    Error("Invalid table ID %d", tableID);

  int err = 1;

  if ( tableID != UNDEFID )
    {
      int npars = parTable[tableID].npars;
      for ( int item = 0; item < npars; item++ )
        {
          if ( parTable[tableID].pars[item].id == code )
            {
              if ( parTable[tableID].pars[item].units )
                strcpy(units, parTable[tableID].pars[item].units);
              err = 0;
              break;
            }
        }
    }

  return (err);
}


void tableInqPar(int tableID, int code, char *name, char *longname, char *units)
{

  if ( ((tableID >= 0) & (tableID < MAX_TABLE)) | (tableID == UNDEFID) ) { } else
    Error("Invalid table ID %d", tableID);

  int npars = parTable[tableID].npars;

  for ( int item = 0; item < npars; item++ )
    {
      if ( parTable[tableID].pars[item].id == code )
        {
          if ( parTable[tableID].pars[item].name )
            strcpy(name, parTable[tableID].pars[item].name);
          if ( parTable[tableID].pars[item].longname )
            strcpy(longname, parTable[tableID].pars[item].longname);
          if ( parTable[tableID].pars[item].units )
            strcpy(units, parTable[tableID].pars[item].units);
          break;
        }
    }
}

int tableInqNumber(void)
{
  if ( ! ParTableInit ) parTableInit();

  return (parTableNum);
}
#if defined (HAVE_CONFIG_H)
#endif

#include <stddef.h>
#include <string.h>


static int DefaultTimeType = TAXIS_ABSOLUTE;
static int DefaultTimeUnit = TUNIT_HOUR;


static const char *Timeunits[] = {
  "undefined",
  "seconds",
  "minutes",
  "quarters",
  "30minutes",
  "hours",
  "3hours",
  "6hours",
  "12hours",
  "days",
  "months",
  "years",
};


static int taxisCompareP ( void * taxisptr1, void * taxisptr2 );
static void taxisDestroyP ( void * taxisptr );
static void taxisPrintKernel(taxis_t *taxisptr, FILE * fp);
static int taxisGetPackSize ( void * taxisptr, void *context );
static void taxisPack ( void * taxisptr, void *buf, int size,
                                 int *position, void *context );
static int taxisTxCode ( void );

const resOps taxisOps = {
  taxisCompareP,
  taxisDestroyP,
  (void (*)(void *, FILE *))taxisPrintKernel,
  taxisGetPackSize,
  taxisPack,
  taxisTxCode
};

#define container_of(ptr,type,member) \
  ((type *)(void*)((unsigned char *)ptr - offsetof(type,member)))

struct refcount_string
{
  int ref_count;
  char string[];
};

static char *
new_refcount_string(size_t len)
{
  struct refcount_string *container
    = (struct refcount_string *) Malloc(sizeof (*container) + len + 1);
  container->ref_count = 1;
  return container->string;
}

static void
delete_refcount_string(void *p)
{
  if (p)
    {
      struct refcount_string *container
        = container_of(p, struct refcount_string, string);
      if (!--(container->ref_count))
        Free(container);
    }
}

static char *
dup_refcount_string(char *p)
{
  if (p)
    {
      struct refcount_string *container
        = container_of(p, struct refcount_string, string);
      ++(container->ref_count);
    }
  return p;
}


#undef container_of

static int TAXIS_Debug = 0;


const char *tunitNamePtr(int unitID)
{
  const char *name;
  int size = sizeof(Timeunits)/sizeof(*Timeunits);

  if ( unitID > 0 && unitID < size )
    name = Timeunits[unitID];
  else
    name = Timeunits[0];

  return (name);
}

#if 0
static
void taxis_defaults(void)
{
  char *timeunit;

  timeunit = getenv("TIMEUNIT");
  if ( timeunit )
    {
      if ( strcmp(timeunit, "minutes") == 0 )
        DefaultTimeUnit = TUNIT_MINUTE;
      else if ( strcmp(timeunit, "hours") == 0 )
        DefaultTimeUnit = TUNIT_HOUR;
      else if ( strcmp(timeunit, "3hours") == 0 )
        DefaultTimeUnit = TUNIT_3HOURS;
      else if ( strcmp(timeunit, "6hours") == 0 )
        DefaultTimeUnit = TUNIT_6HOURS;
      else if ( strcmp(timeunit, "12hours") == 0 )
        DefaultTimeUnit = TUNIT_12HOURS;
      else if ( strcmp(timeunit, "days") == 0 )
        DefaultTimeUnit = TUNIT_DAY;
      else if ( strcmp(timeunit, "months") == 0 )
        DefaultTimeUnit = TUNIT_MONTH;
      else if ( strcmp(timeunit, "years") == 0 )
        DefaultTimeUnit = TUNIT_YEAR;
      else
        Warning("Unsupported TIMEUNIT %s!", timeunit);
    }
}
#endif

static
void taxisDefaultValue(taxis_t* taxisptr)
{
  taxisptr->self = CDI_UNDEFID;
  taxisptr->used = FALSE;
  taxisptr->type = DefaultTimeType;
  taxisptr->vdate = 0;
  taxisptr->vtime = 0;
  taxisptr->rdate = CDI_UNDEFID;
  taxisptr->rtime = 0;
  taxisptr->fdate = CDI_UNDEFID;
  taxisptr->ftime = 0;
  taxisptr->calendar = cdiDefaultCalendar;
  taxisptr->unit = DefaultTimeUnit;
  taxisptr->numavg = 0;
  taxisptr->climatology = FALSE;
  taxisptr->has_bounds = FALSE;
  taxisptr->vdate_lb = 0;
  taxisptr->vtime_lb = 0;
  taxisptr->vdate_ub = 0;
  taxisptr->vtime_ub = 0;
  taxisptr->fc_unit = DefaultTimeUnit;
  taxisptr->fc_period = 0;
  taxisptr->name = NULL;
  taxisptr->longname = NULL;
}

static taxis_t *
taxisNewEntry(cdiResH resH)
{
  taxis_t *taxisptr = (taxis_t*) Malloc(sizeof(taxis_t));

  taxisDefaultValue(taxisptr);
  if (resH == CDI_UNDEFID)
    taxisptr->self = reshPut(taxisptr, &taxisOps);
  else
    {
      taxisptr->self = resH;
      reshReplace(resH, taxisptr, &taxisOps);
    }

  return (taxisptr);
}

static
void taxisInit (void)
{
  static int taxisInitialized = 0;
  char *env;

  if ( taxisInitialized ) return;

  taxisInitialized = 1;

  env = getenv("TAXIS_DEBUG");
  if ( env ) TAXIS_Debug = atoi(env);
}

#if 0
static
void taxis_copy(taxis_t *taxisptr2, taxis_t *taxisptr1)
{
  int taxisID2 = taxisptr2->self;
  memcpy(taxisptr2, taxisptr1, sizeof(taxis_t));
  taxisptr2->self = taxisID2;
}
#endif
int taxisCreate(int taxistype)
{
  if ( CDI_Debug )
    Message("taxistype: %d", taxistype);

  taxisInit ();

  taxis_t *taxisptr = taxisNewEntry(CDI_UNDEFID);
  taxisptr->type = taxistype;

  int taxisID = taxisptr->self;

  if ( CDI_Debug )
    Message("taxisID: %d", taxisID);

  return (taxisID);
}

void taxisDestroyKernel(taxis_t *taxisptr)
{
  delete_refcount_string(taxisptr->name);
  delete_refcount_string(taxisptr->longname);
}
void taxisDestroy(int taxisID)
{
  taxis_t *taxisptr = (taxis_t *)reshGetVal(taxisID, &taxisOps);
  reshRemove(taxisID, &taxisOps);
  taxisDestroyKernel(taxisptr);
  Free(taxisptr);
}


void taxisDestroyP( void * taxisptr )
{
  taxisDestroyKernel((taxis_t *)taxisptr);
  Free(taxisptr);
}


int taxisDuplicate(int taxisID1)
{
  taxis_t *taxisptr1 = (taxis_t *)reshGetVal(taxisID1, &taxisOps);
  taxis_t *taxisptr2 = taxisNewEntry(CDI_UNDEFID);

  int taxisID2 = taxisptr2->self;

  if ( CDI_Debug )
    Message("taxisID2: %d", taxisID2);

  ptaxisCopy(taxisptr2, taxisptr1);
  return (taxisID2);
}


void taxisDefType(int taxisID, int type)
{
  taxis_t *taxisptr = ( taxis_t * ) reshGetVal ( taxisID, &taxisOps );

  if (taxisptr->type != type)
    {
      taxisptr->type = type;
      reshSetStatus(taxisID, &taxisOps, RESH_DESYNC_IN_USE);
    }
}
void taxisDefVdate(int taxisID, int vdate)
{
  taxis_t *taxisptr = (taxis_t *)reshGetVal(taxisID, &taxisOps);

  if (taxisptr->vdate != vdate)
    {
      taxisptr->vdate = vdate;
      reshSetStatus(taxisID, &taxisOps, RESH_DESYNC_IN_USE);
    }
}
void taxisDefVtime(int taxisID, int vtime)
{
  taxis_t *taxisptr = (taxis_t *)reshGetVal(taxisID, &taxisOps);

  if (taxisptr->vtime != vtime)
    {
      taxisptr->vtime = vtime;
      reshSetStatus(taxisID, &taxisOps, RESH_DESYNC_IN_USE);
    }
}
void taxisDefRdate(int taxisID, int rdate)
{
  taxis_t *taxisptr = ( taxis_t * ) reshGetVal ( taxisID, &taxisOps );

  if (taxisptr->rdate != rdate)
    {
      taxisptr->rdate = rdate;
      reshSetStatus(taxisID, &taxisOps, RESH_DESYNC_IN_USE);
    }
}
void taxisDefRtime(int taxisID, int rtime)
{
  taxis_t *taxisptr = ( taxis_t * ) reshGetVal ( taxisID, &taxisOps );

  if (taxisptr->rtime != rtime)
    {
      taxisptr->rtime = rtime;
      reshSetStatus(taxisID, &taxisOps, RESH_DESYNC_IN_USE);
    }
}
void taxisDefFdate(int taxisID, int fdate)
{
  taxis_t *taxisptr = ( taxis_t * ) reshGetVal ( taxisID, &taxisOps );

  if (taxisptr->fdate != fdate)
    {
      taxisptr->fdate = fdate;
      reshSetStatus(taxisID, &taxisOps, RESH_DESYNC_IN_USE);
    }
}
void taxisDefFtime(int taxisID, int ftime)
{
  taxis_t *taxisptr = ( taxis_t * ) reshGetVal ( taxisID, &taxisOps );

  if (taxisptr->ftime != ftime)
    {
      taxisptr->ftime = ftime;
      reshSetStatus(taxisID, &taxisOps, RESH_DESYNC_IN_USE);
    }
}
void taxisDefCalendar(int taxisID, int calendar)
{
  taxis_t *taxisptr = ( taxis_t * ) reshGetVal ( taxisID, &taxisOps );

  if (taxisptr->calendar != calendar)
    {
      taxisptr->calendar = calendar;
      reshSetStatus(taxisID, &taxisOps, RESH_DESYNC_IN_USE);
    }
}


void taxisDefTunit(int taxisID, int unit)
{
  taxis_t *taxisptr = ( taxis_t * ) reshGetVal ( taxisID, &taxisOps );

  if (taxisptr->unit != unit)
    {
      taxisptr->unit = unit;
      reshSetStatus(taxisID, &taxisOps, RESH_DESYNC_IN_USE);
    }
}


void taxisDefForecastTunit(int taxisID, int unit)
{
  taxis_t *taxisptr = (taxis_t *)reshGetVal(taxisID, &taxisOps);

  if (taxisptr->fc_unit != unit)
    {
      taxisptr->fc_unit = unit;
      reshSetStatus(taxisID, &taxisOps, RESH_DESYNC_IN_USE);
    }
}


void taxisDefForecastPeriod(int taxisID, double fc_period)
{
  taxis_t *taxisptr = ( taxis_t * ) reshGetVal ( taxisID, &taxisOps );

  if ( IS_NOT_EQUAL(taxisptr->fc_period, fc_period) )
    {
      taxisptr->fc_period = fc_period;
      reshSetStatus(taxisID, &taxisOps, RESH_DESYNC_IN_USE);
    }
}


void taxisDefNumavg(int taxisID, int numavg)
{
  taxis_t *taxisptr = ( taxis_t * ) reshGetVal ( taxisID, &taxisOps );

  if (taxisptr->numavg != numavg)
    {
      taxisptr->numavg = numavg;
      reshSetStatus(taxisID, &taxisOps, RESH_DESYNC_IN_USE);
    }
}





int taxisInqType(int taxisID)
{
  taxis_t *taxisptr = (taxis_t *)reshGetVal(taxisID, &taxisOps);

  return (taxisptr->type);
}


int taxisHasBounds(int taxisID)
{
  taxis_t *taxisptr = (taxis_t *)reshGetVal(taxisID, &taxisOps);

  return (taxisptr->has_bounds);
}


void taxisDeleteBounds(int taxisID)
{
  taxis_t *taxisptr = ( taxis_t * ) reshGetVal ( taxisID, &taxisOps );

  if (taxisptr->has_bounds != FALSE)
    {
      taxisptr->has_bounds = FALSE;
      reshSetStatus(taxisID, &taxisOps, RESH_DESYNC_IN_USE);
    }
}


void taxisCopyTimestep(int taxisID2, int taxisID1)
{
  taxis_t *taxisptr1 = (taxis_t *)reshGetVal(taxisID1, &taxisOps),
    *taxisptr2 = (taxis_t *)reshGetVal(taxisID2, &taxisOps);

  reshLock();

  taxisptr2->rdate = taxisptr1->rdate;
  taxisptr2->rtime = taxisptr1->rtime;

  taxisptr2->vdate = taxisptr1->vdate;
  taxisptr2->vtime = taxisptr1->vtime;

  if ( taxisptr2->has_bounds )
    {
      taxisptr2->vdate_lb = taxisptr1->vdate_lb;
      taxisptr2->vtime_lb = taxisptr1->vtime_lb;
      taxisptr2->vdate_ub = taxisptr1->vdate_ub;
      taxisptr2->vtime_ub = taxisptr1->vtime_ub;
    }

  taxisptr2->fdate = taxisptr1->fdate;
  taxisptr2->ftime = taxisptr1->ftime;

  taxisptr2->fc_unit = taxisptr1->fc_unit;
  taxisptr2->fc_period = taxisptr1->fc_period;

  reshSetStatus(taxisID2, &taxisOps, RESH_DESYNC_IN_USE);
  reshUnlock();
}
int taxisInqVdate(int taxisID)
{
  taxis_t *taxisptr = (taxis_t *)reshGetVal(taxisID, &taxisOps);

  return (taxisptr->vdate);
}


void taxisInqVdateBounds(int taxisID, int *vdate_lb, int *vdate_ub)
{
  taxis_t *taxisptr = (taxis_t *)reshGetVal(taxisID, &taxisOps);

  *vdate_lb = taxisptr->vdate_lb;
  *vdate_ub = taxisptr->vdate_ub;
}


void taxisDefVdateBounds(int taxisID, int vdate_lb, int vdate_ub)
{
  taxis_t *taxisptr = ( taxis_t * ) reshGetVal ( taxisID, &taxisOps );

  if (taxisptr->vdate_lb != vdate_lb
      || taxisptr->vdate_ub != vdate_ub
      || taxisptr->has_bounds != TRUE)
    {
      taxisptr->vdate_lb = vdate_lb;
      taxisptr->vdate_ub = vdate_ub;
      taxisptr->has_bounds = TRUE;
      reshSetStatus(taxisID, &taxisOps, RESH_DESYNC_IN_USE);
    }
}
int taxisInqVtime(int taxisID)
{
  taxis_t *taxisptr = (taxis_t *)reshGetVal(taxisID, &taxisOps);

  return (taxisptr->vtime);
}


void taxisInqVtimeBounds(int taxisID, int *vtime_lb, int *vtime_ub)
{
  taxis_t *taxisptr = (taxis_t *)reshGetVal(taxisID, &taxisOps);

  *vtime_lb = taxisptr->vtime_lb;
  *vtime_ub = taxisptr->vtime_ub;
}


void taxisDefVtimeBounds(int taxisID, int vtime_lb, int vtime_ub)
{
  taxis_t *taxisptr = ( taxis_t * ) reshGetVal ( taxisID, &taxisOps );

  if (taxisptr->vtime_lb != vtime_lb
      || taxisptr->vtime_ub != vtime_ub
      || taxisptr->has_bounds != TRUE)
    {
      taxisptr->vtime_lb = vtime_lb;
      taxisptr->vtime_ub = vtime_ub;
      taxisptr->has_bounds = TRUE;
      reshSetStatus(taxisID, &taxisOps, RESH_DESYNC_IN_USE);
    }
}
int taxisInqRdate(int taxisID)
{
  taxis_t *taxisptr = (taxis_t *)reshGetVal(taxisID, &taxisOps);

  if ( taxisptr->rdate == -1 )
    {
      taxisptr->rdate = taxisptr->vdate;
      taxisptr->rtime = taxisptr->vtime;
      reshSetStatus(taxisID, &taxisOps, RESH_DESYNC_IN_USE);
    }

  return (taxisptr->rdate);
}
int taxisInqRtime(int taxisID)
{
  taxis_t *taxisptr = (taxis_t *)reshGetVal(taxisID, &taxisOps);

  if ( taxisptr->rdate == -1 )
    {
      taxisptr->rdate = taxisptr->vdate;
      taxisptr->rtime = taxisptr->vtime;
      reshSetStatus(taxisID, &taxisOps, RESH_DESYNC_IN_USE);
    }

  return (taxisptr->rtime);
}
int taxisInqFdate(int taxisID)
{
  taxis_t *taxisptr = (taxis_t *)reshGetVal(taxisID, &taxisOps);

  if ( taxisptr->fdate == -1 )
    {
      taxisptr->fdate = taxisptr->vdate;
      taxisptr->ftime = taxisptr->vtime;
    }

  return (taxisptr->fdate);
}
int taxisInqFtime(int taxisID)
{
  taxis_t *taxisptr = (taxis_t *)reshGetVal(taxisID, &taxisOps);

  if ( taxisptr->fdate == -1 )
    {
      taxisptr->fdate = taxisptr->vdate;
      taxisptr->ftime = taxisptr->vtime;
    }

  return (taxisptr->ftime);
}
int taxisInqCalendar(int taxisID)
{
  taxis_t *taxisptr = ( taxis_t * ) reshGetVal ( taxisID, &taxisOps );

  return (taxisptr->calendar);
}


int taxisInqTunit(int taxisID)
{
  taxis_t *taxisptr = ( taxis_t * ) reshGetVal ( taxisID, &taxisOps );

  return (taxisptr->unit);
}


int taxisInqForecastTunit(int taxisID)
{
  taxis_t *taxisptr = ( taxis_t * ) reshGetVal ( taxisID, &taxisOps );

  return (taxisptr->fc_unit);
}


double taxisInqForecastPeriod(int taxisID)
{
  taxis_t *taxisptr = ( taxis_t * ) reshGetVal ( taxisID, &taxisOps );

  return (taxisptr->fc_period);
}


int taxisInqNumavg(int taxisID)
{
  taxis_t *taxisptr = ( taxis_t * ) reshGetVal ( taxisID, &taxisOps );

  return (taxisptr->numavg);
}


taxis_t *taxisPtr(int taxisID)
{
  taxis_t *taxisptr = (taxis_t *)reshGetVal(taxisID, &taxisOps);

  return (taxisptr);
}

void
ptaxisDefName(taxis_t *taxisptr, const char *name)
{
  if (name)
    {
      size_t len = strlen(name);
      delete_refcount_string(taxisptr->name);
      char *taxisname = taxisptr->name = new_refcount_string(len);
      strcpy(taxisname, name);
    }
}

void
ptaxisDefLongname(taxis_t *taxisptr, const char *longname)
{
  if (longname)
    {
      size_t len = strlen(longname);
      delete_refcount_string(taxisptr->longname);
      char *taxislongname = taxisptr->longname = new_refcount_string(len);
      strcpy(taxislongname, longname);
    }
}


static void
cdiDecodeTimevalue(int timeunit, double timevalue, int *days, int *secs)
{
  static int lwarn = TRUE;

  *days = 0;
  *secs = 0;

  if ( timeunit == TUNIT_MINUTE )
    {
      timevalue *= 60;
      timeunit = TUNIT_SECOND;
    }
  else if ( timeunit == TUNIT_HOUR )
    {
      timevalue /= 24;
      timeunit = TUNIT_DAY;
    }

  if ( timeunit == TUNIT_SECOND )
    {
      *days = (int) (timevalue/86400);
      double seconds = timevalue - *days*86400.;
      *secs = (int)lround(seconds);
      if ( *secs < 0 ) { *days -= 1; *secs += 86400; };







    }
  else if ( timeunit == TUNIT_DAY )
    {
      *days = (int) timevalue;
      double seconds = (timevalue - *days)*86400;
      *secs = (int)lround(seconds);
      if ( *secs < 0 ) { *days -= 1; *secs += 86400; };







    }
  else
    {
      if ( lwarn )
        {
          Warning("timeunit %s unsupported!", tunitNamePtr(timeunit));
          lwarn = FALSE;
        }
    }
}

static
void cdiEncodeTimevalue(int days, int secs, int timeunit, double *timevalue)
{
  static int lwarn = TRUE;

  if ( timeunit == TUNIT_SECOND )
    {
      *timevalue = days*86400. + secs;
    }
  else if ( timeunit == TUNIT_MINUTE ||
            timeunit == TUNIT_QUARTER ||
            timeunit == TUNIT_30MINUTES )
    {
      *timevalue = days*1440. + secs/60.;
    }
  else if ( timeunit == TUNIT_HOUR ||
            timeunit == TUNIT_3HOURS ||
            timeunit == TUNIT_6HOURS ||
            timeunit == TUNIT_12HOURS )
    {
      *timevalue = days*24. + secs/3600.;
    }
  else if ( timeunit == TUNIT_DAY )
    {
      *timevalue = days + secs/86400.;
    }
  else
    {
      if ( lwarn )
        {
          Warning("timeunit %s unsupported!", tunitNamePtr(timeunit));
          lwarn = FALSE;
        }
    }
}

void timeval2vtime(double timevalue, taxis_t *taxis, int *vdate, int *vtime)
{
  int year, month, day, hour, minute, second;
  int rdate, rtime;
  int timeunit;
  int calendar;
  int julday, secofday, days, secs;

  *vdate = 0;
  *vtime = 0;

  timeunit = (*taxis).unit;
  calendar = (*taxis).calendar;

  rdate = (*taxis).rdate;
  rtime = (*taxis).rtime;

  if ( rdate == 0 && rtime == 0 && DBL_IS_EQUAL(timevalue, 0.) ) return;

  cdiDecodeDate(rdate, &year, &month, &day);
  cdiDecodeTime(rtime, &hour, &minute, &second);

  if ( timeunit == TUNIT_MONTH && calendar == CALENDAR_360DAYS )
    {
      timeunit = TUNIT_DAY;
      timevalue *= 30;
    }

  if ( timeunit == TUNIT_MONTH || timeunit == TUNIT_YEAR )
    {
      int nmon, dpm;
      double fmon;

      if ( timeunit == TUNIT_YEAR ) timevalue *= 12;

      nmon = (int) timevalue;
      fmon = timevalue - nmon;

      month += nmon;

      while ( month > 12 ) { month -= 12; year++; }
      while ( month < 1 ) { month += 12; year--; }

      dpm = days_per_month(calendar, year, month);
      timeunit = TUNIT_DAY;
      timevalue = fmon*dpm;
    }

  encode_caldaysec(calendar, year, month, day, hour, minute, second, &julday, &secofday);

  cdiDecodeTimevalue(timeunit, timevalue, &days, &secs);

  julday_add(days, secs, &julday, &secofday);

  decode_caldaysec(calendar, julday, secofday, &year, &month, &day, &hour, &minute, &second);

  *vdate = cdiEncodeDate(year, month, day);
  *vtime = cdiEncodeTime(hour, minute, second);
}


double vtime2timeval(int vdate, int vtime, taxis_t *taxis)
{
  int ryear, rmonth;
  int year, month, day, hour, minute, second;
  int rdate, rtime;
  double value = 0;
  int timeunit;
  int timeunit0;
  int calendar;
  int julday1, secofday1, julday2, secofday2, days, secs;

  timeunit = (*taxis).unit;
  calendar = (*taxis).calendar;

  rdate = (*taxis).rdate;
  rtime = (*taxis).rtime;
  if ( rdate == -1 )
    {
      rdate = (*taxis).vdate;
      rtime = (*taxis).vtime;
    }

  if ( rdate == 0 && rtime == 0 && vdate == 0 && vtime == 0 ) return(value);

  cdiDecodeDate(rdate, &ryear, &rmonth, &day);
  cdiDecodeTime(rtime, &hour, &minute, &second);

  encode_caldaysec(calendar, ryear, rmonth, day, hour, minute, second, &julday1, &secofday1);

  cdiDecodeDate(vdate, &year, &month, &day);
  cdiDecodeTime(vtime, &hour, &minute, &second);

  timeunit0 = timeunit;

  if ( timeunit == TUNIT_MONTH && calendar == CALENDAR_360DAYS )
    {
      timeunit = TUNIT_DAY;
    }

  if ( timeunit == TUNIT_MONTH || timeunit == TUNIT_YEAR )
    {
      int nmonth, dpm;

      value = (year-ryear)*12 - rmonth + month;

      nmonth = (int) value;
      month -= nmonth;

      while ( month > 12 ) { month -= 12; year++; }
      while ( month < 1 ) { month += 12; year--; }

      dpm = days_per_month(calendar, year, month);

      encode_caldaysec(calendar, year, month, day, hour, minute, second, &julday2, &secofday2);

      julday_sub(julday1, secofday1, julday2, secofday2, &days, &secs);

      value += (days+secs/86400.)/dpm;

      if ( timeunit == TUNIT_YEAR ) value = value/12;
    }
  else
    {
      encode_caldaysec(calendar, year, month, day, hour, minute, second, &julday2, &secofday2);

      julday_sub(julday1, secofday1, julday2, secofday2, &days, &secs);

      cdiEncodeTimevalue(days, secs, timeunit, &value);
    }

  if ( timeunit0 == TUNIT_MONTH && calendar == CALENDAR_360DAYS )
    {
      value /= 30;
    }

  return (value);
}


static void conv_timeval(double timevalue, int *rvdate, int *rvtime)
{
  int vdate = 0, vtime = 0;
  int hour, minute, second;
  int daysec;

  vdate = (int) timevalue;
  if ( vdate < 0 )
    daysec = (int) (-(timevalue - vdate)*86400 + 0.01);
  else
    daysec = (int) ( (timevalue - vdate)*86400 + 0.01);

  hour = daysec / 3600;
  minute = (daysec - hour*3600)/60;
  second = daysec - hour*3600 - minute*60;
  vtime = cdiEncodeTime(hour, minute, second);

  *rvdate = vdate;
  *rvtime = vtime;
}


static void
splitTimevalue(double timevalue, int timeunit, int *date, int *time)
{
  int vdate = 0, vtime = 0;
  int hour, minute, second;
  int year, month, day;
  static int lwarn = TRUE;

  if ( timeunit == TUNIT_SECOND )
    {
      timevalue /= 86400;
      conv_timeval(timevalue, &vdate, &vtime);
    }
  else if ( timeunit == TUNIT_HOUR )
    {
      timevalue /= 24;
      conv_timeval(timevalue, &vdate, &vtime);
    }
  else if ( timeunit == TUNIT_DAY )
    {
      conv_timeval(timevalue, &vdate, &vtime);
    }
  else if ( timeunit == TUNIT_MONTH )
    {
      vdate = (int) timevalue*100 - ((vdate < 0) * 2 - 1);
      vtime = 0;
    }
  else if ( timeunit == TUNIT_YEAR )
    {
      if ( timevalue < -214700 )
        {
          Warning("Year %g out of range, set to -214700", timevalue);
          timevalue = -214700;
        }
      else if ( timevalue > 214700 )
        {
          Warning("Year %g out of range, set to 214700", timevalue);
          timevalue = 214700;
        }

      vdate = (int) timevalue*10000;
      vdate += 101;
      vtime = 0;
    }
  else
    {
      if ( lwarn )
        {
          Warning("timeunit %s unsupported!", tunitNamePtr(timeunit));
          lwarn = FALSE;
        }
    }



  cdiDecodeDate(vdate, &year, &month, &day);
  cdiDecodeTime(vtime, &hour, &minute, &second);

  if ( month > 17 || day > 31 || hour > 23 || minute > 59 || second > 59 )
    {
      if ( (month > 17 || day > 31) && (year < -9999 || year > 9999) ) year = 1;
      if ( month > 17 ) month = 1;
      if ( day > 31 ) day = 1;
      if ( hour > 23 ) hour = 0;
      if ( minute > 59 ) minute = 0;
      if ( second > 59 ) second = 0;

      vdate = cdiEncodeDate(year, month, day);
      vtime = cdiEncodeTime(hour, minute, second);

      if ( lwarn )
        {
          lwarn = FALSE;
          Warning("Reset wrong date/time to %4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d!",
                  year, month, day, hour, minute, second);
        }
    }

  *date = vdate;
  *time = vtime;
}


void cdiSetForecastPeriod(double timevalue, taxis_t *taxis)
{
  int year, month, day, hour, minute, second;
  int vdate, vtime;
  int timeunit;
  int calendar;
  int julday, secofday, days, secs;

  (*taxis).fc_period = timevalue;

  timeunit = (*taxis).fc_unit;
  calendar = (*taxis).calendar;

  vdate = (*taxis).vdate;
  vtime = (*taxis).vtime;

  if ( vdate == 0 && vtime == 0 && DBL_IS_EQUAL(timevalue, 0.) ) return;

  cdiDecodeDate(vdate, &year, &month, &day);
  cdiDecodeTime(vtime, &hour, &minute, &second);

  if ( timeunit == TUNIT_MONTH && calendar == CALENDAR_360DAYS )
    {
      timeunit = TUNIT_DAY;
      timevalue *= 30;
    }

  if ( timeunit == TUNIT_MONTH || timeunit == TUNIT_YEAR )
    {
      int nmon, dpm;
      double fmon;

      if ( timeunit == TUNIT_YEAR ) timevalue *= 12;

      nmon = (int) timevalue;
      fmon = timevalue - nmon;

      month -= nmon;

      while ( month > 12 ) { month -= 12; year++; }
      while ( month < 1 ) { month += 12; year--; }

      dpm = days_per_month(calendar, year, month);
      timeunit = TUNIT_DAY;
      timevalue = fmon*dpm;
    }

  encode_caldaysec(calendar, year, month, day, hour, minute, second, &julday, &secofday);

  cdiDecodeTimevalue(timeunit, timevalue, &days, &secs);

  julday_add(-days, -secs, &julday, &secofday);

  decode_caldaysec(calendar, julday, secofday, &year, &month, &day, &hour, &minute, &second);

  (*taxis).fdate = cdiEncodeDate(year, month, day);
  (*taxis).ftime = cdiEncodeTime(hour, minute, second);
}


void cdiDecodeTimeval(double timevalue, taxis_t *taxis, int *date, int *time)
{
  if ( taxis->type == TAXIS_ABSOLUTE )
    splitTimevalue(timevalue, taxis->unit, date, time);
  else
    timeval2vtime(timevalue, taxis, date, time);
}


double cdiEncodeTimeval(int date, int time, taxis_t *taxis)
{
  double timevalue;

  if ( taxis->type == TAXIS_ABSOLUTE )
    {
      if ( taxis->unit == TUNIT_YEAR )
        {
          int year, month, day;
          cdiDecodeDate(date, &year, &month, &day);

          timevalue = year;
        }
      else if ( taxis->unit == TUNIT_MONTH )
        {
          int year, month, day;
          cdiDecodeDate(date, &year, &month, &day);
          timevalue = date/100;
          if ( day != 0 )
            {
              if ( date < 0 ) timevalue -= 0.5;
              else timevalue += 0.5;
            }
        }
      else
        {
          int hour, minute, second;
          cdiDecodeTime(time, &hour, &minute, &second);
          if ( date < 0 )
            timevalue = -(-date + (hour*3600 + minute*60 + second)/86400.);
          else
            timevalue = date + (hour*3600 + minute*60 + second)/86400.;
        }
    }
  else
    timevalue = vtime2timeval(date, time, taxis);

  return (timevalue);
}


void ptaxisInit(taxis_t *taxisptr)
{
  taxisDefaultValue ( taxisptr );
}


void ptaxisCopy(taxis_t *dest, taxis_t *source)
{
  reshLock ();


  dest->used = source->used;
  dest->type = source->type;
  dest->vdate = source->vdate;
  dest->vtime = source->vtime;
  dest->rdate = source->rdate;
  dest->rtime = source->rtime;
  dest->fdate = source->fdate;
  dest->ftime = source->ftime;
  dest->calendar = source->calendar;
  dest->unit = source->unit;
  dest->numavg = source->numavg;
  dest->climatology = source->climatology;
  dest->has_bounds = source->has_bounds;
  dest->vdate_lb = source->vdate_lb;
  dest->vtime_lb = source->vtime_lb;
  dest->vdate_ub = source->vdate_ub;
  dest->vtime_ub = source->vtime_ub;
  dest->fc_unit = source->fc_unit;
  dest->fc_period = source->fc_period;

  dest->climatology = source->climatology;
  delete_refcount_string(dest->name);
  delete_refcount_string(dest->longname);
  dest->name = dup_refcount_string(source->name);
  dest->longname = dup_refcount_string(source->longname);
  if (dest->self != CDI_UNDEFID)
    reshSetStatus(dest->self, &taxisOps, RESH_DESYNC_IN_USE);
  reshUnlock ();
}


static void
taxisPrintKernel(taxis_t * taxisptr, FILE * fp)
{
  int vdate_lb, vdate_ub;
  int vtime_lb, vtime_ub;

  taxisInqVdateBounds ( taxisptr->self, &vdate_lb, &vdate_ub);
  taxisInqVtimeBounds ( taxisptr->self, &vtime_lb, &vtime_ub);

  fprintf(fp,
          "#\n"
          "# taxisID %d\n"
          "#\n"
          "self        = %d\n"
          "used        = %d\n"
          "type        = %d\n"
          "vdate       = %d\n"
          "vtime       = %d\n"
          "rdate       = %d\n"
          "rtime       = %d\n"
          "fdate       = %d\n"
          "ftime       = %d\n"
          "calendar    = %d\n"
          "unit        = %d\n"
          "numavg      = %d\n"
          "climatology = %d\n"
          "has_bounds  = %d\n"
          "vdate_lb    = %d\n"
          "vtime_lb    = %d\n"
          "vdate_ub    = %d\n"
          "vtime_ub    = %d\n"
          "fc_unit     = %d\n"
          "fc_period   = %g\n"
          "\n", taxisptr->self, taxisptr->self,
          taxisptr->used, taxisptr->type,
          taxisptr->vdate, taxisptr->vtime,
          taxisptr->rdate, taxisptr->rtime,
          taxisptr->fdate, taxisptr->ftime,
          taxisptr->calendar, taxisptr->unit,
          taxisptr->numavg, taxisptr->climatology,
          taxisptr->has_bounds,
          vdate_lb, vtime_lb, vdate_ub, vtime_ub,
          taxisptr->fc_unit, taxisptr->fc_period );
}

static int
taxisCompareP(void *taxisptr1, void *taxisptr2)
{
  const taxis_t *t1 = ( const taxis_t * ) taxisptr1,
    *t2 = ( const taxis_t * ) taxisptr2;

  xassert ( t1 && t2 );

  return ! ( t1->used == t2->used &&
             t1->type == t2->type &&
             t1->vdate == t2->vdate &&
             t1->vtime == t2->vtime &&
             t1->rdate == t2->rdate &&
             t1->rtime == t2->rtime &&
             t1->fdate == t2->fdate &&
             t1->ftime == t2->ftime &&
             t1->calendar == t2->calendar &&
             t1->unit == t2->unit &&
             t1->fc_unit == t2->fc_unit &&
             t1->numavg == t2->numavg &&
             t1->climatology == t2->climatology &&
             t1->has_bounds == t2->has_bounds &&
             t1->vdate_lb == t2->vdate_lb &&
             t1->vtime_lb == t2->vtime_lb &&
             t1->vdate_ub == t2->vdate_ub &&
             t1->vtime_ub == t2->vtime_ub );
}


static int
taxisTxCode ( void )
{
  return TAXIS;
}

enum { taxisNint = 21 };

static int
taxisGetPackSize(void *p, void *context)
{
  taxis_t *taxisptr = (taxis_t*) p;
  int packBufferSize
    = serializeGetSize(taxisNint, DATATYPE_INT, context)
    + serializeGetSize(1, DATATYPE_UINT32, context)
    + (taxisptr->name ?
       serializeGetSize((int)strlen(taxisptr->name), DATATYPE_TXT, context) : 0)
    + (taxisptr->longname ?
       serializeGetSize((int)strlen(taxisptr->longname), DATATYPE_TXT,
                        context) : 0);
  return packBufferSize;
}

int
taxisUnpack(char * unpackBuffer, int unpackBufferSize, int * unpackBufferPos,
            int originNamespace, void *context, int force_id)
{
  taxis_t * taxisP;
  int intBuffer[taxisNint];
  uint32_t d;
  int idx = 0;

  serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                  intBuffer, taxisNint, DATATYPE_INT, context);
  serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                  &d, 1, DATATYPE_UINT32, context);

  xassert(cdiCheckSum(DATATYPE_INT, taxisNint, intBuffer) == d);

  taxisInit();

  cdiResH targetID = namespaceAdaptKey(intBuffer[idx++], originNamespace);
  taxisP = taxisNewEntry(force_id?targetID:CDI_UNDEFID);

  xassert(!force_id || targetID == taxisP->self);

  taxisP->used = (short)intBuffer[idx++];
  taxisP->type = intBuffer[idx++];
  taxisP->vdate = intBuffer[idx++];
  taxisP->vtime = intBuffer[idx++];
  taxisP->rdate = intBuffer[idx++];
  taxisP->rtime = intBuffer[idx++];
  taxisP->fdate = intBuffer[idx++];
  taxisP->ftime = intBuffer[idx++];
  taxisP->calendar = intBuffer[idx++];
  taxisP->unit = intBuffer[idx++];
  taxisP->fc_unit = intBuffer[idx++];
  taxisP->numavg = intBuffer[idx++];
  taxisP->climatology = intBuffer[idx++];
  taxisP->has_bounds = (short)intBuffer[idx++];
  taxisP->vdate_lb = intBuffer[idx++];
  taxisP->vtime_lb = intBuffer[idx++];
  taxisP->vdate_ub = intBuffer[idx++];
  taxisP->vtime_ub = intBuffer[idx++];

  if (intBuffer[idx])
    {
      int len = intBuffer[idx];
      char *name = new_refcount_string((size_t)len);
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      name, len, DATATYPE_TXT, context);
      name[len] = '\0';
      taxisP->name = name;
    }
  idx++;
  if (intBuffer[idx])
    {
      int len = intBuffer[idx];
      char *longname = new_refcount_string((size_t)len);
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      longname, len, DATATYPE_TXT, context);
      longname[len] = '\0';
      taxisP->longname = longname;
    }

  reshSetStatus(taxisP->self, &taxisOps,
                reshGetStatus(taxisP->self, &taxisOps) & ~RESH_SYNC_BIT);

  return taxisP->self;
}


static void
taxisPack(void * voidP, void * packBuffer, int packBufferSize, int * packBufferPos,
          void *context)
{
  taxis_t *taxisP = (taxis_t *)voidP;
  int intBuffer[taxisNint];
  uint32_t d;
  int idx = 0;

  intBuffer[idx++] = taxisP->self;
  intBuffer[idx++] = taxisP->used;
  intBuffer[idx++] = taxisP->type;
  intBuffer[idx++] = taxisP->vdate;
  intBuffer[idx++] = taxisP->vtime;
  intBuffer[idx++] = taxisP->rdate;
  intBuffer[idx++] = taxisP->rtime;
  intBuffer[idx++] = taxisP->fdate;
  intBuffer[idx++] = taxisP->ftime;
  intBuffer[idx++] = taxisP->calendar;
  intBuffer[idx++] = taxisP->unit;
  intBuffer[idx++] = taxisP->fc_unit;
  intBuffer[idx++] = taxisP->numavg;
  intBuffer[idx++] = taxisP->climatology;
  intBuffer[idx++] = taxisP->has_bounds;
  intBuffer[idx++] = taxisP->vdate_lb;
  intBuffer[idx++] = taxisP->vtime_lb;
  intBuffer[idx++] = taxisP->vdate_ub;
  intBuffer[idx++] = taxisP->vtime_ub;
  intBuffer[idx++] = taxisP->name ? (int)strlen(taxisP->name) : 0;
  intBuffer[idx++] = taxisP->longname ? (int)strlen(taxisP->longname) : 0;

  serializePack(intBuffer, taxisNint, DATATYPE_INT,
                packBuffer, packBufferSize, packBufferPos, context);
  d = cdiCheckSum(DATATYPE_INT, taxisNint, intBuffer);
  serializePack(&d, 1, DATATYPE_UINT32,
                packBuffer, packBufferSize, packBufferPos, context);
  if (taxisP->name)
    serializePack(taxisP->name, intBuffer[15], DATATYPE_TXT,
                  packBuffer, packBufferSize, packBufferPos, context);
  if (taxisP->longname)
    serializePack(taxisP->longname, intBuffer[16], DATATYPE_TXT,
                  packBuffer, packBufferSize, packBufferPos, context);

}
#include <stdio.h>
#include <stdint.h>
#include <math.h>



void decode_julday(int calendar,
                   int julday,
                   int *year,
                   int *mon,
                   int *day)
{
  int a = julday;
  double b, c;
  double d, e, f;

  b = floor((a - 1867216.25)/36524.25);
  c = a + b - floor(b/4) + 1525;

  if ( calendar == CALENDAR_STANDARD )
    if ( a < 2299161 )
      {
        c = a + 1524;
      }

  d = floor((c - 122.1)/365.25);
  e = floor(365.25*d);
  f = floor((c - e)/30.6001);

  *day = (int)(c - e - floor(30.6001*f));
  *mon = (int)(f - 1 - 12*floor(f/14));
  *year = (int)(d - 4715 - floor((7 + *mon)/10));
}



int encode_julday(int calendar, int year, int month, int day)
{
  int iy;
  int im;
  int ib;
  int julday;

  if ( month <= 2 )
    {
      iy = year - 1;
      im = month + 12;
    }
  else
    {
      iy = year;
      im = month;
    }


  if ( iy < 0 )
    ib = (int)((iy+1)/400) - (int)((iy+1)/100);
  else
    ib = (int)(iy/400) - (int)(iy/100);

  if ( calendar == CALENDAR_STANDARD )
    {
      if ( year > 1582 || (year == 1582 && (month > 10 || (month == 10 && day >= 15))) )
        {



        }
      else
        {



          ib = -2;
        }
    }

  julday = (int) (floor(365.25*iy) + (int)(30.6001*(im+1)) + ib + 1720996.5 + day + 0.5);

  return (julday);
}


int date_to_julday(int calendar, int date)
{
  int julday;
  int year, month, day;

  cdiDecodeDate(date, &year, &month, &day);

  julday = encode_julday(calendar, year, month, day);

  return (julday);
}


int julday_to_date(int calendar, int julday)
{
  int date;
  int year, month, day;

  decode_julday(calendar, julday, &year, &month, &day);

  date = cdiEncodeDate(year, month, day);

  return (date);
}


int time_to_sec(int time)
{
  int secofday;
  int hour, minute, second;

  cdiDecodeTime(time, &hour, &minute, &second);

  secofday = hour*3600 + minute*60 + second;

  return (secofday);
}


int sec_to_time(int secofday)
{
  int time;
  int hour, minute, second;

  hour = secofday/3600;
  minute = secofday/60 - hour*60;
  second = secofday - hour*3600 - minute*60;

  time = cdiEncodeTime(hour, minute, second);

  return (time);
}

static
void adjust_seconds(int *julday, int64_t *secofday)
{
  int64_t secperday = 86400;

  while ( *secofday >= secperday )
    {
      *secofday -= secperday;
      (*julday)++;
    }

  while ( *secofday < 0 )
    {
      *secofday += secperday;
      (*julday)--;
    }
}


void julday_add_seconds(int64_t seconds, int *julday, int *secofday)
{
  int64_t sec_of_day = *secofday;

  sec_of_day += seconds;

  adjust_seconds(julday, &sec_of_day);

  *secofday = (int) sec_of_day;
}


void julday_add(int days, int secs, int *julday, int *secofday)
{
  int64_t sec_of_day = *secofday;

  sec_of_day += secs;
  *julday += days;

  adjust_seconds(julday, &sec_of_day);

  *secofday = (int) sec_of_day;
}


double julday_sub(int julday1, int secofday1, int julday2, int secofday2, int *days, int *secs)
{
  int64_t sec_of_day;
  int64_t seconds;

  *days = julday2 - julday1;
  *secs = secofday2 - secofday1;

  sec_of_day = *secs;

  adjust_seconds(days, &sec_of_day);

  *secs = (int) sec_of_day;

  seconds = *days * 86400 + sec_of_day;

  return (double)seconds;
}


void encode_juldaysec(int calendar, int year, int month, int day, int hour, int minute, int second, int *julday, int *secofday)
{
  *julday = encode_julday(calendar, year, month, day);

  *secofday = (hour*60 + minute)*60 + second;
}


void decode_juldaysec(int calendar, int julday, int secofday, int *year, int *month, int *day, int *hour, int *minute, int *second)
{
  decode_julday(calendar, julday, year, month, day);

  *hour = secofday/3600;
  *minute = secofday/60 - *hour*60;
  *second = secofday - *hour*3600 - *minute*60;
}


#include <limits.h>

static
void tstepsInitEntry(stream_t *streamptr, size_t tsID)
{
  streamptr->tsteps[tsID].curRecID = CDI_UNDEFID;
  streamptr->tsteps[tsID].position = 0;
  streamptr->tsteps[tsID].records = NULL;
  streamptr->tsteps[tsID].recordSize = 0;
  streamptr->tsteps[tsID].nallrecs = 0;
  streamptr->tsteps[tsID].recIDs = NULL;
  streamptr->tsteps[tsID].nrecs = 0;
  streamptr->tsteps[tsID].next = 0;

  ptaxisInit(&streamptr->tsteps[tsID].taxis);
}


int tstepsNewEntry(stream_t *streamptr)
{
  size_t tsID = (size_t)streamptr->tstepsNextID++;
  size_t tstepsTableSize = (size_t)streamptr->tstepsTableSize;
  tsteps_t *tstepsTable = streamptr->tsteps;




  if ( tsID == tstepsTableSize )
    {
      if ( tstepsTableSize == 0 ) tstepsTableSize = 1;
      if ( tstepsTableSize <= INT_MAX / 2)
        tstepsTableSize *= 2;
      else if ( tstepsTableSize < INT_MAX)
        tstepsTableSize = INT_MAX;
      else
        Error("Resizing of tstep table failed!");
      tstepsTable = (tsteps_t *) Realloc(tstepsTable,
                                         tstepsTableSize * sizeof (tsteps_t));
    }

  streamptr->tstepsTableSize = (int)tstepsTableSize;
  streamptr->tsteps = tstepsTable;

  tstepsInitEntry(streamptr, tsID);

  streamptr->tsteps[tsID].taxis.used = TRUE;

  return (int)tsID;
}


void cdiCreateTimesteps(stream_t *streamptr)
{
  long ntsteps;
  long tsID;

  if ( streamptr->ntsteps < 0 || streamptr->tstepsTableSize > 0 )
    return;

  if ( streamptr->ntsteps == 0 ) ntsteps = 1;
  else ntsteps = streamptr->ntsteps;

  streamptr->tsteps = (tsteps_t *) Malloc((size_t)ntsteps*sizeof(tsteps_t));

  streamptr->tstepsTableSize = (int)ntsteps;
  streamptr->tstepsNextID = (int)ntsteps;

  for ( tsID = 0; tsID < ntsteps; tsID++ )
    {
      tstepsInitEntry(streamptr, (size_t)tsID);
      streamptr->tsteps[tsID].taxis.used = TRUE;
    }
}
#ifndef CREATE_UUID_H
#define CREATE_UUID_H

#if defined (HAVE_CONFIG_H)
#endif


void
create_uuid(unsigned char uuid[CDI_UUID_SIZE]);

#endif
#if defined (HAVE_CONFIG_H)
#endif

#define _XOPEN_SOURCE 600

#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>



void cdiPrintDatatypes(void)
{
#define XSTRING(x) #x
#define STRING(x) XSTRING(x)
  fprintf (stderr, "+-------------+-------+\n"
           "| types       | bytes |\n"
           "+-------------+-------+\n"
           "| void *      |   %3d |\n"
           "+-------------+-------+\n"
           "| char        |   %3d |\n"
           "+-------------+-------+\n"
           "| short       |   %3d |\n"
           "| int         |   %3d |\n"
           "| long        |   %3d |\n"
           "| long long   |   %3d |\n"
           "| size_t      |   %3d |\n"
           "| off_t       |   %3d |\n"
           "+-------------+-------+\n"
           "| float       |   %3d |\n"
           "| double      |   %3d |\n"
           "| long double |   %3d |\n"
           "+-------------+-------+\n\n"
           "+-------------+-----------+\n"
           "| INT32       | %-9s |\n"
           "| INT64       | %-9s |\n"
           "| FLT32       | %-9s |\n"
           "| FLT64       | %-9s |\n"
           "+-------------+-----------+\n"
           "\n  byte ordering is %s\n\n",
           (int) sizeof(void *), (int) sizeof(char),
           (int) sizeof(short), (int) sizeof(int), (int) sizeof(long), (int) sizeof(long long),
           (int) sizeof(size_t), (int) sizeof(off_t),
           (int) sizeof(float), (int) sizeof(double), (int) sizeof(long double),
           STRING(INT32), STRING(INT64), STRING(FLT32), STRING(FLT64),
           ((HOST_ENDIANNESS == CDI_BIGENDIAN) ? "BIGENDIAN"
            : ((HOST_ENDIANNESS == CDI_LITTLEENDIAN) ? "LITTLEENDIAN"
               : "Unhandled endianness!")));
#undef STRING
#undef XSTRING
}

static const char uuidFmt[] = "%02hhx%02hhx%02hhx%02hhx-"
  "%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-"
  "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx";

enum {
  uuidNumHexChars = 36,
};

void uuid2str(const unsigned char *uuid, char *uuidstr)
{

  if ( uuid == NULL || uuidstr == NULL ) return;

  int iret = sprintf(uuidstr, uuidFmt,
                     uuid[0], uuid[1], uuid[2], uuid[3],
                     uuid[4], uuid[5], uuid[6], uuid[7],
                     uuid[8], uuid[9], uuid[10], uuid[11],
                     uuid[12], uuid[13], uuid[14], uuid[15]);

  if ( iret != uuidNumHexChars ) uuidstr[0] = 0;
}


int str2uuid(const char *uuidstr, unsigned char *uuid)
{
  if ( uuid == NULL || uuidstr == NULL || strlen(uuidstr) != uuidNumHexChars)
    return -1;

  int iret = sscanf(uuidstr, uuidFmt,
                    &uuid[0], &uuid[1], &uuid[2], &uuid[3],
                    &uuid[4], &uuid[5], &uuid[6], &uuid[7],
                    &uuid[8], &uuid[9], &uuid[10], &uuid[11],
                    &uuid[12], &uuid[13], &uuid[14], &uuid[15]);
  if ( iret != CDI_UUID_SIZE ) return -1;
  return iret;
}


char* cdiEscapeSpaces(const char* string)
{

  size_t escapeCount = 0, length = 0;
  for(; string[length]; ++length)
    escapeCount += string[length] == ' ' || string[length] == '\\';

  char* result = (char *) Malloc(length + escapeCount + 1);
  if(!result) return NULL;


  for(size_t in = 0, out = 0; in < length; ++out, ++in)
    {
      if(string[in] == ' ' || string[in] == '\\') result[out++] = '\\';
      result[out] = string[in];
    }
  result[length + escapeCount] = 0;
  return result;
}




char* cdiUnescapeSpaces(const char* string, const char** outStringEnd)
{

  size_t escapeCount = 0, length = 0;
  for(const char* current = string; *current && *current != ' '; current++)
    {
      if(*current == '\\')
        {
          current++, escapeCount++;
          if(!current) return NULL;
        }
      length++;
    }

  char* result = (char *) Malloc(length + 1);
  if(!result) return NULL;


  for(size_t in = 0, out = 0; out < length;)
    {
      if(string[in] == '\\') in++;
      result[out++] = string[in++];
    }
  result[length] = 0;
  if(outStringEnd) *outStringEnd = &string[length + escapeCount];
  return result;
}

#ifdef HAVE_DECL_UUID_GENERATE
#include <sys/time.h>
#include <uuid/uuid.h>
void
create_uuid(unsigned char *uuid)
{
  static int uuid_seeded = 0;
  static char uuid_rand_state[31 * sizeof (long)];
  char *caller_rand_state;
  if (uuid_seeded)
    caller_rand_state = setstate(uuid_rand_state);
  else
    {
      struct timeval tv;
      int status = gettimeofday(&tv, NULL);
      if (status != 0)
        {
          perror("uuid random seed generation failed!");
          exit(1);
        }
      unsigned seed = (unsigned)(tv.tv_sec ^ tv.tv_usec);
      caller_rand_state = initstate(seed, uuid_rand_state,
                                    sizeof (uuid_rand_state));
      uuid_seeded = 1;
    }
  uuid_generate(uuid);
  setstate(caller_rand_state);
}
#elif defined (HAVE_DECL_UUID_CREATE)
typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
#include <uuid.h>
void
create_uuid(unsigned char *uuid)
{
  uint32_t status;
  uuid_create((uuid_t *)(void *)uuid, &status);
  if (status != uuid_s_ok)
    {
      perror("uuid generation failed!");
      exit(1);
    }
}
#else
#include <sys/time.h>
void
create_uuid(unsigned char *uuid)
{
  static int uuid_seeded = 0;
  static char uuid_rand_state[31 * sizeof (long)];
  char *caller_rand_state;
  if (uuid_seeded)
    caller_rand_state = setstate(uuid_rand_state);
  else
    {
      struct timeval tv;
      int status = gettimeofday(&tv, NULL);
      if (status != 0)
        {
          perror("failed seed generation!");
          exit(1);
        }
      unsigned seed = tv.tv_sec ^ tv.tv_usec;
      caller_rand_state = initstate(seed, uuid_rand_state,
                                    sizeof (uuid_rand_state));
      uuid_seeded = 1;
    }
  for (size_t i = 0; i < CDI_UUID_SIZE; ++i)
    uuid[i] = (unsigned char)random();

  uuid[8] = (unsigned char)((uuid[8] & 0x3f) | (1 << 7));

  uuid[7] = (unsigned char)((uuid[7] & 0x0f) | (4 << 4));
  setstate(caller_rand_state);
}
#endif
#ifndef _ZAXIS_H
#define _ZAXIS_H

void zaxisGetTypeDescription(int zaxisType, int* outPositive, const char** outName, const char** outLongName, const char** outStdName, const char** outUnit);

unsigned cdiZaxisCount(void);

void cdiZaxisGetIndexList(unsigned numIDs, int *IDs);

void
zaxisUnpack(char * unpackBuffer, int unpackBufferSize,
            int * unpackBufferPos, int originNamespace, void *context,
            int force_id);

void zaxisDefLtype2(int zaxisID, int ltype2);

const resOps *getZaxisOps(void);

#endif
#if defined (HAVE_CONFIG_H)
#endif

#include <stdbool.h>
#include <string.h>
#include <math.h>



#undef UNDEFID
#define UNDEFID -1

static size_t Vctsize = 0;
static double *Vct = NULL;

static int numberOfVerticalLevels = 0;
static int numberOfVerticalGrid = 0;
static unsigned char uuidVGrid[CDI_UUID_SIZE];


typedef struct
{
  int level1;
  int level2;
  int recID;
  int lindex;
}
leveltable_t;


typedef struct
{
  int subtypeIndex;

  unsigned nlevels;
  int levelTableSize;
  leveltable_t* levelTable;
} subtypetable_t;


typedef struct
{
  int param;
  int prec;
  int tsteptype;
  int timaccu;
  int gridID;
  int zaxistype;
  int ltype1;
  int ltype2;
  int lbounds;
  int level_sf;
  int level_unit;
  int zaxisID;

  int nsubtypes_alloc;
  int nsubtypes;
  subtypetable_t *recordTable;

  int instID;
  int modelID;
  int tableID;
  int comptype;
  int complevel;
  short timave;
  short lmissval;
  double missval;
  char *name;
  char *stdname;
  char *longname;
  char *units;
  ensinfo_t *ensdata;
  int typeOfGeneratingProcess;
  int productDefinitionTemplate;


  subtype_t *tiles;

  int opt_grib_nentries;
  int opt_grib_kvpair_size;
  opt_key_val_pair_t *opt_grib_kvpair;
}
vartable_t;


static vartable_t *vartable;
static unsigned varTablesize = 0;
static unsigned nvars = 0;


static void
paramInitEntry(unsigned varID, int param)
{
  vartable[varID].param = param;
  vartable[varID].prec = 0;
  vartable[varID].tsteptype = TSTEP_INSTANT;
  vartable[varID].timave = 0;
  vartable[varID].timaccu = 0;
  vartable[varID].gridID = UNDEFID;
  vartable[varID].zaxistype = 0;
  vartable[varID].ltype1 = 0;
  vartable[varID].ltype2 = -1;
  vartable[varID].lbounds = 0;
  vartable[varID].level_sf = 0;
  vartable[varID].level_unit = 0;
  vartable[varID].recordTable = NULL;
  vartable[varID].nsubtypes_alloc= 0;
  vartable[varID].nsubtypes = 0;
  vartable[varID].instID = UNDEFID;
  vartable[varID].modelID = UNDEFID;
  vartable[varID].tableID = UNDEFID;
  vartable[varID].typeOfGeneratingProcess = UNDEFID;
  vartable[varID].productDefinitionTemplate = UNDEFID;
  vartable[varID].comptype = COMPRESS_NONE;
  vartable[varID].complevel = 1;
  vartable[varID].lmissval = 0;
  vartable[varID].missval = 0;
  vartable[varID].name = NULL;
  vartable[varID].stdname = NULL;
  vartable[varID].longname = NULL;
  vartable[varID].units = NULL;
  vartable[varID].ensdata = NULL;
  vartable[varID].tiles = NULL;
}



static unsigned
varGetEntry(int param, int zaxistype, int ltype1, int tsteptype, const char *name, const var_tile_t *tiles)
{
  for ( unsigned varID = 0; varID < varTablesize; varID++ )
    {


      if (vartable[varID].param == param)
        {
          int no_of_tiles = -1;
          if ( tiles ) no_of_tiles = tiles->numberOfTiles;
          int vt_no_of_tiles = -1;
          if ( vartable[varID].tiles )
            vt_no_of_tiles = subtypeGetGlobalDataP(vartable[varID].tiles,
                                                   SUBTYPE_ATT_NUMBER_OF_TILES);
          if ( (vartable[varID].zaxistype == zaxistype) &&
               (vartable[varID].ltype1 == ltype1 ) &&
               (vartable[varID].tsteptype == tsteptype) &&
               (vt_no_of_tiles == no_of_tiles) )
            {
              if ( name && name[0] && vartable[varID].name && vartable[varID].name[0] )
                {
                  if ( strcmp(name, vartable[varID].name) == 0 ) return (varID);
                }
              else
                {
                  return (varID);
                }
            }
        }
    }

  return (unsigned)-1;
}

static
void varFree(void)
{
  if ( CDI_Debug ) Message("call to varFree");

  for ( unsigned varID = 0; varID < nvars; varID++ )
    {
      if ( vartable[varID].recordTable )
        {
          for (int isub=0; isub<vartable[varID].nsubtypes_alloc; isub++)
            Free(vartable[varID].recordTable[isub].levelTable);
          Free(vartable[varID].recordTable);
        }

      if ( vartable[varID].name ) Free(vartable[varID].name);
      if ( vartable[varID].stdname ) Free(vartable[varID].stdname);
      if ( vartable[varID].longname ) Free(vartable[varID].longname);
      if ( vartable[varID].units ) Free(vartable[varID].units);
      if ( vartable[varID].ensdata ) Free(vartable[varID].ensdata);
      if ( vartable[varID].tiles ) subtypeDestroyPtr(vartable[varID].tiles);

      if ( vartable[varID].opt_grib_kvpair )
        {
          for (int i=0; i<vartable[varID].opt_grib_nentries; i++) {
            if ( vartable[varID].opt_grib_kvpair[i].keyword )
              Free(vartable[varID].opt_grib_kvpair[i].keyword);
          }
          Free(vartable[varID].opt_grib_kvpair);
        }
      vartable[varID].opt_grib_nentries = 0;
      vartable[varID].opt_grib_kvpair_size = 0;
      vartable[varID].opt_grib_kvpair = NULL;
    }

  if ( vartable )
    Free(vartable);

  vartable = NULL;
  varTablesize = 0;
  nvars = 0;

  if ( Vct )
    Free(Vct);

  Vct = NULL;
  Vctsize = 0;
}


static int tileGetEntry(unsigned varID, int tile_index)
{
  for (int isub=0; isub<vartable[varID].nsubtypes; isub++)
    if (vartable[varID].recordTable[isub].subtypeIndex == tile_index)
      return isub;
  return CDI_UNDEFID;
}



static int tileNewEntry(int varID)
{
  int tileID = 0;
  if (vartable[varID].nsubtypes_alloc == 0)
    {

      vartable[varID].nsubtypes_alloc = 2;
      vartable[varID].nsubtypes = 0;
      vartable[varID].recordTable =
        (subtypetable_t *) Malloc((size_t)vartable[varID].nsubtypes_alloc * sizeof (subtypetable_t));
      if( vartable[varID].recordTable == NULL )
        SysError("Allocation of leveltable failed!");

      for (int isub = 0; isub<vartable[varID].nsubtypes_alloc; isub++) {
        vartable[varID].recordTable[isub].levelTable = NULL;
        vartable[varID].recordTable[isub].levelTableSize = 0;
        vartable[varID].recordTable[isub].nlevels = 0;
        vartable[varID].recordTable[isub].subtypeIndex = CDI_UNDEFID;
      }
    }
  else
    {

      while(tileID < vartable[varID].nsubtypes_alloc)
        {
          if (vartable[varID].recordTable[tileID].levelTable == NULL) break;
          tileID++;
        }
    }


  if (tileID == vartable[varID].nsubtypes_alloc)
    {
      tileID = vartable[varID].nsubtypes_alloc;
      vartable[varID].nsubtypes_alloc *= 2;
      vartable[varID].recordTable =
        (subtypetable_t *) Realloc(vartable[varID].recordTable,
                                   (size_t)vartable[varID].nsubtypes_alloc * sizeof (subtypetable_t));
      if (vartable[varID].recordTable == NULL)
        SysError("Reallocation of leveltable failed");
      for(int isub=tileID; isub<vartable[varID].nsubtypes_alloc; isub++) {
        vartable[varID].recordTable[isub].levelTable = NULL;
        vartable[varID].recordTable[isub].levelTableSize = 0;
        vartable[varID].recordTable[isub].nlevels = 0;
        vartable[varID].recordTable[isub].subtypeIndex = CDI_UNDEFID;
      }
    }

  return tileID;
}


static int levelNewEntry(unsigned varID, int level1, int level2, int tileID)
{
  int levelID = 0;
  int levelTableSize = vartable[varID].recordTable[tileID].levelTableSize;
  leveltable_t *levelTable = vartable[varID].recordTable[tileID].levelTable;





  if ( ! levelTableSize )
    {
      levelTableSize = 2;
      levelTable = (leveltable_t *) Malloc((size_t)levelTableSize
                                           * sizeof (leveltable_t));
      for ( int i = 0; i < levelTableSize; i++ )
        levelTable[i].recID = UNDEFID;
    }
  else
    {
      while( levelID < levelTableSize
             && levelTable[levelID].recID != UNDEFID )
        ++levelID;
    }



  if( levelID == levelTableSize )
    {
      levelTable = (leveltable_t *) Realloc(levelTable,
                                            (size_t)(levelTableSize *= 2)
                                            * sizeof (leveltable_t));
      for( int i = levelID; i < levelTableSize; i++ )
        levelTable[i].recID = UNDEFID;
    }

  levelTable[levelID].level1 = level1;
  levelTable[levelID].level2 = level2;
  levelTable[levelID].lindex = levelID;

  vartable[varID].recordTable[tileID].nlevels = (unsigned)levelID+1;
  vartable[varID].recordTable[tileID].levelTableSize = levelTableSize;
  vartable[varID].recordTable[tileID].levelTable = levelTable;

  return (levelID);
}

#define UNDEF_PARAM -4711

static unsigned
paramNewEntry(int param)
{
  unsigned varID = 0;





  if ( ! varTablesize )
    {
      varTablesize = 2;
      vartable = (vartable_t *) Malloc((size_t)varTablesize
                                       * sizeof (vartable_t));
      if( vartable == NULL )
        {
          Message("varTablesize = %d", varTablesize);
          SysError("Allocation of vartable failed");
        }

      for( unsigned i = 0; i < varTablesize; i++ )
        {
          vartable[i].param = UNDEF_PARAM;
          vartable[i].opt_grib_kvpair = NULL;
          vartable[i].opt_grib_kvpair_size = 0;
          vartable[i].opt_grib_nentries = 0;
        }
    }
  else
    {
      while( varID < varTablesize )
        {
          if ( vartable[varID].param == UNDEF_PARAM ) break;
          varID++;
        }
    }



  if ( varID == varTablesize )
    {
      vartable = (vartable_t *) Realloc(vartable, (size_t)(varTablesize *= 2)
                                        * sizeof (vartable_t));
      for( unsigned i = varID; i < varTablesize; i++ )
        {
          vartable[i].param = UNDEF_PARAM;
          vartable[i].opt_grib_kvpair = NULL;
          vartable[i].opt_grib_kvpair_size = 0;
          vartable[i].opt_grib_nentries = 0;
        }
    }

  paramInitEntry(varID, param);

  return (varID);
}




static
int varInsertTileSubtype(vartable_t *vptr, const var_tile_t *tiles)
{
  if ( tiles == NULL ) return -1;

  int totalno_of_tileattr_pairs = -1;
  int tileClassification = -1;
  int numberOfTiles = -1;
  int numberOfAttributes = -1;
  int tileindex = -1;
  int attribute = -1;

  if ( tiles )
    {
      totalno_of_tileattr_pairs = tiles->totalno_of_tileattr_pairs;
      tileClassification = tiles->tileClassification;
      numberOfTiles = tiles->numberOfTiles;
      numberOfAttributes = tiles->numberOfAttributes;
      tileindex = tiles->tileindex;
      attribute = tiles->attribute;
    }



  subtype_t *subtype_ptr;
  subtypeAllocate(&subtype_ptr, SUBTYPE_TILES);
  subtypeDefGlobalDataP(subtype_ptr, SUBTYPE_ATT_TOTALNO_OF_TILEATTR_PAIRS, totalno_of_tileattr_pairs);
  subtypeDefGlobalDataP(subtype_ptr, SUBTYPE_ATT_TILE_CLASSIFICATION , tileClassification);
  subtypeDefGlobalDataP(subtype_ptr, SUBTYPE_ATT_NUMBER_OF_TILES , numberOfTiles);





  struct subtype_entry_t *entry = subtypeEntryInsert(subtype_ptr);
  subtypeDefEntryDataP(entry, SUBTYPE_ATT_NUMBER_OF_ATTR, numberOfAttributes);
  subtypeDefEntryDataP(entry, SUBTYPE_ATT_TILEINDEX, tileindex);
  subtypeDefEntryDataP(entry, SUBTYPE_ATT_TILEATTRIBUTE, attribute);

  if (vptr->tiles == NULL) {
    vptr->tiles = subtype_ptr;
    return 0;
  }
  else {
    tilesetInsertP(vptr->tiles, subtype_ptr);
    subtypeDestroyPtr(subtype_ptr);
    return vptr->tiles->nentries - 1;
  }
  return CDI_UNDEFID;
}


void varAddRecord(int recID, int param, int gridID, int zaxistype, int lbounds,
                  int level1, int level2, int level_sf, int level_unit, int prec,
                  int *pvarID, int *plevelID, int tsteptype, int numavg, int ltype1, int ltype2,
                  const char *name, const char *stdname, const char *longname, const char *units,
                  const var_tile_t *tiles, int *tile_index)
{
  unsigned varID = (cdiSplitLtype105 != 1 || zaxistype != ZAXIS_HEIGHT) ?
    varGetEntry(param, zaxistype, ltype1, tsteptype, name, tiles) : (unsigned)UNDEFID;

  if ( varID == (unsigned)UNDEFID )
    {
      nvars++;
      varID = paramNewEntry(param);
      vartable[varID].gridID = gridID;
      vartable[varID].zaxistype = zaxistype;
      vartable[varID].ltype1 = ltype1;
      vartable[varID].ltype2 = ltype2;
      vartable[varID].lbounds = lbounds;
      vartable[varID].level_sf = level_sf;
      vartable[varID].level_unit = level_unit;
      vartable[varID].tsteptype = tsteptype;

      if ( numavg ) vartable[varID].timave = 1;

      if ( name ) if ( name[0] ) vartable[varID].name = strdup(name);
      if ( stdname ) if ( stdname[0] ) vartable[varID].stdname = strdup(stdname);
      if ( longname ) if ( longname[0] ) vartable[varID].longname = strdup(longname);
      if ( units ) if ( units[0] ) vartable[varID].units = strdup(units);
    }
  else
    {
      char paramstr[32];
      cdiParamToString(param, paramstr, sizeof(paramstr));

      if ( vartable[varID].gridID != gridID )
        {
          Message("param = %s gridID = %d", paramstr, gridID);
          Error("horizontal grid must not change for same parameter!");
        }
      if ( vartable[varID].zaxistype != zaxistype )
        {
          Message("param = %s zaxistype = %d", paramstr, zaxistype);
          Error("zaxistype must not change for same parameter!");
        }
    }

  if ( prec > vartable[varID].prec ) vartable[varID].prec = prec;


  int this_tile = varInsertTileSubtype(&vartable[varID], tiles);
  int tileID = tileGetEntry(varID, this_tile);
  if ( tile_index ) (*tile_index) = this_tile;
  if (tileID == CDI_UNDEFID) {
    tileID = tileNewEntry((int)varID);
    vartable[varID].recordTable[tileID].subtypeIndex = this_tile;
    vartable[varID].nsubtypes++;
  }


  int levelID = levelNewEntry(varID, level1, level2, tileID);
  if (CDI_Debug)
    Message("vartable[%d].recordTable[%d].levelTable[%d].recID = %d; level1,2=%d,%d",
            varID, tileID, levelID, recID, level1, level2);
  vartable[varID].recordTable[tileID].levelTable[levelID].recID = recID;

  *pvarID = (int) varID;
  *plevelID = levelID;
}
static
int cmpLevelTable(const void* s1, const void* s2)
{
  int cmp = 0;
  const leveltable_t* x = (const leveltable_t*) s1;
  const leveltable_t* y = (const leveltable_t*) s2;



  if ( x->level1 < y->level1 ) cmp = -1;
  else if ( x->level1 > y->level1 ) cmp = 1;

  return (cmp);
}

static
int cmpLevelTableInv(const void* s1, const void* s2)
{
  int cmp = 0;
  const leveltable_t* x = (const leveltable_t*) s1;
  const leveltable_t* y = (const leveltable_t*) s2;



  if ( x->level1 < y->level1 ) cmp = 1;
  else if ( x->level1 > y->level1 ) cmp = -1;

  return (cmp);
}


typedef struct
{
  int varid;
  int param;
  int ltype;
}
param_t;


static
int cmpparam(const void* s1, const void* s2)
{
  const param_t* x = (const param_t*) s1;
  const param_t* y = (const param_t*) s2;

  int cmp = (( x->param > y->param ) - ( x->param < y->param )) * 2
           + ( x->ltype > y->ltype ) - ( x->ltype < y->ltype );

  return (cmp);
}


void cdi_generate_vars(stream_t *streamptr)
{
  int gridID, zaxisID;

  int instID, modelID, tableID;
  int param, zaxistype, ltype1, ltype2;
  int prec;
  int tsteptype;
  int timave, timaccu;
  int lbounds;
  int comptype;
  char name[CDI_MAX_NAME], longname[CDI_MAX_NAME], units[CDI_MAX_NAME];
  double *dlevels = NULL;
  double *dlevels1 = NULL;
  double *dlevels2 = NULL;
  double level_sf = 1;
  int vlistID = streamptr->vlistID;

  int *varids = (int *) Malloc(nvars*sizeof(int));
  for ( unsigned varID = 0; varID < nvars; varID++ ) varids[varID] = (int)varID;

  if ( streamptr->sortname )
    {
      param_t *varInfo = (param_t *) Malloc((size_t)nvars * sizeof (param_t));

      for ( unsigned varID = 0; varID < nvars; varID++ )
        {
          varInfo[varID].varid = varids[varID];
          varInfo[varID].param = vartable[varID].param;
          varInfo[varID].ltype = vartable[varID].ltype1;
        }
      qsort(varInfo, (size_t)nvars, sizeof(param_t), cmpparam);
      for ( unsigned varID = 0; varID < nvars; varID++ )
        {
          varids[varID] = varInfo[varID].varid;
        }
      Free(varInfo);
    }

  for ( unsigned index = 0; index < nvars; index++ )
    {
      int varid = varids[index];

      gridID = vartable[varid].gridID;
      param = vartable[varid].param;
      ltype1 = vartable[varid].ltype1;
      ltype2 = vartable[varid].ltype2;
      zaxistype = vartable[varid].zaxistype;
      if ( ltype1 == 0 && zaxistype == ZAXIS_GENERIC && cdiDefaultLeveltype != -1 )
        zaxistype = cdiDefaultLeveltype;
      lbounds = vartable[varid].lbounds;
      prec = vartable[varid].prec;
      instID = vartable[varid].instID;
      modelID = vartable[varid].modelID;
      tableID = vartable[varid].tableID;
      tsteptype = vartable[varid].tsteptype;
      timave = vartable[varid].timave;
      timaccu = vartable[varid].timaccu;
      comptype = vartable[varid].comptype;

      level_sf = 1;
      if ( vartable[varid].level_sf != 0 ) level_sf = 1./vartable[varid].level_sf;

      zaxisID = UNDEFID;


      unsigned nlevels = vartable[varid].recordTable[0].nlevels;
      for (int isub=1; isub<vartable[varid].nsubtypes; isub++) {
        if (vartable[varid].recordTable[isub].nlevels != nlevels)
          {
            fprintf(stderr, "var \"%s\": isub = %d / %d :: "
                    "nlevels = %d, vartable[varid].recordTable[isub].nlevels = %d\n",
                    vartable[varid].name, isub, vartable[varid].nsubtypes,
                    nlevels, vartable[varid].recordTable[isub].nlevels);
            Error("zaxis size must not change for same parameter!");
          }

        leveltable_t *t1 = vartable[varid].recordTable[isub-1].levelTable;
        leveltable_t *t2 = vartable[varid].recordTable[isub ].levelTable;
        for (unsigned ilev=0; ilev<nlevels; ilev++)
          if ((t1[ilev].level1 != t2[ilev].level1) ||
              (t1[ilev].level2 != t2[ilev].level2) ||
              (t1[ilev].lindex != t2[ilev].lindex))
            {
              fprintf(stderr, "var \"%s\", varID=%d: isub = %d / %d :: "
                      "nlevels = %d, vartable[varid].recordTable[isub].nlevels = %d\n",
                      vartable[varid].name, varid, isub, vartable[varid].nsubtypes,
                      nlevels, vartable[varid].recordTable[isub].nlevels);
              Message("t1[ilev].level1=%d / t2[ilev].level1=%d",t1[ilev].level1, t2[ilev].level1);
              Message("t1[ilev].level2=%d / t2[ilev].level2=%d",t1[ilev].level2, t2[ilev].level2);
              Message("t1[ilev].lindex=%d / t2[ilev].lindex=%d",t1[ilev].lindex, t2[ilev].lindex);
              Error("zaxis type must not change for same parameter!");
            }
      }
      leveltable_t *levelTable = vartable[varid].recordTable[0].levelTable;

      if ( ltype1 == 0 && zaxistype == ZAXIS_GENERIC && nlevels == 1 &&
           levelTable[0].level1 == 0 )
        zaxistype = ZAXIS_SURFACE;

      dlevels = (double *) Malloc(nlevels*sizeof(double));

      if ( lbounds && zaxistype != ZAXIS_HYBRID && zaxistype != ZAXIS_HYBRID_HALF )
        for (unsigned levelID = 0; levelID < nlevels; levelID++ )
          dlevels[levelID] = (level_sf*levelTable[levelID].level1 +
                              level_sf*levelTable[levelID].level2)/2;
      else
        for (unsigned levelID = 0; levelID < nlevels; levelID++ )
          dlevels[levelID] = level_sf*levelTable[levelID].level1;

      if ( nlevels > 1 )
        {
          bool linc = true, ldec = true, lsort = false;
          for (unsigned levelID = 1; levelID < nlevels; levelID++ )
            {

              linc &= (dlevels[levelID] > dlevels[levelID-1]);

              ldec &= (dlevels[levelID] < dlevels[levelID-1]);
            }





          if ( !ldec && zaxistype == ZAXIS_PRESSURE )
            {
              qsort(levelTable, nlevels, sizeof(leveltable_t), cmpLevelTableInv);
              lsort = true;
            }





          else if ( (!linc && !ldec) ||
                    zaxistype == ZAXIS_HYBRID ||
                    zaxistype == ZAXIS_DEPTH_BELOW_LAND )
            {
              qsort(levelTable, nlevels, sizeof(leveltable_t), cmpLevelTable);
              lsort = true;
            }

          if ( lsort )
            {
              if ( lbounds && zaxistype != ZAXIS_HYBRID && zaxistype != ZAXIS_HYBRID_HALF )
                for (unsigned levelID = 0; levelID < nlevels; levelID++ )
                  dlevels[levelID] = (level_sf*levelTable[levelID].level1 +
                                      level_sf*levelTable[levelID].level2)/2.;
              else
                for (unsigned levelID = 0; levelID < nlevels; levelID++ )
                  dlevels[levelID] = level_sf*levelTable[levelID].level1;
            }
        }

      if ( lbounds )
        {
          dlevels1 = (double *) Malloc(nlevels*sizeof(double));
          for (unsigned levelID = 0; levelID < nlevels; levelID++)
            dlevels1[levelID] = level_sf*levelTable[levelID].level1;
          dlevels2 = (double *) Malloc(nlevels*sizeof(double));
          for (unsigned levelID = 0; levelID < nlevels; levelID++)
            dlevels2[levelID] = level_sf*levelTable[levelID].level2;
        }

      const char *unitptr = cdiUnitNamePtr(vartable[varid].level_unit);
      zaxisID = varDefZaxis(vlistID, zaxistype, (int)nlevels, dlevels, lbounds, dlevels1, dlevels2,
                            (int)Vctsize, Vct, NULL, NULL, unitptr, 0, 0, ltype1);

      if ( ltype1 != ltype2 && ltype2 != -1 )
        {
          zaxisDefLtype2(zaxisID, ltype2);
        }

      if ( zaxisInqType(zaxisID) == ZAXIS_REFERENCE )
        {
          if ( numberOfVerticalLevels > 0 ) zaxisDefNlevRef(zaxisID, numberOfVerticalLevels);
          if ( numberOfVerticalGrid > 0 ) zaxisDefNumber(zaxisID, numberOfVerticalGrid);
          if ( !cdiUUIDIsNull(uuidVGrid) ) zaxisDefUUID(zaxisID, uuidVGrid);
        }

      if ( lbounds ) Free(dlevels1);
      if ( lbounds ) Free(dlevels2);
      Free(dlevels);


      int tilesetID = CDI_UNDEFID;
      if ( vartable[varid].tiles ) tilesetID = vlistDefTileSubtype(vlistID, vartable[varid].tiles);


      int varID = stream_new_var(streamptr, gridID, zaxisID, tilesetID);
      varID = vlistDefVarTiles(vlistID, gridID, zaxisID, tsteptype, tilesetID);

      vlistDefVarParam(vlistID, varID, param);
      vlistDefVarDatatype(vlistID, varID, prec);
      vlistDefVarTimave(vlistID, varID, timave);
      vlistDefVarTimaccu(vlistID, varID, timaccu);
      vlistDefVarCompType(vlistID, varID, comptype);

      if ( vartable[varid].typeOfGeneratingProcess != UNDEFID )
        vlistDefVarTypeOfGeneratingProcess(vlistID, varID, vartable[varid].typeOfGeneratingProcess);

      if ( vartable[varid].productDefinitionTemplate != UNDEFID )
        vlistDefVarProductDefinitionTemplate(vlistID, varID, vartable[varid].productDefinitionTemplate);

      if ( vartable[varid].lmissval ) vlistDefVarMissval(vlistID, varID, vartable[varid].missval);
      if ( vartable[varid].name ) vlistDefVarName(vlistID, varID, vartable[varid].name);
      if ( vartable[varid].stdname ) vlistDefVarStdname(vlistID, varID, vartable[varid].stdname);
      if ( vartable[varid].longname ) vlistDefVarLongname(vlistID, varID, vartable[varid].longname);
      if ( vartable[varid].units ) vlistDefVarUnits(vlistID, varID, vartable[varid].units);

      if ( vartable[varid].ensdata ) vlistDefVarEnsemble(vlistID, varID, vartable[varid].ensdata->ens_index,
                                                          vartable[varid].ensdata->ens_count,
                                                          vartable[varid].ensdata->forecast_init_type);

      int i;
      vlist_t *vlistptr;
      vlistptr = vlist_to_pointer(vlistID);
      for (i=0; i<vartable[varid].opt_grib_nentries; i++)
        {
          resize_opt_grib_entries(&vlistptr->vars[varID], vlistptr->vars[varID].opt_grib_nentries+1);
          vlistptr->vars[varID].opt_grib_nentries += 1;
          int idx = vlistptr->vars[varID].opt_grib_nentries-1;

          vlistptr->vars[varID].opt_grib_kvpair[idx] = vartable[varid].opt_grib_kvpair[i];
          vlistptr->vars[varID].opt_grib_kvpair[idx].keyword = NULL;
          if (vartable[varid].opt_grib_kvpair[i].keyword)
            vlistptr->vars[varID].opt_grib_kvpair[idx].keyword =
              strdupx(vartable[varid].opt_grib_kvpair[i].keyword);
          vlistptr->vars[varID].opt_grib_kvpair[i].update = TRUE;
        }


      if ( cdiDefaultTableID != UNDEFID )
        {
          int pdis, pcat, pnum;
          cdiDecodeParam(param, &pnum, &pcat, &pdis);
          if ( tableInqParNamePtr(cdiDefaultTableID, pnum) )
            {
              if ( tableID != UNDEFID )
                {
                  strcpy(name, tableInqParNamePtr(cdiDefaultTableID, pnum));
                  vlistDefVarName(vlistID, varID, name);
                  if ( tableInqParLongnamePtr(cdiDefaultTableID, pnum) )
                    {
                      strcpy(longname, tableInqParLongnamePtr(cdiDefaultTableID, pnum));
                      vlistDefVarLongname(vlistID, varID, longname);
                    }
                  if ( tableInqParUnitsPtr(cdiDefaultTableID, pnum) )
                    {
                      strcpy(units, tableInqParUnitsPtr(cdiDefaultTableID, pnum));
                      vlistDefVarUnits(vlistID, varID, units);
                    }
                }
              else
                tableID = cdiDefaultTableID;
            }
          if ( cdiDefaultModelID != UNDEFID ) modelID = cdiDefaultModelID;
          if ( cdiDefaultInstID != UNDEFID ) instID = cdiDefaultInstID;
        }

      if ( instID != UNDEFID ) vlistDefVarInstitut(vlistID, varID, instID);
      if ( modelID != UNDEFID ) vlistDefVarModel(vlistID, varID, modelID);
      if ( tableID != UNDEFID ) vlistDefVarTable(vlistID, varID, tableID);
    }

  for ( unsigned index = 0; index < nvars; index++ )
    {
      int varid = varids[index];
      unsigned nlevels = vartable[varid].recordTable[0].nlevels;
      unsigned nsub = vartable[varid].nsubtypes >= 0
        ? (unsigned)vartable[varid].nsubtypes : 0U;
      for (size_t isub=0; isub < nsub; isub++)
        {
          sleveltable_t *restrict streamRecordTable
            = streamptr->vars[index].recordTable + isub;
          leveltable_t *restrict vartableLevelTable
            = vartable[varid].recordTable[isub].levelTable;
          for (unsigned levelID = 0; levelID < nlevels; levelID++)
            {
              streamRecordTable->recordID[levelID]
                = vartableLevelTable[levelID].recID;
              unsigned lindex;
              for (lindex = 0; lindex < nlevels; lindex++ )
                if ( levelID == (unsigned)vartableLevelTable[lindex].lindex )
                  break;
              if ( lindex == nlevels )
                Error("Internal problem! lindex not found.");
              streamRecordTable->lindex[levelID] = (int)lindex;
            }
        }
    }

  Free(varids);

  varFree();
}


void varDefVCT(size_t vctsize, double *vctptr)
{
  if ( Vct == NULL && vctptr != NULL && vctsize > 0 )
    {
      Vctsize = vctsize;
      Vct = (double *) Malloc(vctsize*sizeof(double));
      memcpy(Vct, vctptr, vctsize*sizeof(double));
    }
}


void varDefZAxisReference(int nhlev, int nvgrid, unsigned char uuid[CDI_UUID_SIZE])
{
  numberOfVerticalLevels = nhlev;
  numberOfVerticalGrid = nvgrid;
  memcpy(uuidVGrid, uuid, CDI_UUID_SIZE);
}


int zaxisCompare(int zaxisID, int zaxistype, int nlevels, int lbounds, const double *levels, const char *longname, const char *units, int ltype1)
{
  int differ = 1;
  int levelID;
  int zlbounds = 0;
  int ltype_is_equal = FALSE;

  if ( ltype1 == zaxisInqLtype(zaxisID) ) ltype_is_equal = TRUE;

  if ( ltype_is_equal && (zaxistype == zaxisInqType(zaxisID) || zaxistype == ZAXIS_GENERIC) )
    {
      if ( zaxisInqLbounds(zaxisID, NULL) > 0 ) zlbounds = 1;
      if ( nlevels == zaxisInqSize(zaxisID) && zlbounds == lbounds )
        {
          const double *dlevels;
          char zlongname[CDI_MAX_NAME];
          char zunits[CDI_MAX_NAME];

          dlevels = zaxisInqLevelsPtr(zaxisID);
          for ( levelID = 0; levelID < nlevels; levelID++ )
            {
              if ( fabs(dlevels[levelID] - levels[levelID]) > 1.e-9 )
                break;
            }

          if ( levelID == nlevels ) differ = 0;

          if ( ! differ )
            {
              zaxisInqLongname(zaxisID, zlongname);
              zaxisInqUnits(zaxisID, zunits);
              if ( longname && zlongname[0] )
                {
                  if ( strcmp(longname, zlongname) != 0 ) differ = 1;
                }
              if ( units && zunits[0] )
                {
                  if ( strcmp(units, zunits) != 0 ) differ = 1;
                }
            }
        }
    }

  return (differ);
}

struct varDefZAxisSearchState
{
  int resIDValue;
  int zaxistype;
  int nlevels;
  int lbounds;
  double *levels;
  const char *longname;
  const char *units;
  int ltype;
};

static enum cdiApplyRet
varDefZAxisSearch(int id, void *res, void *data)
{
  struct varDefZAxisSearchState *state = (struct varDefZAxisSearchState *)data;
  (void)res;
  if (zaxisCompare(id, state->zaxistype, state->nlevels, state->lbounds,
                   state->levels, state->longname, state->units, state->ltype)
      == 0)
    {
      state->resIDValue = id;
      return CDI_APPLY_STOP;
    }
  else
    return CDI_APPLY_GO_ON;
}


int varDefZaxis(int vlistID, int zaxistype, int nlevels, double *levels, int lbounds,
                double *levels1, double *levels2, int vctsize, double *vct, char *name,
                char *longname, const char *units, int prec, int mode, int ltype1)
{




  int zaxisdefined = 0;
  int nzaxis;
  int zaxisID = UNDEFID;
  int zaxisglobdefined = 0;
  vlist_t *vlistptr;

  vlistptr = vlist_to_pointer(vlistID);

  nzaxis = vlistptr->nzaxis;

  if ( mode == 0 )
    for ( int index = 0; index < nzaxis; index++ )
      {
        zaxisID = vlistptr->zaxisIDs[index];

        if ( zaxisCompare(zaxisID, zaxistype, nlevels, lbounds, levels, longname, units, ltype1) == 0 )
          {
            zaxisdefined = 1;
            break;
          }
      }

  if ( ! zaxisdefined )
    {
      struct varDefZAxisSearchState query;
      query.zaxistype = zaxistype;
      query.nlevels = nlevels;
      query.levels = levels;
      query.lbounds = lbounds;
      query.longname = longname;
      query.units = units;
      query.ltype = ltype1;

      if ((zaxisglobdefined
           = (cdiResHFilterApply(getZaxisOps(), varDefZAxisSearch, &query)
              == CDI_APPLY_STOP)))
        zaxisID = query.resIDValue;

      if ( mode == 1 && zaxisglobdefined)
        for (int index = 0; index < nzaxis; index++ )
          if ( vlistptr->zaxisIDs[index] == zaxisID )
            {
              zaxisglobdefined = FALSE;
              break;
            }
    }

  if ( ! zaxisdefined )
    {
      if ( ! zaxisglobdefined )
        {
          zaxisID = zaxisCreate(zaxistype, nlevels);
          zaxisDefLevels(zaxisID, levels);
          if ( lbounds )
            {
              zaxisDefLbounds(zaxisID, levels1);
              zaxisDefUbounds(zaxisID, levels2);
            }

          if ( zaxistype == ZAXIS_HYBRID || zaxistype == ZAXIS_HYBRID_HALF )
            {


              if ( vctsize > 0 )
                zaxisDefVct(zaxisID, vctsize, vct);
              else
                Warning("VCT missing");
            }

          zaxisDefName(zaxisID, name);
          zaxisDefLongname(zaxisID, longname);
          zaxisDefUnits(zaxisID, units);
          zaxisDefPrec(zaxisID, prec);
          zaxisDefLtype(zaxisID, ltype1);
        }

      vlistptr->zaxisIDs[nzaxis] = zaxisID;
      vlistptr->nzaxis++;
    }

  return (zaxisID);
}


void varDefMissval(int varID, double missval)
{
  vartable[varID].lmissval = 1;
  vartable[varID].missval = missval;
}


void varDefCompType(int varID, int comptype)
{
  if ( vartable[varID].comptype == COMPRESS_NONE )
    vartable[varID].comptype = comptype;
}


void varDefCompLevel(int varID, int complevel)
{
  vartable[varID].complevel = complevel;
}


int varInqInst(int varID)
{
  return (vartable[varID].instID);
}


void varDefInst(int varID, int instID)
{
  vartable[varID].instID = instID;
}


int varInqModel(int varID)
{
  return (vartable[varID].modelID);
}


void varDefModel(int varID, int modelID)
{
  vartable[varID].modelID = modelID;
}


int varInqTable(int varID)
{
  return (vartable[varID].tableID);
}


void varDefTable(int varID, int tableID)
{
  vartable[varID].tableID = tableID;
}


void varDefEnsembleInfo(int varID, int ens_idx, int ens_count, int forecast_type)
{
  if ( vartable[varID].ensdata == NULL )
      vartable[varID].ensdata = (ensinfo_t *) Malloc( sizeof( ensinfo_t ) );

  vartable[varID].ensdata->ens_index = ens_idx;
  vartable[varID].ensdata->ens_count = ens_count;
  vartable[varID].ensdata->forecast_init_type = forecast_type;
}


void varDefTypeOfGeneratingProcess(int varID, int typeOfGeneratingProcess)
{
  vartable[varID].typeOfGeneratingProcess = typeOfGeneratingProcess;
}


void varDefProductDefinitionTemplate(int varID, int productDefinitionTemplate)
{
  vartable[varID].productDefinitionTemplate = productDefinitionTemplate;
}
#ifndef VLIST_VAR_H
#define VLIST_VAR_H

#ifdef HAVE_CONFIG_H
#endif

#ifndef _VLIST_H
#endif

int vlistVarGetPackSize(vlist_t *p, int varID, void *context);
void vlistVarPack(vlist_t *p, int varID,
                  char * buffer, int bufferSize, int * pos, void *context);
void vlistVarUnpack(int vlistID,
                    char * buf, int size, int *position, int, void *context);
int vlistVarCompare(vlist_t *a, int varIDA, vlist_t *b, int varIDB);
void vlistDefVarIOrank ( int, int, int );
int vlistInqVarIOrank ( int, int );

void cdiVlistCreateVarLevInfo(vlist_t *vlistptr, int varID);

#endif
#ifndef VLIST_ATT_H
#define VLIST_ATT_H

#ifdef HAVE_CONFIG_H
#endif

int
vlistAttsGetSize(vlist_t *p, int varID, void *context);

void
vlistAttsPack(vlist_t *p, int varID,
              void * buf, int size, int *position, void *context);

void
vlistAttsUnpack(int vlistID, int varID,
                void * buf, int size, int *position, void *context);


#endif
static int VLIST_Debug = 0;

static void vlist_initialize(void);

#if defined (HAVE_LIBPTHREAD)
# include <pthread.h>

static pthread_once_t _vlist_init_thread = PTHREAD_ONCE_INIT;

#define VLIST_INIT() \
  pthread_once(&_vlist_init_thread, vlist_initialize)

#else

static int vlistIsInitialized = 0;

#define VLIST_INIT() \
  if ( !vlistIsInitialized ) vlist_initialize()
#endif


static int
vlist_compare(vlist_t *a, vlist_t *b)
{
  int diff;
  diff = (a->nvars != b->nvars) | (a->ngrids != b->ngrids)
    | (a->nzaxis != b->nzaxis) | (a->instID != b->instID)
    | (a->modelID != b->modelID) | (a->tableID != b->tableID)
    | (a->ntsteps != b->ntsteps) | (a->atts.nelems != b->atts.nelems);
  int local_nvars = a->nvars;
  for (int varID = 0; varID < local_nvars; ++varID)
    diff |= vlistVarCompare(a, varID, b, varID);
  size_t natts = a->atts.nelems;
  for (size_t attID = 0; attID < natts; ++attID)
    diff |= vlist_att_compare(a, CDI_GLOBAL, b, CDI_GLOBAL, (int)attID);
  return diff;
}

static void
vlistPrintKernel(vlist_t *vlistptr, FILE * fp );
static void
vlist_delete(vlist_t *vlistptr);

static int vlistGetSizeP ( void * vlistptr, void *context);
static void vlistPackP ( void * vlistptr, void * buff, int size,
                            int *position, void *context);
static int vlistTxCode ( void );

#if !defined(__cplusplus)
const
#endif
resOps vlistOps = {
  (valCompareFunc)vlist_compare,
  (valDestroyFunc)vlist_delete,
  (valPrintFunc)vlistPrintKernel
  , vlistGetSizeP,
  vlistPackP,
  vlistTxCode
};


vlist_t *vlist_to_pointer(int vlistID)
{
  VLIST_INIT();
  return (vlist_t*) reshGetVal(vlistID, &vlistOps );
}

static
void vlist_init_entry(vlist_t *vlistptr)
{
  vlistptr->locked = 0;
  vlistptr->self = CDI_UNDEFID;
  vlistptr->nvars = 0;
  vlistptr->vars = NULL;
  vlistptr->ngrids = 0;
  vlistptr->nzaxis = 0;
  vlistptr->taxisID = CDI_UNDEFID;
  vlistptr->instID = cdiDefaultInstID;
  vlistptr->modelID = cdiDefaultModelID;
  vlistptr->tableID = cdiDefaultTableID;
  vlistptr->varsAllocated = 0;
  vlistptr->ntsteps = CDI_UNDEFID;
  vlistptr->atts.nalloc = MAX_ATTRIBUTES;
  vlistptr->atts.nelems = 0;
  vlistptr->nsubtypes = 0;
  for (int i=0; i<MAX_SUBTYPES_PS; i++)
    vlistptr->subtypeIDs[i] = CDI_UNDEFID;
}

static
vlist_t *vlist_new_entry(cdiResH resH)
{
  vlist_t *vlistptr = (vlist_t*) Malloc(sizeof(vlist_t));
  vlist_init_entry(vlistptr);
  if (resH == CDI_UNDEFID)
    vlistptr->self = reshPut(vlistptr, &vlistOps);
  else
    {
      vlistptr->self = resH;
      reshReplace(resH, vlistptr, &vlistOps);
    }
  return (vlistptr);
}

static
void vlist_delete_entry(vlist_t *vlistptr)
{
  int idx;

  idx = vlistptr->self;

  reshRemove(idx, &vlistOps );

  Free(vlistptr);

  if ( VLIST_Debug )
    Message("Removed idx %d from vlist list", idx);
}

static
void vlist_initialize(void)
{
  char *env;

  env = getenv("VLIST_DEBUG");
  if ( env ) VLIST_Debug = atoi(env);
#ifndef HAVE_LIBPTHREAD
  vlistIsInitialized = TRUE;
#endif
}

static
void vlist_copy(vlist_t *vlistptr2, vlist_t *vlistptr1)
{
  int vlistID2 = vlistptr2->self;
  memcpy(vlistptr2, vlistptr1, sizeof(vlist_t));
  vlistptr2->atts.nelems = 0;
  vlistptr2->self = vlistID2;
}

void vlist_lock(int vlistID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if ( !vlistptr->locked )
    {
      vlistptr->locked += 1;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


void vlist_unlock(int vlistID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if ( vlistptr->locked )
    {
      vlistptr->locked -= 1;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}
int vlistCreate(void)
{
  cdiInitialize();

  VLIST_INIT();

  vlist_t *vlistptr = vlist_new_entry(CDI_UNDEFID);
  if ( CDI_Debug ) Message("create vlistID = %d", vlistptr->self);
  return (vlistptr->self);
}

static void
vlist_delete(vlist_t *vlistptr)
{
  int vlistID = vlistptr->self;
  if ( CDI_Debug ) Message("call to vlist_delete, vlistID = %d", vlistID);

  vlistDelAtts(vlistID, CDI_GLOBAL);

  int local_nvars = vlistptr->nvars;
  var_t *vars = vlistptr->vars;

  for ( int varID = 0; varID < local_nvars; varID++ )
    {
      if ( vars[varID].levinfo ) Free(vars[varID].levinfo);
      if ( vars[varID].name ) Free(vars[varID].name);
      if ( vars[varID].longname ) Free(vars[varID].longname);
      if ( vars[varID].stdname ) Free(vars[varID].stdname);
      if ( vars[varID].units ) Free(vars[varID].units);
      if ( vars[varID].ensdata ) Free(vars[varID].ensdata);

      if ( vlistptr->vars[varID].opt_grib_kvpair )
        {
          for (int i=0; i<vlistptr->vars[varID].opt_grib_nentries; i++) {
            if ( vlistptr->vars[varID].opt_grib_kvpair[i].keyword )
              Free(vlistptr->vars[varID].opt_grib_kvpair[i].keyword);
          }
          Free(vlistptr->vars[varID].opt_grib_kvpair);
        }
      vlistptr->vars[varID].opt_grib_nentries = 0;
      vlistptr->vars[varID].opt_grib_kvpair_size = 0;
      vlistptr->vars[varID].opt_grib_kvpair = NULL;

      vlistDelAtts(vlistID, varID);
    }

  if ( vars ) Free(vars);

  vlist_delete_entry(vlistptr);
}
void vlistDestroy(int vlistID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if ( vlistptr->locked != 0 )
    Warning("Destroying of a locked object (vlistID=%d) failed!", vlistID);
  else
    vlist_delete(vlistptr);
}

static
void var_copy_entries(var_t *var2, var_t *var1)
{
  if ( var1->name ) var2->name = strdupx(var1->name);
  if ( var1->longname ) var2->longname = strdupx(var1->longname);
  if ( var1->stdname ) var2->stdname = strdupx(var1->stdname);
  if ( var1->units ) var2->units = strdupx(var1->units);
  if ( var1->ensdata )
    {
      var2->ensdata = (ensinfo_t *) Malloc(sizeof(ensinfo_t));
      memcpy(var2->ensdata, var1->ensdata, sizeof(ensinfo_t));
    }

  var2->opt_grib_kvpair_size = 0;
  var2->opt_grib_kvpair = NULL;
  var2->opt_grib_nentries = 0;

  resize_opt_grib_entries(var2, var1->opt_grib_nentries);
  var2->opt_grib_nentries = var1->opt_grib_nentries;
  if ((var2->opt_grib_nentries > 0) && CDI_Debug )
    Message("copy %d optional GRIB keywords", var2->opt_grib_nentries);

  for (int i=0; i<var1->opt_grib_nentries; i++) {
    if ( CDI_Debug ) Message("copy entry \"%s\" ...", var1->opt_grib_kvpair[i].keyword);
    var2->opt_grib_kvpair[i].keyword = NULL;
    if ( var1->opt_grib_kvpair[i].keyword != NULL ) {
      var2->opt_grib_kvpair[i] = var1->opt_grib_kvpair[i];
      var2->opt_grib_kvpair[i].keyword = strdupx(var1->opt_grib_kvpair[i].keyword);
      var2->opt_grib_kvpair[i].update = TRUE;
      if ( CDI_Debug ) Message("done.");
    }
    else {
      if ( CDI_Debug ) Message("not done.");
    }
  }
}
void vlistCopy(int vlistID2, int vlistID1)
{
  vlist_t *vlistptr1 = vlist_to_pointer(vlistID1);
  vlist_t *vlistptr2 = vlist_to_pointer(vlistID2);
  if ( CDI_Debug ) Message("call to vlistCopy, vlistIDs %d -> %d", vlistID1, vlistID2);

  var_t *vars1 = vlistptr1->vars;
  var_t *vars2 = vlistptr2->vars;
  vlist_copy(vlistptr2, vlistptr1);

  vlistCopyVarAtts(vlistID1, CDI_GLOBAL, vlistID2, CDI_GLOBAL);

  if ( vars1 )
    {
      int local_nvars = vlistptr1->nvars;


      size_t n = (size_t)vlistptr2->varsAllocated;
      vars2 = (var_t *) Realloc(vars2, n*sizeof(var_t));
      memcpy(vars2, vars1, n*sizeof(var_t));
      vlistptr2->vars = vars2;

      for ( int varID = 0; varID < local_nvars; varID++ )
        {
          var_copy_entries(&vars2[varID], &vars1[varID]);

          vlistptr2->vars[varID].atts.nelems = 0;
          vlistCopyVarAtts(vlistID1, varID, vlistID2, varID);

          if ( vars1[varID].levinfo )
            {
              n = (size_t)zaxisInqSize(vars1[varID].zaxisID);
              vars2[varID].levinfo = (levinfo_t *) Malloc(n*sizeof(levinfo_t));
              memcpy(vars2[varID].levinfo, vars1[varID].levinfo, n*sizeof(levinfo_t));
            }
        }
    }
}
int vlistDuplicate(int vlistID)
{
  if ( CDI_Debug ) Message("call to vlistDuplicate");

  int vlistIDnew = vlistCreate();
  vlistCopy(vlistIDnew, vlistID);
  return (vlistIDnew);
}


void vlistClearFlag(int vlistID)
{
  int varID, levID;
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  for ( varID = 0; varID < vlistptr->nvars; varID++ )
    {
      vlistptr->vars[varID].flag = FALSE;
      if ( vlistptr->vars[varID].levinfo )
        {
          int nlevs = zaxisInqSize(vlistptr->vars[varID].zaxisID);
          for ( levID = 0; levID < nlevs; levID++ )
            vlistptr->vars[varID].levinfo[levID].flag = FALSE;
        }
    }
}


struct vgzSearchState
{
  int resIDValue;
  int zaxistype;
  int nlevels;
  int lbounds;
  const double *levels;
};

int vlistNvars(int vlistID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  return (vlistptr->nvars);
}


int vlistNrecs(int vlistID)
{
  int nrecs = 0;
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  for ( int varID = 0; varID < vlistptr->nvars; varID++ )
    nrecs += zaxisInqSize(vlistptr->vars[varID].zaxisID);

  return (nrecs);
}


int vlistNumber(int vlistID)
{
  int number, number2, datatype;
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  datatype = vlistptr->vars[0].datatype;
  if ( datatype== DATATYPE_CPX32 || datatype == DATATYPE_CPX64 )
    number = CDI_COMP;
  else
    number = CDI_REAL;

  for ( int varID = 1; varID < vlistptr->nvars; varID++ )
    {
      datatype = vlistptr->vars[varID].datatype;
      if ( datatype == DATATYPE_CPX32 || datatype == DATATYPE_CPX64 )
        number2 = CDI_COMP;
      else
        number2 = CDI_REAL;

      if ( number2 != number )
        {
          number = CDI_BOTH;
          break;
        }
    }

  return (number);
}

int vlistNgrids(int vlistID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  return (vlistptr->ngrids);
}

int vlistNzaxis(int vlistID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  return (vlistptr->nzaxis);
}


int vlistNsubtypes(int vlistID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  return (vlistptr->nsubtypes);
}


void vlistDefNtsteps(int vlistID, int nts)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if (vlistptr->ntsteps != nts)
    {
      vlistptr->ntsteps = nts;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistNtsteps(int vlistID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  return (int)vlistptr->ntsteps;
}

static
void vlistPrintKernel(vlist_t *vlistptr, FILE *fp)
{
  char paramstr[32];

  fprintf ( fp, "#\n# vlistID %d\n#\n", vlistptr->self);

  int local_nvars = vlistptr->nvars;

  fprintf(fp, "nvars    : %d\n"
          "ngrids   : %d\n"
          "nzaxis   : %d\n"
          "nsubtypes: %d\n"
          "taxisID  : %d\n"
          "instID   : %d\n"
          "modelID  : %d\n"
          "tableID  : %d\n",
          local_nvars, vlistptr->ngrids, vlistptr->nzaxis, vlistptr->nsubtypes, vlistptr->taxisID,
          vlistptr->instID, vlistptr->modelID, vlistptr->tableID);

  if ( local_nvars > 0 )
    {
      fprintf(fp, " varID param    gridID zaxisID stypeID tsteptype flag iorank"
              " name     longname         units\n");
      for ( int varID = 0; varID < local_nvars; varID++ )
        {
          int param = vlistptr->vars[varID].param;
          int gridID = vlistptr->vars[varID].gridID;
          int zaxisID = vlistptr->vars[varID].zaxisID;
          int subtypeID = vlistptr->vars[varID].subtypeID;
          int tsteptype = vlistptr->vars[varID].tsteptype;
          const char *name = vlistptr->vars[varID].name;
          const char *longname = vlistptr->vars[varID].longname;
          const char *units = vlistptr->vars[varID].units;
          int flag = vlistptr->vars[varID].flag;
          int iorank = vlistptr->vars[varID].iorank;

          cdiParamToString(param, paramstr, sizeof(paramstr));
          fprintf(fp, "%6d %-8s %6d  %6d  %6d  %6d  %5d %6d %-8s %s [%s]\n",
                  varID, paramstr, gridID, zaxisID, subtypeID, tsteptype, flag, iorank,
                  name?name:"", longname?longname:"", units?units:"");
        }

      fputs("\n"
            " varID  levID fvarID flevID mvarID mlevID  index  dtype  flag  level\n", fp);
      for ( int varID = 0; varID < local_nvars; varID++ )
        {
          int zaxisID = vlistptr->vars[varID].zaxisID;
          int nlevs = zaxisInqSize(zaxisID);
          int fvarID = vlistptr->vars[varID].fvarID;
          int mvarID = vlistptr->vars[varID].mvarID;
          int dtype = vlistptr->vars[varID].datatype;
          for ( int levID = 0; levID < nlevs; levID++ )
            {
              levinfo_t li;
              if (vlistptr->vars[varID].levinfo)
                li = vlistptr->vars[varID].levinfo[levID];
              else
                li = DEFAULT_LEVINFO(levID);
              int flevID = li.flevelID;
              int mlevID = li.mlevelID;
              int index = li.index;
              int flag = li.flag;
              double level = zaxisInqLevel(zaxisID, levID);

              fprintf(fp, "%6d %6d %6d %6d %6d %6d %6d %6d %5d  %.9g\n",
                      varID, levID, fvarID, flevID, mvarID, mlevID, index,
                      dtype, flag, level);
            }
        }

      fputs("\n"
            " varID  size iorank\n", fp);
      for ( int varID = 0; varID < local_nvars; varID++ )
        fprintf(fp, "%3d %8d %6d\n", varID,
                zaxisInqSize(vlistptr->vars[varID].zaxisID)
                * gridInqSize(vlistptr->vars[varID].gridID),
                vlistptr->vars[varID].iorank);
    }
}


void vlistPrint(int vlistID)
{
  if ( vlistID == CDI_UNDEFID ) return;
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  vlistPrintKernel(vlistptr, stdout);
}
void vlistDefTaxis(int vlistID, int taxisID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if (vlistptr->taxisID != taxisID)
    {

      vlistptr->taxisID = taxisID;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}
int vlistInqTaxis(int vlistID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  return (vlistptr->taxisID);
}


void vlistDefTable(int vlistID, int tableID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if (vlistptr->tableID != tableID)
    {
      vlistptr->tableID = tableID;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistInqTable(int vlistID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  return (vlistptr->tableID);
}


void vlistDefInstitut(int vlistID, int instID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if (vlistptr->instID != instID)
    {
      vlistptr->instID = instID;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistInqInstitut(int vlistID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  int instID = vlistptr->instID;

  if ( instID == CDI_UNDEFID )
    {
      instID = vlistInqVarInstitut(vlistID, 0);

      for ( int varID = 1; varID < vlistptr->nvars; varID++ )
        if ( instID != vlistInqVarInstitut(vlistID, varID) )
          {
            instID = CDI_UNDEFID;
            break;
      }
      vlistDefInstitut(vlistID, instID);
    }

  return (instID);
}


void vlistDefModel(int vlistID, int modelID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if (vlistptr->modelID != modelID)
    {
      vlistptr->modelID = modelID;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistInqModel(int vlistID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  int modelID = vlistptr->modelID;

  if ( modelID == CDI_UNDEFID )
    {
      modelID = vlistInqVarModel(vlistID, 0);

      for ( int varID = 1; varID < vlistptr->nvars; varID++ )
        if ( modelID != vlistInqVarModel(vlistID, varID) )
          {
            modelID = CDI_UNDEFID;
            break;
          }

      vlistDefModel(vlistID, modelID);
    }

  return (modelID);
}


int vlistGridsizeMax(int vlistID)
{
  int gridsizemax = 0;
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  for ( int index = 0 ; index < vlistptr->ngrids ; index++ )
    {
      int gridID = vlistptr->gridIDs[index];
      int gridsize = gridInqSize(gridID);
      if ( gridsize > gridsizemax ) gridsizemax = gridsize;
    }

  return (gridsizemax);
}


int vlistGrid(int vlistID, int index)
{
  int gridID = CDI_UNDEFID;
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if ( index < vlistptr->ngrids && index >= 0 )
    gridID = vlistptr->gridIDs[index];

  return (gridID);
}


int vlistGridIndex(int vlistID, int gridID)
{
  int index;
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  for ( index = 0 ; index < vlistptr->ngrids ; index++ )
    if ( gridID == vlistptr->gridIDs[index] ) break;

  if ( index == vlistptr->ngrids ) index = -1;

  return (index);
}


void vlistChangeGridIndex(int vlistID, int index, int gridID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  int gridIDold = vlistptr->gridIDs[index];
  if (gridIDold != gridID)
    {
      vlistptr->gridIDs[index] = gridID;

      int local_nvars = vlistptr->nvars;
      for ( int varID = 0; varID < local_nvars; varID++ )
        if ( vlistptr->vars[varID].gridID == gridIDold )
          vlistptr->vars[varID].gridID = gridID;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


void vlistChangeGrid(int vlistID, int gridID1, int gridID2)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if (gridID1 != gridID2)
    {
      int ngrids = vlistptr->ngrids;
      for ( int index = 0; index < ngrids; index++ )
        {
          if ( vlistptr->gridIDs[index] == gridID1 )
            {
              vlistptr->gridIDs[index] = gridID2;
              break;
            }
        }
      int local_nvars = vlistptr->nvars;
      for ( int varID = 0; varID < local_nvars; varID++ )
        if ( vlistptr->vars[varID].gridID == gridID1 )
          vlistptr->vars[varID].gridID = gridID2;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistZaxis(int vlistID, int index)
{
  int zaxisID = CDI_UNDEFID;
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if ( index < vlistptr->nzaxis && index >= 0 )
    zaxisID = vlistptr->zaxisIDs[index];

  return (zaxisID);
}


int vlistZaxisIndex(int vlistID, int zaxisID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  int index;
  for ( index = 0 ; index < vlistptr->nzaxis ; index++ )
    if ( zaxisID == vlistptr->zaxisIDs[index] ) break;

  if ( index == vlistptr->nzaxis ) index = -1;

  return (index);
}


void vlistChangeZaxisIndex(int vlistID, int index, int zaxisID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  int zaxisIDold = vlistptr->zaxisIDs[index];
  if (zaxisIDold != zaxisID)
    {
      vlistptr->zaxisIDs[index] = zaxisID;

      int nlevs = zaxisInqSize(zaxisID),
        nlevsOld = zaxisInqSize(zaxisIDold);
      int local_nvars = vlistptr->nvars;
      for ( int varID = 0; varID < local_nvars; varID++ )
        if ( vlistptr->vars[varID].zaxisID == zaxisIDold )
          {
            vlistptr->vars[varID].zaxisID = zaxisID;
            if ( vlistptr->vars[varID].levinfo && nlevs != nlevsOld )
              {
                vlistptr->vars[varID].levinfo = (levinfo_t *) Realloc(vlistptr->vars[varID].levinfo, (size_t)nlevs * sizeof (levinfo_t));

                for ( int levID = 0; levID < nlevs; levID++ )
                  vlistptr->vars[varID].levinfo[levID] = DEFAULT_LEVINFO(levID);
              }
          }
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


void vlistChangeZaxis(int vlistID, int zaxisID1, int zaxisID2)
{
  int nlevs1 = zaxisInqSize(zaxisID1), nlevs2 = zaxisInqSize(zaxisID2);
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  int nzaxis = vlistptr->nzaxis;
  for ( int index = 0; index < nzaxis; index++ )
    {
      if ( vlistptr->zaxisIDs[index] == zaxisID1 )
        {
          vlistptr->zaxisIDs[index] = zaxisID2;
          break;
        }
    }

  int local_nvars = vlistptr->nvars;
  for ( int varID = 0; varID < local_nvars; varID++ )
    if ( vlistptr->vars[varID].zaxisID == zaxisID1 )
      {
        vlistptr->vars[varID].zaxisID = zaxisID2;

        if ( vlistptr->vars[varID].levinfo && nlevs2 != nlevs1 )
          {
            vlistptr->vars[varID].levinfo
              = (levinfo_t *) Realloc(vlistptr->vars[varID].levinfo,
                                      (size_t)nlevs2 * sizeof(levinfo_t));

            for ( int levID = 0; levID < nlevs2; levID++ )
              vlistptr->vars[varID].levinfo[levID] = DEFAULT_LEVINFO(levID);
          }
      }
  reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
}


int vlistSubtype(int vlistID, int index)
{
  int subtypeID = CDI_UNDEFID;
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if ( index < vlistptr->nsubtypes && index >= 0 )
    subtypeID = vlistptr->subtypeIDs[index];

  return subtypeID;
}


int vlistSubtypeIndex(int vlistID, int subtypeID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  int index;
  for ( index = 0 ; index < vlistptr->nsubtypes ; index++ )
    if ( subtypeID == vlistptr->subtypeIDs[index] ) break;

  if ( index == vlistptr->nsubtypes ) index = -1;

  return (index);
}


int vlistHasTime(int vlistID)
{
  int hastime = FALSE;
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  for ( int varID = 0; varID < vlistptr->nvars; varID++ )
    if ( vlistptr->vars[varID].tsteptype != TSTEP_CONSTANT )
      {
        hastime = TRUE;
        break;
      }

  return (hastime);
}

enum {
  vlist_nints=6,
};

static int
vlistTxCode ( void )
{
  return VLIST;
}


static
int vlistGetSizeP ( void * vlistptr, void *context)
{
  int txsize, varID;
  vlist_t *p = (vlist_t*) vlistptr;
  txsize = serializeGetSize(vlist_nints, DATATYPE_INT, context);
  txsize += serializeGetSize(1, DATATYPE_LONG, context);
  txsize += vlistAttsGetSize(p, CDI_GLOBAL, context);
  for ( varID = 0; varID < p->nvars; varID++ )
    txsize += vlistVarGetPackSize(p, varID, context);
  return txsize;
}


static
void vlistPackP ( void * vlistptr, void * buf, int size, int *position,
                  void *context )
{
  int varID, tempbuf[vlist_nints];
  vlist_t *p = (vlist_t*) vlistptr;
  tempbuf[0] = p->self;
  tempbuf[1] = p->nvars;
  tempbuf[2] = p->taxisID;
  tempbuf[3] = p->tableID;
  tempbuf[4] = p->instID;
  tempbuf[5] = p->modelID;
  serializePack(tempbuf, vlist_nints, DATATYPE_INT, buf, size, position, context);
  serializePack(&p->ntsteps, 1, DATATYPE_LONG, buf, size, position, context);

  vlistAttsPack(p, CDI_GLOBAL, buf, size, position, context);
  for ( varID = 0; varID < p->nvars; varID++ )
    {
      vlistVarPack(p, varID, (char *)buf, size, position, context);
    }
}

void vlistUnpack(char * buf, int size, int *position, int originNamespace,
                 void *context, int force_id)
{
  int tempbuf[vlist_nints];
  serializeUnpack(buf, size, position, tempbuf, vlist_nints, DATATYPE_INT, context);
  int local_nvars = tempbuf[1];
  int targetID = namespaceAdaptKey(tempbuf[0], originNamespace);
  vlist_t *p = vlist_new_entry(force_id?targetID:CDI_UNDEFID);
  xassert(!force_id || p->self == targetID);
  if (!force_id)
    targetID = p->self;
  p->taxisID = namespaceAdaptKey(tempbuf[2], originNamespace);
  p->tableID = tempbuf[3];
  p->instID = namespaceAdaptKey(tempbuf[4], originNamespace);
  p->modelID = namespaceAdaptKey(tempbuf[5], originNamespace);
  serializeUnpack(buf, size, position, &p->ntsteps, 1, DATATYPE_LONG, context);
  vlistAttsUnpack(targetID, CDI_GLOBAL, buf, size, position, context);
  for (int varID = 0; varID < local_nvars; varID++ )
    vlistVarUnpack(targetID, buf, size, position, originNamespace, context);
  reshSetStatus(targetID, &vlistOps,
                reshGetStatus(targetID, &vlistOps) & ~RESH_SYNC_BIT);
}


void vlist_check_contents(int vlistID)
{
  int index, nzaxis, zaxisID;

  nzaxis = vlistNzaxis(vlistID);

  for ( index = 0; index < nzaxis; index++ )
    {
      zaxisID = vlistZaxis(vlistID, index);
      if ( zaxisInqType(zaxisID) == ZAXIS_GENERIC )
        cdiCheckZaxis(zaxisID);
    }
}



void resize_opt_grib_entries(var_t *var, int nentries)
{
  if (var->opt_grib_kvpair_size >= nentries)
    {
      if ( CDI_Debug )
        Message("data structure has size %d, no resize to %d needed.", var->opt_grib_kvpair_size, nentries);
      return;
    }
  else
    {
      if ( CDI_Debug )
        Message("resize data structure, %d -> %d", var->opt_grib_kvpair_size, nentries);

      int i, new_size;
      new_size = (2*var->opt_grib_kvpair_size) > nentries ? (2*var->opt_grib_kvpair_size) : nentries;
      opt_key_val_pair_t *tmp = (opt_key_val_pair_t *) Malloc((size_t)new_size * sizeof (opt_key_val_pair_t));
      for (i=0; i<var->opt_grib_kvpair_size; i++) {
        tmp[i] = var->opt_grib_kvpair[i];
      }
      for (i=var->opt_grib_kvpair_size; i<new_size; i++) {
        tmp[i].int_val = 0;
        tmp[i].dbl_val = 0;
        tmp[i].update = FALSE;
        tmp[i].keyword = NULL;
      }
      var->opt_grib_kvpair_size = new_size;
      Free(var->opt_grib_kvpair);
      var->opt_grib_kvpair = tmp;
    }
}
#ifdef HAVE_CONFIG_H
#endif

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>



static
cdi_atts_t *get_attsp(vlist_t *vlistptr, int varID)
{
  cdi_atts_t *attsp = NULL;

  if ( varID == CDI_GLOBAL )
    {
      attsp = &vlistptr->atts;
    }
  else
    {
      if ( varID >= 0 && varID < vlistptr->nvars )
        attsp = &(vlistptr->vars[varID].atts);
    }

  return (attsp);
}

static
cdi_att_t *find_att(cdi_atts_t *attsp, const char *name)
{
  xassert(attsp != NULL);

  if ( attsp->nelems == 0 ) return NULL;

  size_t slen = strlen(name);
  if ( slen > CDI_MAX_NAME ) slen = CDI_MAX_NAME;

  cdi_att_t *atts = attsp->value;
  for ( size_t attid = 0; attid < attsp->nelems; attid++ )
    {
      cdi_att_t *attp = atts + attid;
      if ( attp->namesz == slen && memcmp(attp->name, name, slen) == 0 )
        return (attp);
    }

  return (NULL);
}

static
cdi_att_t *new_att(cdi_atts_t *attsp, const char *name)
{
  cdi_att_t *attp;
  size_t slen;

  xassert(attsp != NULL);
  xassert(name != NULL);

  if ( attsp->nelems == attsp->nalloc ) return (NULL);

  attp = &(attsp->value[attsp->nelems]);
  attsp->nelems++;

  slen = strlen(name);
  if ( slen > CDI_MAX_NAME ) slen = CDI_MAX_NAME;

  attp->name = (char *) Malloc(slen+1);
  memcpy(attp->name, name, slen+1);
  attp->namesz = slen;
  attp->xvalue = NULL;

  return (attp);
}

static
void fill_att(cdi_att_t *attp, int indtype, int exdtype, size_t nelems, size_t xsz, const void *xvalue)
{
  xassert(attp != NULL);

  attp->xsz = xsz;
  attp->indtype = indtype;
  attp->exdtype = exdtype;
  attp->nelems = nelems;

  if ( xsz > 0 )
    {
      attp->xvalue = Realloc(attp->xvalue, xsz);
      memcpy(attp->xvalue, xvalue, xsz);
    }
}
int vlistInqNatts(int vlistID, int varID, int *nattsp)
{
  int status = CDI_NOERR;
  vlist_t *vlistptr;
  cdi_atts_t *attsp;

  vlistptr = vlist_to_pointer(vlistID);

  attsp = get_attsp(vlistptr, varID);
  xassert(attsp != NULL);

  *nattsp = (int)attsp->nelems;

  return (status);
}
int vlistInqAtt(int vlistID, int varID, int attnum, char *name, int *typep, int *lenp)
{
  int status = CDI_NOERR;
  cdi_att_t *attp = NULL;

  xassert(name != NULL);

  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  cdi_atts_t *attsp = get_attsp(vlistptr, varID);
  xassert(attsp != NULL);

  if ( attnum >= 0 && attnum < (int)attsp->nelems )
    attp = &(attsp->value[attnum]);

  if ( attp != NULL )
    {
      memcpy(name, attp->name, attp->namesz+1);
      *typep = attp->exdtype;
      *lenp = (int)attp->nelems;
    }
  else
    {
      name[0] = 0;
      *typep = -1;
      *lenp = 0;
      status = -1;
    }

  return (status);
}


int vlistDelAtts(int vlistID, int varID)
{
  int status = CDI_NOERR;
  vlist_t *vlistptr;
  cdi_att_t *attp = NULL;
  cdi_atts_t *attsp;
  int attid;

  vlistptr = vlist_to_pointer(vlistID);

  attsp = get_attsp(vlistptr, varID);
  xassert(attsp != NULL);

  for ( attid = 0; attid < (int)attsp->nelems; attid++ )
    {
      attp = &(attsp->value[attid]);
      if ( attp->name ) Free(attp->name);
      if ( attp->xvalue ) Free(attp->xvalue);
    }

  attsp->nelems = 0;

  return (status);
}


int vlistDelAtt(int vlistID, int varID, const char *name)
{
  int status = CDI_NOERR;

  UNUSED(vlistID);
  UNUSED(varID);
  UNUSED(name);

  fprintf(stderr, "vlistDelAtt not implemented!\n");

  return (status);
}

static
int vlist_def_att(int indtype, int exdtype, int vlistID, int varID, const char *name, size_t len, size_t xsz, const void *xp)
{
  int status = CDI_NOERR;
  vlist_t *vlistptr;
  cdi_att_t *attp;
  cdi_atts_t *attsp;

  if ( len != 0 && xp == NULL )
    {
      return (CDI_EINVAL);
    }

  vlistptr = vlist_to_pointer(vlistID);

  attsp = get_attsp(vlistptr, varID);
  xassert(attsp != NULL);

  attp = find_att(attsp, name);
  if ( attp == NULL )
    attp = new_att(attsp, name);

  if ( attp != NULL )
    fill_att(attp, indtype, exdtype, len, xsz, xp);

  return (status);
}

static
int vlist_inq_att(int indtype, int vlistID, int varID, const char *name, size_t mxsz, void *xp)
{
  int status = CDI_NOERR;
  vlist_t *vlistptr;
  cdi_att_t *attp;
  cdi_atts_t *attsp;
  size_t xsz;

  if ( mxsz != 0 && xp == NULL )
    {
      return (CDI_EINVAL);
    }

  vlistptr = vlist_to_pointer(vlistID);

  attsp = get_attsp(vlistptr, varID);
  xassert(attsp != NULL);

  attp = find_att(attsp, name);
  if ( attp != NULL )
    {
      if ( attp->indtype == indtype )
        {
          xsz = attp->xsz;
          if ( mxsz < xsz ) xsz = mxsz;
          if ( xsz > 0 )
            memcpy(xp, attp->xvalue, xsz);
        }
      else
        {
          Warning("Attribute %s has wrong data type!", name);
          status = -2;
        }
    }
  else
    {

      status = -1;
    }

  return (status);
}


int vlistCopyVarAtts(int vlistID1, int varID_1, int vlistID2, int varID_2)
{
  int status = CDI_NOERR;
  vlist_t *vlistptr1;
  cdi_att_t *attp = NULL;
  cdi_atts_t *attsp1;
  int attid;

  vlistptr1 = vlist_to_pointer(vlistID1);

  attsp1 = get_attsp(vlistptr1, varID_1);
  xassert(attsp1 != NULL);

  for ( attid = 0; attid < (int)attsp1->nelems; attid++ )
    {
      attp = &(attsp1->value[attid]);
      vlist_def_att(attp->indtype, attp->exdtype, vlistID2, varID_2, attp->name, attp->nelems, attp->xsz, attp->xvalue);
    }

  return (status);
}
int vlistDefAttInt(int vlistID, int varID, const char *name, int type, int len, const int *ip)
{
  return vlist_def_att(DATATYPE_INT, type, vlistID, varID, name, (size_t)len, (size_t)len * sizeof (int), ip);
}
int vlistDefAttFlt(int vlistID, int varID, const char *name, int type, int len, const double *dp)
{
  return vlist_def_att(DATATYPE_FLT, type, vlistID, varID, name, (size_t)len, (size_t)len * sizeof (double), dp);
}
int vlistDefAttTxt(int vlistID, int varID, const char *name, int len, const char *tp)
{
  return vlist_def_att(DATATYPE_TXT, DATATYPE_TXT, vlistID, varID, name, (size_t)len, (size_t)len, tp);
}
int vlistInqAttInt(int vlistID, int varID, const char *name, int mlen, int *ip)
{
  return vlist_inq_att(DATATYPE_INT, vlistID, varID, name, (size_t)mlen * sizeof (int), ip);
}
int vlistInqAttFlt(int vlistID, int varID, const char *name, int mlen, double *dp)
{
  return vlist_inq_att(DATATYPE_FLT, vlistID, varID, name, (size_t)mlen * sizeof (double), dp);
}
int vlistInqAttTxt(int vlistID, int varID, const char *name, int mlen, char *tp)
{
  return vlist_inq_att(DATATYPE_TXT, vlistID, varID, name, (size_t)mlen * sizeof (char), tp);
}

enum {
  vlist_att_nints = 4,
};

static inline int
vlistAttTypeLookup(cdi_att_t *attp)
{
  int type;
  switch (attp->indtype)
  {
  case DATATYPE_FLT:
    type = DATATYPE_FLT64;
    break;
  case DATATYPE_INT:
  case DATATYPE_TXT:
    type = attp->indtype;
    break;
  default:
    xabort("Unknown datatype encountered in attribute %s: %d\n",
            attp->name, attp->indtype);
  }
  return type;
}


int vlist_att_compare(vlist_t *a, int varIDA, vlist_t *b, int varIDB,
                      int attnum)
{
  cdi_atts_t *attspa = get_attsp(a, varIDA),
    *attspb = get_attsp(b, varIDB);
  if (attspa == NULL && attspb == NULL)
    return 0;
  xassert(attnum >= 0 && attnum < (int)attspa->nelems
          && attnum < (int)attspb->nelems);
  cdi_att_t *attpa = attspa->value + attnum,
    *attpb = attspb->value + attnum;
  size_t len;
  if ((len = attpa->namesz) != attpb->namesz)
    return 1;
  int diff;
  if ((diff = memcmp(attpa->name, attpb->name, len)))
    return 1;
  if (attpa->indtype != attpb->indtype
      || attpa->exdtype != attpb->exdtype
      || attpa->nelems != attpb->nelems)
    return 1;
  return memcmp(attpa->xvalue, attpb->xvalue, attpa->xsz);
}


static int
vlistAttGetSize(vlist_t *vlistptr, int varID, int attnum, void *context)
{
  cdi_atts_t *attsp;
  cdi_att_t *attp;

  xassert(attsp = get_attsp(vlistptr, varID));
  xassert(attnum >= 0 && attnum < (int)attsp->nelems);
  attp = &(attsp->value[attnum]);
  int txsize = serializeGetSize(vlist_att_nints, DATATYPE_INT, context)
    + serializeGetSize((int)attp->namesz, DATATYPE_TXT, context);
  txsize += serializeGetSize((int)attp->nelems, vlistAttTypeLookup(attp), context);
  return txsize;
}

int
vlistAttsGetSize(vlist_t *p, int varID, void *context)
{
  cdi_atts_t *attsp = get_attsp(p, varID);
  int txsize = serializeGetSize(1, DATATYPE_INT, context);
  size_t numAtts = attsp->nelems;
  for (size_t i = 0; i < numAtts; ++i)
    txsize += vlistAttGetSize(p, varID, (int)i, context);
  return txsize;
}

static void
vlistAttPack(vlist_t *vlistptr, int varID, int attnum,
             void * buf, int size, int *position, void *context)
{
  cdi_atts_t *attsp;
  cdi_att_t *attp;
  int tempbuf[vlist_att_nints];

  xassert(attsp = get_attsp(vlistptr, varID));
  xassert(attnum >= 0 && attnum < (int)attsp->nelems);
  attp = &(attsp->value[attnum]);
  tempbuf[0] = (int)attp->namesz;
  tempbuf[1] = attp->exdtype;
  tempbuf[2] = attp->indtype;
  tempbuf[3] = (int)attp->nelems;
  serializePack(tempbuf, vlist_att_nints, DATATYPE_INT, buf, size, position, context);
  serializePack(attp->name, (int)attp->namesz, DATATYPE_TXT, buf, size, position, context);
  serializePack(attp->xvalue, (int)attp->nelems, vlistAttTypeLookup(attp),
                buf, size, position, context);
}

void
vlistAttsPack(vlist_t *p, int varID,
              void * buf, int size, int *position, void *context)
{
  cdi_atts_t *attsp = get_attsp(p, varID);
  size_t numAtts = attsp->nelems;
  int numAttsI = (int)numAtts;
  xassert(numAtts <= INT_MAX);
  serializePack(&numAttsI, 1, DATATYPE_INT, buf, size, position, context);
  for (size_t i = 0; i < numAtts; ++i)
    vlistAttPack(p, varID, (int)i, buf, size, position, context);
}

static void
vlistAttUnpack(int vlistID, int varID,
               void * buf, int size, int *position, void *context)
{
  int tempbuf[vlist_att_nints];

  serializeUnpack(buf, size, position,
                  tempbuf, vlist_att_nints, DATATYPE_INT, context);
  char *attName = (char *) Malloc((size_t)tempbuf[0] + 1);
  serializeUnpack(buf, size, position, attName, tempbuf[0], DATATYPE_TXT, context);
  attName[tempbuf[0]] = '\0';
  int attVDt;
  size_t elemSize;
  switch (tempbuf[2])
  {
  case DATATYPE_FLT:
    attVDt = DATATYPE_FLT64;
    elemSize = sizeof(double);
    break;
  case DATATYPE_INT:
    attVDt = DATATYPE_INT;
    elemSize = sizeof(int);
    break;
  case DATATYPE_TXT:
    attVDt = DATATYPE_TXT;
    elemSize = 1;
    break;
  default:
    xabort("Unknown datatype encountered in attribute %s: %d\n",
           attName, tempbuf[2]);
  }
  void *attData = (void *) Malloc(elemSize * (size_t)tempbuf[3]);
  serializeUnpack(buf, size, position, attData, tempbuf[3], attVDt, context);
  vlist_def_att(tempbuf[2], tempbuf[1], vlistID, varID, attName,
                (size_t)tempbuf[3], (size_t)tempbuf[3] * elemSize, attData);
  Free(attName);
  Free(attData);
}

void
vlistAttsUnpack(int vlistID, int varID,
                void * buf, int size, int *position, void *context)
{
  int numAtts, i;
  serializeUnpack(buf, size, position, &numAtts, 1, DATATYPE_INT, context);
  for (i = 0; i < numAtts; ++i)
  {
    vlistAttUnpack(vlistID, varID, buf, size, position, context);
  }
}
#if defined (HAVE_CONFIG_H)
#endif

#include <limits.h>


static
void vlistvarInitEntry(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistptr->vars[varID].fvarID = varID;
  vlistptr->vars[varID].mvarID = varID;
  vlistptr->vars[varID].flag = 0;
  vlistptr->vars[varID].param = 0;
  vlistptr->vars[varID].datatype = CDI_UNDEFID;
  vlistptr->vars[varID].tsteptype = TSTEP_INSTANT;
  vlistptr->vars[varID].timave = 0;
  vlistptr->vars[varID].timaccu = 0;
  vlistptr->vars[varID].typeOfGeneratingProcess = 0;
  vlistptr->vars[varID].productDefinitionTemplate = -1;
  vlistptr->vars[varID].chunktype = cdiChunkType;
  vlistptr->vars[varID].xyz = 321;
  vlistptr->vars[varID].gridID = CDI_UNDEFID;
  vlistptr->vars[varID].zaxisID = CDI_UNDEFID;
  vlistptr->vars[varID].subtypeID = CDI_UNDEFID;
  vlistptr->vars[varID].instID = CDI_UNDEFID;
  vlistptr->vars[varID].modelID = CDI_UNDEFID;
  vlistptr->vars[varID].tableID = CDI_UNDEFID;
  vlistptr->vars[varID].missvalused = FALSE;
  vlistptr->vars[varID].missval = cdiDefaultMissval;
  vlistptr->vars[varID].addoffset = 0.0;
  vlistptr->vars[varID].scalefactor = 1.0;
  vlistptr->vars[varID].name = NULL;
  vlistptr->vars[varID].longname = NULL;
  vlistptr->vars[varID].stdname = NULL;
  vlistptr->vars[varID].units = NULL;
  vlistptr->vars[varID].extra = NULL;
  vlistptr->vars[varID].levinfo = NULL;
  vlistptr->vars[varID].comptype = COMPRESS_NONE;
  vlistptr->vars[varID].complevel = 1;
  vlistptr->vars[varID].atts.nalloc = MAX_ATTRIBUTES;
  vlistptr->vars[varID].atts.nelems = 0;
  vlistptr->vars[varID].lvalidrange = 0;
  vlistptr->vars[varID].validrange[0] = VALIDMISS;
  vlistptr->vars[varID].validrange[1] = VALIDMISS;
  vlistptr->vars[varID].ensdata = NULL;
  vlistptr->vars[varID].iorank = CDI_UNDEFID;
  vlistptr->vars[varID].opt_grib_kvpair_size = 0;
  vlistptr->vars[varID].opt_grib_kvpair = NULL;
  vlistptr->vars[varID].opt_grib_nentries = 0;
}



static
int vlistvarNewEntry(int vlistID)
{
  int varID = 0;
  int vlistvarSize;
  var_t *vlistvar;
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistvarSize = vlistptr->varsAllocated;
  vlistvar = vlistptr->vars;




  if ( ! vlistvarSize )
    {
      vlistvarSize = 2;
      vlistvar = (var_t *) Malloc((size_t)vlistvarSize * sizeof (var_t));
      for (int i = 0; i < vlistvarSize; i++ )
        vlistvar[i].isUsed = FALSE;
    }
  else
    {
      while (varID < vlistvarSize && vlistvar[varID].isUsed)
        ++varID;
    }



  if ( varID == vlistvarSize )
    {
      vlistvar = (var_t *) Realloc(vlistvar,
                                   (size_t)(vlistvarSize *= 2) * sizeof(var_t));
      for ( int i = varID; i < vlistvarSize; i++ )
        vlistvar[i].isUsed = FALSE;
    }

  vlistptr->varsAllocated = vlistvarSize;
  vlistptr->vars = vlistvar;

  vlistvarInitEntry(vlistID, varID);

  vlistptr->vars[varID].isUsed = TRUE;

  return (varID);
}

void vlistCheckVarID(const char *caller, int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if ( vlistptr == NULL )
    Errorc("vlist undefined!");

  if ( varID < 0 || varID >= vlistptr->nvars )
    Errorc("varID %d undefined!", varID);

  if ( ! vlistptr->vars[varID].isUsed )
    Errorc("varID %d undefined!", varID);
}


int vlistDefVarTiles(int vlistID, int gridID, int zaxisID, int tsteptype, int tilesetID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  if ( CDI_Debug )
    Message("gridID = %d  zaxisID = %d  tsteptype = %d", gridID, zaxisID, tsteptype);

  int varID = vlistvarNewEntry(vlistID);

  vlistptr->nvars++;
  vlistptr->vars[varID].gridID = gridID;
  vlistptr->vars[varID].zaxisID = zaxisID;
  vlistptr->vars[varID].tsteptype = tsteptype;
  vlistptr->vars[varID].subtypeID = tilesetID;

  if ( tsteptype < 0 )
    {
      Message("Unexpected tstep type %d, set to TSTEP_INSTANT!", tsteptype);
      vlistptr->vars[varID].tsteptype = TSTEP_INSTANT;
    }

  vlistAdd2GridIDs(vlistptr, gridID);
  vlistAdd2ZaxisIDs(vlistptr, zaxisID);
  vlistAdd2SubtypeIDs(vlistptr, tilesetID);

  vlistptr->vars[varID].param = cdiEncodeParam(-(varID + 1), 255, 255);
  reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
  return (varID);
}
int vlistDefVar(int vlistID, int gridID, int zaxisID, int tsteptype)
{

  return vlistDefVarTiles(vlistID, gridID, zaxisID, tsteptype, CDI_UNDEFID);
}

void
cdiVlistCreateVarLevInfo(vlist_t *vlistptr, int varID)
{
  xassert(varID >= 0 && varID < vlistptr->nvars
          && vlistptr->vars[varID].levinfo == NULL);
  int zaxisID = vlistptr->vars[varID].zaxisID;
  size_t nlevs = (size_t)zaxisInqSize(zaxisID);

  vlistptr->vars[varID].levinfo
    = (levinfo_t*) Malloc((size_t)nlevs * sizeof(levinfo_t));

  for (size_t levID = 0; levID < nlevs; levID++ )
    vlistptr->vars[varID].levinfo[levID] = DEFAULT_LEVINFO((int)levID);
}
void vlistDefVarParam(int vlistID, int varID, int param)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if (vlistptr->vars[varID].param != param)
    {
      vlistptr->vars[varID].param = param;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}
void vlistDefVarCode(int vlistID, int varID, int code)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  int param = vlistptr->vars[varID].param;
  int pnum, pcat, pdis;
  cdiDecodeParam(param, &pnum, &pcat, &pdis);
  int newParam = cdiEncodeParam(code, pcat, pdis);
  if (vlistptr->vars[varID].param != newParam)
    {
      vlistptr->vars[varID].param = newParam;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


void vlistInqVar(int vlistID, int varID, int *gridID, int *zaxisID, int *tsteptype)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  *gridID = vlistptr->vars[varID].gridID;
  *zaxisID = vlistptr->vars[varID].zaxisID;
  *tsteptype = vlistptr->vars[varID].tsteptype;

  return;
}
int vlistInqVarGrid(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  return (vlistptr->vars[varID].gridID);
}
int vlistInqVarZaxis(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  return (vlistptr->vars[varID].zaxisID);
}
int vlistInqVarSubtype(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);
  return (vlistptr->vars[varID].subtypeID);
}
int vlistInqVarParam(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  return (vlistptr->vars[varID].param);
}
int vlistInqVarCode(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  int param = vlistptr->vars[varID].param;
  int pdis, pcat, pnum;
  cdiDecodeParam(param, &pnum, &pcat, &pdis);
  int code = pnum;
  if ( pdis != 255 ) code = -varID-1;

  if ( code < 0 && vlistptr->vars[varID].tableID != -1 && vlistptr->vars[varID].name != NULL )
    {
      tableInqParCode(vlistptr->vars[varID].tableID, vlistptr->vars[varID].name, &code);
    }

  return (code);
}


const char *vlistInqVarNamePtr(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  return (vlistptr->vars[varID].name);
}


const char *vlistInqVarLongnamePtr(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  return (vlistptr->vars[varID].longname);
}


const char *vlistInqVarStdnamePtr(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  return (vlistptr->vars[varID].stdname);
}


const char *vlistInqVarUnitsPtr(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  return (vlistptr->vars[varID].units);
}
void vlistInqVarName(int vlistID, int varID, char *name)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if ( vlistptr->vars[varID].name == NULL )
    {
      int param = vlistptr->vars[varID].param;
      int pdis, pcat, pnum;
      cdiDecodeParam(param, &pnum, &pcat, &pdis);
      if ( pdis == 255 )
        {
          int code = pnum;
          int tableID = vlistptr->vars[varID].tableID;
          if ( tableInqParName(tableID, code, name) != 0 )
            sprintf(name, "var%d", code);
        }
      else
        {
          sprintf(name, "param%d.%d.%d", pnum, pcat, pdis);
        }
    }
  else
    strcpy(name, vlistptr->vars[varID].name);

  return;
}
char* vlistCopyVarName(int vlistId, int varId)
{
  vlist_t* vlistptr = vlist_to_pointer(vlistId);
  vlistCheckVarID(__func__, vlistId, varId);


  const char* name = vlistptr->vars[varId].name;
  if(name) return strdup(name);


  int param = vlistptr->vars[varId].param;
  int discipline, category, number;
  cdiDecodeParam(param, &number, &category, &discipline);
  char *result = NULL;
  if(discipline == 255)
    {
      int tableId = vlistptr->vars[varId].tableID;
      if(( name = tableInqParNamePtr(tableId, number) ))
        result = strdup(name);
      {

        result = (char *) Malloc(3 + 3 * sizeof (int) * CHAR_BIT / 8 + 2);
        sprintf(result, "var%d", number);
      }
    }
  else
    {
      result = (char *) Malloc(5 + 2 + 3 * (3 * sizeof (int) * CHAR_BIT + 1) + 1);
      sprintf(result, "param%d.%d.%d", number, category, discipline);
    }

  return result;
}
void vlistInqVarLongname(int vlistID, int varID, char *longname)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  longname[0] = '\0';

  if ( vlistptr->vars[varID].longname == NULL )
    {
      int param = vlistptr->vars[varID].param;
      int pdis, pcat, pnum;
      cdiDecodeParam(param, &pnum, &pcat, &pdis);
      if ( pdis == 255 )
        {
          int code = pnum;
          int tableID = vlistptr->vars[varID].tableID;
          if ( tableInqParLongname(tableID, code, longname) != 0 )
            longname[0] = '\0';
        }
    }
  else
    strcpy(longname, vlistptr->vars[varID].longname);

  return;
}
void vlistInqVarStdname(int vlistID, int varID, char *stdname)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if ( vlistptr->vars[varID].stdname == NULL )
    {
      stdname[0] = '\0';
    }
  else
    strcpy(stdname, vlistptr->vars[varID].stdname);

  return;
}
void vlistInqVarUnits(int vlistID, int varID, char *units)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  units[0] = '\0';

  if ( vlistptr->vars[varID].units == NULL )
    {
      int param = vlistptr->vars[varID].param;
      int pdis, pcat, pnum;
      cdiDecodeParam(param, &pnum, &pcat, &pdis);
      if ( pdis == 255 )
        {
          int code = pnum;
          int tableID = vlistptr->vars[varID].tableID;
          if ( tableInqParUnits(tableID, code, units) != 0 )
            units[0] = '\0';
        }
    }
  else
    strcpy(units, vlistptr->vars[varID].units);

  return;
}


int vlistInqVarID(int vlistID, int code)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  for ( int varID = 0; varID < vlistptr->nvars; varID++ )
    {
      int param = vlistptr->vars[varID].param;
      int pdis, pcat, pnum;
      cdiDecodeParam(param, &pnum, &pcat, &pdis);
      if ( pnum == code ) return (varID);
    }

  return (CDI_UNDEFID);
}


int vlistInqVarSize(int vlistID, int varID)
{
  vlistCheckVarID(__func__, vlistID, varID);

  int zaxisID, gridID;
  int tsteptype;
  vlistInqVar(vlistID, varID, &gridID, &zaxisID, &tsteptype);

  int nlevs = zaxisInqSize(zaxisID);

  int gridsize = gridInqSize(gridID);

  int size = gridsize*nlevs;

  return (size);
}
int vlistInqVarDatatype(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  return (vlistptr->vars[varID].datatype);
}


int vlistInqVarNumber(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  int number = CDI_REAL;
  if ( vlistptr->vars[varID].datatype == DATATYPE_CPX32 ||
       vlistptr->vars[varID].datatype == DATATYPE_CPX64 )
    number = CDI_COMP;

  return (number);
}
void vlistDefVarDatatype(int vlistID, int varID, int datatype)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if (vlistptr->vars[varID].datatype != datatype)
    {
      vlistptr->vars[varID].datatype = datatype;

      if ( vlistptr->vars[varID].missvalused == FALSE )
        switch (datatype)
          {
          case DATATYPE_INT8: vlistptr->vars[varID].missval = -SCHAR_MAX; break;
          case DATATYPE_UINT8: vlistptr->vars[varID].missval = UCHAR_MAX; break;
          case DATATYPE_INT16: vlistptr->vars[varID].missval = -SHRT_MAX; break;
          case DATATYPE_UINT16: vlistptr->vars[varID].missval = USHRT_MAX; break;
          case DATATYPE_INT32: vlistptr->vars[varID].missval = -INT_MAX; break;
          case DATATYPE_UINT32: vlistptr->vars[varID].missval = UINT_MAX; break;
          }
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


void vlistDefVarInstitut(int vlistID, int varID, int instID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  if (vlistptr->vars[varID].instID != instID)
    {
      vlistptr->vars[varID].instID = instID;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistInqVarInstitut(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  return (vlistptr->vars[varID].instID);
}


void vlistDefVarModel(int vlistID, int varID, int modelID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  if (vlistptr->vars[varID].modelID != modelID)
    {
      vlistptr->vars[varID].modelID = modelID;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistInqVarModel(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  return (vlistptr->vars[varID].modelID);
}


void vlistDefVarTable(int vlistID, int varID, int tableID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if (vlistptr->vars[varID].tableID != tableID)
    {
      vlistptr->vars[varID].tableID = tableID;
      int tablenum = tableInqNum(tableID);

      int param = vlistptr->vars[varID].param;

      int pnum, pcat, pdis;
      cdiDecodeParam(param, &pnum, &pcat, &pdis);
      vlistptr->vars[varID].param = cdiEncodeParam(pnum, tablenum, pdis);
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistInqVarTable(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  return (vlistptr->vars[varID].tableID);
}
void vlistDefVarName(int vlistID, int varID, const char *name)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if ( name )
    {
      if ( vlistptr->vars[varID].name )
        {
          Free(vlistptr->vars[varID].name);
          vlistptr->vars[varID].name = NULL;
        }

      vlistptr->vars[varID].name = strdupx(name);
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}
void vlistDefVarLongname(int vlistID, int varID, const char *longname)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if ( longname )
    {
      if ( vlistptr->vars[varID].longname )
        {
          Free(vlistptr->vars[varID].longname);
          vlistptr->vars[varID].longname = 0;
        }

      vlistptr->vars[varID].longname = strdupx(longname);
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}
void vlistDefVarStdname(int vlistID, int varID, const char *stdname)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if ( stdname )
    {
      if ( vlistptr->vars[varID].stdname )
        {
          Free(vlistptr->vars[varID].stdname);
          vlistptr->vars[varID].stdname = 0;
        }

      vlistptr->vars[varID].stdname = strdupx(stdname);
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}
void vlistDefVarUnits(int vlistID, int varID, const char *units)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if ( units )
    {
      if ( vlistptr->vars[varID].units )
        {
          Free(vlistptr->vars[varID].units);
          vlistptr->vars[varID].units = 0;
        }

      vlistptr->vars[varID].units = strdupx(units);
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}
double vlistInqVarMissval(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  return (vlistptr->vars[varID].missval);
}
void vlistDefVarMissval(int vlistID, int varID, double missval)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  vlistptr->vars[varID].missval = missval;
  vlistptr->vars[varID].missvalused = TRUE;
}
void vlistDefVarExtra(int vlistID, int varID, const char *extra)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if ( extra )
    {
      if ( vlistptr->vars[varID].extra )
        {
          Free(vlistptr->vars[varID].extra);
          vlistptr->vars[varID].extra = NULL;
        }

      vlistptr->vars[varID].extra = strdupx(extra);
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}
void vlistInqVarExtra(int vlistID, int varID, char *extra)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if ( vlistptr->vars[varID].extra == NULL )
      sprintf(extra, "-");
  else
    strcpy(extra, vlistptr->vars[varID].extra);

  return;
}


int vlistInqVarValidrange(int vlistID, int varID, double *validrange)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if ( validrange != NULL && vlistptr->vars[varID].lvalidrange )
    {
      validrange[0] = vlistptr->vars[varID].validrange[0];
      validrange[1] = vlistptr->vars[varID].validrange[1];
    }

  return (vlistptr->vars[varID].lvalidrange);
}


void vlistDefVarValidrange(int vlistID, int varID, const double *validrange)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  vlistptr->vars[varID].validrange[0] = validrange[0];
  vlistptr->vars[varID].validrange[1] = validrange[1];
  vlistptr->vars[varID].lvalidrange = TRUE;
  reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
}


double vlistInqVarScalefactor(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  return (vlistptr->vars[varID].scalefactor);
}


double vlistInqVarAddoffset(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  return (vlistptr->vars[varID].addoffset);
}


void vlistDefVarScalefactor(int vlistID, int varID, double scalefactor)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if ( IS_NOT_EQUAL(vlistptr->vars[varID].scalefactor, scalefactor) )
    {
      vlistptr->vars[varID].scalefactor = scalefactor;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


void vlistDefVarAddoffset(int vlistID, int varID, double addoffset)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if ( IS_NOT_EQUAL(vlistptr->vars[varID].addoffset, addoffset))
    {
      vlistptr->vars[varID].addoffset = addoffset;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


void vlistDefVarTsteptype(int vlistID, int varID, int tsteptype)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  if (vlistptr->vars[varID].tsteptype != tsteptype)
    {
      vlistptr->vars[varID].tsteptype = tsteptype;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistInqVarTsteptype(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  return (vlistptr->vars[varID].tsteptype);
}


void vlistDefVarTimave(int vlistID, int varID, int timave)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  if (vlistptr->vars[varID].timave != timave)
    {
      vlistptr->vars[varID].timave = timave;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistInqVarTimave(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  return (vlistptr->vars[varID].timave);
}


void vlistDefVarTimaccu(int vlistID, int varID, int timaccu)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  if (vlistptr->vars[varID].timaccu != timaccu)
    {
      vlistptr->vars[varID].timaccu = timaccu;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistInqVarTimaccu(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  return (vlistptr->vars[varID].timaccu);
}


void vlistDefVarTypeOfGeneratingProcess(int vlistID, int varID, int typeOfGeneratingProcess)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  if (vlistptr->vars[varID].typeOfGeneratingProcess != typeOfGeneratingProcess)
    {
      vlistptr->vars[varID].typeOfGeneratingProcess = typeOfGeneratingProcess;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistInqVarTypeOfGeneratingProcess(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  return (vlistptr->vars[varID].typeOfGeneratingProcess);
}


void vlistDefVarProductDefinitionTemplate(int vlistID, int varID, int productDefinitionTemplate)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if (vlistptr->vars[varID].productDefinitionTemplate != productDefinitionTemplate)
    {
      vlistptr->vars[varID].productDefinitionTemplate = productDefinitionTemplate;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistInqVarProductDefinitionTemplate(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  return (vlistptr->vars[varID].productDefinitionTemplate);
}


void vlistDestroyVarName(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  if ( vlistptr->vars[varID].name )
    {
      Free(vlistptr->vars[varID].name);
      vlistptr->vars[varID].name = NULL;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


void vlistDestroyVarLongname(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if ( vlistptr->vars[varID].longname )
    {
      Free(vlistptr->vars[varID].longname);
      vlistptr->vars[varID].longname = NULL;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


void vlistDestroyVarStdname(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if ( vlistptr->vars[varID].stdname )
    {
      Free(vlistptr->vars[varID].stdname);
      vlistptr->vars[varID].stdname = NULL;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


void vlistDestroyVarUnits(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if ( vlistptr->vars[varID].units )
    {
      Free(vlistptr->vars[varID].units);
      vlistptr->vars[varID].units = NULL;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistInqVarMissvalUsed(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  return (vlistptr->vars[varID].missvalused);
}


void vlistDefFlag(int vlistID, int varID, int levID, int flag)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  levinfo_t li = DEFAULT_LEVINFO(levID);
  if (vlistptr->vars[varID].levinfo)
    ;
  else if (flag != li.flag)
    cdiVlistCreateVarLevInfo(vlistptr, varID);
  else
    return;

  vlistptr->vars[varID].levinfo[levID].flag = flag;

  vlistptr->vars[varID].flag = 0;

  int nlevs = zaxisInqSize(vlistptr->vars[varID].zaxisID);
  for ( int levelID = 0; levelID < nlevs; levelID++ )
    {
      if ( vlistptr->vars[varID].levinfo[levelID].flag )
        {
          vlistptr->vars[varID].flag = 1;
          break;
        }
    }

  reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
}


int vlistInqFlag(int vlistID, int varID, int levID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if (vlistptr->vars[varID].levinfo)
    return (vlistptr->vars[varID].levinfo[levID].flag);
  else
    {
      levinfo_t li = DEFAULT_LEVINFO(levID);
      return li.flag;
    }
}


int vlistFindVar(int vlistID, int fvarID)
{
  int varID;
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  for ( varID = 0; varID < vlistptr->nvars; varID++ )
    {
      if ( vlistptr->vars[varID].fvarID == fvarID ) break;
    }

  if ( varID == vlistptr->nvars )
    {
      varID = -1;
      Message("varID not found for fvarID %d in vlistID %d!", fvarID, vlistID);
    }

  return (varID);
}


int vlistFindLevel(int vlistID, int fvarID, int flevelID)
{
  int levelID = -1;
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  int varID = vlistFindVar(vlistID, fvarID);

  if ( varID != -1 )
    {
      int nlevs = zaxisInqSize(vlistptr->vars[varID].zaxisID);
      for ( levelID = 0; levelID < nlevs; levelID++ )
        {
          if ( vlistptr->vars[varID].levinfo[levelID].flevelID == flevelID ) break;
        }

      if ( levelID == nlevs )
        {
          levelID = -1;
          Message("levelID not found for fvarID %d and levelID %d in vlistID %d!",
                  fvarID, flevelID, vlistID);
        }
    }

  return (levelID);
}


int vlistMergedVar(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  return (vlistptr->vars[varID].mvarID);
}


int vlistMergedLevel(int vlistID, int varID, int levelID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if (vlistptr->vars[varID].levinfo)
    return vlistptr->vars[varID].levinfo[levelID].mlevelID;
  else
    {
      levinfo_t li = DEFAULT_LEVINFO(levelID);
      return li.mlevelID;
    }
}


void vlistDefIndex(int vlistID, int varID, int levelID, int index)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  levinfo_t li = DEFAULT_LEVINFO(levelID);
  if (vlistptr->vars[varID].levinfo)
    ;
  else if (index != li.index)
    cdiVlistCreateVarLevInfo(vlistptr, varID);
  else
    return;
  vlistptr->vars[varID].levinfo[levelID].index = index;
  reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
}


int vlistInqIndex(int vlistID, int varID, int levelID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  if (vlistptr->vars[varID].levinfo)
    return (vlistptr->vars[varID].levinfo[levelID].index);
  else
    {
      levinfo_t li = DEFAULT_LEVINFO(levelID);
      return li.index;
    }
}


void vlistChangeVarZaxis(int vlistID, int varID, int zaxisID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  int nlevs1 = zaxisInqSize(vlistptr->vars[varID].zaxisID);
  int nlevs2 = zaxisInqSize(zaxisID);

  if ( nlevs1 != nlevs2 ) Error("Number of levels must not change!");

  int local_nvars = vlistptr->nvars;
  int found = 0;
  int oldZaxisID = vlistptr->vars[varID].zaxisID;
  for ( int i = 0; i < varID; ++i)
    found |= (vlistptr->vars[i].zaxisID == oldZaxisID);
  for ( int i = varID + 1; i < local_nvars; ++i)
    found |= (vlistptr->vars[i].zaxisID == oldZaxisID);

  if (found)
    {
      int nzaxis = vlistptr->nzaxis;
      for (int i = 0; i < nzaxis; ++i)
        if (vlistptr->zaxisIDs[i] == oldZaxisID )
          vlistptr->zaxisIDs[i] = zaxisID;
    }
  else
    vlistAdd2ZaxisIDs(vlistptr, zaxisID);

  vlistptr->vars[varID].zaxisID = zaxisID;
  reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
}


void vlistChangeVarGrid(int vlistID, int varID, int gridID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  int local_nvars = vlistptr->nvars;
  int index;
  for ( index = 0; index < local_nvars; index++ )
    if ( index != varID )
      if ( vlistptr->vars[index].gridID == vlistptr->vars[varID].gridID ) break;

  if ( index == local_nvars )
    {
      for ( index = 0; index < vlistptr->ngrids; index++ )
        if ( vlistptr->gridIDs[index] == vlistptr->vars[varID].gridID )
          vlistptr->gridIDs[index] = gridID;
    }
  else
    vlistAdd2GridIDs(vlistptr, gridID);

  vlistptr->vars[varID].gridID = gridID;
  reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
}


void vlistDefVarCompType(int vlistID, int varID, int comptype)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if (vlistptr->vars[varID].comptype != comptype)
    {
      vlistptr->vars[varID].comptype = comptype;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistInqVarCompType(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  return (vlistptr->vars[varID].comptype);
}


void vlistDefVarCompLevel(int vlistID, int varID, int complevel)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if (vlistptr->vars[varID].complevel != complevel)
    {
      vlistptr->vars[varID].complevel = complevel;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistInqVarCompLevel(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  return (vlistptr->vars[varID].complevel);
}


void vlistDefVarChunkType(int vlistID, int varID, int chunktype)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if (vlistptr->vars[varID].chunktype != chunktype)
    {
      vlistptr->vars[varID].chunktype = chunktype;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistInqVarChunkType(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  return (vlistptr->vars[varID].chunktype);
}

static
int vlistEncodeXyz(int (*dimorder)[3])
{
  return (*dimorder)[0]*100 + (*dimorder)[1]*10 + (*dimorder)[2];
}


static
void vlistDecodeXyz(int xyz, int (*outDimorder)[3])
{
  (*outDimorder)[0] = xyz/100, xyz -= (*outDimorder)[0]*100;
  (*outDimorder)[1] = xyz/10, xyz -= (*outDimorder)[1]*10;
  (*outDimorder)[2] = xyz;
}


void vlistDefVarXYZ(int vlistID, int varID, int xyz)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if ( xyz == 3 ) xyz = 321;


  {
    int dimorder[3];
    vlistDecodeXyz(xyz, &dimorder);
    int dimx = 0, dimy = 0, dimz = 0;
    for ( int id = 0; id < 3; ++id )
      {
        switch ( dimorder[id] )
          {
            case 1: dimx++; break;
            case 2: dimy++; break;
            case 3: dimz++; break;
            default: dimorder[id] = 0; break;
          }
     }
    if ( dimz > 1 || dimy > 1 || dimx > 1 ) xyz = 321;
    else
      {
        if ( dimz == 0 ) for ( int id = 0; id < 3; ++id ) if ( dimorder[id] == 0 ) {dimorder[id] = 3; break;}
        if ( dimy == 0 ) for ( int id = 0; id < 3; ++id ) if ( dimorder[id] == 0 ) {dimorder[id] = 2; break;}
        if ( dimx == 0 ) for ( int id = 0; id < 3; ++id ) if ( dimorder[id] == 0 ) {dimorder[id] = 1; break;}
        xyz = vlistEncodeXyz(&dimorder);
      }
  }

  assert(xyz == 123 || xyz == 312 || xyz == 231 || xyz == 321 || xyz == 132 || xyz == 213);

  vlistptr->vars[varID].xyz = xyz;
  reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
}


void vlistInqVarDimorder(int vlistID, int varID, int (*outDimorder)[3])
{
  vlist_t *vlistptr;

  vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  vlistDecodeXyz(vlistptr->vars[varID].xyz, outDimorder);
}


int vlistInqVarXYZ(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  return (vlistptr->vars[varID].xyz);
}


void vlistDefVarEnsemble(int vlistID, int varID, int ensID, int ensCount, int forecast_type )
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if ( vlistptr->vars[varID].ensdata == NULL )
    vlistptr->vars[varID].ensdata
      = (ensinfo_t *) Malloc( sizeof( ensinfo_t ) );

  vlistptr->vars[varID].ensdata->ens_index = ensID;
  vlistptr->vars[varID].ensdata->ens_count = ensCount;
  vlistptr->vars[varID].ensdata->forecast_init_type = forecast_type;
  reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
}


int vlistInqVarEnsemble( int vlistID, int varID, int *ensID, int *ensCount, int *forecast_type )
{
  int status = 0;
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  if ( vlistptr->vars[varID].ensdata )
    {
      *ensID = vlistptr->vars[varID].ensdata->ens_index;
      *ensCount = vlistptr->vars[varID].ensdata->ens_count;
      *forecast_type = vlistptr->vars[varID].ensdata->forecast_init_type;

      status = 1;
    }

  return (status);
}




void vlistDefVarIntKey(int vlistID, int varID, const char *name, int value)
{
  (void)vlistID;
  (void)varID;
  (void)name;
  (void)value;
}


void vlistDefVarDblKey(int vlistID, int varID, const char *name, double value)
{
  (void)vlistID;
  (void)varID;
  (void)name;
  (void)value;
}



void cdiClearAdditionalKeys()
{

}


void cdiDefAdditionalKey(const char *name)
{
  (void)name;
}


int vlistHasVarKey(int vlistID, int varID, const char* name)
{
  (void)vlistID;
  (void)varID;
  (void)name;
  return 0;
}


double vlistInqVarDblKey(int vlistID, int varID, const char* name)
{
  double value = 0;
  (void)vlistID;
  (void)varID;
  (void)name;
  return value;
}



int vlistInqVarIntKey(int vlistID, int varID, const char* name)
{
  long int value = 0;
  (void)vlistID;
  (void)varID;
  (void)name;
  return (int) value;
}


void vlistDefVarIOrank(int vlistID, int varID, int iorank)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID );

  vlistCheckVarID ( __func__, vlistID, varID );

  if (vlistptr->vars[varID].iorank != iorank)
    {
      vlistptr->vars[varID].iorank = iorank;
      reshSetStatus(vlistID, &vlistOps, RESH_DESYNC_IN_USE);
    }
}


int vlistInqVarIOrank(int vlistID, int varID)
{
  vlist_t *vlistptr = vlist_to_pointer(vlistID);

  vlistCheckVarID(__func__, vlistID, varID);

  return vlistptr->vars[varID].iorank;
}


int vlistVarCompare(vlist_t *a, int varIDA, vlist_t *b, int varIDB)
{
  xassert(a && b
          && varIDA >= 0 && varIDA < a->nvars
          && varIDB >= 0 && varIDB < b->nvars);
  var_t *pva = a->vars + varIDA, *pvb = b->vars + varIDB;
#define FCMP(f) ((pva->f) != (pvb->f))
#define FCMPFLT(f) (IS_NOT_EQUAL((pva->f), (pvb->f)))
#define FCMPSTR(fs) ((pva->fs) != (pvb->fs) && strcmp((pva->fs), (pvb->fs)))
#define FCMP2(f) (namespaceResHDecode(pva->f).idx \
                  != namespaceResHDecode(pvb->f).idx)
  int diff = FCMP(fvarID) | FCMP(mvarID) | FCMP(flag) | FCMP(param)
    | FCMP(datatype) | FCMP(tsteptype) | FCMP(timave) | FCMP(timaccu)
    | FCMP(chunktype) | FCMP(xyz) | FCMP2(gridID) | FCMP2(zaxisID)
    | FCMP2(instID) | FCMP2(modelID) | FCMP2(tableID) | FCMP(missvalused)
    | FCMPFLT(missval) | FCMPFLT(addoffset) | FCMPFLT(scalefactor) | FCMPSTR(name)
    | FCMPSTR(longname) | FCMPSTR(stdname) | FCMPSTR(units) | FCMPSTR(extra)
    | FCMP(comptype) | FCMP(complevel) | FCMP(lvalidrange)
    | FCMPFLT(validrange[0]) | FCMPFLT(validrange[1]);
#undef FCMP
#undef FCMPFLT
#undef FCMP2
  if ((diff |= ((pva->levinfo == NULL) ^ (pvb->levinfo == NULL))))
    return 1;
  if (pva->levinfo)
    {
      int zaxisID = pva->zaxisID;
      size_t nlevs = (size_t)zaxisInqSize(zaxisID);
      diff |= (memcmp(pva->levinfo, pvb->levinfo, sizeof (levinfo_t) * nlevs)
               != 0);
      if (diff)
        return 1;
    }
  size_t natts = a->vars[varIDA].atts.nelems;
  if (natts != b->vars[varIDB].atts.nelems)
    return 1;
  for (size_t attID = 0; attID < natts; ++attID)
    diff |= vlist_att_compare(a, varIDA, b, varIDB, (int)attID);
  if ((diff |= ((pva->ensdata == NULL) ^ (pvb->ensdata == NULL))))
    return 1;
  if (pva->ensdata)
    diff = (memcmp(pva->ensdata, pvb->ensdata, sizeof (*(pva->ensdata)))) != 0;
  return diff;
}



enum {
  vlistvar_nints = 21,
  vlistvar_ndbls = 3,
};

int vlistVarGetPackSize(vlist_t *p, int varID, void *context)
{
  var_t *var = p->vars + varID;
  int varsize = serializeGetSize(vlistvar_nints, DATATYPE_INT, context)
    + serializeGetSize(vlistvar_ndbls, DATATYPE_FLT64, context);
  if (var->name)
    varsize += serializeGetSize((int)strlen(var->name), DATATYPE_TXT, context);
  if (var->longname)
    varsize += serializeGetSize((int)strlen(var->longname), DATATYPE_TXT, context);
  if (var->stdname)
    varsize += serializeGetSize((int)strlen(var->stdname), DATATYPE_TXT, context);
  if (var->units)
    varsize += serializeGetSize((int)strlen(var->units), DATATYPE_TXT, context);
  if (var->extra)
    varsize += serializeGetSize((int)strlen(var->extra), DATATYPE_TXT, context);
  varsize += serializeGetSize(4 * zaxisInqSize(var->zaxisID),
                              DATATYPE_INT, context);
  varsize += vlistAttsGetSize(p, varID, context);
  return varsize;
}

void vlistVarPack(vlist_t *p, int varID, char * buf, int size, int *position,
                  void *context)
{
  double dtempbuf[vlistvar_ndbls];
  var_t *var = p->vars + varID;
  int tempbuf[vlistvar_nints], namesz, longnamesz, stdnamesz, unitssz,
    extralen;

  tempbuf[0] = var->flag;
  tempbuf[1] = var->gridID;
  tempbuf[2] = var->zaxisID;
  tempbuf[3] = var->tsteptype;
  tempbuf[4] = namesz = var->name?(int)strlen(var->name):0;
  tempbuf[5] = longnamesz = var->longname?(int)strlen(var->longname):0;
  tempbuf[6] = stdnamesz = var->stdname?(int)strlen(var->stdname):0;
  tempbuf[7] = unitssz = var->units?(int)strlen(var->units):0;
  tempbuf[8] = var->datatype;
  tempbuf[9] = var->param;
  tempbuf[10] = var->instID;
  tempbuf[11] = var->modelID;
  tempbuf[12] = var->tableID;
  tempbuf[13] = var->timave;
  tempbuf[14] = var->timaccu;
  tempbuf[15] = var->missvalused;
  tempbuf[16] = var->comptype;
  tempbuf[17] = var->complevel;
  int nlevs = var->levinfo ? zaxisInqSize(var->zaxisID) : 0;
  tempbuf[18] = nlevs;
  tempbuf[19] = var->iorank;
  tempbuf[20] = extralen = var->extra?(int)strlen(var->extra):0;
  dtempbuf[0] = var->missval;
  dtempbuf[1] = var->scalefactor;
  dtempbuf[2] = var->addoffset;
  serializePack(tempbuf, vlistvar_nints, DATATYPE_INT,
                buf, size, position, context);
  serializePack(dtempbuf, vlistvar_ndbls, DATATYPE_FLT64,
                buf, size, position, context);
  if (namesz)
    serializePack(var->name, namesz, DATATYPE_TXT, buf, size, position, context);
  if (longnamesz)
    serializePack(var->longname, longnamesz, DATATYPE_TXT,
                  buf, size, position, context);
  if (stdnamesz)
    serializePack(var->stdname, stdnamesz, DATATYPE_TXT,
                  buf, size, position, context);
  if (unitssz)
    serializePack(var->units, unitssz, DATATYPE_TXT,
                  buf, size, position, context);
  if (extralen)
    serializePack(var->extra, extralen, DATATYPE_TXT,
                  buf, size, position, context);
  if (nlevs)
    {
      int levbuf[nlevs][4];
      for (int levID = 0; levID < nlevs; ++levID)
        {
          levbuf[levID][0] = var->levinfo[levID].flag;
          levbuf[levID][1] = var->levinfo[levID].index;
          levbuf[levID][2] = var->levinfo[levID].mlevelID;
          levbuf[levID][3] = var->levinfo[levID].flevelID;
        }
      serializePack(levbuf, nlevs * 4, DATATYPE_INT,
                    buf, size, position, context);
    }
  vlistAttsPack(p, varID, buf, size, position, context);
}

static inline int
imax(int a, int b)
{
  return a>=b?a:b;
}


void vlistVarUnpack(int vlistID, char * buf, int size, int *position,
                    int originNamespace, void *context)
{
  double dtempbuf[vlistvar_ndbls];
  int tempbuf[vlistvar_nints];
  char *varname = NULL;
  vlist_t *vlistptr = vlist_to_pointer(vlistID);
  serializeUnpack(buf, size, position,
                  tempbuf, vlistvar_nints, DATATYPE_INT, context);
  serializeUnpack(buf, size, position,
                  dtempbuf, vlistvar_ndbls, DATATYPE_FLT64, context);





  int newvar = vlistDefVar ( vlistID,
                         namespaceAdaptKey ( tempbuf[1], originNamespace ),
                         namespaceAdaptKey ( tempbuf[2], originNamespace ),
                         tempbuf[3]);
  if (tempbuf[4] || tempbuf[5] || tempbuf[6] || tempbuf[7] || tempbuf[20])
    varname = (char *)Malloc((size_t)imax(imax(imax(imax(tempbuf[4],
                                                         tempbuf[5]),
                                                    tempbuf[6]),
                                               tempbuf[7]), tempbuf[20]) + 1);
  if (tempbuf[4])
  {
    serializeUnpack(buf, size, position,
                    varname, tempbuf[4], DATATYPE_TXT, context);
    varname[tempbuf[4]] = '\0';
    vlistDefVarName(vlistID, newvar, varname);
  }
  if (tempbuf[5])
  {
    serializeUnpack(buf, size, position,
                    varname, tempbuf[5], DATATYPE_TXT, context);
    varname[tempbuf[5]] = '\0';
    vlistDefVarLongname(vlistID, newvar, varname);
  }
  if (tempbuf[6])
  {
    serializeUnpack(buf, size, position,
                    varname, tempbuf[6], DATATYPE_TXT, context);
    varname[tempbuf[6]] = '\0';
    vlistDefVarStdname(vlistID, newvar, varname);
  }
  if (tempbuf[7])
  {
    serializeUnpack(buf, size, position,
                    varname, tempbuf[7], DATATYPE_TXT, context);
    varname[tempbuf[7]] = '\0';
    vlistDefVarUnits(vlistID, newvar, varname);
  }
  if (tempbuf[20])
    {
      serializeUnpack(buf, size, position,
                      varname, tempbuf[20], DATATYPE_TXT, context);
      varname[tempbuf[20]] = '\0';
      vlistDefVarExtra(vlistID, newvar, varname);
    }
  Free(varname);
  vlistDefVarDatatype(vlistID, newvar, tempbuf[8]);
  vlistDefVarInstitut ( vlistID, newvar,
                        namespaceAdaptKey ( tempbuf[10], originNamespace ));
  vlistDefVarModel ( vlistID, newvar,
                     namespaceAdaptKey ( tempbuf[11], originNamespace ));
  vlistDefVarTable(vlistID, newvar, tempbuf[12]);

  vlistDefVarParam(vlistID, newvar, tempbuf[9]);
  vlistDefVarTimave(vlistID, newvar, tempbuf[13]);
  vlistDefVarTimaccu(vlistID, newvar, tempbuf[14]);
  if (tempbuf[15])
    vlistDefVarMissval(vlistID, newvar, dtempbuf[0]);
  vlistDefVarScalefactor(vlistID, newvar, dtempbuf[1]);
  vlistDefVarAddoffset(vlistID, newvar, dtempbuf[2]);
  vlistDefVarCompType(vlistID, newvar, tempbuf[16]);
  vlistDefVarCompLevel(vlistID, newvar, tempbuf[17]);
  int nlevs = tempbuf[18];
  if (nlevs)
    {
      int levbuf[nlevs][4];
      var_t *var = vlistptr->vars + newvar;
      int i, flagSetLev = 0;
      cdiVlistCreateVarLevInfo(vlistptr, newvar);
      serializeUnpack(buf, size, position,
                      levbuf, nlevs * 4, DATATYPE_INT, context);
      for (i = 0; i < nlevs; ++i)
        {
          vlistDefFlag(vlistID, newvar, i, levbuf[i][0]);
          vlistDefIndex(vlistID, newvar, i, levbuf[i][1]);

          var->levinfo[i].mlevelID = levbuf[i][2];
          var->levinfo[i].flevelID = levbuf[i][3];
          if (levbuf[i][0] == tempbuf[0])
            flagSetLev = i;
        }
      vlistDefFlag(vlistID, newvar, flagSetLev, levbuf[flagSetLev][0]);
    }
  vlistDefVarIOrank(vlistID, newvar, tempbuf[19]);
  vlistAttsUnpack(vlistID, newvar, buf, size, position, context);
}
#if defined (HAVE_CONFIG_H)
#endif

#include <string.h>
#include <math.h>
#include <float.h>



#define LevelUp 1
#define LevelDown 2


static const struct {
  unsigned char positive;
  const char *name;
  const char *longname;
  const char *stdname;
  const char *units;
}
ZaxistypeEntry[] = {
  { 0, "sfc", "surface", "", ""},
  { 0, "lev", "generic", "", "level"},
  { 2, "lev", "hybrid", "", "level"},
  { 2, "lev", "hybrid_half", "", "level"},
  { 2, "lev", "pressure", "air_pressure", "Pa"},
  { 1, "height", "height", "height", "m"},
  { 2, "depth", "depth_below_sea", "depth", "m"},
  { 2, "depth", "depth_below_land", "", "cm"},
  { 0, "lev", "isentropic", "", "K"},
  { 0, "lev", "trajectory", "", ""},
  { 1, "alt", "altitude", "", "m"},
  { 0, "lev", "sigma", "", "level"},
  { 0, "lev", "meansea", "", "level"},
  { 0, "toa", "top_of_atmosphere", "", ""},
  { 0, "seabottom", "sea_bottom", "", ""},
  { 0, "atmosphere", "atmosphere", "", ""},
  { 0, "cloudbase", "cloud_base", "", ""},
  { 0, "cloudtop", "cloud_top", "", ""},
  { 0, "isotherm0", "isotherm_zero", "", ""},
  { 0, "snow", "snow", "", ""},
  { 0, "lakebottom", "lake_bottom", "", ""},
  { 0, "sedimentbottom", "sediment_bottom", "", ""},
  { 0, "sedimentbottomta", "sediment_bottom_ta", "", ""},
  { 0, "sedimentbottomtw", "sediment_bottom_tw", "", ""},
  { 0, "mixlayer", "mix_layer", "", ""},
  { 0, "height", "generalized_height", "height", ""},
};

enum {
  CDI_NumZaxistype = sizeof(ZaxistypeEntry) / sizeof(ZaxistypeEntry[0]),
};


typedef struct {
  unsigned char positive;
  char name[CDI_MAX_NAME];
  char longname[CDI_MAX_NAME];
  char stdname[CDI_MAX_NAME];
  char units[CDI_MAX_NAME];
  char psname[CDI_MAX_NAME];
  double *vals;
  double *lbounds;
  double *ubounds;
  double *weights;
  int self;
  int prec;
  int scalar;
  int type;
  int ltype;
  int ltype2;
  int size;
  int direction;
  int vctsize;
  double *vct;
  int number;
  int nhlev;
  unsigned char uuid[CDI_UUID_SIZE];
}
zaxis_t;

static int zaxisCompareP (zaxis_t *z1, zaxis_t *z2);
static void zaxisDestroyP ( void * zaxisptr );
static void zaxisPrintP ( void * zaxisptr, FILE * fp );
static int zaxisGetPackSize ( void * zaxisptr, void *context);
static void zaxisPack ( void * zaxisptr, void * buffer, int size, int *pos, void *context);
static int zaxisTxCode ( void );

static const resOps zaxisOps = {
  (int (*)(void *, void *))zaxisCompareP,
  zaxisDestroyP,
  zaxisPrintP,
  zaxisGetPackSize,
  zaxisPack,
  zaxisTxCode
};

const resOps *getZaxisOps(void)
{
  return &zaxisOps;
}

static int ZAXIS_Debug = 0;

void zaxisGetTypeDescription(int zaxisType, int* outPositive, const char** outName, const char** outLongName, const char** outStdName, const char** outUnit)
{
  if(zaxisType < 0 || zaxisType >= CDI_NumZaxistype)
    {
      if(outPositive) *outPositive = 0;
      if(outName) *outName = NULL;
      if(outLongName) *outLongName = NULL;
      if(outStdName) *outStdName = NULL;
      if(outUnit) *outUnit = NULL;
    }
  else
    {
      if(outPositive) *outPositive = ZaxistypeEntry[zaxisType].positive;
      if(outName) *outName = ZaxistypeEntry[zaxisType].name;
      if(outLongName) *outLongName = ZaxistypeEntry[zaxisType].longname;
      if(outStdName) *outStdName = ZaxistypeEntry[zaxisType].stdname;
      if(outUnit) *outUnit = ZaxistypeEntry[zaxisType].units;
    }
}

static
void zaxisDefaultValue(zaxis_t *zaxisptr)
{
  zaxisptr->self = CDI_UNDEFID;
  zaxisptr->name[0] = 0;
  zaxisptr->longname[0] = 0;
  zaxisptr->stdname[0] = 0;
  zaxisptr->units[0] = 0;
  zaxisptr->psname[0] = 0;
  zaxisptr->vals = NULL;
  zaxisptr->ubounds = NULL;
  zaxisptr->lbounds = NULL;
  zaxisptr->weights = NULL;
  zaxisptr->type = CDI_UNDEFID;
  zaxisptr->ltype = 0;
  zaxisptr->ltype2 = -1;
  zaxisptr->positive = 0;
  zaxisptr->scalar = 0;
  zaxisptr->direction = 0;
  zaxisptr->prec = 0;
  zaxisptr->size = 0;
  zaxisptr->vctsize = 0;
  zaxisptr->vct = NULL;
  zaxisptr->number = 0;
  zaxisptr->nhlev = 0;
  memset(zaxisptr->uuid, 0, CDI_UUID_SIZE);
}


static
zaxis_t *zaxisNewEntry(int id)
{
  zaxis_t *zaxisptr = (zaxis_t *) Malloc(sizeof(zaxis_t));

  zaxisDefaultValue ( zaxisptr );

  if (id == CDI_UNDEFID)
    zaxisptr->self = reshPut(zaxisptr, &zaxisOps);
  else
    {
      zaxisptr->self = id;
      reshReplace(id, zaxisptr, &zaxisOps);
    }

  return (zaxisptr);
}

static inline zaxis_t *
zaxisID2Ptr(int id)
{
  return (zaxis_t *)reshGetVal(id, &zaxisOps);
}


static
void zaxisInit(void)
{
  static int zaxisInitialized = 0;
  char *env;

  if ( zaxisInitialized ) return;

  zaxisInitialized = 1;

  env = getenv("ZAXIS_DEBUG");
  if ( env ) ZAXIS_Debug = atoi(env);
}

static
void zaxis_copy(zaxis_t *zaxisptr2, zaxis_t *zaxisptr1)
{
  int zaxisID2 = zaxisptr2->self;
  memcpy(zaxisptr2, zaxisptr1, sizeof(zaxis_t));
  zaxisptr2->self = zaxisID2;
}

unsigned cdiZaxisCount(void)
{
  return reshCountType(&zaxisOps);
}


static int
zaxisCreate_(int zaxistype, int size, int id)
{
  zaxis_t *zaxisptr = zaxisNewEntry(id);

  xassert(size >= 0);
  zaxisptr->type = zaxistype;
  zaxisptr->size = size;

  if ( zaxistype >= CDI_NumZaxistype || zaxistype < 0 )
    Error("Internal problem! zaxistype > CDI_MaxZaxistype");

  int zaxisID = zaxisptr->self;
  zaxisDefName(zaxisID, ZaxistypeEntry[zaxistype].name);
  zaxisDefLongname(zaxisID, ZaxistypeEntry[zaxistype].longname);
  zaxisDefUnits(zaxisID, ZaxistypeEntry[zaxistype].units);

  if ( *ZaxistypeEntry[zaxistype].stdname )
    strcpy(zaxisptr->stdname, ZaxistypeEntry[zaxistype].stdname);

  zaxisptr->positive = ZaxistypeEntry[zaxistype].positive;

  double *vals = zaxisptr->vals
    = (double *) Malloc((size_t)size * sizeof(double));

  for ( int ilev = 0; ilev < size; ilev++ )
    vals[ilev] = 0.0;

  return zaxisID;
}
int zaxisCreate(int zaxistype, int size)
{
  if ( CDI_Debug )
    Message("zaxistype: %d size: %d ", zaxistype, size);

  zaxisInit ();
  return zaxisCreate_(zaxistype, size, CDI_UNDEFID);
}


static void zaxisDestroyKernel( zaxis_t * zaxisptr )
{
  xassert ( zaxisptr );

  int id = zaxisptr->self;

  if ( zaxisptr->vals ) Free( zaxisptr->vals );
  if ( zaxisptr->lbounds ) Free( zaxisptr->lbounds );
  if ( zaxisptr->ubounds ) Free( zaxisptr->ubounds );
  if ( zaxisptr->weights ) Free( zaxisptr->weights );
  if ( zaxisptr->vct ) Free( zaxisptr->vct );

  Free( zaxisptr );

  reshRemove ( id, &zaxisOps );
}
void zaxisDestroy(int zaxisID)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  zaxisDestroyKernel ( zaxisptr );
}


static
void zaxisDestroyP ( void * zaxisptr )
{
  zaxisDestroyKernel (( zaxis_t * ) zaxisptr );
}


const char *zaxisNamePtr(int zaxistype)
{
  const char *name = (zaxistype >= 0 && zaxistype < CDI_NumZaxistype)
    ? ZaxistypeEntry[zaxistype].longname
    : ZaxistypeEntry[ZAXIS_GENERIC].longname;
  return (name);
}


void zaxisName(int zaxistype, char *zaxisname)
{
  strcpy(zaxisname, zaxisNamePtr(zaxistype));
}
void zaxisDefName(int zaxisID, const char *name)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  if ( name )
    {
      strncpy(zaxisptr->name, name, CDI_MAX_NAME - 1);
      zaxisptr->name[CDI_MAX_NAME - 1] = '\0';
      reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
    }
}
void zaxisDefLongname(int zaxisID, const char *longname)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  if ( longname )
    {
      strncpy(zaxisptr->longname, longname, CDI_MAX_NAME - 1);
      zaxisptr->longname[CDI_MAX_NAME - 1] = '\0';
      reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
    }
}
void zaxisDefUnits(int zaxisID, const char *units)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  if ( units )
    {
      strncpy(zaxisptr->units, units, CDI_MAX_NAME - 1);
      zaxisptr->units[CDI_MAX_NAME - 1] = '\0';
      reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
    }
}


void zaxisDefPsName(int zaxisID, const char *psname)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  if ( psname )
    {
      strncpy(zaxisptr->psname, psname, CDI_MAX_NAME - 1);
      zaxisptr->name[CDI_MAX_NAME - 1] = '\0';
      reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
    }
}
void zaxisInqName(int zaxisID, char *name)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  strcpy(name, zaxisptr->name);
}
void zaxisInqLongname(int zaxisID, char *longname)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  strcpy(longname, zaxisptr->longname);
}
void zaxisInqUnits(int zaxisID, char *units)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  strcpy(units, zaxisptr->units);
}


void zaxisInqStdname(int zaxisID, char *stdname)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  strcpy(stdname, zaxisptr->stdname);
}


void zaxisInqPsName(int zaxisID, char *psname)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  strcpy(psname, zaxisptr->psname);
}


void zaxisDefPrec(int zaxisID, int prec)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  if (zaxisptr->prec != prec)
    {
      zaxisptr->prec = prec;
      reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
    }
}


int zaxisInqPrec(int zaxisID)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  return (zaxisptr->prec);
}


void zaxisDefPositive(int zaxisID, int positive)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  if (zaxisptr->positive != positive)
    {
      zaxisptr->positive = (unsigned char)positive;
      reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
    }
}


int zaxisInqPositive(int zaxisID)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  return zaxisptr->positive;
}


void zaxisDefScalar(int zaxisID)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  zaxisptr->scalar = 1;
  reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
}

int zaxisInqScalar(int zaxisID)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  return zaxisptr->scalar;
}


void zaxisDefLtype(int zaxisID, int ltype)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  if (zaxisptr->ltype != ltype)
    {
      zaxisptr->ltype = ltype;
      reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
    }
}


int zaxisInqLtype(int zaxisID)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  return zaxisptr->ltype;
}


void zaxisDefLtype2(int zaxisID, int ltype2)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  if (zaxisptr->ltype2 != ltype2)
    {
      zaxisptr->ltype2 = ltype2;
      reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
    }
}


int zaxisInqLtype2(int zaxisID)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  return zaxisptr->ltype2;
}
void zaxisDefLevels(int zaxisID, const double *levels)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  int size = zaxisptr->size;

  double *vals = zaxisptr->vals;

  for (int ilev = 0; ilev < size; ilev++ )
    vals[ilev] = levels[ilev];
  reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
}
void zaxisDefLevel(int zaxisID, int levelID, double level)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  if ( levelID >= 0 && levelID < zaxisptr->size )
    zaxisptr->vals[levelID] = level;
  reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
}


void zaxisDefNlevRef(int zaxisID, const int nhlev)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  if (zaxisptr->nhlev != nhlev)
    {
      zaxisptr->nhlev = nhlev;
      reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
    }
}


int zaxisInqNlevRef(int zaxisID)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  return zaxisptr->nhlev;
}
void zaxisDefNumber(int zaxisID, const int number)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  if (zaxisptr->number != number)
    {
      zaxisptr->number = number;
      reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
    }
}
int zaxisInqNumber(int zaxisID)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  return zaxisptr->number;
}
void zaxisDefUUID(int zaxisID, const unsigned char uuid[CDI_UUID_SIZE])
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  memcpy(zaxisptr->uuid, uuid, CDI_UUID_SIZE);
  reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
}
void zaxisInqUUID(int zaxisID, unsigned char uuid[CDI_UUID_SIZE])
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  memcpy(uuid, zaxisptr->uuid, CDI_UUID_SIZE);
}
double zaxisInqLevel(int zaxisID, int levelID)
{
  double level = 0;
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  if ( levelID >= 0 && levelID < zaxisptr->size )
    level = zaxisptr->vals[levelID];

  return level;
}

double zaxisInqLbound(int zaxisID, int index)
{
  double level = 0;
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  if ( zaxisptr->lbounds && ( index >= 0 && index < zaxisptr->size ) )
      level = zaxisptr->lbounds[index];

  return level;
}


double zaxisInqUbound(int zaxisID, int index)
{
  double level = 0;
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  if ( zaxisptr->ubounds && ( index >= 0 && index < zaxisptr->size ) )
    level = zaxisptr->ubounds[index];
  return level;
}


const double *zaxisInqLevelsPtr(int zaxisID)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  return zaxisptr->vals;
}
void zaxisInqLevels(int zaxisID, double *levels)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  int size = zaxisptr->size;
  for (int i = 0; i < size; i++ )
    levels[i] = zaxisptr->vals[i];
}


int zaxisInqLbounds(int zaxisID, double *lbounds)
{
  int size = 0;
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  if ( zaxisptr->lbounds )
    {
      size = zaxisptr->size;

      if ( lbounds )
        for (int i = 0; i < size; i++ )
          lbounds[i] = zaxisptr->lbounds[i];
    }

  return (size);
}


int zaxisInqUbounds(int zaxisID, double *ubounds)
{
  int size = 0;
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  if ( zaxisptr->ubounds )
    {
      size = zaxisptr->size;

      if ( ubounds )
        for (int i = 0; i < size; i++ )
          ubounds[i] = zaxisptr->ubounds[i];
    }

  return (size);
}


int zaxisInqWeights(int zaxisID, double *weights)
{
  int size = 0;
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  if ( zaxisptr->weights )
    {
      size = zaxisptr->size;

      if ( weights )
        for ( int i = 0; i < size; i++ )
          weights[i] = zaxisptr->weights[i];
    }

  return (size);
}


int zaxisInqLevelID(int zaxisID, double level)
{
  int levelID = CDI_UNDEFID;
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  int size = zaxisptr->size;
  for ( int i = 0; i < size; i++ )
    if ( fabs(level-zaxisptr->vals[i]) < DBL_EPSILON )
      {
        levelID = i;
        break;
      }

  return levelID;
}
int zaxisInqType(int zaxisID)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  return (zaxisptr->type);
}
int zaxisInqSize(int zaxisID)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  return (zaxisptr->size);
}


void cdiCheckZaxis(int zaxisID)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  if ( zaxisInqType(zaxisID) == ZAXIS_GENERIC )
    {
      int size = zaxisptr->size;
      if ( size > 1 )
        {

          if ( ! zaxisptr->direction )
            {
              int ups = 0, downs = 0;
              for ( int i = 1; i < size; i++ )
                {
                  ups += (zaxisptr->vals[i] > zaxisptr->vals[i-1]);
                  downs += (zaxisptr->vals[i] < zaxisptr->vals[i-1]);
                }
              if ( ups == size-1 )
                {
                  zaxisptr->direction = LevelUp;
                }
              else if ( downs == size-1 )
                {
                  zaxisptr->direction = LevelDown;
                }
              else
                {
                  Warning("Direction undefined for zaxisID %d", zaxisID);
                }
            }
        }
    }
}


void zaxisDefVct(int zaxisID, int size, const double *vct)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  if ( zaxisptr->vct == 0 || zaxisptr->vctsize != size )
    {
      zaxisptr->vctsize = size;
      zaxisptr->vct = (double *) Realloc(zaxisptr->vct, (size_t)size*sizeof(double));
    }

  memcpy(zaxisptr->vct, vct, (size_t)size*sizeof(double));
  reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
}


void zaxisInqVct(int zaxisID, double *vct)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  memcpy(vct, zaxisptr->vct, (size_t)zaxisptr->vctsize * sizeof (double));
}


int zaxisInqVctSize(int zaxisID)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  return (zaxisptr->vctsize);
}


const double *zaxisInqVctPtr(int zaxisID)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  return (zaxisptr->vct);
}


void zaxisDefLbounds(int zaxisID, const double *lbounds)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  size_t size = (size_t)zaxisptr->size;

  if ( CDI_Debug )
    if ( zaxisptr->lbounds != NULL )
      Warning("Lower bounds already defined for zaxisID = %d", zaxisID);

  if ( zaxisptr->lbounds == NULL )
    zaxisptr->lbounds = (double *) Malloc(size*sizeof(double));

  memcpy(zaxisptr->lbounds, lbounds, size*sizeof(double));
  reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
}


void zaxisDefUbounds(int zaxisID, const double *ubounds)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  size_t size = (size_t)zaxisptr->size;

  if ( CDI_Debug )
    if ( zaxisptr->ubounds != NULL )
      Warning("Upper bounds already defined for zaxisID = %d", zaxisID);

  if ( zaxisptr->ubounds == NULL )
    zaxisptr->ubounds = (double *) Malloc(size*sizeof(double));

  memcpy(zaxisptr->ubounds, ubounds, size*sizeof(double));
  reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
}


void zaxisDefWeights(int zaxisID, const double *weights)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  size_t size = (size_t)zaxisptr->size;

  if ( CDI_Debug )
    if ( zaxisptr->weights != NULL )
      Warning("Weights already defined for zaxisID = %d", zaxisID);

  if ( zaxisptr->weights == NULL )
    zaxisptr->weights = (double *) Malloc(size*sizeof(double));

  memcpy(zaxisptr->weights, weights, size*sizeof(double));
  reshSetStatus(zaxisID, &zaxisOps, RESH_DESYNC_IN_USE);
}


void zaxisChangeType(int zaxisID, int zaxistype)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);
  zaxisptr->type = zaxistype;
}


void zaxisResize(int zaxisID, int size)
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  xassert(size >= 0);

  zaxisptr->size = size;

  if ( zaxisptr->vals )
    zaxisptr->vals = (double *) Realloc(zaxisptr->vals, (size_t)size * sizeof(double));
}


int zaxisDuplicate(int zaxisID)
{
  int zaxisIDnew;
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  int zaxistype = zaxisInqType(zaxisID);
  int zaxissize = zaxisInqSize(zaxisID);

  zaxisIDnew = zaxisCreate(zaxistype, zaxissize);
  zaxis_t *zaxisptrnew = zaxisID2Ptr(zaxisIDnew);

  zaxis_copy(zaxisptrnew, zaxisptr);

  strcpy(zaxisptrnew->name, zaxisptr->name);
  strcpy(zaxisptrnew->longname, zaxisptr->longname);
  strcpy(zaxisptrnew->units, zaxisptr->units);

  if ( zaxisptr->vals != NULL )
    {
      size_t size = (size_t)zaxissize;

      zaxisptrnew->vals = (double *) Malloc(size * sizeof (double));
      memcpy(zaxisptrnew->vals, zaxisptr->vals, size * sizeof (double));
    }

  if ( zaxisptr->lbounds )
    {
      size_t size = (size_t)zaxissize;

      zaxisptrnew->lbounds = (double *) Malloc(size * sizeof (double));
      memcpy(zaxisptrnew->lbounds, zaxisptr->lbounds, size * sizeof(double));
    }

  if ( zaxisptr->ubounds )
    {
      size_t size = (size_t)zaxissize;

      zaxisptrnew->ubounds = (double *) Malloc(size * sizeof (double));
      memcpy(zaxisptrnew->ubounds, zaxisptr->ubounds, size * sizeof (double));
    }

  if ( zaxisptr->vct != NULL )
    {
      size_t size = (size_t)zaxisptr->vctsize;

      if ( size )
        {
          zaxisptrnew->vctsize = (int)size;
          zaxisptrnew->vct = (double *) Malloc(size * sizeof (double));
          memcpy(zaxisptrnew->vct, zaxisptr->vct, size * sizeof (double));
        }
    }

  return (zaxisIDnew);
}


static void zaxisPrintKernel ( zaxis_t * zaxisptr, int index, FILE * fp )
{
  unsigned char uuid[CDI_UUID_SIZE];
  int levelID;
  int nbyte;
  double level;

  xassert ( zaxisptr );

  int zaxisID = zaxisptr->self;

  int type = zaxisptr->type;
  int nlevels = zaxisptr->size;

  int nbyte0 = 0;
  fprintf(fp, "#\n");
  fprintf(fp, "# zaxisID %d\n", index);
  fprintf(fp, "#\n");
  fprintf(fp, "zaxistype = %s\n", zaxisNamePtr(type));
  fprintf(fp, "size      = %d\n", nlevels);
  if ( zaxisptr->name[0] ) fprintf(fp, "name      = %s\n", zaxisptr->name);
  if ( zaxisptr->longname[0] ) fprintf(fp, "longname  = %s\n", zaxisptr->longname);
  if ( zaxisptr->units[0] ) fprintf(fp, "units     = %s\n", zaxisptr->units);

  nbyte0 = fprintf(fp, "levels    = ");
  nbyte = nbyte0;
  for ( levelID = 0; levelID < nlevels; levelID++ )
    {
      if ( nbyte > 80 )
        {
          fprintf(fp, "\n");
          fprintf(fp, "%*s", nbyte0, "");
          nbyte = nbyte0;
        }
      level = zaxisInqLevel(zaxisID, levelID);
      nbyte += fprintf(fp, "%.9g ", level);
    }
  fprintf(fp, "\n");

  if ( zaxisptr->lbounds && zaxisptr->ubounds )
    {
      double level1, level2;
      nbyte = nbyte0;
      nbyte0 = fprintf(fp, "bounds    = ");
      for ( levelID = 0; levelID < nlevels; levelID++ )
        {
          if ( nbyte > 80 )
            {
              fprintf(fp, "\n");
              fprintf(fp, "%*s", nbyte0, "");
              nbyte = nbyte0;
            }
          level1 = zaxisInqLbound(zaxisID, levelID);
          level2 = zaxisInqUbound(zaxisID, levelID);
          nbyte += fprintf(fp, "%.9g-%.9g ", level1, level2);
        }
      fprintf(fp, "\n");
    }

  if ( type == ZAXIS_HYBRID || type == ZAXIS_HYBRID_HALF )
    {
      int i;
      int vctsize;
      const double *vct;

      vctsize = zaxisptr->vctsize;
      vct = zaxisptr->vct;
      fprintf(fp, "vctsize   = %d\n", vctsize);
      if ( vctsize )
        {
          nbyte0 = fprintf(fp, "vct       = ");
          nbyte = nbyte0;
          for ( i = 0; i < vctsize; i++ )
            {
              if ( nbyte > 70 || i == vctsize/2 )
                {
                  fprintf(fp, "\n%*s", nbyte0, "");
                  nbyte = nbyte0;
                }
              nbyte += fprintf(fp, "%.9g ", vct[i]);
            }
          fprintf(fp, "\n");
        }
    }

  if ( type == ZAXIS_REFERENCE )
    {
      zaxisInqUUID(zaxisID, uuid);
      if ( *uuid )
        {
          const unsigned char *d = uuid;
          fprintf(fp, "uuid      = %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
                  d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
                  d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
        }
    }
}


void zaxisPrint ( int zaxisID, int index )
{
  zaxis_t *zaxisptr = zaxisID2Ptr(zaxisID);

  zaxisPrintKernel ( zaxisptr, index, stdout );
}


static
void zaxisPrintP ( void * voidptr, FILE * fp )
{
  zaxis_t *zaxisptr = ( zaxis_t * ) voidptr;

  xassert ( zaxisptr );

  zaxisPrintKernel(zaxisptr, zaxisptr->self, fp);
}


static int
zaxisCompareP(zaxis_t *z1, zaxis_t *z2)
{
  enum {
    differ = 1,
  };
  int diff = 0;
  xassert(z1 && z2);

  diff |= (z1->type != z2->type)
    | (z1->ltype != z2->ltype)
    | (z1->direction != z2->direction)
    | (z1->prec != z2->prec)
    | (z1->size != z2->size)
    | (z1->vctsize != z2->vctsize)
    | (z1->positive != z2->positive);

  if (diff)
    return differ;
  int size = z1->size;
  int anyPresent = 0;
  int present = (z1->vals != NULL);
  diff |= (present ^ (z2->vals != NULL));
  anyPresent |= present;
  if (!diff && present)
    {
      const double *p = z1->vals, *q = z2->vals;
      for (int i = 0; i < size; i++)
        diff |= IS_NOT_EQUAL(p[i], q[i]);
    }

  present = (z1->lbounds != NULL);
  diff |= (present ^ (z2->lbounds != NULL));
  anyPresent |= present;
  if (!diff && present)
    {
      const double *p = z1->lbounds, *q = z2->lbounds;
      for (int i = 0; i < size; i++)
        diff |= IS_NOT_EQUAL(p[i], q[i]);
    }

  present = (z1->ubounds != NULL);
  diff |= (present ^ (z2->ubounds != NULL));
  anyPresent |= present;
  if (!diff && present)
    {
      const double *p = z1->ubounds, *q = z2->ubounds;
      for (int i = 0; i < size; ++i)
        diff |= IS_NOT_EQUAL(p[i], q[i]);
    }

  present = (z1->weights != NULL);
  diff |= (present ^ (z2->weights != NULL));
  anyPresent |= present;
  if (!diff && present)
    {
      const double *p = z1->weights, *q = z2->weights;
      for (int i = 0; i < size; ++i)
        diff |= IS_NOT_EQUAL(p[i], q[i]);
    }

  present = (z1->vct != NULL);
  diff |= (present ^ (z2->vct != NULL));
  if (!diff && present)
    {
      int vctsize = z1->vctsize;
      xassert(vctsize);
      const double *p = z1->vct, *q = z2->vct;
      for (int i = 0; i < vctsize; ++i)
        diff |= IS_NOT_EQUAL(p[i], q[i]);
    }

  if (anyPresent)
    xassert(size);

  diff |= strcmp(z1->name, z2->name)
    | strcmp(z1->longname, z2->longname)
    | strcmp(z1->stdname, z2->stdname)
    | strcmp(z1->units, z2->units)
    | memcmp(z1->uuid, z2->uuid, CDI_UUID_SIZE);
  return diff != 0;
}


static int
zaxisTxCode ( void )
{
  return ZAXIS;
}

enum { zaxisNint = 8,
       vals = 1 << 0,
       lbounds = 1 << 1,
       ubounds = 1 << 2,
       weights = 1 << 3,
       vct = 1 << 4,
       zaxisHasUUIDFlag = 1 << 5,
};

#define ZAXIS_STR_SERIALIZE { zaxisP->name, zaxisP->longname, \
      zaxisP->stdname, zaxisP->units }

static
int zaxisGetMemberMask ( zaxis_t * zaxisP )
{
  int memberMask = 0;

  if ( zaxisP->vals ) memberMask |= vals;
  if ( zaxisP->lbounds ) memberMask |= lbounds;
  if ( zaxisP->ubounds ) memberMask |= ubounds;
  if ( zaxisP->weights ) memberMask |= weights;
  if ( zaxisP->vct ) memberMask |= vct;
  if (!cdiUUIDIsNull(zaxisP->uuid)) memberMask |= zaxisHasUUIDFlag;
  return memberMask;
}

static int
zaxisGetPackSize(void * voidP, void *context)
{
  zaxis_t * zaxisP = ( zaxis_t * ) voidP;
  int packBufferSize = serializeGetSize(zaxisNint, DATATYPE_INT, context)
    + serializeGetSize(1, DATATYPE_UINT32, context);

  if (zaxisP->vals || zaxisP->lbounds || zaxisP->ubounds || zaxisP->weights)
    xassert(zaxisP->size);

  if ( zaxisP->vals )
    packBufferSize += serializeGetSize(zaxisP->size, DATATYPE_FLT64, context)
      + serializeGetSize(1, DATATYPE_UINT32, context);

  if ( zaxisP->lbounds )
    packBufferSize += serializeGetSize(zaxisP->size, DATATYPE_FLT64, context)
      + serializeGetSize(1, DATATYPE_UINT32, context);

  if ( zaxisP->ubounds )
    packBufferSize += serializeGetSize(zaxisP->size, DATATYPE_FLT64, context)
      + serializeGetSize(1, DATATYPE_UINT32, context);

  if ( zaxisP->weights )
    packBufferSize += serializeGetSize(zaxisP->size, DATATYPE_FLT64, context)
      + serializeGetSize(1, DATATYPE_UINT32, context);

  if ( zaxisP->vct )
    {
      xassert ( zaxisP->vctsize );
      packBufferSize += serializeGetSize(zaxisP->vctsize, DATATYPE_FLT64, context)
        + serializeGetSize(1, DATATYPE_UINT32, context);
    }

  {
    const char *strTab[] = ZAXIS_STR_SERIALIZE;
    size_t numStr = sizeof (strTab) / sizeof (strTab[0]);
    packBufferSize
      += serializeStrTabGetPackSize(strTab, (int)numStr, context);
  }

  packBufferSize += serializeGetSize(1, DATATYPE_UCHAR, context);

  if (!cdiUUIDIsNull(zaxisP->uuid))
    packBufferSize += serializeGetSize(CDI_UUID_SIZE, DATATYPE_UCHAR, context);

  return packBufferSize;
}


void
zaxisUnpack(char * unpackBuffer, int unpackBufferSize,
            int * unpackBufferPos, int originNamespace, void *context,
            int force_id)
{
  int intBuffer[zaxisNint], memberMask;
  uint32_t d;

  serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                  intBuffer, zaxisNint, DATATYPE_INT, context);
  serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                  &d, 1, DATATYPE_UINT32, context);

  xassert(cdiCheckSum(DATATYPE_INT, zaxisNint, intBuffer) == d);

  zaxisInit();

  zaxis_t *zaxisP
    = zaxisNewEntry(force_id ? namespaceAdaptKey(intBuffer[0], originNamespace)
                    : CDI_UNDEFID);

  zaxisP->prec = intBuffer[1];
  zaxisP->type = intBuffer[2];
  zaxisP->ltype = intBuffer[3];
  zaxisP->size = intBuffer[4];
  zaxisP->direction = intBuffer[5];
  zaxisP->vctsize = intBuffer[6];
  memberMask = intBuffer[7];

  if (memberMask & vals)
    {
      int size = zaxisP->size;
      xassert(size >= 0);

      zaxisP->vals = (double *) Malloc((size_t)size * sizeof (double));
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      zaxisP->vals, size, DATATYPE_FLT64, context);
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      &d, 1, DATATYPE_UINT32, context);
      xassert(cdiCheckSum(DATATYPE_FLT, size, zaxisP->vals) == d);
    }

  if (memberMask & lbounds)
    {
      int size = zaxisP->size;
      xassert(size >= 0);

      zaxisP->lbounds = (double *) Malloc((size_t)size * sizeof (double));
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      zaxisP->lbounds, size, DATATYPE_FLT64, context);
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      &d, 1, DATATYPE_UINT32, context);
      xassert(cdiCheckSum(DATATYPE_FLT, size, zaxisP->lbounds) == d);
    }

  if (memberMask & ubounds)
    {
      int size = zaxisP->size;
      xassert(size >= 0);

      zaxisP->ubounds = (double *) Malloc((size_t)size * sizeof (double));
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      zaxisP->ubounds, size, DATATYPE_FLT64, context);
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      &d, 1, DATATYPE_UINT32, context);
      xassert(cdiCheckSum(DATATYPE_FLT, size, zaxisP->ubounds) == d);
    }

  if (memberMask & weights)
    {
      int size = zaxisP->size;
      xassert(size >= 0);

      zaxisP->weights = (double *) Malloc((size_t)size * sizeof (double));
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      zaxisP->weights, size, DATATYPE_FLT64, context);
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      &d, 1, DATATYPE_UINT32, context);
      xassert(cdiCheckSum(DATATYPE_FLT, size, zaxisP->weights) == d);
    }

  if ( memberMask & vct )
    {
      int size = zaxisP->vctsize;
      xassert(size >= 0);

      zaxisP->vct = (double *) Malloc((size_t)size * sizeof (double));
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      zaxisP->vct, size, DATATYPE_FLT64, context);
      serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                      &d, 1, DATATYPE_UINT32, context);
      xassert(cdiCheckSum(DATATYPE_FLT64, size, zaxisP->vct) == d);
    }

  {
    char *strTab[] = ZAXIS_STR_SERIALIZE;
    int numStr = sizeof (strTab) / sizeof (strTab[0]);
    serializeStrTabUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                          strTab, numStr, context);
  }

  serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                  &zaxisP->positive, 1, DATATYPE_UCHAR, context);

  if (memberMask & zaxisHasUUIDFlag)
    serializeUnpack(unpackBuffer, unpackBufferSize, unpackBufferPos,
                    zaxisP->uuid, CDI_UUID_SIZE, DATATYPE_UCHAR, context);

  reshSetStatus(zaxisP->self, &zaxisOps,
                reshGetStatus(zaxisP->self, &zaxisOps) & ~RESH_SYNC_BIT);
}

static void
zaxisPack(void * voidP, void * packBuffer, int packBufferSize,
          int * packBufferPos, void *context)
{
  zaxis_t * zaxisP = ( zaxis_t * ) voidP;
  int intBuffer[zaxisNint];
  int memberMask;
  uint32_t d;

  intBuffer[0] = zaxisP->self;
  intBuffer[1] = zaxisP->prec;
  intBuffer[2] = zaxisP->type;
  intBuffer[3] = zaxisP->ltype;
  intBuffer[4] = zaxisP->size;
  intBuffer[5] = zaxisP->direction;
  intBuffer[6] = zaxisP->vctsize;
  intBuffer[7] = memberMask = zaxisGetMemberMask ( zaxisP );

  serializePack(intBuffer, zaxisNint, DATATYPE_INT,
                packBuffer, packBufferSize, packBufferPos, context);
  d = cdiCheckSum(DATATYPE_INT, zaxisNint, intBuffer);
  serializePack(&d, 1, DATATYPE_UINT32,
                packBuffer, packBufferSize, packBufferPos, context);


  if ( memberMask & vals )
    {
      xassert(zaxisP->size);
      serializePack(zaxisP->vals, zaxisP->size, DATATYPE_FLT64,
                    packBuffer, packBufferSize, packBufferPos, context);
      d = cdiCheckSum(DATATYPE_FLT, zaxisP->size, zaxisP->vals );
      serializePack(&d, 1, DATATYPE_UINT32,
                    packBuffer, packBufferSize, packBufferPos, context);
    }

  if (memberMask & lbounds)
    {
      xassert(zaxisP->size);
      serializePack(zaxisP->lbounds, zaxisP->size, DATATYPE_FLT64,
                    packBuffer, packBufferSize, packBufferPos, context);
      d = cdiCheckSum(DATATYPE_FLT, zaxisP->size, zaxisP->lbounds);
      serializePack(&d, 1, DATATYPE_UINT32,
                    packBuffer, packBufferSize, packBufferPos, context);
    }

  if (memberMask & ubounds)
    {
      xassert(zaxisP->size);

      serializePack(zaxisP->ubounds, zaxisP->size, DATATYPE_FLT64,
                    packBuffer, packBufferSize, packBufferPos, context);
      d = cdiCheckSum(DATATYPE_FLT, zaxisP->size, zaxisP->ubounds);
      serializePack(&d, 1, DATATYPE_UINT32,
                    packBuffer, packBufferSize, packBufferPos, context);
    }

  if (memberMask & weights)
    {
      xassert(zaxisP->size);

      serializePack(zaxisP->weights, zaxisP->size, DATATYPE_FLT64,
                    packBuffer, packBufferSize, packBufferPos, context);
      d = cdiCheckSum(DATATYPE_FLT, zaxisP->size, zaxisP->weights);
      serializePack(&d, 1, DATATYPE_UINT32,
                    packBuffer, packBufferSize, packBufferPos, context);
    }

  if (memberMask & vct)
    {
      xassert(zaxisP->vctsize);

      serializePack(zaxisP->vct, zaxisP->vctsize, DATATYPE_FLT64,
                    packBuffer, packBufferSize, packBufferPos, context);
      d = cdiCheckSum(DATATYPE_FLT64, zaxisP->vctsize, zaxisP->vct);
      serializePack(&d, 1, DATATYPE_UINT32,
                    packBuffer, packBufferSize, packBufferPos, context);
    }

  {
    const char *strTab[] = ZAXIS_STR_SERIALIZE;
    int numStr = sizeof (strTab) / sizeof (strTab[0]);
    serializeStrTabPack(strTab, numStr,
                        packBuffer, packBufferSize, packBufferPos, context);
  }

  serializePack(&zaxisP->positive, 1, DATATYPE_UCHAR,
                packBuffer, packBufferSize, packBufferPos, context);

  if (memberMask & zaxisHasUUIDFlag)
    serializePack(zaxisP->uuid, CDI_UUID_SIZE, DATATYPE_UCHAR,
                  packBuffer, packBufferSize, packBufferPos, context);

}


void cdiZaxisGetIndexList(unsigned nzaxis, int *zaxisResHs)
{
  reshGetResHListOfType(nzaxis, zaxisResHs, &zaxisOps);
}

#undef ZAXIS_STR_SERIALIZE
   static const char cdi_libvers[] = "1.7.1rc1" " of " "Nov  3 2015"" " "13:52:59";
const char *cdiLibraryVersion(void)
{
  return (cdi_libvers);
}
