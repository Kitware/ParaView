/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClipDataSet.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVClipDataSet - Clip filter
//
// .SECTION Description
// This is a subclass of vtkTableBasedClipDataSet that allows selection of input scalars

#ifndef __vtkPVClipDataSet_h
#define __vtkPVClipDataSet_h

#include "vtkTableBasedClipDataSet.h"

class VTK_EXPORT vtkPVClipDataSet : public vtkTableBasedClipDataSet
{
public:
  vtkTypeRevisionMacro(vtkPVClipDataSet,vtkTableBasedClipDataSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  static vtkPVClipDataSet *New();

protected:
  vtkPVClipDataSet(vtkImplicitFunction *cf=NULL);
  ~vtkPVClipDataSet();

private:
  vtkPVClipDataSet(const vtkPVClipDataSet&);  // Not implemented.
  void operator=(const vtkPVClipDataSet&);  // Not implemented.
};

#endif
