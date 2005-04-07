/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSource.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVConfig.h"
#include "vtkPVSource.h"
#include "vtkObjectFactory.h"
#include "vtkPVColorMap.h"
#include "vtkArrayMap.txx"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkDataSet.h"
#include "vtkKWFrame.h"
#include "vtkKWMenu.h"
#include "vtkPVSourceNotebook.h"
#include "vtkKWView.h"
#include "vtkPVApplication.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkSMPart.h"
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  #include "vtkSMDisplayProxy.h"
  #include "vtkSMIntVectorProperty.h"
  #include "vtkSMStringVectorProperty.h"
  #include "vtkSMRenderModuleProxy.h"
  #include "vtkSMProxyProperty.h"
  #include "vtkSMDoubleVectorProperty.h"
#else
  #include "vtkSMPartDisplay.h"
#endif
#include "vtkPVCornerAnnotation.h"
#include "vtkPVInputProperty.h"
#include "vtkPVNumberOfOutputsInformation.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderModule.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVWidgetCollection.h"
#include "vtkPVWindow.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkRenderer.h"
#include "vtkPVAnimationInterface.h"
#include "vtkSMCubeAxesDisplayProxy.h"
#include "vtkSMPointLabelDisplayProxy.h"
#include "vtkDataSetAttributes.h"
#include <vtkstd/vector>


vtkStandardNewMacro(vtkPVSource);
vtkCxxRevisionMacro(vtkPVSource, "1.427.2.9");
vtkCxxSetObjectMacro(vtkPVSource,Notebook,vtkPVSourceNotebook);
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  vtkCxxSetObjectMacro(vtkPVSource,DisplayProxy, vtkSMDisplayProxy);
#else
  vtkCxxSetObjectMacro(vtkPVSource,PartDisplay,vtkSMPartDisplay);
#endif

int vtkPVSourceCommand(ClientData cd, Tcl_Interp *interp,
                           int argc, char *argv[]);

vtkCxxSetObjectMacro(vtkPVSource, View, vtkKWView);
vtkCxxSetObjectMacro(vtkPVSource, Proxy, vtkSMSourceProxy);

//----------------------------------------------------------------------------
vtkPVSource::vtkPVSource()
{
  this->CommandFunction = vtkPVSourceCommand;
  this->DataInformationValid = 0;

  this->NumberOfOutputsInformation = vtkPVNumberOfOutputsInformation::New();
  
  // Number of instances cloned from this prototype
  this->PrototypeInstanceCount = 0;

  this->Name = 0;
  this->Label = 0;
  this->ModuleName = 0;
  this->MenuName = 0;
  this->ShortHelp = 0;
  this->LongHelp  = 0;
  this->SourceList = 0;
  this->OverideAutoAccept = 0;

  // Initialize the data only after  Accept is invoked for the first time.
  // This variable is used to determine that.
  this->Initialized = 0;

  // The notebook which holds Parameters, Display and Information pages.
  this->Notebook = 0;  
  this->PVInputs = NULL;
  this->NumberOfPVInputs = 0;
  
  this->NumberOfPVConsumers = 0;
  this->PVConsumers = 0;
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  this->DisplayProxy = 0;
#else
  this->PartDisplay = 0;
#endif

  this->ParameterFrame = vtkKWFrame::New();
  this->Widgets = vtkPVWidgetCollection::New();
    
  this->ReplaceInput = 1;

  this->View = NULL;

  this->VisitedFlag = 0;

  this->SourceClassName = 0;

  this->RequiredNumberOfInputParts = -1;
  this->VTKMultipleInputsFlag = 0;
  this->InputProperties = vtkCollection::New();

  this->VTKMultipleProcessFlag = 2;
  
  this->IsPermanent = 0;
  
  this->AcceptCallbackFlag = 0;

  this->SourceGrabbed = 0;

  this->ToolbarModule = 0;

  this->UpdateSourceInBatch = 0;

  this->LabelSetByUser = 0;

  this->Proxy = 0;
  this->CubeAxesDisplayProxy = 0;
  this->CubeAxesVisibility = 0;
  this->PointLabelDisplayProxy = 0;
  this->PointLabelVisibility = 0;
  
  this->PVColorMap = 0;  

  this->ResetInSelect = 1;
}

//----------------------------------------------------------------------------
vtkPVSource::~vtkPVSource()
{
  this->RemoveAllPVInputs();

  this->NumberOfOutputsInformation->Delete();
  this->NumberOfOutputsInformation = NULL;
  
  if (this->PVConsumers)
    {
    delete [] this->PVConsumers;
    this->PVConsumers = NULL;
    this->NumberOfPVConsumers = 0;
    }

  vtkSMProxyManager* proxm = vtkSMObject::GetProxyManager();
  if (proxm && this->GetName())
    {
    proxm->UnRegisterProxy(this->GetName());
    
    const char* proxyName = 0;
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
    proxyName = 
      proxm->GetProxyName("animateable", this->DisplayProxy);
#else
    proxyName = 
      proxm->GetProxyName("animateable", this->PartDisplay);
#endif
    if (proxyName)
      {
      proxm->UnRegisterProxy("animateable", proxyName);
      }
    proxyName = 
      proxm->GetProxyName("animateable", this->Proxy);
    if (proxyName)
      {
      proxm->UnRegisterProxy("animateable", proxyName);
      }
    }
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  this->SetDisplayProxy(0);
#else
  this->SetPartDisplay(0);
#endif
  this->SetProxy(0);

  // Do not use SetName() or SetLabel() here. These make
  // the navigation window update when it should not.
  delete[] this->Name;
  delete[] this->Label;

  this->SetSourceList(0);
  this->SetMenuName(0);
  this->SetShortHelp(0);
  this->SetLongHelp(0);

  // This is necessary in order to make the parent frame release it's
  // reference to the widgets. Otherwise, the widgets get deleted only
  // when the parent (usually the parameters notebook page) is deleted.
  // (I must of fixed this bug twice. keep both comments).
  // Since the notebook is now shared and remains around after 
  // this source deletes.  This parameters from as a child will 
  // not be deleted either.  Parent keeps a list of children.
  this->SetNotebook(0);
  this->ParameterFrame->SetParent(0);
  this->ParameterFrame->Delete();
  this->ParameterFrame = NULL;
  this->Widgets->Delete();
  this->Widgets = NULL;

  this->SetView(NULL);

  this->SetSourceClassName(0);
 
  this->InputProperties->Delete();
  this->InputProperties = NULL;

  this->SetModuleName(0);
  if (this->CubeAxesDisplayProxy)
    {
    this->CubeAxesDisplayProxy->Delete();
    this->CubeAxesDisplayProxy = 0;
    }
  if (this->PointLabelDisplayProxy)
    {
    this->PointLabelDisplayProxy->Delete();
    this->PointLabelDisplayProxy = 0;
    }

  this->SetPVColorMap(0);
  this->SetSourceList(0);
}

//----------------------------------------------------------------------------
void vtkPVSource::AddPVInput(vtkPVSource *pvs)
{
  this->SetPVInputInternal("Input", this->NumberOfPVInputs, pvs, 0);
}

//----------------------------------------------------------------------------
void vtkPVSource::SetPVInput(const char* name, int idx, vtkPVSource *pvs)
{
  this->SetPVInputInternal(name, idx, pvs, 1);
}

//----------------------------------------------------------------------------
void vtkPVSource::SetPVInputInternal(
  const char* iname, int idx, vtkPVSource *pvs, int doInit)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (pvs == 0)
    {
    vtkErrorMacro("NULL input is not allowed.");
    return;
    }

  if (pvApp == 0)
    {
    vtkErrorMacro(
      "No Application. Create the source before setting the input.");
    return;
    }
  // Handle visibility of old and new input.
  if (this->ReplaceInput)
    {
    vtkPVSource *oldInput = this->GetNthPVInput(idx);
    if (oldInput)
      {
      oldInput->SetVisibility(1);
      this->GetPVRenderView()->EventuallyRender();
      }
    }

  if (this->Proxy)
    {
    vtkSMProxyProperty* inputp = vtkSMProxyProperty::SafeDownCast(
      this->Proxy->GetProperty(iname));
    if (inputp)
      {
      if (doInit)
        {
        inputp->RemoveAllProxies();
        }
      inputp->AddProxy(pvs->GetProxy());
      }
    }

  // Set the paraview reference to the new input.
  this->SetNthPVInput(idx, pvs);
  if (pvs == NULL)
    {
    return;
    }

  this->GetPVRenderView()->UpdateNavigationWindow(this, 0);
}

//----------------------------------------------------------------------------
void vtkPVSource::AddPVConsumer(vtkPVSource *c)
{
  // make sure it isn't already there
  if (this->IsPVConsumer(c))
    {
    return;
    }
  // add it to the list, reallocate memory
  vtkPVSource **tmp = this->PVConsumers;
  this->NumberOfPVConsumers++;
  this->PVConsumers = new vtkPVSource* [this->NumberOfPVConsumers];
  for (int i = 0; i < (this->NumberOfPVConsumers-1); i++)
    {
    this->PVConsumers[i] = tmp[i];
    }
  this->PVConsumers[this->NumberOfPVConsumers-1] = c;
  // free old memory
  delete [] tmp;
}

//----------------------------------------------------------------------------
void vtkPVSource::RemovePVConsumer(vtkPVSource *c)
{
  // make sure it is already there
  if (!this->IsPVConsumer(c))
    {
    return;
    }
  // remove it from the list, reallocate memory
  vtkPVSource **tmp = this->PVConsumers;
  this->NumberOfPVConsumers--;
  this->PVConsumers = new vtkPVSource* [this->NumberOfPVConsumers];
  int cnt = 0;
  int i;
  for (i = 0; i <= this->NumberOfPVConsumers; i++)
    {
    if (tmp[i] != c)
      {
      this->PVConsumers[cnt] = tmp[i];
      cnt++;
      }
    }
  // free old memory
  delete [] tmp;
}

//----------------------------------------------------------------------------
int vtkPVSource::IsPVConsumer(vtkPVSource *c)
{
  int i;
  for (i = 0; i < this->NumberOfPVConsumers; i++)
    {
    if (this->PVConsumers[i] == c)
      {
      return 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkPVSource *vtkPVSource::GetPVConsumer(int i)
{
  if (i >= this->NumberOfPVConsumers)
    {
    return 0;
    }
  return this->PVConsumers[i];
}

//----------------------------------------------------------------------------
int vtkPVSource::GetNumberOfParts()
{
  return this->Proxy->GetNumberOfParts();
}

//----------------------------------------------------------------------------
vtkSMPart* vtkPVSource::GetPart(int idx)
{
  return this->Proxy->GetPart(idx);
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkPVSource::GetDataInformation()
{
  vtkPVDataInformation* info = this->Proxy->GetDataInformation();
  if (this->DataInformationValid == 0)
    {
    // Where else should I put this?
    //law int fixme; // Although this should probably go into vtkPVSource::Update,
    // I am going to get rid of this refernece anyway. (Or should I?)
    // Window will know to update the InformationGUI when the current PVSource
    // is changed, but how will it detect that a source has changed?
    // Used to only update the information.  We could make it specific to info again ...
    this->DataInformationValid = 1;
    if (this->Notebook)
      {
      this->Notebook->Update();
      }
    }
  return info;
}

//----------------------------------------------------------------------------
void vtkPVSource::InvalidateDataInformation()
{
  this->DataInformationValid = 0;
}

//----------------------------------------------------------------------------
void vtkPVSource::Update()
{
  this->Proxy->UpdatePipeline();
  if (this->PVColorMap)
    {
    this->PVColorMap->UpdateForSource(this);
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if( this->Notebook)
    {
    this->PropagateEnableState(this->Notebook);
    this->Notebook->UpdateEnableStateWithSource(this);
    }
  this->PropagateEnableState(this->PVColorMap);

  if ( this->Widgets )
    {
    vtkPVWidget *pvWidget;
    vtkCollectionIterator *it = this->Widgets->NewIterator();
    it->InitTraversal();

    int i;
    for (i = 0; i < this->Widgets->GetNumberOfItems(); i++)
      {
      pvWidget = static_cast<vtkPVWidget*>(it->GetCurrentObject());
      pvWidget->SetEnabled(this->Enabled);
      it->GoToNextItem();
      }
    it->Delete();
    }
}
  

//----------------------------------------------------------------------------
// Functions to update the progress bar
void vtkPVSourceStartProgress(void* vtkNotUsed(arg))
{
  //vtkPVSource *me = (vtkPVSource*)arg;
  //vtkSource *vtkSource = me->GetVTKSource();
  //static char str[200];
  
  //if (vtkSource && me->GetWindow())
  //  {
  //  sprintf(str, "Processing %s", vtkSource->GetClassName());
  //  me->GetWindow()->SetStatusText(str);
  //  }
}
//----------------------------------------------------------------------------
void vtkPVSourceReportProgress(void* vtkNotUsed(arg))
{
  //vtkPVSource *me = (vtkPVSource*)arg;
  //vtkSource *vtkSource = me->GetVTKSource();

  //if (me->GetWindow())
  //  {
  //  me->GetWindow()->GetProgressGauge()->SetValue((int)(vtkSource->GetProgress() * 100));
  //  }
}
//----------------------------------------------------------------------------
void vtkPVSourceEndProgress(void* vtkNotUsed(arg))
{
  //vtkPVSource *me = (vtkPVSource*)arg;
  
  //if (me->GetWindow())
  //  {
  //  me->GetWindow()->SetStatusText("");
  //  me->GetWindow()->GetProgressGauge()->SetValue(0);
  //  }
}


int vtkPVSource::GetNumberOfVTKSources()
{
  if (!this->Proxy)
    {
    return 0;
    }
  return this->Proxy->GetNumberOfIDs();
}

//----------------------------------------------------------------------------
unsigned int vtkPVSource::GetVTKSourceIDAsInt(int idx)
{
  vtkClientServerID id = this->GetVTKSourceID(idx);
  return id.ID;
}

//----------------------------------------------------------------------------
vtkClientServerID vtkPVSource::GetVTKSourceID(int idx)
{
  if(idx >= this->GetNumberOfVTKSources() || !this->Proxy)
    {
    vtkClientServerID id = {0};
    return id;
    }
  return this->Proxy->GetID(idx);
}

//----------------------------------------------------------------------------
vtkPVWindow* vtkPVSource::GetPVWindow()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (pvApp == NULL)
    {
    return NULL;
    }
  
  return pvApp->GetMainWindow();
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVSource::GetPVApplication()
{
  if (this->GetApplication() == NULL)
    {
    return NULL;
    }
  
  if (this->GetApplication()->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->GetApplication());
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}

//----------------------------------------------------------------------------
void vtkPVSource::CreateProperties()
{
  if (this->Notebook == 0)
    {
    vtkErrorMacro("Notebook has not been set yet.");  
    }

  // Set the description frame
  // Try to do something that looks like the parameters, i.e. fixed-width
  // labels and "expandable" values. This has to be fixed later when the
  // parameters will be properly aligned (i.e. gridded)

  this->ParameterFrame->SetParent(this->Notebook->GetMainParameterFrame());

  this->ParameterFrame->ScrollableOn();
  this->ParameterFrame->Create(this->GetApplication(),0);

  this->UpdateProperties();

  vtkPVWidget *pvWidget;
  vtkCollectionIterator *it = this->Widgets->NewIterator();
  it->InitTraversal();
  
  int i;
  for (i = 0; i < this->Widgets->GetNumberOfItems(); i++)
    {
    pvWidget = static_cast<vtkPVWidget*>(it->GetCurrentObject());
    pvWidget->SetParent(this->ParameterFrame->GetFrame());
    pvWidget->Create(this->GetApplication());
    if (!pvWidget->GetHideGUI())
      {
      this->Script("pack %s -side top -fill x -expand t", 
                   pvWidget->GetWidgetName());
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkPVSource::GrabFocus()
{
  this->SourceGrabbed = 1;

  this->GetPVRenderView()->UpdateNavigationWindow(this, 1);
}

//----------------------------------------------------------------------------
void vtkPVSource::UnGrabFocus()
{

  if ( this->SourceGrabbed )
    {
    this->GetPVRenderView()->UpdateNavigationWindow(this, 0);
    }
  this->SourceGrabbed = 0;
  if (this->Initialized)
    {
    // UpdateEnableState causes a call to CreateParts which updates the source.
    // We do not want to do this if the user is deleting before they accept.
    this->GetPVWindow()->UpdateEnableState();
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::Pack()
{
  // The update is needed to work around a packing problem which
  // occur for large windows. Do not remove.
  this->GetPVRenderView()->UpdateTclButAvoidRendering();

  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->Notebook->GetMainParameterFrame()->GetWidgetName());
  this->Script("pack %s -fill both -expand t -side top", 
               this->ParameterFrame->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVSource::Select()
{
  this->Pack();
  
  this->UpdateProperties();
  // This may best be merged with the UpdateProperties call but ...
  // We make the call here to update the input menu, 
  // which is now just another pvWidget.
  if (this->ResetInSelect)
    {
    this->ResetCallback();
    }
  
  if (this->Notebook)
    {
    this->Notebook->SetPVSource(this);
    this->Notebook->Update();
    }

  if (this->GetPVRenderView())
    {
    this->GetPVRenderView()->UpdateNavigationWindow(this, this->SourceGrabbed);
    }

  int i;
  vtkPVWidget *pvWidget = 0;
  vtkCollectionIterator *it = this->Widgets->NewIterator();
  it->InitTraversal();
  for (i = 0; i < this->Widgets->GetNumberOfItems(); i++)
    {
    pvWidget = static_cast<vtkPVWidget*>(it->GetCurrentObject());
    pvWidget->Select();
    it->GoToNextItem();
    }
  it->Delete();
  this->Notebook->ShowPage("Display");
  this->Notebook->ShowPage("Information");
}

//----------------------------------------------------------------------------
void vtkPVSource::Deselect(int)
{
  if (this->Notebook)
    {
    this->Notebook->SetPVSource(0);
    }

  int i;
  vtkPVWidget *pvWidget = 0;
  vtkCollectionIterator *it = this->Widgets->NewIterator();
  it->InitTraversal();
  
  for (i = 0; i < this->Widgets->GetNumberOfItems(); i++)
    {

    pvWidget = static_cast<vtkPVWidget*>(it->GetCurrentObject());
    pvWidget->Deselect();
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
char* vtkPVSource::GetName()
{
  return this->Name;
}

//----------------------------------------------------------------------------
void vtkPVSource::SetName (const char* arg) 
{ 
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " 
                << this->Name << " to " << arg ); 
  if ( this->Name && arg && (!strcmp(this->Name,arg))) 
    { 
    return;
    } 
  if (this->Name) 
    { 
    delete [] this->Name; 
    } 
  if (arg) 
    { 
    this->Name = new char[strlen(arg)+1]; 
    strcpy(this->Name,arg); 
    } 
  else 
    { 
    this->Name = NULL;
    }
  this->Modified();
  
  // Make sure the description frame is upto date.
  this->Notebook->Update();
  
  // Update the nav window (that usually might display name + description)
  if (this->GetPVRenderView())
    {
    this->GetPVRenderView()->UpdateNavigationWindow(this, this->SourceGrabbed);
    }
} 

//----------------------------------------------------------------------------
char* vtkPVSource::GetLabel() 
{ 
  // Design choice: if the description is empty, initialize it with the
  // Tcl name, so that the user knows what to overrides in the nav window.

  if (this->Label == NULL)
    {
    this->SetLabelNoTrace(this->GetName());
    }
  return this->Label;
}

//----------------------------------------------------------------------------
void vtkPVSource::SetLabelOnce(const char* arg) 
{
  if (!this->LabelSetByUser)
    {
    this->SetLabelNoTrace(arg);
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::SetLabel(const char* arg) 
{ 
  this->LabelSetByUser = 1;

  this->SetLabelNoTrace(arg);

  if ( !this->GetApplication() )
    {
    return;
    }
  // Update the nav window (that usually might display name + description)
  vtkPVSource* current = this->GetPVWindow()->GetCurrentPVSource();
  if (this->GetPVRenderView() && current)
    {
    this->GetPVRenderView()->UpdateNavigationWindow(
      current, current->SourceGrabbed);
    }
  // Trace here, not in SetLabel (design choice)
  this->GetPVApplication()->AddTraceEntry("$kw(%s) SetLabel {%s}",
                                          this->GetTclName(),
                                          this->Label);
}

//----------------------------------------------------------------------------
void vtkPVSource::SetLabelNoTrace(const char* arg) 
{ 
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " 
                << this->Label << " to " << arg ); 
  if ( this->Label && arg && (!strcmp(this->Label,arg))) 
    { 
    return;
    } 
  if (this->Label) 
    { 
    delete [] this->Label; 
    } 
  if (arg) 
    { 
    this->Label = new char[strlen(arg)+1]; 
    strcpy(this->Label,arg); 
    } 
  else 
    { 
    this->Label = NULL;
    }
  this->Modified();

  // Make sure the description frame is upto date.
  this->Notebook->Update();

  vtkPVWindow *window = this->GetPVWindow();
  if (window)
    {
    window->UpdateSelectMenu();
    window->UpdateAnimationInterface();
    }

} 

//----------------------------------------------------------------------------
void vtkPVSource::SetVisibility(int v)
{
  if (this->GetVisibility() == v)
    {
    return;
    }
  
  this->AddTraceEntry("$kw(%s) SetVisibility %d", this->GetTclName(), v);
  this->SetVisibilityNoTrace(v);
}

//----------------------------------------------------------------------------
void vtkPVSource::SetVisibilityNoTrace(int v)
{
  if (this->GetVisibility() == v || 
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
      this->DisplayProxy == 0
#else
      this->PartDisplay == 0
#endif
  )
    {
    return;
    }

  int cubeAxesVisibility = this->GetCubeAxesVisibility();
  int pointLabelVisibility = this->GetPointLabelVisibility();

#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  this->DisplayProxy->cmSetVisibility(v); 
#else
  this->PartDisplay->SetVisibility(v);
#endif
  this->CubeAxesDisplayProxy->cmSetVisibility(v && cubeAxesVisibility);
  this->PointLabelDisplayProxy->cmSetVisibility(v && pointLabelVisibility);

  // Handle visibility of shared colormap.
  if (this->PVColorMap)
    {
    // Use count to manage color map visibility.
    if (v)
      {
      this->PVColorMap->IncrementUseCount();
      }
    else
      {
      this->PVColorMap->DecrementUseCount();
      }
    }

  // Update GUI.
  // Maybe the display GUI reference should be set to NULL
  // when the source is not current.
  if (this->Notebook)
    { // Notebook will only be set when this source is current.
    // We could update the whole notbook, but that would be wasteful.
    this->Notebook->GetDisplayGUI()->UpdateVisibilityCheck();
    }
  if ( this->GetPVRenderView() && this->GetPVWindow())
    {
    this->GetPVRenderView()->UpdateNavigationWindow(
                                this->GetPVWindow()->GetCurrentPVSource(), 0);
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::SetCubeAxesVisibility(int val)
{
  if (this->CubeAxesVisibility == val)
    {
    return;
    }
  this->AddTraceEntry("$kw(%s) SetCubeAxesVisibility %d", this->GetTclName(), val);
  this->SetCubeAxesVisibilityNoTrace(val);
}

//----------------------------------------------------------------------------
void vtkPVSource::SetCubeAxesVisibilityNoTrace(int val)
{
  if (this->CubeAxesVisibility == val)
    {
    return;
    }
  this->CubeAxesVisibility = val;
  this->CubeAxesDisplayProxy->cmSetVisibility(this->GetVisibility() && val);
  
  if (this->Notebook)
    {
    this->Notebook->GetDisplayGUI()->UpdateCubeAxesVisibilityCheck();
    }
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }  
}

//----------------------------------------------------------------------------
void vtkPVSource::SetPointLabelVisibility(int val)
{
  if (this->PointLabelVisibility == val)
    {
    return;
    }
  this->AddTraceEntry("$kw(%s) SetPointLabelVisibility %d", this->GetTclName(), val);
  this->SetPointLabelVisibilityNoTrace(val);
}

//----------------------------------------------------------------------------
void vtkPVSource::SetPointLabelVisibilityNoTrace(int val)
{
  if (this->PointLabelVisibility == val)
    {
    return;
    }
  this->PointLabelVisibility = val;
  this->PointLabelDisplayProxy->cmSetVisibility(this->GetVisibility() && val);
  
  if (this->Notebook)
    {
    this->Notebook->GetDisplayGUI()->UpdatePointLabelVisibilityCheck();
    }
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }  
}

//----------------------------------------------------------------------------
vtkPVDisplayGUI* vtkPVSource::GetPVOutput()
{
  return this->Notebook->GetDisplayGUI();
}

//----------------------------------------------------------------------------
int vtkPVSource::GetVisibility()
{
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  if (!this->DisplayProxy)
    {
    return 0;
    }
  return this->DisplayProxy->cmGetVisibility();
#else
  if ( this->GetPartDisplay() && this->GetPartDisplay()->GetVisibility() )
    {
    return 1;
    }
  return 0;
#endif
}

//----------------------------------------------------------------------------
void vtkPVSource::SetPVColorMap(vtkPVColorMap *colorMap)
{
  if (this->PVColorMap == colorMap)
    {
    return;
    }

  // Get rid of previous color map.
  if (this->PVColorMap)
    {
    // Use count to manage color map visibility.
    if (this->GetVisibility())
      {
      this->PVColorMap->DecrementUseCount();
      }
    this->PVColorMap->UnRegister(this);
    this->PVColorMap = NULL;
    }

  this->PVColorMap = colorMap;
  if (this->PVColorMap)
    {
    if (this->GetVisibility())
      {
      this->PVColorMap->IncrementUseCount();
      }
    this->PVColorMap->Register(this);
    }
    
  // Give the color map a chance to change the scalar range.
  if (this->PVColorMap)
    {
    this->PVColorMap->UpdateForSource(this);  
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::AcceptCallback()
{
  // This method is purposely not virtual.  The AcceptCallbackFlag
  // must be 1 for the duration of the accept callback no matter what
  // subclasses might do.  All of the real AcceptCallback functionality
  // should be implemented in AcceptCallbackInternal.
  this->AcceptCallbackFlag = 1;
  this->AcceptCallbackInternal();
  this->AcceptCallbackFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVSource::PreAcceptCallback()
{
  if ( ! this->Notebook->GetAcceptButtonRed())
    {
    return;
    }
  this->Script("%s configure -cursor watch",
               this->GetPVWindow()->GetWidgetName());
  this->Script("after idle {%s AcceptCallback}", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVSource::AcceptCallbackInternal()
{
  // I had trouble with this object destructing too early
  // in this method, because of an unregistered reference ...
  // It was an error condition (no output was created).
  this->Register(this);

  int dialogAlreadyUp=0;
  // This is here to prevent the user from killing the
  // application from inside the accept callback.
  if (!this->GetApplication()->GetDialogUp())
    {
    this->GetApplication()->SetDialogUp(1);
    }
  else
    {
    dialogAlreadyUp=1;
    }
  this->Accept(0);
  this->GetPVApplication()->AddTraceEntry("$kw(%s) AcceptCallback",
                                          this->GetTclName());
  if (!dialogAlreadyUp)
    {
    this->GetApplication()->SetDialogUp(0);
    }


  // I had trouble with this object destructing too early
  // in this method, because of an unregistered reference ...
  this->UnRegister(this);
}

//----------------------------------------------------------------------------
void vtkPVSource::Accept(int hideFlag, int hideSource)
{
  vtkPVWindow *window;
  vtkPVSource *input;

  // vtkPVSource is taking over some of the update decisions because
  // client does not have a real pipeline.
  if ( ! this->Notebook->GetAcceptButtonRed() )
    {
    return;
    } 

  this->GetPVApplication()->GetProcessModule()->SendPrepareProgress();

  window = this->GetPVWindow();

  this->Notebook->ShowPage("Display");
  this->Notebook->ShowPage("Information");
  this->Notebook->SetAcceptButtonColorToUnmodified();
  this->GetPVRenderView()->UpdateTclButAvoidRendering();
  
  // We need to pass the parameters from the UI to the VTK objects before
  // we check whether to insert ExtractPieces.  Otherwise, we'll get errors
  // about unspecified file names, etc., when ExecuteInformation is called on
  // the VTK source.  (The vtkPLOT3DReader is a good example of this.)
  this->UpdateVTKSourceParameters();

  this->MarkSourcesForUpdate();
    
  // Initialize the output if necessary.
  if ( ! this->Initialized )
    { // This is the first time, initialize data. 
    // I used to see if the display gui was properly set, but that was a legacy
    // check.  I removed the check.
     
    
    // Create the display and add it to the render module.
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
    vtkSMRenderModuleProxy* rm = this->GetPVApplication()->GetRenderModuleProxy();
    vtkSMDisplayProxy* pDisp = rm->CreateDisplayProxy();
#else
    vtkPVRenderModule* rm = 
        this->GetPVApplication()->GetProcessModule()->GetRenderModule();
    vtkSMPartDisplay *pDisp = rm->CreatePartDisplay();
#endif
    // Parts need to be created before we set source as inpout to part display.
    // This creates the proxy parts.
    this->InitializeData();

#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
    // Generally we cannot use Input property here....since it leads to a call
    // to CreateVTKObjects(1) (which is wrong in this case...). However,
    // vtkSMDisplayProxy does not create itself until the Input is set,
    // so this works fine. If fact, using it this was is essential,
    // so that the DisplayProxy is added as a consumer of the input automatically.
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      pDisp->GetProperty("Input"));
    pp->RemoveAllProxies();
    pp->AddProxy(this->GetProxy());
    pDisp->UpdateVTKObjects();

    this->AddDisplayToRenderModule(pDisp);
#else
    // Display needs an input.
    pDisp->SetInput(this->GetProxy());
    // Render module keeps a list of all the displays.
    rm->AddDisplay(pDisp);
#endif
    if (!this->GetSourceList())
      {
      vtkErrorMacro("SourceList should not be empty. "
                    "Cannot register display for animation.");
      }
    else
      {
      /* TODO: I am not registering Display as animateable....
      vtkSMProxyManager* proxm = vtkSMObject::GetProxyManager();
      ostrstream animName_with_warning_C4701;
      animName_with_warning_C4701 << this->GetSourceList() << "." 
                                  << this->GetName() << "."
                                  << "Display"
                                  << ends;
      proxm->RegisterProxy(
        "animateable", animName_with_warning_C4701.str(), pDisp);
      delete[] animName_with_warning_C4701.str();
      */
      }

    // Create the Cube Axes.
    this->CubeAxesDisplayProxy = vtkSMCubeAxesDisplayProxy::SafeDownCast(
      vtkSMObject::GetProxyManager()
      ->NewProxy("displays", "CubeAxesDisplay"));
    // Hookup cube axes display.
    vtkSMProxyProperty* ccpp = vtkSMProxyProperty::SafeDownCast(
      this->CubeAxesDisplayProxy->GetProperty("Input"));
    if (!ccpp)
      {
      vtkErrorMacro("Failed to find property Input on CubeAxesDisplayProxy.");
      }
    else
      {
      ccpp->AddProxy(this->Proxy);
      this->CubeAxesDisplayProxy->UpdateVTKObjects();
      }
    this->CubeAxesDisplayProxy->cmSetVisibility(0);
    this->AddDisplayToRenderModule(this->CubeAxesDisplayProxy);

    // Create the Point Label Display proxies.
    this->PointLabelDisplayProxy = vtkSMPointLabelDisplayProxy::SafeDownCast(
      vtkSMObject::GetProxyManager()
      ->NewProxy("displays", "PointLabelDisplay"));
    
    // Hookup point label display.
    ccpp = vtkSMProxyProperty::SafeDownCast(
      this->PointLabelDisplayProxy->GetProperty("Input"));
    if (!ccpp)
      {
      vtkErrorMacro("Failed to find property Input on PointLabelDisplayProxy");
      }
    else
      {
      ccpp->AddProxy(this->Proxy);
      this->PointLabelDisplayProxy->UpdateVTKObjects();
      }
    this->PointLabelDisplayProxy->cmSetVisibility(0);
    this->AddDisplayToRenderModule(this->PointLabelDisplayProxy);

    this->SetDisplayProxy(pDisp);
    pDisp->Delete();

    // Make the last data invisible.
    input = this->GetPVInput(0);
    if (input)
      {
      // I used to also check that the input was created by looking at the application of the DisplayGUI.
      if (this->ReplaceInput && hideSource)
        { // Application is set when the widget is created.
        input->SetVisibilityNoTrace(0);
        }
      }

    if (hideFlag)
      {
      this->SetVisibilityNoTrace(0);
      }

    // We need to update so we will have correct information for initialization.
    if (this->Notebook)
      {
      // Update the VTK data.
      this->Update();
      }

    // The best test I could come up with to only reset
    // the camera when the first source is created.
    if (window->GetSourceList("Sources")->GetNumberOfItems() == 1)
      {
      double bds[6];
      this->GetDataInformation()->GetBounds(bds);
      if (bds[0] <= bds[1] && bds[2] <= bds[3] && bds[4] <= bds[5])
        {
        window->SetCenterOfRotation(0.5*(bds[0]+bds[1]), 
                                    0.5*(bds[2]+bds[3]),
                                    0.5*(bds[4]+bds[5]));
        window->ResetCenterCallback();
        window->GetMainView()->GetRenderer()->ResetCamera(
          bds[0], bds[1], bds[2], bds[3], bds[4], bds[5]);
        }
      }

    //Law int fixme;
    // Quick fix to get paraview working again.  What does initiailze do?
    // Could we update instead.
    this->Notebook->GetDisplayGUI()->Initialize();
    // This causes input to be checked for validity.
    // I put it at the end so the InputFixedTypeRequirement will work.
    this->UnGrabFocus();
    this->SetDefaultColorParameters();
    // Has an implicit update.
    window->AddDefaultAnimation(this);

    // If a property called TimestepValues exists, automatically add
    // it to the annotation. In time, all readers that support time
    // (and know about the actual time values) should provide this
    // information.
    if (this->Proxy->GetProperty("TimestepValues"))
      {
      vtkPVCornerAnnotation* annot =
        this->GetPVRenderView()->GetCornerAnnotation();
      const char* prevText = annot->GetCornerText(1);
      if (prevText && prevText[0] == '\0')
        {
        prevText = 0;
        }
      ostrstream cornerText;
      if (prevText)
        {
        //cornerText << prevText << "\n";
        }
      cornerText 
        << "Time = [smGet Sources " << this->GetName()
        << " TimestepValues "
        << "[smGet Sources " << this->GetName()
        << " TimeStep] 13.5f]"
        << ends;
      annot->SetCornerText(cornerText.str(), 1);
      delete[] cornerText.str();
      annot->SetVisibility(1);
      }

    // One may be tempted to move this to the start of this if condition,
    // to avoid badly behaving PVWidgets reset calls to lead to
    // modification of the Accept button state, but that leads to 
    // crashes. So it's a good idea to recode the ResetInternal
    // method of the problematic PVWidget instead.
    this->Initialized = 1;
    }
  else
    {
    if (this->Notebook->GetDisplayGUI()->GetShouldReinitialize())
      {
      this->Notebook->GetDisplayGUI()->Initialize();
      this->SetDefaultColorParameters();
      }
    // This executes the filter (from update suppressor)
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
    vtkSMProperty* p = this->DisplayProxy->GetProperty("Update");
    p->Modified();
    this->DisplayProxy->UpdateVTKObjects();
#else
    this->PartDisplay->Update();
#endif
    this->GetPVWindow()->UpdateEnableState();
    }

  window->GetMenuView()->CheckRadioButton(
                                  window->GetMenuView(), "Radio", 2);
  this->UpdateProperties();
  this->GetPVRenderView()->EventuallyRender();

  // Update the selection menu.
  window->UpdateSelectMenu();

  // Regenerate the data property page in case something has changed.
  if (this->Notebook)
    {
    // Update the vtk data which has already been done ...
    this->Update();
    // Causes the data information to be updated if the filter executed.
    // Note has to be done here because tcl update causes render which
    // causes the filter to execute.
    this->Notebook->Update();  
    }

  this->GetPVRenderView()->UpdateTclButAvoidRendering();

#ifdef _WIN32
  this->Script("%s configure -cursor arrow", window->GetWidgetName());
#else
  this->Script("%s configure -cursor left_ptr", window->GetWidgetName());
#endif  

  this->GetPVApplication()->GetProcessModule()->SendCleanupPendingProgress();\
  
  // Make sure the buttons and filter menu reflect the new current source.
  // This did not happen when the source was selected because it was
  // not initialized.
  this->GetPVWindow()->UpdateEnableState();
}

//----------------------------------------------------------------------------
// Only call this when the source is current!
// The rules are:
// If the source created a NEW point scalar array, use it.
// Else if the source created a NEW cell scalar array, use it.
// Else if the input clolor by array exists in this source, use it.
// Else color by property.
void vtkPVSource::SetDefaultColorParameters()
{
  vtkPVSource* input = this->GetPVInput(0);
  vtkPVDataInformation* inDataInfo = 0;
  vtkPVDataSetAttributesInformation* inAttrInfo = 0;
  vtkPVArrayInformation* inArrayInfo = 0;
  vtkPVDataInformation* dataInfo;
  vtkPVDataSetAttributesInformation* attrInfo;
  vtkPVArrayInformation* arrayInfo;
  vtkPVColorMap* colorMap;  
    
  dataInfo = this->GetDataInformation();
  if (input)
    {
    inDataInfo = input->GetDataInformation();
    }
    
  // Inherit property color from input.
  if (input)
    {
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      input->GetDisplayProxy()->GetProperty("Color"));
    if (!dvp)
      {
      vtkErrorMacro("Failed to find property Color on input->DisplayProxy.");
      return;
      }
    double* rgb = dvp->GetElements();
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetDisplayProxy()->GetProperty("Color"));
    if (!dvp)
      {
      vtkErrorMacro("Failed to find property Color on DisplayProxy.");
      return;
      }   
    dvp->SetElements(rgb);
    this->GetDisplayProxy()->UpdateVTKObjects();
#else
    float rgb[3];
    input->GetPartDisplay()->GetColor(rgb);
    this->GetPartDisplay()->SetColor(rgb[0], rgb[1], rgb[2]);
#endif
    } 
    
  // Check for new point scalars.
  attrInfo = dataInfo->GetPointDataInformation();
  arrayInfo = attrInfo->GetAttributeInformation(vtkDataSetAttributes::SCALARS);
  if (arrayInfo)
    {
    if (inDataInfo)
      {
      inAttrInfo = inDataInfo->GetPointDataInformation();
      inArrayInfo = inAttrInfo->GetAttributeInformation(vtkDataSetAttributes::SCALARS);
      }
    if (inArrayInfo == 0 ||  strcmp(arrayInfo->GetName(),inArrayInfo->GetName()) != 0)
      { // No input or different scalars: use the new scalars.
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
      this->ColorByArray(arrayInfo->GetName(), vtkSMDisplayProxy::POINT_FIELD_DATA);
#else
      colorMap = this->GetPVWindow()->GetPVColorMap(arrayInfo->GetName(), 
                                        arrayInfo->GetNumberOfComponents());
      this->PartDisplay->ColorByArray(colorMap->GetProxyByName("LookupTable"), 
                                      vtkDataSet::POINT_DATA_FIELD);
#endif
      return;
      }
    }

  // Check for new cell scalars.
  attrInfo = dataInfo->GetCellDataInformation();
  arrayInfo = attrInfo->GetAttributeInformation(vtkDataSetAttributes::SCALARS);
  if (arrayInfo)
    {
    if (inDataInfo)
      {
      inAttrInfo = inDataInfo->GetCellDataInformation();
      inArrayInfo = inAttrInfo->GetAttributeInformation(vtkDataSetAttributes::SCALARS);
      }
    if (inArrayInfo == 0 ||  strcmp(arrayInfo->GetName(),inArrayInfo->GetName()) != 0)
      { // No input or different scalars: use the new scalars.
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
      this->ColorByArray(arrayInfo->GetName(), vtkSMDisplayProxy::CELL_FIELD_DATA);
#else
      colorMap = this->GetPVWindow()->GetPVColorMap(arrayInfo->GetName(), 
                                        arrayInfo->GetNumberOfComponents());
      this->PartDisplay->ColorByArray(colorMap->GetProxyByName("LookupTable"), 
                                      vtkDataSet::CELL_DATA_FIELD);
#endif
      return;
      }
    }

  // Try to use the same array selected by the input.
  if (input)
    {
    colorMap = input->GetPVColorMap();
    int colorField = -1;
    if (colorMap)
      {
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
      vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
        input->GetDisplayProxy()->GetProperty("ScalarMode"));
      if (ivp)
        {
        colorField = ivp->GetElement(0);
        }
#else
      colorField = input->GetPartDisplay()->GetColorField();
#endif
      // Find the array in our info.
      switch (colorField)
        {
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
      case vtkSMDisplayProxy::POINT_FIELD_DATA:
#else
        case vtkDataSet::POINT_DATA_FIELD:
#endif
          attrInfo = dataInfo->GetPointDataInformation();
          arrayInfo = attrInfo->GetArrayInformation(colorMap->GetArrayName());
          if (arrayInfo && colorMap->MatchArrayName(arrayInfo->GetName(),
                                       arrayInfo->GetNumberOfComponents()))
            {  
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
            this->ColorByArray(colorMap, vtkSMDisplayProxy::POINT_FIELD_DATA);
#else
            this->PartDisplay->ColorByArray(colorMap->GetProxyByName("LookupTable"), 
                                            vtkDataSet::POINT_DATA_FIELD);
#endif
            return;
            }
          break;
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
        case vtkSMDisplayProxy::CELL_FIELD_DATA:
#else
        case vtkDataSet::CELL_DATA_FIELD:
#endif
          attrInfo = dataInfo->GetCellDataInformation();
          arrayInfo = attrInfo->GetArrayInformation(colorMap->GetArrayName());
          if (arrayInfo && colorMap->MatchArrayName(arrayInfo->GetName(),
                                       arrayInfo->GetNumberOfComponents()))
            {  
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
            this->ColorByArray(colorMap, vtkSMDisplayProxy::CELL_FIELD_DATA);
#else
            this->PartDisplay->ColorByArray(colorMap->GetProxyByName("LookupTable"), 
                                            vtkDataSet::CELL_DATA_FIELD);
#endif
            return;
            }
          break;
        default:
          vtkErrorMacro("Bad attribute.");
          return;
        }

      }
    }

  // Color by property.
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  this->ColorByArray((vtkPVColorMap*)0, 0);
#else
  this->GetPartDisplay()->SetScalarVisibility(0);
#endif
}

//----------------------------------------------------------------------------
void vtkPVSource::ColorByArray(const char* arrayname, int field)
{
  if (field != vtkSMDisplayProxy::POINT_FIELD_DATA &&
    field != vtkSMDisplayProxy::CELL_FIELD_DATA)
    {
    vtkErrorMacro("Can color only with Point Field Data or Cell Field data.");
    return;
    }
  
  vtkPVDataInformation* dataInfo = this->GetDataInformation();
  vtkPVDataSetAttributesInformation* attrInfo =
    (field == vtkSMDisplayProxy::POINT_FIELD_DATA) ?
    dataInfo->GetPointDataInformation() :  dataInfo->GetCellDataInformation();
  vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(
    arrayname);
  if (!arrayInfo)
    {
    vtkErrorMacro("Failed to find " << arrayname);
    return;
    }

  vtkPVColorMap* colorMap = this->GetPVWindow()->GetPVColorMap(arrayname,
    arrayInfo->GetNumberOfComponents());
  this->ColorByArray( colorMap, field);
}

//----------------------------------------------------------------------------
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
void vtkPVSource::ColorByArray(vtkPVColorMap* colorMap, int field)
{
  vtkSMProxyProperty* pp;
  vtkSMIntVectorProperty* ivp;
  vtkSMDoubleVectorProperty* dvp;

  this->SetPVColorMap(colorMap);
  if (colorMap)
    {
    pp = vtkSMProxyProperty::SafeDownCast(
      this->DisplayProxy->GetProperty("LookupTable"));
    if (!pp)
      {
      vtkErrorMacro("Failed to find property LookupTable on vtkSMDisplayProxy.");
      return;
      }
    pp->RemoveAllProxies();
    pp->AddProxy(colorMap->GetProxyByName("LookupTable"));

    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->DisplayProxy->GetProperty("ScalarMode"));
    if (!ivp)
      {
      vtkErrorMacro("Failed to find property ScalarMode on vtkSMDisplayProxy.");
      return;
      }
    ivp->SetElement(0, field);

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->DisplayProxy->GetProperty("Specular"));
    if (!dvp)
      {
      vtkErrorMacro("Failed to find propery Specular on vtkSMDisplayProxy.");
      return;
      }
    // Turn off specular so that it does not interfere with data.
    dvp->SetElement(0, 0.0);

    vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
      colorMap->GetProxyByName("LookupTable")->GetProperty("ArrayName"));
    // TODO: should the array name be removed from the LookupTable?.
    // LookupTable has nothing to do with the arrayname.
    // Should the ColorMap keep it instead?
    if (!svp)
      {
      vtkErrorMacro("Failed to find property ArrayName on LookupTable.");
      return;
      }
    vtkSMStringVectorProperty* d_svp = vtkSMStringVectorProperty::SafeDownCast(
      this->DisplayProxy->GetProperty("ColorArray"));
    if (!d_svp)
      {
      vtkErrorMacro("Failed to find property ColorArray on DisplayProxy.");
      return;
      }
    d_svp->SetElement(0, svp->GetElement(0));
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->DisplayProxy->GetProperty("ScalarVisibility"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ScalarVisibility on vtkSMDisplayProxy.");
    return;
    }
  if (colorMap)
    {
    ivp->SetElement(0, 1);
    }
  else
    {
    ivp->SetElement(0, 0);
    }
  this->DisplayProxy->UpdateVTKObjects();
}

#endif
//----------------------------------------------------------------------------
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
// TODO: may be this should be a convienience method instead.
void vtkPVSource::VolumeRenderByArray(const char* arrayname, int field)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->DisplayProxy->GetProperty("ScalarMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ScalarMode on DisplayProxy.");
    return;
    }
  ivp->SetElement(0, field);

  this->DisplayProxy->UpdateVTKObjects();

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->DisplayProxy->GetProperty("SelectScalarArray"));
  if (!svp)
    {
    vtkErrorMacro("Failed to find property SelectScalarArray on DisplayProxy.");
    return;
    }
  svp->SetElement(0, arrayname);
  this->DisplayProxy->UpdateVTKObjects();

  vtkSMProperty* p = this->DisplayProxy->GetProperty("ResetTransferFunctions");
  if (!p)
    {
    vtkErrorMacro("Failed to find property ResetTransferFunctions on DisplayProxy.");
    return;
    }
  p->Modified(); // immediate_update property.
}
#endif
//----------------------------------------------------------------------------
void vtkPVSource::MarkSourcesForUpdate()
{
  int idx;
  vtkPVSource* consumer;

  this->InvalidateDataInformation();
  this->Proxy->MarkConsumersAsModified();

  // Get rid of caches.
  int numParts;
  vtkSMPart *part;
  numParts = this->GetNumberOfParts();
  for (idx = 0; idx < numParts; ++idx)
    {
    part = this->GetPart(idx);
    part->MarkForUpdate();
    }

  for (idx = 0; idx < this->NumberOfPVConsumers; ++idx)
    {
    consumer = this->GetPVConsumer(idx);
    consumer->MarkSourcesForUpdate();
    }  

}

//----------------------------------------------------------------------------
void vtkPVSource::ResetCallback()
{
  this->Reset();
  if (this->Initialized)
    {
    this->GetPVRenderView()->EventuallyRender();
    this->Script("update");

    this->Notebook->SetAcceptButtonColorToUnmodified();
    }
}

//---------------------------------------------------------------------------
void vtkPVSource::DeleteCallback()
{
  int i;
  int initialized = this->Initialized;
  vtkPVSource *prev = NULL;
  vtkPVSource *current = 0;
  vtkPVWindow *window = this->GetPVWindow();

  current = window->GetCurrentPVSource();
  window->SetCurrentPVSourceCallback(0);
  if (this->GetNumberOfPVConsumers() > 0 )
    {
    vtkErrorMacro("An output is used.  We cannot delete this source.");
    return;
    }

  // Do this to here to release resource (decrement use count).
  this->SetPVColorMap(0);

  window->GetAnimationInterface()->DeleteSource(this);

  if (this->Notebook)
    { // Delete call back set the cube axes visibility 
    // and point label visibility off.
    // Cube axes is controled by this source so we could handle it.
    // Point label visibility has not been made into a display yet ...
    this->Notebook->GetDisplayGUI()->DeleteCallback();
    
    // Deleted source was leaving the accept button green.
    this->Notebook->SetAcceptButtonColorToUnmodified();
    }

  // Just in case cursor was left in a funny state.
#ifdef _WIN32
  this->Script("%s configure -cursor arrow", window->GetWidgetName());
#else
  this->Script("%s configure -cursor left_ptr", window->GetWidgetName());
#endif  

  if ( ! this->Initialized)
    {
    // Remove the local grab
    this->UnGrabFocus();
    this->Script("update");   
    this->Initialized = 1;
    }
    
  // Save this action in the trace file.
  this->GetPVApplication()->AddTraceEntry("$kw(%s) DeleteCallback",
                                          this->GetTclName());

  // Get the input so we can make it visible and make it current.
  if (this->GetNumberOfPVInputs() > 0)
    {
    prev = this->PVInputs[0];
    // Just a sanity check
    if (prev == NULL)
      {
      vtkErrorMacro("Expecting an input but none found.");
      }
    else
      {
      prev->SetVisibilityNoTrace(1);
      }
    }

  // Remove this source from the inputs users collection.
  for (i = 0; i < this->GetNumberOfPVInputs(); i++)
    {
    if (this->PVInputs[i])
      {
      this->PVInputs[i]->RemovePVConsumer(this);
      }
    }
    
  // Look for a source to make current.
  if (prev == NULL)
    {
    prev = this->GetPVWindow()->GetPreviousPVSource();
    }
  // Just remember it. We set the current pv source later.
  //this->GetPVWindow()->SetCurrentPVSourceCallback(prev);
  if ( prev == NULL && window->GetSourceList("Sources")->GetNumberOfItems() > 0 )
    {
    vtkCollectionIterator *it = window->GetSourceList("Sources")->NewIterator();
    it->InitTraversal();
    while ( !it->IsDoneWithTraversal() )
      {
      prev = static_cast<vtkPVSource*>( it->GetCurrentObject() );
      if ( prev != this )
        {
        break;
        }
      else
        {
        prev = 0;
        }
      it->GoToNextItem();
      }
    it->Delete();
    }

  if ( this == current || window->GetSourceList("Sources")->GetNumberOfItems() == 1)
    {
    current = prev;
  
    if (prev == NULL)
      {
      // Show the 3D View settings
      window->GetMainView()->ShowViewProperties();
      }
    }
        
  // Remove all of the actors mappers. from the renderer.
  if (this->Notebook)
    {
    vtkSMDisplayProxy* pDisp = this->GetDisplayProxy();
    if (pDisp)
      {
      this->RemoveDisplayFromRenderModule(pDisp);
      }
    this->RemoveDisplayFromRenderModule(this->CubeAxesDisplayProxy);
    this->RemoveDisplayFromRenderModule(this->PointLabelDisplayProxy);
    }

  // I doubt this is necessary (may to break a reference loop).
  if (this->Notebook)
    {
    this->Notebook->SetPVSource(0);
    }
  this->SetNotebook(0);
  
  if ( initialized )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
  
  // This should delete this source.
  // "this" will no longer be valid after the call.
  window->RemovePVSource("Sources", this);
  window->SetCurrentPVSourceCallback(current);

  // Make sure the menus reflect this change.
  // I could also use UpdateFilterMenu.  
  window->UpdateEnableState();
}

//----------------------------------------------------------------------------
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
void vtkPVSource::AddDisplayToRenderModule(vtkSMDisplayProxy* pDisp)
{
  vtkSMRenderModuleProxy* rm = this->GetPVApplication()->GetRenderModuleProxy();
  if (!rm)
    {
    return;
    }
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    rm->GetProperty("Displays"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Displays on vtkSMRenderModuleProxy.");
    return;
    }
  pp->AddProxy(pDisp);
  rm->UpdateVTKObjects(); 
}
#endif
//----------------------------------------------------------------------------
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
void vtkPVSource::RemoveDisplayFromRenderModule(vtkSMDisplayProxy* pDisp)
{
  vtkSMRenderModuleProxy* rm = this->GetPVApplication()->GetRenderModuleProxy();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    rm->GetProperty("Displays"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Displays on vtkSMRenderModuleProxy.");
    return;
    }
  pp->RemoveProxy(pDisp);
  rm->UpdateVTKObjects();
}
#endif


//----------------------------------------------------------------------------
void vtkPVSource::Reset()
{
  vtkPVWidget *pvw;
  vtkCollectionIterator *it = this->Widgets->NewIterator();
  it->InitTraversal();
  while( !it->IsDoneWithTraversal() )
    {
    pvw = static_cast<vtkPVWidget*>(it->GetCurrentObject());
    // Do not try to reset the widget if it is not initialized
    if (pvw && (pvw->GetModifiedFlag() || !this->Initialized) || 
        !this->DataInformationValid)
      {
      pvw->Reset();
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateVTKObjects()
{
 if (this->Proxy)
    {
    this->Proxy->UpdateVTKObjects();
    }

  vtkCollectionIterator *it = this->Widgets->NewIterator();
  while( !it->IsDoneWithTraversal() )
    {
    vtkPVWidget* pvw = static_cast<vtkPVWidget*>(it->GetCurrentObject());
    if (pvw)
      {
      pvw->UpdateVTKObjects();
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
// This should be apart of AcceptCallbackInternal.
void vtkPVSource::UpdateVTKSourceParameters()
{
  vtkPVWidget *pvw;
  vtkCollectionIterator *it;

  it = this->Widgets->NewIterator();
  it->InitTraversal();
  while( !it->IsDoneWithTraversal() )
    {
    pvw = static_cast<vtkPVWidget*>(it->GetCurrentObject());
    if (pvw && (!this->Initialized || pvw->GetModifiedFlag()))
      {
      pvw->Accept();
      }
    it->GoToNextItem();
    }

  if (this->Proxy)
    {
    this->Proxy->UpdateVTKObjects();
    }

  it->InitTraversal();
  while( !it->IsDoneWithTraversal() )
    {
    pvw = static_cast<vtkPVWidget*>(it->GetCurrentObject());
    if (pvw)
      {
      pvw->PostAccept();
      }
    it->GoToNextItem();
    }

  it->Delete();
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateProperties()
{
  this->UpdateEnableState();
  if ( this->Notebook )
    {
    this->Notebook->Update();
    }
}

//----------------------------------------------------------------------------
int vtkPVSource::IsDeletable()
{
  if (this->IsPermanent || this->GetNumberOfPVConsumers() > 0 )
    {
    return 0;
    }

  return 1;
}
  
//---------------------------------------------------------------------------
void vtkPVSource::SetNumberOfPVInputs(int num)
{
  int idx;
  vtkPVSource** inputs;

  // in case nothing has changed.
  if (num == this->NumberOfPVInputs)
    {
    return;
    }
  
  // Allocate new arrays.
  inputs = new vtkPVSource* [num];

  // Initialize with NULLs.
  for (idx = 0; idx < num; ++idx)
    {
    inputs[idx] = NULL;
    }

  // Copy old inputs
  for (idx = 0; idx < num && idx < this->NumberOfPVInputs; ++idx)
    {
    inputs[idx] = this->PVInputs[idx];
    }
  
  // delete the previous arrays
  if (this->PVInputs)
    {
    delete [] this->PVInputs;
    this->PVInputs = NULL;
    this->NumberOfPVInputs = 0;
    }
  
  // Set the new array
  this->PVInputs = inputs;
  
  this->NumberOfPVInputs = num;
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkPVSource::SetNthPVInput(int idx, vtkPVSource *pvs)
{
  if (idx < 0)
    {
    vtkErrorMacro(<< "SetNthPVInput: " << idx << ", cannot set input. ");
    return;
    }
  
  // Expand array if necessary.
  if (idx >= this->NumberOfPVInputs)
    {
    this->SetNumberOfPVInputs(idx + 1);
    }
  
  // Does this change anything?  Yes, it keeps the object from being modified.
  if (pvs == this->PVInputs[idx])
    {
    return;
    }
  
  if (this->PVInputs[idx])
    {
    this->PVInputs[idx]->RemovePVConsumer(this);
    this->PVInputs[idx]->UnRegister(this);
    this->PVInputs[idx] = NULL;
    }
  
  if (pvs)
    {
    pvs->Register(this);
    pvs->AddPVConsumer(this);
    this->PVInputs[idx] = pvs;
    }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkPVSource::RemoveAllPVInputs()
{
  if ( this->PVInputs )
    {
    int idx;
    for (idx = 0; idx < this->NumberOfPVInputs; ++idx)
      {
      this->SetNthPVInput(idx, NULL);
      }

    delete [] this->PVInputs;
    this->PVInputs = NULL;
    this->NumberOfPVInputs = 0;

    // Make sure to disconnect all VTK filters as well
    vtkPVApplication *pvApp = this->GetPVApplication();
    if (pvApp)
      {
      vtkPVProcessModule* pm = pvApp->GetProcessModule();

      int numSources = this->GetNumberOfVTKSources();
      vtkClientServerStream stream;
      for (idx = 0; idx < numSources; ++idx)
        {
        vtkClientServerID sourceID = this->GetVTKSourceID(idx);
        stream << vtkClientServerStream::Invoke 
               << sourceID << "SetInputConnection" << 0 << 0
               << vtkClientServerStream::End;
        }
      pm->SendStream(vtkProcessModule::DATA_SERVER, stream);

      if (this->Proxy)
        {
        vtkSMPropertyIterator* iter = this->Proxy->NewPropertyIterator();
        iter->Begin();
        while (!iter->IsAtEnd())
          {
          vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
            iter->GetProperty());
          if (ip)
            {
            ip->RemoveAllProxies();
            }
          iter->Next();
          }
        iter->Delete();
        }

      }

    this->Modified();
    }
}

//---------------------------------------------------------------------------
vtkPVSource *vtkPVSource::GetNthPVInput(int idx)
{
  if (idx >= this->NumberOfPVInputs)
    {
    return NULL;
    }
  
  return (vtkPVSource *)(this->PVInputs[idx]);
}

//----------------------------------------------------------------------------
void vtkPVSource::SaveInBatchScript(ofstream *file)
{
  // This should not be needed, but We can check anyway.
  if (this->VisitedFlag)
    {
    return;
    }

  this->SaveFilterInBatchScript(file);
  // Add the mapper, actor, scalar bar actor ...
  if (this->GetVisibility())
    {
    if (this->PVColorMap)
      {
      this->PVColorMap->SaveInBatchScript(file);
      }
    
    vtkSMDisplayProxy* pDisp = this->GetDisplayProxy();
    if (pDisp)
      {
      *file << "#Display Proxy" << endl;
      pDisp->SaveInBatchScript(file);
      }
    
    if (this->GetCubeAxesVisibility())
      {
      *file << "#Cube Axes Display" << endl;
      this->CubeAxesDisplayProxy->SaveInBatchScript(file);
      }

    if (this->GetPointLabelVisibility())
      {
      *file << "#Point Label display" << endl;
      this->PointLabelDisplayProxy->SaveInBatchScript(file);
      }
    }
}  


//----------------------------------------------------------------------------
void vtkPVSource::SaveFilterInBatchScript(ofstream *file)
{
  int i;

  // Detect special sources we do not handle yet.
  if (this->GetSourceClassName() == NULL)
    {
    return;
    }

  // This is the recursive part.
  this->VisitedFlag = 1;
  // Loop through all of the inputs
  for (i = 0; i < this->NumberOfPVInputs; ++i)
    {
    if (this->PVInputs[i] && this->PVInputs[i]->GetVisitedFlag() != 2)
      {
      this->PVInputs[i]->SaveInBatchScript(file);
      }
    }
  
  // Save the object in the script.
  *file << "\n"; 
  const char* module_group = 0;
  vtkPVSource* input0 = this->GetPVInput(0);
  if (input0)
    {
    module_group = "filters";
    }
  else
    {
    module_group = "sources";
    }
  *file << "set pvTemp" <<  this->GetVTKSourceID(0)
        << " [$proxyManager NewProxy " << module_group << " " 
        << this->GetModuleName() << "]"
        << endl;
  *file << "  $proxyManager RegisterProxy " << module_group << " pvTemp" 
        << this->GetVTKSourceID(0) << " $pvTemp" << this->GetVTKSourceID(0)
        << endl;
  *file << "  $pvTemp" << this->GetVTKSourceID(0) << " UnRegister {}" << endl;

  this->SetInputsInBatchScript(file);

  // Let the PVWidgets set up the object.
  vtkCollectionIterator *it = this->Widgets->NewIterator();
  vtkPVWidget *pvw;
  it->InitTraversal();
  while ( !it->IsDoneWithTraversal() )
    {
    pvw = static_cast<vtkPVWidget*>(it->GetCurrentObject());
    pvw->SaveInBatchScript(file);
    it->GoToNextItem();
    }
  it->Delete();

  *file << "  $pvTemp" <<  this->GetVTKSourceID(0)
        << " UpdateVTKObjects" << endl;
}

//----------------------------------------------------------------------------
void vtkPVSource::SaveStateVisibility(ofstream *file)
{

  *file << "$kw(" << this->GetTclName() << ") SetVisibility " 
        << this->GetVisibility() << endl;
}

//----------------------------------------------------------------------------
void vtkPVSource::SaveStateDisplay(ofstream *file)
{
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  return;
#else
  // Set the color map here for simplicity.
  if (this->PVColorMap)
    {
    if (this->GetPartDisplay()->GetColorField() == vtkDataSet::POINT_DATA_FIELD)
      {
      *file << "[$kw(" << this->GetTclName()
            << ") GetPVOutput] ColorByPointField {" 
            << this->PVColorMap->GetArrayName() << "} " 
            << this->PVColorMap->GetNumberOfVectorComponents() << endl;
      }
    if (this->GetPartDisplay()->GetColorField() == vtkDataSet::CELL_DATA_FIELD)
      {
      *file << "[$kw(" << this->GetTclName()
            << ") GetPVOutput] ColorByCellField {" 
            << this->PVColorMap->GetArrayName() << "} " 
            << this->PVColorMap->GetNumberOfVectorComponents() << endl;
      }
    }
  else
    {
    *file << "[$kw(" << this->GetTclName()
          << ") GetPVOutput] ColorByProperty\n";
    }

  // Add command to switch to Volume Render mode if required
  this->GetPVOutput()->SaveVolumeRenderStateDisplay(file);

  // Save the options from the display GUI.
  char dispTclName[512];
  sprintf(dispTclName, "$pvDisp(%s)", this->GetTclName());
  *file << "set " << dispTclName+1 
        << " [$kw(" << this->GetTclName() << ") GetPartDisplay]" << endl;
  vtkIndent indent;
  this->GetPartDisplay()->SavePVState(file, dispTclName, indent);

  *file << "$kw(" << this->GetTclName()
        << ") SetCubeAxesVisibility " << this->GetCubeAxesVisibility() << endl; 

  *file << "$kw(" << this->GetTclName()
        << ") SetPointLabelVisibility " << this->GetPointLabelVisibility() << endl; 

  // Make sure the GUI is upto date.  
  *file << "[$kw(" << this->GetTclName()
        << ") GetPVOutput] Update\n"; 
#endif  
}
  
//----------------------------------------------------------------------------
void vtkPVSource::SaveState(ofstream *file)
{
  int i, numWidgets;
  vtkPVWidget *pvw;

  // Detect if this source is in Glyph sourcesm and already exists.
  if (this->GetTraceReferenceCommand())
    {
    *file << "set kw(" << this->GetTclName() << ") [$kw(" 
          << this->GetTraceReferenceObject()->GetTclName() << ") " 
          << this->GetTraceReferenceCommand() << "]\n";
    return;
    }

  // This should not be needed, but We can check anyway.
  if (this->VisitedFlag)
    {
    return;
    }

  // This is the recursive part.
  this->VisitedFlag = 1;
  // Loop through all of the inputs
  for (i = 0; i < this->NumberOfPVInputs; ++i)
    {
    if (this->PVInputs[i] && this->PVInputs[i]->GetVisitedFlag() != 2)
      {
      this->PVInputs[i]->SaveState(file);
      }
    }
  
  // We have to set the first input as the current source,
  // because CreatePVSource uses it as default input.
  // We may not have a input menu to set it for us.
  if (this->GetPVInput(0))
    {
    *file << "$kw(" << this->GetPVWindow()->GetTclName() << ") "
          << "SetCurrentPVSourceCallback $kw("
          << this->GetPVInput(0)->GetTclName() << ")\n";
    }

  // Save the object in the script.
  *file << "set kw(" << this->GetTclName() << ") "
        << "[$kw(" << this->GetPVWindow()->GetTclName() << ") "
        << "CreatePVSource " << this->GetModuleName() << "]" << endl;

  *file << "$kw(" << this->GetTclName() << ") SetLabel {" 
        << this->GetLabel() << "}" << endl;

  // Let the PVWidgets set up the object.
  numWidgets = this->Widgets->GetNumberOfItems();
  vtkCollectionIterator *it = this->Widgets->NewIterator();
  it->InitTraversal();
  for (i = 0; i < numWidgets; i++)
    {
    pvw = static_cast<vtkPVWidget*>(it->GetCurrentObject());
    pvw->SaveState(file);
    it->GoToNextItem();
    }
  it->Delete();
  
  // Call accept.
  *file << "$kw(" << this->GetTclName() << ") AcceptCallback" << endl;

  this->SaveStateDisplay(file);
}

//----------------------------------------------------------------------------
void vtkPVSource::SetInputsInBatchScript(ofstream *file)
{
  int numInputs = this->GetNumberOfPVInputs();

  for (int inpIdx=0; inpIdx<numInputs; inpIdx++)
    {
    // Just PVInput 0 for now.
    vtkPVSource* pvs = this->GetNthPVInput(inpIdx);

    // Set the VTK reference to the new input.
    const char* inputName;
    vtkPVInputProperty* ip=0;
    if (this->VTKMultipleInputsFlag)
      {
      ip = this->GetInputProperty(0);
      }
    else
      {
      ip = this->GetInputProperty(inpIdx);
      }

    if (ip)
      {
      inputName = ip->GetName();
      }
    else
      {
      vtkErrorMacro("No input property defined, setting to default.");
      inputName = "Input";
      }

    *file << "  [$pvTemp" <<  this->GetVTKSourceID(0) 
          << " GetProperty " << inputName << "]"
          << " AddProxy $pvTemp" << pvs->GetVTKSourceID(0)
          << endl;
    }

}

//----------------------------------------------------------------------------
void vtkPVSource::AddPVWidget(vtkPVWidget *pvw)
{
  char str[512];
  this->Widgets->AddItem(pvw);

  if (pvw->GetTraceName() == NULL)
    {
    vtkWarningMacro("TraceName not set. Widget class: " 
                    << pvw->GetClassName());
    return;
    }

  pvw->SetTraceReferenceObject(this);
  sprintf(str, "GetPVWidget {%s}", pvw->GetTraceName());
  pvw->SetTraceReferenceCommand(str);
}

//----------------------------------------------------------------------------
vtkIdType vtkPVSource::GetNumberOfInputProperties()
{
  return this->InputProperties->GetNumberOfItems();
}

//----------------------------------------------------------------------------
vtkPVInputProperty* vtkPVSource::GetInputProperty(int idx)
{
  return static_cast<vtkPVInputProperty*>(this->InputProperties->GetItemAsObject(idx));
}

//----------------------------------------------------------------------------
vtkPVInputProperty* vtkPVSource::GetInputProperty(const char* name)
{
  int idx, num;
  vtkPVInputProperty *inProp;
  
  num = this->GetNumberOfInputProperties();
  for (idx = 0; idx < num; ++idx)
    {
    inProp = static_cast<vtkPVInputProperty*>(this->GetInputProperty(idx));
    if (strcmp(name, inProp->GetName()) == 0)
      {
      return inProp;
      }
    }

  // Propery has not been created yet.  Create and  save one.
  inProp = vtkPVInputProperty::New();
  inProp->SetName(name);
  this->InputProperties->AddItem(inProp);
  inProp->Delete();

  return inProp;
}

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVSource::GetPVWidget(const char *name)
{
  vtkObject *o;
  vtkPVWidget *pvw;
  this->Widgets->InitTraversal();
  
  while ( (o = this->Widgets->GetNextItemAsObject()) )
    {
    pvw = vtkPVWidget::SafeDownCast(o);
    if (pvw && pvw->GetTraceName() && strcmp(pvw->GetTraceName(), name) == 0)
      {
      return pvw;
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVSource::GetPVRenderView()
{
  return vtkPVRenderView::SafeDownCast(this->View);
}

//----------------------------------------------------------------------------
int vtkPVSource::GetNumberOfProcessorsValid()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  if (!pvApp)
    {
    return 0;
    }
  int numProcs = pvApp->GetProcessModule()->GetNumberOfPartitions();
  
  switch (this->VTKMultipleProcessFlag)
    {
    case 0:
      if (numProcs > 1)
        {
        return 0;
        }
      break;
    case 1:
      if (numProcs == 1)
        {
        return 0;
        }
      break;
    case 2:
      break;
    default:
      return 0;
    }
  return 1;
}
//----------------------------------------------------------------------------
int vtkPVSource::CloneAndInitialize(int makeCurrent, vtkPVSource*& clone)
{
  int retVal = this->ClonePrototypeInternal(clone);
  if (retVal != VTK_OK)
    {
    return retVal;
    }

  retVal = clone->InitializeClone(makeCurrent);


  if (retVal != VTK_OK)
    {
    clone->Delete();
    clone = 0;
    return retVal;
    }

  clone->Notebook->SetAcceptButtonColorToModified();

  return VTK_OK;
}

//----------------------------------------------------------------------------
void vtkPVSource::RegisterProxy(const char* sourceList, vtkPVSource* clone)
{
  const char* module_group = 0;
  if (sourceList)
    {
    if (strcmp(sourceList, "GlyphSources") == 0)
      {
        module_group = "glyph_sources";
      }
    else
      {
      module_group = sourceList;
      }
    }
  else if (this->GetNumberOfInputProperties() > 0)
    {
    module_group = "filters";
    }
  else
    {
    module_group = "sources";
    }

  vtkSMProxyManager* proxm = vtkSMObject::GetProxyManager();
  proxm->RegisterProxy(module_group, clone->GetName(), clone->Proxy);

  if (!sourceList)
    {
    sourceList = "Sources";
    }
  ostrstream animName_with_warning_C4701;
  animName_with_warning_C4701 << sourceList << "." 
                              << clone->GetName()
                              << ends;
  proxm->RegisterProxy(
    "animateable", animName_with_warning_C4701.str(), clone->Proxy);
  delete[] animName_with_warning_C4701.str();

  clone->SetSourceList(sourceList);
}

//----------------------------------------------------------------------------
int vtkPVSource::ClonePrototype(vtkPVSource*& clone)
{
  return this->ClonePrototypeInternal(clone);
}

//----------------------------------------------------------------------------
int vtkPVSource::ClonePrototypeInternal(vtkPVSource*& clone)
{
  int idx;

  clone = 0;

  vtkPVSource* pvs = this->NewInstance();
  // Copy properties
  pvs->SetApplication(this->GetApplication());
  pvs->SetReplaceInput(this->ReplaceInput);
  pvs->SetNotebook(this->Notebook);
  // Copy OverideAutoAccept
  pvs->SetOverideAutoAccept( this->GetOverideAutoAccept());

  pvs->SetShortHelp(this->GetShortHelp());
  pvs->SetLongHelp(this->GetLongHelp());
  pvs->SetVTKMultipleInputsFlag(this->GetVTKMultipleInputsFlag());

  pvs->SetSourceClassName(this->SourceClassName);

  // Create a (unique) name for the PVSource.
  // Beware: If two prototypes have the same name, the name 
  // will not be unique.
  // Does this name really have to be unique?
  char tclName[1024];
  if (this->Name && this->Name[0] != '\0')
    { 
    sprintf(tclName, "%s%d", this->Name, this->PrototypeInstanceCount);
    }
  else
    {
    vtkErrorMacro("The prototype must have a name. Cloning aborted.");
    pvs->Delete();
    return VTK_ERROR;
    }
  pvs->SetName(tclName);

  vtkSMProxyManager* proxm = vtkSMObject::GetProxyManager();

  const char* module_group = 0;
  if (this->GetNumberOfInputProperties() > 0)
    {
    module_group = "filters";
    }
  else
    {
    module_group = "sources";
    }

  pvs->Proxy = vtkSMSourceProxy::SafeDownCast(
    proxm->NewProxy(module_group, this->GetModuleName()));
  pvs->Proxy->Register(pvs);
  pvs->Proxy->Delete();
  if (!pvs->Proxy)
    {
    vtkErrorMacro("Can not create " 
                  << (this->GetModuleName()?this->GetModuleName():"(nil)")
                  << " : " << module_group);
    pvs->Delete();
    return VTK_ERROR;
    }
  this->RegisterProxy(this->SourceList, pvs);


  // Copy the VTK input stuff.
  vtkIdType numItems = this->GetNumberOfInputProperties();
  vtkIdType id;
  vtkPVInputProperty *inProp;
  for(id=0; id<numItems; id++)
    {
    inProp = this->GetInputProperty(id);
    pvs->GetInputProperty(inProp->GetName())->Copy(inProp);
    }

  pvs->SetModuleName(this->ModuleName);

  vtkPVApplication* pvApp = 
    vtkPVApplication::SafeDownCast(pvs->GetApplication());


  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  
  // We need oue source for each part.
  int numSources = 1;
  // Set the input if necessary.
  if (this->GetNumberOfInputProperties() > 0)
    {
    vtkPVSource *input = this->GetPVWindow()->GetCurrentPVSource();
    numSources = input->GetNumberOfParts();
    }
  // If the VTK filter takes multiple inputs (vtkAppendPolyData)
  // then we create only one filter with each part as an input.
  if (this->GetVTKMultipleInputsFlag())
    {
    numSources = 1;
    }

  // Force the proxy to create numSources objects
  pvs->Proxy->GetID(numSources-1);

  vtkClientServerStream stream;
  for (idx = 0; idx < numSources; ++idx)
    {
    // Create a vtkSource
    vtkClientServerID sourceId = pvs->Proxy->GetID(idx);

    // Keep track of how long each filter takes to execute.
    ostrstream filterName_with_warning_C4701;
    filterName_with_warning_C4701 << "Execute " << this->SourceClassName
                                  << " id: " << sourceId.ID << ends;
    vtkClientServerStream start;
    start << vtkClientServerStream::Invoke << pm->GetProcessModuleID() 
          << "LogStartEvent" << filterName_with_warning_C4701.str()
          << vtkClientServerStream::End;
    vtkClientServerStream end;
    end << vtkClientServerStream::Invoke << pm->GetProcessModuleID() 
        << "LogEndEvent" << filterName_with_warning_C4701.str()
        << vtkClientServerStream::End;
    delete[] filterName_with_warning_C4701.str();

    stream << vtkClientServerStream::Invoke 
           << sourceId << "AddObserver" << "StartEvent" << start
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke 
           << sourceId << "AddObserver" << "EndEvent" << end
           << vtkClientServerStream::End;
    
    }
  pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
  pvs->SetView(this->GetPVWindow()->GetMainView());

  pvs->PrototypeInstanceCount = this->PrototypeInstanceCount;
  this->PrototypeInstanceCount++;

  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* widgetMap =
    vtkArrayMap<vtkPVWidget*, vtkPVWidget*>::New();


  // Copy all widgets
  vtkPVWidget *pvWidget, *clonedWidget;
  vtkCollectionIterator *it = this->Widgets->NewIterator();
  it->InitTraversal();

  int i;
  for (i = 0; i < this->Widgets->GetNumberOfItems(); i++)
    {
    pvWidget = static_cast<vtkPVWidget*>(it->GetCurrentObject());
    clonedWidget = pvWidget->ClonePrototype(pvs, widgetMap);
    pvs->AddPVWidget(clonedWidget);
    clonedWidget->Delete();
    it->GoToNextItem();
    }
  widgetMap->Delete();
  it->Delete();

  clone = pvs;
  return VTK_OK;
}

//----------------------------------------------------------------------------
void vtkPVSource::InitializeWidgets()

{
  vtkPVWidget *pvw;
  vtkCollectionIterator *it;
  
  it = this->Widgets->NewIterator();

  // First initialize the input widgets (since others tend to depend
  // on input being set)
  it->InitTraversal();
  while( !it->IsDoneWithTraversal() )
    {
    pvw = static_cast<vtkPVWidget*>(it->GetCurrentObject());
    if (pvw && vtkPVInputMenu::SafeDownCast(pvw))
      {
      pvw->Initialize();
      }
    it->GoToNextItem();
    }

  // Next initialize the rest
  it->InitTraversal();
  while( !it->IsDoneWithTraversal() )
    {
    pvw = static_cast<vtkPVWidget*>(it->GetCurrentObject());
    if (pvw && !vtkPVInputMenu::SafeDownCast(pvw))
      {
      pvw->Initialize();
      }
    it->GoToNextItem();
    }
  it->Delete();
  
}

//----------------------------------------------------------------------------
int vtkPVSource::InitializeClone(int makeCurrent)

{
  // Create the properties frame etc.
  this->CreateProperties();

  this->InitializeWidgets();

  // If this is not a reader module, call UpdateInformation.
  // Some filters may require UpdateInformation is called to
  // initialize widgets.
  // Unfortunately, there are some filters/modules with which
  // this does not work. These are avoided.
  if (!this->IsA("vtkPVReaderModule") && !this->IsA("vtkPVProbe"))
    {
    this->Proxy->UpdateInformation();
    }

  // Display page must be created before source is selected.
  if (makeCurrent)
    {
    this->ResetInSelect = 0;
    this->GetPVWindow()->SetCurrentPVSourceCallback(this);
    this->ResetInSelect = 1;
    } 
    
  // Show the source page, and hide the display and information pages.
  if (this->Notebook)
    {
    this->Notebook->Raise("Parameters");
    this->Notebook->HidePage("Display");
    this->Notebook->HidePage("Information");
    }    

  return VTK_OK;
}

//----------------------------------------------------------------------------
// This stuff used to be in Initialize clone.
// I want to initialize the output after accept is called.
int vtkPVSource::InitializeData()
{
  //law int fixme;  //I would like to get rid of these part references.

  this->Proxy->CreateParts();

  return VTK_OK;
}

//----------------------------------------------------------------------------
void vtkPVSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Initialized: " << (this->Initialized?"yes":"no") << endl;
  os << indent << "Name: " << (this->Name ? this->Name : "none") << endl;
  os << indent << "LongHelp: " << (this->LongHelp ? this->LongHelp : "none") 
     << endl;
  os << indent << "ShortHelp: " << (this->ShortHelp ? this->ShortHelp : "none") 
     << endl;
  os << indent << "Description: " 
     << (this->Label ? this->Label : "none") << endl;
  os << indent << "ModuleName: " << (this->ModuleName?this->ModuleName:"none")
     << endl;
  os << indent << "MenuName: " << (this->MenuName?this->MenuName:"none")
     << endl;
  os << indent << "Notebook: " << this->GetNotebook() << endl;
  os << indent << "NumberOfPVInputs: " << this->GetNumberOfPVInputs() << endl;
  os << indent << "Notebook: " << this->GetNotebook() << endl;
  os << indent << "ReplaceInput: " << this->GetReplaceInput() << endl;
  os << indent << "View: " << this->GetView() << endl;
  os << indent << "VisitedFlag: " << this->GetVisitedFlag() << endl;
  os << indent << "Widgets: " << this->GetWidgets() << endl;
  os << indent << "IsPermanent: " << this->IsPermanent << endl;
  os << indent << "SourceClassName: " 
     << (this->SourceClassName?this->SourceClassName:"null") << endl;
  os << indent << "ToolbarModule: " << this->ToolbarModule << endl;

  os << indent << "NumberOfPVConsumers: " << this->GetNumberOfPVConsumers() << endl;
  os << indent << "NumberOfParts: " << this->GetNumberOfParts() << endl;
  if (this->PVColorMap)
    {
    os << indent << "PVColorMap: " << this->PVColorMap->GetScalarBarTitle() << endl;
    }
  else
    {
    os << indent << "PVColorMap: NULL\n";
    }

  os << indent << "VTKMultipleInputsFlag: " << this->VTKMultipleInputsFlag << endl;
  os << indent << "VTKMultipleProcessFlag: " << this->VTKMultipleProcessFlag
     << endl;
  os << indent << "InputProperties: \n";
  vtkIndent i2 = indent.GetNextIndent();
  int num, idx;
  num = this->GetNumberOfInputProperties();
  for (idx = 0; idx < num; ++idx)
    {
    this->GetInputProperty(idx)->PrintSelf(os, i2);
    }
  os << indent << "NumberOfOutputsInformation: "
     << this->NumberOfOutputsInformation << endl;
  os << indent << "SourceGrabbed: " << (this->SourceGrabbed?"on":"off") << endl;
  os << indent << "Proxy: ";
  if (this->Proxy)
    {
    os << endl;
    this->Proxy->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "ColorMap: " << this->PVColorMap << endl;    
#if defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
  if (this->DisplayProxy)
    {
    this->DisplayProxy->PrintSelf(os << endl, indent.GetNextIndent());
    }
  else
    {
    os << indent << "DisplayProxy: (none)" << endl;
    }
#else
  os << indent << "PartDisplay: ";
  if( this->PartDisplay )
    {
    this->PartDisplay->PrintSelf(os << endl, indent.GetNextIndent() );
    }
  else
    {
    os << "(none)" << endl;
    }
#endif
  os << indent << "CubeAxesVisibility: " << this->CubeAxesVisibility << endl;
  os << indent << "PointLabelVisibility: " << this->PointLabelVisibility << endl;
  os << indent << "OverideAutoAccept: " << (this->OverideAutoAccept?"yes":"no") << endl;
}

//----------------------------------------------------------------------------
void vtkPVSource::SetAcceptButtonColorToModified()
{
  this->Notebook->SetAcceptButtonColorToModified();
}

