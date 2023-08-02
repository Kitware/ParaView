// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVStereoCursorView.h"

#include "vtk3DCursorRepresentation.h"
#include "vtk3DCursorWidget.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

struct vtkPVStereoCursorView::vtkInternals
{
  vtkNew<vtk3DCursorWidget> Cursor;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVStereoCursorView);

//----------------------------------------------------------------------------
vtkPVStereoCursorView::vtkPVStereoCursorView()
  : Internals(new vtkPVStereoCursorView::vtkInternals())
{
}

//----------------------------------------------------------------------------
void vtkPVStereoCursorView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVStereoCursorView::SetupInteractor(vtkRenderWindowInteractor* rwi)
{
  this->Superclass::SetupInteractor(rwi);
  if (this->Interactor)
  {
    this->Internals->Cursor->SetInteractor(this->Interactor);
    this->Internals->Cursor->On();
    this->Interactor->HideCursor();
  }
}

//----------------------------------------------------------------------------
void vtkPVStereoCursorView::SetCursorSize(int size)
{
  vtkWidgetRepresentation* representation = this->Internals->Cursor->GetRepresentation();
  if (!representation)
  {
    vtkWarningMacro("Unable to retrieve the widget representation.");
    return;
  }

  representation->SetHandleSize(static_cast<double>(size));
}

//----------------------------------------------------------------------------
void vtkPVStereoCursorView::SetCursorShape(int shape)
{
  vtk3DCursorRepresentation* representation =
    vtk3DCursorRepresentation::SafeDownCast(this->Internals->Cursor->GetRepresentation());
  if (!representation)
  {
    vtkWarningMacro("Unable to retrieve the widget representation.");
    return;
  }

  representation->SetCursorShape(shape);
}
