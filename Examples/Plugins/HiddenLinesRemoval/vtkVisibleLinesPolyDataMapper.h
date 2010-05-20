/*=========================================================================

  Program:   ParaView
  Module:    vtkVisibleLinesPolyDataMapper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVisibleLinesPolyDataMapper
// .SECTION Description
//

#ifndef __vtkVisibleLinesPolyDataMapper_h
#define __vtkVisibleLinesPolyDataMapper_h

#include "vtkPainterPolyDataMapper.h"

class VTK_EXPORT vtkVisibleLinesPolyDataMapper : public vtkPainterPolyDataMapper
{
public:
  static vtkVisibleLinesPolyDataMapper* New();
  vtkTypeMacro(vtkVisibleLinesPolyDataMapper, vtkPainterPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkVisibleLinesPolyDataMapper();
  ~vtkVisibleLinesPolyDataMapper();

private:
  vtkVisibleLinesPolyDataMapper(const vtkVisibleLinesPolyDataMapper&); // Not implemented
  void operator=(const vtkVisibleLinesPolyDataMapper&); // Not implemented
//ETX
};

#endif

