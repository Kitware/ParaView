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

#include "vtkSMTooltipSelectionPipeline.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkCoordinate.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataMover.h"
#include "vtkPVDataUtilities.h"
#include "vtkPVExtractSelection.h"
#include "vtkPVRenderView.h"
#include "vtkPVSelectionSource.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkProcessModule.h"
#include "vtkReductionFilter.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMInputProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSelectionNode.h"

#include <sstream>

vtkStandardNewMacro(vtkSMTooltipSelectionPipeline);

//----------------------------------------------------------------------------
vtkSMTooltipSelectionPipeline::vtkSMTooltipSelectionPipeline()
  : DataMover(nullptr)
{
  this->PreviousSelectionId = 0;
  this->SelectionFound = false;
  this->TooltipEnabled = true;
}

//----------------------------------------------------------------------------
vtkSMTooltipSelectionPipeline::~vtkSMTooltipSelectionPipeline()
{
  this->ClearCache();
}

//----------------------------------------------------------------------------
void vtkSMTooltipSelectionPipeline::ClearCache()
{
  this->DataMover = nullptr;
  this->Superclass::ClearCache();
}

//----------------------------------------------------------------------------
void vtkSMTooltipSelectionPipeline::Hide(vtkSMRenderViewProxy* view)
{
  this->Superclass::Hide(view);
  this->SelectionFound = false;
  this->TooltipEnabled = true;
}

//----------------------------------------------------------------------------
void vtkSMTooltipSelectionPipeline::Show(
  vtkSMSourceProxy* representation, vtkSMSourceProxy* selection, vtkSMRenderViewProxy* view)
{
  this->Superclass::Show(representation, selection, view);

  vtkIdType currSelectionId;
  if (this->GetCurrentSelectionId(view, currSelectionId))
  {
    this->SelectionFound = true;
    if (currSelectionId != this->PreviousSelectionId)
    {
      this->PreviousSelectionId = currSelectionId;
      this->TooltipEnabled = true;
    }
  }
  else
  {
    this->PreviousSelectionId = 0;
    this->TooltipEnabled = true;
  }
}

//-----------------------------------------------------------------------------
bool vtkSMTooltipSelectionPipeline::GetCurrentSelectionId(
  vtkSMRenderViewProxy* view, vtkIdType& selId)
{
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(view->GetClientSideObject());
  vtkSelection* selection = rv->GetLastSelection();
  if (!selection || selection->GetNumberOfNodes() != 1)
  {
    return false;
  }

  vtkSelectionNode* selectionNode = selection->GetNode(0);
  if (!selectionNode)
  {
    return false;
  }

  vtkIdTypeArray* selectionArray = vtkIdTypeArray::SafeDownCast(selectionNode->GetSelectionList());
  if (!selectionArray || selectionArray->GetNumberOfTuples() != 1)
  {
    return false;
  }

  selId = selectionArray->GetValue(0);
  return true;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkSMTooltipSelectionPipeline::ConnectPVMoveSelectionToClient(
  vtkSMSourceProxy* source, unsigned int sourceOutputPort)
{
  auto pxm = source->GetSessionProxyManager();

  if (this->DataMover == nullptr)
  {
    this->DataMover = vtk::TakeSmartPointer(pxm->NewProxy("misc", "DataMover"));
    if (this->DataMover == nullptr)
    {
      vtkErrorMacro("Failed to create required proxy 'DataMover'");
      return nullptr;
    }
  }

  vtkSMPropertyHelper(this->DataMover, "Producer").Set(source);
  vtkSMPropertyHelper(this->DataMover, "PortNumber").Set(static_cast<int>(sourceOutputPort));
  vtkSMPropertyHelper(this->DataMover, "SkipEmptyDataSets").Set(1);
  this->DataMover->UpdateVTKObjects();
  this->DataMover->InvokeCommand("Execute");

  auto dataMover = vtkPVDataMover::SafeDownCast(this->DataMover->GetClientSideObject());
  if (dataMover->GetNumberOfDataSets() >= 1)
  {
    return dataMover->GetDataSetAtIndex(0);
  }

  return nullptr;
}

#ifndef VTK_LEGACY_REMOVE
//----------------------------------------------------------------------------
bool vtkSMTooltipSelectionPipeline::GetTooltipInfo(
  int association, double pos[2], std::string& tooltipText)
{
  VTK_LEGACY_BODY("vtkSMTooltipSelectionPipeline::GetTooltipInfo", "ParaView 5.9");
  pos[0] = 0.0;
  pos[1] = 0.0;
  return this->GetTooltipInfo(association, tooltipText);
}
#endif

//----------------------------------------------------------------------------
bool vtkSMTooltipSelectionPipeline::GetTooltipInfo(int association, std::string& tooltipText)
{
  vtkSMSourceProxy* extractSource = this->ExtractInteractiveSelection;
  unsigned int extractOutputPort = extractSource->GetOutputPort((unsigned int)0)->GetPortIndex();
  vtkDataObject* dataObject =
    this->ConnectPVMoveSelectionToClient(extractSource, extractOutputPort);

  bool compositeFound;
  std::string compositeName;
  vtkDataSet* ds = this->FindDataSet(dataObject, compositeFound, compositeName);
  if (!ds)
  {
    return false;
  }

  std::ostringstream tooltipTextStream;

  tooltipTextStream << "<p style='white-space:pre'>";

  // name of the filter which generated the selected dataset
  if (this->PreviousRepresentation)
  {
    vtkSMPropertyHelper representationHelper(this->PreviousRepresentation, "Input", true);
    vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(representationHelper.GetAsProxy());
    vtkSMSessionProxyManager* proxyManager = source->GetSessionProxyManager();
    tooltipTextStream << "<b>" << proxyManager->GetProxyName("sources", source) << "</b>";
  }

  // composite name
  if (compositeFound)
  {
    tooltipTextStream << "\nBlock: " << compositeName;
  }

  vtkFieldData* fieldData = nullptr;
  vtkDataArray* originalIds = nullptr;
  double point[3];
  if (association == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    // point index
    vtkPointData* pointData = ds->GetPointData();
    fieldData = pointData;
    originalIds = pointData->GetArray("vtkOriginalPointIds");
    if (originalIds)
    {
      tooltipTextStream << "\nId: " << originalIds->GetTuple1(0);
    }

    // point coords
    ds->GetPoint(0, point);
    tooltipTextStream << "\nCoords: (" << point[0] << ", " << point[1] << ", " << point[2] << ")";
  }
  else if (association == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    // cell index
    vtkCellData* cellData = ds->GetCellData();
    fieldData = cellData;
    originalIds = cellData->GetArray("vtkOriginalCellIds");
    if (originalIds)
    {
      tooltipTextStream << "\nId: " << originalIds->GetTuple1(0);
    }
    // cell type? cell points?
    if (ds->GetNumberOfCells() > 0)
    {
      vtkCell* cell = ds->GetCell(0);
      tooltipTextStream << "\nType: "
                        << vtkSMCoreUtilities::GetStringForCellType(cell->GetCellType());
    }
  }

  if (fieldData)
  {
    // point attributes
    vtkIdType nbArrays = fieldData->GetNumberOfArrays();
    for (vtkIdType i_arr = 0; i_arr < nbArrays; i_arr++)
    {
      vtkDataArray* array = fieldData->GetArray(i_arr);
      if (!array || originalIds == array)
      {
        continue;
      }
      tooltipTextStream << "\n" << array->GetName() << ": ";
      if (array->GetNumberOfComponents() > 1)
      {
        tooltipTextStream << "(";
      }
      vtkIdType nbComps = array->GetNumberOfComponents();
      for (vtkIdType i_comp = 0; i_comp < nbComps; i_comp++)
      {
        tooltipTextStream << array->GetTuple(0)[i_comp];
        if (i_comp + 1 < nbComps)
        {
          tooltipTextStream << ", ";
        }
      }
      if (array->GetNumberOfComponents() > 1)
      {
        tooltipTextStream << ")";
      }
    }
  }

  tooltipTextStream << "</p>";

  tooltipText = tooltipTextStream.str();

  this->TooltipEnabled = false;
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMTooltipSelectionPipeline::CanDisplayTooltip(bool& showTooltip)
{
  showTooltip = false;
  if (!this->PreviousRepresentation || !this->ExtractInteractiveSelection || !this->PreviousView)
  {
    return false;
  }

  if (!this->SelectionFound)
  {
    showTooltip = false;
    return true;
  }

  if (!this->TooltipEnabled)
  {
    return false;
  }

  showTooltip = true;
  return true;
}

//----------------------------------------------------------------------------
vtkSMTooltipSelectionPipeline* vtkSMTooltipSelectionPipeline::GetInstance()
{
  static vtkSmartPointer<vtkSMTooltipSelectionPipeline> Instance;
  if (Instance.GetPointer() == nullptr)
  {
    vtkSMTooltipSelectionPipeline* pipeline = vtkSMTooltipSelectionPipeline::New();
    Instance = pipeline;
    pipeline->FastDelete();
  }

  return Instance;
}

//----------------------------------------------------------------------------
void vtkSMTooltipSelectionPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  (void)os;
  (void)indent;
}

//----------------------------------------------------------------------------
vtkDataSet* vtkSMTooltipSelectionPipeline::FindDataSet(
  vtkDataObject* dataObject, bool& compositeFound, std::string& compositeName)
{
  auto cd = vtkCompositeDataSet::SafeDownCast(dataObject);
  compositeFound = cd != nullptr;
  if (!compositeFound)
  {
    return vtkDataSet::SafeDownCast(dataObject);
  }

  vtkPVDataUtilities::AssignNamesToBlocks(cd);
  auto datasets = vtkCompositeDataSet::GetDataSets(cd);
  if (!datasets.empty())
  {
    auto ds = datasets.front();
    compositeName = vtkPVDataUtilities::GetAssignedNameForBlock(ds);
    return ds;
  }

  return nullptr;
}
