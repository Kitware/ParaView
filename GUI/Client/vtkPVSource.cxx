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
#include "vtkPVSource.h"

#include "vtkPVColorMap.h"
#include "vtkArrayMap.txx"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkDataSet.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWLabeledLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMessageDialog.h"
#include "vtkPVSourceNotebook.h"
#include "vtkKWPushButton.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkSMPart.h"
#include "vtkSMPartDisplay.h"
#include "vtkPVInputProperty.h"
#include "vtkPVNumberOfOutputsInformation.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderModule.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVWidgetCollection.h"
#include "vtkPVWindow.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPart.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSource.h"
#include "vtkString.h"
#include "vtkRenderer.h"
#include "vtkArrayMap.txx"
#include "vtkStringList.h"
#include "vtkPVAnimationInterface.h"
#include "vtkSMCubeAxesDisplay.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSet.h"
#include <vtkstd/vector>


vtkStandardNewMacro(vtkPVSource);
vtkCxxRevisionMacro(vtkPVSource, "1.396");
vtkCxxSetObjectMacro(vtkPVSource,Notebook,vtkPVSourceNotebook);
vtkCxxSetObjectMacro(vtkPVSource,PartDisplay,vtkSMPartDisplay);

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

  // Initialize the data only after  Accept is invoked for the first time.
  // This variable is used to determine that.
  this->Initialized = 0;

  // The notebook which holds Parameters, Display and Information pages.
  this->Notebook = 0;  
  this->PVInputs = NULL;
  this->NumberOfPVInputs = 0;
  
  this->NumberOfPVConsumers = 0;
  this->PVConsumers = 0;
  
  this->PartDisplay = 0;

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
  this->CubeAxesDisplay = vtkSMCubeAxesDisplay::New();
  this->CubeAxesVisibility = 0;
  
  this->PVColorMap = 0;  
}

//----------------------------------------------------------------------------
vtkPVSource::~vtkPVSource()
{
  this->SetPartDisplay(0);
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
    }
  this->SetProxy(0);

  // Do not use SetName() or SetLabel() here. These make
  // the navigation window update when it should not.
  delete[] this->Name;
  delete[] this->Label;

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
  this->CubeAxesDisplay->Delete();
  this->CubeAxesDisplay = 0;
  
  this->SetPVColorMap(0);
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

  if (pvApp == NULL)
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
  if (this->DataInformationValid == 0)
    {
    this->GatherDataInformation();
    
    // Where else should I put this?
    //law int fixme; // Although this should probably go into vtkPVSource::Update,
    // I am going to get rid of this refernece anyway. (Or should I?)
    // Window will know to update the InformationGUI when the current PVSource
    // is changed, but how will it detect that a source has changed?
    // Used to only update the information.  We could make it specific to info again ...
    this->Notebook->Update();
    }
  return this->Proxy->GetDataInformation();
}

//----------------------------------------------------------------------------
void vtkPVSource::InvalidateDataInformation()
{
  this->DataInformationValid = 0;
}

//----------------------------------------------------------------------------
void vtkPVSource::GatherDataInformation()
{
  vtkSMPart *part;
  int i, num;

  //law int fixme;  // Try just calling gather on the partdisplay.

  num = this->GetNumberOfParts();
  for (i = 0; i < num; ++i)
    {
    part = this->GetPart(i);
    part->GatherDataInformation();
    }
  this->DataInformationValid = 1;

  // Used to only update the display gui ...
  if (this->Notebook)
    {
    this->Notebook->Update();
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::Update()
{
  this->Proxy->UpdatePipeline();
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Notebook);
  this->Notebook->UpdateEnableStateWithSource(this);
  this->PropagateEnableState(this->PVColorMap);

  if ( this->Widgets )
    {
    vtkPVWidget *pvWidget;
    vtkCollectionIterator *it = this->Widgets->NewIterator();
    it->InitTraversal();

    int i;
    for (i = 0; i < this->Widgets->GetNumberOfItems(); i++)
      {
      pvWidget = static_cast<vtkPVWidget*>(it->GetObject());
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
    pvWidget = static_cast<vtkPVWidget*>(it->GetObject());
    pvWidget->SetParent(this->ParameterFrame->GetFrame());
    pvWidget->Create(this->GetApplication());
    this->Script("pack %s -side top -fill x -expand t", 
                 pvWidget->GetWidgetName());
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
  this->UpdateParameterWidgets();
  
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
    pvWidget = static_cast<vtkPVWidget*>(it->GetObject());
    pvWidget->Select();
    it->GoToNextItem();
    }
  it->Delete();
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

    pvWidget = static_cast<vtkPVWidget*>(it->GetObject());
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
  this->GetPVApplication()->AddTraceEntry("$kw(%s) LabelEntryCallback",
                                          this->GetTclName());
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
  if (this->GetVisibility() == v || this->PartDisplay == 0)
    {
    return;
    }

  int cubeAxesVisibility = this->GetCubeAxesVisibility();
  
  this->PartDisplay->SetVisibility(v);
  this->CubeAxesDisplay->SetVisibility(v && cubeAxesVisibility);

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
  this->CubeAxesDisplay->SetVisibility(this->GetVisibility() && val);
  
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
vtkPVDisplayGUI* vtkPVSource::GetPVOutput()
{
  return this->Notebook->GetDisplayGUI();
}

//----------------------------------------------------------------------------
int vtkPVSource::GetVisibility()
{
  if ( this->GetPartDisplay() && this->GetPartDisplay()->GetVisibility() )
    {
    return 1;
    }
  return 0;
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
}

//----------------------------------------------------------------------------
void vtkPVSource::AcceptCallback()
{
  // This method is purposely not virtual.  The AcceptCallbackFlag
  // must be 1 for the duration of the accept callback no matter what
  // subclasses might do.  All of the real AcceptCallback funcionality
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

  // vtkPVSource is taking over some of the update descisions because
  // client does not have a real pipeline.
  if ( ! this->Notebook->GetAcceptButtonRed())
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
  if ( ! this->Initialized)
    { // This is the first time, initialize data. 
    // I used to see if the display gui was properly set, but that was a legacy
    // check.  I removed the check.

    // Create the display and add it to the render module.
    vtkPVRenderModule* rm = 
        this->GetPVApplication()->GetProcessModule()->GetRenderModule();
    vtkSMPartDisplay *pDisp = rm->CreatePartDisplay();
    // Parts need to be created before we set source as inpout to part display.
    // This creates the proxy parts.
    this->InitializeData();
    // Display needs an input.
    pDisp->SetInput(this->GetProxy());
    // Render module keeps a list of all the displays.
    rm->AddDisplay(pDisp);
    
    // Hookup cube axes display.
    this->CubeAxesDisplay->SetInput(this->Proxy);
    this->CubeAxesDisplay->SetVisibility(0);
    rm->AddDisplay(this->CubeAxesDisplay);   
    
    // Display GUI is shared. PartDisplay and source of DisplayGUI is set
    // when the source selected as current.
    this->SetPartDisplay(pDisp);
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

    // Set the current data of the window.
    if ( ! hideFlag)
      {
      window->SetCurrentPVSource(this);
      }
    else
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
    this->Initialized = 1;
    }
  else
    {
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
    float rgb[3];
    input->GetPartDisplay()->GetColor(rgb);
    this->GetPartDisplay()->SetColor(rgb[0], rgb[1], rgb[2]);
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
      colorMap = this->GetPVWindow()->GetPVColorMap(arrayInfo->GetName(), 
                                        arrayInfo->GetNumberOfComponents());
      this->SetPVColorMap(colorMap);                                   
      this->PartDisplay->ColorByArray(colorMap->GetRMScalarBarWidget(), 
                                      vtkDataSet::POINT_DATA_FIELD);
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
      colorMap = this->GetPVWindow()->GetPVColorMap(arrayInfo->GetName(), 
                                        arrayInfo->GetNumberOfComponents());
      this->SetPVColorMap(colorMap);                                   
      this->PartDisplay->ColorByArray(colorMap->GetRMScalarBarWidget(), 
                                      vtkDataSet::CELL_DATA_FIELD);
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
      colorField = input->GetPartDisplay()->GetColorField();
      // Find the array in our info.
      switch (colorField)
        {
        case vtkDataSet::POINT_DATA_FIELD:
          attrInfo = dataInfo->GetPointDataInformation();
          arrayInfo = attrInfo->GetArrayInformation(colorMap->GetArrayName());
          if (arrayInfo && colorMap->MatchArrayName(arrayInfo->GetName(),
                                       arrayInfo->GetNumberOfComponents()))
            {  
            this->SetPVColorMap(colorMap);                                   
            this->PartDisplay->ColorByArray(colorMap->GetRMScalarBarWidget(), 
                                            vtkDataSet::POINT_DATA_FIELD);
            return;
            }
          break;
        case vtkDataSet::CELL_DATA_FIELD:
          attrInfo = dataInfo->GetCellDataInformation();
          arrayInfo = attrInfo->GetArrayInformation(colorMap->GetArrayName());
          if (arrayInfo && colorMap->MatchArrayName(arrayInfo->GetName(),
                                       arrayInfo->GetNumberOfComponents()))
            {  
            this->SetPVColorMap(colorMap);                                   
            this->PartDisplay->ColorByArray(colorMap->GetRMScalarBarWidget(), 
                                            vtkDataSet::CELL_DATA_FIELD);
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
  this->SetPVColorMap(0);
  this->GetPartDisplay()->SetScalarVisibility(0);
}

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
  this->UpdateParameterWidgets();
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
      prev = static_cast<vtkPVSource*>( it->GetObject() );
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
    vtkSMPartDisplay* pDisp = this->GetPartDisplay();
    if (pDisp)
      {
      this->GetPVApplication()->GetProcessModule()->GetRenderModule()->RemoveDisplay(this->CubeAxesDisplay);
      this->GetPVApplication()->GetProcessModule()->GetRenderModule()->RemoveDisplay(pDisp);
      }
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
void vtkPVSource::UpdateParameterWidgets()
{
  vtkPVWidget *pvw;
  vtkCollectionIterator *it = this->Widgets->NewIterator();
  it->InitTraversal();
  while( !it->IsDoneWithTraversal() )
    {
    pvw = static_cast<vtkPVWidget*>(it->GetObject());
    // Do not try to reset the widget if it is not initialized
    if (pvw->GetApplication())
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
    vtkPVWidget* pvw = static_cast<vtkPVWidget*>(it->GetObject());
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
    pvw = static_cast<vtkPVWidget*>(it->GetObject());
    if (pvw && pvw->GetModifiedFlag())
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
    pvw = static_cast<vtkPVWidget*>(it->GetObject());
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
  this->Notebook->Update();
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
      vtkClientServerStream& stream = pm->GetStream();
      for (idx = 0; idx < numSources; ++idx)
        {
        vtkClientServerID sourceID = this->GetVTKSourceID(idx);
        stream << vtkClientServerStream::Invoke 
               << sourceID << "SetInputConnection" << 0 << 0
               << vtkClientServerStream::End;
        }
      pm->SendStream(vtkProcessModule::DATA_SERVER);

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
    vtkSMPartDisplay *partD = this->GetPartDisplay();
    if (partD)
      {
      partD->SaveInBatchScript(file, this->GetProxy());
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
    pvw = static_cast<vtkPVWidget*>(it->GetObject());
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
  // Save the options from the display GUI.
  char dispTclName[512];
  sprintf(dispTclName, "$pvDisp(%s)", this->GetTclName());
  *file << "set " << dispTclName+1 
        << " [$kw(" << this->GetTclName() << ") GetPartDisplay]" << endl;
  vtkIndent indent;
  this->GetPartDisplay()->SavePVState(file, dispTclName, indent);

  *file << "$kw(" << this->GetTclName()
        << ") SetCubeAxesVisibility " << this->GetCubeAxesVisibility() << endl; 

  // Make sure the GUI is upto date.  
  *file << "[$kw(" << this->GetTclName()
        << ") GetPVOutput] Update\n"; 
  
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
    pvw = static_cast<vtkPVWidget*>(it->GetObject());
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
  pvw->Select();
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
void vtkPVSource::SetAcceptButtonColorToModified()
{
  this->Notebook->SetAcceptButtonColorToModified();
}

//----------------------------------------------------------------------------
int vtkPVSource::CloneAndInitialize(int makeCurrent, vtkPVSource*& clone)
{
  int retVal = this->ClonePrototypeInternal(clone);
  if (retVal != VTK_OK)
    {
    return retVal;
    }

  vtkPVSource *current = this->GetPVWindow()->GetCurrentPVSource();
  retVal = clone->InitializeClone(current, makeCurrent);


  if (retVal != VTK_OK)
    {
    clone->Delete();
    clone = 0;
    return retVal;
    }

  clone->SetAcceptButtonColorToModified();

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

  pvs->SetShortHelp(this->GetShortHelp());
  pvs->SetLongHelp(this->GetLongHelp());
  pvs->SetVTKMultipleInputsFlag(this->GetVTKMultipleInputsFlag());

  pvs->SetSourceClassName(this->SourceClassName);
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

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(pvs->GetApplication());
  if (!pvApp)
    {
    vtkErrorMacro("vtkPVApplication is not set properly. Aborting clone.");
    pvs->Delete();
    return VTK_ERROR;
    }

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
  pvs->Proxy->CreateVTKObjects(numSources);

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

    pm->GetStream() << vtkClientServerStream::Invoke << sourceId 
                    << "AddObserver"
                    << "StartEvent"
                    << start
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke << sourceId 
                    << "AddObserver"
                    << "EndEvent"
                    << end
                    << vtkClientServerStream::End;
    
    }
  pm->SendStream(vtkProcessModule::DATA_SERVER);
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
    pvWidget = static_cast<vtkPVWidget*>(it->GetObject());
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
int vtkPVSource::InitializeClone(vtkPVSource* input,
                                 int makeCurrent)

{
  // Set the input if necessary.
  if (this->GetNumberOfInputProperties() > 0)
    {
    // Set the VTK reference to the new input.
    vtkPVInputProperty* ip = this->GetInputProperty(0);
    if (ip)
      {
      this->SetPVInput(ip->GetName(), 0, input);
      }
    else
      {
      this->SetPVInput("Input",  0, input);
      }
    }

  // Create the properties frame etc.
  this->CreateProperties();
  
  // Display page must be created before source is selected.
  if (makeCurrent)
    {
    this->GetPVWindow()->SetCurrentPVSourceCallback(this);
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
  os << indent << "PartDisplay: ";
  if( this->PartDisplay )
    {
    this->PartDisplay->PrintSelf(os << endl, indent.GetNextIndent() );
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "CubeAxesVisibility: " << this->CubeAxesVisibility << endl;
}
