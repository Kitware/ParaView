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
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextView.h"
#include "vtkProcessModule.h"
#include "vtkProperty.h"
#include "vtkSelection.h"
#include "vtkSelectionDeliveryFilter.h"
#include "vtkSelectionSerializer.h"

#include <assert.h>
#include <sstream>

vtkStandardNewMacro(vtkChartSelectionRepresentation);
//----------------------------------------------------------------------------
vtkChartSelectionRepresentation::vtkChartSelectionRepresentation()
{
  this->EnableServerSideRendering = false;
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
int vtkChartSelectionRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_RENDER_SERVER)
  {
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  vtkSmartPointer<vtkSelection> localSelection;

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkSelection* inputSelection = vtkSelection::GetData(inputVector[0], 0);
    assert(inputSelection);

    vtkNew<vtkSelection> clone;
    clone->ShallowCopy(inputSelection);

    // map data-selection to view. We do this here so that the mapping logic is
    // called on the processes that have the input data (which may come in
    // handy at some point).
    if (this->ChartRepresentation->MapSelectionToView(clone.GetPointer()) == false)
    {
      clone->Initialize();
    }

    vtkNew<vtkSelectionDeliveryFilter> courier;
    courier->SetInputDataObject(clone.GetPointer());
    courier->Update();
    localSelection = vtkSelection::SafeDownCast(courier->GetOutputDataObject(0));

    if (this->EnableServerSideRendering)
    {
      // distribute the selection from the root node to all processes.
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
      int myId = pm->GetPartitionId();
      int numProcs = pm->GetNumberOfLocalPartitions();

      if (numProcs > 1)
      {
        vtkMultiProcessController* controller = pm->GetGlobalController();
        if (myId == 0)
        {
          std::ostringstream res;
          vtkSelectionSerializer::PrintXML(res, vtkIndent(), 1, clone.GetPointer());

          // Send the size of the string.
          int size = static_cast<int>(res.str().size());
          controller->Broadcast(&size, 1, 0);

          // Send the XML string.
          controller->Broadcast(const_cast<char*>(res.str().c_str()), size, 0);

          localSelection = clone.GetPointer();
        }
        else
        {
          int size = 0;
          controller->Broadcast(&size, 1, 0);
          char* xml = new char[size + 1];

          // Get the string itself.
          controller->Broadcast(xml, size, 0);
          xml[size] = 0;

          // Parse the XML.
          vtkNew<vtkSelection> sel;
          vtkSelectionSerializer::Parse(xml, sel.GetPointer());
          delete[] xml;

          localSelection = sel.GetPointer();
        }
      }
    }
  }
  else
  {
    vtkNew<vtkSelectionDeliveryFilter> courier;
    courier->Update();
    localSelection = vtkSelection::SafeDownCast(courier->GetOutputDataObject(0));
  }

  if (this->ContextView)
  {
    this->ContextView->SetSelection(this->ChartRepresentation, localSelection);
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
