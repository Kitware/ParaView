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
#include "vtkColorTransferFunction.h"
#include "vtkImageData.h"
#include "vtkImageMapper.h"
#include "vtkPointData.h"
#include "vtkProperty2D.h"
#include "vtkUnsignedCharArray.h"

vtkCxxRevisionMacro(vtkTransferFunctionEditorRepresentation, "1.8");

vtkCxxSetObjectMacro(vtkTransferFunctionEditorRepresentation,
                     ColorFunction, vtkColorTransferFunction);

//----------------------------------------------------------------------------
vtkTransferFunctionEditorRepresentation::vtkTransferFunctionEditorRepresentation()
{
  this->HistogramImage = vtkImageData::New();
  this->HistogramImage->SetScalarTypeToUnsignedChar();
  this->HistogramMapper = vtkImageMapper::New();
  this->HistogramMapper->SetInput(this->HistogramImage);
  this->HistogramMapper->SetColorWindow(256);
  this->HistogramMapper->SetColorLevel(128);
  this->HistogramActor = vtkActor2D::New();
  this->HistogramActor->SetMapper(this->HistogramMapper);
  this->HistogramActor->SetPosition(0, 0);
  this->HistogramActor->SetPosition2(1, 1);
  this->HistogramActor->SetLayerNumber(0);
  this->HistogramActor->GetProperty()->SetDisplayLocationToBackground();

  this->BackgroundImage = vtkImageData::New();
  this->BackgroundImage->SetScalarTypeToUnsignedChar();
  this->BackgroundMapper = vtkImageMapper::New();
  this->BackgroundMapper->SetInput(this->BackgroundImage);
  this->BackgroundMapper->SetColorWindow(256);
  this->BackgroundMapper->SetColorLevel(128);
  this->BackgroundActor = vtkActor2D::New();
  this->BackgroundActor->SetMapper(this->BackgroundMapper);
  this->BackgroundActor->SetPosition(0, 0);
  this->BackgroundActor->SetPosition2(1, 1);
  this->BackgroundActor->SetLayerNumber(1);
  this->BackgroundActor->GetProperty()->SetDisplayLocationToBackground();
  
  this->HistogramVisibility = 1;
  this->ScalarBinRange[0] = 1;
  this->ScalarBinRange[1] = 0;
  this->ShowColorFunctionInBackground = 0;
  this->ShowColorFunctionInHistogram = 0;
  this->ColorElementsByColorFunction = 1;
  this->ElementsColor[0] = this->ElementsColor[1] = this->ElementsColor[2] = 1;

  this->HistogramColor[0] = this->HistogramColor[1] = this->HistogramColor[2] =
    0.8;
  this->ColorFunction = NULL;

  this->DisplaySize[0] = this->DisplaySize[1] = 100;

  this->VisibleScalarRange[0] = 1;
  this->VisibleScalarRange[1] = 0;
}

//----------------------------------------------------------------------------
vtkTransferFunctionEditorRepresentation::~vtkTransferFunctionEditorRepresentation()
{
  this->HistogramImage->Delete();
  this->HistogramMapper->Delete();
  this->HistogramActor->Delete();
  this->SetColorFunction(NULL);
  this->BackgroundImage->Delete();
  this->BackgroundMapper->Delete();
  this->BackgroundActor->Delete();
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorRepresentation::HasTranslucentPolygonalGeometry()
{
  int ret = 0;
  if (this->HistogramVisibility)
    {
    ret |= this->HistogramActor->HasTranslucentPolygonalGeometry();
    }
  if (this->ShowColorFunctionInBackground)
    {
    ret |= this->BackgroundActor->HasTranslucentPolygonalGeometry();
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorRepresentation::RenderOverlay(
  vtkViewport *viewport)
{
  int ret = 0;
  if (this->ShowColorFunctionInBackground)
    {
    ret += this->BackgroundActor->RenderOverlay(viewport);
    }
  if (this->HistogramVisibility)
    {
    ret += this->HistogramActor->RenderOverlay(viewport);
    }

  return ret;
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation::SetDisplaySize(int x, int y)
{
  if (this->DisplaySize[0] != x || this->DisplaySize[1] != y)
    {
    this->DisplaySize[0] = x;
    this->DisplaySize[1] = y;
    this->Modified();
    }

  if (this->HistogramImage)
    {
    this->HistogramImage->Initialize();
    this->HistogramImage->SetDimensions(this->DisplaySize[0],
                                        this->DisplaySize[1], 1);
    this->HistogramImage->SetNumberOfScalarComponents(4);
    this->HistogramImage->AllocateScalars();
    vtkUnsignedCharArray *array = vtkUnsignedCharArray::SafeDownCast(
      this->HistogramImage->GetPointData()->GetScalars());
    if (array)
      {
      array->FillComponent(0, 0);
      array->FillComponent(1, 0);
      array->FillComponent(2, 0);
      array->FillComponent(3, 0);
      }
    }
  if (this->BackgroundImage)
    {
    this->BackgroundImage->Initialize();
    this->BackgroundImage->SetDimensions(this->DisplaySize[0],
                                         this->DisplaySize[1], 1);
    this->BackgroundImage->SetNumberOfScalarComponents(4);
    this->BackgroundImage->AllocateScalars();
    vtkUnsignedCharArray *array = vtkUnsignedCharArray::SafeDownCast(
      this->BackgroundImage->GetPointData()->GetScalars());
    if (array)
      {
      array->FillComponent(0, 0);
      array->FillComponent(1, 0);
      array->FillComponent(2, 0);
      array->FillComponent(3, 0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DisplaySize: " << this->DisplaySize[0] << " "
     << this->DisplaySize[1] << endl;
  os << indent << "ColorElementsByColorFunction: "
     << this->ColorElementsByColorFunction << endl;
  os << indent << "HistogramVisibility: " << this->HistogramVisibility << endl;
  os << indent << "ColorElementsByColorFunction: "
     << this->ColorElementsByColorFunction << endl;
  os << indent << "VisibleScalarRange: " << this->VisibleScalarRange[0] << " "
     << this->VisibleScalarRange[1] << endl;
}
