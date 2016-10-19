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

#include "vtkUncertaintySurfaceRepresentation.h"

#include "vtkCompositeDataSet.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUncertaintySurfaceDefaultPainter.h"
#include "vtkUncertaintySurfacePainter.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkUncertaintySurfaceRepresentation)

  //----------------------------------------------------------------------------
  vtkUncertaintySurfaceRepresentation::vtkUncertaintySurfaceRepresentation()
{
  this->Painter = vtkUncertaintySurfacePainter::New();

  // setup default painter
  vtkUncertaintySurfaceDefaultPainter* defaultPainter = vtkUncertaintySurfaceDefaultPainter::New();
  defaultPainter->SetUncertaintySurfacePainter(this->Painter);
  vtkCompositePolyDataMapper2* compositeMapper =
    vtkCompositePolyDataMapper2::SafeDownCast(this->Mapper);
  defaultPainter->SetDelegatePainter(compositeMapper->GetPainter()->GetDelegatePainter());
  compositeMapper->SetPainter(defaultPainter);
  defaultPainter->Delete();
}

//----------------------------------------------------------------------------
vtkUncertaintySurfaceRepresentation::~vtkUncertaintySurfaceRepresentation()
{
  this->Painter->Delete();
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfaceRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfaceRepresentation::SetUncertaintyArray(const char* name)
{
  this->Painter->SetUncertaintyArrayName(name);

  // update transfer function
  this->RescaleUncertaintyTransferFunctionToDataRange();

  this->Modified();
}

//----------------------------------------------------------------------------
const char* vtkUncertaintySurfaceRepresentation::GetUncertaintyArray() const
{
  return this->Painter->GetUncertaintyArrayName();
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfaceRepresentation::SetUncertaintyTransferFunction(
  vtkPiecewiseFunction* function)
{
  this->Painter->SetTransferFunction(function);
  this->Modified();
}

//----------------------------------------------------------------------------
vtkPiecewiseFunction* vtkUncertaintySurfaceRepresentation::GetUncertaintyTransferFunction() const
{
  return this->Painter->GetTransferFunction();
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfaceRepresentation::RescaleUncertaintyTransferFunctionToDataRange()
{
  const char* uncertaintyArrayName = this->GetUncertaintyArray();
  vtkPiecewiseFunction* transferFunction = this->GetUncertaintyTransferFunction();

  double range[2] = { 0.0, 1.0 };

  vtkDataObject* input = this->GetInput();
  vtkDataSet* inputDS = vtkDataSet::SafeDownCast(input);
  if (inputDS)
  {
    vtkAbstractArray* array = inputDS->GetPointData()->GetAbstractArray(uncertaintyArrayName);
    if (vtkIntArray* intArray = vtkIntArray::SafeDownCast(array))
    {
      intArray->GetRange(range);
    }
    else if (vtkFloatArray* floatArray = vtkFloatArray::SafeDownCast(array))
    {
      floatArray->GetRange(range);
    }
    else if (vtkDoubleArray* doubleArray = vtkDoubleArray::SafeDownCast(array))
    {
      doubleArray->GetRange(range);
    }
  }

  // set range
  transferFunction->RemoveAllPoints();
  transferFunction->AddPoint(range[0], 1.0, 0.5, 0.0);
  transferFunction->AddPoint(range[1], 1.0, 0.5, 0.0);
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfaceRepresentation::SetUncertaintyScaleFactor(double density)
{
  this->Painter->SetUncertaintyScaleFactor(density);
  this->Modified();
}

//----------------------------------------------------------------------------
double vtkUncertaintySurfaceRepresentation::GetUncertaintyScaleFactor() const
{
  return this->Painter->GetUncertaintyScaleFactor();
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfaceRepresentation::UpdateColoringParameters()
{
  this->Superclass::UpdateColoringParameters();

  // always map and interpolate scalars for uncertainty surface
  this->SetMapScalars(1);
  this->SetInterpolateScalarsBeforeMapping(1);
}
