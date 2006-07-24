/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSpecializedBoxClip.h

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
// This is a subclass of vtkClipDataSet 

#ifndef __vtkPVSpecializedBoxClip_h
#define __vtkPVSpecializedBoxClip_h

#include "vtkClipDataSet.h"

class VTK_EXPORT vtkPVSpecializedBoxClip : public vtkClipDataSet
{
public:
  vtkTypeRevisionMacro(vtkPVSpecializedBoxClip,vtkClipDataSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  static vtkPVSpecializedBoxClip *New();

protected:
  vtkPVSpecializedBoxClip(vtkImplicitFunction *cf=NULL);
  ~vtkPVSpecializedBoxClip();

private:
  vtkPVSpecializedBoxClip(const vtkPVSpecializedBoxClip&);  // Not implemented.
  void operator=(const vtkPVSpecializedBoxClip&);  // Not implemented.
};

#endif
