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
// Note that it's essential that the mapper can handle composite datasets. If
// your mapper cannot, then simply use an append filter internally to
// merge the blocks into a single polydata.

#ifndef vtkMySpecialPolyDataMapper_h
#define vtkMySpecialPolyDataMapper_h

#include "vtkCompositePolyDataMapper2.h"

class VTK_EXPORT vtkMySpecialPolyDataMapper : public vtkCompositePolyDataMapper2
{
public:
  static vtkMySpecialPolyDataMapper* New();
  vtkTypeMacro(vtkMySpecialPolyDataMapper, vtkCompositePolyDataMapper2);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkMySpecialPolyDataMapper();
  ~vtkMySpecialPolyDataMapper();

private:
  vtkMySpecialPolyDataMapper(const vtkMySpecialPolyDataMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMySpecialPolyDataMapper&) VTK_DELETE_FUNCTION;
};

#endif
