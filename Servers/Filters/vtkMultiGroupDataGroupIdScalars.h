/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiGroupDataGroupIdScalars.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiGroupDataGroupIdScalars - DEPRECATED!!!
// Will be removed in the next release. Provided for backward-compatibility
// between 3.2 and 3.4
// .SECTION Description
// vtkMultiGroupDataGroupIdScalars is mimicks the behaviour of
// vtkMultiGroupDataGroupIdScalars in ParaView 3.2. This is only for
// backward-compatibility between 3.2 and 3.4 and will be removed before the
// next release.
//
// .SECTION Original Description 
// vtkMultiGroupDataGroupIdScalars is a filter to that generates scalars 
// using multi-group data group information. For example, it will assign
// an vtkUnsignedCharArray named GroupIdScalars and of value 0 to all 
// datasets in group 0.


#ifndef __vtkMultiGroupDataGroupIdScalars_h
#define __vtkMultiGroupDataGroupIdScalars_h

#include "vtkCompositeDataSetAlgorithm.h"

class VTK_EXPORT vtkMultiGroupDataGroupIdScalars : public vtkCompositeDataSetAlgorithm
{
public:
  static vtkMultiGroupDataGroupIdScalars* New();
  vtkTypeRevisionMacro(vtkMultiGroupDataGroupIdScalars, vtkCompositeDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkMultiGroupDataGroupIdScalars();
  ~vtkMultiGroupDataGroupIdScalars();

  int RequestDataObject(vtkInformation*,
    vtkInformationVector** inputVector, vtkInformationVector* outputVector);
  int RequestData(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector);

  int ColorBlock(vtkDataObject* dObj, int group);
private:
  vtkMultiGroupDataGroupIdScalars(const vtkMultiGroupDataGroupIdScalars&); // Not implemented
  void operator=(const vtkMultiGroupDataGroupIdScalars&); // Not implemented
//ETX
};

#endif

