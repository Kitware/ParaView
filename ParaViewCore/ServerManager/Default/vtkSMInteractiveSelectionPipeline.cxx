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

#include "vtkSMInteractiveSelectionPipeline.h"

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSettings.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMInteractiveSelectionPipeline);


//----------------------------------------------------------------------------
vtkSMInteractiveSelectionPipeline::vtkSMInteractiveSelectionPipeline() :
  ExtractInteractiveSelection(NULL),
  SelectionRepresentation(NULL),

  PreviousView(NULL),
  PreviousRepresentation(NULL),
  ColorObserver(NULL),
  ConnectionObserver(NULL)
{
  this->ConnectionObserver = vtkCallbackCommand::New();
  this->ConnectionObserver->SetCallback(
    &vtkSMInteractiveSelectionPipeline::OnConnectionClosed);
  this->ConnectionObserver->SetClientData(this);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->AddObserver(vtkCommand::ConnectionClosedEvent, this->ConnectionObserver);
}

//----------------------------------------------------------------------------
vtkSMInteractiveSelectionPipeline::~vtkSMInteractiveSelectionPipeline()
{
  this->OnConnectionClosed();
  if (this->ConnectionObserver)
    {
    this->ConnectionObserver->Delete();
    }
}


//----------------------------------------------------------------------------
void vtkSMInteractiveSelectionPipeline::OnConnectionClosed(
  vtkObject*, unsigned long, void* clientdata, void *)
{
  vtkSMInteractiveSelectionPipeline* This =
    static_cast<vtkSMInteractiveSelectionPipeline*>(clientdata);
  This->OnConnectionClosed();
}

//----------------------------------------------------------------------------
void vtkSMInteractiveSelectionPipeline::OnConnectionClosed()
{
  if (this->ExtractInteractiveSelection)
    {
    this->ExtractInteractiveSelection->Delete();
    this->ExtractInteractiveSelection = NULL;
    }
  if (this->SelectionRepresentation)
    {
    this->SelectionRepresentation->Delete();
    this->SelectionRepresentation = NULL;
    }
  this->PreviousView = NULL;
  this->PreviousRepresentation = NULL;
  if (this->ColorObserver)
    {
    this->ColorObserver->Delete();
    this->ColorObserver = NULL;
    }
}


//----------------------------------------------------------------------------
vtkSMSourceProxy* vtkSMInteractiveSelectionPipeline::ConnectPVExtractSelection(
  vtkSMSourceProxy* source, unsigned int sourceOutputPort,
  vtkSMSourceProxy* selection)
{
  vtkSMSessionProxyManager* proxyManager =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  vtkSMSourceProxy* extract = NULL;
  if (! this->ExtractInteractiveSelection)
    {
    extract = vtkSMSourceProxy::SafeDownCast(
      proxyManager->NewProxy("filters", "PVExtractSelection"));
    }
  else
    {
    extract = this->ExtractInteractiveSelection;
    }

  // set the selection
  vtkSMInputProperty* inputProperty = vtkSMInputProperty::SafeDownCast(
    extract->GetProperty("Selection"));
  inputProperty->RemoveAllProxies();
  inputProperty->AddInputConnection(selection, 0);
  // set the source
  inputProperty = vtkSMInputProperty::SafeDownCast(
    extract->GetProperty("Input"));
  inputProperty->RemoveAllProxies();
  inputProperty->AddInputConnection(source, sourceOutputPort);
  extract->UpdateVTKObjects();

  if (! this->ExtractInteractiveSelection)
    {
    this->ExtractInteractiveSelection = extract;
    }
  return extract;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMInteractiveSelectionPipeline::CreateSelectionRepresentation(
  vtkSMSourceProxy* extract)
{
  if (! this->SelectionRepresentation)
    {
    vtkSMSessionProxyManager* proxyManager =
      vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
    vtkSMProxy* representation =
      proxyManager->NewProxy("representations", "SelectionRepresentation");
    vtkSMSettings * settings = vtkSMSettings::GetInstance();
    settings->GetProxySettings(representation);

    vtkSMPropertyHelper(representation, "Input").Set(
      extract);
    vtkSMPropertyHelper(representation, "Visibility").Set(extract != NULL);
    double color[] = {0.5, 0, 1};
    vtkSMProxy* colorPalette =
      proxyManager->GetProxy("global_properties", "ColorPalette");
    if (! this->ColorObserver)
      {
      // setup a callback to set the interactive selection color
      this->ColorObserver = vtkCallbackCommand::New();
      this->ColorObserver->SetCallback(
        &vtkSMInteractiveSelectionPipeline::OnColorModified);
      this->ColorObserver->SetClientData(this);
      }
    if (colorPalette)
      {
      vtkSMProperty* colorProperty =
        colorPalette->GetProperty("InteractiveSelectionColor");
      colorProperty->AddObserver(
        vtkCommand::ModifiedEvent, this->ColorObserver);
      vtkSMPropertyHelper(colorPalette,
                          "InteractiveSelectionColor").Get(color, 3);
      }
    vtkSMPropertyHelper(representation, "Color").Set(color, 3);
    representation->UpdateVTKObjects();
    this->SelectionRepresentation = representation;
    }
  else
    {
    vtkSMPropertyHelper(this->SelectionRepresentation, "Input").Set(
      extract);
    vtkSMPropertyHelper(this->SelectionRepresentation,
                        "Visibility").Set(extract != NULL);
    this->SelectionRepresentation->UpdateVTKObjects();
    }
  return this->SelectionRepresentation;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMInteractiveSelectionPipeline::GetSelectionRepresentation() const
{
  return this->SelectionRepresentation;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMInteractiveSelectionPipeline::GetOrCreateSelectionRepresentation()
{
  vtkSMProxy* proxyISelectionRepresentation = 
    vtkSMInteractiveSelectionPipeline::GetInstance()->
    GetSelectionRepresentation();
  if (!proxyISelectionRepresentation)
    {
    proxyISelectionRepresentation = 
      vtkSMInteractiveSelectionPipeline::GetInstance()->
      CreateSelectionRepresentation(NULL);
    }
  return proxyISelectionRepresentation;
}

//----------------------------------------------------------------------------
void vtkSMInteractiveSelectionPipeline::OnColorModified(
  vtkObject* source, unsigned long, void* clientdata, void *)
{
  vtkSMInteractiveSelectionPipeline* This =
    static_cast<vtkSMInteractiveSelectionPipeline*>(clientdata);
  vtkSMProperty* property = vtkSMProperty::SafeDownCast(source);
  double color[] = {0, 0, 0};
  vtkSMPropertyHelper(property).Get(color, 3);
  vtkSMPropertyHelper(
    This->SelectionRepresentation, "Color").Set(color, 3);
  This->SelectionRepresentation->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMInteractiveSelectionPipeline::Hide(vtkSMRenderViewProxy* view)
{
  if (this->SelectionRepresentation && view)
    {
    vtkSMPropertyHelper(this->SelectionRepresentation, "Visibility").Set(0);
    this->SelectionRepresentation->UpdateVTKObjects();
    view->StillRender();
    }
}

//----------------------------------------------------------------------------
void vtkSMInteractiveSelectionPipeline::Show(
  vtkSMSourceProxy* representation,
  vtkSMSourceProxy* selection, vtkSMRenderViewProxy* view)
{
  if (representation && view)
    {
    if (this->PreviousRepresentation != representation)
      {
      this->CopyLabels(representation);
      this->PreviousRepresentation = representation;
      }
    vtkSMPropertyHelper representationHelper(representation, "Input", true);
    vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(
      representationHelper.GetAsProxy());
    unsigned int sourceOutputPort = representationHelper.GetOutputPort();
    vtkSMSourceProxy* extract =
      this->ConnectPVExtractSelection(source, sourceOutputPort, selection);
    this->CreateSelectionRepresentation(extract);
    if (this->PreviousView)
      {
      vtkSMPropertyHelper(this->PreviousView, "Representations").Remove(
        this->SelectionRepresentation);
      this->PreviousView->UpdateVTKObjects();
      }
    vtkSMPropertyHelper(view, "Representations").Add(
      this->SelectionRepresentation);
    view->UpdateVTKObjects();
    view->StillRender();
    this->PreviousView = view;
    }
  else
    {
    this->PreviousRepresentation = NULL;
    this->Hide(view);
    }
}

//----------------------------------------------------------------------------
vtkSMInteractiveSelectionPipeline* vtkSMInteractiveSelectionPipeline::GetInstance()
{
  static vtkSmartPointer<vtkSMInteractiveSelectionPipeline> Instance;
  if (Instance.GetPointer() == NULL)
    {
    vtkSMInteractiveSelectionPipeline* pipeline =
      vtkSMInteractiveSelectionPipeline::New();
    Instance = pipeline;
    pipeline->FastDelete();
    }

  return Instance;
}

void vtkSMInteractiveSelectionPipeline::CopyLabels(
  vtkSMProxy* representation)
{
  vtkSMProxy* iSelectionRepresentation = this->GetSelectionRepresentation();
  if (iSelectionRepresentation)
    {
    const char* selectionArrayNames[] = 
      {"SelectionCellFieldDataArrayName", "SelectionPointFieldDataArrayName"};
    const char* selectionVisibilityNames[] = 
      {"SelectionCellLabelVisibility", "SelectionPointLabelVisibility"};
    const char* iSelectionArrayNames[] =
      {"CellFieldDataArrayName", "PointFieldDataArrayName"};
    const char* iSelectionVisibilityNames[] = 
      {"CellLabelVisibility", "PointLabelVisibility"};
    for (int i = 0; i < 2; ++i)
      {
      const char* iSelectionVisibilityName = iSelectionVisibilityNames[i];
      const char* iSelectionArrayName = iSelectionArrayNames[i];
      const char* selectionVisibilityName = selectionVisibilityNames[i];
      const char* selectionArrayName = selectionArrayNames[i];
      int visibility = 0;
      const char* selectionArray = "";
      if (representation)
        {
        visibility = vtkSMPropertyHelper(
          representation, selectionVisibilityName, true).GetAsInt();
        selectionArray = vtkSMPropertyHelper(
          representation, selectionArrayName, true).GetAsString();
        }
      vtkSMPropertyHelper(iSelectionRepresentation,
                          iSelectionVisibilityName, true).Set(visibility);
      vtkSMPropertyHelper(iSelectionRepresentation, 
                          iSelectionArrayName, true).Set(
        selectionArray);
      iSelectionRepresentation->UpdateVTKObjects();
      }
    }
}


//----------------------------------------------------------------------------
void vtkSMInteractiveSelectionPipeline::PrintSelf(ostream& os, vtkIndent indent )
{
  (void)os;
  (void)indent;
}
