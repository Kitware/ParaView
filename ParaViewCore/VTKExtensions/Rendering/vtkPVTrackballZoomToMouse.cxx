/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballZoomToMouse.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTrackballZoomToMouse.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkPVInteractorStyle.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkPVTrackballZoomToMouse);

//-------------------------------------------------------------------------
vtkPVTrackballZoomToMouse::vtkPVTrackballZoomToMouse()
{
  this->ZoomPosition[0] = 0;
  this->ZoomPosition[1] = 0;
}

//-------------------------------------------------------------------------
vtkPVTrackballZoomToMouse::~vtkPVTrackballZoomToMouse()
{
}

//-------------------------------------------------------------------------
void vtkPVTrackballZoomToMouse::OnButtonDown(
  int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi)
{
  this->Superclass::OnButtonDown(x, y, ren, rwi);
  rwi->GetEventPosition(this->ZoomPosition);
}

//-------------------------------------------------------------------------
void vtkPVTrackballZoomToMouse::OnMouseMove(
  int vtkNotUsed(x), int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi)
{
  double dy = rwi->GetLastEventPosition()[1] - y;
  double k = dy * this->ZoomScale;
  vtkPVInteractorStyle::DollyToPosition((1.0 - k), this->ZoomPosition, ren);
  rwi->Render();
}

//-------------------------------------------------------------------------
void vtkPVTrackballZoomToMouse::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ZoomPosition: " << this->ZoomPosition[0] << " " << this->ZoomPosition[1] << endl;
}
