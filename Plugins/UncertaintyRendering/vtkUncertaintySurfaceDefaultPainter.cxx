/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUncertaintySurfaceDefaultPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkUncertaintySurfaceDefaultPainter.h"

#include "vtkClipPlanesPainter.h"
#include "vtkObjectFactory.h"
#include "vtkUncertaintySurfacePainter.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkUncertaintySurfaceDefaultPainter)

  //----------------------------------------------------------------------------
  vtkCxxSetObjectMacro(
    vtkUncertaintySurfaceDefaultPainter, UncertaintySurfacePainter, vtkUncertaintySurfacePainter)

  //----------------------------------------------------------------------------
  vtkUncertaintySurfaceDefaultPainter::vtkUncertaintySurfaceDefaultPainter()
{
  this->UncertaintySurfacePainter = vtkUncertaintySurfacePainter::New();
}

//----------------------------------------------------------------------------
vtkUncertaintySurfaceDefaultPainter::~vtkUncertaintySurfaceDefaultPainter()
{
  this->SetUncertaintySurfacePainter(0);
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfaceDefaultPainter::BuildPainterChain()
{
  this->Superclass::BuildPainterChain();

  // Now insert the UncertaintySurfacePainter after the clip planes painter.
  vtkPainter* prevPainter = this->GetClipPlanesPainter();
  this->UncertaintySurfacePainter->SetDelegatePainter(prevPainter->GetDelegatePainter());
  prevPainter->SetDelegatePainter(this->UncertaintySurfacePainter);
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfaceDefaultPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UncertaintySurfacePainter: " << this->UncertaintySurfacePainter << endl;
}
