/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSphereWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSphereWidget.h"

#include "vtkArrayMap.txx"
#include "vtkCamera.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVInputMenu.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderer.h"
#include "vtkPVProcessModule.h"
#include "vtkPVWindow.h"
#include "vtkPVTraceHelper.h"

#include "vtkKWEvent.h"
#include "vtkSMSphereWidgetProxy.h"

#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkCommand.h"

vtkStandardNewMacro(vtkPVSphereWidget);
vtkCxxRevisionMacro(vtkPVSphereWidget, "1.59");

vtkCxxSetObjectMacro(vtkPVSphereWidget, InputMenu, vtkPVInputMenu);

int vtkPVSphereWidgetCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);


//*****************************************************************************
//----------------------------------------------------------------------------
vtkPVSphereWidget::vtkPVSphereWidget()
{
  int cc;

  this->InputMenu = 0;

  this->Labels[0] = vtkKWLabel::New();
  this->Labels[1] = vtkKWLabel::New();  
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->CenterEntry[cc] = vtkKWEntry::New();
    this->CoordinateLabel[cc] = vtkKWLabel::New();
   }
  this->RadiusEntry = vtkKWEntry::New();
  this->CenterResetButton = vtkKWPushButton::New();
  
  this->ImplicitFunctionProxy = 0;
  this->SetWidgetProxyXMLName("SphereWidgetProxy");
}

//----------------------------------------------------------------------------
vtkPVSphereWidget::~vtkPVSphereWidget()
{
  int i;
  this->UnsetPropertyObservers();
  this->SetInputMenu(NULL);
  this->Labels[0]->Delete();
  this->Labels[1]->Delete();
  for (i=0; i<3; i++)
    {
    this->CenterEntry[i]->Delete();
    this->CoordinateLabel[i]->Delete();
    }
  this->RadiusEntry->Delete();
  this->CenterResetButton->Delete();
  
  if (this->ImplicitFunctionProxy)
    {
    vtkSMProxyManager* proxyM = vtkSMObject::GetProxyManager();
    const char* proxyName = proxyM->GetProxyName(
      "implicit_functions", this->ImplicitFunctionProxy);
    if (proxyName)
      {
      proxyM->UnRegisterProxy("implicit_functions", proxyName);
      }
    this->UnregisterAnimateableProxies();
    this->ImplicitFunctionProxy->Delete();
    this->ImplicitFunctionProxy = 0;
    }
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkPVSphereWidget::GetProxyByName(const char*)
{
  return this->ImplicitFunctionProxy;
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::CenterResetCallback()
{
  vtkPVSource *input;
  double bds[6];

  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource has not been set.");
    return;
    }

  input = this->PVSource->GetPVInput(0);
  if (input == NULL)
    {
    return;
    }
  input->GetDataInformation()->GetBounds(bds);
  this->SetCenter(0.5*(bds[0]+bds[1]),
                  0.5*(bds[2]+bds[3]),
                  0.5*(bds[4]+bds[5]));

}


//----------------------------------------------------------------------------
void vtkPVSphereWidget::ActualPlaceWidget()
{
  this->Superclass::ActualPlaceWidget();
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::Initialize()
{
  this->PlaceWidget();

  this->Accept();
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::ResetInternal()
{
  if ( ! this->ModifiedFlag )
    {
    return;
    }

  vtkSMDoubleVectorProperty* sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ImplicitFunctionProxy->GetProperty("Center"));
  if(sdvp)
    {
    double center[3];
    center[0] = sdvp->GetElement(0);
    center[1] = sdvp->GetElement(1);
    center[2] = sdvp->GetElement(2);
    this->SetCenterInternal(center[0],center[1],center[2]);
    }
  else
    {
    vtkErrorMacro("Could not find property Center for widget: "<< 
      this->ImplicitFunctionProxy->GetVTKClassName());
    }
  sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ImplicitFunctionProxy->GetProperty("Radius"));
  if(sdvp)
    {
    double radius = sdvp->GetElement(0);
    this->SetRadiusInternal(radius);
    }
  else
    {
    vtkErrorMacro("Could not find property Radius for widget: "<< 
      this->ImplicitFunctionProxy->GetVTKClassName());
    }
  this->Superclass::ResetInternal();
}

//---------------------------------------------------------------------------
void vtkPVSphereWidget::Accept()
{
  int modFlag = this->GetModifiedFlag();
  double center[3];
  double radius;
  
  this->WidgetProxy->UpdateInformation();
  this->GetCenterInternal(center);
  radius = this->GetRadiusInternal();

  vtkSMDoubleVectorProperty* sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ImplicitFunctionProxy->GetProperty("Center"));
  if (sdvp)
    {
    sdvp->SetElements3(center[0],center[1],center[2]);
    }
  else
    {
    vtkErrorMacro("Could not find property Center for widget: "
      << this->ImplicitFunctionProxy->GetVTKClassName());
    }
  
  sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ImplicitFunctionProxy->GetProperty("Radius"));
  if (sdvp)
    {
    sdvp->SetElements1(radius);
    }
  else
    {
    vtkErrorMacro("Could not find property Radius for widget: "
      << this->ImplicitFunctionProxy->GetVTKClassName());
    }
  this->ImplicitFunctionProxy->UpdateVTKObjects();
  // 3DWidgets need to explictly call UpdateAnimationInterface on accept
  // since the animatable proxies might have been registered/unregistered
  // which needs to be updated in the Animation interface.
  this->GetPVApplication()->GetMainWindow()->UpdateAnimationInterface();
  this->ModifiedFlag = 0;
  
  // I put this after the accept internal, because
  // vtkPVGroupWidget inactivates and builds an input list ...
  // Putting this here simplifies subclasses AcceptInternal methods.
  if (modFlag)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    ofstream* file = pvApp->GetTraceFile();
    if (file)
      {
      this->Trace(file);
      }
    }

  this->ValueChanged = 0;
}
//---------------------------------------------------------------------------
void vtkPVSphereWidget::Trace(ofstream *file)
{
  double rad;
  double val[3];
  int cc;
  
  if ( ! this->GetTraceHelper()->Initialize(file))
    {
    return;
    }

  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof( this->CenterEntry[cc]->GetValue() );
    }
  *file << "$kw(" << this->GetTclName() << ") SetCenter "
        << val[0] << " " << val[1] << " " << val[2] << endl;

  rad = atof(this->RadiusEntry->GetValue());
  *file << "$kw(" << this->GetTclName() << ") SetRadius "
        << rad << endl;
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SaveInBatchScript(ofstream *file)
{
  if (!this->ImplicitFunctionProxy)
    {
    vtkErrorMacro("ImplicitFunction Proxy must be set to save to a batch script");
    return;
    }
  
  this->WidgetProxy->SaveInBatchScript(file);

  vtkClientServerID sphereID = this->ImplicitFunctionProxy->GetID(0);

  *file << endl;
  *file << "set pvTemp" << sphereID.ID
    << " [$proxyManager NewProxy implicit_functions Sphere]"
    << endl;
  *file << "  $proxyManager RegisterProxy implicit_functions pvTemp"
    << sphereID.ID << " $pvTemp" << sphereID.ID
    << endl;
  *file << "  $pvTemp" << sphereID.ID << " UnRegister {}" << endl;
  
  vtkSMDoubleVectorProperty* sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ImplicitFunctionProxy->GetProperty("Center"));  
  if(sdvp)
    {
    *file << "  [$pvTemp" << sphereID.ID << " GetProperty Center] "
      << "SetElements3 " 
      << sdvp->GetElement(0) << " "
      << sdvp->GetElement(1) << " "
      << sdvp->GetElement(2)
      << endl;
    *file << "  [$pvTemp" << sphereID.ID << " GetProperty Center]"
      << " SetControllerProxy $pvTemp" << this->WidgetProxy->GetID(0) 
      << endl;
    *file << "  [$pvTemp" << sphereID.ID << " GetProperty Center]"
      << " SetControllerProperty [$pvTemp" << this->WidgetProxy->GetID(0)
      << " GetProperty Center]" << endl;
    }

  sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ImplicitFunctionProxy->GetProperty("Radius"));
  if (sdvp)
    {
    *file << "  [$pvTemp" << sphereID.ID << " GetProperty Radius] "
      << "SetElements1 "
      << sdvp->GetElement(0) << endl << endl;
    *file << "  [$pvTemp" << sphereID.ID << " GetProperty Radius]"
      << " SetControllerProxy $pvTemp" << this->WidgetProxy->GetID(0) 
      << endl;
    *file << "  [$pvTemp" << sphereID.ID << " GetProperty Radius]"
      << " SetControllerProperty [$pvTemp" << this->WidgetProxy->GetID(0)
      << " GetProperty Radius]" << endl;
    }
  *file << "  $pvTemp" << sphereID.ID << " UpdateVTKObjects" << endl;
  *file << endl;

}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ImplicitFunctionProxy: " << this->ImplicitFunctionProxy << endl;
  os << indent << "InputMenu: " << this->InputMenu << endl;

}

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVSphereWidget::ClonePrototypeInternal(
  vtkPVSource* pvSource,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
    vtkPVWidget* pvWidget = 0;
  // Check if a clone of this widget has already been created
  if ( map->GetItem(this, pvWidget) != VTK_OK )
    {
    // If not, create one and add it to the map
    pvWidget = this->NewInstance();
    map->SetItem(this, pvWidget);
    // Now copy all the properties
    this->CopyProperties(pvWidget, pvSource, map);

    vtkPVSphereWidget* sw = vtkPVSphereWidget::SafeDownCast(
      pvWidget);
    if(!sw)
      {
      vtkErrorMacro("Internal error. Could not downcast pointer.");
      pvWidget->Delete();
      return 0;
      }
     if (this->InputMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVInputMenu* im = this->InputMenu->ClonePrototype(pvSource, map);
      sw->SetInputMenu(im);
      im->Delete();
      }
    }
  else
    {
    // Increment the reference count. This is necessary
    // to make the behavior same whether a widget is created
    // or returned from the map. Always call Delete() after
    // cloning.
    pvWidget->Register(this);
    }
  return pvWidget;
}


//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetBalloonHelpString(const char *str)
{
  this->Superclass::SetBalloonHelpString(str);

  if (this->Labels[0])
    {
    this->Labels[0]->SetBalloonHelpString(str);
    }

  if (this->Labels[1])
    {
    this->Labels[1]->SetBalloonHelpString(str);
    }

  if (this->RadiusEntry)
    {
    this->RadiusEntry->SetBalloonHelpString(str);
    }

  if (this->CenterResetButton)
    {
    this->CenterResetButton->SetBalloonHelpString(str);
    }

  for (int i=0; i<3; i++)
    {
    if (this->CoordinateLabel[i])
      {
      this->CoordinateLabel[i]->SetBalloonHelpString(str);
      }
    if (this->CenterEntry[i])
      {
      this->CenterEntry[i]->SetBalloonHelpString(str);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::ChildCreate(vtkPVApplication* pvApp)
{
  if ((this->GetTraceHelper()->GetObjectNameState() == 
       vtkPVTraceHelper::ObjectNameStateUninitialized ||
      this->GetTraceHelper()->GetObjectNameState() == 
       vtkPVTraceHelper::ObjectNameStateDefault) )
    {
    this->GetTraceHelper()->SetObjectName("Sphere");
    this->GetTraceHelper()->SetObjectNameState(
      vtkPVTraceHelper::ObjectNameStateSelfInitialized);
    }

  this->SetFrameLabel("Sphere Widget");
  this->Labels[0]->SetParent(this->Frame->GetFrame());
  this->Labels[0]->Create(pvApp, "");
  this->Labels[0]->SetText("Center");
  this->Labels[1]->SetParent(this->Frame->GetFrame());
  this->Labels[1]->Create(pvApp, "");
  this->Labels[1]->SetText("Radius");

  int i;
  for (i=0; i<3; i++)
    {
    this->CoordinateLabel[i]->SetParent(this->Frame->GetFrame());
    this->CoordinateLabel[i]->Create(pvApp, "");
    char buffer[3];
    sprintf(buffer, "%c", "xyz"[i]);
    this->CoordinateLabel[i]->SetText(buffer);
    }
  for (i=0; i<3; i++)
    {
    this->CenterEntry[i]->SetParent(this->Frame->GetFrame());
    this->CenterEntry[i]->Create(pvApp, "");
    }
  this->RadiusEntry->SetParent(this->Frame->GetFrame());
  this->RadiusEntry->Create(pvApp, "");

  this->Script("grid propagate %s 1",
    this->Frame->GetFrame()->GetWidgetName());

  this->Script("grid x %s %s %s -sticky ew",
    this->CoordinateLabel[0]->GetWidgetName(),
    this->CoordinateLabel[1]->GetWidgetName(),
    this->CoordinateLabel[2]->GetWidgetName());
  this->Script("grid %s %s %s %s -sticky ew",
    this->Labels[0]->GetWidgetName(),
    this->CenterEntry[0]->GetWidgetName(),
    this->CenterEntry[1]->GetWidgetName(),
    this->CenterEntry[2]->GetWidgetName());
  this->Script("grid %s %s - - -sticky ew",
    this->Labels[1]->GetWidgetName(),
    this->RadiusEntry->GetWidgetName());

  this->Script("grid columnconfigure %s 0 -weight 0", 
    this->Frame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 1 -weight 2", 
    this->Frame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 2 -weight 2", 
    this->Frame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 3 -weight 2", 
    this->Frame->GetFrame()->GetWidgetName());

  for (i=0; i<3; i++)
    {
    this->Script("bind %s <Key> {%s SetValueChanged}",
      this->CenterEntry[i]->GetWidgetName(),
      this->GetTclName());
    this->Script("bind %s <FocusOut> {%s SetCenter}",
      this->CenterEntry[i]->GetWidgetName(),
      this->GetTclName());
    this->Script("bind %s <KeyPress-Return> {%s SetCenter}",
      this->CenterEntry[i]->GetWidgetName(),
      this->GetTclName());
    }
  this->Script("bind %s <Key> {%s SetValueChanged}",
    this->RadiusEntry->GetWidgetName(),
    this->GetTclName());
  this->Script("bind %s <FocusOut> {%s SetRadius}",
    this->RadiusEntry->GetWidgetName(),
    this->GetTclName());
  this->Script("bind %s <KeyPress-Return> {%s SetRadius}",
    this->RadiusEntry->GetWidgetName(),
    this->GetTclName());
  this->CenterResetButton->SetParent(this->Frame->GetFrame());
  this->CenterResetButton->Create(pvApp, "");
  this->CenterResetButton->SetText("Set Sphere Center to Center of Bounds");
  this->CenterResetButton->SetCommand(this, "CenterResetCallback"); 
  this->Script("grid %s - - - - -sticky ew", 
    this->CenterResetButton->GetWidgetName());
  // Initialize the center of the sphere based on the input bounds.
  if (this->PVSource)
    {
    vtkPVSource *input = this->PVSource->GetPVInput(0);
    if (input)
      {
      double bds[6];
      input->GetDataInformation()->GetBounds(bds);
      this->SetCenter(0.5*(bds[0]+bds[1]), 0.5*(bds[2]+bds[3]),
        0.5*(bds[4]+bds[5]));
      this->SetRadius(0.5*(bds[1]-bds[0]));
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::Create( vtkKWApplication *app)
{
  this->Superclass::Create(app);


  static int proxyNum = 0;
  vtkSMProxyManager *pm = vtkSMObject::GetProxyManager();
  this->ImplicitFunctionProxy = pm->NewProxy("implicit_functions", "Sphere");
  ostrstream str;
  str << "Sphere" << proxyNum << ends;
  proxyNum++;
  pm->RegisterProxy("implicit_functions",str.str(),this->ImplicitFunctionProxy);
  delete[] str.str();

  this->SetupPropertyObservers();
}
//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetupPropertyObservers()
{
  if (!this->ImplicitFunctionProxy)
    {
    return;
    }
  vtkSMProperty*p = this->ImplicitFunctionProxy->GetProperty("Center");
  if (p)
    {
    this->AddPropertyObservers(p);
    }
  p = this->ImplicitFunctionProxy->GetProperty("Radius");
  if (p)
    {
    this->AddPropertyObservers(p);
    }
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::UnsetPropertyObservers()
{
  if (!this->ImplicitFunctionProxy)
    {
    return;
    }
  vtkSMProperty*p = this->ImplicitFunctionProxy->GetProperty("Center");
  if (p)
    {
    this->RemovePropertyObservers(p);
    }
  p = this->ImplicitFunctionProxy->GetProperty("Radius");
  if (p)
    {
    this->RemovePropertyObservers(p);
    }
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::ExecuteEvent(vtkObject* wdg, unsigned long l, void* p)
{
  if (vtkSM3DWidgetProxy::SafeDownCast(wdg))
    {
    if(l == vtkCommand::WidgetModifiedEvent)
      {
      double center[3];
      double radius;
      this->WidgetProxy->UpdateInformation();
      this->GetCenterInternal(center);
      radius = this->GetRadiusInternal();
      this->CenterEntry[0]->SetValue(center[0]);
      this->CenterEntry[1]->SetValue(center[1]);
      this->CenterEntry[2]->SetValue(center[2]);
      this->RadiusEntry->SetValue(radius);
      this->Render();
      this->ModifiedCallback();
      this->ValueChanged = 0;
      }
    }
  if (vtkSMProperty::SafeDownCast(wdg))
    {
    switch(l)
      {
    case vtkCommand::ModifiedEvent:
      if (!this->ModifiedFlag)
        {
        // This is the reset to make the widget reflect the state of the properties.
        // If the widget has been modified, we don't reset it. This also helps
        // avoid the reset from being called while 'Accept'ing the values.
        this->ResetInternal();
        }
      break;
      }
    }
  this->Superclass::ExecuteEvent(wdg, l, p);
}

//----------------------------------------------------------------------------
int vtkPVSphereWidget::ReadXMLAttributes(vtkPVXMLElement* element,
  vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }  

  // Setup the InputMenu.
  const char* input_menu = element->GetAttribute("input_menu");
  if(!input_menu)
    {
    vtkErrorMacro("No input_menu attribute.");
    return 0;
    }

  vtkPVXMLElement* ame = element->LookupElement(input_menu);
  if (!ame)
    {
    vtkErrorMacro("Couldn't find InputMenu element " << input_menu);
    return 0;
    }
  
  vtkPVWidget* w = this->GetPVWidgetFromParser(ame, parser);
  vtkPVInputMenu* imw = vtkPVInputMenu::SafeDownCast(w);
  if(!imw)
    {
    if(w) { w->Delete(); }
    vtkErrorMacro("Couldn't get InputMenu widget " << input_menu);
    return 0;
    }
  imw->AddDependent(this);
  this->SetInputMenu(imw);
  imw->Delete();  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::GetCenter(double pt[3])
{
  if(!this->IsCreated())
    {
    vtkErrorMacro("Not created yet");
    return;
    }
  this->WidgetProxy->UpdateInformation();
  this->GetCenterInternal(pt);
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::GetCenterInternal(double pt[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("CenterInfo"));
  pt[0] = dvp->GetElement(0);
  pt[1] = dvp->GetElement(1);
  pt[2] = dvp->GetElement(2);
}

//----------------------------------------------------------------------------
double vtkPVSphereWidget::GetRadius()
{
  if(!this->IsCreated())
    {
    vtkErrorMacro("Not created yet");
    return 0.0;
    }
  this->WidgetProxy->UpdateInformation();
  return this->GetRadiusInternal();
}

//----------------------------------------------------------------------------
double vtkPVSphereWidget::GetRadiusInternal()
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("RadiusInfo"));
  return dvp->GetElement(0);
}
//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetCenter(double x, double y, double z)
{
  this->SetCenterInternal(x, y, z);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetCenter %f %f %f",
    this->GetTclName(), x, y, z);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetCenter()
{
  if(!this->ValueChanged)
    {
    return;
    }
  double val[3];
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof(this->CenterEntry[cc]->GetValue());
    } 
  this->SetCenterInternal(val[0],val[1],val[2]);
  this->Render();
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetCenterInternal(double x, double y, double z)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Not created yet");
    return;
    }
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Center"));
  dvp->SetElements3(x,y,z);
  this->WidgetProxy->UpdateVTKObjects();

  this->CenterEntry[0]->SetValue(x);
  this->CenterEntry[1]->SetValue(y);
  this->CenterEntry[2]->SetValue(z);
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetRadiusInternal(double r)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Not created yet");
    return;
    }
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Radius"));
  dvp->SetElements1(r);
  this->WidgetProxy->UpdateVTKObjects(); 

  this->RadiusEntry->SetValue(r);
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetRadius(double r)
{
  this->SetRadiusInternal(r);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetRadius %f ", this->GetTclName(), r);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetRadius()
{
  if(this-ValueChanged == 0)
    {
    return;
    }
  double val;
  val = atof(this->RadiusEntry->GetValue());
  this->SetRadiusInternal(val);
  this->Render();
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->InputMenu);
  this->PropagateEnableState(this->RadiusEntry);
  this->PropagateEnableState(this->CenterResetButton);

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->PropagateEnableState(this->CenterEntry[cc]);
    this->PropagateEnableState(this->CoordinateLabel[cc]);
    }

  this->PropagateEnableState(this->Labels[0]);
  this->PropagateEnableState(this->Labels[1]);
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::RegisterAnimateableProxies()
{
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  if (this->PVSource && this->ImplicitFunctionProxy)
    {
    vtkSMSourceProxy* sproxy = this->PVSource->GetProxy();
    if (sproxy)
      {
      const char* root = pm->GetProxyName("animateable", sproxy);
      if (root)
        {
        ostrstream animName;
        animName << root << ".Sphere" << ends;
        pm->RegisterProxy(
          "animateable", animName.str(), this->ImplicitFunctionProxy);
        delete[] animName.str();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::UnregisterAnimateableProxies()
{
  if (!this->ImplicitFunctionProxy)
    {
    return;
    }
  vtkSMProxyManager* proxyM = vtkSMObject::GetProxyManager();

  const char* proxyName = proxyM->GetProxyName("animateable", this->ImplicitFunctionProxy);
  if (proxyName)
    {
    proxyM->UnRegisterProxy("animateable", proxyName);
    }

}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::Update()
{
  vtkPVSource* input;
  double bds[6];

  this->Superclass::Update();
  //Input bounds may have changed so call place widget
  input = this->InputMenu->GetCurrentValue();
  if (input)
    {
    input->GetDataInformation()->GetBounds(bds);
    this->PlaceWidget(bds);
    this->Render();
    }
}
