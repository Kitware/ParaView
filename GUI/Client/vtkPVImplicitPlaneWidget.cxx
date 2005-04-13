/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImplicitPlaneWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVImplicitPlaneWidget.h"

#include "vtkArrayMap.txx"
#include "vtkCamera.h"
#include "vtkKWEntry.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVInputMenu.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVXMLElement.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSMBoundsDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMImplicitPlaneWidgetProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkCommand.h"
#include "vtkPVWindow.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVImplicitPlaneWidget);
vtkCxxRevisionMacro(vtkPVImplicitPlaneWidget, "1.52.2.2");

vtkCxxSetObjectMacro(vtkPVImplicitPlaneWidget, InputMenu, vtkPVInputMenu);

int vtkPVImplicitPlaneWidgetCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVImplicitPlaneWidget::vtkPVImplicitPlaneWidget()
{
  int cc;

  this->InputMenu = 0;

  this->Labels[0] = vtkKWLabel::New();
  this->Labels[1] = vtkKWLabel::New();  
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->CenterEntry[cc] = vtkKWEntry::New();
    this->NormalEntry[cc] = vtkKWEntry::New();
    this->CoordinateLabel[cc] = vtkKWLabel::New();
   }
  this->OffsetLabel = vtkKWLabel::New();
  this->OffsetEntry = vtkKWEntry::New();
  this->CenterResetButton = vtkKWPushButton::New();
  this->NormalButtonFrame = vtkKWWidget::New();
  this->NormalCameraButton = vtkKWPushButton::New();
  this->NormalXButton = vtkKWPushButton::New();
  this->NormalYButton = vtkKWPushButton::New();
  this->NormalZButton = vtkKWPushButton::New();

  this->ImplicitFunctionProxy = 0;
  this->SetWidgetProxyXMLName("ImplicitPlaneWidgetProxy");
}

//----------------------------------------------------------------------------
vtkPVImplicitPlaneWidget::~vtkPVImplicitPlaneWidget()
{
  // We unset the property observers, since we don't want to catch any events
  // while this thing it getting destroyed, it can crash otherwise!
  this->UnsetPropertyObservers();
  this->SetInputMenu(NULL);
  int i;
  this->Labels[0]->Delete();
  this->Labels[1]->Delete();
  for (i=0; i<3; i++)
    {
    this->CenterEntry[i]->Delete();
    this->NormalEntry[i]->Delete();
    this->CoordinateLabel[i]->Delete();
    }
  this->OffsetLabel->Delete();
  this->OffsetEntry->Delete();
  this->CenterResetButton->Delete();
  this->NormalButtonFrame->Delete();
  this->NormalCameraButton->Delete();
  this->NormalXButton->Delete();
  this->NormalYButton->Delete();
  this->NormalZButton->Delete();
  
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
void vtkPVImplicitPlaneWidget::CenterResetCallback()
{
  vtkPVSource *input;
  double bds[6];

  //Input bounds may have changed so call place widget
  input = this->InputMenu->GetCurrentValue();
  if (input == NULL)
    {
    vtkErrorMacro("No input set to reset the center");
    return;
    }
  input->GetDataInformation()->GetBounds(bds);
  this->SetCenter(0.5*(bds[0]+bds[1]),0.5*(bds[2]+bds[3]),
    0.5*(bds[4]+bds[5]));
}


//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::NormalCameraCallback()
{
  vtkKWView *view;
  vtkRenderer *ren;
  vtkCamera *cam;
  double normal[3];

  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource has not been set.");
    return;
    }

  view = this->PVSource->GetView();
  if (view == NULL)
    {
    vtkErrorMacro("Could not get the view/camera to set the normal.");
    return;
    }
  ren = vtkRenderer::SafeDownCast(view->GetViewport());
  if (ren == NULL)
    {
    vtkErrorMacro("Could not get the renderer/camera to set the normal.");
    return;
    }
  cam = ren->GetActiveCamera();
  if (cam == NULL)
    {
    vtkErrorMacro("Could not get the camera to set the normal.");
    return;
    }
  cam->GetViewPlaneNormal(normal);

  this->SetNormal(-normal[0], -normal[1], -normal[2]);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::NormalXCallback()
{
  this->SetNormal(1,0,0);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::NormalYCallback()
{
  this->SetNormal(0,1,0);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::NormalZCallback()
{
  this->SetNormal(0,0,1);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::CommonReset()
{
  vtkSMDoubleVectorProperty* sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ImplicitFunctionProxy->GetProperty("Origin"));
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
    vtkErrorMacro("Could not find property Origin for widget: "<< 
                  this->ImplicitFunctionProxy->GetVTKClassName());
    }
  
  sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ImplicitFunctionProxy->GetProperty("Normal"));
  if(sdvp)
    {
    double normal[3];
    normal[0] = sdvp->GetElement(0);
    normal[1] = sdvp->GetElement(1);
    normal[2] = sdvp->GetElement(2);
    this->SetNormalInternal(normal[0],normal[1],normal[2]);
    }
  else
    {
    vtkErrorMacro("Could not find property Normal for widget: "<< 
                  this->ImplicitFunctionProxy->GetVTKClassName());
    }
  
  sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ImplicitFunctionProxy->GetProperty("Offset"));
  if(sdvp)
    {
    double offset = sdvp->GetElement(0);
    this->OffsetEntry->SetValue(offset);
    }
  else
    {
    vtkErrorMacro("Could not find property Offset for widget: "<< 
                  this->ImplicitFunctionProxy->GetVTKClassName());
    }
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::Initialize()
{
  this->PlaceWidget();
  this->SetNormalInternal(0,0,1);
  this->UpdateOffsetRange();

  this->Accept();

  this->CommonReset();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::ResetInternal()
{
  this->CommonReset();
  
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("DrawPlane"));
  if (ivp)
    {
    ivp->SetElements1(0);
    }
  this->WidgetProxy->UpdateVTKObjects();

  this->UpdateOffsetRange();

  this->Superclass::ResetInternal();
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkPVImplicitPlaneWidget::GetProxyByName(const char*)
{
  return this->ImplicitFunctionProxy;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::Accept()
{
  int modFlag = this->GetModifiedFlag();
  double center[3];
  double normal[3];

  this->WidgetProxy->UpdateInformation();
  this->GetCenterInternal(center);
  this->GetNormalInternal(normal);

  vtkSMDoubleVectorProperty* sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ImplicitFunctionProxy->GetProperty("Origin"));
  if (sdvp)
    {
    sdvp->SetElements(center);
    }
  else
    {
    vtkErrorMacro("Could not find property Origin for widget: "
      << this->ImplicitFunctionProxy->GetVTKClassName());
    }
  sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ImplicitFunctionProxy->GetProperty("Normal"));
  if (sdvp)
    {
    sdvp->SetElements(normal);
    }
  else
    {
    vtkErrorMacro("Could not find property Normal for widget: "
      << this->ImplicitFunctionProxy->GetVTKClassName());
    }
  sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ImplicitFunctionProxy->GetProperty("Offset"));
  if (sdvp)
    {
    sdvp->SetElement(0, this->OffsetEntry->GetValueAsFloat());
    }
  else
    {
    vtkErrorMacro("Could not find property Normal for widget: "
      << this->ImplicitFunctionProxy->GetVTKClassName());
    }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("DrawPlane"));
  if (ivp)
    {
    ivp->SetElements1(0);
    }
  this->WidgetProxy->UpdateVTKObjects();
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
void vtkPVImplicitPlaneWidget::Trace(ofstream *file)
{
  double val[3];
  int cc;

  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof( this->CenterEntry[cc]->GetValue() );
    }
  *file << "$kw(" << this->GetTclName() << ") SetCenter "
    << val[0] << " " << val[1] << " " << val[2] << endl;
  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof( this->NormalEntry[cc]->GetValue() );
    }
  *file << "$kw(" << this->GetTclName() << ") SetNormal "
    << val[0] << " " << val[1] << " " << val[2] << endl;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SaveInBatchScript(ofstream* file)
{
  this->WidgetProxy->SaveInBatchScript(file);

  vtkClientServerID planeID = this->ImplicitFunctionProxy->GetID(0);
  *file << endl;
  *file << "set pvTemp" <<  planeID.ID
    << " [$proxyManager NewProxy implicit_functions Plane]"
    << endl;
  *file << "  $proxyManager RegisterProxy implicit_functions pvTemp"
    << planeID.ID << " $pvTemp" << planeID.ID
    << endl;
  *file << "  $pvTemp" << planeID.ID << " UnRegister {}" << endl;

  vtkSMDoubleVectorProperty* sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ImplicitFunctionProxy->GetProperty("Origin"));
  if (sdvp)
    {
    *file << "  [$pvTemp" << planeID.ID << " GetProperty Origin] "
      << "SetElements3 " 
      << sdvp->GetElement(0) << " " 
      << sdvp->GetElement(1) << " " 
      << sdvp->GetElement(2) << endl;
    *file << "  [$pvTemp" << planeID.ID << " GetProperty Origin]"
      << " SetControllerProxy $pvTemp" << this->WidgetProxy->GetID(0) << endl;
    *file << "  [$pvTemp" << planeID.ID << " GetProperty Origin]"
      << " SetControllerProperty [$pvTemp" << this->WidgetProxy->GetID(0)
      << " GetProperty Center]" << endl;
    }

  sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ImplicitFunctionProxy->GetProperty("Normal"));
  if(sdvp)
    {
    *file << "  [$pvTemp" << planeID.ID << " GetProperty Normal] "
      << "SetElements3 " 
      << sdvp->GetElement(0) << " " 
      << sdvp->GetElement(1) << " " 
      << sdvp->GetElement(2) << endl;
    *file << "  [$pvTemp" << planeID.ID << " GetProperty Normal]"
      << " SetControllerProxy $pvTemp" << this->WidgetProxy->GetID(0) << endl;
    *file << "  [$pvTemp" << planeID.ID << " GetProperty Normal]"
      << " SetControllerProperty [$pvTemp" << this->WidgetProxy->GetID(0)
      << " GetProperty Normal]" << endl;
    }
  *file << "  $pvTemp" << planeID.ID << " UpdateVTKObjects" << endl;
  *file << endl;

}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent 
     << "ImplicitFunctionProxy: " << this->ImplicitFunctionProxy 
     << endl;
  os << indent << "InputMenu: " << this->InputMenu << endl;
}

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVImplicitPlaneWidget::ClonePrototypeInternal(
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

    vtkPVImplicitPlaneWidget* ipw = vtkPVImplicitPlaneWidget::SafeDownCast(pvWidget);
    if (!ipw)
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
      ipw->SetInputMenu(im);
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
void vtkPVImplicitPlaneWidget::SetBalloonHelpString(const char *str)
{

  // A little overkill.
  if (this->BalloonHelpString == NULL && str == NULL)
    {
    return;
    }

  // This check is needed to prevent errors when using
  // this->SetBalloonHelpString(this->BalloonHelpString)
  if (str != this->BalloonHelpString)
    {
    // Normal string stuff.
    if (this->BalloonHelpString)
      {
      delete [] this->BalloonHelpString;
      this->BalloonHelpString = NULL;
      }
    if (str != NULL)
      {
      this->BalloonHelpString = new char[strlen(str)+1];
      strcpy(this->BalloonHelpString, str);
      }
    }

  if ( this->GetApplication() && !this->BalloonHelpInitialized )
    {
    this->Labels[0]->SetBalloonHelpString(this->BalloonHelpString);
    this->Labels[1]->SetBalloonHelpString(this->BalloonHelpString);

    this->CenterResetButton->SetBalloonHelpString(this->BalloonHelpString);
    this->NormalCameraButton->SetBalloonHelpString(this->BalloonHelpString);
    this->NormalXButton->SetBalloonHelpString(this->BalloonHelpString);
    this->NormalYButton->SetBalloonHelpString(this->BalloonHelpString);
    this->NormalZButton->SetBalloonHelpString(this->BalloonHelpString);

    for (int i=0; i<3; i++)
      {
      this->CoordinateLabel[i]->SetBalloonHelpString(this->BalloonHelpString);
      this->CenterEntry[i]->SetBalloonHelpString(this->BalloonHelpString);
      this->NormalEntry[i]->SetBalloonHelpString(this->BalloonHelpString);
      }

    this->OffsetEntry->SetBalloonHelpString(this->BalloonHelpString);
    this->OffsetLabel->SetBalloonHelpString(this->BalloonHelpString);

    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::ChildCreate(vtkPVApplication* pvApp)
{
  // Now that the 3D widget is on each process,
  // we do not need to create our own plane (but it does not hurt).
  if ((this->TraceNameState == vtkPVWidget::Uninitialized ||
      this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName("Plane");
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }

  this->SetFrameLabel("Plane Widget");
  this->Labels[0]->SetParent(this->Frame->GetFrame());
  this->Labels[0]->Create(pvApp, "");
  this->Labels[0]->SetText("Center");
  this->Labels[1]->SetParent(this->Frame->GetFrame());
  this->Labels[1]->Create(pvApp, "");
  this->Labels[1]->SetText("Normal");

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

  for (i=0; i<3; i++)    
    {
    this->NormalEntry[i]->SetParent(this->Frame->GetFrame());
    this->NormalEntry[i]->Create(pvApp, "");
    }

  this->OffsetLabel->SetParent(this->Frame->GetFrame());
  this->OffsetLabel->SetText("Offset");
  this->OffsetLabel->Create(pvApp, "");

  this->OffsetEntry->SetParent(this->Frame->GetFrame());
  this->OffsetEntry->Create(pvApp, "");

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
  this->Script("grid %s %s %s %s -sticky ew",
    this->Labels[1]->GetWidgetName(),
    this->NormalEntry[0]->GetWidgetName(),
    this->NormalEntry[1]->GetWidgetName(),
    this->NormalEntry[2]->GetWidgetName());
  this->Script("grid %s %s -sticky ew",
    this->OffsetLabel->GetWidgetName(),
    this->OffsetEntry->GetWidgetName());

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
    this->Script("bind %s <Key> {%s SetValueChanged}",
      this->NormalEntry[i]->GetWidgetName(),
      this->GetTclName());
    this->Script("bind %s <FocusOut> {%s SetCenter}",
      this->CenterEntry[i]->GetWidgetName(),
      this->GetTclName());
    this->Script("bind %s <FocusOut> {%s SetNormal}",
      this->NormalEntry[i]->GetWidgetName(),
      this->GetTclName());
    this->Script("bind %s <KeyPress-Return> {%s SetCenter}",
      this->CenterEntry[i]->GetWidgetName(),
      this->GetTclName());
    this->Script("bind %s <KeyPress-Return> {%s SetNormal}",
      this->NormalEntry[i]->GetWidgetName(),
      this->GetTclName());
    }
  this->Script("bind %s <Key> {%s UpdateOffsetRange; %s ModifiedCallback}", 
               this->OffsetEntry->GetWidgetName(), 
               this->GetTclName(),
               this->GetTclName());

  this->CenterResetButton->SetParent(this->Frame->GetFrame());
  this->CenterResetButton->Create(pvApp, "");
  this->CenterResetButton->SetText("Set Plane Center to Center of Bounds");
  this->CenterResetButton->SetCommand(this, "CenterResetCallback"); 
  this->Script("grid %s - - - - -sticky ew", 
    this->CenterResetButton->GetWidgetName());

  this->NormalButtonFrame->SetParent(this->Frame->GetFrame());
  this->NormalButtonFrame->Create(pvApp, "frame", "");
  this->Script("grid %s - - - - -sticky ew", 
    this->NormalButtonFrame->GetWidgetName());

  this->NormalCameraButton->SetParent(this->NormalButtonFrame);
  this->NormalCameraButton->Create(pvApp, "");
  this->NormalCameraButton->SetText("Use Camera Normal");
  this->NormalCameraButton->SetCommand(this, "NormalCameraCallback"); 
  this->Script("pack %s -side left -fill x -expand t",
    this->NormalCameraButton->GetWidgetName());
  this->NormalXButton->SetParent(this->NormalButtonFrame);
  this->NormalXButton->Create(pvApp, "");
  this->NormalXButton->SetText("X Normal");
  this->NormalXButton->SetCommand(this, "NormalXCallback"); 
  this->Script("pack %s -side left -fill x -expand t",
    this->NormalXButton->GetWidgetName());
  this->NormalYButton->SetParent(this->NormalButtonFrame);
  this->NormalYButton->Create(pvApp, "");
  this->NormalYButton->SetText("Y Normal");
  this->NormalYButton->SetCommand(this, "NormalYCallback"); 
  this->Script("pack %s -side left -fill x -expand t",
    this->NormalYButton->GetWidgetName());
  this->NormalZButton->SetParent(this->NormalButtonFrame);
  this->NormalZButton->Create(pvApp, "");
  this->NormalZButton->SetText("Z Normal");
  this->NormalZButton->SetCommand(this, "NormalZCallback"); 
  this->Script("pack %s -side left -fill x -expand t",
    this->NormalZButton->GetWidgetName());

  // Initialize the center of the plane based on the input bounds.
  if (this->PVSource)  
    {
    vtkPVSource *input = this->PVSource->GetPVInput(0);
    if (input)
      {
      double bds[6];
      input->GetDataInformation()->GetBounds(bds);
      this->SetCenter(0.5*(bds[0]+bds[1]), 0.5*(bds[2]+bds[3]),
        0.5*(bds[4]+bds[5]));
      this->SetNormal(0, 0, 1);
      }
    }
  this->SetBalloonHelpString(this->BalloonHelpString);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::Create(vtkKWApplication *app)
{
  this->Superclass::Create(app);

  static int proxyNum = 0;
  vtkSMProxyManager *pm = vtkSMObject::GetProxyManager();
  this->ImplicitFunctionProxy = pm->NewProxy("implicit_functions", "Plane");
  ostrstream str;
  str << "Plane" << proxyNum << ends;
  proxyNum++;
  pm->RegisterProxy("implicit_functions",str.str(),this->ImplicitFunctionProxy);
  delete[] str.str();
  this->SetupPropertyObservers();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::ExecuteEvent(vtkObject* wdg, unsigned long l, void* p)
{

  if (vtkSM3DWidgetProxy::SafeDownCast(wdg))
    {
    double center[3];
    double normal[3];
    this->WidgetProxy->UpdateInformation();
    this->GetCenterInternal(center);
    this->GetNormalInternal(normal);

    if(l == vtkCommand::WidgetModifiedEvent)
      {
      this->CenterEntry[0]->SetValue(center[0]);
      this->CenterEntry[1]->SetValue(center[1]);
      this->CenterEntry[2]->SetValue(center[2]);

      this->NormalEntry[0]->SetValue(normal[0]);
      this->NormalEntry[1]->SetValue(normal[1]);
      this->NormalEntry[2]->SetValue(normal[2]);

      this->ModifiedCallback();
      this->ValueChanged = 0;
      }
    else
      {
      vtkSMDoubleVectorProperty *oProp = vtkSMDoubleVectorProperty::SafeDownCast(
        this->ImplicitFunctionProxy->GetProperty("Origin"));

      vtkSMDoubleVectorProperty *nProp = vtkSMDoubleVectorProperty::SafeDownCast(
        this->ImplicitFunctionProxy->GetProperty("Normal"));

      if (oProp)
        {
        oProp->SetUncheckedElement(0, center[0]);
        oProp->SetUncheckedElement(1, center[1]);
        oProp->SetUncheckedElement(2, center[2]);
        }
      if (nProp)
        {
        nProp->SetUncheckedElement(0, normal[0]);
        nProp->SetUncheckedElement(1, normal[1]);
        nProp->SetUncheckedElement(2, normal[2]);
        }
      oProp->UpdateDependentDomains();
      nProp->UpdateDependentDomains();
      }
    }
  if (vtkSMProperty::SafeDownCast(wdg))
    {
    switch (l)
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
int vtkPVImplicitPlaneWidget::ReadXMLAttributes(vtkPVXMLElement* element,
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
void vtkPVImplicitPlaneWidget::SetCenterInternal(double x, double y, double z)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Center"));
  dvp->SetElements3(x,y,z);
  this->WidgetProxy->UpdateVTKObjects();
  this->Render();

  this->CenterEntry[0]->SetValue(x);
  this->CenterEntry[1]->SetValue(y);
  this->CenterEntry[2]->SetValue(z);

  this->UpdateOffsetRange();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetCenter(double x, double y, double z)
{
  this->SetCenterInternal(x,y,z);
  this->AddTraceEntry("$kw(%s) SetCenter %f %f %f",
    this->GetTclName(), x, y, z);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::GetCenter(double pt[3])
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
void vtkPVImplicitPlaneWidget::GetCenterInternal(double pt[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("CenterInfo"));
  pt[0] = dvp->GetElement(0);
  pt[1] = dvp->GetElement(1);
  pt[2] = dvp->GetElement(2);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetNormalInternal(double x, double y, double z)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Normal"));
  dvp->SetElements3(x,y,z);
  this->WidgetProxy->UpdateVTKObjects();
  this->Render();

  this->NormalEntry[0]->SetValue(x);
  this->NormalEntry[1]->SetValue(y);
  this->NormalEntry[2]->SetValue(z);

  this->UpdateOffsetRange();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetNormal(double x, double y, double z)
{
  this->SetNormalInternal(x, y, z);
  this->AddTraceEntry("$kw(%s) SetNormal %f %f %f",
    this->GetTclName(), x, y, z);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::GetNormal(double pt[3])
{
  if(!this->IsCreated())
    {
    vtkErrorMacro("Not created yet");
    return;
    }
  this->WidgetProxy->UpdateInformation();
  this->GetNormalInternal(pt);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::GetNormalInternal(double pt[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("NormalInfo"));
  pt[0] = dvp->GetElement(0);
  pt[1] = dvp->GetElement(1);
  pt[2] = dvp->GetElement(2);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetCenter()
{
  if (!this->ValueChanged)
    {
    return;
    }
  double val[3];
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof(this->CenterEntry[cc]->GetValue());
    }
  this->SetCenter(val[0], val[1], val[2]);
  this->Render();
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetNormal()
{
  if (!this->ValueChanged)
    {
    return;
    }
  double val[3];
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof(this->NormalEntry[cc]->GetValue());
    }
  this->SetNormal(val[0], val[1], val[2]);
  this->Render();
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::UpdateOffsetRange()
{
  double center[3];
  double normal[3];
  this->WidgetProxy->UpdateInformation();
  this->GetCenterInternal(center);
  this->GetNormalInternal(normal);

  vtkSMDoubleVectorProperty *oProp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ImplicitFunctionProxy->GetProperty("Origin"));
  
  vtkSMDoubleVectorProperty *nProp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ImplicitFunctionProxy->GetProperty("Normal"));
  
  if (oProp)
    {
    oProp->SetUncheckedElement(0, center[0]);
    oProp->SetUncheckedElement(1, center[1]);
    oProp->SetUncheckedElement(2, center[2]);
    }
  if (nProp)
    {
    nProp->SetUncheckedElement(0, normal[0]);
    nProp->SetUncheckedElement(1, normal[1]);
    nProp->SetUncheckedElement(2, normal[2]);
    }
  oProp->UpdateDependentDomains();
  nProp->UpdateDependentDomains();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::Update()
{
  vtkPVSource *input;

  this->Superclass::Update();

  if (this->InputMenu == NULL)
    {
    return;
    }

  input = this->InputMenu->GetCurrentValue();
  if (input)
    {
    double bds[6];
    input->GetDataInformation()->GetBounds(bds);
    this->PlaceWidget(bds);

    double center[3];
    double normal[3];
    this->WidgetProxy->UpdateInformation();
    this->GetCenterInternal(center);
    this->GetNormalInternal(normal);
  
    this->CenterEntry[0]->SetValue(center[0]);
    this->CenterEntry[1]->SetValue(center[1]);
    this->CenterEntry[2]->SetValue(center[2]);
    
    this->NormalEntry[0]->SetValue(normal[0]);
    this->NormalEntry[1]->SetValue(normal[1]);
    this->NormalEntry[2]->SetValue(normal[2]);

    vtkSMProperty *prop = this->ImplicitFunctionProxy->GetProperty("Offset");
    vtkSMBoundsDomain *rangeDomain = vtkSMBoundsDomain::SafeDownCast(
    prop->GetDomain("range"));

    if (rangeDomain)
      {
      rangeDomain->SetInputInformation(input->GetDataInformation());
      }

    prop->UpdateDependentDomains();
    }
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->InputMenu);

  this->PropagateEnableState(this->CenterResetButton);
  this->PropagateEnableState(this->NormalButtonFrame);
  this->PropagateEnableState(this->NormalCameraButton);
  this->PropagateEnableState(this->NormalXButton);
  this->PropagateEnableState(this->NormalYButton);
  this->PropagateEnableState(this->NormalZButton);

  this->PropagateEnableState(this->Labels[0]);
  this->PropagateEnableState(this->Labels[1]);

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->PropagateEnableState(this->CoordinateLabel[cc]);
    this->PropagateEnableState(this->CenterEntry[cc]);
    this->PropagateEnableState(this->NormalEntry[cc]);
    }

  this->PropagateEnableState(this->OffsetLabel);
  this->PropagateEnableState(this->OffsetEntry);
}

//-----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetupPropertyObservers()
{
  if (!this->ImplicitFunctionProxy)
    {
    return;
    }
  vtkSMProperty* p = this->ImplicitFunctionProxy->GetProperty("Origin");
  if (p)
    {
    this->AddPropertyObservers(p);
    }
  p = this->ImplicitFunctionProxy->GetProperty("Normal");
  if (p)
    {
    this->AddPropertyObservers(p);
    }
  p = this->ImplicitFunctionProxy->GetProperty("Offset");
  if (p)
    {
    this->AddPropertyObservers(p);
    }
}


//-----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::UnsetPropertyObservers()
{
  if (!this->ImplicitFunctionProxy)
    {
    return;
    }
  vtkSMProperty* p = this->ImplicitFunctionProxy->GetProperty("Origin");
  if (p)
    {
    this->RemovePropertyObservers(p);
    }
  p = this->ImplicitFunctionProxy->GetProperty("Normal");
  if (p)
    {
    this->RemovePropertyObservers(p);
    }
  p = this->ImplicitFunctionProxy->GetProperty("Offset");
  if (p)
    {
    this->RemovePropertyObservers(p);
    }
}

//-----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::UpdateVTKObjects()
{
  this->ImplicitFunctionProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::RegisterAnimateableProxies()
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
        animName << root << ".Plane" << ends;
        pm->RegisterProxy(
          "animateable", animName.str(), this->ImplicitFunctionProxy);
        delete[] animName.str();
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::UnregisterAnimateableProxies()
{
  vtkSMProxyManager* proxyM = vtkSMObject::GetProxyManager();
  if (this->ImplicitFunctionProxy)
    {
    const char* proxyName = proxyM->GetProxyName("animateable", this->ImplicitFunctionProxy);
    if (proxyName)
      {
      proxyM->UnRegisterProxy("animateable", proxyName);
      }
    }
}
