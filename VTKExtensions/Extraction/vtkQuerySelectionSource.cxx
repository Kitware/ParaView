/*=========================================================================

  Program:   ParaView
  Module:    vtkQuerySelectionSource.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQuerySelectionSource.h"

#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"

#include <sstream>
#include <vector>
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkQuerySelectionSource);
vtkCxxSetObjectMacro(vtkQuerySelectionSource, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkQuerySelectionSource::vtkQuerySelectionSource()
  : Controller(nullptr)
  , QueryString(nullptr)
  , ElementType(vtkDataObject::CELL)
  , AssemblyName(nullptr)
  , Selectors()
  , AMRLevel(-1)
  , AMRIndex(-1)
  , ProcessID(-1)
  , NumberOfLayers(0)
  , Inverse(false)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkQuerySelectionSource::~vtkQuerySelectionSource()
{
  this->SetController(nullptr);
  this->SetAssemblyName(nullptr);
}

//----------------------------------------------------------------------------
void vtkQuerySelectionSource::AddSelector(const char* selector)
{
  if (selector)
  {
    this->Selectors.push_back(selector);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkQuerySelectionSource::ClearSelectors()
{
  if (!this->Selectors.empty())
  {
    this->Selectors.clear();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkQuerySelectionSource::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkQuerySelectionSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  auto output = vtkSelection::GetData(outputVector, 0);
  auto selNode = vtkSelectionNode::New();
  output->AddNode(selNode);
  selNode->FastDelete();

  const int rank = this->Controller ? this->Controller->GetLocalProcessId() : -1;
  if (this->ProcessID != -1 && this->Controller != nullptr &&
    this->Controller->GetLocalProcessId() != this->ProcessID)
  {
    // empty selection on this rank since.
    return 1;
  }

  auto properties = selNode->GetProperties();

  // Add qualifiers.
  for (const auto& selector : this->Selectors)
  {
    properties->Append(vtkSelectionNode::SELECTORS(), selector.c_str());
  }
  if (this->AssemblyName != nullptr)
  {
    properties->Set(vtkSelectionNode::ASSEMBLY_NAME(), this->AssemblyName);
  }

  if (this->AMRLevel != -1)
  {
    properties->Set(vtkSelectionNode::HIERARCHICAL_LEVEL(), this->AMRLevel);
  }

  if (this->AMRIndex != -1)
  {
    properties->Set(vtkSelectionNode::HIERARCHICAL_INDEX(), this->AMRIndex);
  }

  properties->Set(vtkSelectionNode::FIELD_TYPE(),
    vtkSelectionNode::ConvertAttributeTypeToSelectionField(this->ElementType));
  properties->Set(vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::QUERY);
  properties->Set(vtkSelectionNode::INVERSE(), this->Inverse ? 1 : 0);
  properties->Set(vtkSelectionNode::CONNECTED_LAYERS(), this->NumberOfLayers);
  selNode->SetQueryString(this->QueryString);
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkQuerySelectionSource::GetUserFriendlyText()
{
  return this->GetQueryString();
}

//----------------------------------------------------------------------------
void vtkQuerySelectionSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
