// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
vtkPVTrackballZoomToMouse::~vtkPVTrackballZoomToMouse() = default;

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
  // The camera's Dolly function scales by the distance already, so we need
  // to un-scale by it here so the zoom is not too fast or slow.
  double k = dy * this->ZoomScale / ren->GetActiveCamera()->GetDistance();
  vtkPVInteractorStyle::DollyToPosition((1.0 + k), this->ZoomPosition, ren);
  rwi->Render();
}

//-------------------------------------------------------------------------
void vtkPVTrackballZoomToMouse::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ZoomPosition: " << this->ZoomPosition[0] << " " << this->ZoomPosition[1] << endl;
}
