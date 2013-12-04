/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __MPIRawArrayIO_hxx
#define __MPIRawArrayIO_hxx

#include "CartesianExtent.h"
#include "SQMacros.h"
#include "postream.h"

#ifdef SQTK_WITHOUT_MPI
#define MPI_FLOAT 0
#define MPI_DOUBLE 0
#define MPI_CHAR 0
#define MPI_UNSIGNED_CHAR 0
#define MPI_SHORT 0
#define MPI_UNSIGNED_SHORT 0
#define MPI_INT 0
#define MPI_UNSIGNED_INT 0
#define MPI_UNSIGNED 0
#define MPI_LONG 0
#define MPI_UNSIGNED_LONG 0
#define MPI_LONG_LONG 0
#define MPI_UNSIGNED_LONG_LONG 0
typedef void * MPI_Status;
typedef void * MPI_Datatype;
typedef void * MPI_Info;
typedef void * MPI_File;
typedef void * MPI_Comm;
#else
#include "SQWriteStringsWarningSupression.h"
#include "SQMPICHWarningSupression.h"
#include <mpi.h>
#endif

//=============================================================================
template<typename T> class DataTraits;

//=============================================================================
template <>
class DataTraits<float>
{
public:
  static MPI_Datatype Type(){
    return MPI_FLOAT;
    }
};

//=============================================================================
template <>
class DataTraits<double>
{
public:
  static MPI_Datatype Type(){
    return MPI_DOUBLE;
    }
};

//=============================================================================
template <>
class DataTraits<short int>
{
public:
  static MPI_Datatype Type(){
    return MPI_SHORT;
    }
};

//=============================================================================
template <>
class DataTraits<unsigned short int>
{
public:
  static MPI_Datatype Type(){
    return MPI_UNSIGNED_SHORT;
    }
};

//=============================================================================
template <>
class DataTraits<int>
{
public:
  static MPI_Datatype Type(){
    return MPI_INT;
    }
};

//=============================================================================
template <>
class DataTraits<unsigned int>
{
public:
  static MPI_Datatype Type(){
    return MPI_UNSIGNED;
    }
};

//=============================================================================
template <>
class DataTraits<long>
{
public:
  static MPI_Datatype Type(){
    return MPI_LONG;
    }
};

//=============================================================================
template <>
class DataTraits<unsigned long>
{
public:
  static MPI_Datatype Type(){
    return MPI_UNSIGNED_LONG;
    }
};

//=============================================================================
template <>
class DataTraits<long long>
{
public:
  static MPI_Datatype Type(){
    return MPI_LONG_LONG;
    }
};

//=============================================================================
template <>
class DataTraits<unsigned long long>
{
public:
  static MPI_Datatype Type(){
    return MPI_UNSIGNED_LONG_LONG;
    }
};

//=============================================================================
template <>
class DataTraits<signed char>
{
public:
  static MPI_Datatype Type(){
    return MPI_CHAR;
    }
};

//=============================================================================
template <>
class DataTraits<char>
{
public:
  static MPI_Datatype Type(){
    return MPI_CHAR;
    }
};

//=============================================================================
template <>
class DataTraits<unsigned char>
{
public:
  static MPI_Datatype Type(){
    return MPI_UNSIGNED_CHAR;
    }
};

//*****************************************************************************
template<typename T>
void CreateCartesianView(
      const CartesianExtent &domain,
      const CartesianExtent &decomp,
      MPI_Datatype &view)
{
  #ifdef SQTK_WITHOUT_MPI
  (void)domain;
  (void)decomp;
  (void)view;
  #else
  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    sqErrorMacro(
      std::cerr,
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    return;
    }

  int iErr;

  MPI_Datatype nativeType=DataTraits<T>::Type();

  int domainDims[3];
  domain.Size(domainDims);
  int domainStart[3];
  domain.GetStartIndex(domainStart);

  int decompDims[3];
  decomp.Size(decompDims);
  int decompStart[3];
  decomp.GetStartIndex(decompStart,domainStart);

  unsigned long nCells=static_cast<unsigned long>(decomp.Size());

  // use a contiguous type when possible.
  if (domain==decomp)
    {
    iErr=MPI_Type_contiguous((int)nCells,nativeType,&view);
    if (iErr)
      {
      sqErrorMacro(pCerr(),"MPI_Type_contiguous failed.");
      }
    }
  else
    {
    iErr=MPI_Type_create_subarray(
        3,
        domainDims,
        decompDims,
        decompStart,
        MPI_ORDER_FORTRAN,
        nativeType,
        &view);
    if (iErr)
      {
      sqErrorMacro(pCerr(),"MPI_Type_create_subarray failed.");
      }
    }
  iErr=MPI_Type_commit(&view);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_commit failed.");
    }
  #endif
}

//*****************************************************************************
template<typename T>
void CreateCartesianView(
      const CartesianExtent &domain,
      const CartesianExtent &decomp,
      int nComps,
      MPI_Datatype &view)
{
  #ifdef SQTK_WITHOUT_MPI
  (void)domain;
  (void)decomp;
  (void)nComps;
  (void)view;
  #else
  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    sqErrorMacro(
      std::cerr,
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    return;
    }

  int iErr;

  MPI_Datatype nativeType;
  iErr=MPI_Type_contiguous(nComps,DataTraits<T>::Type(),&nativeType);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_contiguous failed.");
    }

  int domainDims[3];
  domain.Size(domainDims);
  int domainStart[3];
  domain.GetStartIndex(domainStart);

  int decompDims[3];
  decomp.Size(decompDims);
  int decompStart[3];
  decomp.GetStartIndex(decompStart,domainStart);

  unsigned long long nCells=decomp.Size();

  // use a contiguous type when possible.
  if (domain==decomp)
    {
    iErr=MPI_Type_contiguous((int)nCells,nativeType,&view);
    if (iErr)
      {
      sqErrorMacro(pCerr(),"MPI_Type_contiguous failed.");
      }
    }
  else
    {
    iErr=MPI_Type_create_subarray(
        3,
        domainDims,
        decompDims,
        decompStart,
        MPI_ORDER_FORTRAN,
        nativeType,
        &view);
    if (iErr)
      {
      sqErrorMacro(pCerr(),"MPI_Type_create_subarray failed.");
      }
    }
  iErr=MPI_Type_commit(&view);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_commit failed.");
    }

  MPI_Type_free(&nativeType);
  #endif
}

//*****************************************************************************
template<typename T>
int WriteDataArray(
        const char *fileName,          // File name to write.
        MPI_Comm comm,                 // MPI communicator handle
        MPI_Info hints,                // MPI file hints
        const CartesianExtent &domain, // entire region, dataset extents
        const CartesianExtent &decomp, // region to be wrote, block extents
        T *data)                       // pointer to a buffer to write to disk.
{
  #ifdef SQTK_WITHOUT_MPI
  (void)fileName;
  (void)comm;
  (void)hints;
  (void)domain;
  (void)decomp;
  (void)data;
  #else
  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    sqErrorMacro(
      std::cerr,
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    return 0;
    }

  int iErr;
  const int eStrLen=2048;
  char eStr[eStrLen]={'\0'};
  // Open the file
  MPI_File file;
  iErr=MPI_File_open(
      comm,
      const_cast<char *>(fileName),
      MPI_MODE_RDONLY,
      MPI_INFO_NULL,
      &file);
  if (iErr!=MPI_SUCCESS)
    {
    MPI_Error_string(iErr,eStr,const_cast<int *>(&eStrLen));
    sqErrorMacro(pCerr(),
        << "Error opeing file: " << fileName << std::endl
        << eStr);
    return 0;
    }

  // Locate our data.
  int domainDims[3];
  domain.Size(domainDims);
  int decompDims[3];
  decomp.Size(decompDims);
  int decompStart[3];
  decomp.GetStartIndex(decompStart);

  unsigned long long nCells=decomp.Size();

  // file view
  MPI_Datatype nativeType=DataTraits<T>::Type();
  MPI_Datatype fileView;
  iErr=MPI_Type_create_subarray(
      3,
      domainDims,
      decompDims,
      decompStart,
      MPI_ORDER_FORTRAN,
      nativeType,
      &fileView);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_create_subarray failed.");
    }
  iErr=MPI_Type_commit(&fileView);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_commit failed.");
    }
  iErr=MPI_File_set_view(
      file,
      0,
      nativeType,
      fileView,
      "native",
      hints);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_File_set_view failed.");
    }
  // memory view
  MPI_Datatype memView;
  iErr=MPI_Type_contiguous((int)nCells,nativeType,&memView);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_contiguous failed.");
    }
  iErr=MPI_Type_commit(&memView);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_commit failed.");
    }

  // Write
  MPI_Status status;
  iErr=MPI_File_write_all(file,data,1,memView,&status);
  MPI_File_close(&file);
  MPI_Type_free(&fileView);
  MPI_Type_free(&memView);
  if (iErr!=MPI_SUCCESS)
    {
    MPI_Error_string(iErr,eStr,const_cast<int *>(&eStrLen));
    sqErrorMacro(pCerr(),
        << "Error writing file: " << fileName << std::endl
        << eStr);
    return 0;
    }
  #endif
  return 1;
}

/**
NOTE: The file is a scalar, but the memory can be vector
where file elements are seperated by a constant stride.
WARNING it's very slow to read into a strided array if
data seiving is off, or seive buffer is small.
*/
//*****************************************************************************
template <typename T>
int ReadDataArray(
        const char *fileName,           // File name to read.
        MPI_Comm comm,                  // MPI communicator handle
        MPI_Info hints,                 // MPI file hints
        const CartesianExtent &domain,  // entire region, dataset extents
        const CartesianExtent &decomp,  // region to be read, block extents
        int nCompsMem,                  // stride in memory array
        int compNoMem,                  // start offset in mem array
        T *data)                        // pointer to a buffer to read into.
{
  #ifdef SQTK_WITHOUT_MPI
  (void)fileName;
  (void)comm;
  (void)hints;
  (void)domain;
  (void)decomp;
  (void)nCompsMem;
  (void)compNoMem;
  (void)data;
  #else
  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    sqErrorMacro(
      std::cerr,
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    return 0;
    }

  int iErr;
  int eStrLen=256;
  char eStr[256]={'\0'};
  // Open the file
  MPI_File file;
  iErr=MPI_File_open(
      comm,
      const_cast<char *>(fileName),
      MPI_MODE_RDONLY,
      hints,
      &file);
  if (iErr!=MPI_SUCCESS)
    {
    MPI_Error_string(iErr,eStr,const_cast<int *>(&eStrLen));
    sqErrorMacro(pCerr(),
        << "Error opeing file: " << fileName << std::endl
        << eStr);
    return 0;
    }

  // Locate our data.
  int domainDims[3];
  domain.Size(domainDims);
  int decompDims[3];
  decomp.Size(decompDims);
  int decompStart[3];
  decomp.GetStartIndex(decompStart);

  unsigned long long nCells=decomp.Size();

  // file view
  MPI_Datatype nativeType=DataTraits<T>::Type();
  MPI_Datatype fileView;
  iErr=MPI_Type_create_subarray(
      3,
      domainDims,
      decompDims,
      decompStart,
      MPI_ORDER_FORTRAN,
      nativeType,
      &fileView);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_create_subarray failed.");
    }
  iErr=MPI_Type_commit(&fileView);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_commit failed.");
    }
  iErr=MPI_File_set_view(
      file,
      0,
      nativeType,
      fileView,
      "native",
      hints);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_File_set_view failed.");
    }

  // memory view.
  MPI_Datatype memView;
  if (nCompsMem==1)
    {
    iErr=MPI_Type_contiguous((int)nCells,nativeType,&memView);
    if (iErr)
      {
      sqErrorMacro(pCerr(),"MPI_Type_contiguous failed.");
      }
    }
  else
    {
    iErr=MPI_Type_vector((int)nCells,1,nCompsMem,nativeType,&memView);
    if (iErr)
      {
      sqErrorMacro(pCerr(),"MPI_Type_vector failed.");
      }
    }
  iErr=MPI_Type_commit(&memView);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_commit failed.");
    }

  // Read
  MPI_Status status;
  iErr=MPI_File_read_all(file,data+compNoMem,1,memView,&status);
  MPI_Type_free(&fileView);
  MPI_Type_free(&memView);
  MPI_File_close(&file);
  if (iErr!=MPI_SUCCESS)
    {
    MPI_Error_string(iErr,eStr,&eStrLen);
    sqErrorMacro(pCerr(),
        << "Error reading file: " << fileName << std::endl
        << eStr);
    return 0;
    }
  #endif
  return 1;
}

/**
NOTE: The file is always a scalar, but the memory can be vector
where file elements are seperated by a constant stride.
it's very slow to write from/read into a strided array
better buffer above.
*/
//*****************************************************************************
template <typename T>
int WriteDataArray(
        MPI_File file,                 // MPI file handle
        MPI_Info hints,                // MPI file hints
        const CartesianExtent &domain, // entire region, dataset extents
        const CartesianExtent &decomp, // region to be read, block extents
        int nCompsMem,                 // stride in memory array
        int compNoMem,                 // start offset in mem array
        T *data)                       // pointer to a buffer to write from.
{
  #ifdef SQTK_WITHOUT_MPI
  (void)file;
  (void)hints;
  (void)domain;
  (void)decomp;
  (void)nCompsMem;
  (void)compNoMem;
  (void)data;
  #else
  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    sqErrorMacro(
      std::cerr,
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    return 0;
    }

  int iErr;
  int eStrLen=256;
  char eStr[256]={'\0'};

  // Locate our data.
  int domainDims[3];
  domain.Size(domainDims);
  int decompDims[3];
  decomp.Size(decompDims);
  int decompStart[3];
  decomp.GetStartIndex(decompStart);

  unsigned long long nCells=decomp.Size();

  // file view
  MPI_Datatype nativeType=DataTraits<T>::Type();
  MPI_Datatype fileView;
  iErr=MPI_Type_create_subarray(3,
      domainDims,
      decompDims,
      decompStart,
      MPI_ORDER_FORTRAN,
      nativeType,
      &fileView);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_create_subarray failed.");
    }
  iErr=MPI_Type_commit(&fileView);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_commit failed.");
    }
  iErr=MPI_File_set_view(
      file,
      0,
      nativeType,
      fileView,
      "native",
      hints);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_File_set_view failed.");
    }

  // memory view.
  MPI_Datatype memView;
  if (nCompsMem==1)
    {
    iErr=MPI_Type_contiguous((int)nCells,nativeType,&memView);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_contiguous failed.");
    }
    }
  else
    {
    iErr=MPI_Type_vector((int)nCells,1,nCompsMem,nativeType,&memView);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_vector failed.");
    }
    }
  MPI_Type_commit(&memView);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_commit failed.");
    }

  // write
  MPI_Status status;
  iErr=MPI_File_write_all(file,data+compNoMem,1,memView,&status);
  MPI_Type_free(&fileView);
  MPI_Type_free(&memView);
  if (iErr!=MPI_SUCCESS)
    {
    MPI_Error_string(iErr,eStr,&eStrLen);
    sqErrorMacro(pCerr(),
        << "Error writing file." << std::endl
        << eStr);
    return 0;
    }
  #endif
  return 1;
}

//*****************************************************************************
template <typename T>
int ReadDataArray(
        MPI_File file,                 // MPI file handle
        MPI_Info hints,                // MPI file hints
        const CartesianExtent &domain, // entire region, dataset extents
        const CartesianExtent &decomp, // region to be read, block extents
        int nCompsMem,                 // stride in memory array
        int compNoMem,                 // start offset in mem array
        T *data)                       // pointer to a buffer to read into.
{
  #ifdef SQTK_WITHOUT_MPI
  (void)file;
  (void)hints;
  (void)domain;
  (void)decomp;
  (void)nCompsMem;
  (void)compNoMem;
  (void)data;
  #else
  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    sqErrorMacro(
      std::cerr,
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    return 0;
    }

  int iErr;
  int eStrLen=256;
  char eStr[256]={'\0'};

  // Locate our data.
  int domainDims[3];
  domain.Size(domainDims);
  int decompDims[3];
  decomp.Size(decompDims);
  int decompStart[3];
  decomp.GetStartIndex(decompStart);

  unsigned long long nCells=decomp.Size();

  // file view
  MPI_Datatype nativeType=DataTraits<T>::Type();
  MPI_Datatype fileView;
  iErr=MPI_Type_create_subarray(3,
      domainDims,
      decompDims,
      decompStart,
      MPI_ORDER_FORTRAN,
      nativeType,
      &fileView);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_create_subarray failed.");
    }
  iErr=MPI_Type_commit(&fileView);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_commit failed.");
    }
  iErr=MPI_File_set_view(
      file,
      0,
      nativeType,
      fileView,
      "native",
      hints);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_File_set_view failed.");
    }

  // memory view.
  MPI_Datatype memView;
  if (nCompsMem==1)
    {
    iErr=MPI_Type_contiguous((int)nCells,nativeType,&memView);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_contiguous failed.");
    }
    }
  else
    {
    iErr=MPI_Type_vector((int)nCells,1,nCompsMem,nativeType,&memView);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_vector failed.");
    }
    }
  MPI_Type_commit(&memView);
  if (iErr)
    {
    sqErrorMacro(pCerr(),"MPI_Type_commit failed.");
    }

  // Read
  MPI_Status status;
  iErr=MPI_File_read_all(file,data+compNoMem,1,memView,&status);
  MPI_Type_free(&fileView);
  MPI_Type_free(&memView);
  if (iErr!=MPI_SUCCESS)
    {
    MPI_Error_string(iErr,eStr,&eStrLen);
    sqErrorMacro(pCerr(),
        << "Error reading file." << std::endl
        << eStr);
    return 0;
    }
  #endif
  return 1;
}
/**
Read the from the region defined by the file view into the
region defined by the memory veiw.
*/
//*****************************************************************************
template <typename T>
int ReadDataArray(
        MPI_File file,                 // MPI file handle
        MPI_Info hints,                // MPI file hints
        MPI_Datatype memView,          // memory region
        MPI_Datatype fileView,         // file layout
        T *data)                       // pointer to a buffer to read into.
{
  #ifdef SQTK_WITHOUT_MPI
  (void)file;
  (void)hints;
  (void)memView;
  (void)fileView;
  (void)data;
  #else
  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    sqErrorMacro(
      std::cerr,
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    return 0;
    }

  int iErr;
  int eStrLen=256;
  char eStr[256]={'\0'};

  MPI_Datatype nativeType=DataTraits<T>::Type();
  iErr=MPI_File_set_view(
      file,
      0,
      nativeType,
      fileView,
      "native",
      hints);

  MPI_Status status;
  iErr=MPI_File_read_all(file,data,1,memView,&status);
  if (iErr!=MPI_SUCCESS)
    {
    MPI_Error_string(iErr,eStr,&eStrLen);
    sqErrorMacro(pCerr(),
        << "Error reading file." << std::endl
        << eStr);
    return 0;
    }
  #endif
  return 1;
}

#endif
