/*=========================================================================

  Program:   ParaView
  Module:    vtkPVComparativeVisManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVComparativeVisManager.h"

#include "vtkCamera.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkInteractorStyleTrackballMultiActor.h"
#include "vtkKWToolbarSet.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVComparativeVis.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkRenderer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSimpleDisplayProxy.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"

#include <vtkstd/list>
#include <vtkstd/vector>

vtkStandardNewMacro(vtkPVComparativeVisManager);
vtkCxxRevisionMacro(vtkPVComparativeVisManager, "1.13");

vtkCxxSetObjectMacro(
  vtkPVComparativeVisManager, Application, vtkPVApplication);

// Private implementation
struct vtkPVComparativeVisManagerInternals
{
  vtkPVComparativeVisManagerInternals()
    {
      this->CurrentPVSource = 0;
      this->MainPanelVisibility = 1;
      this->InteractorStyle = 0;
    }

  // These are used to store the state of the main window prior
  // to Show(). This state is later restored after Hide().
  vtkstd::list<vtkSMSimpleDisplayProxy*> VisibleDisplayProxies;
  int MainPanelVisibility;
  int InteractorStyle;
  vtkstd::list<vtkKWToolbar*> VisibleToolbars;
  vtkPVSource* CurrentPVSource;
  int ParallelProjection;
  double CameraPosition[3];
  double CameraFocalPoint[3];
  double CameraViewUp[3];
  
  typedef 
  vtkstd::vector<vtkSmartPointer<vtkPVComparativeVis> > VisualizationsType;
  VisualizationsType Visualizations;
};

//-----------------------------------------------------------------------------
vtkPVComparativeVisManager::vtkPVComparativeVisManager()
{
  this->Application = 0;
  this->Internal = new vtkPVComparativeVisManagerInternals;
  this->IStyle = 0;
  this->SelectedVisualizationName = 0;
  this->CurrentlyDisplayedVisualization = 0;

  this->IStyle = 
    vtkInteractorStyleTrackballMultiActor::New();
}

//-----------------------------------------------------------------------------
vtkPVComparativeVisManager::~vtkPVComparativeVisManager()
{
  this->SetApplication(0);
  delete this->Internal;
  if (this->IStyle)
    {
    this->IStyle->Delete();
    }
  this->SetSelectedVisualizationName(0);
}

//-----------------------------------------------------------------------------
unsigned int vtkPVComparativeVisManager::GetNumberOfVisualizations()
{
  return this->Internal->Visualizations.size();
}

//-----------------------------------------------------------------------------
vtkPVComparativeVis* vtkPVComparativeVisManager::GetVisualization(
  unsigned int idx)
{
  return this->Internal->Visualizations[idx].GetPointer();
}

//-----------------------------------------------------------------------------
vtkPVComparativeVis* vtkPVComparativeVisManager::GetVisualization(
  const char* name)
{
  vtkPVComparativeVisManagerInternals::VisualizationsType::iterator iter = 
    this->Internal->Visualizations.begin();
  for(; iter != this->Internal->Visualizations.end(); iter++)
    {
    vtkPVComparativeVis* vis = iter->GetPointer();
    if (vis && vis->GetName() && name && strcmp(name, vis->GetName()) == 0)
      {
      return iter->GetPointer();
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::GenerateVisualization(vtkPVComparativeVis* vis)
{
  if (!vis)
    {
    return;
    }

  this->Hide();
  vis->Generate();
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::AddVisualization(vtkPVComparativeVis* vis)
{
  if (!vis->GetName())
    {
    vtkErrorMacro("Cannot add visualization without a name!");
    return;
    }
  if (this->Application)
    {
    vis->SetApplication(this->Application);
    }
  this->Internal->Visualizations.push_back(vis);

  if (!this->SelectedVisualizationName)
    {
    this->SetSelectedVisualizationName(vis->GetName());
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::RemoveVisualization(const char* name)
{
  vtkPVComparativeVis* curVis = 
    this->GetVisualization(this->SelectedVisualizationName);
  vtkPVComparativeVisManagerInternals::VisualizationsType::iterator iter = 
    this->Internal->Visualizations.begin();
  for(; iter != this->Internal->Visualizations.end(); iter++)
    {
    vtkPVComparativeVis* vis = iter->GetPointer();
    if (vis && vis->GetName() && name && strcmp(name, vis->GetName()) == 0)
      {
      if (iter->GetPointer() == curVis)
        {
        this->Hide();
        }
      this->Internal->Visualizations.erase(iter);
      break;
      }
    }
}

//-----------------------------------------------------------------------------
int vtkPVComparativeVisManager::Show()
{
  if (!this->Application)
    {
    vtkErrorMacro("Application is not set. Cannot show");
    return 0;
    }

  vtkPVComparativeVis* currentVis = this->GetVisualization(
    this->SelectedVisualizationName);
  if (!currentVis)
    {
    vtkErrorMacro("No current visualization defined. Cannot switch to "
                  "comparative visualization mode.");
    return 0;
    }

  if (currentVis == this->CurrentlyDisplayedVisualization)
    {
    return 1;
    }

  if (!this->CurrentlyDisplayedVisualization)
    {
    this->IStyle->SetApplication(this->Application);
    
    vtkPVWindow* window = this->Application->GetMainWindow();
    // Make sure the main window updates it's enable state based
    // on InComparativeVis.
    window->SetInComparativeVis(1);
    window->UpdateEnableState();
    
    // Store the state and change it according to comparative vis
    // requirements. During comparative vis mode, the user should
    // not be able to change any visualization settings (create, modify,
    // delete sources etc.)
    this->Internal->InteractorStyle = window->GetInteractorStyle();
    window->SetInteractorStyle(vtkPVWindow::INTERACTOR_STYLE_2D);
    window->GetInteractor()->SetInteractorStyle(this->IStyle);
    this->Internal->CurrentPVSource = window->GetCurrentPVSource();
    window->SetCurrentPVSource(0);
    this->Internal->MainPanelVisibility = window->GetMainPanelVisibility();
    window->SetMainPanelVisibility(0);
    
    vtkKWToolbarSet* toolbars = window->GetMainToolbarSet();
    int numToolbars = toolbars->GetNumberOfToolbars();
    this->Internal->VisibleToolbars.clear();
    int i;
    for (i=0; i< numToolbars; i++)
      {
      vtkKWToolbar* toolbar = toolbars->GetNthToolbar(i);
      if (toolbars->GetToolbarVisibility(toolbar))
        {
        this->Internal->VisibleToolbars.push_back(toolbar);
        toolbars->SetToolbarVisibility(toolbar, 0);
        }
      }
    
    toolbars = window->GetSecondaryToolbarSet();
    numToolbars = toolbars->GetNumberOfToolbars();
    for (i=0; i< numToolbars; i++)
      {
      vtkKWToolbar* toolbar = toolbars->GetNthToolbar(i);
      if (toolbars->GetToolbarVisibility(toolbar))
        {
        this->Internal->VisibleToolbars.push_back(toolbar);
        toolbars->SetToolbarVisibility(toolbar, 0);
        }
      }
    window->UpdateToolbarState();
    
    this->Internal->VisibleDisplayProxies.clear();
    
    // Hide all visible displays (to be restored after Hide())
    vtkSMRenderModuleProxy* ren =
      this->Application->GetRenderModuleProxy();
    vtkCollection* displays = ren->GetDisplays();
    vtkCollectionIterator* iter = displays->NewIterator();
    for(iter->GoToFirstItem(); 
        !iter->IsDoneWithTraversal(); 
        iter->GoToNextItem())
      {
      vtkSMSimpleDisplayProxy* pDisp = vtkSMSimpleDisplayProxy::SafeDownCast(
        iter->GetCurrentObject());
      if (pDisp && pDisp->GetVisibilityCM())
        {
        pDisp->SetVisibilityCM(0);
        this->Internal->VisibleDisplayProxies.push_back(pDisp);
        }
      }
    iter->Delete();
    
    // Store camera settings
    vtkPVRenderView* mainView = this->Application->GetMainView();
    vtkCamera* camera = 
      this->Application->GetMainView()->GetRenderer()->GetActiveCamera();
    camera->GetPosition(this->Internal->CameraPosition);
    camera->GetFocalPoint(this->Internal->CameraFocalPoint);
    camera->GetViewUp(this->Internal->CameraViewUp);
    
    vtkSMIntVectorProperty* parallelProj =
      vtkSMIntVectorProperty::SafeDownCast(
        ren->GetProperty("CameraParallelProjection"));
    this->Internal->ParallelProjection = parallelProj->GetElement(0);
    parallelProj->SetElements1(1);
    
    ren->UpdateVTKObjects();
    
    mainView->ForceRender();

    // Make sure the render window size is updated (we hid the left panel)
    this->Application->Script("update idletasks");
    }
  else
    {
    this->CurrentlyDisplayedVisualization->Hide();
    }

  int retVal = 1;
  vtkTimerLog::MarkStartEvent("Show Vis");
  this->CurrentlyDisplayedVisualization = currentVis;
  if (!currentVis->Show())
    {
    this->Hide();
    retVal = 0;
    }
  this->IStyle->SetHelperProxy(currentVis->GetMultiActorHelper());
  vtkTimerLog::MarkEndEvent("Show Vis");
  return retVal;
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::Hide()
{
  if (!this->CurrentlyDisplayedVisualization)
    {
    return;
    }
  // Restore state prior to Show()
  vtkPVWindow* window = this->Application->GetMainWindow();
  vtkKWToolbarSet* toolbars = window->GetMainToolbarSet();
  vtkstd::list<vtkKWToolbar*>::iterator iter = 
      this->Internal->VisibleToolbars.begin();
  for(; iter != this->Internal->VisibleToolbars.end(); iter++)
    {
    toolbars->SetToolbarVisibility(*iter, 1);
    window->GetSecondaryToolbarSet()->SetToolbarVisibility(*iter, 1);
    }
  window->UpdateToolbarState();

  window->SetMainPanelVisibility(this->Internal->MainPanelVisibility);

  window->SetInteractorStyle(this->Internal->InteractorStyle);

  vtkstd::list<vtkSMSimpleDisplayProxy*>::iterator iter2 = 
      this->Internal->VisibleDisplayProxies.begin();
  for(; iter2 != this->Internal->VisibleDisplayProxies.end(); iter2++)
    {
    (*iter2)->SetVisibilityCM(1);
    }

  vtkSMRenderModuleProxy* ren =
    this->Application->GetRenderModuleProxy();

  if (ren)
    {
    vtkSMIntVectorProperty* parallelProj =
      vtkSMIntVectorProperty::SafeDownCast(
        ren->GetProperty("CameraParallelProjection"));
    parallelProj->SetElements1(this->Internal->ParallelProjection);
    
    ren->UpdateVTKObjects();
    }

  window->SetCurrentPVSource(this->Internal->CurrentPVSource);

  vtkPVRenderView* mainView = this->Application->GetMainView();
  mainView->SetCameraState(
    this->Internal->CameraPosition[0], 
    this->Internal->CameraPosition[1], 
    this->Internal->CameraPosition[2],
    this->Internal->CameraFocalPoint[0], 
    this->Internal->CameraFocalPoint[1], 
    this->Internal->CameraFocalPoint[2],
    this->Internal->CameraViewUp[0], 
    this->Internal->CameraViewUp[1], 
    this->Internal->CameraViewUp[2]
    );

  vtkPVComparativeVis* currentVis = this->GetVisualization(
    this->SelectedVisualizationName);
  if (currentVis)
    {
    currentVis->Hide();
    }
  this->Application->GetMainView()->ForceRender();

  if (ren)
    {
    ren->ResetCameraClippingRange();
    }

  this->CurrentlyDisplayedVisualization = 0;
  window->SetInComparativeVis(0);
  window->UpdateEnableState();
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SelectedVisualizationName: " 
     << (this->SelectedVisualizationName?this->SelectedVisualizationName:"(null)")
     << endl;
  os << indent << "CurrentlyDisplayedVisualization: ";
  if (this->CurrentlyDisplayedVisualization)
    {
    this->CurrentlyDisplayedVisualization->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
