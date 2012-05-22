/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointSpriteRepresentation.h"

#include "vtk1DGaussianTransferFunction.h"
#include "vtk1DLookupTableTransferFunction.h"
#include "vtk1DLookupTableTransferFunction.h"
#include "vtk1DTransferFunctionChooser.h"
#include "vtk1DTransferFunctionFilter.h"
#include "vtkCellPointsFilter.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkDepthSortPainter.h"
#include "vtkImageSpriteSource.h"
#include "vtkObjectFactory.h"
#include "vtkPointSpriteDefaultPainter.h"
#include "vtkPointSpriteProperty.h"
#include "vtkPVLODActor.h"
#include "vtkTexture.h"
#include "vtkTwoScalarsToColorsPainter.h"
#include "vtkPVCacheKeeper.h"

vtkStandardNewMacro(vtkPointSpriteRepresentation);
vtkCxxSetObjectMacro(vtkPointSpriteRepresentation, TextureInternal, vtkTexture);
//----------------------------------------------------------------------------
vtkPointSpriteRepresentation::vtkPointSpriteRepresentation()
{
  // Replace the GeometryFilter created by the superclas.
  this->GeometryFilter->Delete();
  vtkCellPointsFilter* cpf = vtkCellPointsFilter::New();
  this->GeometryFilter = cpf;
  cpf->SetVertexCells(1);

  this->ArrayToRadiusFilter = vtk1DTransferFunctionFilter::New();
  this->ArrayToOpacityFilter = vtk1DTransferFunctionFilter::New();
  this->PSProperty = vtkPointSpriteProperty::New();

  // replace the superclass's property.
  this->Property->Delete();
  this->Property = this->PSProperty;
  this->Actor->SetProperty(this->Property);

  this->PointSpriteDefaultPainter = vtkPointSpriteDefaultPainter::New();
  this->LODPointSpriteDefaultPainter = vtkPointSpriteDefaultPainter::New();
  this->DepthSortPainter = vtkDepthSortPainter::New();
  this->LODDepthSortPainter = vtkDepthSortPainter::New();
  this->ScalarsToColorsPainter = vtkTwoScalarsToColorsPainter::New();
  this->LODScalarsToColorsPainter = vtkTwoScalarsToColorsPainter::New();
  this->RadiusTransferFunctionChooser = vtk1DTransferFunctionChooser::New();
  this->OpacityTransferFunctionChooser = vtk1DTransferFunctionChooser::New();
  this->RadiusTableTransferFunction = vtk1DLookupTableTransferFunction::New();
  this->OpacityTableTransferFunction = vtk1DLookupTableTransferFunction::New();
  this->RadiusGaussianTransferFunction = vtk1DGaussianTransferFunction::New();
  this->OpacityGaussianTransferFunction = vtk1DGaussianTransferFunction::New();

  // InterpolateScalarsBeforeMapping creates colors sending them to the GPU.
  // if set, a 1D texture is created, which conflicts with the points sprites
  this->SetInterpolateScalarsBeforeMapping(0);

  // Ensure that the new geometry filter is connected into the pipeline.
  this->MultiBlockMaker->SetInputConnection(this->GeometryFilter->GetOutputPort());

  this->ArrayToRadiusFilter->SetEnabled(0);
  this->ArrayToRadiusFilter->SetConcatenateOutputNameWithInput(0);
  this->ArrayToRadiusFilter->SetOutputArrayName("ArrayMappedToRadius");
  this->ArrayToRadiusFilter->SetForceSameTypeAsInputArray(0);
  this->ArrayToRadiusFilter->SetOutputArrayType(VTK_FLOAT);
  this->ArrayToRadiusFilter->SetTransferFunction(
    this->RadiusTransferFunctionChooser);
  this->RadiusTransferFunctionChooser->SetGaussianTransferFunction(
    this->RadiusGaussianTransferFunction);
  this->RadiusTransferFunctionChooser->SetLookupTableTransferFunction(
    this->RadiusTableTransferFunction);

  this->ArrayToOpacityFilter->SetEnabled(0);
  this->ArrayToOpacityFilter->SetConcatenateOutputNameWithInput(0);
  this->ArrayToOpacityFilter->SetOutputArrayName("ArrayMappedToOpacity");
  this->ArrayToOpacityFilter->SetForceSameTypeAsInputArray(0);
  this->ArrayToOpacityFilter->SetOutputArrayType(VTK_FLOAT);
  this->ArrayToOpacityFilter->SetTransferFunction(
    this->OpacityTransferFunctionChooser);
  this->OpacityTransferFunctionChooser->SetGaussianTransferFunction(
    this->OpacityGaussianTransferFunction);
  this->OpacityTransferFunctionChooser->SetLookupTableTransferFunction(
    this->OpacityTableTransferFunction);

  this->PSProperty->SetRadiusArrayName("ArrayMappedToRadius");
  this->ScalarsToColorsPainter->SetOpacityArrayName("ArrayMappedToOpacity");
  this->LODScalarsToColorsPainter->SetOpacityArrayName("ArrayMappedToOpacity");

  this->ScalarsToColorsPainter->SetEnableOpacity(0);
  this->LODScalarsToColorsPainter->SetEnableOpacity(0);

  this->PointSpriteDefaultPainter->SetScalarsToColorsPainter(
    this->ScalarsToColorsPainter);
  this->PointSpriteDefaultPainter->SetDepthSortPainter(
    this->DepthSortPainter);

  this->LODPointSpriteDefaultPainter->SetScalarsToColorsPainter(
    this->LODScalarsToColorsPainter);
  this->LODPointSpriteDefaultPainter->SetDepthSortPainter(
    this->LODDepthSortPainter);

  vtkCompositePolyDataMapper2* compositeMapper =
    vtkCompositePolyDataMapper2::SafeDownCast(this->Mapper);
  this->PointSpriteDefaultPainter->SetDelegatePainter(
    compositeMapper->GetPainter()->GetDelegatePainter());
  compositeMapper->SetPainter(this->PointSpriteDefaultPainter);

  compositeMapper = vtkCompositePolyDataMapper2::SafeDownCast
    (this->LODMapper);
  this->LODPointSpriteDefaultPainter->SetDelegatePainter(
    compositeMapper->GetPainter()->GetDelegatePainter());
  compositeMapper->SetPainter(this->LODPointSpriteDefaultPainter);

  // change the pipeline setup by the superclass to insert our filters in it.
  this->ArrayToRadiusFilter->SetInputConnection(
    this->CacheKeeper->GetInputConnection(0, 0));
  this->ArrayToOpacityFilter->SetInputConnection(
    this->ArrayToRadiusFilter->GetOutputPort());
  this->CacheKeeper->SetInputConnection(this->ArrayToOpacityFilter->GetOutputPort());

  // Setup default textures.
  this->SphereTexture = vtkTexture::New();
  vtkImageSpriteSource* sphereSource = vtkImageSpriteSource::New();
  sphereSource->SetWholeExtent(0, 65, 0, 65, 0, 0);
  sphereSource->SetMaximum(255);
  sphereSource->SetStandardDeviation(0.3);
  sphereSource->SetAlphaMethod(2);
  sphereSource->SetAlphaThreshold(63);
  this->SphereTexture->SetInputConnection(sphereSource->GetOutputPort());
  this->SphereSource = sphereSource;

  this->BlurTexture = vtkTexture::New();
  vtkImageSpriteSource* blurSource = vtkImageSpriteSource::New();
  blurSource->SetWholeExtent(0, 65, 0, 65, 0, 0);
  blurSource->SetStandardDeviation(0.2);
  blurSource->SetAlphaMethod(1);
  this->BlurTexture->SetInputConnection(blurSource->GetOutputPort());
  this->BlurSource = blurSource;

  this->TextureInternal = 0;
  this->RenderMode = -1;
}

//----------------------------------------------------------------------------
vtkPointSpriteRepresentation::~vtkPointSpriteRepresentation()
{
  this->ArrayToRadiusFilter->Delete();
  this->ArrayToOpacityFilter->Delete();
  this->PointSpriteDefaultPainter->Delete();
  this->LODPointSpriteDefaultPainter->Delete();
  this->DepthSortPainter->Delete();
  this->LODDepthSortPainter->Delete();
  this->ScalarsToColorsPainter->Delete();
  this->LODScalarsToColorsPainter->Delete();
  this->RadiusTransferFunctionChooser->Delete();
  this->OpacityTransferFunctionChooser->Delete();
  this->RadiusTableTransferFunction->Delete();
  this->OpacityTableTransferFunction->Delete();
  this->RadiusGaussianTransferFunction->Delete();
  this->OpacityGaussianTransferFunction->Delete();
  this->BlurSource->Delete();
  this->BlurTexture->Delete();
  this->SphereSource->Delete();
  this->SphereTexture->Delete();

  this->SetTextureInternal(0);
}

//----------------------------------------------------------------------------
void vtkPointSpriteRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPointSpriteRepresentation::SetTexture(vtkTexture* texture)
{
  this->SetTextureInternal(texture);
  if (this->RenderMode == vtkPointSpriteProperty::TexturedSprite)
    {
    this->Superclass::SetTexture(texture);
    }
  // Don't override the this->Actor->Texture unless the user wants to use a
  // custom texture.
}

//***************************************************************************
// Forwarded to ArrayToRadiusFilter
void vtkPointSpriteRepresentation::SetRadiusTransferFunctionEnabled(int val)
{
  this->ArrayToRadiusFilter->SetEnabled(val);
  this->MarkModified();
}

void vtkPointSpriteRepresentation::SetRadiusArrayToProcess(
  int a, int b, int c, int d, const char* e)
{
  this->ArrayToRadiusFilter->SetInputArrayToProcess(a, b, c, d, e);
  this->MarkModified();
}

//***************************************************************************
// Forwarded to ArrayToOpacityFilter
void vtkPointSpriteRepresentation::SetOpacityTransferFunctionEnabled(int val)
{
  this->ArrayToOpacityFilter->SetEnabled(val);
  this->MarkModified();
}

void vtkPointSpriteRepresentation::SetOpacityArrayToProcess(
  int a, int b, int c, int d, const char* e)
{
  this->ArrayToOpacityFilter->SetInputArrayToProcess(a, b, c, d, e);
  this->MarkModified();
}

//***************************************************************************
// Forwarded to PSProperty (vtkPointSpriteProperty).
void vtkPointSpriteRepresentation::SetRenderMode(int val)
{
  this->RenderMode = val;
  if (val == vtkPointSpriteProperty::Quadrics ||
    val == vtkPointSpriteProperty::TexturedSprite ||
    val == vtkPointSpriteProperty::SimplePoint)
    {
    this->PSProperty->SetRenderMode(val);
    if (val == vtkPointSpriteProperty::TexturedSprite)
      {
      this->Actor->SetTexture(this->TextureInternal);
      }
    }
  if (val == 3) // SphereTexture
    {
    this->PSProperty->SetRenderMode(1);
    this->Actor->SetTexture(this->SphereTexture);
    }
  else if (val == 4) // BlurTexture
    {
    this->PSProperty->SetRenderMode(1);
    this->Actor->SetTexture(this->BlurTexture);
    }
}

void vtkPointSpriteRepresentation::SetRadiusMode(int val)
{
  this->PSProperty->SetRadiusMode(val);
}

void vtkPointSpriteRepresentation::SetConstantRadius(double val)
{
  this->PSProperty->SetConstantRadius(val);
}

void vtkPointSpriteRepresentation::SetRadiusRange(double val0, double val1)
{
  this->PSProperty->SetRadiusRange(val0, val1);
}

void vtkPointSpriteRepresentation::SetMaxPixelSize(double val)
{
  this->PSProperty->SetMaxPixelSize(val);
}

//***************************************************************************
// Forwarded to ScalarsToColorsPainter and LODScalarsToColorsPainter
void vtkPointSpriteRepresentation::SetEnableOpacity(int val)
{
  this->ScalarsToColorsPainter->SetEnableOpacity(val);
  this->LODScalarsToColorsPainter->SetEnableOpacity(val);
}

//***************************************************************************
// Forwarded to RadiusTransferFunctionChooser
void vtkPointSpriteRepresentation::SetRadiusTransferFunctionMode(int val)
{
  this->RadiusTransferFunctionChooser->SetTransferFunctionMode(val);
}

void vtkPointSpriteRepresentation::SetRadiusVectorComponent(int val)
{
  this->RadiusTransferFunctionChooser->SetVectorComponent(val);
}

void vtkPointSpriteRepresentation::SetRadiusScalarRange(double val0, double val1)
{
  this->RadiusTransferFunctionChooser->SetInputRange(val0, val1);
}

void vtkPointSpriteRepresentation::SetRadiusUseScalarRange(int val)
{
  this->RadiusTransferFunctionChooser->SetUseScalarRange(val);
}

//***************************************************************************
// Forwarded to OpacityTransferFunctionChooser
void vtkPointSpriteRepresentation::SetOpacityTransferFunctionMode(int val)
{
  this->OpacityTransferFunctionChooser->SetTransferFunctionMode(val);
}

void vtkPointSpriteRepresentation::SetOpacityVectorComponent(int val)
{
  this->OpacityTransferFunctionChooser->SetVectorComponent(val);
}

void vtkPointSpriteRepresentation::SetOpacityScalarRange(double val0, double val1)
{
  this->OpacityTransferFunctionChooser->SetInputRange(val0, val1);
}

void vtkPointSpriteRepresentation::SetOpacityUseScalarRange(int val)
{
  this->OpacityTransferFunctionChooser->SetUseScalarRange(val);
}

//***************************************************************************
// Forwarded to RadiusTableTransferFunction
void vtkPointSpriteRepresentation::SetRadiusTableValues(int index, double val)
{
  this->RadiusTableTransferFunction->SetTableValue(index, val);
}

void vtkPointSpriteRepresentation::SetNumberOfRadiusTableValues(int val)
{
  this->RadiusTableTransferFunction->SetNumberOfTableValues(val);
}

void vtkPointSpriteRepresentation::RemoveAllRadiusTableValues()
{
  this->RadiusTableTransferFunction->RemoveAllTableValues();
}

//***************************************************************************
// Forwarded to OpacityTableTransferFunction
void vtkPointSpriteRepresentation::SetOpacityTableValues(int index, double val)
{
  this->OpacityTableTransferFunction->SetTableValue(index, val);
}

void vtkPointSpriteRepresentation::SetNumberOfOpacityTableValues(int val)
{
  this->OpacityTableTransferFunction->SetNumberOfTableValues(val);
}

void vtkPointSpriteRepresentation::RemoveAllOpacityTableValues()
{
  this->OpacityTableTransferFunction->RemoveAllTableValues();
}

//***************************************************************************
// Forwarded to RadiusGaussianTransferFunction
void vtkPointSpriteRepresentation::SetRadiusGaussianControlPoints(
  int index, double val0, double val1, double val2, double val3, double val4)
{
  this->RadiusGaussianTransferFunction->SetGaussianControlPoint(
    index, val0, val1, val2, val3, val4);
}

void vtkPointSpriteRepresentation::SetNumberOfRadiusGaussianControlPoints(int val)
{
  this->RadiusGaussianTransferFunction->SetNumberOfGaussianControlPoints(val);
}

void vtkPointSpriteRepresentation::RemoveAllRadiusGaussianControlPoints()
{
  this->RadiusGaussianTransferFunction->RemoveAllGaussianControlPoints();
}

//***************************************************************************
// Forwarded to OpacityGaussianTransferFunction
void vtkPointSpriteRepresentation::SetOpacityGaussianControlPoints(int index,
  double val0, double val1, double val2, double val3, double val4)
{
  this->OpacityGaussianTransferFunction->SetGaussianControlPoint(index,
    val0, val1, val2, val3, val4);
}

void vtkPointSpriteRepresentation::SetNumberOfOpacityGaussianControlPoints(int val)
{
  this->OpacityGaussianTransferFunction->SetNumberOfGaussianControlPoints(val);
}

void vtkPointSpriteRepresentation::RemoveAllOpacityGaussianControlPoints()
{
  this->OpacityGaussianTransferFunction->RemoveAllGaussianControlPoints();
}
