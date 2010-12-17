/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPieceCacheExecutive.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPieceCacheExecutive - An executive for the PieceCacheFilter
// .SECTION Description
// vtkPieceCacheExecutive is used along with the PieceCacheFilter to cache
// data pieces. The filter stores the data. The executive prevents requests
// that can be satisfied by the cache from causing upstream pipeline updates.
// .SEE ALSO
// vtkPieceCacheFilter

#ifndef __vtkPieceCacheExecutive_h
#define __vtkPieceCacheExecutive_h

#include "vtkCompositeDataPipeline.h"

class VTK_EXPORT vtkPieceCacheExecutive :
  public vtkCompositeDataPipeline
{
public:
  static vtkPieceCacheExecutive* New();
  vtkTypeMacro(vtkPieceCacheExecutive,
                       vtkCompositeDataPipeline);

protected:
  vtkPieceCacheExecutive();
  ~vtkPieceCacheExecutive();

  //overridden to short circuit upstream requests when cache has data
  virtual int NeedToExecuteData(int outputPort,
                                vtkInformationVector** inInfoVec,
                                vtkInformationVector* outInfoVec);

private:
  vtkPieceCacheExecutive(const vtkPieceCacheExecutive&);  // Not implemented.
  void operator=(const vtkPieceCacheExecutive&);  // Not implemented.
};

#endif
