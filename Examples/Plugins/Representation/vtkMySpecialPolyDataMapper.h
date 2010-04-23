/*=========================================================================

  Program:   ParaView
  Module:    vtkMySpecialPolyDataMapper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMySpecialPolyDataMapper - dummy special polydata mapper.
// .SECTION Description
// This is a place holder for a poly data mapper. This example simply uses the
// standard polydata mapper

#ifndef __vtkMySpecialPolyDataMapper_h
#define __vtkMySpecialPolyDataMapper_h

#include "vtkPainterPolyDataMapper.h"

class VTK_EXPORT vtkMySpecialPolyDataMapper : public vtkPainterPolyDataMapper
{
public:
  static vtkMySpecialPolyDataMapper* New();
  vtkTypeMacro(vtkMySpecialPolyDataMapper, vtkPainterPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkMySpecialPolyDataMapper();
  ~vtkMySpecialPolyDataMapper();

private:
  vtkMySpecialPolyDataMapper(const vtkMySpecialPolyDataMapper&); // Not implemented
  void operator=(const vtkMySpecialPolyDataMapper&); // Not implemented
//ETX
};

#endif

