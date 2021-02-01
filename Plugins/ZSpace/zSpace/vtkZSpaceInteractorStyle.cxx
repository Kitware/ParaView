/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkZSpaceInteractorStyle.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkZSpaceInteractorStyle.h"

#include "vtkCellData.h"
#include "vtkCellPicker.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVZSpaceView.h"
#include "vtkPointData.h"
#include "vtkPointSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkZSpaceRayActor.h"

#include <cmath>
#include <sstream>

vtkStandardNewMacro(vtkZSpaceInteractorStyle);

//----------------------------------------------------------------------------
vtkZSpaceInteractorStyle::vtkZSpaceInteractorStyle()
{
  // Map controller inputs to interaction states
  this->MapInputToAction(
    vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::Trigger, VTKIS_PICK);

  this->MapInputToAction(
    vtkEventDataDevice::GenericTracker, vtkEventDataDeviceInput::Trigger, VTKIS_POSITION_PROP);

  vtkNew<vtkPolyDataMapper> pdm;
  this->PickActor->SetMapper(pdm);
  this->PickActor->GetProperty()->SetLineWidth(4);
  this->PickActor->GetProperty()->RenderLinesAsTubesOn();
  this->PickActor->GetProperty()->SetRepresentationToWireframe();
  this->PickActor->DragableOff();

  this->TextActor->GetTextProperty()->SetFontSize(17);

  vtkNew<vtkCellPicker> exactPicker;
  this->SetInteractionPicker(exactPicker);
}

//----------------------------------------------------------------------------
vtkZSpaceInteractorStyle::~vtkZSpaceInteractorStyle()
{
  this->CurrentRenderer = nullptr;
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InteractivePicking: " << this->InteractivePicking << endl;

  this->PickActor->PrintSelf(os, indent.GetNextIndent());
  this->PickedInteractionProp->PrintSelf(os, indent.GetNextIndent());
  this->TextActor->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
// Generic events binding
//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::OnMove3D(vtkEventData* edata)
{
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  this->CurrentRenderer = this->Interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
  if (!edd || !this->CurrentRenderer)
  {
    return;
  }

  switch (this->State)
  {
    case VTKIS_POSITION_PROP:
      this->PositionProp(edd);
      this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
      break;
  }

  this->UpdateRay(edd);

  this->UpdatePickActor();
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::OnButton3D(vtkEventData* edata)
{
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  this->CurrentRenderer = this->Interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
  if (!edd || !this->CurrentRenderer)
  {
    return;
  }

  this->State = this->GetStateByReference(edd->GetDevice(), edd->GetInput());
  if (this->State == VTKIS_NONE)
  {
    return;
  }

  switch (edd->GetAction())
  {
    case vtkEventDataAction::Press:
      this->StartAction(this->State, edd);
      break;
    case vtkEventDataAction::Release:
      this->EndAction(this->State, edd);
      break;
  }
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Interaction entry points
//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::StartPick(vtkEventDataDevice3D* edata)
{
  vtkDebugMacro("Start Pick");

  this->RemovePickActor();
  this->State = VTKIS_PICK;
  // update ray length
  this->UpdateRay(edata);
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::EndPick(vtkEventDataDevice3D* edata)
{
  vtkDebugMacro("End Pick");

  // perform probe
  this->ProbeData();
  this->State = VTKIS_NONE;
  this->UpdateRay(edata);
}

//----------------------------------------------------------------------------
bool vtkZSpaceInteractorStyle::HardwareSelect()
{
  vtkDebugMacro("Hardware Select");

  if (!this->ZSpaceView)
  {
    return false;
  }

  this->ZSpaceView->SelectWithRay(this->InteractionProp);

  return true;
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::StartPositionProp(vtkEventDataDevice3D* edata)
{
  vtkDebugMacro("Start Position Prop");

  // Do not position another prop if one is already selected
  if (this->InteractionProp != nullptr)
  {
    return;
  }

  double pos[3];
  double orient[4];
  edata->GetWorldPosition(pos);
  edata->GetWorldOrientation(orient);

  this->FindPickedActor(pos, orient);
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::EndPositionProp(vtkEventDataDevice3D* vtkNotUsed(edata))
{
  vtkDebugMacro("End Position Prop");
  this->State = VTKIS_NONE;
  this->InteractionProp = nullptr;
}

//----------------------------------------------------------------------------
// Interaction methods
//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::ProbeData()
{
  vtkDebugMacro("Probe Data");

  // Invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent, nullptr);

  if (!this->HardwareSelect())
  {
    return;
  }

  // Invoke end pick method if defined
  if (this->HandleObservers && this->HasObserver(vtkCommand::EndPickEvent))
  {
    this->InvokeEvent(vtkCommand::EndPickEvent, this->ZSpaceView->GetLastSelection());
  }
  else
  {
    this->EndPickCallback(this->ZSpaceView->GetLastSelection());
  }
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::EndPickCallback(vtkSelection* sel)
{
  vtkDebugMacro("End Pick Callback");

  vtkSmartPointer<vtkDataSet> ds;
  vtkIdType aid;
  if (!this->FindDataSet(sel, ds, aid))
  {
    return;
  }

  // Create the corresponding pick actor
  if (this->ZSpaceView->GetPickingFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    vtkCell* cell = ds->GetCell(aid);
    this->CreatePickCell(cell);
  }
  else
  {
    double* point = ds->GetPoint(aid);
    this->CreatePickPoint(point);
  }

  if (this->PickedInteractionProp)
  {
    this->PickActor->SetPosition(this->PickedInteractionProp->GetPosition());
    this->PickActor->SetScale(this->PickedInteractionProp->GetScale());
    this->PickActor->SetUserMatrix(this->PickedInteractionProp->GetUserMatrix());
    this->PickActor->SetOrientation(this->PickedInteractionProp->GetOrientation());
  }
  else
  {
    this->PickActor->SetPosition(0.0, 0.0, 0.0);
    this->PickActor->SetScale(1.0, 1.0, 1.0);
  }
  this->CurrentRenderer->AddActor(this->PickActor);

  // Compute the text info about cell or point
  std::string pickedText = this->GetPickedText(ds, aid);

  this->TextActor->SetDisplayPosition(50, 50);
  this->TextActor->SetInput(pickedText.c_str());
  this->CurrentRenderer->AddActor2D(this->TextActor);
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::PositionProp(
  vtkEventData* ed, double* vtkNotUsed(lwpos), double* vtkNotUsed(lwori))
{
  if (this->InteractionProp == nullptr || !this->InteractionProp->GetDragable())
  {
    return;
  }

  // PVRenderView does not support vtkRenderWindowInteractor3D for instance
  // So give to the InteractorStyle3D the stored LastWorldEventPosition
  // and LastWorldEventOrientation
  this->Superclass::PositionProp(ed, this->ZSpaceView->GetLastWorldEventPosition(),
    this->ZSpaceView->GetLastWorldEventOrientation());
}

//----------------------------------------------------------------------------
// Utility routines
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
inline int& vtkZSpaceInteractorStyle::GetStateByReference(
  const vtkEventDataDevice& device, const vtkEventDataDeviceInput& input)
{
  return this
    ->InputMap[static_cast<int>(device) * vtkEventDataNumberOfInputs + static_cast<int>(input)];
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::MapInputToAction(
  vtkEventDataDevice device, vtkEventDataDeviceInput input, int state)
{
  if (input >= vtkEventDataDeviceInput::NumberOfInputs || state < VTKIS_NONE)
  {
    return;
  }

  int& storedState = this->GetStateByReference(device, input);
  if (storedState == state)
  {
    return;
  }

  storedState = state;

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::StartAction(int state, vtkEventDataDevice3D* edata)
{
  switch (state)
  {
    case VTKIS_POSITION_PROP:
      this->StartPositionProp(edata);
      break;
    case VTKIS_PICK:
      this->StartPick(edata);
      break;
  }
}
//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::EndAction(int state, vtkEventDataDevice3D* edata)
{
  switch (state)
  {
    case VTKIS_POSITION_PROP:
      this->EndPositionProp(edata);
      break;
    case VTKIS_PICK:
      this->EndPick(edata);
      break;
  }
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::UpdateRay(vtkEventDataDevice3D* edata)
{
  if (!this->Interactor)
  {
    return;
  }

  double p0[3];
  double wxyz[4];
  edata->GetWorldPosition(p0);
  edata->GetWorldOrientation(wxyz);

  // Create the appropriate ray user transform from event position and orientation
  vtkNew<vtkTransform> stylusT;
  stylusT->Identity();
  stylusT->Translate(p0);
  stylusT->RotateWXYZ(wxyz[0], wxyz[1], wxyz[2], wxyz[3]);

  double rayLength = this->RayMaxLength;
  if (this->State == VTKIS_POSITION_PROP)
  {
    rayLength = this->ZSpaceRayActor->GetLength();
  }
  // Make sure that the ray length is updated in case of a pick
  else if (this->InteractivePicking || this->State == VTKIS_PICK)
  {
    this->FindPickedActor(p0, wxyz);
    // If something is picked, set the length accordingly
    if (this->InteractionProp)
    {
      // Compute the length of the ray
      double p1[3];
      this->InteractionPicker->GetPickPosition(p1);
      rayLength = sqrt(vtkMath::Distance2BetweenPoints(p0, p1));
    }
  }

  if (rayLength == this->RayMaxLength)
  {
    this->ZSpaceRayActor->SetNoPick();
  }
  else
  {
    this->ZSpaceRayActor->SetPick();
  }

  this->ZSpaceRayActor->SetLength(rayLength);
  stylusT->Scale(rayLength, rayLength, rayLength);
  this->ZSpaceRayActor->SetUserTransform(stylusT);

  return;
}

//----------------------------------------------------------------------------
bool vtkZSpaceInteractorStyle::FindDataSet(
  vtkSelection* sel, vtkSmartPointer<vtkDataSet>& ds, vtkIdType& aid)
{
  if (!sel)
  {
    return false;
  }

  vtkSelectionNode* node = sel->GetNode(0);
  if (!node || !node->GetProperties()->Has(vtkSelectionNode::PROP()))
  {
    return false;
  }

  vtkProp3D* prop = vtkProp3D::SafeDownCast(node->GetProperties()->Get(vtkSelectionNode::PROP()));
  if (!prop)
  {
    return false;
  }

  // Save this prop that is linked to the pick actor
  this->PickedInteractionProp = prop;

  vtkPVDataRepresentation* repr =
    vtkPVDataRepresentation::SafeDownCast(node->GetProperties()->Get(vtkSelectionNode::SOURCE()));

  if (!repr)
  {
    return false;
  }

  vtkDataObject* dobj = repr->GetInput();
  vtkCompositeDataSet* cds = vtkCompositeDataSet::SafeDownCast(dobj);
  // handle composite datasets
  if (cds)
  {
    vtkIdType cid = node->GetProperties()->Get(vtkSelectionNode::COMPOSITE_INDEX());
    vtkNew<vtkDataObjectTreeIterator> iter;
    iter->SetDataSet(cds);
    iter->SkipEmptyNodesOn();
    iter->SetVisitOnlyLeaves(1);
    iter->InitTraversal();
    while (iter->GetCurrentFlatIndex() != cid && !iter->IsDoneWithTraversal())
    {
      iter->GoToNextItem();
    }
    if (iter->GetCurrentFlatIndex() == cid)
    {
      ds.TakeReference(vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()));
    }
  }
  else
  {
    ds.TakeReference(vtkDataSet::SafeDownCast(dobj));
  }
  if (!ds)
  {
    return false;
  }

  // get the picked cell
  vtkIdTypeArray* ids = vtkArrayDownCast<vtkIdTypeArray>(node->GetSelectionList());
  if (ids == 0)
  {
    return false;
  }
  aid = ids->GetComponent(0, 0);

  ds->Register(this);

  return true;
}

//----------------------------------------------------------------------------
std::string vtkZSpaceInteractorStyle::GetPickedText(vtkDataSet* ds, const vtkIdType& aid)
{
  // Compute the text from the selected point or cell
  // It would be nice to be able to factorize this code
  // with the vtkSMTooltipSelectionPipeline code
  std::stringstream ssPickedText;

  vtkFieldData* fieldData = nullptr;
  vtkDataArray* originalIds = nullptr;
  // We selected a cell
  if (this->ZSpaceView->GetPickingFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    ssPickedText << "Cell id : " << aid << "\n";
    vtkCellData* cellData = ds->GetCellData();
    fieldData = cellData;

    originalIds = cellData->GetArray("vtkOriginalCellIds");
    if (originalIds)
    {
      ssPickedText << "Id: " << originalIds->GetTuple1(0) << "\n";
    }

    // Cell type
    vtkCell* cell = ds->GetCell(aid);
    ssPickedText << "Type: " << vtkSMCoreUtilities::GetStringForCellType(cell->GetCellType())
                 << "\n";
  }
  else // or a point
  {
    ssPickedText << "Point id : " << aid << "\n";
    vtkPointData* pointData = ds->GetPointData();
    fieldData = pointData;

    originalIds = pointData->GetArray("vtkOriginalPointIds");
    if (originalIds)
    {
      ssPickedText << "Id: " << originalIds->GetTuple1(0) << "\n";
    }

    // Point coords
    double* point = ds->GetPoint(aid);
    ssPickedText << "Coords: (" << point[0] << ", " << point[1] << ", " << point[2] << ")\n";
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
      ssPickedText << array->GetName() << ": ";
      if (array->GetNumberOfComponents() > 1)
      {
        ssPickedText << "(";
      }
      vtkIdType nbComps = array->GetNumberOfComponents();
      for (vtkIdType i_comp = 0; i_comp < nbComps; i_comp++)
      {
        ssPickedText << array->GetTuple(0)[i_comp];
        if (i_comp + 1 < nbComps)
        {
          ssPickedText << ", ";
        }
      }
      if (array->GetNumberOfComponents() > 1)
      {
        ssPickedText << ")\n";
      }
    }
  }

  return std::move(ssPickedText.str());
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::CreatePickCell(vtkCell* cell)
{
  vtkNew<vtkPolyData> pd;
  vtkNew<vtkPoints> pdpts;
  pdpts->SetDataTypeToDouble();
  vtkNew<vtkCellArray> lines;

  this->PickActor->GetProperty()->SetColor(this->PickColor);

  int nedges = cell->GetNumberOfEdges();

  if (nedges)
  {
    for (int edgenum = 0; edgenum < nedges; ++edgenum)
    {
      vtkCell* edge = cell->GetEdge(edgenum);
      vtkPoints* pts = edge->GetPoints();
      int npts = edge->GetNumberOfPoints();
      lines->InsertNextCell(npts);
      for (int ep = 0; ep < npts; ++ep)
      {
        vtkIdType newpt = pdpts->InsertNextPoint(pts->GetPoint(ep));
        lines->InsertCellPoint(newpt);
      }
    }
  }
  else if (cell->GetCellType() == VTK_LINE || cell->GetCellType() == VTK_POLY_LINE)
  {
    vtkPoints* pts = cell->GetPoints();
    int npts = cell->GetNumberOfPoints();
    lines->InsertNextCell(npts);
    for (int ep = 0; ep < npts; ++ep)
    {
      vtkIdType newpt = pdpts->InsertNextPoint(pts->GetPoint(ep));
      lines->InsertCellPoint(newpt);
    }
  }
  else
  {
    return;
  }

  pd->SetPoints(pdpts.Get());
  pd->SetLines(lines.Get());

  static_cast<vtkPolyDataMapper*>(this->PickActor->GetMapper())->SetInputData(pd);
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::CreatePickPoint(double* point)
{
  this->PickActor->GetProperty()->SetColor(this->PickColor);
  this->PickActor->GetProperty()->SetPointSize(8.0);

  vtkNew<vtkPointSource> pointSource;
  pointSource->SetCenter(point);
  pointSource->SetNumberOfPoints(1);
  pointSource->SetRadius(0);

  pointSource->Update();
  static_cast<vtkPolyDataMapper*>(this->PickActor->GetMapper())
    ->SetInputData(pointSource->GetOutput());
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::UpdatePickActor()
{

  if (this->PickedInteractionProp)
  {
    // Remove the pick actor if it has been deleted
    if (!this->CurrentRenderer->HasViewProp(this->PickedInteractionProp))
    {
      this->RemovePickActor();
      return;
    }

    // Update the visibility
    this->PickActor->SetVisibility(this->PickedInteractionProp->GetVisibility());
    this->TextActor->SetVisibility(this->PickedInteractionProp->GetVisibility());

    // Move the point/cell picked with the prop
    if (this->PickedInteractionProp->GetUserMatrix() != nullptr)
    {
      this->PickActor->SetUserMatrix(this->PickedInteractionProp->GetUserMatrix());
    }
  }
}

//----------------------------------------------------------------------------
void vtkZSpaceInteractorStyle::RemovePickActor()
{
  if (this->CurrentRenderer)
  {
    this->CurrentRenderer->RemoveActor(this->PickActor);
    this->CurrentRenderer->RemoveActor2D(this->TextActor);
    this->PickedInteractionProp = nullptr;
  }
}
