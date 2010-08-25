

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPostFilterExecutive.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPostFilterExecutive - Executive supporting post filters.
// .SECTION Description
// vtkPVPostFilterExecutive is an executive that supports the creation of
// post filters for the following uses cases:
// Provide the ability to automatically use a vector component as a scalar
// input property.
//
//  Interpolate cell centered data to point data, and the inverse if needed
// by the filter.
// .SECTION See also
//  vtkCompositeDataPipeline

#ifndef __vtkPVPostFilterExecutive_h
#define __vtkPVPostFilterExecutive_h

#include "vtkCompositeDataPipeline.h"

class VTK_EXPORT vtkPVPostFilterExecutive : public vtkCompositeDataPipeline
{
public:
  static vtkPVPostFilterExecutive* New();
  vtkTypeMacro(vtkPVPostFilterExecutive,vtkCompositeDataPipeline);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the data object stored with the DATA_OBJECT() in the
  // input port
  vtkDataObject* GetCompositeInputData(
    int port, int index, vtkInformationVector **inInfoVec);

protected:
  vtkPVPostFilterExecutive();
  ~vtkPVPostFilterExecutive();

  // Overriden to always return true
  virtual int NeedToExecuteData(int outputPort,
                                vtkInformationVector** inInfoVec,
                                vtkInformationVector* outInfoVec);
private:
  vtkPVPostFilterExecutive(const vtkPVPostFilterExecutive&);  // Not implemented.
  void operator=(const vtkPVPostFilterExecutive&);  // Not implemented.
};

#endif
