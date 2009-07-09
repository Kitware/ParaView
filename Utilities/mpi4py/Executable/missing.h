#ifndef PyMPI_MISSING_H
#define PyMPI_MISSING_H

#ifndef PyMPI_UNUSED
# if defined(__GNUC__)
#   if !defined(__cplusplus) || (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
#     define PyMPI_UNUSED __attribute__ ((__unused__))
#   else
#     define PyMPI_UNUSED
#   endif
# elif defined(__INTEL_COMPILER) || defined(__ICC)
#   define PyMPI_UNUSED __attribute__ ((__unused__))
# else
#   define PyMPI_UNUSED
# endif
#endif

static PyMPI_UNUSED int PyMPI_UNAVAILABLE(const char *name,...)
{
  /* XXX do someting with name */
  return -1;
}

#ifdef PyMPI_MISSING_MPI_AINT
#if !defined(MPI_Aint)
typedef long PyMPI_MPI_Aint;
#define MPI_Aint PyMPI_MPI_Aint
#endif
#endif

#ifdef PyMPI_MISSING_MPI_OFFSET
#if !defined(MPI_Offset)
typedef long PyMPI_MPI_Offset;
#define MPI_Offset PyMPI_MPI_Offset
#endif
#endif

#ifdef PyMPI_MISSING_MPI_STATUS
#if !defined(MPI_Status)
typedef struct PyMPI_MPI_Status {
  int MPI_SOURCE;
  int MPI_TAG;
  int MPI_ERROR;
} PyMPI_MPI_Status;
#define MPI_Status PyMPI_MPI_Status
#endif
#endif

#ifdef PyMPI_MISSING_MPI_DATATYPE
#if !defined(MPI_Datatype)
typedef void *PyMPI_MPI_Datatype;
#define MPI_Datatype PyMPI_MPI_Datatype
#endif
#endif

#ifdef PyMPI_MISSING_MPI_REQUEST
#if !defined(MPI_Request)
typedef void *PyMPI_MPI_Request;
#define MPI_Request PyMPI_MPI_Request
#endif
#endif

#ifdef PyMPI_MISSING_MPI_OP
#if !defined(MPI_Op)
typedef void *PyMPI_MPI_Op;
#define MPI_Op PyMPI_MPI_Op
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GROUP
#if !defined(MPI_Group)
typedef void *PyMPI_MPI_Group;
#define MPI_Group PyMPI_MPI_Group
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INFO
#if !defined(MPI_Info)
typedef void *PyMPI_MPI_Info;
#define MPI_Info PyMPI_MPI_Info
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM
#if !defined(MPI_Comm)
typedef void *PyMPI_MPI_Comm;
#define MPI_Comm PyMPI_MPI_Comm
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN
#if !defined(MPI_Win)
typedef void *PyMPI_MPI_Win;
#define MPI_Win PyMPI_MPI_Win
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE
#if !defined(MPI_File)
typedef void *PyMPI_MPI_File;
#define MPI_File PyMPI_MPI_File
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERRHANDLER
#if !defined(MPI_Errhandler)
typedef void *PyMPI_MPI_Errhandler;
#define MPI_Errhandler PyMPI_MPI_Errhandler
#endif
#endif

#ifdef PyMPI_MISSING_MPI_UNDEFINED
#if !defined(MPI_UNDEFINED)
#define MPI_UNDEFINED (-32766)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ANY_SOURCE
#if !defined(MPI_ANY_SOURCE)
#define MPI_ANY_SOURCE (MPI_UNDEFINED)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ANY_TAG
#if !defined(MPI_ANY_TAG)
#define MPI_ANY_TAG (MPI_UNDEFINED)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_PROC_NULL
#if !defined(MPI_PROC_NULL)
#define MPI_PROC_NULL (MPI_UNDEFINED)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ROOT
#if !defined(MPI_ROOT)
#define MPI_ROOT (MPI_PROC_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_IDENT
#if !defined(MPI_IDENT)
#define MPI_IDENT (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_CONGRUENT
#if !defined(MPI_CONGRUENT)
#define MPI_CONGRUENT (2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SIMILAR
#if !defined(MPI_SIMILAR)
#define MPI_SIMILAR (3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_UNEQUAL
#if !defined(MPI_UNEQUAL)
#define MPI_UNEQUAL (4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_BOTTOM
#if !defined(MPI_BOTTOM)
#define MPI_BOTTOM ((void*)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_IN_PLACE
#if !defined(MPI_IN_PLACE)
#define MPI_IN_PLACE ((void*)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_KEYVAL_INVALID
#if !defined(MPI_KEYVAL_INVALID)
#define MPI_KEYVAL_INVALID (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MAX_OBJECT_NAME
#if !defined(MPI_MAX_OBJECT_NAME)
#define MPI_MAX_OBJECT_NAME (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_DATATYPE_NULL
#if !defined(MPI_DATATYPE_NULL)
#define MPI_DATATYPE_NULL ((MPI_Datatype)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_CHAR
#if !defined(MPI_CHAR)
#define MPI_CHAR ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WCHAR
#if !defined(MPI_WCHAR)
#define MPI_WCHAR ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SIGNED_CHAR
#if !defined(MPI_SIGNED_CHAR)
#define MPI_SIGNED_CHAR ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_UNSIGNED_CHAR
#if !defined(MPI_UNSIGNED_CHAR)
#define MPI_UNSIGNED_CHAR ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SHORT
#if !defined(MPI_SHORT)
#define MPI_SHORT ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_UNSIGNED_SHORT
#if !defined(MPI_UNSIGNED_SHORT)
#define MPI_UNSIGNED_SHORT ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INT
#if !defined(MPI_INT)
#define MPI_INT ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_UNSIGNED
#if !defined(MPI_UNSIGNED)
#define MPI_UNSIGNED ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_LONG
#if !defined(MPI_LONG)
#define MPI_LONG ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_UNSIGNED_LONG
#if !defined(MPI_UNSIGNED_LONG)
#define MPI_UNSIGNED_LONG ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FLOAT
#if !defined(MPI_FLOAT)
#define MPI_FLOAT ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_DOUBLE
#if !defined(MPI_DOUBLE)
#define MPI_DOUBLE ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_LONG_DOUBLE
#if !defined(MPI_LONG_DOUBLE)
#define MPI_LONG_DOUBLE ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_BYTE
#if !defined(MPI_BYTE)
#define MPI_BYTE ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_PACKED
#if !defined(MPI_PACKED)
#define MPI_PACKED ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SHORT_INT
#if !defined(MPI_SHORT_INT)
#define MPI_SHORT_INT ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_2INT
#if !defined(MPI_2INT)
#define MPI_2INT ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_LONG_INT
#if !defined(MPI_LONG_INT)
#define MPI_LONG_INT ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FLOAT_INT
#if !defined(MPI_FLOAT_INT)
#define MPI_FLOAT_INT ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_DOUBLE_INT
#if !defined(MPI_DOUBLE_INT)
#define MPI_DOUBLE_INT ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_LONG_DOUBLE_INT
#if !defined(MPI_LONG_DOUBLE_INT)
#define MPI_LONG_DOUBLE_INT ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_LONG_LONG
#if !defined(MPI_LONG_LONG)
#define MPI_LONG_LONG ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_UNSIGNED_LONG_LONG
#if !defined(MPI_UNSIGNED_LONG_LONG)
#define MPI_UNSIGNED_LONG_LONG ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_LONG_LONG_INT
#if !defined(MPI_LONG_LONG_INT)
#define MPI_LONG_LONG_INT ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_CHARACTER
#if !defined(MPI_CHARACTER)
#define MPI_CHARACTER ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_LOGICAL
#if !defined(MPI_LOGICAL)
#define MPI_LOGICAL ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INTEGER
#if !defined(MPI_INTEGER)
#define MPI_INTEGER ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_REAL
#if !defined(MPI_REAL)
#define MPI_REAL ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_DOUBLE_PRECISION
#if !defined(MPI_DOUBLE_PRECISION)
#define MPI_DOUBLE_PRECISION ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMPLEX
#if !defined(MPI_COMPLEX)
#define MPI_COMPLEX ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_DOUBLE_COMPLEX
#if !defined(MPI_DOUBLE_COMPLEX)
#define MPI_DOUBLE_COMPLEX ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_UB
#if !defined(MPI_UB)
#define MPI_UB ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_LB
#if !defined(MPI_LB)
#define MPI_LB ((MPI_Datatype)MPI_DATATYPE_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_SIZE
#if !defined(MPI_Type_size)
#define MPI_Type_size(a1,a2) PyMPI_UNAVAILABLE("MPI_TYPE_SIZE",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_EXTENT
#if !defined(MPI_Type_extent)
#define MPI_Type_extent(a1,a2) PyMPI_UNAVAILABLE("MPI_TYPE_EXTENT",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_LB
#if !defined(MPI_Type_lb)
#define MPI_Type_lb(a1,a2) PyMPI_UNAVAILABLE("MPI_TYPE_LB",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_UB
#if !defined(MPI_Type_ub)
#define MPI_Type_ub(a1,a2) PyMPI_UNAVAILABLE("MPI_TYPE_UB",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_DUP
#if !defined(MPI_Type_dup)
#define MPI_Type_dup(a1,a2) PyMPI_UNAVAILABLE("MPI_TYPE_DUP",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_CONTIGUOUS
#if !defined(MPI_Type_contiguous)
#define MPI_Type_contiguous(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_TYPE_CONTIGUOUS",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_VECTOR
#if !defined(MPI_Type_vector)
#define MPI_Type_vector(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_TYPE_VECTOR",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_INDEXED
#if !defined(MPI_Type_indexed)
#define MPI_Type_indexed(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_TYPE_INDEXED",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_CREATE_INDEXED_BLOCK
#if !defined(MPI_Type_create_indexed_block)
#define MPI_Type_create_indexed_block(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_TYPE_CREATE_INDEXED_BLOCK",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ORDER_C
#if !defined(MPI_ORDER_C)
#define MPI_ORDER_C (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ORDER_FORTRAN
#if !defined(MPI_ORDER_FORTRAN)
#define MPI_ORDER_FORTRAN (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_CREATE_SUBARRAY
#if !defined(MPI_Type_create_subarray)
#define MPI_Type_create_subarray(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_TYPE_CREATE_SUBARRAY",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_DISTRIBUTE_NONE
#if !defined(MPI_DISTRIBUTE_NONE)
#define MPI_DISTRIBUTE_NONE (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_DISTRIBUTE_BLOCK
#if !defined(MPI_DISTRIBUTE_BLOCK)
#define MPI_DISTRIBUTE_BLOCK (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_DISTRIBUTE_CYCLIC
#if !defined(MPI_DISTRIBUTE_CYCLIC)
#define MPI_DISTRIBUTE_CYCLIC (2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_DISTRIBUTE_DFLT_DARG
#if !defined(MPI_DISTRIBUTE_DFLT_DARG)
#define MPI_DISTRIBUTE_DFLT_DARG (3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_CREATE_DARRAY
#if !defined(MPI_Type_create_darray)
#define MPI_Type_create_darray(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) PyMPI_UNAVAILABLE("MPI_TYPE_CREATE_DARRAY",a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ADDRESS
#if !defined(MPI_Address)
#define MPI_Address(a1,a2) PyMPI_UNAVAILABLE("MPI_ADDRESS",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_HVECTOR
#if !defined(MPI_Type_hvector)
#define MPI_Type_hvector(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_TYPE_HVECTOR",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_HINDEXED
#if !defined(MPI_Type_hindexed)
#define MPI_Type_hindexed(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_TYPE_HINDEXED",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_STRUCT
#if !defined(MPI_Type_struct)
#define MPI_Type_struct(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_TYPE_STRUCT",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GET_ADDRESS
#if !defined(MPI_Get_address)
#define MPI_Get_address MPI_Address
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_CREATE_HVECTOR
#if !defined(MPI_Type_create_hvector)
#define MPI_Type_create_hvector MPI_Type_hvector
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_CREATE_HINDEXED
#if !defined(MPI_Type_create_hindexed)
#define MPI_Type_create_hindexed MPI_Type_hindexed
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_CREATE_STRUCT
#if !defined(MPI_Type_create_struct)
#define MPI_Type_create_struct MPI_Type_struct
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_GET_EXTENT
#if !defined(MPI_Type_get_extent)
#define MPI_Type_get_extent(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_TYPE_GET_EXTENT",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_CREATE_RESIZED
#if !defined(MPI_Type_create_resized)
#define MPI_Type_create_resized(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_TYPE_CREATE_RESIZED",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_GET_TRUE_EXTENT
#if !defined(MPI_Type_get_true_extent)
#define MPI_Type_get_true_extent(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_TYPE_GET_TRUE_EXTENT",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPECLASS_INTEGER
#if !defined(MPI_TYPECLASS_INTEGER)
#define MPI_TYPECLASS_INTEGER (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPECLASS_REAL
#if !defined(MPI_TYPECLASS_REAL)
#define MPI_TYPECLASS_REAL (2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPECLASS_COMPLEX
#if !defined(MPI_TYPECLASS_COMPLEX)
#define MPI_TYPECLASS_COMPLEX (3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_MATCH_SIZE
#if !defined(MPI_Type_match_size)
#define MPI_Type_match_size(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_TYPE_MATCH_SIZE",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_COMMIT
#if !defined(MPI_Type_commit)
#define MPI_Type_commit(a1) PyMPI_UNAVAILABLE("MPI_TYPE_COMMIT",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_FREE
#if !defined(MPI_Type_free)
#define MPI_Type_free(a1) PyMPI_UNAVAILABLE("MPI_TYPE_FREE",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_PACK
#if !defined(MPI_Pack)
#define MPI_Pack(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_PACK",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_UNPACK
#if !defined(MPI_Unpack)
#define MPI_Unpack(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_UNPACK",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_PACK_SIZE
#if !defined(MPI_Pack_size)
#define MPI_Pack_size(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_PACK_SIZE",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_PACK_EXTERNAL
#if !defined(MPI_Pack_external)
#define MPI_Pack_external(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_PACK_EXTERNAL",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_UNPACK_EXTERNAL
#if !defined(MPI_Unpack_external)
#define MPI_Unpack_external(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_UNPACK_EXTERNAL",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_PACK_EXTERNAL_SIZE
#if !defined(MPI_Pack_external_size)
#define MPI_Pack_external_size(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_PACK_EXTERNAL_SIZE",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_NAMED
#if !defined(MPI_COMBINER_NAMED)
#define MPI_COMBINER_NAMED (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_DUP
#if !defined(MPI_COMBINER_DUP)
#define MPI_COMBINER_DUP (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_CONTIGUOUS
#if !defined(MPI_COMBINER_CONTIGUOUS)
#define MPI_COMBINER_CONTIGUOUS (2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_VECTOR
#if !defined(MPI_COMBINER_VECTOR)
#define MPI_COMBINER_VECTOR (3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_HVECTOR_INTEGER
#if !defined(MPI_COMBINER_HVECTOR_INTEGER)
#define MPI_COMBINER_HVECTOR_INTEGER (4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_HVECTOR
#if !defined(MPI_COMBINER_HVECTOR)
#define MPI_COMBINER_HVECTOR (5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_INDEXED
#if !defined(MPI_COMBINER_INDEXED)
#define MPI_COMBINER_INDEXED (6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_HINDEXED_INTEGER
#if !defined(MPI_COMBINER_HINDEXED_INTEGER)
#define MPI_COMBINER_HINDEXED_INTEGER (7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_HINDEXED
#if !defined(MPI_COMBINER_HINDEXED)
#define MPI_COMBINER_HINDEXED (8)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_INDEXED_BLOCK
#if !defined(MPI_COMBINER_INDEXED_BLOCK)
#define MPI_COMBINER_INDEXED_BLOCK (9)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_STRUCT_INTEGER
#if !defined(MPI_COMBINER_STRUCT_INTEGER)
#define MPI_COMBINER_STRUCT_INTEGER (10)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_STRUCT
#if !defined(MPI_COMBINER_STRUCT)
#define MPI_COMBINER_STRUCT (11)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_SUBARRAY
#if !defined(MPI_COMBINER_SUBARRAY)
#define MPI_COMBINER_SUBARRAY (12)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_DARRAY
#if !defined(MPI_COMBINER_DARRAY)
#define MPI_COMBINER_DARRAY (13)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_F90_REAL
#if !defined(MPI_COMBINER_F90_REAL)
#define MPI_COMBINER_F90_REAL (14)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_F90_COMPLEX
#if !defined(MPI_COMBINER_F90_COMPLEX)
#define MPI_COMBINER_F90_COMPLEX (15)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_F90_INTEGER
#if !defined(MPI_COMBINER_F90_INTEGER)
#define MPI_COMBINER_F90_INTEGER (16)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMBINER_RESIZED
#if !defined(MPI_COMBINER_RESIZED)
#define MPI_COMBINER_RESIZED (17)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_GET_ENVELOPE
#if !defined(MPI_Type_get_envelope)
#define MPI_Type_get_envelope(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_TYPE_GET_ENVELOPE",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_GET_CONTENTS
#if !defined(MPI_Type_get_contents)
#define MPI_Type_get_contents(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_TYPE_GET_CONTENTS",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_GET_NAME
#if !defined(MPI_Type_get_name)
#define MPI_Type_get_name(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_TYPE_GET_NAME",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_SET_NAME
#if !defined(MPI_Type_set_name)
#define MPI_Type_set_name(a1,a2) PyMPI_UNAVAILABLE("MPI_TYPE_SET_NAME",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_GET_ATTR
#if !defined(MPI_Type_get_attr)
#define MPI_Type_get_attr(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_TYPE_GET_ATTR",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_SET_ATTR
#if !defined(MPI_Type_set_attr)
#define MPI_Type_set_attr(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_TYPE_SET_ATTR",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_DELETE_ATTR
#if !defined(MPI_Type_delete_attr)
#define MPI_Type_delete_attr(a1,a2) PyMPI_UNAVAILABLE("MPI_TYPE_DELETE_ATTR",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_COPY_ATTR_FUNCTION
#if !defined(MPI_Type_copy_attr_function)
typedef int (PyMPI_MPI_Type_copy_attr_function)(MPI_Datatype,int,void*,void*,void*,int*);
#define MPI_Type_copy_attr_function PyMPI_MPI_Type_copy_attr_function
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_DELETE_ATTR_FUNCTION
#if !defined(MPI_Type_delete_attr_function)
typedef int (PyMPI_MPI_Type_delete_attr_function)(MPI_Datatype,int,void*,void*);
#define MPI_Type_delete_attr_function PyMPI_MPI_Type_delete_attr_function
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_NULL_COPY_FN
#if !defined(MPI_TYPE_NULL_COPY_FN)
#define MPI_TYPE_NULL_COPY_FN (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_DUP_FN
#if !defined(MPI_TYPE_DUP_FN)
#define MPI_TYPE_DUP_FN (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_NULL_DELETE_FN
#if !defined(MPI_TYPE_NULL_DELETE_FN)
#define MPI_TYPE_NULL_DELETE_FN (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_CREATE_KEYVAL
#if !defined(MPI_Type_create_keyval)
#define MPI_Type_create_keyval(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_TYPE_CREATE_KEYVAL",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_FREE_KEYVAL
#if !defined(MPI_Type_free_keyval)
#define MPI_Type_free_keyval(a1) PyMPI_UNAVAILABLE("MPI_TYPE_FREE_KEYVAL",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_STATUS_IGNORE
#if !defined(MPI_STATUS_IGNORE)
#define MPI_STATUS_IGNORE (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_STATUSES_IGNORE
#if !defined(MPI_STATUSES_IGNORE)
#define MPI_STATUSES_IGNORE (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GET_COUNT
#if !defined(MPI_Get_count)
#define MPI_Get_count(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_GET_COUNT",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GET_ELEMENTS
#if !defined(MPI_Get_elements)
#define MPI_Get_elements(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_GET_ELEMENTS",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_STATUS_SET_ELEMENTS
#if !defined(MPI_Status_set_elements)
#define MPI_Status_set_elements(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_STATUS_SET_ELEMENTS",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TEST_CANCELLED
#if !defined(MPI_Test_cancelled)
#define MPI_Test_cancelled(a1,a2) PyMPI_UNAVAILABLE("MPI_TEST_CANCELLED",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_STATUS_SET_CANCELLED
#if !defined(MPI_Status_set_cancelled)
#define MPI_Status_set_cancelled(a1,a2) PyMPI_UNAVAILABLE("MPI_STATUS_SET_CANCELLED",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_REQUEST_NULL
#if !defined(MPI_REQUEST_NULL)
#define MPI_REQUEST_NULL ((MPI_Request)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_REQUEST_FREE
#if !defined(MPI_Request_free)
#define MPI_Request_free(a1) PyMPI_UNAVAILABLE("MPI_REQUEST_FREE",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WAIT
#if !defined(MPI_Wait)
#define MPI_Wait(a1,a2) PyMPI_UNAVAILABLE("MPI_WAIT",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TEST
#if !defined(MPI_Test)
#define MPI_Test(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_TEST",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_REQUEST_GET_STATUS
#if !defined(MPI_Request_get_status)
#define MPI_Request_get_status(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_REQUEST_GET_STATUS",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_CANCEL
#if !defined(MPI_Cancel)
#define MPI_Cancel(a1) PyMPI_UNAVAILABLE("MPI_CANCEL",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WAITANY
#if !defined(MPI_Waitany)
#define MPI_Waitany(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_WAITANY",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TESTANY
#if !defined(MPI_Testany)
#define MPI_Testany(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_TESTANY",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WAITALL
#if !defined(MPI_Waitall)
#define MPI_Waitall(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_WAITALL",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TESTALL
#if !defined(MPI_Testall)
#define MPI_Testall(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_TESTALL",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WAITSOME
#if !defined(MPI_Waitsome)
#define MPI_Waitsome(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_WAITSOME",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TESTSOME
#if !defined(MPI_Testsome)
#define MPI_Testsome(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_TESTSOME",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_START
#if !defined(MPI_Start)
#define MPI_Start(a1) PyMPI_UNAVAILABLE("MPI_START",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_STARTALL
#if !defined(MPI_Startall)
#define MPI_Startall(a1,a2) PyMPI_UNAVAILABLE("MPI_STARTALL",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GREQUEST_CANCEL_FUNCTION
#if !defined(MPI_Grequest_cancel_function)
typedef int (PyMPI_MPI_Grequest_cancel_function)(void*,int);
#define MPI_Grequest_cancel_function PyMPI_MPI_Grequest_cancel_function
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GREQUEST_FREE_FUNCTION
#if !defined(MPI_Grequest_free_function)
typedef int (PyMPI_MPI_Grequest_free_function)(void*);
#define MPI_Grequest_free_function PyMPI_MPI_Grequest_free_function
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GREQUEST_QUERY_FUNCTION
#if !defined(MPI_Grequest_query_function)
typedef int (PyMPI_MPI_Grequest_query_function)(void*,MPI_Status*);
#define MPI_Grequest_query_function PyMPI_MPI_Grequest_query_function
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GREQUEST_START
#if !defined(MPI_Grequest_start)
#define MPI_Grequest_start(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_GREQUEST_START",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GREQUEST_COMPLETE
#if !defined(MPI_Grequest_complete)
#define MPI_Grequest_complete(a1) PyMPI_UNAVAILABLE("MPI_GREQUEST_COMPLETE",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_OP_NULL
#if !defined(MPI_OP_NULL)
#define MPI_OP_NULL ((MPI_Op)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MAX
#if !defined(MPI_MAX)
#define MPI_MAX ((MPI_Op)MPI_OP_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MIN
#if !defined(MPI_MIN)
#define MPI_MIN ((MPI_Op)MPI_OP_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SUM
#if !defined(MPI_SUM)
#define MPI_SUM ((MPI_Op)MPI_OP_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_PROD
#if !defined(MPI_PROD)
#define MPI_PROD ((MPI_Op)MPI_OP_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_LAND
#if !defined(MPI_LAND)
#define MPI_LAND ((MPI_Op)MPI_OP_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_BAND
#if !defined(MPI_BAND)
#define MPI_BAND ((MPI_Op)MPI_OP_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_LOR
#if !defined(MPI_LOR)
#define MPI_LOR ((MPI_Op)MPI_OP_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_BOR
#if !defined(MPI_BOR)
#define MPI_BOR ((MPI_Op)MPI_OP_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_LXOR
#if !defined(MPI_LXOR)
#define MPI_LXOR ((MPI_Op)MPI_OP_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_BXOR
#if !defined(MPI_BXOR)
#define MPI_BXOR ((MPI_Op)MPI_OP_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MAXLOC
#if !defined(MPI_MAXLOC)
#define MPI_MAXLOC ((MPI_Op)MPI_OP_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MINLOC
#if !defined(MPI_MINLOC)
#define MPI_MINLOC ((MPI_Op)MPI_OP_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_REPLACE
#if !defined(MPI_REPLACE)
#define MPI_REPLACE ((MPI_Op)MPI_OP_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_OP_FREE
#if !defined(MPI_Op_free)
#define MPI_Op_free(a1) PyMPI_UNAVAILABLE("MPI_OP_FREE",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_USER_FUNCTION
#if !defined(MPI_User_function)
typedef void (PyMPI_MPI_User_function)(void*, void*, int*, MPI_Datatype*);
#define MPI_User_function PyMPI_MPI_User_function
#endif
#endif

#ifdef PyMPI_MISSING_MPI_OP_CREATE
#if !defined(MPI_Op_create)
#define MPI_Op_create(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_OP_CREATE",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INFO_NULL
#if !defined(MPI_INFO_NULL)
#define MPI_INFO_NULL ((MPI_Info)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INFO_FREE
#if !defined(MPI_Info_free)
#define MPI_Info_free(a1) PyMPI_UNAVAILABLE("MPI_INFO_FREE",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INFO_CREATE
#if !defined(MPI_Info_create)
#define MPI_Info_create(a1) PyMPI_UNAVAILABLE("MPI_INFO_CREATE",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INFO_DUP
#if !defined(MPI_Info_dup)
#define MPI_Info_dup(a1,a2) PyMPI_UNAVAILABLE("MPI_INFO_DUP",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MAX_INFO_KEY
#if !defined(MPI_MAX_INFO_KEY)
#define MPI_MAX_INFO_KEY (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MAX_INFO_VAL
#if !defined(MPI_MAX_INFO_VAL)
#define MPI_MAX_INFO_VAL (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INFO_GET
#if !defined(MPI_Info_get)
#define MPI_Info_get(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_INFO_GET",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INFO_SET
#if !defined(MPI_Info_set)
#define MPI_Info_set(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_INFO_SET",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INFO_DELETE
#if !defined(MPI_Info_delete)
#define MPI_Info_delete(a1,a2) PyMPI_UNAVAILABLE("MPI_INFO_DELETE",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INFO_GET_NKEYS
#if !defined(MPI_Info_get_nkeys)
#define MPI_Info_get_nkeys(a1,a2) PyMPI_UNAVAILABLE("MPI_INFO_GET_NKEYS",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INFO_GET_NTHKEY
#if !defined(MPI_Info_get_nthkey)
#define MPI_Info_get_nthkey(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_INFO_GET_NTHKEY",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INFO_GET_VALUELEN
#if !defined(MPI_Info_get_valuelen)
#define MPI_Info_get_valuelen(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_INFO_GET_VALUELEN",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GROUP_NULL
#if !defined(MPI_GROUP_NULL)
#define MPI_GROUP_NULL ((MPI_Group)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GROUP_EMPTY
#if !defined(MPI_GROUP_EMPTY)
#define MPI_GROUP_EMPTY ((MPI_Group)1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GROUP_FREE
#if !defined(MPI_Group_free)
#define MPI_Group_free(a1) PyMPI_UNAVAILABLE("MPI_GROUP_FREE",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GROUP_SIZE
#if !defined(MPI_Group_size)
#define MPI_Group_size(a1,a2) PyMPI_UNAVAILABLE("MPI_GROUP_SIZE",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GROUP_RANK
#if !defined(MPI_Group_rank)
#define MPI_Group_rank(a1,a2) PyMPI_UNAVAILABLE("MPI_GROUP_RANK",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GROUP_TRANSLATE_RANKS
#if !defined(MPI_Group_translate_ranks)
#define MPI_Group_translate_ranks(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_GROUP_TRANSLATE_RANKS",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GROUP_COMPARE
#if !defined(MPI_Group_compare)
#define MPI_Group_compare(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_GROUP_COMPARE",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GROUP_UNION
#if !defined(MPI_Group_union)
#define MPI_Group_union(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_GROUP_UNION",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GROUP_INTERSECTION
#if !defined(MPI_Group_intersection)
#define MPI_Group_intersection(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_GROUP_INTERSECTION",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GROUP_DIFFERENCE
#if !defined(MPI_Group_difference)
#define MPI_Group_difference(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_GROUP_DIFFERENCE",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GROUP_INCL
#if !defined(MPI_Group_incl)
#define MPI_Group_incl(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_GROUP_INCL",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GROUP_EXCL
#if !defined(MPI_Group_excl)
#define MPI_Group_excl(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_GROUP_EXCL",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GROUP_RANGE_INCL
#if !defined(MPI_Group_range_incl)
#define MPI_Group_range_incl(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_GROUP_RANGE_INCL",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GROUP_RANGE_EXCL
#if !defined(MPI_Group_range_excl)
#define MPI_Group_range_excl(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_GROUP_RANGE_EXCL",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_NULL
#if !defined(MPI_COMM_NULL)
#define MPI_COMM_NULL ((MPI_Comm)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_SELF
#if !defined(MPI_COMM_SELF)
#define MPI_COMM_SELF ((MPI_Comm)MPI_COMM_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_WORLD
#if !defined(MPI_COMM_WORLD)
#define MPI_COMM_WORLD ((MPI_Comm)MPI_COMM_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_FREE
#if !defined(MPI_Comm_free)
#define MPI_Comm_free(a1) PyMPI_UNAVAILABLE("MPI_COMM_FREE",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_GROUP
#if !defined(MPI_Comm_group)
#define MPI_Comm_group(a1,a2) PyMPI_UNAVAILABLE("MPI_COMM_GROUP",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_SIZE
#if !defined(MPI_Comm_size)
#define MPI_Comm_size(a1,a2) PyMPI_UNAVAILABLE("MPI_COMM_SIZE",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_RANK
#if !defined(MPI_Comm_rank)
#define MPI_Comm_rank(a1,a2) PyMPI_UNAVAILABLE("MPI_COMM_RANK",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_COMPARE
#if !defined(MPI_Comm_compare)
#define MPI_Comm_compare(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_COMM_COMPARE",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_CART
#if !defined(MPI_CART)
#define MPI_CART (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GRAPH
#if !defined(MPI_GRAPH)
#define MPI_GRAPH (2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TOPO_TEST
#if !defined(MPI_Topo_test)
#define MPI_Topo_test(a1,a2) PyMPI_UNAVAILABLE("MPI_TOPO_TEST",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_TEST_INTER
#if !defined(MPI_Comm_test_inter)
#define MPI_Comm_test_inter(a1,a2) PyMPI_UNAVAILABLE("MPI_COMM_TEST_INTER",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ABORT
#if !defined(MPI_Abort)
#define MPI_Abort(a1,a2) PyMPI_UNAVAILABLE("MPI_ABORT",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_HANDLER_FUNCTION
#if !defined(MPI_Handler_function)
typedef void (PyMPI_MPI_Handler_function)(MPI_Comm*,int*,...);
#define MPI_Handler_function PyMPI_MPI_Handler_function
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERRHANDLER_CREATE
#if !defined(MPI_Errhandler_create)
#define MPI_Errhandler_create(a1,a2) PyMPI_UNAVAILABLE("MPI_ERRHANDLER_CREATE",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERRHANDLER_GET
#if !defined(MPI_Errhandler_get)
#define MPI_Errhandler_get(a1,a2) PyMPI_UNAVAILABLE("MPI_ERRHANDLER_GET",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERRHANDLER_SET
#if !defined(MPI_Errhandler_set)
#define MPI_Errhandler_set(a1,a2) PyMPI_UNAVAILABLE("MPI_ERRHANDLER_SET",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_ERRHANDLER_FN
#if !defined(MPI_Comm_errhandler_fn)
#define MPI_Comm_errhandler_fn MPI_Handler_function
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_CREATE_ERRHANDLER
#if !defined(MPI_Comm_create_errhandler)
#define MPI_Comm_create_errhandler MPI_Errhandler_create
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_GET_ERRHANDLER
#if !defined(MPI_Comm_get_errhandler)
#define MPI_Comm_get_errhandler MPI_Errhandler_get
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_SET_ERRHANDLER
#if !defined(MPI_Comm_set_errhandler)
#define MPI_Comm_set_errhandler MPI_Errhandler_set
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_CALL_ERRHANDLER
#if !defined(MPI_Comm_call_errhandler)
#define MPI_Comm_call_errhandler(a1,a2) PyMPI_UNAVAILABLE("MPI_COMM_CALL_ERRHANDLER",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_GET_NAME
#if !defined(MPI_Comm_get_name)
#define MPI_Comm_get_name(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_COMM_GET_NAME",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_SET_NAME
#if !defined(MPI_Comm_set_name)
#define MPI_Comm_set_name(a1,a2) PyMPI_UNAVAILABLE("MPI_COMM_SET_NAME",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TAG_UB
#if !defined(MPI_TAG_UB)
#define MPI_TAG_UB (MPI_KEYVAL_INVALID)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_HOST
#if !defined(MPI_HOST)
#define MPI_HOST (MPI_KEYVAL_INVALID)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_IO
#if !defined(MPI_IO)
#define MPI_IO (MPI_KEYVAL_INVALID)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WTIME_IS_GLOBAL
#if !defined(MPI_WTIME_IS_GLOBAL)
#define MPI_WTIME_IS_GLOBAL (MPI_KEYVAL_INVALID)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_UNIVERSE_SIZE
#if !defined(MPI_UNIVERSE_SIZE)
#define MPI_UNIVERSE_SIZE (MPI_KEYVAL_INVALID)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_APPNUM
#if !defined(MPI_APPNUM)
#define MPI_APPNUM (MPI_KEYVAL_INVALID)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_LASTUSEDCODE
#if !defined(MPI_LASTUSEDCODE)
#define MPI_LASTUSEDCODE (MPI_KEYVAL_INVALID)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ATTR_GET
#if !defined(MPI_Attr_get)
#define MPI_Attr_get(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_ATTR_GET",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ATTR_PUT
#if !defined(MPI_Attr_put)
#define MPI_Attr_put(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_ATTR_PUT",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ATTR_DELETE
#if !defined(MPI_Attr_delete)
#define MPI_Attr_delete(a1,a2) PyMPI_UNAVAILABLE("MPI_ATTR_DELETE",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_GET_ATTR
#if !defined(MPI_Comm_get_attr)
#define MPI_Comm_get_attr MPI_Attr_get
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_SET_ATTR
#if !defined(MPI_Comm_set_attr)
#define MPI_Comm_set_attr MPI_Attr_put
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_DELETE_ATTR
#if !defined(MPI_Comm_delete_attr)
#define MPI_Comm_delete_attr MPI_Attr_delete
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COPY_FUNCTION
#if !defined(MPI_Copy_function)
typedef int (PyMPI_MPI_Copy_function)(MPI_Comm,int,void*,void*,void*,int*);
#define MPI_Copy_function PyMPI_MPI_Copy_function
#endif
#endif

#ifdef PyMPI_MISSING_MPI_DELETE_FUNCTION
#if !defined(MPI_Delete_function)
typedef int (PyMPI_MPI_Delete_function)(MPI_Comm,int,void*,void*);
#define MPI_Delete_function PyMPI_MPI_Delete_function
#endif
#endif

#ifdef PyMPI_MISSING_MPI_DUP_FN
#if !defined(MPI_DUP_FN)
#define MPI_DUP_FN (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_NULL_COPY_FN
#if !defined(MPI_NULL_COPY_FN)
#define MPI_NULL_COPY_FN (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_NULL_DELETE_FN
#if !defined(MPI_NULL_DELETE_FN)
#define MPI_NULL_DELETE_FN (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_KEYVAL_CREATE
#if !defined(MPI_Keyval_create)
#define MPI_Keyval_create(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_KEYVAL_CREATE",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_KEYVAL_FREE
#if !defined(MPI_Keyval_free)
#define MPI_Keyval_free(a1) PyMPI_UNAVAILABLE("MPI_KEYVAL_FREE",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_COPY_ATTR_FUNCTION
#if !defined(MPI_Comm_copy_attr_function)
#define MPI_Comm_copy_attr_function MPI_Copy_function
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_DELETE_ATTR_FUNCTION
#if !defined(MPI_Comm_delete_attr_function)
#define MPI_Comm_delete_attr_function MPI_Delete_function
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_DUP_FN
#if !defined(MPI_COMM_DUP_FN)
#define MPI_COMM_DUP_FN (MPI_DUP_FN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_NULL_COPY_FN
#if !defined(MPI_COMM_NULL_COPY_FN)
#define MPI_COMM_NULL_COPY_FN (MPI_NULL_COPY_FN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_NULL_DELETE_FN
#if !defined(MPI_COMM_NULL_DELETE_FN)
#define MPI_COMM_NULL_DELETE_FN (MPI_NULL_DELETE_FN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_CREATE_KEYVAL
#if !defined(MPI_Comm_create_keyval)
#define MPI_Comm_create_keyval MPI_Keyval_create
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_FREE_KEYVAL
#if !defined(MPI_Comm_free_keyval)
#define MPI_Comm_free_keyval MPI_Keyval_free
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SEND
#if !defined(MPI_Send)
#define MPI_Send(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_SEND",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_RECV
#if !defined(MPI_Recv)
#define MPI_Recv(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_RECV",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SENDRECV
#if !defined(MPI_Sendrecv)
#define MPI_Sendrecv(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) PyMPI_UNAVAILABLE("MPI_SENDRECV",a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SENDRECV_REPLACE
#if !defined(MPI_Sendrecv_replace)
#define MPI_Sendrecv_replace(a1,a2,a3,a4,a5,a6,a7,a8,a9) PyMPI_UNAVAILABLE("MPI_SENDRECV_REPLACE",a1,a2,a3,a4,a5,a6,a7,a8,a9)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_BSEND_OVERHEAD
#if !defined(MPI_BSEND_OVERHEAD)
#define MPI_BSEND_OVERHEAD (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_BUFFER_ATTACH
#if !defined(MPI_Buffer_attach)
#define MPI_Buffer_attach(a1,a2) PyMPI_UNAVAILABLE("MPI_BUFFER_ATTACH",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_BUFFER_DETACH
#if !defined(MPI_Buffer_detach)
#define MPI_Buffer_detach(a1,a2) PyMPI_UNAVAILABLE("MPI_BUFFER_DETACH",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_BSEND
#if !defined(MPI_Bsend)
#define MPI_Bsend(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_BSEND",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SSEND
#if !defined(MPI_Ssend)
#define MPI_Ssend(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_SSEND",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_RSEND
#if !defined(MPI_Rsend)
#define MPI_Rsend(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_RSEND",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ISEND
#if !defined(MPI_Isend)
#define MPI_Isend(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_ISEND",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_IBSEND
#if !defined(MPI_Ibsend)
#define MPI_Ibsend(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_IBSEND",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ISSEND
#if !defined(MPI_Issend)
#define MPI_Issend(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_ISSEND",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_IRSEND
#if !defined(MPI_Irsend)
#define MPI_Irsend(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_IRSEND",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_IRECV
#if !defined(MPI_Irecv)
#define MPI_Irecv(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_IRECV",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SEND_INIT
#if !defined(MPI_Send_init)
#define MPI_Send_init(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_SEND_INIT",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_BSEND_INIT
#if !defined(MPI_Bsend_init)
#define MPI_Bsend_init(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_BSEND_INIT",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SSEND_INIT
#if !defined(MPI_Ssend_init)
#define MPI_Ssend_init(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_SSEND_INIT",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_RSEND_INIT
#if !defined(MPI_Rsend_init)
#define MPI_Rsend_init(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_RSEND_INIT",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_RECV_INIT
#if !defined(MPI_Recv_init)
#define MPI_Recv_init(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_RECV_INIT",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_PROBE
#if !defined(MPI_Probe)
#define MPI_Probe(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_PROBE",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_IPROBE
#if !defined(MPI_Iprobe)
#define MPI_Iprobe(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_IPROBE",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_BARRIER
#if !defined(MPI_Barrier)
#define MPI_Barrier(a1) PyMPI_UNAVAILABLE("MPI_BARRIER",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_BCAST
#if !defined(MPI_Bcast)
#define MPI_Bcast(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_BCAST",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GATHER
#if !defined(MPI_Gather)
#define MPI_Gather(a1,a2,a3,a4,a5,a6,a7,a8) PyMPI_UNAVAILABLE("MPI_GATHER",a1,a2,a3,a4,a5,a6,a7,a8)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GATHERV
#if !defined(MPI_Gatherv)
#define MPI_Gatherv(a1,a2,a3,a4,a5,a6,a7,a8,a9) PyMPI_UNAVAILABLE("MPI_GATHERV",a1,a2,a3,a4,a5,a6,a7,a8,a9)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SCATTER
#if !defined(MPI_Scatter)
#define MPI_Scatter(a1,a2,a3,a4,a5,a6,a7,a8) PyMPI_UNAVAILABLE("MPI_SCATTER",a1,a2,a3,a4,a5,a6,a7,a8)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SCATTERV
#if !defined(MPI_Scatterv)
#define MPI_Scatterv(a1,a2,a3,a4,a5,a6,a7,a8,a9) PyMPI_UNAVAILABLE("MPI_SCATTERV",a1,a2,a3,a4,a5,a6,a7,a8,a9)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ALLGATHER
#if !defined(MPI_Allgather)
#define MPI_Allgather(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_ALLGATHER",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ALLGATHERV
#if !defined(MPI_Allgatherv)
#define MPI_Allgatherv(a1,a2,a3,a4,a5,a6,a7,a8) PyMPI_UNAVAILABLE("MPI_ALLGATHERV",a1,a2,a3,a4,a5,a6,a7,a8)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ALLTOALL
#if !defined(MPI_Alltoall)
#define MPI_Alltoall(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_ALLTOALL",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ALLTOALLV
#if !defined(MPI_Alltoallv)
#define MPI_Alltoallv(a1,a2,a3,a4,a5,a6,a7,a8,a9) PyMPI_UNAVAILABLE("MPI_ALLTOALLV",a1,a2,a3,a4,a5,a6,a7,a8,a9)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ALLTOALLW
#if !defined(MPI_Alltoallw)
#define MPI_Alltoallw(a1,a2,a3,a4,a5,a6,a7,a8,a9) PyMPI_UNAVAILABLE("MPI_ALLTOALLW",a1,a2,a3,a4,a5,a6,a7,a8,a9)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_REDUCE
#if !defined(MPI_Reduce)
#define MPI_Reduce(a1,a2,a3,a4,a5,a6,a7) PyMPI_UNAVAILABLE("MPI_REDUCE",a1,a2,a3,a4,a5,a6,a7)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ALLREDUCE
#if !defined(MPI_Allreduce)
#define MPI_Allreduce(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_ALLREDUCE",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_REDUCE_SCATTER
#if !defined(MPI_Reduce_scatter)
#define MPI_Reduce_scatter(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_REDUCE_SCATTER",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SCAN
#if !defined(MPI_Scan)
#define MPI_Scan(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_SCAN",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_EXSCAN
#if !defined(MPI_Exscan)
#define MPI_Exscan(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_EXSCAN",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_DUP
#if !defined(MPI_Comm_dup)
#define MPI_Comm_dup(a1,a2) PyMPI_UNAVAILABLE("MPI_COMM_DUP",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_CREATE
#if !defined(MPI_Comm_create)
#define MPI_Comm_create(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_COMM_CREATE",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_SPLIT
#if !defined(MPI_Comm_split)
#define MPI_Comm_split(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_COMM_SPLIT",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_CART_CREATE
#if !defined(MPI_Cart_create)
#define MPI_Cart_create(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_CART_CREATE",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GRAPH_CREATE
#if !defined(MPI_Graph_create)
#define MPI_Graph_create(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_GRAPH_CREATE",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INTERCOMM_CREATE
#if !defined(MPI_Intercomm_create)
#define MPI_Intercomm_create(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_INTERCOMM_CREATE",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_CARTDIM_GET
#if !defined(MPI_Cartdim_get)
#define MPI_Cartdim_get(a1,a2) PyMPI_UNAVAILABLE("MPI_CARTDIM_GET",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_CART_GET
#if !defined(MPI_Cart_get)
#define MPI_Cart_get(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_CART_GET",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_CART_RANK
#if !defined(MPI_Cart_rank)
#define MPI_Cart_rank(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_CART_RANK",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_CART_COORDS
#if !defined(MPI_Cart_coords)
#define MPI_Cart_coords(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_CART_COORDS",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_CART_SHIFT
#if !defined(MPI_Cart_shift)
#define MPI_Cart_shift(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_CART_SHIFT",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_CART_SUB
#if !defined(MPI_Cart_sub)
#define MPI_Cart_sub(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_CART_SUB",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_CART_MAP
#if !defined(MPI_Cart_map)
#define MPI_Cart_map(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_CART_MAP",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_DIMS_CREATE
#if !defined(MPI_Dims_create)
#define MPI_Dims_create(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_DIMS_CREATE",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GRAPHDIMS_GET
#if !defined(MPI_Graphdims_get)
#define MPI_Graphdims_get(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_GRAPHDIMS_GET",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GRAPH_GET
#if !defined(MPI_Graph_get)
#define MPI_Graph_get(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_GRAPH_GET",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GRAPH_MAP
#if !defined(MPI_Graph_map)
#define MPI_Graph_map(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_GRAPH_MAP",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GRAPH_NEIGHBORS_COUNT
#if !defined(MPI_Graph_neighbors_count)
#define MPI_Graph_neighbors_count(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_GRAPH_NEIGHBORS_COUNT",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GRAPH_NEIGHBORS
#if !defined(MPI_Graph_neighbors)
#define MPI_Graph_neighbors(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_GRAPH_NEIGHBORS",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_REMOTE_GROUP
#if !defined(MPI_Comm_remote_group)
#define MPI_Comm_remote_group(a1,a2) PyMPI_UNAVAILABLE("MPI_COMM_REMOTE_GROUP",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_REMOTE_SIZE
#if !defined(MPI_Comm_remote_size)
#define MPI_Comm_remote_size(a1,a2) PyMPI_UNAVAILABLE("MPI_COMM_REMOTE_SIZE",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INTERCOMM_MERGE
#if !defined(MPI_Intercomm_merge)
#define MPI_Intercomm_merge(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_INTERCOMM_MERGE",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MAX_PORT_NAME
#if !defined(MPI_MAX_PORT_NAME)
#define MPI_MAX_PORT_NAME (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_OPEN_PORT
#if !defined(MPI_Open_port)
#define MPI_Open_port(a1,a2) PyMPI_UNAVAILABLE("MPI_OPEN_PORT",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_CLOSE_PORT
#if !defined(MPI_Close_port)
#define MPI_Close_port(a1) PyMPI_UNAVAILABLE("MPI_CLOSE_PORT",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_PUBLISH_NAME
#if !defined(MPI_Publish_name)
#define MPI_Publish_name(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_PUBLISH_NAME",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_UNPUBLISH_NAME
#if !defined(MPI_Unpublish_name)
#define MPI_Unpublish_name(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_UNPUBLISH_NAME",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_LOOKUP_NAME
#if !defined(MPI_Lookup_name)
#define MPI_Lookup_name(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_LOOKUP_NAME",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_ACCEPT
#if !defined(MPI_Comm_accept)
#define MPI_Comm_accept(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_COMM_ACCEPT",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_CONNECT
#if !defined(MPI_Comm_connect)
#define MPI_Comm_connect(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_COMM_CONNECT",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_JOIN
#if !defined(MPI_Comm_join)
#define MPI_Comm_join(a1,a2) PyMPI_UNAVAILABLE("MPI_COMM_JOIN",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_DISCONNECT
#if !defined(MPI_Comm_disconnect)
#define MPI_Comm_disconnect(a1) PyMPI_UNAVAILABLE("MPI_COMM_DISCONNECT",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ARGV_NULL
#if !defined(MPI_ARGV_NULL)
#define MPI_ARGV_NULL ((char**)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ARGVS_NULL
#if !defined(MPI_ARGVS_NULL)
#define MPI_ARGVS_NULL ((char***)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERRCODES_IGNORE
#if !defined(MPI_ERRCODES_IGNORE)
#define MPI_ERRCODES_IGNORE ((int*)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_SPAWN
#if !defined(MPI_Comm_spawn)
#define MPI_Comm_spawn(a1,a2,a3,a4,a5,a6,a7,a8) PyMPI_UNAVAILABLE("MPI_COMM_SPAWN",a1,a2,a3,a4,a5,a6,a7,a8)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_SPAWN_MULTIPLE
#if !defined(MPI_Comm_spawn_multiple)
#define MPI_Comm_spawn_multiple(a1,a2,a3,a4,a5,a6,a7,a8,a9) PyMPI_UNAVAILABLE("MPI_COMM_SPAWN_MULTIPLE",a1,a2,a3,a4,a5,a6,a7,a8,a9)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_GET_PARENT
#if !defined(MPI_Comm_get_parent)
#define MPI_Comm_get_parent(a1) PyMPI_UNAVAILABLE("MPI_COMM_GET_PARENT",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_NULL
#if !defined(MPI_WIN_NULL)
#define MPI_WIN_NULL ((MPI_Win)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_FREE
#if !defined(MPI_Win_free)
#define MPI_Win_free(a1) PyMPI_UNAVAILABLE("MPI_WIN_FREE",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_CREATE
#if !defined(MPI_Win_create)
#define MPI_Win_create(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_WIN_CREATE",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_GET_GROUP
#if !defined(MPI_Win_get_group)
#define MPI_Win_get_group(a1,a2) PyMPI_UNAVAILABLE("MPI_WIN_GET_GROUP",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GET
#if !defined(MPI_Get)
#define MPI_Get(a1,a2,a3,a4,a5,a6,a7,a8) PyMPI_UNAVAILABLE("MPI_GET",a1,a2,a3,a4,a5,a6,a7,a8)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_PUT
#if !defined(MPI_Put)
#define MPI_Put(a1,a2,a3,a4,a5,a6,a7,a8) PyMPI_UNAVAILABLE("MPI_PUT",a1,a2,a3,a4,a5,a6,a7,a8)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ACCUMULATE
#if !defined(MPI_Accumulate)
#define MPI_Accumulate(a1,a2,a3,a4,a5,a6,a7,a8,a9) PyMPI_UNAVAILABLE("MPI_ACCUMULATE",a1,a2,a3,a4,a5,a6,a7,a8,a9)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MODE_NOCHECK
#if !defined(MPI_MODE_NOCHECK)
#define MPI_MODE_NOCHECK (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MODE_NOSTORE
#if !defined(MPI_MODE_NOSTORE)
#define MPI_MODE_NOSTORE (2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MODE_NOPUT
#if !defined(MPI_MODE_NOPUT)
#define MPI_MODE_NOPUT (4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MODE_NOPRECEDE
#if !defined(MPI_MODE_NOPRECEDE)
#define MPI_MODE_NOPRECEDE (8)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MODE_NOSUCCEED
#if !defined(MPI_MODE_NOSUCCEED)
#define MPI_MODE_NOSUCCEED (16)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_LOCK_EXCLUSIVE
#if !defined(MPI_LOCK_EXCLUSIVE)
#define MPI_LOCK_EXCLUSIVE (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_LOCK_SHARED
#if !defined(MPI_LOCK_SHARED)
#define MPI_LOCK_SHARED (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_FENCE
#if !defined(MPI_Win_fence)
#define MPI_Win_fence(a1,a2) PyMPI_UNAVAILABLE("MPI_WIN_FENCE",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_POST
#if !defined(MPI_Win_post)
#define MPI_Win_post(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_WIN_POST",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_START
#if !defined(MPI_Win_start)
#define MPI_Win_start(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_WIN_START",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_COMPLETE
#if !defined(MPI_Win_complete)
#define MPI_Win_complete(a1) PyMPI_UNAVAILABLE("MPI_WIN_COMPLETE",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_LOCK
#if !defined(MPI_Win_lock)
#define MPI_Win_lock(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_WIN_LOCK",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_UNLOCK
#if !defined(MPI_Win_unlock)
#define MPI_Win_unlock(a1,a2) PyMPI_UNAVAILABLE("MPI_WIN_UNLOCK",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_WAIT
#if !defined(MPI_Win_wait)
#define MPI_Win_wait(a1) PyMPI_UNAVAILABLE("MPI_WIN_WAIT",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_TEST
#if !defined(MPI_Win_test)
#define MPI_Win_test(a1,a2) PyMPI_UNAVAILABLE("MPI_WIN_TEST",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_ERRHANDLER_FN
#if !defined(MPI_Win_errhandler_fn)
typedef void (PyMPI_MPI_Win_errhandler_fn)(MPI_Win*,int*,...);
#define MPI_Win_errhandler_fn PyMPI_MPI_Win_errhandler_fn
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_CREATE_ERRHANDLER
#if !defined(MPI_Win_create_errhandler)
#define MPI_Win_create_errhandler(a1,a2) PyMPI_UNAVAILABLE("MPI_WIN_CREATE_ERRHANDLER",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_GET_ERRHANDLER
#if !defined(MPI_Win_get_errhandler)
#define MPI_Win_get_errhandler(a1,a2) PyMPI_UNAVAILABLE("MPI_WIN_GET_ERRHANDLER",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_SET_ERRHANDLER
#if !defined(MPI_Win_set_errhandler)
#define MPI_Win_set_errhandler(a1,a2) PyMPI_UNAVAILABLE("MPI_WIN_SET_ERRHANDLER",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_CALL_ERRHANDLER
#if !defined(MPI_Win_call_errhandler)
#define MPI_Win_call_errhandler(a1,a2) PyMPI_UNAVAILABLE("MPI_WIN_CALL_ERRHANDLER",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_GET_NAME
#if !defined(MPI_Win_get_name)
#define MPI_Win_get_name(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_WIN_GET_NAME",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_SET_NAME
#if !defined(MPI_Win_set_name)
#define MPI_Win_set_name(a1,a2) PyMPI_UNAVAILABLE("MPI_WIN_SET_NAME",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_BASE
#if !defined(MPI_WIN_BASE)
#define MPI_WIN_BASE (MPI_KEYVAL_INVALID)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_SIZE
#if !defined(MPI_WIN_SIZE)
#define MPI_WIN_SIZE (MPI_KEYVAL_INVALID)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_DISP_UNIT
#if !defined(MPI_WIN_DISP_UNIT)
#define MPI_WIN_DISP_UNIT (MPI_KEYVAL_INVALID)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_GET_ATTR
#if !defined(MPI_Win_get_attr)
#define MPI_Win_get_attr(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_WIN_GET_ATTR",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_SET_ATTR
#if !defined(MPI_Win_set_attr)
#define MPI_Win_set_attr(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_WIN_SET_ATTR",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_DELETE_ATTR
#if !defined(MPI_Win_delete_attr)
#define MPI_Win_delete_attr(a1,a2) PyMPI_UNAVAILABLE("MPI_WIN_DELETE_ATTR",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_COPY_ATTR_FUNCTION
#if !defined(MPI_Win_copy_attr_function)
typedef int (PyMPI_MPI_Win_copy_attr_function)(MPI_Win,int,void*,void*,void*,int*);
#define MPI_Win_copy_attr_function PyMPI_MPI_Win_copy_attr_function
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_DELETE_ATTR_FUNCTION
#if !defined(MPI_Win_delete_attr_function)
typedef int (PyMPI_MPI_Win_delete_attr_function)(MPI_Win,int,void*,void*);
#define MPI_Win_delete_attr_function PyMPI_MPI_Win_delete_attr_function
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_DUP_FN
#if !defined(MPI_WIN_DUP_FN)
#define MPI_WIN_DUP_FN (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_NULL_COPY_FN
#if !defined(MPI_WIN_NULL_COPY_FN)
#define MPI_WIN_NULL_COPY_FN (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_NULL_DELETE_FN
#if !defined(MPI_WIN_NULL_DELETE_FN)
#define MPI_WIN_NULL_DELETE_FN (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_CREATE_KEYVAL
#if !defined(MPI_Win_create_keyval)
#define MPI_Win_create_keyval(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_WIN_CREATE_KEYVAL",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_FREE_KEYVAL
#if !defined(MPI_Win_free_keyval)
#define MPI_Win_free_keyval(a1) PyMPI_UNAVAILABLE("MPI_WIN_FREE_KEYVAL",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_NULL
#if !defined(MPI_FILE_NULL)
#define MPI_FILE_NULL ((MPI_File)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MODE_RDONLY
#if !defined(MPI_MODE_RDONLY)
#define MPI_MODE_RDONLY (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MODE_RDWR
#if !defined(MPI_MODE_RDWR)
#define MPI_MODE_RDWR (2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MODE_WRONLY
#if !defined(MPI_MODE_WRONLY)
#define MPI_MODE_WRONLY (4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MODE_CREATE
#if !defined(MPI_MODE_CREATE)
#define MPI_MODE_CREATE (8)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MODE_EXCL
#if !defined(MPI_MODE_EXCL)
#define MPI_MODE_EXCL (16)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MODE_DELETE_ON_CLOSE
#if !defined(MPI_MODE_DELETE_ON_CLOSE)
#define MPI_MODE_DELETE_ON_CLOSE (32)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MODE_UNIQUE_OPEN
#if !defined(MPI_MODE_UNIQUE_OPEN)
#define MPI_MODE_UNIQUE_OPEN (64)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MODE_APPEND
#if !defined(MPI_MODE_APPEND)
#define MPI_MODE_APPEND (128)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MODE_SEQUENTIAL
#if !defined(MPI_MODE_SEQUENTIAL)
#define MPI_MODE_SEQUENTIAL (256)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_OPEN
#if !defined(MPI_File_open)
#define MPI_File_open(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_FILE_OPEN",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_CLOSE
#if !defined(MPI_File_close)
#define MPI_File_close(a1) PyMPI_UNAVAILABLE("MPI_FILE_CLOSE",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_DELETE
#if !defined(MPI_File_delete)
#define MPI_File_delete(a1,a2) PyMPI_UNAVAILABLE("MPI_FILE_DELETE",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_SET_SIZE
#if !defined(MPI_File_set_size)
#define MPI_File_set_size(a1,a2) PyMPI_UNAVAILABLE("MPI_FILE_SET_SIZE",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_PREALLOCATE
#if !defined(MPI_File_preallocate)
#define MPI_File_preallocate(a1,a2) PyMPI_UNAVAILABLE("MPI_FILE_PREALLOCATE",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_GET_SIZE
#if !defined(MPI_File_get_size)
#define MPI_File_get_size(a1,a2) PyMPI_UNAVAILABLE("MPI_FILE_GET_SIZE",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_GET_GROUP
#if !defined(MPI_File_get_group)
#define MPI_File_get_group(a1,a2) PyMPI_UNAVAILABLE("MPI_FILE_GET_GROUP",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_GET_AMODE
#if !defined(MPI_File_get_amode)
#define MPI_File_get_amode(a1,a2) PyMPI_UNAVAILABLE("MPI_FILE_GET_AMODE",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_SET_INFO
#if !defined(MPI_File_set_info)
#define MPI_File_set_info(a1,a2) PyMPI_UNAVAILABLE("MPI_FILE_SET_INFO",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_GET_INFO
#if !defined(MPI_File_get_info)
#define MPI_File_get_info(a1,a2) PyMPI_UNAVAILABLE("MPI_FILE_GET_INFO",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MAX_DATAREP_STRING
#if !defined(MPI_MAX_DATAREP_STRING)
#define MPI_MAX_DATAREP_STRING (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_GET_VIEW
#if !defined(MPI_File_get_view)
#define MPI_File_get_view(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_FILE_GET_VIEW",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_SET_VIEW
#if !defined(MPI_File_set_view)
#define MPI_File_set_view(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_FILE_SET_VIEW",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_READ_AT
#if !defined(MPI_File_read_at)
#define MPI_File_read_at(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_FILE_READ_AT",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_READ_AT_ALL
#if !defined(MPI_File_read_at_all)
#define MPI_File_read_at_all(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_FILE_READ_AT_ALL",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_WRITE_AT
#if !defined(MPI_File_write_at)
#define MPI_File_write_at(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_FILE_WRITE_AT",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_WRITE_AT_ALL
#if !defined(MPI_File_write_at_all)
#define MPI_File_write_at_all(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_FILE_WRITE_AT_ALL",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_IREAD_AT
#if !defined(MPI_File_iread_at)
#define MPI_File_iread_at(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_FILE_IREAD_AT",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_IWRITE_AT
#if !defined(MPI_File_iwrite_at)
#define MPI_File_iwrite_at(a1,a2,a3,a4,a5,a6) PyMPI_UNAVAILABLE("MPI_FILE_IWRITE_AT",a1,a2,a3,a4,a5,a6)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SEEK_SET
#if !defined(MPI_SEEK_SET)
#define MPI_SEEK_SET (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SEEK_CUR
#if !defined(MPI_SEEK_CUR)
#define MPI_SEEK_CUR (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SEEK_END
#if !defined(MPI_SEEK_END)
#define MPI_SEEK_END (2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_DISPLACEMENT_CURRENT
#if !defined(MPI_DISPLACEMENT_CURRENT)
#define MPI_DISPLACEMENT_CURRENT (3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_SEEK
#if !defined(MPI_File_seek)
#define MPI_File_seek(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_FILE_SEEK",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_GET_POSITION
#if !defined(MPI_File_get_position)
#define MPI_File_get_position(a1,a2) PyMPI_UNAVAILABLE("MPI_FILE_GET_POSITION",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_GET_BYTE_OFFSET
#if !defined(MPI_File_get_byte_offset)
#define MPI_File_get_byte_offset(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_FILE_GET_BYTE_OFFSET",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_READ
#if !defined(MPI_File_read)
#define MPI_File_read(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_FILE_READ",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_READ_ALL
#if !defined(MPI_File_read_all)
#define MPI_File_read_all(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_FILE_READ_ALL",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_WRITE
#if !defined(MPI_File_write)
#define MPI_File_write(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_FILE_WRITE",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_WRITE_ALL
#if !defined(MPI_File_write_all)
#define MPI_File_write_all(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_FILE_WRITE_ALL",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_IREAD
#if !defined(MPI_File_iread)
#define MPI_File_iread(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_FILE_IREAD",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_IWRITE
#if !defined(MPI_File_iwrite)
#define MPI_File_iwrite(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_FILE_IWRITE",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_READ_SHARED
#if !defined(MPI_File_read_shared)
#define MPI_File_read_shared(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_FILE_READ_SHARED",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_WRITE_SHARED
#if !defined(MPI_File_write_shared)
#define MPI_File_write_shared(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_FILE_WRITE_SHARED",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_IREAD_SHARED
#if !defined(MPI_File_iread_shared)
#define MPI_File_iread_shared(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_FILE_IREAD_SHARED",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_IWRITE_SHARED
#if !defined(MPI_File_iwrite_shared)
#define MPI_File_iwrite_shared(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_FILE_IWRITE_SHARED",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_READ_ORDERED
#if !defined(MPI_File_read_ordered)
#define MPI_File_read_ordered(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_FILE_READ_ORDERED",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_WRITE_ORDERED
#if !defined(MPI_File_write_ordered)
#define MPI_File_write_ordered(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_FILE_WRITE_ORDERED",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_SEEK_SHARED
#if !defined(MPI_File_seek_shared)
#define MPI_File_seek_shared(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_FILE_SEEK_SHARED",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_GET_POSITION_SHARED
#if !defined(MPI_File_get_position_shared)
#define MPI_File_get_position_shared(a1,a2) PyMPI_UNAVAILABLE("MPI_FILE_GET_POSITION_SHARED",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_READ_AT_ALL_BEGIN
#if !defined(MPI_File_read_at_all_begin)
#define MPI_File_read_at_all_begin(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_FILE_READ_AT_ALL_BEGIN",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_READ_AT_ALL_END
#if !defined(MPI_File_read_at_all_end)
#define MPI_File_read_at_all_end(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_FILE_READ_AT_ALL_END",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_WRITE_AT_ALL_BEGIN
#if !defined(MPI_File_write_at_all_begin)
#define MPI_File_write_at_all_begin(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_FILE_WRITE_AT_ALL_BEGIN",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_WRITE_AT_ALL_END
#if !defined(MPI_File_write_at_all_end)
#define MPI_File_write_at_all_end(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_FILE_WRITE_AT_ALL_END",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_READ_ALL_BEGIN
#if !defined(MPI_File_read_all_begin)
#define MPI_File_read_all_begin(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_FILE_READ_ALL_BEGIN",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_READ_ALL_END
#if !defined(MPI_File_read_all_end)
#define MPI_File_read_all_end(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_FILE_READ_ALL_END",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_WRITE_ALL_BEGIN
#if !defined(MPI_File_write_all_begin)
#define MPI_File_write_all_begin(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_FILE_WRITE_ALL_BEGIN",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_WRITE_ALL_END
#if !defined(MPI_File_write_all_end)
#define MPI_File_write_all_end(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_FILE_WRITE_ALL_END",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_READ_ORDERED_BEGIN
#if !defined(MPI_File_read_ordered_begin)
#define MPI_File_read_ordered_begin(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_FILE_READ_ORDERED_BEGIN",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_READ_ORDERED_END
#if !defined(MPI_File_read_ordered_end)
#define MPI_File_read_ordered_end(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_FILE_READ_ORDERED_END",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_WRITE_ORDERED_BEGIN
#if !defined(MPI_File_write_ordered_begin)
#define MPI_File_write_ordered_begin(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_FILE_WRITE_ORDERED_BEGIN",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_WRITE_ORDERED_END
#if !defined(MPI_File_write_ordered_end)
#define MPI_File_write_ordered_end(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_FILE_WRITE_ORDERED_END",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_GET_TYPE_EXTENT
#if !defined(MPI_File_get_type_extent)
#define MPI_File_get_type_extent(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_FILE_GET_TYPE_EXTENT",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_SET_ATOMICITY
#if !defined(MPI_File_set_atomicity)
#define MPI_File_set_atomicity(a1,a2) PyMPI_UNAVAILABLE("MPI_FILE_SET_ATOMICITY",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_GET_ATOMICITY
#if !defined(MPI_File_get_atomicity)
#define MPI_File_get_atomicity(a1,a2) PyMPI_UNAVAILABLE("MPI_FILE_GET_ATOMICITY",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_SYNC
#if !defined(MPI_File_sync)
#define MPI_File_sync(a1) PyMPI_UNAVAILABLE("MPI_FILE_SYNC",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_ERRHANDLER_FN
#if !defined(MPI_File_errhandler_fn)
typedef void (PyMPI_MPI_File_errhandler_fn)(MPI_File*,int*,...);
#define MPI_File_errhandler_fn PyMPI_MPI_File_errhandler_fn
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_CREATE_ERRHANDLER
#if !defined(MPI_File_create_errhandler)
#define MPI_File_create_errhandler(a1,a2) PyMPI_UNAVAILABLE("MPI_FILE_CREATE_ERRHANDLER",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_GET_ERRHANDLER
#if !defined(MPI_File_get_errhandler)
#define MPI_File_get_errhandler(a1,a2) PyMPI_UNAVAILABLE("MPI_FILE_GET_ERRHANDLER",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_SET_ERRHANDLER
#if !defined(MPI_File_set_errhandler)
#define MPI_File_set_errhandler(a1,a2) PyMPI_UNAVAILABLE("MPI_FILE_SET_ERRHANDLER",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_CALL_ERRHANDLER
#if !defined(MPI_File_call_errhandler)
#define MPI_File_call_errhandler(a1,a2) PyMPI_UNAVAILABLE("MPI_FILE_CALL_ERRHANDLER",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_DATAREP_CONVERSION_FUNCTION
#if !defined(MPI_Datarep_conversion_function)
typedef int (PyMPI_MPI_Datarep_conversion_function)(void*,MPI_Datatype,int,void*,MPI_Offset,void*);
#define MPI_Datarep_conversion_function PyMPI_MPI_Datarep_conversion_function
#endif
#endif

#ifdef PyMPI_MISSING_MPI_DATAREP_EXTENT_FUNCTION
#if !defined(MPI_Datarep_extent_function)
typedef int (PyMPI_MPI_Datarep_extent_function)(MPI_Datatype,MPI_Aint*,void*);
#define MPI_Datarep_extent_function PyMPI_MPI_Datarep_extent_function
#endif
#endif

#ifdef PyMPI_MISSING_MPI_REGISTER_DATAREP
#if !defined(MPI_Register_datarep)
#define MPI_Register_datarep(a1,a2,a3,a4,a5) PyMPI_UNAVAILABLE("MPI_REGISTER_DATAREP",a1,a2,a3,a4,a5)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERRHANDLER_NULL
#if !defined(MPI_ERRHANDLER_NULL)
#define MPI_ERRHANDLER_NULL ((MPI_Errhandler)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERRORS_RETURN
#if !defined(MPI_ERRORS_RETURN)
#define MPI_ERRORS_RETURN ((MPI_Errhandler)MPI_ERRHANDLER_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERRORS_ARE_FATAL
#if !defined(MPI_ERRORS_ARE_FATAL)
#define MPI_ERRORS_ARE_FATAL ((MPI_Errhandler)MPI_ERRHANDLER_NULL)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERRHANDLER_FREE
#if !defined(MPI_Errhandler_free)
#define MPI_Errhandler_free(a1) PyMPI_UNAVAILABLE("MPI_ERRHANDLER_FREE",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MAX_ERROR_STRING
#if !defined(MPI_MAX_ERROR_STRING)
#define MPI_MAX_ERROR_STRING (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERROR_CLASS
#if !defined(MPI_Error_class)
#define MPI_Error_class(a1,a2) PyMPI_UNAVAILABLE("MPI_ERROR_CLASS",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERROR_STRING
#if !defined(MPI_Error_string)
#define MPI_Error_string(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_ERROR_STRING",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ADD_ERROR_CLASS
#if !defined(MPI_Add_error_class)
#define MPI_Add_error_class(a1) PyMPI_UNAVAILABLE("MPI_ADD_ERROR_CLASS",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ADD_ERROR_CODE
#if !defined(MPI_Add_error_code)
#define MPI_Add_error_code(a1,a2) PyMPI_UNAVAILABLE("MPI_ADD_ERROR_CODE",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ADD_ERROR_STRING
#if !defined(MPI_Add_error_string)
#define MPI_Add_error_string(a1,a2) PyMPI_UNAVAILABLE("MPI_ADD_ERROR_STRING",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SUCCESS
#if !defined(MPI_SUCCESS)
#define MPI_SUCCESS (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_LASTCODE
#if !defined(MPI_ERR_LASTCODE)
#define MPI_ERR_LASTCODE (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_COMM
#if !defined(MPI_ERR_COMM)
#define MPI_ERR_COMM (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_GROUP
#if !defined(MPI_ERR_GROUP)
#define MPI_ERR_GROUP (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_TYPE
#if !defined(MPI_ERR_TYPE)
#define MPI_ERR_TYPE (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_REQUEST
#if !defined(MPI_ERR_REQUEST)
#define MPI_ERR_REQUEST (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_OP
#if !defined(MPI_ERR_OP)
#define MPI_ERR_OP (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_BUFFER
#if !defined(MPI_ERR_BUFFER)
#define MPI_ERR_BUFFER (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_COUNT
#if !defined(MPI_ERR_COUNT)
#define MPI_ERR_COUNT (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_TAG
#if !defined(MPI_ERR_TAG)
#define MPI_ERR_TAG (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_RANK
#if !defined(MPI_ERR_RANK)
#define MPI_ERR_RANK (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_ROOT
#if !defined(MPI_ERR_ROOT)
#define MPI_ERR_ROOT (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_TRUNCATE
#if !defined(MPI_ERR_TRUNCATE)
#define MPI_ERR_TRUNCATE (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_IN_STATUS
#if !defined(MPI_ERR_IN_STATUS)
#define MPI_ERR_IN_STATUS (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_PENDING
#if !defined(MPI_ERR_PENDING)
#define MPI_ERR_PENDING (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_TOPOLOGY
#if !defined(MPI_ERR_TOPOLOGY)
#define MPI_ERR_TOPOLOGY (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_DIMS
#if !defined(MPI_ERR_DIMS)
#define MPI_ERR_DIMS (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_ARG
#if !defined(MPI_ERR_ARG)
#define MPI_ERR_ARG (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_OTHER
#if !defined(MPI_ERR_OTHER)
#define MPI_ERR_OTHER (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_UNKNOWN
#if !defined(MPI_ERR_UNKNOWN)
#define MPI_ERR_UNKNOWN (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_INTERN
#if !defined(MPI_ERR_INTERN)
#define MPI_ERR_INTERN (MPI_ERR_LASTCODE)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_INFO
#if !defined(MPI_ERR_INFO)
#define MPI_ERR_INFO (MPI_ERR_ARG)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_FILE
#if !defined(MPI_ERR_FILE)
#define MPI_ERR_FILE (MPI_ERR_ARG)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_WIN
#if !defined(MPI_ERR_WIN)
#define MPI_ERR_WIN (MPI_ERR_ARG)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_KEYVAL
#if !defined(MPI_ERR_KEYVAL)
#define MPI_ERR_KEYVAL (MPI_ERR_ARG)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_INFO_KEY
#if !defined(MPI_ERR_INFO_KEY)
#define MPI_ERR_INFO_KEY (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_INFO_VALUE
#if !defined(MPI_ERR_INFO_VALUE)
#define MPI_ERR_INFO_VALUE (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_INFO_NOKEY
#if !defined(MPI_ERR_INFO_NOKEY)
#define MPI_ERR_INFO_NOKEY (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_ACCESS
#if !defined(MPI_ERR_ACCESS)
#define MPI_ERR_ACCESS (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_AMODE
#if !defined(MPI_ERR_AMODE)
#define MPI_ERR_AMODE (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_BAD_FILE
#if !defined(MPI_ERR_BAD_FILE)
#define MPI_ERR_BAD_FILE (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_FILE_EXISTS
#if !defined(MPI_ERR_FILE_EXISTS)
#define MPI_ERR_FILE_EXISTS (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_FILE_IN_USE
#if !defined(MPI_ERR_FILE_IN_USE)
#define MPI_ERR_FILE_IN_USE (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_NO_SPACE
#if !defined(MPI_ERR_NO_SPACE)
#define MPI_ERR_NO_SPACE (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_NO_SUCH_FILE
#if !defined(MPI_ERR_NO_SUCH_FILE)
#define MPI_ERR_NO_SUCH_FILE (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_IO
#if !defined(MPI_ERR_IO)
#define MPI_ERR_IO (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_READ_ONLY
#if !defined(MPI_ERR_READ_ONLY)
#define MPI_ERR_READ_ONLY (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_CONVERSION
#if !defined(MPI_ERR_CONVERSION)
#define MPI_ERR_CONVERSION (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_DUP_DATAREP
#if !defined(MPI_ERR_DUP_DATAREP)
#define MPI_ERR_DUP_DATAREP (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_UNSUPPORTED_DATAREP
#if !defined(MPI_ERR_UNSUPPORTED_DATAREP)
#define MPI_ERR_UNSUPPORTED_DATAREP (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_UNSUPPORTED_OPERATION
#if !defined(MPI_ERR_UNSUPPORTED_OPERATION)
#define MPI_ERR_UNSUPPORTED_OPERATION (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_NAME
#if !defined(MPI_ERR_NAME)
#define MPI_ERR_NAME (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_NO_MEM
#if !defined(MPI_ERR_NO_MEM)
#define MPI_ERR_NO_MEM (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_NOT_SAME
#if !defined(MPI_ERR_NOT_SAME)
#define MPI_ERR_NOT_SAME (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_PORT
#if !defined(MPI_ERR_PORT)
#define MPI_ERR_PORT (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_QUOTA
#if !defined(MPI_ERR_QUOTA)
#define MPI_ERR_QUOTA (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_SERVICE
#if !defined(MPI_ERR_SERVICE)
#define MPI_ERR_SERVICE (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_SPAWN
#if !defined(MPI_ERR_SPAWN)
#define MPI_ERR_SPAWN (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_BASE
#if !defined(MPI_ERR_BASE)
#define MPI_ERR_BASE (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_SIZE
#if !defined(MPI_ERR_SIZE)
#define MPI_ERR_SIZE (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_DISP
#if !defined(MPI_ERR_DISP)
#define MPI_ERR_DISP (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_LOCKTYPE
#if !defined(MPI_ERR_LOCKTYPE)
#define MPI_ERR_LOCKTYPE (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_ASSERT
#if !defined(MPI_ERR_ASSERT)
#define MPI_ERR_ASSERT (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_RMA_CONFLICT
#if !defined(MPI_ERR_RMA_CONFLICT)
#define MPI_ERR_RMA_CONFLICT (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERR_RMA_SYNC
#if !defined(MPI_ERR_RMA_SYNC)
#define MPI_ERR_RMA_SYNC (MPI_ERR_UNKNOWN)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ALLOC_MEM
#if !defined(MPI_Alloc_mem)
#define MPI_Alloc_mem(a1,a2,a3) PyMPI_UNAVAILABLE("MPI_ALLOC_MEM",a1,a2,a3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FREE_MEM
#if !defined(MPI_Free_mem)
#define MPI_Free_mem(a1) PyMPI_UNAVAILABLE("MPI_FREE_MEM",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INIT
#if !defined(MPI_Init)
#define MPI_Init(a1,a2) PyMPI_UNAVAILABLE("MPI_INIT",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FINALIZE
#if !defined(MPI_Finalize)
#define MPI_Finalize() PyMPI_UNAVAILABLE("MPI_FINALIZE")
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INITIALIZED
#if !defined(MPI_Initialized)
#define MPI_Initialized(a1) PyMPI_UNAVAILABLE("MPI_INITIALIZED",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FINALIZED
#if !defined(MPI_Finalized)
#define MPI_Finalized(a1) PyMPI_UNAVAILABLE("MPI_FINALIZED",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_THREAD_SINGLE
#if !defined(MPI_THREAD_SINGLE)
#define MPI_THREAD_SINGLE (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_THREAD_FUNNELED
#if !defined(MPI_THREAD_FUNNELED)
#define MPI_THREAD_FUNNELED (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_THREAD_SERIALIZED
#if !defined(MPI_THREAD_SERIALIZED)
#define MPI_THREAD_SERIALIZED (2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_THREAD_MULTIPLE
#if !defined(MPI_THREAD_MULTIPLE)
#define MPI_THREAD_MULTIPLE (3)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INIT_THREAD
#if !defined(MPI_Init_thread)
#define MPI_Init_thread(a1,a2,a3,a4) PyMPI_UNAVAILABLE("MPI_INIT_THREAD",a1,a2,a3,a4)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_QUERY_THREAD
#if !defined(MPI_Query_thread)
#define MPI_Query_thread(a1) PyMPI_UNAVAILABLE("MPI_QUERY_THREAD",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_IS_THREAD_MAIN
#if !defined(MPI_Is_thread_main)
#define MPI_Is_thread_main(a1) PyMPI_UNAVAILABLE("MPI_IS_THREAD_MAIN",a1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_VERSION
#if !defined(MPI_VERSION)
#define MPI_VERSION (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_SUBVERSION
#if !defined(MPI_SUBVERSION)
#define MPI_SUBVERSION (0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GET_VERSION
#if !defined(MPI_Get_version)
#define MPI_Get_version(a1,a2) PyMPI_UNAVAILABLE("MPI_GET_VERSION",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_MAX_PROCESSOR_NAME
#if !defined(MPI_MAX_PROCESSOR_NAME)
#define MPI_MAX_PROCESSOR_NAME (1)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GET_PROCESSOR_NAME
#if !defined(MPI_Get_processor_name)
#define MPI_Get_processor_name(a1,a2) PyMPI_UNAVAILABLE("MPI_GET_PROCESSOR_NAME",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WTIME
#if !defined(MPI_Wtime)
#define MPI_Wtime() PyMPI_UNAVAILABLE("MPI_WTIME")
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WTICK
#if !defined(MPI_Wtick)
#define MPI_Wtick() PyMPI_UNAVAILABLE("MPI_WTICK")
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FINT
#if !defined(MPI_Fint)
typedef int PyMPI_MPI_Fint;
#define MPI_Fint PyMPI_MPI_Fint
#endif
#endif

#ifdef PyMPI_MISSING_MPI_F_STATUS_IGNORE
#if !defined(MPI_F_STATUS_IGNORE)
#define MPI_F_STATUS_IGNORE ((MPI_Fint*)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_F_STATUSES_IGNORE
#if !defined(MPI_F_STATUSES_IGNORE)
#define MPI_F_STATUSES_IGNORE ((MPI_Fint*)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_STATUS_C2F
#if !defined(MPI_Status_c2f)
#define MPI_Status_c2f(a1,a2) PyMPI_UNAVAILABLE("MPI_STATUS_C2F",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_STATUS_F2C
#if !defined(MPI_Status_f2c)
#define MPI_Status_f2c(a1,a2) PyMPI_UNAVAILABLE("MPI_STATUS_F2C",a1,a2)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_C2F
#if !defined(MPI_Type_c2f)
#define MPI_Type_c2f(a1) ((MPI_Fint)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_REQUEST_C2F
#if !defined(MPI_Request_c2f)
#define MPI_Request_c2f(a1) ((MPI_Fint)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_OP_C2F
#if !defined(MPI_Op_c2f)
#define MPI_Op_c2f(a1) ((MPI_Fint)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INFO_C2F
#if !defined(MPI_Info_c2f)
#define MPI_Info_c2f(a1) ((MPI_Fint)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GROUP_C2F
#if !defined(MPI_Group_c2f)
#define MPI_Group_c2f(a1) ((MPI_Fint)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_C2F
#if !defined(MPI_Comm_c2f)
#define MPI_Comm_c2f(a1) ((MPI_Fint)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_C2F
#if !defined(MPI_Win_c2f)
#define MPI_Win_c2f(a1) ((MPI_Fint)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_C2F
#if !defined(MPI_File_c2f)
#define MPI_File_c2f(a1) ((MPI_Fint)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERRHANDLER_C2F
#if !defined(MPI_Errhandler_c2f)
#define MPI_Errhandler_c2f(a1) ((MPI_Fint)0)
#endif
#endif

#ifdef PyMPI_MISSING_MPI_TYPE_F2C
#if !defined(MPI_Type_f2c)
#define MPI_Type_f2c(a1) MPI_DATATYPE_NULL
#endif
#endif

#ifdef PyMPI_MISSING_MPI_REQUEST_F2C
#if !defined(MPI_Request_f2c)
#define MPI_Request_f2c(a1) MPI_REQUEST_NULL
#endif
#endif

#ifdef PyMPI_MISSING_MPI_OP_F2C
#if !defined(MPI_Op_f2c)
#define MPI_Op_f2c(a1) MPI_OP_NULL
#endif
#endif

#ifdef PyMPI_MISSING_MPI_INFO_F2C
#if !defined(MPI_Info_f2c)
#define MPI_Info_f2c(a1) MPI_INFO_NULL
#endif
#endif

#ifdef PyMPI_MISSING_MPI_GROUP_F2C
#if !defined(MPI_Group_f2c)
#define MPI_Group_f2c(a1) MPI_GROUP_NULL
#endif
#endif

#ifdef PyMPI_MISSING_MPI_COMM_F2C
#if !defined(MPI_Comm_f2c)
#define MPI_Comm_f2c(a1) MPI_COMM_NULL
#endif
#endif

#ifdef PyMPI_MISSING_MPI_WIN_F2C
#if !defined(MPI_Win_f2c)
#define MPI_Win_f2c(a1) MPI_WIN_NULL
#endif
#endif

#ifdef PyMPI_MISSING_MPI_FILE_F2C
#if !defined(MPI_File_f2c)
#define MPI_File_f2c(a1) MPI_FILE_NULL
#endif
#endif

#ifdef PyMPI_MISSING_MPI_ERRHANDLER_F2C
#if !defined(MPI_Errhandler_f2c)
#define MPI_Errhandler_f2c(a1) MPI_ERRHANDLER_NULL
#endif
#endif

#endif /* !PyMPI_MISSING_H */
