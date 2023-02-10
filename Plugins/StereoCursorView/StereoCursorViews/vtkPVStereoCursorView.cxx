/*=========================================================================

  Program:   ParaView
  Module:    vtkPVStereoCursorView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVStereoCursorView.h"

#include "vtk3DCursorWidget.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkWidgetRepresentation.h"

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
