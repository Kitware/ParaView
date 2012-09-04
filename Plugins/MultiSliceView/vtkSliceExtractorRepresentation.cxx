/*=========================================================================

  Program:   ParaView
  Module:    vtkSliceExtractorRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSliceExtractorRepresentation.h"

#include "vtkCommand.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkHardwareSelectionPolyDataPainter.h"
#include "vtkMapper.h"
#include "vtkMultiSliceRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkThreeSliceFilter.h"

#include "vtkNew.h"
#include "vtkConeSource.h"

vtkStandardNewMacro(vtkSliceExtractorRepresentation);
//----------------------------------------------------------------------------
vtkSliceExtractorRepresentation::vtkSliceExtractorRepresentation()
{
  // setup the selection mapper so that we don't need to make any selection
  // conversions after rendering.
//  vtkCompositePolyDataMapper2* mapper =
//      vtkCompositePolyDataMapper2::SafeDownCast(this->Mapper);
//  vtkHardwareSelectionPolyDataPainter* selPainter =
//      vtkHardwareSelectionPolyDataPainter::SafeDownCast(
//        mapper->GetSelectionPainter()->GetDelegatePainter());
//  selPainter->SetPointIdArrayName("-");
//  selPainter->SetCellIdArrayName("vtkSliceOriginalCellIds");

  // Make sure by default the mapper has an input
  vtkConeSource* cone = vtkConeSource::New();

//  vtkNew<vtkConeSource> cone;
//  this->Mapper->SetInputConnection(cone->GetOutputPort(0));
}

//----------------------------------------------------------------------------
vtkSliceExtractorRepresentation::~vtkSliceExtractorRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkSliceExtractorRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSliceExtractorRepresentation::SetMultiSliceRepresentation(vtkMultiSliceRepresentation* multiSliceRep, int port)
{
  //this->SetInputConnection(multiSliceRep->GetInternalVTKFilter()->GetOutputPort(port));
}
