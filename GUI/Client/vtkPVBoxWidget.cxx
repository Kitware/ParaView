/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBoxWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVBoxWidget.h"

#include "vtkArrayMap.txx"
#include "vtkKWEntry.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWScale.h"
#include "vtkKWThumbWheel.h"
#include "vtkKWView.h"
#include "vtkMatrix4x4.h" 
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVDataInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVXMLElement.h"
#include "vtkSMBoxWidgetProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkCommand.h"

vtkStandardNewMacro(vtkPVBoxWidget);
vtkCxxRevisionMacro(vtkPVBoxWidget, "1.48");

vtkCxxSetObjectMacro(vtkPVBoxWidget, InputMenu, vtkPVInputMenu);

int vtkPVBoxWidgetCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVBoxWidget::vtkPVBoxWidget()
{
  this->CommandFunction = vtkPVBoxWidgetCommand;
  this->InputMenu = 0;
  this->ControlFrame = vtkKWFrame::New();
  this->TranslateLabel = vtkKWLabel::New();
  this->ScaleLabel = vtkKWLabel::New();
  this->OrientationLabel = vtkKWLabel::New();

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->TranslateThumbWheel[cc] = vtkKWThumbWheel::New();
    this->ScaleThumbWheel[cc] = vtkKWThumbWheel::New();
    this->OrientationScale[cc] = vtkKWScale::New();
    }

  this->BoxProxy = 0; // This is the implicit function proxy
  this->BoxTransformProxy = 0;
  
  this->SetWidgetProxyXMLName("BoxWidgetProxy");
}

//----------------------------------------------------------------------------
vtkPVBoxWidget::~vtkPVBoxWidget()
{
  this->SetInputMenu(NULL);
  this->ControlFrame->Delete();
  this->TranslateLabel->Delete();
  this->ScaleLabel->Delete();
  this->OrientationLabel->Delete();

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->TranslateThumbWheel[cc]->Delete();
    this->ScaleThumbWheel[cc]->Delete();
    this->OrientationScale[cc]->Delete();
    }
  if(this->BoxProxy)
    {
    vtkSMProxyManager* proxyM = vtkSMObject::GetProxyManager();
    const char* proxyName 
      = proxyM->GetProxyName("implicit_functions", this->BoxProxy);
    if (proxyName)
      {
      proxyM->UnRegisterProxy("implicit_functions", proxyName);
      }
    proxyName = proxyM->GetProxyName("animateable", this->BoxProxy);
    if (proxyName)
      {
      proxyM->UnRegisterProxy("animateable", proxyName);
      }
    this->BoxProxy->Delete();
    this->BoxProxy = 0;
    }

  if(this->BoxTransformProxy)
    {
    vtkSMProxyManager* proxyM = vtkSMObject::GetProxyManager();
    const char* proxyName 
      = proxyM->GetProxyName("transforms", this->BoxTransformProxy);
    if (proxyName)
      {
      proxyM->UnRegisterProxy("transforms", proxyName);
      }
    proxyName = proxyM->GetProxyName("animateable", this->BoxTransformProxy);
    if (proxyName)
      {
      proxyM->UnRegisterProxy("animateable", proxyName);
      }
    this->BoxTransformProxy->Delete();
    this->BoxTransformProxy = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::Initialize()
{
  this->PlaceWidget();

  this->Accept();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::ResetInternal()
{
  if ( !this->ModifiedFlag )
    {
    return;
    }
  const char* properties[] = {"Scale","Position","Rotation", 0 };
  int i;
  for (i=0;properties[i]; i++)
    {
    vtkSMDoubleVectorProperty* sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->BoxTransformProxy->GetProperty(properties[i]));
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->WidgetProxy->GetProperty(properties[i]));
    if (sdvp && dvp)
      {
      dvp->SetElements(sdvp->GetElements());
      }
    else
      {
      vtkErrorMacro("BoxTransformProxy or WidgetProxy has missing property " << properties[i]);
      }
    }
  
  this->WidgetProxy->UpdateVTKObjects(); 
  this->Superclass::ResetInternal();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::PlaceWidget(double bds[6])
{
  this->Superclass::PlaceWidget(bds);
  if (this->BoxProxy)
    {
    vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->BoxProxy->GetProperty("Bounds"));
    if (dvp)
      {
      dvp->SetElements(bds);
      }
    this->BoxProxy->UpdateVTKObjects(); 
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::Accept()
{
  int modFlag = this->GetModifiedFlag();
  
  double values[3][3];
  this->WidgetProxy->UpdateInformation();
  this->GetScaleInternal(values[0]);
  this->GetPositionInternal(values[1]);
  this->GetRotationInternal(values[2]);

  const char* properties[] = { "Scale","Position","Rotation", 0 };
  int i;
  for (i=0; properties[i]; i++)
    {
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->BoxProxy->GetProperty(properties[i]));
    if (dvp)
      {
      dvp->SetElements(values[i]);
      }
    else
      {
      vtkErrorMacro("BoxProxy does not have "<< properties[i] <<" property");
      }  
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->BoxTransformProxy->GetProperty(properties[i]));
    if (dvp)
      {
      dvp->SetElements(values[i]);
      }
    else
      {
      vtkErrorMacro("BoxTransformProxy does not have "<< properties[i] <<" property");
      }
    }
  this->BoxProxy->UpdateVTKObjects();
  this->BoxTransformProxy->UpdateVTKObjects();
  
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

}

//---------------------------------------------------------------------------
void vtkPVBoxWidget::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  this->GetRotationFromGUI();
  this->GetScaleFromGUI();
  this->GetPositionFromGUI();

  *file << "$kw(" << this->GetTclName() << ") SetScale "
    << this->ScaleGUI[0] << " "
    << this->ScaleGUI[1] << " "
    << this->ScaleGUI[2] << endl;
  *file << "$kw(" << this->GetTclName() << ") SetTranslate "
    << this->PositionGUI[0] << " "
    << this->PositionGUI[1] << " "
    << this->PositionGUI[2] << endl;
  if ( this->RotationGUI[0] < 0 ) { this->RotationGUI[0] += 360; }
  if ( this->RotationGUI[1] < 0 ) { this->RotationGUI[1] += 360; }
  if ( this->RotationGUI[2] < 0 ) { this->RotationGUI[2] += 360; }
  *file << "$kw(" << this->GetTclName() << ") SetOrientation "
    << this->RotationGUI[0] << " "
    << this->RotationGUI[1] << " "
    << this->RotationGUI[2] << endl;
  /*
  for ( cc = 0; cc < 3; cc ++ )
  {
  val[cc] = atof( this->CenterEntry[cc]->GetValue() );
  }
   *file << "$kw(" << this->GetTclName() << ") SetCenter "
   << val[0] << " " << val[1] << " " << val[2] << endl;

   rad = atof(this->RadiusEntry->GetValue());
   this->AddTraceEntry("$kw(%s) SetRadius %f", 
   this->GetTclName(), rad);
   *file << "$kw(" << this->GetTclName() << ") SetRadius "
   << rad << endl;
   */
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SaveInBatchScript(ofstream *file)
{
  this->WidgetProxy->SaveInBatchScript(file);
  
  *file << endl;
  int i;
  if (this->BoxTransformProxy)
    {
    vtkClientServerID boxTransformID = this->BoxTransformProxy->GetID(0);
    *file << "set pvTemp" << boxTransformID.ID
      << " [$proxyManager NewProxy transforms Transform2]"
      << endl;
    *file << "  $proxyManager RegisterProxy transforms pvTemp" << boxTransformID.ID
      << " $pvTemp" << boxTransformID.ID << endl;
    *file << "  $pvTemp" << boxTransformID.ID << " UnRegister {}" << endl;

    //NOw, set the properties of the BoxTransformProxy
    const char *properties[] = { "Rotation", "Scale", "Position" , 0};
    for (i=0; properties[i] != 0; i++)
      {
      vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
        this->BoxProxy->GetProperty(properties[i]));
      if (dvp)
        {
        *file << "  [$pvTemp" << boxTransformID.ID << " GetProperty " << properties[i] 
          << "] SetElement 0 " << dvp->GetElement(0) << endl;
        *file << "  [$pvTemp" << boxTransformID.ID << " GetProperty " << properties[i] 
          << "] SetElement 1 " << dvp->GetElement(1) << endl;
        *file << "  [$pvTemp" << boxTransformID.ID << " GetProperty " << properties[i] 
          << "] SetElement 2 " << dvp->GetElement(2) << endl;
        *file << "  [$pvTemp" << boxTransformID.ID << " GetProperty " << properties[i]
          << "] SetControllerProxy $pvTemp" << this->WidgetProxy->GetID(0) << endl;
        *file << "  [$pvTemp" << boxTransformID.ID << " GetProperty " << properties[i]
          << "] SetControllerProperty [$pvTemp" << this->WidgetProxy->GetID(0)
          << " GetProperty " << properties[i] << "]" << endl;
        }
      }
    *file << "  $pvTemp" << boxTransformID.ID
      << " UpdateVTKObjects"  << endl;
    *file << endl;
    }

  if (this->BoxProxy)
    {
    vtkClientServerID boxID = this->BoxProxy->GetID(0);
    *file << "set pvTemp" << boxID.ID
      << " [$proxyManager NewProxy implicit_functions Box]" << endl;
    *file << "  $proxyManager RegisterProxy implicit_functions pvTemp" << boxID.ID
      << " $pvTemp" << boxID.ID << endl;
    *file << "  $pvTemp" << boxID.ID << " UnRegister {}" << endl;

    //Now, set the properties of the BoxProxy
    vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->BoxProxy->GetProperty("Bounds"));
    if (dvp)
      {
      for( i=0;i<6;i++)
        {
        *file << "  [$pvTemp" << boxID.ID << " GetProperty Bounds] SetElement " 
          << i << " "  << dvp->GetElement(i) << endl;
        }
      }
    const char *properties[] = { "Rotation", "Scale", "Position" , 0};
    for (i=0; properties[i] != 0; i++)
      {
      dvp = vtkSMDoubleVectorProperty::SafeDownCast(
        this->BoxProxy->GetProperty(properties[i]));
      if (dvp)
        {
        *file << "  [$pvTemp" << boxID.ID << " GetProperty " << properties[i] 
          << "] SetElement 0 " << dvp->GetElement(0) << endl;
        *file << "  [$pvTemp" << boxID.ID << " GetProperty " << properties[i] 
          << "] SetElement 1 " << dvp->GetElement(1) << endl;
        *file << "  [$pvTemp" << boxID.ID << " GetProperty " << properties[i] 
          << "] SetElement 2 " << dvp->GetElement(2) << endl;
        *file << "  [$pvTemp" << boxID.ID << " GetProperty " << properties[i]
          << "] SetControllerProxy $pvTemp" << this->WidgetProxy->GetID(0) << endl;
        *file << "  [$pvTemp" << boxID.ID << " GetProperty " << properties[i]
          << "] SetControllerProperty [$pvTemp" << this->WidgetProxy->GetID(0)
          << " GetProperty " << properties[i] << "]" << endl;

        }
      }
    *file << "  $pvTemp" << boxID.ID << " UpdateVTKObjects" << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "BoxProxy: " << this->BoxProxy << endl;
  os << indent << "BoxTransformProxy: " << this->BoxTransformProxy << endl;
  os << indent << "InputMenu: " << this->InputMenu << endl;

}

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVBoxWidget::ClonePrototypeInternal(
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

    vtkPVBoxWidget* bw = vtkPVBoxWidget::SafeDownCast(pvWidget);
    if (!bw)
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
      bw->SetInputMenu(im);
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
void vtkPVBoxWidget::SetBalloonHelpString(const char *str)
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
    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::ChildCreate(vtkPVApplication* )
{
  if ((this->TraceNameState == vtkPVWidget::Uninitialized ||
      this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName("Box");
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }

  this->SetFrameLabel("Box Widget");
  this->ControlFrame->SetParent(this->Frame->GetFrame());
  this->ControlFrame->Create(this->GetApplication(), 0);

  this->TranslateLabel->SetParent(this->ControlFrame->GetFrame());
  this->TranslateLabel->Create(this->GetApplication(), 0);
  this->TranslateLabel->SetLabel("Translate:");
  this->TranslateLabel->SetBalloonHelpString(
    "Translate the geometry relative to the dataset location.");

  this->ScaleLabel->SetParent(this->ControlFrame->GetFrame());
  this->ScaleLabel->Create(this->GetApplication(), 0);
  this->ScaleLabel->SetLabel("Scale:");
  this->ScaleLabel->SetBalloonHelpString(
    "Scale the geometry relative to the size of the dataset.");

  this->OrientationLabel->SetParent(this->ControlFrame->GetFrame());
  this->OrientationLabel->Create(this->GetApplication(), 0);
  this->OrientationLabel->SetLabel("Orientation:");
  this->OrientationLabel->SetBalloonHelpString(
    "Orient the geometry relative to the dataset origin.");

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->TranslateThumbWheel[cc]->SetParent(this->ControlFrame->GetFrame());
    this->TranslateThumbWheel[cc]->PopupModeOn();
    this->TranslateThumbWheel[cc]->SetValue(0.0);
    this->TranslateThumbWheel[cc]->SetResolution(0.001);
    this->TranslateThumbWheel[cc]->Create(this->GetApplication(), 0);
    this->TranslateThumbWheel[cc]->DisplayEntryOn();
    this->TranslateThumbWheel[cc]->DisplayEntryAndLabelOnTopOff();
    this->TranslateThumbWheel[cc]->ExpandEntryOn();
    this->TranslateThumbWheel[cc]->GetEntry()->SetWidth(5);
    this->TranslateThumbWheel[cc]->GetEntry()->SetBind(this,
      "<Key>", "SetValueChanged");
    //EntryCommand is called on <Return> and <FocusOut>
    this->TranslateThumbWheel[cc]->SetEntryCommand(this, "SetTranslate");
    //Command is called when the value is changed  
    this->TranslateThumbWheel[cc]->SetCommand(this, "SetValueChanged");
    //EndCommand is called when Thumbwheel/Scale motion is stopped
    this->TranslateThumbWheel[cc]->SetEndCommand(this,"SetTranslate");
    this->TranslateThumbWheel[cc]->SetBalloonHelpString(
      "Translate the geometry relative to the dataset location.");

    this->ScaleThumbWheel[cc]->SetParent(this->ControlFrame->GetFrame());
    this->ScaleThumbWheel[cc]->PopupModeOn();
    this->ScaleThumbWheel[cc]->SetValue(1.0);
    this->ScaleThumbWheel[cc]->SetResolution(0.001);
    this->ScaleThumbWheel[cc]->Create(this->GetApplication(), 0);
    this->ScaleThumbWheel[cc]->DisplayEntryOn();
    this->ScaleThumbWheel[cc]->DisplayEntryAndLabelOnTopOff();
    this->ScaleThumbWheel[cc]->ExpandEntryOn();
    this->ScaleThumbWheel[cc]->GetEntry()->SetWidth(5);
    this->ScaleThumbWheel[cc]->GetEntry()->SetBind(this,
      "<Key>", "SetValueChanged");
    //EntryCommand is called on <Return> and <FocusOut>
    this->ScaleThumbWheel[cc]->SetEntryCommand(this,"SetScale");
    this->ScaleThumbWheel[cc]->SetCommand(this, "SetValueChanged");
    this->ScaleThumbWheel[cc]->SetEndCommand(this, "SetScale");
    this->ScaleThumbWheel[cc]->SetBalloonHelpString(
      "Scale the geometry relative to the size of the dataset.");

    this->OrientationScale[cc]->SetParent(this->ControlFrame->GetFrame());
    this->OrientationScale[cc]->PopupScaleOn();
    this->OrientationScale[cc]->Create(this->GetApplication(), 0);
    this->OrientationScale[cc]->SetRange(0, 360);
    this->OrientationScale[cc]->SetResolution(.001);
    this->OrientationScale[cc]->SetValue(0);
    this->OrientationScale[cc]->DisplayEntry();
    this->OrientationScale[cc]->DisplayEntryAndLabelOnTopOff();
    this->OrientationScale[cc]->ExpandEntryOn();
    this->OrientationScale[cc]->GetEntry()->SetWidth(5);
    this->OrientationScale[cc]->GetEntry()->SetBind(this,
      "<Key>", "SetValueChanged");
    //EntryCommand is called on <Return> and <FocusOut>
    this->OrientationScale[cc]->SetEntryCommand(this,"SetOrientation");
    this->OrientationScale[cc]->SetCommand(this, "SetValueChanged");
    this->OrientationScale[cc]->SetEndCommand(this, "SetOrientation");
    this->OrientationScale[cc]->SetBalloonHelpString(
      "Orient the geometry relative to the dataset origin.");

    }
  int button_pady = 1;
  this->Script("grid %s %s %s %s -sticky news -pady %d",
    this->TranslateLabel->GetWidgetName(),
    this->TranslateThumbWheel[0]->GetWidgetName(),
    this->TranslateThumbWheel[1]->GetWidgetName(),
    this->TranslateThumbWheel[2]->GetWidgetName(),
    button_pady);

  this->Script("grid %s -sticky nws",
    this->TranslateLabel->GetWidgetName());

  this->Script("grid %s %s %s %s -sticky news -pady %d",
    this->ScaleLabel->GetWidgetName(),
    this->ScaleThumbWheel[0]->GetWidgetName(),
    this->ScaleThumbWheel[1]->GetWidgetName(),
    this->ScaleThumbWheel[2]->GetWidgetName(),
    button_pady);

  this->Script("grid %s -sticky nws",
    this->ScaleLabel->GetWidgetName());

  this->Script("grid %s %s %s %s -sticky news -pady %d",
    this->OrientationLabel->GetWidgetName(),
    this->OrientationScale[0]->GetWidgetName(),
    this->OrientationScale[1]->GetWidgetName(),
    this->OrientationScale[2]->GetWidgetName(),
    button_pady);

  this->Script("grid %s -sticky nws",
    this->OrientationLabel->GetWidgetName());

  this->Script("grid columnconfigure %s 0 -weight 0", 
    this->ControlFrame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 1 -weight 2", 
    this->ControlFrame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 2 -weight 2", 
    this->ControlFrame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 3 -weight 2", 
    this->ControlFrame->GetFrame()->GetWidgetName());

  this->Script("pack %s -fill x -expand t -pady 2",
    this->ControlFrame->GetWidgetName());

  // Initialize the center of the sphere based on the input bounds.
  if (this->PVSource)
    {
    vtkPVSource *input = this->PVSource->GetPVInput(0);
    if (input)
      {
      this->Reset();
      this->ActualPlaceWidget();
      }
    }

  this->SetBalloonHelpString(this->BalloonHelpString);
}
//----------------------------------------------------------------------------
void vtkPVBoxWidget::Create( vtkKWApplication *app)
{
  this->Superclass::Create(app);

  static int instanceCount = 0;
  vtkSMProxyManager *pm = vtkSMObject::GetProxyManager();
  this->BoxProxy = pm->NewProxy("implicit_functions", "Box");
  ostrstream str1;
  str1 << "vtkPVBoxWidget_Box" << instanceCount << ends;
  pm->RegisterProxy("implicit_functions", str1.str(), this->BoxProxy);
  delete[] str1.str();
  if (this->PVSource)
    {
    vtkSMSourceProxy* sproxy = this->PVSource->GetProxy();
    if (sproxy)
      {
      const char* root = pm->GetProxyName("animateable", sproxy);
      if (root)
        {
        ostrstream animName;
        animName << root << ";Box" << ends;
        pm->RegisterProxy("animateable", animName.str(), this->BoxProxy);
        delete[] animName.str();
        }
      }
    }

  this->BoxTransformProxy = pm->NewProxy("transforms", "Transform2");
  ostrstream str2;
  str2 << "vtkPVBoxWidget_BoxTransform" << instanceCount << ends;
  pm->RegisterProxy("transforms", str2.str(), this->BoxTransformProxy);
  delete[] str2.str();
  if (this->PVSource)
    {
    vtkSMSourceProxy* sproxy = this->PVSource->GetProxy();
    if (sproxy)
      {
      const char* root = pm->GetProxyName("animateable", sproxy);
      if (root)
        {
        ostrstream animName;
        animName << root << ";BoxTransform" << ends;
        pm->RegisterProxy(
          "animateable", animName.str(), this->BoxTransformProxy);
        delete[] animName.str();
        }
      }
    }
  this->SetupPropertyObservers();
  instanceCount++;
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetScale()
{
  if (!this->ValueChanged)
    {
    return;
    }
  double val[3];
  int cc;
  for ( cc = 0; cc < 3; cc++)
    {
    val[cc] = atof(this->ScaleThumbWheel[cc]->GetEntry()->GetValue());
    }
  this->SetScale(val[0],val[1],val[2]);
  this->Render();
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetTranslate()
{
  if ( !this->ValueChanged )
    {
    return;
    }
  double val[3];
  int cc;
  for ( cc = 0; cc < 3; cc++)
    {
    val[cc] = atof(this->TranslateThumbWheel[cc]->GetEntry()->GetValue());
    }
  this->SetTranslate(val[0],val[1],val[2]);
  this->Render();
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetOrientation()
{
  if (!this->ValueChanged)
    {
    return;
    }
  double val[3];
  int cc;
  for ( cc = 0; cc < 3; cc++)
    {
    val[cc] = atof(this->OrientationScale[cc]->GetEntry()->GetValue());
    }
  this->SetOrientation(val[0],val[1],val[2]);
  this->Render();
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetOrientationInternal(double x, double y, double z)
{
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Rotation"));
  if (dvp)
    {
    dvp->SetElements3(x,y,z);
    }
  this->WidgetProxy->UpdateVTKObjects();

  this->OrientationScale[0]->GetEntry()->SetValue(x);
  this->OrientationScale[1]->GetEntry()->SetValue(y);
  this->OrientationScale[2]->GetEntry()->SetValue(z);
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetTranslateInternal(double x, double y, double z)
{
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Position"));
  if (dvp)
    {
    dvp->SetElements3(x,y,z);
    }
  this->WidgetProxy->UpdateVTKObjects();

  this->TranslateThumbWheel[0]->GetEntry()->SetValue(x);
  this->TranslateThumbWheel[1]->GetEntry()->SetValue(y);
  this->TranslateThumbWheel[2]->GetEntry()->SetValue(z);
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetScaleInternal(double x, double y, double z)
{
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Scale"));
  if(dvp)
    {
    dvp->SetElements3(x,y,z);
    }
  this->WidgetProxy->UpdateVTKObjects();
  
  this->ScaleThumbWheel[0]->GetEntry()->SetValue(x);
  this->ScaleThumbWheel[1]->GetEntry()->SetValue(y);
  this->ScaleThumbWheel[2]->GetEntry()->SetValue(z);
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetOrientation(double px, double py, double pz)
{
  if ( px < 0 ) { px += 360; }
  if ( py < 0 ) { py += 360; }
  if ( pz < 0 ) { pz += 360; }
  this->SetOrientationInternal(px, py, pz);
  this->AddTraceEntry("$kw(%s) SetOrientation %f %f %f",
    this->GetTclName(), px, py, pz);  
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetScale(double px, double py, double pz)
{
  this->SetScaleInternal(px, py, pz);
  this->AddTraceEntry("$kw(%s) SetScale %f %f %f",
    this->GetTclName(), px, py, pz);  
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetTranslate(double px, double py, double pz)
{
  this->SetTranslateInternal(px, py, pz);
  this->AddTraceEntry("$kw(%s) SetTranslate %f %f %f",
    this->GetTclName(), px, py, pz);  
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::GetScaleInternal(double scale[3])
{
 vtkSMDoubleVectorProperty *dvpScale = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("ScaleInfo")); 
 if (dvpScale)
   {
   scale[0] = dvpScale->GetElement(0);
   scale[1] = dvpScale->GetElement(1);
   scale[2] = dvpScale->GetElement(2);
   }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::GetRotationInternal(double rotation[3])
{
  vtkSMDoubleVectorProperty *dvpRotation = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("RotationInfo"));
  if (dvpRotation)
    {
    rotation[0] = dvpRotation->GetElement(0);
    rotation[1] = dvpRotation->GetElement(1);
    rotation[2] = dvpRotation->GetElement(2);
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::GetPositionInternal(double position[3])
{
  vtkSMDoubleVectorProperty *dvpPosition = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("PositionInfo"));
  if (dvpPosition)
    {
    position[0] = dvpPosition->GetElement(0);
    position[1] = dvpPosition->GetElement(1);
    position[2] = dvpPosition->GetElement(2);
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::UpdateFromBox()
{
  this->WidgetProxy->UpdateInformation();

  double scale[3], position[3], rotation[3];
  this->GetScaleInternal(scale);
  this->GetPositionInternal(position);
  this->GetRotationInternal(rotation);

  this->ScaleThumbWheel[0]->SetValue(scale[0]);
  this->ScaleThumbWheel[1]->SetValue(scale[1]);
  this->ScaleThumbWheel[2]->SetValue(scale[2]);

  this->TranslateThumbWheel[0]->SetValue(position[0]);
  this->TranslateThumbWheel[1]->SetValue(position[1]);
  this->TranslateThumbWheel[2]->SetValue(position[2]);

  double orientation[3];
  orientation[0] = rotation[0];
  orientation[1] = rotation[1];
  orientation[2] = rotation[2];
  if ( orientation[0] < 0 ) { orientation[0] += 360; }
  if ( orientation[1] < 0 ) { orientation[1] += 360; }
  if ( orientation[2] < 0 ) { orientation[2] += 360; }
  this->OrientationScale[0]->SetValue(orientation[0]);
  this->OrientationScale[1]->SetValue(orientation[1]);
  this->OrientationScale[2]->SetValue(orientation[2]);
}

//----------------------------------------------------------------------------
double* vtkPVBoxWidget::GetPositionFromGUI()
{
  this->PositionGUI[0] = this->TranslateThumbWheel[0]->GetValue();
  this->PositionGUI[1] = this->TranslateThumbWheel[1]->GetValue();
  this->PositionGUI[2] = this->TranslateThumbWheel[2]->GetValue();
  return this->PositionGUI;
}

//----------------------------------------------------------------------------
double* vtkPVBoxWidget::GetRotationFromGUI()
{
  this->RotationGUI[0] = this->OrientationScale[0]->GetValue();
  this->RotationGUI[1] = this->OrientationScale[1]->GetValue();
  this->RotationGUI[2] = this->OrientationScale[2]->GetValue();
  return this->RotationGUI;
}

//----------------------------------------------------------------------------
double* vtkPVBoxWidget::GetScaleFromGUI()
{
  this->ScaleGUI[0] = this->ScaleThumbWheel[0]->GetValue();
  this->ScaleGUI[1] = this->ScaleThumbWheel[1]->GetValue();
  this->ScaleGUI[2] = this->ScaleThumbWheel[2]->GetValue();
  return this->ScaleGUI;
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::ExecuteEvent(vtkObject* wdg, unsigned long event, void* p)
{
  if(vtkSM3DWidgetProxy::SafeDownCast(wdg) && event == vtkKWEvent::WidgetModifiedEvent)
    {//case to update the display values from iVars
    this->UpdateFromBox();
    }
  if (vtkSMProperty::SafeDownCast(wdg))
    {
    switch(event)
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
  this->Superclass::ExecuteEvent(wdg, event, p);
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetupPropertyObservers()
{
  const char* properties[] = {"Scale","Position","Rotation", 0 };
  int i;
  for (i=0;properties[i]; i++)
    {
    vtkSMProperty* pT = this->BoxTransformProxy->GetProperty(properties[i]);
    vtkSMProperty* pB = this->BoxProxy->GetProperty(properties[i]);
    
    if (pT)
      {
      this->AddPropertyObservers(pT);
      }
    if (pB)
      {
      this->AddPropertyObservers(pB);
      }
    } 
}

//----------------------------------------------------------------------------
int vtkPVBoxWidget::ReadXMLAttributes(vtkPVXMLElement* element,
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
vtkSMProxy* vtkPVBoxWidget::GetProxyByName(const char *name)
{
  if (!strcmp(name, "Box"))
    {
    return this->BoxProxy;
    }
  if (!strcmp(name, "BoxTransform"))
    {
    return this->BoxTransformProxy;
    }
  vtkErrorMacro("GetProxyByName called with invalid proxy name: " << name);
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();


  this->PropagateEnableState(this->InputMenu);
  this->PropagateEnableState(this->ControlFrame);
  this->PropagateEnableState(this->TranslateLabel);
  this->PropagateEnableState(this->ScaleLabel);
  this->PropagateEnableState(this->OrientationLabel);

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->PropagateEnableState(this->TranslateThumbWheel[cc]);
    this->PropagateEnableState(this->ScaleThumbWheel[cc]);
    this->PropagateEnableState(this->OrientationScale[cc]);
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::Update()
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
    }
}
