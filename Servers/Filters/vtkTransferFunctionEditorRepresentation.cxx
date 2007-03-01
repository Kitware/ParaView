/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransferFunctionEditorRepresentation.h"

#include "vtkImageActor.h"

vtkCxxRevisionMacro(vtkTransferFunctionEditorRepresentation, "1.2");

//----------------------------------------------------------------------------
vtkTransferFunctionEditorRepresentation::vtkTransferFunctionEditorRepresentation()
{
  this->HistogramActor = vtkImageActor::New();
  this->HistogramActor->SetPosition(0, 0, -10);

  this->HistogramVisibility = 1;
  this->ScalarBinRange[0] = 1;
  this->ScalarBinRange[1] = 0;
}

//----------------------------------------------------------------------------
vtkTransferFunctionEditorRepresentation::~vtkTransferFunctionEditorRepresentation()
{
  this->HistogramActor->Delete();
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorRepresentation::RenderOpaqueGeometry(
  vtkViewport *viewport)
{
  if (this->HistogramVisibility)
    {
    return this->HistogramActor->RenderOpaqueGeometry(viewport);
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorRepresentation::RenderTranslucentGeometry(
  vtkViewport *viewport)
{
  if (this->HistogramVisibility)
    {
    return this->HistogramActor->RenderTranslucentGeometry(viewport);
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorRepresentation::RenderOverlay(
  vtkViewport *viewport)
{
  if (this->HistogramVisibility)
    {
    return this->HistogramActor->RenderOverlay(viewport);
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DisplaySize: " << this->DisplaySize[0] << " "
     << this->DisplaySize[1] << endl;
}
