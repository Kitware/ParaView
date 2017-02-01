/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUncertaintySurfaceDefaultPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkUncertaintySurfaceDefaultPainter_h
#define vtkUncertaintySurfaceDefaultPainter_h

#include "vtkDefaultPainter.h"

class vtkUncertaintySurfacePainter;

class VTK_EXPORT vtkUncertaintySurfaceDefaultPainter : public vtkDefaultPainter
{
public:
  static vtkUncertaintySurfaceDefaultPainter* New();
  vtkTypeMacro(vtkUncertaintySurfaceDefaultPainter, vtkDefaultPainter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // Description:
  // Get/Set the uncertainty surface painter.
  void SetUncertaintySurfacePainter(vtkUncertaintySurfacePainter*);
  vtkGetObjectMacro(UncertaintySurfacePainter, vtkUncertaintySurfacePainter)

    protected : vtkUncertaintySurfaceDefaultPainter();
  ~vtkUncertaintySurfaceDefaultPainter();

  // Description:
  // Setups the the painter chain.
  void BuildPainterChain();

private:
  vtkUncertaintySurfacePainter* UncertaintySurfacePainter;
};

#endif // vtkUncertaintySurfaceDefaultPainter_h
