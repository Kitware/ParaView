/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDepthSortPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkDepthSortPainter
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#include "vtkDepthSortPainter.h"

#include "vtkObjectFactory.h"
#include "vtkIdTypeArray.h"
#include "vtkDataSet.h"
#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkFloatArray.h"
#include "vtkCell.h"
#include "vtkMath.h"
#include "vtkSortDataArray.h"
#include "vtkPoints.h"
#include "vtkRenderer.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkTexture.h"
#include "vtkProperty.h"
#include "vtkDepthSortPolyData.h"
#include "vtkScalarsToColors.h"

#include <vtkstd/vector>
#include <vtkstd/algorithm>
#include <functional>

#include <cmath>
#include "vtkImageData.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkDepthSortPainter)
//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkDepthSortPainter,DepthSortPolyData,vtkDepthSortPolyData)
;
vtkCxxSetObjectMacro(vtkDepthSortPainter,OutputData,vtkDataObject)
;

vtkDepthSortPainter::vtkDepthSortPainter()
{
  this->DepthSortEnableMode = ENABLE_SORT_IF_NO_DEPTH_PEELING;
  this->CachedIsTextureSemiTranslucent = 1;
  this->CachedIsColorSemiTranslucent = 1;
  this->DepthSortPolyData = vtkDepthSortPolyData::New();
  this->OutputData = NULL;
}
//-----------------------------------------------------------------------------
vtkDepthSortPainter::~vtkDepthSortPainter()
{
  this->SetDepthSortPolyData(NULL);
  this->SetOutputData(NULL);
}
//-----------------------------------------------------------------------------
void vtkDepthSortPainter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkDepthSortPainter::PrepareForRendering(vtkRenderer* renderer,
    vtkActor* actor)
{
  // first set the DepthSortPolyData ivars
  if (this->DepthSortPolyData != NULL)
    {
    this->DepthSortPolyData->SetCamera(renderer->GetActiveCamera());
    this->DepthSortPolyData->SetProp3D(actor);
    this->DepthSortPolyData->SetDirectionToBackToFront();
    }

  // check if we need to update
  if (this->GetMTime() < this->SortTime && this->DepthSortPolyData->GetMTime()
      < this->SortTime && this->GetInput()->GetMTime() < this->SortTime)
    {
    return;
    }

  // update the OutputData, initialize it with a shallow copy of the input
  this->SetOutputData(NULL);
  vtkDataObject * input = this->GetInput();
  if (!input)
    return;

  vtkDataObject* output = input->NewInstance();
  output->ShallowCopy(input);
  this->SetOutputData(output);
  output->Delete();

  if (this->DepthSortPolyData != NULL && this->NeedSorting(renderer, actor))
    {
    if (input->IsA("vtkCompositeDataSet"))
      {
      vtkCompositeDataSet* cdInput = vtkCompositeDataSet::SafeDownCast(input);
      vtkCompositeDataSet* cdOutput = vtkCompositeDataSet::SafeDownCast(
          this->OutputData);
      vtkCompositeDataIterator* iter = cdInput->NewIterator();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
        vtkDataSet* pdInput = vtkDataSet::SafeDownCast(
            iter->GetCurrentDataObject());
        vtkDataSet* pdOutput = vtkDataSet::SafeDownCast(cdOutput->GetDataSet(
            iter));
        if (pdInput && pdOutput)
          {
          this->Sort(pdOutput, pdInput, renderer, actor);
          }
        }

      iter->Delete();
      }
    else
      {
      this->Sort(vtkDataSet::SafeDownCast(this->OutputData),
          vtkDataSet::SafeDownCast(input), renderer, actor);
      }
    this->SortTime.Modified();
    }
}

void vtkDepthSortPainter::Sort(vtkDataSet* output,
    vtkDataSet* input,
    vtkRenderer* vtkNotUsed(renderer),
    vtkActor* vtkNotUsed(actor))
{
  this->DepthSortPolyData->SetInput(input);

  this->DepthSortPolyData->Update();

  vtkPolyData* polyData = this->DepthSortPolyData->GetOutput();

  output->ShallowCopy(polyData);
}

int vtkDepthSortPainter::NeedSorting(vtkRenderer* renderer, vtkActor* actor)
{
  if (!actor || !renderer)
    return false;

  if (this->GetDepthSortEnableMode() == ENABLE_SORT_NEVER)
    return false;

  if (this->GetDepthSortEnableMode() == ENABLE_SORT_IF_NO_DEPTH_PEELING
      && renderer->GetUseDepthPeeling())
    return false;

  if (actor->GetProperty()->GetOpacity() < 1)
    return true;

  // if the color array has an alpha component, return true.
  // rem : this alpha component can come from the vtkTwoScalarsToColors painter,
  // and thus cannot be simply deduced from the opacity and lut.
  vtkUnsignedCharArray* colors = NULL;
  vtkPolyData* input = vtkPolyData::SafeDownCast(this->GetInput());
  if (input)
    {
    colors = vtkUnsignedCharArray::SafeDownCast(
        input->GetPointData()->GetScalars());
    if (!colors)
      {
      colors = vtkUnsignedCharArray::SafeDownCast(
          input->GetCellData()->GetScalars());
      }
    if (!colors)
      {
      colors = vtkUnsignedCharArray::SafeDownCast(
          input->GetFieldData()->GetArray("Color"));
      }
    if (colors && this->IsColorSemiTranslucent(colors))
      return true;
    }

  // if the texture is either fully opaque or fully transparent, return false
  if (actor->GetTexture() != NULL && !this->IsTextureSemiTranslucent(
      actor->GetTexture()))
    return false;

  return actor->HasTranslucentPolygonalGeometry();
}

int vtkDepthSortPainter::IsTextureSemiTranslucent(vtkTexture* tex)
{
  if (tex == NULL)
    {
    return -1; //undetermined
    }
  if (tex == this->CachedTexture && this->CachedIsTextureSemiTranslucentTime
      > tex->GetMTime() && this->CachedIsTextureSemiTranslucentTime
      > this->GetMTime())
    {
    return this->CachedIsTextureSemiTranslucent;
    }
  this->CachedIsTextureSemiTranslucent = 1;
  this->CachedTexture = tex;
  this->CachedIsTextureSemiTranslucentTime.Modified();

  if (!tex->GetMapColorScalarsThroughLookupTable() && tex->GetInput())
    {
    vtkImageData* image = tex->GetInput();
    vtkUnsignedCharArray* data = vtkUnsignedCharArray::SafeDownCast(
        image->GetPointData()->GetScalars());
    if (data)
      {
      int ncomp = data->GetNumberOfComponents();
      if (ncomp % 2 == 0)
        {
        int partiallyTranslucent = false;
        unsigned char* ptr = data->GetPointer(0);
        for (vtkIdType i = 0; i < data->GetNumberOfTuples(); i++)
          {
          unsigned char alpha = ptr[i * ncomp + ncomp - 1];
          // the texture is not translucent if it is either
          // fully transparent (alpha=0) or fully opaque (alpha = 255)
          if (alpha != 0 && alpha != 255)
            {
            partiallyTranslucent = true;
            break;
            }
          }
        if (!partiallyTranslucent)
          {
          this->CachedIsTextureSemiTranslucent = 0;
          return 0;
          }
        else
          {
          return 1;
          }
        }
      else
        {
        return 1;
        }
      }
    else //if(data)
      {
      this->CachedIsTextureSemiTranslucent = -1;
      return -1; //undetermined
      }
    }
  else
    {
    vtkScalarsToColors* lut = tex->GetLookupTable();
    if (lut && lut->IsOpaque())
      {
      this->CachedIsTextureSemiTranslucent = 0;
      return 0;
      }
    else
      {
      this->CachedIsTextureSemiTranslucent = 1;
      return 1;
      }
    }
/*
  this->CachedIsTextureSemiTranslucent = -1;
  return -1;
*/
}

int vtkDepthSortPainter::IsColorSemiTranslucent(vtkUnsignedCharArray* color)
{
  if (color == this->CachedColors && color->GetMTime()
      < this->CachedIsColorSemiTranslucentTime && this->GetMTime()
      < this->CachedIsColorSemiTranslucentTime)
    {
    return this->CachedIsColorSemiTranslucent;
    }
  this->CachedColors = color;
  this->CachedIsColorSemiTranslucentTime.Modified();
  if (color == NULL)
    {
    this->CachedIsColorSemiTranslucent = -1;
    return -1;
    }

  int ncomp = color->GetNumberOfComponents();
  vtkIdType ntuples = color->GetNumberOfTuples();

  if (ncomp % 2 != 0)
    {
    this->CachedIsColorSemiTranslucent = 0;
    return 0;
    }
  vtkIdType i;
  unsigned char* values = color->GetPointer(0);
  for (i = 0; i < ntuples; i++)
    {
    if (values[ncomp - 1] != 0 || values[ncomp - 1] != 255)
      {
      this->CachedIsColorSemiTranslucent = 1;
      return 1;
      }
    values += ncomp;
    }
  this->CachedIsColorSemiTranslucent = 0;
  return 0;
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkDepthSortPainter::GetOutput()
{
  return vtkDataObject::SafeDownCast(this->OutputData);
}
