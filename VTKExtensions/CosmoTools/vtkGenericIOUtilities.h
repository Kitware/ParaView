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
/**
 * @class   vtkGenericIOUtilities
 *
 *
 * This file provides some common utility functions that are used in the
 * implementation of GenericIO readers.
*/

#ifndef vtkGenericIOUtilities_h
#define vtkGenericIOUtilities_h

#include "vtkType.h"

#include <algorithm>
#include <functional>
#include <string>

#include "vtk_mpi.h"

class vtkMultiProcessController;
class vtkDataArray;

namespace gio
{
class GenericIOReader;
class GenericIOWriter;
}

namespace vtkGenericIOUtilities
{

//==============================================================================
//@{
/**
 * Trims leading whitespace from a string.
 */
inline std::string& ltrim(std::string& s)
{
  s.erase(
    s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
  return s;
}
//@}

//==============================================================================
//@{
/**
 * Trims trailing whitespace from a string.
 */
inline std::string& rtrim(std::string& s)
{
  s.erase(
    std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
    s.end());
  return s;
}
//@}

//==============================================================================
/**
 * Trims leading & trailing whitespace from a string.
 */
inline std::string& trim(std::string& s)
{
  return ltrim(rtrim(s));
}

//==============================================================================
/**
 * Returns the corresponding MPI communicator for the multi-process
 * controller used by this instance.
 */
MPI_Comm GetMPICommunicator(vtkMultiProcessController* mpc);

//==============================================================================
/**
 * This method parses the data in the rawbuffer and reads it into a vtkDataArray
 * that can be attached as vtkPointData to a vtkDataSet, in this case, a
 * vtkUnstructuredGrid that consists of the particles.
 */
vtkDataArray* GetVtkDataArray(std::string name, int type, void* rawBuffer, int N);

//==============================================================================
/**
 * This method accesses the user-supplied buffer at the given index and
 * returns the data as a vtkIdType. It is intended as a convenience method
 * to allow the user to access the data in an agnostic-type fashion.
 */
vtkIdType GetIdFromRawBuffer(const int type, void* buffer, vtkIdType buffer_idx);

//==============================================================================
/**
 * This method accesses the user-supplied buffer at the given index and
 * returns the data as a double. It is intended as a convenience method
 * to allow the user to access the data in an agnostic-type fashion.
 */
double GetDoubleFromRawBuffer(const int type, void* buffer, vtkIdType buffer_idx);

//==============================================================================
/**
 * This method constructs and returns the underlying GenericIO reader.
 */
gio::GenericIOReader* GetReader(
  MPI_Comm comm, bool posix, int distribution, const std::string& fileName);

//==============================================================================
/**
 * This method constructs and returns the underlying GenericIO writer.
 * The returned writer will have its filename set already
 */
gio::GenericIOWriter* GetWriter(MPI_Comm comm, const std::string& fileName);

//==============================================================================
//@{
/**
 * If the pointer given is non-nullptr this function deletes the string and
 * set the pointer to nullptr.  This sets the pointer variable in the calling
 * function since the pointer is passed by reference.
 */
inline void SafeDeleteString(char*& str)
{
  if (str != nullptr)
  {
    delete[] str;
    str = nullptr;
  }
}
//@}
}

#endif /* vtkGenericIOUtilities_h */
// VTK-HeaderTest-Exclude: vtkGenericIOUtilities.h
