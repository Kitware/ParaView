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

#include "vtkCompositePolyDataMapper2.h"
#include "vtkObjectFactory.h"
#include "vtkUncertaintySurfacePainter.h"
#include "vtkUncertaintySurfaceDefaultPainter.h"
#include "vtkDataObject.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkUncertaintySurfaceRepresentation)

//----------------------------------------------------------------------------
vtkUncertaintySurfaceRepresentation::vtkUncertaintySurfaceRepresentation()
{
  this->Painter = vtkUncertaintySurfacePainter::New();

  // setup default painter
  vtkUncertaintySurfaceDefaultPainter *defaultPainter =
    vtkUncertaintySurfaceDefaultPainter::New();
  defaultPainter->SetUncertaintySurfacePainter(this->Painter);
  vtkCompositePolyDataMapper2* compositeMapper =
    vtkCompositePolyDataMapper2::SafeDownCast(this->Mapper);
  defaultPainter->SetDelegatePainter(
    compositeMapper->GetPainter()->GetDelegatePainter());
  compositeMapper->SetPainter(defaultPainter);
  defaultPainter->Delete();
}

//----------------------------------------------------------------------------
vtkUncertaintySurfaceRepresentation::~vtkUncertaintySurfaceRepresentation()
{
  this->Painter->Delete();
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfaceRepresentation::PrintSelf(ostream& os,
                                                    vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfaceRepresentation::SetUncertaintyArray(const char *name)
{
  this->Painter->SetUncertaintyArrayName(name);
  this->Modified();
}

//----------------------------------------------------------------------------
const char* vtkUncertaintySurfaceRepresentation::GetUncertaintyArray() const
{
  return this->Painter->GetUncertaintyArrayName();
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfaceRepresentation::SetUncertaintyTransferFunction(vtkPiecewiseFunction *function)
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
void vtkUncertaintySurfaceRepresentation::UpdateColoringParameters()
{
  this->Superclass::UpdateColoringParameters();

  // always map and interpolate scalars for uncertainty surface
  this->SetMapScalars(1);
  this->SetInterpolateScalarsBeforeMapping(1);
}
