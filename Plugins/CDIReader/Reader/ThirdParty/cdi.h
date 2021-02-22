// -*- c -*-
/*=========================================================================
 *
 *  Program:   Visualization Toolkit
 *  Module:    cdi.h
 *
 *  Copyright (c) 2018 Niklas Roeber, DKRZ Hamburg
 *  All rights reserved.
 *  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
 *
 *     This software is distributed WITHOUT ANY WARRANTY; without even
 *     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *     PURPOSE.  See the above copyright notice for more information.
 *
 *  =========================================================================*/
// .NAME cdi.h - part of the ICON/CDI netCDF reader

#ifndef  CDI_H_
# define  CDI_H_

# include <stdio.h>
# include <stdint.h>    // int64_t
# include <sys/types.h>

# ifdef __cplusplus
extern "C" {
# endif


# define  CDI_MAX_NAME             256   // max length of a name

# define  CDI_UNDEFID               -1
# define  CDI_GLOBAL                -1   // Global var ID for vlist

/* Byte order */

# define  CDI_BIGENDIAN              0   // Byte order BIGENDIAN
# define  CDI_LITTLEENDIAN           1   // Byte order LITTLEENDIAN
# define  CDI_PDPENDIAN              2

# define  CDI_REAL                   1   // Real numbers
# define  CDI_COMP                   2   // Complex numbers
# define  CDI_BOTH                   3   // Both numbers

/* Error identifier */

# define	 CDI_NOERR        	    0   // No Error
# define  CDI_EEOF                  -1   // The end of file was encountered
# define  CDI_ETMOF                 -9   // Too many open files
# define  CDI_ESYSTEM              -10   // Operating system error
# define  CDI_EINVAL               -20   // Invalid argument
# define  CDI_EISDIR               -21   // Is a directory
# define  CDI_EISEMPTY             -22   // Is empty
# define  CDI_EUFTYPE              -23   // Unsupported file type
# define  CDI_ELIBNAVAIL           -24   // xxx library not available
# define  CDI_EUFSTRUCT            -25   // Unsupported file structure
# define  CDI_EUNC4                -26   // Unsupported NetCDF4 structure
# define  CDI_EDIMSIZE             -27   // Invalid dimension size
# define  CDI_ELIMIT               -99   // Internal limits exceeded

/* File types */

# define  CDI_FILETYPE_GRB           1   // File type GRIB
# define  CDI_FILETYPE_GRB2          2   // File type GRIB version 2
# define  CDI_FILETYPE_NC            3   // File type NetCDF
# define  CDI_FILETYPE_NC2           4   // File type NetCDF version 2 (64-bit offset)
# define  CDI_FILETYPE_NC4           5   // File type NetCDF version 4
# define  CDI_FILETYPE_NC4C          6   // File type NetCDF version 4 (classic)
# define  CDI_FILETYPE_NC5           7   // File type NetCDF version 5 (64-bit data)
# define  CDI_FILETYPE_SRV           8   // File type SERVICE
# define  CDI_FILETYPE_EXT           9   // File type EXTRA
# define  CDI_FILETYPE_IEG          10   // File type IEG

/* Compress types */

# define  CDI_COMPRESS_NONE          0
# define  CDI_COMPRESS_SZIP          1
# define  CDI_COMPRESS_AEC           2
# define  CDI_COMPRESS_ZIP           3
# define  CDI_COMPRESS_JPEG          4

/* external data types */

# define  CDI_DATATYPE_PACK          0
# define  CDI_DATATYPE_PACK1         1
# define  CDI_DATATYPE_PACK2         2
# define  CDI_DATATYPE_PACK3         3
# define  CDI_DATATYPE_PACK4         4
# define  CDI_DATATYPE_PACK5         5
# define  CDI_DATATYPE_PACK6         6
# define  CDI_DATATYPE_PACK7         7
# define  CDI_DATATYPE_PACK8         8
# define  CDI_DATATYPE_PACK9         9
# define  CDI_DATATYPE_PACK10       10
# define  CDI_DATATYPE_PACK11       11
# define  CDI_DATATYPE_PACK12       12
# define  CDI_DATATYPE_PACK13       13
# define  CDI_DATATYPE_PACK14       14
# define  CDI_DATATYPE_PACK15       15
# define  CDI_DATATYPE_PACK16       16
# define  CDI_DATATYPE_PACK17       17
# define  CDI_DATATYPE_PACK18       18
# define  CDI_DATATYPE_PACK19       19
# define  CDI_DATATYPE_PACK20       20
# define  CDI_DATATYPE_PACK21       21
# define  CDI_DATATYPE_PACK22       22
# define  CDI_DATATYPE_PACK23       23
# define  CDI_DATATYPE_PACK24       24
# define  CDI_DATATYPE_PACK25       25
# define  CDI_DATATYPE_PACK26       26
# define  CDI_DATATYPE_PACK27       27
# define  CDI_DATATYPE_PACK28       28
# define  CDI_DATATYPE_PACK29       29
# define  CDI_DATATYPE_PACK30       30
# define  CDI_DATATYPE_PACK31       31
# define  CDI_DATATYPE_PACK32       32
# define  CDI_DATATYPE_CPX32        64
# define  CDI_DATATYPE_CPX64       128
# define  CDI_DATATYPE_FLT32       132
# define  CDI_DATATYPE_FLT64       164
# define  CDI_DATATYPE_INT8        208
# define  CDI_DATATYPE_INT16       216
# define  CDI_DATATYPE_INT32       232
# define  CDI_DATATYPE_UINT8       308
# define  CDI_DATATYPE_UINT16      316
# define  CDI_DATATYPE_UINT32      332

/* internal data types */
# define  CDI_DATATYPE_INT         251
# define  CDI_DATATYPE_FLT         252
# define  CDI_DATATYPE_TXT         253
# define  CDI_DATATYPE_CPX         254
# define  CDI_DATATYPE_UCHAR       255
# define  CDI_DATATYPE_LONG        256
# define  CDI_DATATYPE_UINT        257

/* Chunks */

# define  CDI_CHUNK_AUTO             1  // use default chunk size
# define  CDI_CHUNK_GRID             2
# define  CDI_CHUNK_LINES            3

/* GRID types */

# define  GRID_GENERIC               1  // Generic grid
# define  GRID_GAUSSIAN              2  // Regular Gaussian lon/lat grid
# define  GRID_GAUSSIAN_REDUCED      3  // Reduced Gaussian lon/lat grid
# define  GRID_LONLAT                4  // Regular longitude/latitude grid
# define  GRID_SPECTRAL              5  // Spherical harmonic coefficients
# define  GRID_FOURIER               6  // Fourier coefficients
# define  GRID_GME                   7  // Icosahedral-hexagonal GME grid
# define  GRID_TRAJECTORY            8  // Trajectory
# define  GRID_UNSTRUCTURED          9  // General unstructured grid
# define  GRID_CURVILINEAR          10  // Curvilinear grid
# define  GRID_PROJECTION           12  // Projected coordinates
# define  GRID_CHARXY               13  // One horizontal character dimension

# define  CDI_PROJ_RLL              21  // Rotated Latitude Longitude
# define  CDI_PROJ_LCC              22  // Lambert Conformal Conic
# define  CDI_PROJ_LAEA             23  // Lambert Azimuthal Equal Area
# define  CDI_PROJ_SINU             24  // Sinusoidal
# define  CDI_PROJ_STERE            25  // Polar stereographic

/* ZAXIS types */

# define  ZAXIS_SURFACE              0  // Surface level
# define  ZAXIS_GENERIC              1  // Generic level
# define  ZAXIS_HYBRID               2  // Hybrid level
# define  ZAXIS_HYBRID_HALF          3  // Hybrid half level
# define  ZAXIS_PRESSURE             4  // Isobaric pressure level in Pascal
# define  ZAXIS_HEIGHT               5  // Height above ground
# define  ZAXIS_DEPTH_BELOW_SEA      6  // Depth below sea level in meters
# define  ZAXIS_DEPTH_BELOW_LAND     7  // Depth below land surface in centimeters
# define  ZAXIS_ISENTROPIC           8  // Isentropic
# define  ZAXIS_TRAJECTORY           9  // Trajectory
# define  ZAXIS_ALTITUDE            10  // Altitude above mean sea level in meters
# define  ZAXIS_SIGMA               11  // Sigma level
# define  ZAXIS_MEANSEA             12  // Mean sea level
# define  ZAXIS_TOA                 13  // Norminal top of atmosphere
# define  ZAXIS_SEA_BOTTOM          14  // Sea bottom
# define  ZAXIS_ATMOSPHERE          15  // Entire atmosphere
# define  ZAXIS_CLOUD_BASE          16  // Cloud base level
# define  ZAXIS_CLOUD_TOP           17  // Level of cloud tops
# define  ZAXIS_ISOTHERM_ZERO       18  // Level of 0o C isotherm
# define  ZAXIS_SNOW                19  // Snow level
# define  ZAXIS_LAKE_BOTTOM         20  // Lake or River Bottom
# define  ZAXIS_SEDIMENT_BOTTOM     21  // Bottom Of Sediment Layer
# define  ZAXIS_SEDIMENT_BOTTOM_TA  22  // Bottom Of Thermally Active Sediment Layer
# define  ZAXIS_SEDIMENT_BOTTOM_TW  23  // Bottom Of Sediment Layer Penetrated By Thermal Wave
# define  ZAXIS_MIX_LAYER           24  // Mixing Layer
# define  ZAXIS_REFERENCE           25  // zaxis reference number
# define  ZAXIS_CHAR                26  // Area types

/* SUBTYPE types */

    enum {
        SUBTYPE_TILES                   = 0                 // Tiles variable
      };

# define MAX_KV_PAIRS_MATCH 10

    /* Data structure defining a key-value search, possibly with multiple
     * key-value pairs in combination.
     * 
     * Currently, only multiple pairs combined by AND are supported.
     */
    typedef struct  {
        int nAND;                                   /* no. of key-value pairs that have to match */
        int key_value_pairs[2][MAX_KV_PAIRS_MATCH]; /* key-value pairs */
      } subtype_query_t;



/* TIME types */

# define  TIME_CONSTANT              0  /* Time constant              */
# define  TIME_VARYING               1  /* Time varying               */
# define  TIME_VARIABLE              1  /* obsolate, use TIME_VARYING */

/* TSTEP types */

# define  TSTEP_INSTANT              1  /* Instant             */
# define  TSTEP_AVG                  2  /* Average             */
# define  TSTEP_ACCUM                3  /* Accumulation        */
# define  TSTEP_MAX                  4  /* Maximum             */
# define  TSTEP_MIN                  5  /* Minimum             */
# define  TSTEP_DIFF                 6  /* Difference          */
# define  TSTEP_RMS                  7  /* Root mean square    */
# define  TSTEP_SD                   8  /* Standard deviation  */
# define  TSTEP_COV                  9  /* Covariance          */
# define  TSTEP_RATIO               10  /* Ratio               */
# define  TSTEP_RANGE               11
# define  TSTEP_INSTANT2            12
# define  TSTEP_INSTANT3            13

/* TAXIS types */

# define  TAXIS_ABSOLUTE           1
# define  TAXIS_RELATIVE           2
# define  TAXIS_FORECAST           3

/* TUNIT types */

# define  TUNIT_SECOND             1
# define  TUNIT_MINUTE             2
# define  TUNIT_QUARTER            3
# define  TUNIT_30MINUTES          4
# define  TUNIT_HOUR               5
# define  TUNIT_3HOURS             6
# define  TUNIT_6HOURS             7
# define  TUNIT_12HOURS            8
# define  TUNIT_DAY                9
# define  TUNIT_MONTH             10
# define  TUNIT_YEAR              11

/* CALENDAR types */

# define  CALENDAR_STANDARD        0  /* don't change this value (used also in cgribexlib)! */
# define  CALENDAR_GREGORIAN       1
# define  CALENDAR_PROLEPTIC       2
# define  CALENDAR_360DAYS         3
# define  CALENDAR_365DAYS         4
# define  CALENDAR_366DAYS         5
# define  CALENDAR_NONE            6

/* number of unsigned char needed to store UUID */
# define  CDI_UUID_SIZE           16

/* Structs that are used to return data to the user */

    typedef struct CdiParam { int discipline; int category; int number; } CdiParam;


    /* Opaque types */
    typedef struct CdiIterator CdiIterator;
    typedef struct CdiGribIterator CdiGribIterator;

/* CDI control routines */

    void    cdiReset(void);

    const char *cdiStringError(int cdiErrno);

    void    cdiDebug(int debug);

    const char *cdiLibraryVersion(void);
    void    cdiPrintVersion(void);

    int     cdiHaveFiletype(int filetype);

    void    cdiDefMissval(double missval);
    double  cdiInqMissval(void);
    double  cdiInqGridMissval(void);
    void    cdiDefGlobal(const char *string, int val);

    int     namespaceNew(void);
    void    namespaceSetActive(int namespaceID);
    int     namespaceGetActive(void);
    void    namespaceDelete(int namespaceID);


/* CDI converter routines */

/* parameter */

    void    cdiParamToString(int param, char *paramstr, int maxlen);

    void    cdiDecodeParam(int param, int *pnum, int *pcat, int *pdis);
    int     cdiEncodeParam(int pnum, int pcat, int pdis);

/* date format:  YYYYMMDD */
/* time format:    hhmmss */

    void    cdiDecodeDate(int64_t date, int *year, int *month, int *day);
    int64_t cdiEncodeDate(int year, int month, int day);

    void    cdiDecodeTime(int time, int *hour, int *minute, int *second);
    int     cdiEncodeTime(int hour, int minute, int second);


/* STREAM control routines */

    int     cdiGetFiletype(const char *path, int *byteorder);

    /*      streamOpenRead: Open a dataset for reading */
    int     streamOpenRead(const char *path);

    /*      streamOpenWrite: Create a new dataset */
    int     streamOpenWrite(const char *path, int filetype);

    int     streamOpenAppend(const char *path);

    /*      streamClose: Close an open dataset */
    void    streamClose(int streamID);

    /*      streamSync: Synchronize an Open Dataset to Disk */
    void    streamSync(int streamID);

    /*      streamDefVlist: Define the Vlist for a stream */
    void    streamDefVlist(int streamID, int vlistID);

    /*      streamInqVlist: Get the Vlist of a stream */
    int     streamInqVlist(int streamID);

    /*      streamInqFiletype: Get the filetype */
    int     streamInqFiletype(int streamID);

    /*      streamDefByteorder: Define the byteorder */
    void    streamDefByteorder(int streamID, int byteorder);

    /*      streamInqByteorder: Get the byteorder */
    int     streamInqByteorder(int streamID);

    /*      streamDefCompType: Define compression type */
    void    streamDefCompType(int streamID, int comptype);

    /*      streamInqCompType: Get compression type */
    int     streamInqCompType(int streamID);

    /*      streamDefCompLevel: Define compression level */
    void    streamDefCompLevel(int streamID, int complevel);

    /*      streamInqCompLevel: Get compression level */
    int     streamInqCompLevel(int streamID);

    /*      streamDefTimestep: Define time step */
    int     streamDefTimestep(int streamID, int tsID);

    /*      streamInqTimestep: Get time step */
    int     streamInqTimestep(int streamID, int tsID);

    /*      PIO: query currently set timestep id  */
    int     streamInqCurTimestepID(int streamID);

    const char *streamFilename(int streamID);
    const char *streamFilesuffix(int filetype);

    size_t  streamNvals(int streamID);

    int     streamInqNvars(int streamID);

/* STREAM var I/O routines (random access) */

    /*      streamWriteVar: Write a variable */
    void    streamWriteVar(int streamID, int varID, const double data[], size_t nmiss);
    void    streamWriteVarF(int streamID, int varID, const float data[], size_t nmiss);

    /*      streamReadVar: Read a variable */
    void    streamReadVar(int streamID, int varID, double data[], size_t *nmiss);
    void    streamReadVarF(int streamID, int varID, float data[], size_t *nmiss);
    void    streamReadVarPart(int streamID, int varID, int varType, int start, size_t size, void *data, size_t *nmiss, int memtype);

    /*      streamWriteVarSlice: Write a horizontal slice of a variable */
    void    streamWriteVarSlice(int streamID, int varID, int levelID, const double data[], size_t nmiss);
    void    streamWriteVarSliceF(int streamID, int varID, int levelID, const float data[], size_t nmiss);
    void    streamReadVarSlicePart(int streamID, int varID, int levelID, int varType, int start, size_t size, void *data, size_t *nmiss, int memtype);

    /*      streamReadVarSlice: Read a horizontal slice of a variable */
    void    streamReadVarSlice(int streamID, int varID, int levelID, double data[], size_t *nmiss);
    void    streamReadVarSliceF(int streamID, int varID, int levelID, float data[], size_t *nmiss);

    void    streamWriteVarChunk(int streamID, int varID, const int rect[3][2], const double data[], size_t nmiss);


/* STREAM record I/O routines (sequential access) */

    void    streamDefRecord(int streamID, int  varID, int  levelID);
    void    streamInqRecord(int streamID, int *varID, int *levelID);
    void    streamWriteRecord(int streamID, const double data[], size_t nmiss);
    void    streamWriteRecordF(int streamID, const float data[], size_t nmiss);
    void    streamReadRecord(int streamID, double data[], size_t *nmiss);
    void    streamReadRecordF(int streamID, float data[], size_t *nmiss);
    void    streamCopyRecord(int streamIDdest, int streamIDsrc);

    void    streamInqGRIBinfo(int streamID, int *intnum, float *fltnum, off_t *bignum);


/* File driven I/O (may yield better performance than using the streamXXX functions) */

    //Creation & Destruction
    CdiIterator *cdiIterator_new(const char *path);         //Requires a subsequent call to cdiIteratorNextField() to point the iterator at the first field.
    CdiIterator *cdiIterator_clone(CdiIterator *me);
    char *cdiIterator_serialize(CdiIterator *me);           //Returns a malloc'ed string.
    //description is a string that was returned by cdiIteratorSerialize(). Returns a copy of the original iterator.
    CdiIterator *cdiIterator_deserialize(const char *description);
    void cdiIterator_print(CdiIterator *me, FILE *stream);
    void cdiIterator_delete(CdiIterator *me);

    //Advancing an iterator
    int cdiIterator_nextField(CdiIterator *me);             //Points the iterator at the next field, returns CDI_EEOF if there are no more fields in the file.

    //Introspecting metadata
    //All outXXX arguments to these functions may be NULL.
    char *cdiIterator_inqStartTime(CdiIterator *me);        //Returns the (start) time as an ISO-8601 coded string. The caller is responsible to Free() the returned string.
    char *cdiIterator_inqEndTime(CdiIterator *me);          //Returns the end time of an integration period as an ISO-8601 coded string, or NULL if there is no end time. The caller is responsible to Free() the returned string.
    char *cdiIterator_inqRTime(CdiIterator *me);            //Returns the reference date as an ISO-8601 coded string. The caller is responsible to Free() the returned string.
    char *cdiIterator_inqVTime(CdiIterator *me);            //Returns the validity date as an ISO-8601 coded string. The caller is responsible to Free() the returned string.
    //callers are responsible to Free() strings that they request
    int cdiIterator_inqLevelType(CdiIterator *me, int levelSelector, char **outName_optional, char **outLongName_optional, char **outStdName_optional, char **outUnit_optional);
    //outValue2 is only written to if the level is a hybrid level
    int cdiIterator_inqLevel(CdiIterator *me, int levelSelector, double *outValue1_optional, double *outValue2_optional);
    //outUuid must point to a buffer of 16 bytes, returns an error code if no generalized zaxis is used.
    int cdiIterator_inqLevelUuid(CdiIterator *me, int *outVgridNumber_optional, int *outLevelCount_optional, unsigned char outUuid_optional[CDI_UUID_SIZE]);
    //Returns CDI_EINVAL if there is no tile information connected to the current field, *outTileIndex and *outTileAttribute will be set to -1 in this case.
    int cdiIterator_inqTile(CdiIterator *me, int *outTileIndex, int *outTileAttribute);
    //outTileAttributeCount is the count for the tile associated with the current field, a total attribute count cannot be inquired. Returns CDI_EINVAL if there is no tile information connected to the current field, *outTileCount and *outTileAttributeCount will be set to 0 in this case.
    int cdiIterator_inqTileCount(CdiIterator *me, int *outTileCount, int *outTileAttributeCount);
    CdiParam cdiIterator_inqParam(CdiIterator *me);
    //Some FORTRAN compilers produce wrong code for the cdiIterator_inqParam()-wrapper, rendering it unusable from FORTRAN. This function is the workaround.
    void cdiIterator_inqParamParts(CdiIterator *me, int *outDiscipline, int *outCategory, int *outNumber);
    int cdiIterator_inqDatatype(CdiIterator *me);
    int cdiIterator_inqFiletype(CdiIterator *me);
    int cdiIterator_inqTsteptype(CdiIterator *me);
    char *cdiIterator_inqVariableName(CdiIterator *me);     //The caller is responsible to Free() the returned buffer.
    int cdiIterator_inqGridId(CdiIterator *me);             //The returned id is only valid until the next call to cdiIteratorNextField().

    //Reading data
    void cdiIterator_readField(CdiIterator *me, double data[], size_t *nmiss_optional);
    void cdiIterator_readFieldF(CdiIterator *me, float data[], size_t *nmiss_optional);
    //TODO[NH]: Add functions to read partial fields.


    //Direct access to grib fields
    CdiGribIterator *cdiGribIterator_clone(CdiIterator *me);  //Returns NULL if the associated file is not a GRIB file.
    void cdiGribIterator_delete(CdiGribIterator *me);

    //Callthroughs to GRIB-API
    //Same semantics as grib_get_long().
    int cdiGribIterator_getLong(CdiGribIterator *me, const char *key, long *value);
    //Same semantics as grib_get_double().
    int cdiGribIterator_getDouble(CdiGribIterator *me, const char *key, double *value);
    //Same semantics as grib_get_length().
    int cdiGribIterator_getLength(CdiGribIterator *me, const char *key, size_t *value);
    //Same semantics as grib_get_string().
    int cdiGribIterator_getString(CdiGribIterator *me, const char *key, char *value, size_t *length);
    //Same semantics as grib_get_size().
    int cdiGribIterator_getSize(CdiGribIterator *me, const char *key, size_t *value);
    //Same semantics as grib_get_long_array().
    int cdiGribIterator_getLongArray(CdiGribIterator *me, const char *key, long *value, size_t *array_size);
    //Same semantics as grib_get_double_array().
    int cdiGribIterator_getDoubleArray(CdiGribIterator *me, const char *key, double *value, size_t *array_size);

    //Convenience functions for accessing GRIB-API keys
    int cdiGribIterator_inqEdition(CdiGribIterator *me);
    //Aborts on failure to fetch the given key.
    long cdiGribIterator_inqLongValue(CdiGribIterator *me, const char *key);
    //Returns the default value if the given key is not present.
    long cdiGribIterator_inqLongDefaultValue(CdiGribIterator *me, const char *key, long defaultValue);
    //Aborts on failure to fetch the given key.
    double cdiGribIterator_inqDoubleValue(CdiGribIterator *me, const char *key);
    //Returns the default value if the given key is not present.
    double cdiGribIterator_inqDoubleDefaultValue(CdiGribIterator *me, const char *key, double defaultValue);
    //Returns a malloc'ed string.
    char *cdiGribIterator_inqStringValue(CdiGribIterator *me, const char *key);

/* VLIST routines */

    /*      vlistCreate: Create a variable list */
    int     vlistCreate(void);

    /*      vlistDestroy: Destroy a variable list */
    void    vlistDestroy(int vlistID);

    /*      vlistDuplicate: Duplicate a variable list */
    int     vlistDuplicate(int vlistID);

    /*      vlistCopy: Copy a variable list */
    void    vlistCopy(int vlistID2, int vlistID1);

    /*      vlistCopyFlag: Copy some entries of a variable list */
    void    vlistCopyFlag(int vlistID2, int vlistID1);

    void    vlistClearFlag(int vlistID);

    /*      vlistCat: Concatenate two variable lists */
    void    vlistCat(int vlistID2, int vlistID1);

    /*      vlistMerge: Merge two variable lists */
    void    vlistMerge(int vlistID2, int vlistID1);

    void    vlistPrint(int vlistID);

    /*      vlistNumber: Number type in a variable list */
    int     vlistNumber(int vlistID);

    /*      vlistNvars: Number of variables in a variable list */
    int     vlistNvars(int vlistID);

    /*      vlistNgrids: Number of grids in a variable list */
    int     vlistNgrids(int vlistID);

    /*      vlistNzaxis: Number of zaxis in a variable list */
    int     vlistNzaxis(int vlistID);

    /*      vlistNsubtypes: Number of subtypes in a variable list */
    int     vlistNsubtypes(int vlistID);

    void    vlistDefNtsteps(int vlistID, int nts);
    int     vlistNtsteps(int vlistID);
    size_t  vlistGridsizeMax(int vlistID);
    int     vlistGrid(int vlistID, int index);
    int     vlistGridIndex(int vlistID, int gridID);
    void    vlistChangeGridIndex(int vlistID, int index, int gridID);
    void    vlistChangeGrid(int vlistID, int gridID1, int gridID2);
    int     vlistZaxis(int vlistID, int index);
    int     vlistZaxisIndex(int vlistID, int zaxisID);
    void    vlistChangeZaxisIndex(int vlistID, int index, int zaxisID);
    void    vlistChangeZaxis(int vlistID, int zaxisID1, int zaxisID2);
    int     vlistNrecs(int vlistID);
    int     vlistSubtype(int vlistID, int index);
    int     vlistSubtypeIndex(int vlistID, int subtypeID);

    /*      vlistDefTaxis: Define the time axis of a variable list */
    void    vlistDefTaxis(int vlistID, int taxisID);

    /*      vlistInqTaxis: Get the time axis of a variable list */
    int     vlistInqTaxis(int vlistID);

    void    vlistDefTable(int vlistID, int tableID);
    int     vlistInqTable(int vlistID);
    void    vlistDefInstitut(int vlistID, int instID);
    int     vlistInqInstitut(int vlistID);
    void    vlistDefModel(int vlistID, int modelID);
    int     vlistInqModel(int vlistID);


/* VLIST VAR routines */

    /*      vlistDefVarTiles: Create a new tile-based variable */
    int     vlistDefVarTiles(int vlistID, int gridID, int zaxisID, int timetype, int tilesetID);

    /*      vlistDefVar: Create a new variable */
    int     vlistDefVar(int vlistID, int gridID, int zaxisID, int timetype);

    void    vlistChangeVarGrid(int vlistID, int varID, int gridID);
    void    vlistChangeVarZaxis(int vlistID, int varID, int zaxisID);

    void    vlistInqVar(int vlistID, int varID, int *gridID, int *zaxisID, int *timetype);
    int     vlistInqVarGrid(int vlistID, int varID);
    int     vlistInqVarZaxis(int vlistID, int varID);

    /* used in MPIOM */
    int     vlistInqVarID(int vlistID, int code);

    void    vlistDefVarTimetype(int vlistID, int varID, int timetype);
    int     vlistInqVarTimetype(int vlistID, int varID);

    void    vlistDefVarTsteptype(int vlistID, int varID, int tsteptype);

    /*      vlistInqVarTsteptype: Get the timestep type of a Variable */
    int     vlistInqVarTsteptype(int vlistID, int varID);

    void    vlistDefVarCompType(int vlistID, int varID, int comptype);
    int     vlistInqVarCompType(int vlistID, int varID);
    void    vlistDefVarCompLevel(int vlistID, int varID, int complevel);
    int     vlistInqVarCompLevel(int vlistID, int varID);

    /*      vlistDefVarParam: Define the parameter number of a Variable */
    void    vlistDefVarParam(int vlistID, int varID, int param);

    /*      vlistInqVarParam: Get the parameter number of a Variable */
    int     vlistInqVarParam(int vlistID, int varID);

    /*      vlistDefVarCode: Define the code number of a Variable */
    void    vlistDefVarCode(int vlistID, int varID, int code);

    /*      vlistInqVarCode: Get the code number of a Variable */
    int     vlistInqVarCode(int vlistID, int varID);

    /*      vlistDefVarDatatype: Define the data type of a Variable */
    void    vlistDefVarDatatype(int vlistID, int varID, int datatype);

    /*      vlistInqVarDatatype: Get the data type of a Variable */
    int     vlistInqVarDatatype(int vlistID, int varID);

    void    vlistDefVarChunkType(int vlistID, int varID, int chunktype);
    int     vlistInqVarChunkType(int vlistID, int varID);

    void    vlistDefVarXYZ(int vlistID, int varID, int xyz);
    int     vlistInqVarXYZ(int vlistID, int varID);

    int     vlistInqVarNumber(int vlistID, int varID);

    void    vlistDefVarInstitut(int vlistID, int varID, int instID);
    int     vlistInqVarInstitut(int vlistID, int varID);
    void    vlistDefVarModel(int vlistID, int varID, int modelID);
    int     vlistInqVarModel(int vlistID, int varID);
    void    vlistDefVarTable(int vlistID, int varID, int tableID);
    int     vlistInqVarTable(int vlistID, int varID);

    /*      vlistDefVarName: Define the name of a Variable */
    void    vlistDefVarName(int vlistID, int varID, const char *name);

    /*      vlistInqVarName: Get the name of a Variable */
    void    vlistInqVarName(int vlistID, int varID, char *name);

    /*      vlistCopyVarName: Safe and convenient version of vlistInqVarName */
    char   *vlistCopyVarName(int vlistId, int varId);

    /*      vlistDefVarStdname: Define the standard name of a Variable */
    void    vlistDefVarStdname(int vlistID, int varID, const char *stdname);

    /*      vlistInqVarStdname: Get the standard name of a Variable */
    void    vlistInqVarStdname(int vlistID, int varID, char *stdname);

    /*      vlistDefVarLongname: Define the long name of a Variable */
    void    vlistDefVarLongname(int vlistID, int varID, const char *longname);

    /*      vlistInqVarLongname: Get the long name of a Variable */
    void    vlistInqVarLongname(int vlistID, int varID, char *longname);

    /*      vlistDefVarUnits: Define the units of a Variable */
    void    vlistDefVarUnits(int vlistID, int varID, const char *units);

    /*      vlistInqVarUnits: Get the units of a Variable */
    void    vlistInqVarUnits(int vlistID, int varID, char *units);

    /*      vlistDefVarMissval: Define the missing value of a Variable */
    void    vlistDefVarMissval(int vlistID, int varID, double missval);

    /*      vlistInqVarMissval: Get the missing value of a Variable */
    double  vlistInqVarMissval(int vlistID, int varID);

    /*      vlistDefVarExtra: Define extra information of a Variable */
    void    vlistDefVarExtra(int vlistID, int varID, const char *extra);

    /*      vlistInqVarExtra: Get extra information of a Variable */
    void    vlistInqVarExtra(int vlistID, int varID, char *extra);

    void    vlistDefVarScalefactor(int vlistID, int varID, double scalefactor);
    double  vlistInqVarScalefactor(int vlistID, int varID);
    void    vlistDefVarAddoffset(int vlistID, int varID, double addoffset);
    double  vlistInqVarAddoffset(int vlistID, int varID);

    void    vlistDefVarTimave(int vlistID, int varID, int timave);
    int     vlistInqVarTimave(int vlistID, int varID);

    size_t  vlistInqVarSize(int vlistID, int varID);

    void    vlistDefIndex(int vlistID, int varID, int levID, int index);
    int     vlistInqIndex(int vlistID, int varID, int levID);
    void    vlistDefFlag(int vlistID, int varID, int levID, int flag);
    int     vlistInqFlag(int vlistID, int varID, int levID);
    int     vlistFindVar(int vlistID, int fvarID);
    int     vlistFindLevel(int vlistID, int fvarID, int flevelID);
    int     vlistMergedVar(int vlistID, int varID);
    int     vlistMergedLevel(int vlistID, int varID, int levelID);

    /* cdiClearAdditionalKeys: Clear the list of additional GRIB keys. */
    void    cdiClearAdditionalKeys(void);
    /* cdiDefAdditionalKey: Register an additional GRIB key which is read when file is opened. */
    void    cdiDefAdditionalKey(const char *string);

    /* vlistDefVarIntKey: Set an arbitrary keyword/integer value pair for GRIB API */
    void    vlistDefVarIntKey(int vlistID, int varID, const char *name, int value);
    /* vlistDefVarDblKey: Set an arbitrary keyword/double value pair for GRIB API */
    void    vlistDefVarDblKey(int vlistID, int varID, const char *name, double value);

    /* vlistHasVarKey: returns 1 if meta-data key was read, 0 otherwise. */
    int     vlistHasVarKey(int vlistID, int varID, const char *name);
    /* vlistInqVarDblKey: raw access to GRIB meta-data */
    double  vlistInqVarDblKey(int vlistID, int varID, const char *name);
    /* vlistInqVarIntKey: raw access to GRIB meta-data */
    int     vlistInqVarIntKey(int vlistID, int varID, const char *name);

/* CDI attributes */

    /*      cdiInqNatts: Get number of attributes assigned to this variable */
    int     cdiInqNatts(int cdiID, int varID, int *nattsp);
    /*      cdiInqAtt: Get information about an attribute */
    int     cdiInqAtt(int cdiID, int varID, int attrnum, char *name, int *typep, int *lenp);
    int     cdiDelAtt(int cdiID, int varID, const char *name);

    int     cdiCopyAtts(int cdiID1, int varID1, int cdiID2, int varID2);

    /*      cdiDefAttInt: Define an integer attribute */
    int     cdiDefAttInt(int cdiID, int varID, const char *name, int type, int len, const int ip[]);
    /*      cdiDefAttFlt: Define a floating point attribute */
    int     cdiDefAttFlt(int cdiID, int varID, const char *name, int type, int len, const double dp[]);
    /*      cdiDefAttTxt: Define a text attribute */
    int     cdiDefAttTxt(int cdiID, int varID, const char *name, int len, const char *tp_cbuf);

    /*      cdiInqAttInt: Get the value(s) of an integer attribute */
    int     cdiInqAttInt(int cdiID, int varID, const char *name, int mlen, int ip[]);
    /*      cdiInqAttFlt: Get the value(s) of a floating point attribute */
    int     cdiInqAttFlt(int cdiID, int varID, const char *name, int mlen, double dp[]);
    /*      cdiInqAttTxt: Get the value(s) of a text attribute */
    int     cdiInqAttTxt(int cdiID, int varID, const char *name, int mlen, char *tp_cbuf);


/* GRID routines */

    void    gridName(int gridtype, char *gridname);
    const char *gridNamePtr(int gridtype);

    void    gridCompress(int gridID);

    void    gridDefMaskGME(int gridID, const int mask[]);
    int     gridInqMaskGME(int gridID, int mask[]);

    void    gridDefMask(int gridID, const int mask[]);
    int     gridInqMask(int gridID, int mask[]);

    void    gridPrint(int gridID, int opt);

    /*      gridCreate: Create a horizontal Grid */
    int     gridCreate(int gridtype, size_t size);

    /*      gridDestroy: Destroy a horizontal Grid */
    void    gridDestroy(int gridID);

    /*      gridDuplicate: Duplicate a Grid */
    int     gridDuplicate(int gridID);

    /*      gridDefProj: Define the projection ID of a Grid */
    void    gridDefProj(int gridID, int projID);

    /*      gridInqProj: Get the projection ID of a Grid */
    int     gridInqProj(int gridID);

    /*      gridInqProjType: Get the projection type */
    int     gridInqProjType(int gridID);

    /*      gridInqType: Get the type of a Grid */
    int     gridInqType(int gridID);

    /*      gridInqSize: Get the size of a Grid */
    size_t  gridInqSize(int gridID);

    /*      gridDefXsize: Define the size of a X-axis */
    void    gridDefXsize(int gridID, size_t xsize);

    /*      gridInqXsize: Get the size of a X-axis */
    size_t  gridInqXsize(int gridID);

    /*      gridDefYsize: Define the size of a Y-axis */
    void    gridDefYsize(int gridID, size_t ysize);

    /*      gridInqYsize: Get the size of a Y-axis */
    size_t  gridInqYsize(int gridID);

    /*      gridDefNP: Define the number of parallels between a pole and the equator */
    void    gridDefNP(int gridID, int np);

    /*      gridInqNP: Get the number of parallels between a pole and the equator */
    int     gridInqNP(int gridID);

    /*      gridDefXvals: Define the values of a X-axis */
    void    gridDefXvals(int gridID, const double xvals[]);

    /*      gridInqXvals: Get all values of a X-axis */
    size_t  gridInqXvals(int gridID, double xvals[]);
    size_t  gridInqXvalsPart(int gridID, int start, size_t size, double xvals[]);

    /*      gridInqXIsc: Find out whether X-coordinate is of type CHAR */
    int     gridInqXIsc(int gridID);

    /*      gridInqXCvals: Get strings from X-axis in case grid is of type GRID_CHARXY */
    size_t  gridInqXCvals(int gridID, char *xcvals[]);

    /*      gridDefYvals: Define the values of a Y-axis */
    void    gridDefYvals(int gridID, const double yvals[]);

    /*      gridInqYvals: Get all values of a Y-axis */
    size_t  gridInqYvals(int gridID, double yvals[]);
    size_t  gridInqYvalsPart(int gridID, int start, size_t size, double yvals[]);

    /*      gridInqYIsc: Find out whether Y-coordinate is of type CHAR */
    int     gridInqYIsc(int gridID);

    /*      gridInqYCvals: Get strings from Y-axis in case grid is of type GRID_CHARXY */
    size_t  gridInqYCvals(int gridID, char *ycvals[]);

// CDI var keys
# define  CDI_KEY_TABLESVERSION                 801     // GRIB2 tablesVersion
# define  CDI_KEY_LOCALTABLESVERSION            802  // GRIB2 localTablesVersion
# define  CDI_KEY_TYPEOFGENERATINGPROCESS       803  // GRIB2 typeOfGeneratingProcess
# define  CDI_KEY_PRODUCTDEFINITIONTEMPLATE     804  // GRIB2 productDefinitionTemplate
# define  CDI_KEY_TYPEOFPROCESSEDDATA           805  // GRIB2 typeOfProcessedData
# define  CDI_KEY_SHAPEOFTHEEARTH               806  // GRIB2 shapeOfTheEarth
# define  CDI_KEY_BACKGROUNDPROCESS             807  // GRIB2 backgroundProcess
# define  CDI_KEY_TYPEOFENSEMBLEFORECAST        808  // GRIB2 typeOfEnsembleForecast
# define  CDI_KEY_NUMBEROFFORECASTSINENSEMBLE   809  // GRIB2 numberOfForecastsInEnsemble
# define  CDI_KEY_PERTURBATIONNUMBER            810  // GRIB2 perturbationNumber
# define  CDI_KEY_CENTRE                        811  // GRIB2 centre
# define  CDI_KEY_SUBCENTRE                     812  // GRIB2 subCentre
# define  CDI_KEY_MPIMTYPE                      813  // GRIB2 mpimType
# define  CDI_KEY_MPIMCLASS                     814  // GRIB2 mpimClass
# define  CDI_KEY_MPIMUSER                      815  // GRIB2 mpimUser
# define  CDI_KEY_REVSTATUS                     816  // GRIB2 revStatus
# define  CDI_KEY_REVNUMBER                     817  // GRIB2 revNumber
# define  CDI_KEY_GRIB2LOCALSECTIONNUMBER       818  // GRIB2 grib2LocalSectionNumber
# define  CDI_KEY_SECTION2PADDINGLENGTH         819  // GRIB2 length of section2Padding
# define  CDI_KEY_SECTION2PADDING               820  // GRIB2 section2Padding

    //      cdiDefKeyInt: Define an integer value from a key of a CDI variable
    int     cdiDefKeyInt(int cdiID, int varID, int key, int value);

    //      cdiInqKeyInt: Get an integer value from a key of a CDI variable
    int     cdiInqKeyInt(int cdiID, int varID, int key, int *value);

    //      cdiDefKeyBytes: Define a bytes array from a key of a CDI variable
    int     cdiDefKeyBytes(int cdiID, int varID, int key, const unsigned char bytes[], int length);

    //      cdiInqKeyBytes: Get a bytes array from a key of a CDI variable
    int     cdiInqKeyBytes(int cdiID, int varID, int key, unsigned char bytes[], int *length);

    //      cdiDefKeyString: Define a string value from a key of a CDI variable
    int     cdiDefKeyString(int cdiID, int varID, int key, const char *string);

    //      cdiInqKeyString: Get a string value from a key of a CDI variable
    int     cdiInqKeyString(int cdiID, int varID, int key, char *string, int *length);

    //      cdiInqKeyLen: Get the length of the string representation of the key
    int     cdiInqKeyLen(int cdiID, int varID, int key, int *length);

    int     cdiCopyKeys(int cdiID1, int varID1, int cdiID2, int varID2);

// CDI grid keys
# define  CDI_KEY_XNAME       901                       // X-axis name
# define  CDI_KEY_XDIMNAME    902  // X-axis dimension name
# define  CDI_KEY_XLONGNAME   903  // X-axis longname
# define  CDI_KEY_XUNITS      904  // X-axis units
# define  CDI_KEY_YNAME       911  // Y-axis name
# define  CDI_KEY_YDIMNAME    912  // Y-axis dimension name
# define  CDI_KEY_YLONGNAME   913  // Y-axis longname
# define  CDI_KEY_YUNITS      914  // Y-axis units
# define  CDI_KEY_VDIMNAME    920  // Vertex dimension name
# define  CDI_KEY_MAPPING     921  // Grid mapping var name
# define  CDI_KEY_MAPNAME     922  // Grid mapping name

// CDI zaxis keys
# define  CDI_KEY_NAME        941                       // Z-axis name
# define  CDI_KEY_DIMNAME     942  // Z-axis dimension name
# define  CDI_KEY_LONGNAME    943  // Z-axis longname
# define  CDI_KEY_UNITS       944  // Z-axis units
# define  CDI_KEY_PSNAME      950  // Z-axis surface pressure name
# define  CDI_KEY_P0NAME      951  // Z-axis reference pressure name
# define  CDI_KEY_P0VALUE     952  // Z-axis reference pressure in Pa

    //      cdiGridDefKeyStr: Define a CDI grid string value from a key
    int     cdiGridDefKeyStr(int gridID, int key, int size, const char *mesg);

    //      cdiGridInqKeyStr: Get a CDI grid string value from a key
    int     cdiGridInqKeyStr(int gridID, int key, int size, char *mesg);

    //      cdiZaxisDefKeyStr: Define a CDI Z-axis string value from a key
    int     cdiZaxisDefKeyStr(int zaxisID, int key, int size, const char *mesg);

    //      cdiZaxisInqKeyStr: Get a CDI Z-axis string value from a key
    int     cdiZaxisInqKeyStr(int zaxisID, int key, int size, char *mesg);

    //      cdiZaxisDefKeyFlt: Define a CDI Z-axis floating point value from a key
    int     cdiZaxisDefKeyFlt(int zaxisID, int key, double value);

    //      cdiZaxisInqKeyFlt: Get a CDI Z-axis floating point value from a key
    int     cdiZaxisInqKeyFlt(int zaxisID, int key, double *value);

    /*      gridDefXname: Define the name of a X-axis */
    void    gridDefXname(int gridID, const char *xname);

    /*      gridInqXname: Get the name of a X-axis */
    void    gridInqXname(int gridID, char *xname);

    /*      gridDefXlongname: Define the longname of a X-axis  */
    void    gridDefXlongname(int gridID, const char *xlongname);

    /*      gridInqXlongname: Get the longname of a X-axis */
    void    gridInqXlongname(int gridID, char *xlongname);

    /*      gridDefXunits: Define the units of a X-axis */
    void    gridDefXunits(int gridID, const char *xunits);

    /*      gridInqXunits: Get the units of a X-axis */
    void    gridInqXunits(int gridID, char *xunits);

    /*      gridDefYname: Define the name of a Y-axis */
    void    gridDefYname(int gridID, const char *yname);

    /*      gridInqYname: Get the name of a Y-axis */
    void    gridInqYname(int gridID, char *yname);

    /*      gridDefYlongname: Define the longname of a Y-axis */
    void    gridDefYlongname(int gridID, const char *ylongname);

    /*      gridInqYlongname: Get the longname of a Y-axis */
    void    gridInqYlongname(int gridID, char *ylongname);

    /*      gridDefYunits: Define the units of a Y-axis */
    void    gridDefYunits(int gridID, const char *yunits);

    /*      gridInqYunits: Get the units of a Y-axis */
    void    gridInqYunits(int gridID, char *yunits);

    /*      gridInqXstdname: Get the standard name of a X-axis */
    void    gridInqXstdname(int gridID, char *xstdname);

    /*      gridInqYstdname: Get the standard name of a Y-axis */
    void    gridInqYstdname(int gridID, char *ystdname);

    /*      gridDefDatatype: Define the data type of a Grid */
    void    gridDefDatatype(int gridID, int prec);

    /*      gridInqDatatype: Get the data type of a Grid */
    int     gridInqDatatype(int gridID);

    /*      gridInqXval: Get one value of a X-axis */
    double  gridInqXval(int gridID, size_t index);

    /*      gridInqYval: Get one value of a Y-axis */
    double  gridInqYval(int gridID, size_t index);

    double  gridInqXinc(int gridID);
    double  gridInqYinc(int gridID);

    int     gridIsCircular(int gridID);

    int     gridInqTrunc(int gridID);
    void    gridDefTrunc(int gridID, int trunc);

/* Reference of an unstructured grid */

    /*      gridDefNumber: Define the reference number for an unstructured grid */
    void    gridDefNumber(int gridID, int number);

    /*      gridInqNumber: Get the reference number to an unstructured grid */
    int     gridInqNumber(int gridID);

    /*      gridDefPosition: Define the position of grid in the reference file */
    void    gridDefPosition(int gridID, int position);

    /*      gridInqPosition: Get the position of grid in the reference file */
    int     gridInqPosition(int gridID);

    /*      gridDefReference: Define the reference URI for an unstructured grid */
    void    gridDefReference(int gridID, const char *reference);

    /*      gridInqReference: Get the reference URI to an unstructured grid */
    int     gridInqReference(int gridID, char *reference);

    /*      gridDefUUID: Define the UUID of an unstructured grid */
    void    gridDefUUID(int gridID, const unsigned char uuid[CDI_UUID_SIZE]);

    /*      gridInqUUID: Get the UUID of an unstructured grid */
    void    gridInqUUID(int gridID, unsigned char uuid[CDI_UUID_SIZE]);

    /* Rotated Lon/Lat grid */
    void    gridDefParamRLL(int gridID, double xpole, double ypole, double angle);
    void    gridInqParamRLL(int gridID, double *xpole, double *ypole, double *angle);

    /* Hexagonal GME grid */
    void    gridDefParamGME(int gridID, int nd, int ni, int ni2, int ni3);
    void    gridInqParamGME(int gridID, int *nd, int *ni, int *ni2, int *ni3);

    /* Lambert Conformal Conic grid */
    void gridDefParamLCC(int gridID, double missval, double lon_0, double lat_0, double lat_1, double lat_2, double a, double rf, double xval_0, double yval_0, double x_0, double y_0);
    int gridInqParamLCC(int gridID, double missval, double *lon_0, double *lat_0, double *lat_1, double *lat_2, double *a, double *rf, double *xval_0, double *yval_0, double *x_0, double *y_0);

    /* Polar stereographic grid */
    void gridDefParamSTERE(int gridID, double missval, double lon_0, double lat_ts, double lat_0, double a, double xval_0, double yval_0, double x_0, double y_0);
    int gridInqParamSTERE(int gridID, double missval, double *lon_0, double *lat_ts, double *lat_0, double *a, double *xval_0, double *yval_0, double *x_0, double *y_0);

    void    gridDefArea(int gridID, const double area[]);
    void    gridInqArea(int gridID, double area[]);
    int     gridHasArea(int gridID);

    /*      gridDefNvertex: Define the number of vertex of a Gridbox */
    void    gridDefNvertex(int gridID, int nvertex);

    /*      gridInqNvertex: Get the number of vertex of a Gridbox */
    int     gridInqNvertex(int gridID);

    /*      gridDefXbounds: Define the bounds of a X-axis */
    void    gridDefXbounds(int gridID, const double xbounds[]);

    /*      gridInqXbounds: Get the bounds of a X-axis */
    size_t  gridInqXbounds(int gridID, double xbounds[]);
    size_t  gridInqXboundsPart(int gridID, int start, size_t size, double xbounds[]);

    /*      gridDefYbounds: Define the bounds of a Y-axis */
    void    gridDefYbounds(int gridID, const double ybounds[]);

    /*      gridInqYbounds: Get the bounds of a Y-axis */
    size_t  gridInqYbounds(int gridID, double ybounds[]);
    size_t  gridInqYboundsPart(int gridID, int start, size_t size, double ybounds[]);

    void    gridDefRowlon(int gridID, int nrowlon, const int rowlon[]);
    void    gridInqRowlon(int gridID, int rowlon[]);
    void    gridChangeType(int gridID, int gridtype);

    void    gridDefComplexPacking(int gridID, int lpack);
    int     gridInqComplexPacking(int gridID);

    void    gridDefUvRelativeToGrid(int gridID, int uvRelativeToGrid);
    int     gridInqUvRelativeToGrid(int gridID);

    void    gridDefScanningMode(int gridID, int mode);
    int     gridInqScanningMode(int gridID);

/* ZAXIS routines */

    void    zaxisName(int zaxistype, char *zaxisname);
    const char *zaxisNamePtr(int leveltype);

    /*      zaxisCreate: Create a vertical Z-axis */
    int     zaxisCreate(int zaxistype, int size);

    /*      zaxisDestroy: Destroy a vertical Z-axis */
    void    zaxisDestroy(int zaxisID);

    /*      zaxisInqType: Get the type of a Z-axis */
    int     zaxisInqType(int zaxisID);

    /*      zaxisInqSize: Get the size of a Z-axis */
    int     zaxisInqSize(int zaxisID);

    /*      zaxisDuplicate: Duplicate a Z-axis */
    int     zaxisDuplicate(int zaxisID);

    void    zaxisPrint(int zaxisID);

    /*      zaxisDefLevels: Define the levels of a Z-axis */
    void    zaxisDefLevels(int zaxisID, const double levels[]);

    /*      zaxisDefCvals: Define area types of a Z-axis */
    void    zaxisDefCvals(int zaxisID, const char *cvals[], int clength);

    /*      zaxisInqLevels: Get all levels of a Z-axis */
    int     zaxisInqLevels(int zaxisID, double levels[]);

    /*      zaxisInqCLen: Get maximal string length of character Z-axis */
    int     zaxisInqCLen(int zaxisID);

    /*      zaxisInqCVals: Get all string values of a character Z-axis */
    int     zaxisInqCVals(int zaxisID, char ***clevels);

    /*      zaxisDefLevel: Define one level of a Z-axis */
    void    zaxisDefLevel(int zaxisID, int levelID, double levels);

    /*      zaxisInqLevel: Get one level of a Z-axis */
    double  zaxisInqLevel(int zaxisID, int levelID);

    /*      zaxisDefNlevRef: Define the number of half levels of a generalized Z-axis */
    void    zaxisDefNlevRef(int gridID, int nhlev);

    /*      zaxisInqNlevRef: Get the number of half levels of a generalized Z-axis */
    int     zaxisInqNlevRef(int gridID);

    /*      zaxisDefNumber: Define the reference number for a generalized Z-axis */
    void    zaxisDefNumber(int gridID, int number);

    /*      zaxisInqNumber: Get the reference number to a generalized Z-axis */
    int     zaxisInqNumber(int gridID);

    /*      zaxisDefUUID: Define the UUID of a generalized Z-axis */
    void    zaxisDefUUID(int zaxisID, const unsigned char uuid[CDI_UUID_SIZE]);

    /*      zaxisInqUUID: Get the UUID of a generalized Z-axis */
    void    zaxisInqUUID(int zaxisID, unsigned char uuid[CDI_UUID_SIZE]);

    /*      zaxisDefName: Define the name of a Z-axis */
    void    zaxisDefName(int zaxisID, const char *name_optional);

    /*      zaxisInqName: Get the name of a Z-axis */
    void    zaxisInqName(int zaxisID, char *name);

    /*      zaxisDefLongname: Define the longname of a Z-axis */
    void    zaxisDefLongname(int zaxisID, const char *longname_optional);

    /*      zaxisInqLongname: Get the longname of a Z-axis */
    void    zaxisInqLongname(int zaxisID, char *longname);

    /*      zaxisDefUnits: Define the units of a Z-axis */
    void    zaxisDefUnits(int zaxisID, const char *units_optional);

    /*      zaxisInqUnits: Get the units of a Z-axis */
    void    zaxisInqUnits(int zaxisID, char *units);

    /*      zaxisInqStdname: Get the standard name of a Z-axis */
    void    zaxisInqStdname(int zaxisID, char *stdname);

    void    zaxisDefDatatype(int zaxisID, int prec);
    int     zaxisInqDatatype(int zaxisID);

    void    zaxisDefPositive(int zaxisID, int positive);
    int     zaxisInqPositive(int zaxisID);

    void    zaxisDefScalar(int zaxisID);
    int     zaxisInqScalar(int zaxisID);

    void    zaxisDefLtype(int zaxisID, int ltype);
    int     zaxisInqLtype(int zaxisID);

    void    zaxisDefVct(int zaxisID, int size, const double vct[]);
    void    zaxisInqVct(int zaxisID, double vct[]);
    int     zaxisInqVctSize(int zaxisID);
    const double *zaxisInqVctPtr(int zaxisID);
    void    zaxisDefLbounds(int zaxisID, const double lbounds[]);
    int     zaxisInqLbounds(int zaxisID, double lbounds_optional[]);
    double  zaxisInqLbound(int zaxisID, int index);
    void    zaxisDefUbounds(int zaxisID, const double ubounds[]);
    int     zaxisInqUbounds(int zaxisID, double ubounds_optional[]);
    double  zaxisInqUbound(int zaxisID, int index);
    void    zaxisDefWeights(int zaxisID, const double weights[]);
    int     zaxisInqWeights(int zaxisID, double weights_optional[]);
    void    zaxisChangeType(int zaxisID, int zaxistype);

/* TAXIS routines */

    /*      taxisCreate: Create a Time axis */
    int     taxisCreate(int taxistype);

    /*      taxisDestroy: Destroy a Time axis */
    void    taxisDestroy(int taxisID);

    int     taxisDuplicate(int taxisID);

    void    taxisCopyTimestep(int taxisIDdes, int taxisIDsrc);

    void    taxisDefType(int taxisID, int taxistype);
    int     taxisInqType(int taxisID);

    /*      taxisDefVdate: Define the verification date */
    void    taxisDefVdate(int taxisID, int64_t date);

    /*      taxisDefVtime: Define the verification time */
    void    taxisDefVtime(int taxisID, int time);

    /*      taxisInqVdate: Get the verification date */
    int64_t taxisInqVdate(int taxisID);

    /*      taxisInqVtime: Get the verification time */
    int     taxisInqVtime(int taxisID);

    /*      taxisDefRdate: Define the reference date */
    void    taxisDefRdate(int taxisID, int64_t date);

    /*      taxisDefRtime: Define the reference time */
    void    taxisDefRtime(int taxisID, int time);

    /*      taxisInqRdate: Get the reference date */
    int64_t taxisInqRdate(int taxisID);

    /*      taxisInqRtime: Get the reference time */
    int     taxisInqRtime(int taxisID);

    /*      taxisDefFdate: Define the forecast reference date */
    void    taxisDefFdate(int taxisID, int64_t date);

    /*      taxisDefFtime: Define the forecast reference time */
    void    taxisDefFtime(int taxisID, int time);

    /*      taxisInqFdate: Get the forecast reference date */
    int64_t taxisInqFdate(int taxisID);

    /*      taxisInqFtime: Get the forecast reference time */
    int     taxisInqFtime(int taxisID);

    int     taxisHasBounds(int taxisID);
    void    taxisWithBounds(int taxisID);

    void    taxisDeleteBounds(int taxisID);

    void    taxisDefVdateBounds(int taxisID, int64_t vdate_lb, int64_t vdate_ub);

    void    taxisDefVtimeBounds(int taxisID, int vtime_lb, int vtime_ub);

    void    taxisInqVdateBounds(int taxisID, int64_t *vdate_lb, int64_t *vdate_ub);

    void    taxisInqVtimeBounds(int taxisID, int *vtime_lb, int *vtime_ub);

    /*      taxisDefCalendar: Define the calendar */
    void    taxisDefCalendar(int taxisID, int calendar);

    /*      taxisInqCalendar: Get the calendar */
    int     taxisInqCalendar(int taxisID);

    void    taxisDefTunit(int taxisID, int tunit);
    int     taxisInqTunit(int taxisID);

    void    taxisDefForecastTunit(int taxisID, int tunit);
    int     taxisInqForecastTunit(int taxisID);

    void    taxisDefForecastPeriod(int taxisID, double fc_period);
    double  taxisInqForecastPeriod(int taxisID);

    void    taxisDefNumavg(int taxisID, int numavg);
    int     taxisInqNumavg(int taxisID);

    const char *tunitNamePtr(int tunitID);


/* Institut routines */

    int     institutDef(int center, int subcenter, const char *name, const char *longname);
    int     institutInq(int center, int subcenter, const char *name, const char *longname);
    int     institutInqNumber(void);
    int     institutInqCenter(int instID);
    int     institutInqSubcenter(int instID);
    const char *institutInqNamePtr(int instID);
    const char *institutInqLongnamePtr(int instID);

/* Model routines */

    int     modelDef(int instID, int modelgribID, const char *name);
    int     modelInq(int instID, int modelgribID, const char *name);
    int     modelInqInstitut(int modelID) ;
    int     modelInqGribID(int modelID);
    const char *modelInqNamePtr(int modelID);

/* Table routines */

    /*      tableFWriteC: write table of parameters to FILE* in C language format */
    void    tableFWriteC(FILE *ptfp, int tableID);
    /*      tableWrite: write table of parameters to file in tabular format */
    void    tableWrite(const char *filename, int tableID);
    /*      tableRead: read table of parameters from file in tabular format */
    int     tableRead(const char *tablefile);
    int     tableDef(int modelID, int tablenum, const char *tablename);

    const char *tableInqNamePtr(int tableID);

    int     tableInq(int modelID, int tablenum, const char *tablename);
    int     tableInqNumber(void);

    int     tableInqNum(int tableID);
    int     tableInqModel(int tableID);

    void    tableInqEntry(int tableID, int id, int ltype, char *name, char *longname, char *units);

/* History routines */

    void    streamDefHistory(int streamID, int size, const char *history);
    int     streamInqHistorySize(int streamID);
    void    streamInqHistoryString(int streamID, char *history);

/* Subtype routines */

    /*      subtypeCreate: Create a variable subtype */
    int     subtypeCreate(int subtype);

    /*      Gives a textual summary of the variable subtype */
    void    subtypePrint(int subtypeID);

    /* Compares two subtype data structures. */
    int     subtypeCompare(int subtypeID1, int subtypeID2);

    /*      subtypeInqSize: Get the size of a subtype (e.g. no. of tiles). */
    int     subtypeInqSize(int subtypeID);

    /*      subtypeInqActiveIndex: Get the currently active index of a subtype (e.g. current tile index). */
    int     subtypeInqActiveIndex(int subtypeID);

    /*      subtypeDefActiveIndex: Set the currently active index of a subtype (e.g. current tile index). */
    void    subtypeDefActiveIndex(int subtypeID, int index);

    /*      Generate a "query object" out of a key-value pair. */
    subtype_query_t keyValuePair(const char *key, int value);

    /*       Generate an AND-combined "query object" out of two previous
     *        query objects. */
    subtype_query_t matchAND(subtype_query_t q1, subtype_query_t q2);

    /*      subtypeInqSubEntry: Returns subtype entry ID for a given criterion. */
    int     subtypeInqSubEntry(int subtypeID, subtype_query_t criterion);

    /*      subtypeInqTile: Specialized version of subtypeInqSubEntry looking for tile/attribute pair. */
    int     subtypeInqTile(int subtypeID, int tileindex, int attribute);

    /*      subtypeInqAttribute: Inquire the value of a subtype attribute. Returns CDI_EINVAL if the attribute does not exist.*/
    int     subtypeInqAttribute(int subtypeID, int index, const char *key, int *outValue);

    /*      vlistInqVarSubtype: Return subtype ID for a given variable. */
    int     vlistInqVarSubtype(int vlistID, int varID);
    
    void gribapiLibraryVersion(int *major_version, int *minor_version, int *revision_version);
    
    
    #if defined (__cplusplus)
}
#endif

#endif  /* CDI_H_ */
/*
 * Local Variables:
 * c-file-style: "Java"
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * show-trailing-whitespace: t
 * require-trailing-newline: t
 * End:
 */
