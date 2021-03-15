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
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkCoordinate.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
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
  : MoveSelectionToClient(nullptr)
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
  if (this->MoveSelectionToClient)
  {
    this->MoveSelectionToClient->Delete();
    this->MoveSelectionToClient = nullptr;
  }
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
  vtkSMSessionProxyManager* proxyManager =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

  // Reduce data
  vtkSMSourceProxy* reduceSelectionToClient =
    vtkSMSourceProxy::SafeDownCast(proxyManager->NewProxy("filters", "ReductionFilter"));
  // set input
  vtkSMInputProperty* inputProperty =
    vtkSMInputProperty::SafeDownCast(reduceSelectionToClient->GetProperty("Input"));
  inputProperty->RemoveAllProxies();
  inputProperty->AddInputConnection(source, sourceOutputPort);
  // set postGatherHelperName
  std::string postGatherHelperName;
  if (source->GetDataInformation(sourceOutputPort)->IsCompositeDataSet())
  {
    postGatherHelperName = "vtkMultiBlockDataGroupFilter";
  }
  else if (std::string(source->GetDataInformation(sourceOutputPort)->GetDataClassName()) ==
    std::string("vtkPolyData"))
  {
    postGatherHelperName = "vtkAppendPolyData";
  }
  else if (std::string(source->GetDataInformation(sourceOutputPort)->GetDataClassName()) ==
    std::string("vtkRectilinearGrid"))
  {
    postGatherHelperName = "vtkAppendRectilinearGrid";
  }
  else
  {
    postGatherHelperName = "vtkAppendFilter";
  }
  vtkSMPropertyHelper(reduceSelectionToClient, "PostGatherHelperName")
    .Set(postGatherHelperName.c_str());
  reduceSelectionToClient->UpdateVTKObjects();
  reduceSelectionToClient->UpdatePipeline();

  // Move data to client
  if (!this->MoveSelectionToClient)
  {
    this->MoveSelectionToClient =
      vtkSMSourceProxy::SafeDownCast(proxyManager->NewProxy("filters", "ClientServerMoveData"));
  }

  // set input, manually modifying VTK object so it is updated
  vtkSMPropertyHelper(this->MoveSelectionToClient, "Input").Set(reduceSelectionToClient, 0);
  vtkObject::SafeDownCast(this->MoveSelectionToClient->GetClientSideObject())->Modified();
  reduceSelectionToClient->Delete();

  // set data type
  vtkPVDataInformation* info = reduceSelectionToClient->GetDataInformation(0);
  int dataType = info->GetDataSetType();
  if (info->GetCompositeDataSetType() > 0)
  {
    dataType = info->GetCompositeDataSetType();
  }
  vtkSMPropertyHelper(this->MoveSelectionToClient, "OutputDataType").Set(dataType);

  this->MoveSelectionToClient->UpdateVTKObjects();
  this->MoveSelectionToClient->UpdatePipeline();
  return vtkAlgorithm::SafeDownCast(this->MoveSelectionToClient->GetClientSideObject())
    ->GetOutputDataObject(0);
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
  vtkDataSet* ds = vtkDataSet::SafeDownCast(dataObject);
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(dataObject);

  // handle composite case
  compositeFound = false;
  if (cd)
  {
    vtkCompositeDataIterator* it = cd->NewIterator();
    it->SkipEmptyNodesOn();
    it->InitTraversal();
    while (!it->IsDoneWithTraversal())
    {
      ds = vtkDataSet::SafeDownCast(it->GetCurrentDataObject());
      if (ds)
      {
        compositeFound = true;
        std::stringstream ssname;
        ssname << it->GetCurrentFlatIndex() - 1;
        const char* name = it->GetCurrentMetaData()->Get(vtkCompositeDataSet::NAME());
        if (name)
        {
          ssname << ": " << name;
        }
        compositeName = ssname.str();
        break;
      }
      it->GoToNextItem();
    }
    it->Delete();
    if (!compositeFound)
    {
      return nullptr;
    }
  }
  return ds;
}
