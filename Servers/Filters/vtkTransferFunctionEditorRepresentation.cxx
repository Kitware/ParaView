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

#include "vtkActor2D.h"
#include "vtkImageMapper.h"

vtkCxxRevisionMacro(vtkTransferFunctionEditorRepresentation, "1.1");

//----------------------------------------------------------------------------
vtkTransferFunctionEditorRepresentation::vtkTransferFunctionEditorRepresentation()
{
  this->HistogramMapper = vtkImageMapper::New();
  this->HistogramMapper->SetColorWindow(256);
  this->HistogramMapper->SetColorLevel(128);
  this->HistogramActor = vtkActor2D::New();
  this->HistogramActor->SetMapper(this->HistogramMapper);
  this->HistogramActor->SetPosition(0, 0);
  this->HistogramActor->SetPosition2(1, 1);

  this->HistogramVisibility = 1;
  this->ScalarBinRange[0] = 1;
  this->ScalarBinRange[1] = 0;
}

//----------------------------------------------------------------------------
vtkTransferFunctionEditorRepresentation::~vtkTransferFunctionEditorRepresentation()
{
  this->HistogramMapper->Delete();
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
