// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

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
#include "vtkStringArray.h"

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
//----------------------------------------------------------------------------
bool vtkSMTooltipSelectionPipeline::GetTooltipInfo(int association, std::string& tooltipText)
{
  std::string unused;
  return this->GetTooltipInfo(association, tooltipText, unused);
}

//----------------------------------------------------------------------------
bool vtkSMTooltipSelectionPipeline::GetTooltipInfo(
  int association, std::string& formatedTooltipText, std::string& plainTooltipText)
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

  // name of the filter which generated the selected dataset
  std::string proxyName;
  if (this->PreviousRepresentation)
  {
    vtkSMPropertyHelper representationHelper(this->PreviousRepresentation, "Input", true);
    vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(representationHelper.GetAsProxy());
    vtkSMSessionProxyManager* proxyManager = source->GetSessionProxyManager();
    const char* name = proxyManager->GetProxyName("sources", source);
    if (name)
    {
      proxyName = name;
    }
  }

  std::ostringstream tooltipTextStream;

  // Composite dataset name
  if (compositeFound)
  {
    tooltipTextStream << "\n<b>  Block: " << compositeName << "</b>";
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
      tooltipTextStream << "\n  Id: " << originalIds->GetTuple1(0);
    }

    // point coords
    ds->GetPoint(0, point);
    tooltipTextStream << "\n  Coords: (" << point[0] << ", " << point[1] << ", " << point[2] << ")";
  }
  else if (association == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    // cell index
    vtkCellData* cellData = ds->GetCellData();
    fieldData = cellData;
    originalIds = cellData->GetArray("vtkOriginalCellIds");
    if (originalIds)
    {
      tooltipTextStream << "\n  Id: " << originalIds->GetTuple1(0);
    }
    // cell type? cell points?
    if (ds->GetNumberOfCells() > 0)
    {
      vtkCell* cell = ds->GetCell(0);
      tooltipTextStream << "\n  Type: "
                        << vtkSMCoreUtilities::GetStringForCellType(cell->GetCellType());
    }
  }

  if (fieldData)
  {
    // point or cell attributes
    vtkIdType nbArrays = fieldData->GetNumberOfArrays();
    for (vtkIdType i_arr = 0; i_arr < nbArrays; i_arr++)
    {
      vtkDataArray* array = fieldData->GetArray(i_arr);
      if (!array || originalIds == array)
      {
        continue;
      }
      tooltipTextStream << "\n  " << array->GetName() << ": ";
      vtkIdType nbComps = array->GetNumberOfComponents();

      if (nbComps > 1)
      {
        tooltipTextStream << "(";
      }
      for (vtkIdType i_comp = 0; i_comp < nbComps; i_comp++)
      {
        tooltipTextStream << array->GetTuple(0)[i_comp];
        if (i_comp + 1 < nbComps)
        {
          tooltipTextStream << ", ";
        }
      }
      if (nbComps > 1)
      {
        tooltipTextStream << ")";
      }
    }
  }

  // Add field data arrays with one tuple
  fieldData = ds->GetFieldData();

  if (fieldData)
  {
    // Add separation line if needed
    if (fieldData->GetNumberOfArrays() > 0)
    {
      tooltipTextStream << "<hr>";
    }

    for (vtkIdType i_arr = 0; i_arr < fieldData->GetNumberOfArrays(); i_arr++)
    {
      vtkAbstractArray* array = fieldData->GetAbstractArray(i_arr);
      if (!array || array->GetNumberOfTuples() != 1)
      {
        continue;
      }

      tooltipTextStream << "\n  " << array->GetName() << ": ";

      // String arrays are not data arrays
      vtkStringArray* strArray = vtkStringArray::SafeDownCast(array);
      vtkDataArray* dataArray = vtkDataArray::SafeDownCast(array);

      if (strArray)
      {
        tooltipTextStream << strArray->GetValue(0);
      }
      else if (dataArray)
      {
        vtkIdType nbComps = array->GetNumberOfComponents();
        vtkIdType maxDisplayedComp = 9;

        if (nbComps > 1)
        {
          tooltipTextStream << "(";
        }

        for (vtkIdType i_comp = 0; i_comp < std::min(nbComps, maxDisplayedComp); i_comp++)
        {
          tooltipTextStream << dataArray->GetTuple(0)[i_comp];

          if (i_comp + 1 < nbComps && i_comp < maxDisplayedComp)
          {
            tooltipTextStream << ", ";
          }
        }

        if (nbComps > 1)
        {
          if (nbComps > maxDisplayedComp)
          {
            tooltipTextStream << "...";
          }
          tooltipTextStream << ")";
        }
      }
    }
  }

  formatedTooltipText =
    "<p style='white-space:pre'><b>" + proxyName + "</b>" + tooltipTextStream.str() + "</p>";
  plainTooltipText = proxyName + tooltipTextStream.str();

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
