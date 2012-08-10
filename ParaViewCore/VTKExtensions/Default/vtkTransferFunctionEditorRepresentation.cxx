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

#include "vtkActor.h"
#include "vtkColorTransferFunction.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkTexture.h"
#include "vtkUnsignedCharArray.h"


vtkCxxSetObjectMacro(vtkTransferFunctionEditorRepresentation,
                     ColorFunction, vtkColorTransferFunction);

//----------------------------------------------------------------------------
vtkTransferFunctionEditorRepresentation::vtkTransferFunctionEditorRepresentation()
{
  this->HistogramImage = vtkImageData::New();
  this->HistogramImage->SetScalarType(VTK_UNSIGNED_CHAR, this->HistogramImage->GetInformation());
  this->HistogramTexture = vtkTexture::New();
  this->HistogramTexture->SetInputData(this->HistogramImage);
  this->HistogramGeometry = vtkPolyData::New();
  this->HistogramMapper = vtkPolyDataMapper::New();
  this->HistogramMapper->SetInputData(this->HistogramGeometry);
  this->HistogramActor = vtkActor::New();
  this->HistogramActor->SetTexture(this->HistogramTexture);
  this->HistogramActor->SetMapper(this->HistogramMapper);

  this->BackgroundImage = vtkPolyData::New();
  this->BackgroundMapper = vtkPolyDataMapper::New();
  this->BackgroundMapper->SetInputData(this->BackgroundImage);
  this->BackgroundMapper->InterpolateScalarsBeforeMappingOn();
  this->BackgroundActor = vtkActor::New();
  this->BackgroundActor->SetMapper(this->BackgroundMapper);
  
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

  this->BorderWidth = 0;

  this->HistogramMTime = 0;

  this->Histogram = 0;
}

//----------------------------------------------------------------------------
vtkTransferFunctionEditorRepresentation::~vtkTransferFunctionEditorRepresentation()
{
  this->HistogramImage->Delete();
  this->HistogramTexture->Delete();
  this->HistogramGeometry->Delete();
  this->HistogramMapper->Delete();
  this->HistogramActor->Delete();
  this->SetColorFunction(NULL);
  this->BackgroundImage->Delete();
  this->BackgroundMapper->Delete();
  this->BackgroundActor->Delete();
  this->SetHistogram(NULL);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation::SetHistogram(
  vtkIntArray *hist)
{
  if (this->Histogram != hist)
    {
    vtkIntArray *tmp = this->Histogram;
    this->Histogram = hist;
    if (this->Histogram != NULL)
      {
      this->Histogram->Register(this);
      this->HistogramMTime = this->Histogram->GetMTime();
      }
    if (tmp != NULL)
      {
      tmp->UnRegister(this);
      }
    }
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

  return ret;
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorRepresentation::RenderOpaqueGeometry(
  vtkViewport *viewport)
{
  int ret = 0;
  if (this->ShowColorFunctionInBackground)
    {
    ret += this->BackgroundActor->RenderOpaqueGeometry(viewport);
    }

  return ret;
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorRepresentation::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  int ret = 0;
  if (this->HistogramVisibility)
    {
    ret += this->HistogramActor->RenderTranslucentPolygonalGeometry(viewport);
    }

  return ret;
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation::ReleaseGraphicsResources(
  vtkWindow *window)
{
  if (this->BackgroundActor)
    {
    this->BackgroundActor->ReleaseGraphicsResources(window);
    }
  if (this->HistogramActor)
    {
    this->HistogramActor->ReleaseGraphicsResources(window);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation::SetDisplaySize(int x, int y)
{
  if (this->DisplaySize[0] != x || this->DisplaySize[1] != y)
    {
    this->DisplaySize[0] = x;
    this->DisplaySize[1] = y;

    if (this->HistogramImage)
      {
      this->InitializeImage(this->HistogramImage);
      this->HistogramGeometry->Initialize();
      }
    if (this->BackgroundImage)
      {
      this->BackgroundImage->Initialize();
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation::SetBorderWidth(int width)
{
  if (this->BorderWidth != width)
    {
    this->BorderWidth = width;
    if (this->DisplaySize[0] > 0 && this->DisplaySize[1] > 0)
      {
      this->InitializeImage(this->HistogramImage);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation::InitializeImage(
  vtkImageData *image)
{
  if (image)
    {
    image->Initialize();
    image->SetDimensions(
      this->DisplaySize[0] - 2*this->BorderWidth,
      this->DisplaySize[1] - 2*this->BorderWidth, 1);
    image->AllocateScalars(image->GetScalarType(), 4);
    vtkUnsignedCharArray *array = vtkUnsignedCharArray::SafeDownCast(
      image->GetPointData()->GetScalars());
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
void vtkTransferFunctionEditorRepresentation::UpdateHistogramMTime()
{
  if (this->Histogram)
    {
    this->HistogramMTime = this->Histogram->GetMTime();
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
  os << indent << "ShowColorFunctionInHistogram: "
     << this->ShowColorFunctionInHistogram << endl;
  os << indent << "HistogramColor: " 
    << this->HistogramColor[0] << ", "
    << this->HistogramColor[1] << ", "
    << this->HistogramColor[2] << endl;
}
