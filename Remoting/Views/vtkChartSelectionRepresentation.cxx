/*=========================================================================

  Program:   ParaView
  Module:    vtkChartSelectionRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkChartSelectionRepresentation.h"

#include "vtkChartRepresentation.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextView.h"
#include "vtkSelection.h"

#include <assert.h>
#include <sstream>

vtkStandardNewMacro(vtkChartSelectionRepresentation);
//----------------------------------------------------------------------------
vtkChartSelectionRepresentation::vtkChartSelectionRepresentation()
{
  this->EnableServerSideRendering = false;
  this->Cache = vtkSmartPointer<vtkSelection>::New();
}

//----------------------------------------------------------------------------
vtkChartSelectionRepresentation::~vtkChartSelectionRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkChartSelectionRepresentation::SetChartRepresentation(vtkChartRepresentation* repr)
{
  this->ChartRepresentation = repr;
}

//----------------------------------------------------------------------------
void vtkChartSelectionRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
}

//----------------------------------------------------------------------------
bool vtkChartSelectionRepresentation::AddToView(vtkView* view)
{
  vtkPVContextView* pvview = vtkPVContextView::SafeDownCast(view);
  if (pvview)
  {
    this->ContextView = pvview;
    this->EnableServerSideRendering = pvview->InTileDisplayMode();
    return this->Superclass::AddToView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkChartSelectionRepresentation::RemoveFromView(vtkView* view)
{
  if (view == this->ContextView.GetPointer())
  {
    this->ContextView = NULL;
    this->EnableServerSideRendering = false;
    return this->Superclass::RemoveFromView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
int vtkChartSelectionRepresentation::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkChartSelectionRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request, vtkInformation* ininfo, vtkInformation* outinfo)
{
  if (!this->Superclass::ProcessViewRequest(request, ininfo, outinfo))
  {
    return 0;
  }

  if (request == vtkPVView::REQUEST_UPDATE())
  {
    vtkPVView::SetPiece(ininfo, this, this->Cache);
  }
  else if (request == vtkPVView::REQUEST_RENDER())
  {
    assert(this->ContextView != NULL);
    this->ContextView->SetSelection(this->ChartRepresentation,
      vtkSelection::SafeDownCast(vtkPVView::GetDeliveredPiece(ininfo, this)));
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkChartSelectionRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Cache->Initialize();

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkSelection* inputSelection = vtkSelection::GetData(inputVector[0], 0);
    assert(inputSelection);

    this->Cache->ShallowCopy(inputSelection);

    // map data-selection to view. We do this here so that the mapping logic is
    // called on the processes that have the input data (which may come in
    // handy at some point).
    if (this->ChartRepresentation->MapSelectionToView(this->Cache) == false)
    {
      this->Cache->Initialize();
    }
  }
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkChartSelectionRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ChartRepresentation: " << this->ChartRepresentation.GetPointer() << endl;
  os << indent << "EnableServerSideRendering: " << this->EnableServerSideRendering << endl;
}
