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
#include "vtkCamera.h"
#include "vtkKWCompositeCollection.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVInputMenu.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderer.h"

#include "vtkKWFrame.h"
#include "vtkKWThumbWheel.h"
#include "vtkKWScale.h"
#include "vtkPVRenderView.h"
#include "vtkCommand.h"
#include "vtkPVProcessModule.h"

#include "vtkSMBoxWidgetProxy.h"
#include "vtkKWEvent.h"
#include "vtkMatrix4x4.h" 

#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"

vtkStandardNewMacro(vtkPVBoxWidget);
vtkCxxRevisionMacro(vtkPVBoxWidget, "1.39");

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
  
  this->BoxProxyName = 0;
  this->BoxTransformProxyName = 0;
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
  if (this->BoxProxyName)
    {
    vtkSMObject::GetProxyManager()->UnRegisterProxy("implicit_functions",
                                                    this->BoxProxyName);
    }
  this->SetBoxProxyName(0);
  if (this->BoxProxy)
    {
    this->BoxProxy->Delete();
    this->BoxProxy = 0;
    }
  
  if (this->BoxTransformProxyName)
    {
    vtkSMObject::GetProxyManager()->UnRegisterProxy("transforms",
                                                    this->BoxTransformProxyName);
    }
  this->SetBoxTransformProxyName(0);
  if (this->BoxTransformProxy)
    {
    this->BoxTransformProxy->Delete();
    this->BoxTransformProxy = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::ResetInternal()
{
  if ( !this->ModifiedFlag || this->SuppressReset)
    {
    return;
    }
  
  if ( !this->AcceptCalled)
    {
    this->ActualPlaceWidget();
    return;
    }
  
  vtkSMDoubleVectorProperty* sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->BoxTransformProxy->GetProperty("Matrix"));
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Matrix"));
  if(dvp && sdvp)
    {
    dvp->SetElements(sdvp->GetElements());
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

  this->WidgetProxy->UpdateInformation();
  vtkSMDoubleVectorProperty *matProperty = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("MatrixInfo"));
  if (matProperty)
    {
    //Set matrix on Transform Proxy
    vtkSMDoubleVectorProperty *matProp =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->BoxTransformProxy->GetProperty("Matrix"));
    if (matProp)
      {
      matProp->SetElements(matProperty->GetElements());
      }
    this->BoxTransformProxy->UpdateVTKObjects();
    //Set transform on Box proxy
    vtkSMDoubleVectorProperty *transProperty = vtkSMDoubleVectorProperty::SafeDownCast(
      this->BoxProxy->GetProperty("Transform"));
    if (transProperty)
      {
      vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
      matrix->DeepCopy(matProperty->GetElements());
      matrix->Invert();
      transProperty->SetElements(reinterpret_cast<double*>(matrix->Element));
      matrix->Delete();
      }
    else
      {
      vtkErrorMacro("BoxProxy does not have Transform property");
      }
    this->BoxProxy->UpdateVTKObjects();
    }
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

  this->AcceptCalled = 1;
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

  *file << endl;
  vtkSMDoubleVectorProperty* matrixProperty = vtkSMDoubleVectorProperty::SafeDownCast(
    this->BoxTransformProxy->GetProperty("Matrix"));
  if (matrixProperty)
    {
    vtkClientServerID boxTransformID = this->BoxTransformProxy->GetID(0);

    *file << "set pvTemp" << boxTransformID.ID
      << " [$proxyManager NewProxy transforms Transform]"
      << endl;
    *file << "  $proxyManager RegisterProxy transforms pvTemp" << boxTransformID.ID
      << " $pvTemp" << boxTransformID.ID << endl;
    *file << "  $pvTemp" << boxTransformID.ID << " UnRegister {}" << endl;
    for (int i=0; i < 16; i++)
      {
      *file << "  [$pvTemp" << boxTransformID.ID
        << " GetProperty Matrix] SetElement " << i
        << " " << matrixProperty->GetElement(i) 
        << endl;
      }
    *file << "  $pvTemp" << boxTransformID.ID
      << " UpdateVTKObjects"  << endl;
    *file << endl;
    }


  matrixProperty = vtkSMDoubleVectorProperty::SafeDownCast(
    this->BoxProxy->GetProperty("Transform"));
  if(matrixProperty)
    {
    vtkClientServerID boxID = this->BoxProxy->GetID(0);
    *file << "set pvTemp" << boxID.ID
      << " [$proxyManager NewProxy implicit_functions Box]" << endl;
    *file << "  $proxyManager RegisterProxy implicit_functions pvTemp" << boxID.ID
      << " $pvTemp" << boxID.ID << endl;
    *file << "  $pvTemp" << boxID.ID << " UnRegister {}" << endl;
    for (int i=0; i < 16; i++)
      {
      *file << "  [$pvTemp" << boxID.ID
        << " GetProperty Transform] SetElement " << i
        << " " << matrixProperty->GetElement(i) 
        << endl;
      }

    vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->BoxProxy->GetProperty("Bounds"));
    if (dvp)
      {
      for(int i=0;i<6;i++)
        {
        *file << "  [$pvTemp" << boxID.ID
          << " GetProperty Bounds] SetElement " << i << " "
          << dvp->GetElement(i)
          << endl;
        }
      }

    *file << "  $pvTemp" << boxID.ID << " UpdateVTKObjects" << endl;
    }
  this->WidgetProxy->SaveInBatchScript(file);
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "BoxProxyName: " << (this->BoxProxyName? this->BoxProxyName: "None") << endl;
  os << indent << "BoxProxy: " << this->BoxProxy << endl;
  os << indent << "BoxTransformProxyName: " 
    << (this->BoxTransformProxyName? this->BoxTransformProxyName : "None" )<< endl;
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
      "<KeyRelease>", "TranslateKeyPressCallback");
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
      "<KeyRelease>", "ScaleKeyPressCallback");
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
      "<KeyRelease>", "OrientationKeyPressCallback");
    this->OrientationScale[cc]->SetBalloonHelpString(
      "Orient the geometry relative to the dataset origin.");

    }
  this->EnableCallbacks();
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
  this->SetBoxProxyName(str1.str());
  pm->RegisterProxy("implicit_functions", this->BoxProxyName, this->BoxProxy);
  this->BoxProxy->CreateVTKObjects(1);
  str1.rdbuf()->freeze(0);

  this->BoxTransformProxy = pm->NewProxy("transforms", "Transform");
  ostrstream str2;
  str2 << "vtkPVBoxWidget_BoxTransform" << instanceCount << ends;
  this->SetBoxTransformProxyName(str2.str());
  pm->RegisterProxy("transforms", this->BoxTransformProxyName,
    this->BoxTransformProxy);
  this->BoxTransformProxy->CreateVTKObjects(1);
  str2.rdbuf()->freeze(0);

  instanceCount++;
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::EnableCallbacks()
{
  int cc;
  for(cc=0 ; cc < 3 ; cc++)
    {
    this->TranslateThumbWheel[cc]->SetCommand(this, "TranslateCallback");
    //this->TranslateThumbWheel[cc]->SetEndCommand(this, "TranslateEndCallback");
    this->TranslateThumbWheel[cc]->SetEntryCommand(this,"TranslateCallback");

    this->ScaleThumbWheel[cc]->SetCommand(this, "ScaleCallback");
    //this->ScaleThumbWheel[cc]->SetEndCommand(this, "ScaleEndCallback");
    this->ScaleThumbWheel[cc]->SetEntryCommand(this, "ScaleCallback");

    this->OrientationScale[cc]->SetCommand(this, "OrientationCallback");
    //this->OrientationScale[cc]->SetEndCommand(this, "OrientationEndCallback");
    this->OrientationScale[cc]->SetEntryCommand(this,"OrientationCallback");
    }
}
//----------------------------------------------------------------------------
void vtkPVBoxWidget::DisableCallbacks()
{
  int cc;
  for(cc=0; cc < 3; cc++)
    {
    this->TranslateThumbWheel[cc]->SetCommand(NULL, NULL);
    this->TranslateThumbWheel[cc]->SetEndCommand(NULL, NULL);
    this->TranslateThumbWheel[cc]->SetEntryCommand(NULL,NULL);

    this->ScaleThumbWheel[cc]->SetCommand(NULL, NULL);
    this->ScaleThumbWheel[cc]->SetEndCommand(NULL, NULL);
    this->ScaleThumbWheel[cc]->SetEntryCommand(NULL, NULL);

    this->OrientationScale[cc]->SetCommand(NULL, NULL);
    this->OrientationScale[cc]->SetEndCommand(NULL,NULL);
    this->OrientationScale[cc]->SetEntryCommand(NULL,NULL);
    }
}
//----------------------------------------------------------------------------
void vtkPVBoxWidget::ScaleCallback()
{
  this->GetScaleFromGUI();
  this->SetScaleInternal(this->ScaleGUI[0], this->ScaleGUI[1],
    this->ScaleGUI[2]);
  this->Render();
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::TranslateCallback()
{
  this->GetPositionFromGUI();
  this->SetTranslateInternal(this->PositionGUI[0],this->PositionGUI[1],
    this->PositionGUI[2]);
  this->Render();
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::OrientationCallback()
{
  this->GetRotationFromGUI();
  this->SetOrientationInternal(this->RotationGUI[0], this->RotationGUI[1],
    this->RotationGUI[2]);
  this->Render();
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::ScaleEndCallback()
{
  this->SetScale(this->GetScaleFromGUI());
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::TranslateEndCallback()
{
  this->SetTranslate(this->GetPositionFromGUI());
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::OrientationEndCallback()
{
  this->SetOrientation(this->GetRotationFromGUI());
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::ScaleKeyPressCallback()
{
  const char* pos0 = this->ScaleThumbWheel[0]->GetEntry()->GetValue();
  const char* pos1 = this->ScaleThumbWheel[1]->GetEntry()->GetValue();
  const char* pos2 = this->ScaleThumbWheel[2]->GetEntry()->GetValue();
  if ( *pos0 && *pos1 && *pos2 )
    {
    this->ScaleThumbWheel[0]->EntryCallback();
    this->ScaleThumbWheel[1]->EntryCallback();
    this->ScaleThumbWheel[2]->EntryCallback();
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::TranslateKeyPressCallback()
{
  const char* pos0 = this->TranslateThumbWheel[0]->GetEntry()->GetValue();
  const char* pos1 = this->TranslateThumbWheel[1]->GetEntry()->GetValue();
  const char* pos2 = this->TranslateThumbWheel[2]->GetEntry()->GetValue();
  if ( *pos0 && *pos1 && *pos2 )
    {
    this->TranslateThumbWheel[0]->EntryCallback();
    this->TranslateThumbWheel[1]->EntryCallback();
    this->TranslateThumbWheel[2]->EntryCallback();
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::OrientationKeyPressCallback()
{
  const char* pos0 = this->OrientationScale[0]->GetEntry()->GetValue();
  const char* pos1 = this->OrientationScale[1]->GetEntry()->GetValue();
  const char* pos2 = this->OrientationScale[2]->GetEntry()->GetValue();
  if ( *pos0 && *pos1 && *pos2 )
    {
    this->OrientationScale[0]->EntryValueCallback();
    this->OrientationScale[1]->EntryValueCallback();
    this->OrientationScale[2]->EntryValueCallback();
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetOrientationInternal(double x, double y, double z)
{
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Rotation"));
  if (dvp)
    {
    dvp->SetElement(0,x);
    dvp->SetElement(1,y);
    dvp->SetElement(2,z);
    }
  this->WidgetProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetTranslateInternal(double x, double y, double z)
{
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Position"));
  if (dvp)
    {
    dvp->SetElement(0,x);
    dvp->SetElement(1,y);
    dvp->SetElement(2,z);
    }
  this->WidgetProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetScaleInternal(double x, double y, double z)
{
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Scale"));
  if(dvp)
    {
    dvp->SetElement(0,x);
    dvp->SetElement(1,y);
    dvp->SetElement(2,z);
    }
  this->WidgetProxy->UpdateVTKObjects();
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
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetScale(double px, double py, double pz)
{
  this->SetScaleInternal(px, py, pz);
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetTranslate(double px, double py, double pz)
{
  this->SetTranslateInternal(px, py, pz);
  this->AddTraceEntry("$kw(%s) SetTranslate %f %f %f",
    this->GetTclName(), px, py, pz);  
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::UpdateFromBox()
{
  this->DisableCallbacks();
  this->WidgetProxy->UpdateInformation();

  vtkSMDoubleVectorProperty *dvpScale = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("ScaleInfo"));
  vtkSMDoubleVectorProperty *dvpRotation = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("RotationInfo"));
  vtkSMDoubleVectorProperty *dvpPosition = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("PositionInfo"));

  if(dvpScale)
    {
    this->ScaleThumbWheel[0]->SetValue(dvpScale->GetElement(0));
    this->ScaleThumbWheel[1]->SetValue(dvpScale->GetElement(1));
    this->ScaleThumbWheel[2]->SetValue(dvpScale->GetElement(2));
    }

  if(dvpPosition)
    {
    this->TranslateThumbWheel[0]->SetValue(dvpPosition->GetElement(0));
    this->TranslateThumbWheel[1]->SetValue(dvpPosition->GetElement(1));
    this->TranslateThumbWheel[2]->SetValue(dvpPosition->GetElement(2));
    }

  if(dvpRotation)
    {
    double orientation[3];
    orientation[0] = dvpRotation->GetElement(0);
    orientation[1] = dvpRotation->GetElement(1);
    orientation[2] = dvpRotation->GetElement(2);
    if ( orientation[0] < 0 ) { orientation[0] += 360; }
    if ( orientation[1] < 0 ) { orientation[1] += 360; }
    if ( orientation[2] < 0 ) { orientation[2] += 360; }
    this->OrientationScale[0]->SetValue(orientation[0]);
    this->OrientationScale[1]->SetValue(orientation[1]);
    this->OrientationScale[2]->SetValue(orientation[2]);
    }
  this->EnableCallbacks();
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
void vtkPVBoxWidget::ExecuteEvent(vtkObject* wdg, unsigned long l, void* p)
{
  if(l == vtkKWEvent::WidgetModifiedEvent)
    {//case to update the display values from iVars
    this->UpdateFromBox();
    if ( this->GetPVSource()->GetPVRenderView() )
      {
      this->GetPVSource()->GetPVRenderView()->EventuallyRender();
      }
    }
  this->Superclass::ExecuteEvent(wdg, l, p);
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
