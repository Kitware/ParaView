/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericIOUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericIOUtilities -- Utility functions for GenericIO readers
//
// .SECTION Description
// This file provides some common utility functions that are used in the
// implementation of GenericIO readers.

#ifndef VTKGENERICIOUTILITIES_H_
#define VTKGENERICIOUTILITIES_H_

// VTK includes
#include "vtkDataArray.h"
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkMultiProcessController.h"

// GenericIO includes
#include "GenericIOMPIReader.h"
#include "GenericIOPosixReader.h"
#include "GenericIOReader.h"

// C/C++ includes
#include <algorithm>
#include <cassert>
#include <string>

// MPI
#include <mpi.h>

namespace vtkGenericIOUtilities {

//==============================================================================
// Description:
// Trims leading whitespace from a string.
static std::string &ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(),
      std::not1(std::ptr_fun<int, int>(std::isspace))));
  return s;
}

//==============================================================================
// Description:
// Trims trailing whitespace from a string.
static std::string &rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
      std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
  return s;
}

//==============================================================================
// Description:
// Trims leading & trailing whitespace from a string.
static std::string &trim(std::string &s) {
  return ltrim(rtrim(s));
}

//==============================================================================
// Description:
// Returns the corresponding MPI communicator for the multi-process
// controller used by this instance.
static MPI_Comm GetMPICommunicator(vtkMultiProcessController *mpc)
{
  assert("pre: null multiprocess controller!" && (mpc != NULL) );
  MPI_Comm comm = MPI_COMM_NULL;

  // STEP 0: Get the communicator from the controller
  vtkCommunicator *vtkComm =  mpc->GetCommunicator();
  assert("pre: VTK communicator is NULL" && (vtkComm != NULL) );

  // STEP 1: Safe downcast to an vtkMPICommunicator
  vtkMPICommunicator* vtkMPIComm = vtkMPICommunicator::SafeDownCast(vtkComm);
  assert("pre: MPI communicator is NULL" && (vtkMPIComm != NULL) );

  // STEP 2: Get the opaque VTK MPI communicator
  vtkMPICommunicatorOpaqueComm* mpiComm = vtkMPIComm->GetMPIComm();
  assert("pre: Opaque MPI communicator is NULL" && (mpiComm != NULL) );

  // STEP 3: Finally, get the MPI comm
  comm = *(mpiComm->GetHandle());

  return( comm );
}

//==============================================================================
// Description:
// This method parses the data in the rawbuffer and reads it into a vtkDataArray
// that can be attached as vtkPointData to a vtkDataSet, in this case, a
// vtkUnstructuredGrid that consists of the particles.
static vtkDataArray* GetVtkDataArray(
      std::string name, int type, void* rawBuffer, int N)
{
  assert("pre: cannot read from null buffer!" && (rawBuffer != NULL) );
  vtkDataArray *dataArray = NULL;
  size_t dataSize = 0;

  switch( type )
    {
    case gio::GENERIC_IO_INT32_TYPE:
     dataArray = vtkDataArray::CreateDataArray(VTK_INT);
     dataSize  = sizeof(int32_t);
     break;
    case gio::GENERIC_IO_INT64_TYPE:
     dataArray = vtkDataArray::CreateDataArray(VTK_LONG_LONG);
     // i.e., don't run this on windows
     assert(sizeof(long long)==sizeof(int64_t));
     dataSize = sizeof(int64_t);
     break;
    case gio::GENERIC_IO_UINT32_TYPE:
     dataArray = vtkDataArray::CreateDataArray(VTK_UNSIGNED_INT);
     dataSize  = sizeof(uint32_t);
     break;
    case gio::GENERIC_IO_UINT64_TYPE:
     dataArray = vtkDataArray::CreateDataArray(VTK_UNSIGNED_LONG_LONG);
     // i.e., don't run this on windows
     assert(sizeof(unsigned long long) == sizeof(uint64_t) );
     dataSize = sizeof(uint64_t);
     break;
    case gio::GENERIC_IO_DOUBLE_TYPE:
     dataArray = vtkDataArray::CreateDataArray(VTK_DOUBLE);
     dataSize  = sizeof(double);
     break;
    case gio::GENERIC_IO_FLOAT_TYPE:
     dataArray = vtkDataArray::CreateDataArray(VTK_FLOAT);
     dataSize  = sizeof(float);
     break;
    default:
     dataSize = 0;
     return NULL;
    } // END switch

  assert("pre: null data array!" && (dataArray != NULL) );

  dataArray->SetNumberOfComponents(1);
  dataArray->SetNumberOfTuples( N );
  dataArray->SetName(name.c_str());
  void *dataBuffer = dataArray->GetVoidPointer(0);
  assert("pre: encountered NULL data buffer!" && (dataBuffer != NULL) );
  memcpy(dataBuffer,rawBuffer,N*dataSize);
  return( dataArray );
}

//==============================================================================
// Description:
// This method accesses the user-supplied buffer at the given index and
// returns the data as a double. It is intended as a convenience method
// to allow the user to access the data in an agnostic-type fashion.
static double GetDoubleFromRawBuffer(
      const int type, void* buffer, vtkIdType buffer_idx)
{
  assert("pre: cannot read from null buffer!" && (buffer != NULL) );

  double dataItem = 0.0;

  switch( type )
    {
   case gio::GENERIC_IO_INT32_TYPE:
     {
     int32_t *dataPtr = static_cast<int32_t*>(buffer);
     dataItem = static_cast<double>(dataPtr[buffer_idx]);
     }
     break;
   case gio::GENERIC_IO_INT64_TYPE:
     {
     int64_t *dataPtr = static_cast<int64_t*>(buffer);
     dataItem = static_cast<double>(dataPtr[buffer_idx]);
     }
     break;
   case gio::GENERIC_IO_UINT32_TYPE:
     {
     uint32_t *dataPtr = static_cast<uint32_t*>(buffer);
     dataItem = static_cast<double>(dataPtr[buffer_idx]);
     }
     break;
   case gio::GENERIC_IO_UINT64_TYPE:
     {
     uint64_t *dataPtr = static_cast<uint64_t*>(buffer);
     dataItem = static_cast<double>(dataPtr[buffer_idx]);
     }
     break;
   case gio::GENERIC_IO_DOUBLE_TYPE:
     {
     double *dataPtr = static_cast<double*>(buffer);
     dataItem = dataPtr[buffer_idx];
     }
     break;
   case gio::GENERIC_IO_FLOAT_TYPE:
     {
     float *dataPtr = static_cast<float*>(buffer);
     dataItem = static_cast<double>(dataPtr[buffer_idx]);
     }
     break;
   default:
     assert("pre: Undefined GENERIC IO type: " && true);
    } // END switch

  return( dataItem );
}

//==============================================================================
// Description:
// This method constructs and returns the underlying GenericIO reader.
static gio::GenericIOReader* GetReader(
    MPI_Comm comm, bool posix, int distribution, std::string fileName)
{
  gio::GenericIOReader *reader = NULL;
  if( posix )
    {
    reader = new gio::GenericIOPosixReader();
    }
  else
    {
    reader = new gio::GenericIOMPIReader();
    }
  assert( "pre: reader is NULL!" && (reader != NULL) );

  reader->SetCommunicator(comm);
  reader->SetBlockAssignmentStrategy(distribution);
  reader->SetFileName(fileName);
  return( reader );
}

//==============================================================================
// Description:
// This method constructs and returns the underlying GenericIO reader.
static void SafeDeleteString(char* str)
{
  if(str != NULL)
    {
    delete [] str;
    str = NULL;
    }
}

}



#endif /* VTKGENERICIOUTILITIES_H_ */
