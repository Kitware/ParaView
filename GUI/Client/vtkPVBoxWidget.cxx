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
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkBoxWidget.h"
#include "vtkRenderer.h"

#include "vtkKWFrame.h"
#include "vtkKWThumbWheel.h"
#include "vtkKWScale.h"
#include "vtkPVRenderView.h"
#include "vtkTransform.h"
#include "vtkCommand.h"
#include "vtkPVProcessModule.h"
#include "vtkPlanes.h"
#include "vtkPlane.h"

vtkStandardNewMacro(vtkPVBoxWidget);
vtkCxxRevisionMacro(vtkPVBoxWidget, "1.30");

int vtkPVBoxWidgetCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVBoxWidget::vtkPVBoxWidget()
{
  this->CommandFunction = vtkPVBoxWidgetCommand;
  this->BoxID.ID = 0;
  this->BoxMatrixID.ID = 0;
  this->BoxTransformID.ID = 0;

  this->BoxTransform = 0;
  this->Box = 0;

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

    this->PositionGUI[cc] = 0.0;
    this->ScaleGUI[cc]    = 1.0;
    this->RotationGUI[cc] = 0.0;
    this->StoredPosition[cc] = 0.0;
    this->StoredScale[cc]    = 1.0;
    this->StoredRotation[cc] = 0.0;
    }

  this->Initialized = 0;
}

//----------------------------------------------------------------------------
vtkPVBoxWidget::~vtkPVBoxWidget()
{
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
  vtkPVProcessModule* pm = 0;
  if(this->GetPVApplication())
    {
    pm = this->GetPVApplication()->GetProcessModule();
    }
  if (pm && this->BoxID.ID)
    {
    pm->DeleteStreamObject(this->BoxID);
    this->BoxID.ID = 0;
    }
  if (pm && this->BoxTransformID.ID )
    {
    pm->DeleteStreamObject(this->BoxTransformID);
    this->BoxTransformID.ID = 0; 
    }
  if (pm && this->BoxMatrixID.ID)
    {
    pm->DeleteStreamObject(this->BoxMatrixID);
    this->BoxMatrixID.ID = 0;
    }
  if(pm)
    {
    pm->SendStreamToRenderServerClientAndServer();
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::ResetInternal()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }
  if ( this->BoxID.ID )
    {
    //this->Script("eval %s SetState [ %s GetState ]", 
    //this->GetTclName(), this->BoxTclName);
    //this->SetPositionGUI(this->StoredPosition);
    //this->SetRotationGUI(this->StoredRotation);
    //this->SetScaleGUI(this->StoredScale);
    if ( this->StoredRotation[0] < 0 ) { this->StoredRotation[0] += 360; }
    if ( this->StoredRotation[1] < 0 ) { this->StoredRotation[1] += 360; }
    if ( this->StoredRotation[2] < 0 ) { this->StoredRotation[2] += 360; }
    this->OrientationScale[0]->SetValue(this->StoredRotation[0]);
    this->OrientationScale[1]->SetValue(this->StoredRotation[1]);
    this->OrientationScale[2]->SetValue(this->StoredRotation[2]);
    this->ScaleThumbWheel[0]->SetValue(this->StoredScale[0]);
    this->ScaleThumbWheel[1]->SetValue(this->StoredScale[1]);
    this->ScaleThumbWheel[2]->SetValue(this->StoredScale[2]);
    this->TranslateThumbWheel[0]->SetValue(this->StoredPosition[0]);
    this->TranslateThumbWheel[1]->SetValue(this->StoredPosition[1]);
    this->TranslateThumbWheel[2]->SetValue(this->StoredPosition[2]);
    this->UpdateBox(1);
    }
  this->Superclass::ResetInternal();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::ActualPlaceWidget()
{
  this->Superclass::ActualPlaceWidget();
  vtkPVApplication *pvApp = static_cast<vtkPVApplication*>(
    this->Application);
  vtkPVProcessModule* pm = pvApp->GetProcessModule(); 
  // now make sure the widget is placed on the server
  // since this is the only 3d widget on the server....
  double bds[6];
  this->PVSource->GetPVInput(0)->GetDataInformation()->GetBounds(bds);
  pvApp->GetProcessModule()->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID
                  << "PlaceWidget" 
                  << bds[0] << bds[1] << bds[2] << bds[3] 
                  << bds[4] << bds[5] << vtkClientServerStream::End;
  pvApp->GetProcessModule()->SendStreamToServer();
  
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->Widget3DID << "GetPlanes" << this->BoxID 
                  << vtkClientServerStream::End;
  pm->SendStreamToRenderServerClientAndServer();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::AcceptInternal(vtkClientServerID sourceID)  
{
  vtkPVApplication *pvApp = static_cast<vtkPVApplication*>(this->Application);
  vtkPVProcessModule* pm = pvApp->GetProcessModule();

  this->PlaceWidget(); 
  if ( ! this->ModifiedFlag)
    {
    return;
    }
  if ( this->BoxID.ID )
    {
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->Widget3DID << "GetPlanes" << this->BoxID 
                    << vtkClientServerStream::End;
    this->SetStoredPosition(this->PositionGUI);
    this->SetStoredRotation(this->RotationGUI);
    this->SetStoredScale(this->ScaleGUI);
    pm->SendStreamToRenderServerClientAndServer(); 
    }
  this->Superclass::AcceptInternal(sourceID);
  this->Initialized = 1;
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
void vtkPVBoxWidget::UpdateVTKObject(const char*)
{
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SaveInBatchScript(ofstream *file)
{
  vtkTransform* trans = this->BoxTransform;
  trans->Identity();
  trans->Translate(this->GetPositionFromGUI());
  this->GetRotationFromGUI();
  trans->RotateZ(this->RotationGUI[2]);
  trans->RotateX(this->RotationGUI[0]);
  trans->RotateY(this->RotationGUI[1]);
  trans->Scale(this->GetScaleFromGUI());
  vtkMatrix4x4* mat = trans->GetMatrix();

  *file << endl;
  *file << "set pvTemp" << this->BoxMatrixID.ID
        << " [$proxyManager NewProxy matrices Matrix4x4]"
        << endl;
  *file << "  $proxyManager RegisterProxy math pvTemp" << this->BoxMatrixID.ID
        << " $pvTemp" << this->BoxMatrixID.ID << endl;
  *file << "  $pvTemp" << this->BoxMatrixID.ID << " UnRegister {}" << endl;

  for(int i=0; i<16; i++)
    {
    *file << "  [$pvTemp" << this->BoxMatrixID.ID
          << " GetProperty DeepCopy] SetElement " << i
          << " " << *(&mat->Element[0][0] + i)
          << endl;
    }
  *file << "  $pvTemp" << this->BoxMatrixID.ID
        << " UpdateVTKObjects" << endl;

  *file << endl;
  *file << "set pvTemp" << this->BoxTransformID.ID
        << " [$proxyManager NewProxy transforms Transform]"
        << endl;
  *file << "$proxyManager RegisterProxy math pvTemp" << this->BoxTransformID.ID
        << " $pvTemp" << this->BoxTransformID.ID << endl;
  *file << " $pvTemp" << this->BoxTransformID.ID << " UnRegister {}" << endl;
  *file << "  [$pvTemp" << this->BoxTransformID.ID
        << " GetProperty Matrix] AddProxy $pvTemp" << this->BoxMatrixID.ID
        << endl;
  *file << "  $pvTemp" << this->BoxTransformID.ID
        << " UpdateVTKObjects"  << endl;

  *file << endl;
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "BoxID: " << this->BoxID.ID
     << endl;
  os << indent << "Box: " 
    << this->Box << endl;
  os << indent << "BoxTransform: " 
    << this->BoxTransform << endl;
  os << indent << "BoxTransformID" << this->BoxTransformID << endl;
}

//----------------------------------------------------------------------------
vtkPVBoxWidget* vtkPVBoxWidget::ClonePrototype(vtkPVSource* pvSource,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVBoxWidget::SafeDownCast(clone);
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

  if ( this->Application && !this->BalloonHelpInitialized )
    {
    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::ChildCreate(vtkPVApplication* pvApp)
{
  vtkPVProcessModule* pm = pvApp->GetProcessModule();

  if ((this->TraceNameState == vtkPVWidget::Uninitialized ||
      this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName("Box");
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }

  this->Widget3DID = pm->NewStreamObject("vtkBoxWidget");
  pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID << "SetPlaceFactor" << 1.0 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID << "PlaceWidget"
                  << 0 << 1 << 0 << 1 << 0 << 1
                  << vtkClientServerStream::End;
  pm->SendStreamToRenderServerClientAndServer();
  this->BoxID = pm->NewStreamObject("vtkPlanes");
  this->BoxMatrixID = pm->NewStreamObject("vtkMatrix4x4");
  this->BoxTransformID = pm->NewStreamObject("vtkTransform");
  pm->SendStreamToRenderServerClientAndServer();
  
  this->Box = vtkPlanes::SafeDownCast(pm->GetObjectFromID(this->BoxID));
  this->BoxTransform = vtkTransform::SafeDownCast(pm->GetObjectFromID(this->BoxTransformID));
  this->SetFrameLabel("Box Widget");

  this->ControlFrame->SetParent(this->Frame->GetFrame());
  this->ControlFrame->Create(this->Application, 0);

  this->TranslateLabel->SetParent(this->ControlFrame->GetFrame());
  this->TranslateLabel->Create(this->Application, 0);
  this->TranslateLabel->SetLabel("Translate:");
  this->TranslateLabel->SetBalloonHelpString(
    "Translate the geometry relative to the dataset location.");

  this->ScaleLabel->SetParent(this->ControlFrame->GetFrame());
  this->ScaleLabel->Create(this->Application, 0);
  this->ScaleLabel->SetLabel("Scale:");
  this->ScaleLabel->SetBalloonHelpString(
    "Scale the geometry relative to the size of the dataset.");

  this->OrientationLabel->SetParent(this->ControlFrame->GetFrame());
  this->OrientationLabel->Create(this->Application, 0);
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
    this->TranslateThumbWheel[cc]->Create(this->Application, 0);
    this->TranslateThumbWheel[cc]->DisplayEntryOn();
    this->TranslateThumbWheel[cc]->DisplayEntryAndLabelOnTopOff();
    this->TranslateThumbWheel[cc]->ExpandEntryOn();
    this->TranslateThumbWheel[cc]->GetEntry()->SetWidth(5);
    this->TranslateThumbWheel[cc]->SetCommand(this, "TranslateCallback");
    this->TranslateThumbWheel[cc]->SetEndCommand(this, 
                                                 "TranslateEndCallback");
    this->TranslateThumbWheel[cc]->SetEntryCommand(this,
                                                   "TranslateEndCallback");
    this->TranslateThumbWheel[cc]->GetEntry()->SetBind(this,
      "<KeyRelease>", "TranslateKeyPressCallback");
    this->TranslateThumbWheel[cc]->SetBalloonHelpString(
      "Translate the geometry relative to the dataset location.");

    this->ScaleThumbWheel[cc]->SetParent(this->ControlFrame->GetFrame());
    this->ScaleThumbWheel[cc]->PopupModeOn();
    this->ScaleThumbWheel[cc]->SetValue(1.0);
    this->ScaleThumbWheel[cc]->SetMinimumValue(0.0);
    this->ScaleThumbWheel[cc]->ClampMinimumValueOn();
    this->ScaleThumbWheel[cc]->SetResolution(0.001);
    this->ScaleThumbWheel[cc]->Create(this->Application, 0);
    this->ScaleThumbWheel[cc]->DisplayEntryOn();
    this->ScaleThumbWheel[cc]->DisplayEntryAndLabelOnTopOff();
    this->ScaleThumbWheel[cc]->ExpandEntryOn();
    this->ScaleThumbWheel[cc]->GetEntry()->SetWidth(5);
    this->ScaleThumbWheel[cc]->SetCommand(this, "ScaleCallback");
    this->ScaleThumbWheel[cc]->SetEndCommand(this, "ScaleEndCallback");
    this->ScaleThumbWheel[cc]->SetEntryCommand(this, "ScaleEndCallback");
    this->ScaleThumbWheel[cc]->GetEntry()->SetBind(this,
      "<KeyRelease>", "ScaleKeyPressCallback");
    this->ScaleThumbWheel[cc]->SetBalloonHelpString(
      "Scale the geometry relative to the size of the dataset.");

    this->OrientationScale[cc]->SetParent(this->ControlFrame->GetFrame());
    this->OrientationScale[cc]->PopupScaleOn();
    this->OrientationScale[cc]->Create(this->Application, 0);
    this->OrientationScale[cc]->SetRange(0, 360);
    this->OrientationScale[cc]->SetResolution(.001);
    this->OrientationScale[cc]->SetValue(0);
    this->OrientationScale[cc]->DisplayEntry();
    this->OrientationScale[cc]->DisplayEntryAndLabelOnTopOff();
    this->OrientationScale[cc]->ExpandEntryOn();
    this->OrientationScale[cc]->GetEntry()->SetWidth(5);
    this->OrientationScale[cc]->SetCommand(this, "OrientationCallback");
    this->OrientationScale[cc]->SetEndCommand(this, 
                                              "OrientationEndCallback");
    this->OrientationScale[cc]->SetEntryCommand(this, 
                                                "OrientationEndCallback");
    this->OrientationScale[cc]->GetEntry()->SetBind(this,
      "<KeyRelease>", "OrientationKeyPressCallback");
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
void vtkPVBoxWidget::ScaleCallback()
{
  this->SetScaleNoTrace(this->GetScaleFromGUI());
  if ( this->GetPVSource()->GetPVRenderView() )
    {
    this->GetPVSource()->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::TranslateCallback()
{
  this->SetTranslateNoTrace(this->GetPositionFromGUI());
  if ( this->GetPVSource()->GetPVRenderView() )
    {
    this->GetPVSource()->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::OrientationCallback()
{
  this->SetOrientationNoTrace(this->GetRotationFromGUI());
  if ( this->GetPVSource()->GetPVRenderView() )
    {
    this->GetPVSource()->GetPVRenderView()->EventuallyRender();
    }
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
void vtkPVBoxWidget::SetOrientationNoTrace(float, float, float)
{
  this->UpdateBox(1);
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetTranslateNoTrace(float, float, float)
{
  this->UpdateBox(1);
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetScaleNoTrace(float, float, float)
{
  this->UpdateBox(1);
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetOrientation(float px, float py, float pz)
{
  this->GetRotationFromGUI();
  if ( !( px == this->RotationGUI[0] && 
      py == this->RotationGUI[1] && 
      pz == this->RotationGUI[2] ) )
    {
    if ( this->RotationGUI[0] < 0 ) { this->RotationGUI[0] += 360; }
    if ( this->RotationGUI[1] < 0 ) { this->RotationGUI[1] += 360; }
    if ( this->RotationGUI[2] < 0 ) { this->RotationGUI[2] += 360; }
    if ( px < 0 ) { px += 360; }
    if ( py < 0 ) { py += 360; }
    if ( pz < 0 ) { pz += 360; }
    this->OrientationScale[0]->SetValue(px);
    this->OrientationScale[1]->SetValue(py);
    this->OrientationScale[2]->SetValue(pz);
    }
  this->SetOrientationNoTrace(px, py, pz);
  if ( this->GetPVSource()->GetPVRenderView() )
    {
    this->GetPVSource()->GetPVRenderView()->EventuallyRender();
    }

  this->AddTraceEntry("$kw(%s) SetOrientation %f %f %f",
                      this->GetTclName(), px, py, pz);  
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetScale(float px, float py, float pz)
{
  this->GetScaleFromGUI();
  if ( !( px == this->ScaleGUI[0] && 
      py == this->ScaleGUI[1] && 
      pz == this->ScaleGUI[2] ) )
    {
    this->ScaleThumbWheel[0]->SetValue(px);
    this->ScaleThumbWheel[1]->SetValue(py);
    this->ScaleThumbWheel[2]->SetValue(pz);
    }
  this->SetScaleNoTrace(px, py, pz);
  if ( this->GetPVSource()->GetPVRenderView() )
    {
    this->GetPVSource()->GetPVRenderView()->EventuallyRender();
    }

}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetTranslate(float px, float py, float pz)
{
  this->GetPositionFromGUI();
  if ( !( px == this->PositionGUI[0] && 
      py == this->PositionGUI[1] && 
      pz == this->PositionGUI[2] ) )
    {
    this->TranslateThumbWheel[0]->SetValue(px);
    this->TranslateThumbWheel[1]->SetValue(py);
    this->TranslateThumbWheel[2]->SetValue(pz);
    }
  this->SetTranslateNoTrace(px, py, pz);
  if ( this->GetPVSource()->GetPVRenderView() )
    {
    this->GetPVSource()->GetPVRenderView()->EventuallyRender();
    }

  this->AddTraceEntry("$kw(%s) SetTranslate %f %f %f",
                      this->GetTclName(), px, py, pz);  
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::UpdateBox(int update)
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkTransform* trans = this->BoxTransform;
  trans->Identity();
  if ( update || !this->Initialized )
    {
    this->GetPositionFromGUI();
    this->GetRotationFromGUI();
    this->GetScaleFromGUI();
    this->Initialized = 1;
    }
  trans->Translate(this->PositionGUI);
  trans->RotateZ(this->RotationGUI[2]);
  trans->RotateX(this->RotationGUI[0]);
  trans->RotateY(this->RotationGUI[1]);
  trans->Scale(this->ScaleGUI);
  vtkMatrix4x4* mat = trans->GetMatrix();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  pm->GetStream() << vtkClientServerStream::Invoke << this->BoxTransformID
                  << "Identity" << vtkClientServerStream::End
                  << vtkClientServerStream::Invoke << this->BoxMatrixID
                  << "DeepCopy" 
                  << vtkClientServerStream::InsertArray(&mat->Element[0][0], 16)
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke << this->BoxTransformID
                  << "SetMatrix" << this->BoxMatrixID << vtkClientServerStream::End;
  pm->SendStreamToRenderServerClientAndServer();
  pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID
                  << "SetTransform" << this->BoxTransformID << vtkClientServerStream::End;
  pm->SendStreamToRenderServerClientAndServer();
  this->SetValueChanged();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::UpdateFromBox()
{
  vtkBoxWidget* box = static_cast<vtkBoxWidget*>(this->Widget3D);
  box->GetTransform(this->BoxTransform);
  float orientation[3];
  float scale[3];
  float position[3];
  this->BoxTransform->GetOrientation(orientation);
  this->BoxTransform->GetScale(scale);
  this->BoxTransform->GetPosition(position);

  this->GetScaleFromGUI();
  if ( !( scale[0] == this->ScaleGUI[0] && 
      scale[1] == this->ScaleGUI[1] && 
      scale[2] == this->ScaleGUI[2] ) )
    {
    this->ScaleThumbWheel[0]->SetValue(scale[0]);
    this->ScaleThumbWheel[1]->SetValue(scale[1]);
    this->ScaleThumbWheel[2]->SetValue(scale[2]);
    this->SetScaleNoTrace(scale);
    }
  this->GetPositionFromGUI();
  if ( !( position[0] == this->PositionGUI[0] && 
      position[1] == this->PositionGUI[1] && 
      position[2] == this->PositionGUI[2] ) )
    {
    this->TranslateThumbWheel[0]->SetValue(position[0]);
    this->TranslateThumbWheel[1]->SetValue(position[1]);
    this->TranslateThumbWheel[2]->SetValue(position[2]);
    this->SetTranslateNoTrace(position);
    }

  this->GetRotationFromGUI();
  if ( !( orientation[0] == this->RotationGUI[0] && 
      orientation[1] == this->RotationGUI[1] && 
      orientation[2] == this->RotationGUI[2] ) )
    {
    if ( orientation[0] < 0 ) { orientation[0] += 360; }
    if ( orientation[1] < 0 ) { orientation[1] += 360; }
    if ( orientation[2] < 0 ) { orientation[2] += 360; }
    this->OrientationScale[0]->SetValue(orientation[0]);
    this->OrientationScale[1]->SetValue(orientation[1]);
    this->OrientationScale[2]->SetValue(orientation[2]);
    this->SetOrientationNoTrace(orientation);
    }
}

//----------------------------------------------------------------------------
float* vtkPVBoxWidget::GetPositionFromGUI()
{
  this->PositionGUI[0] = this->TranslateThumbWheel[0]->GetValue();
  this->PositionGUI[1] = this->TranslateThumbWheel[1]->GetValue();
  this->PositionGUI[2] = this->TranslateThumbWheel[2]->GetValue();
  return this->PositionGUI;
}

//----------------------------------------------------------------------------
float* vtkPVBoxWidget::GetRotationFromGUI()
{
  this->RotationGUI[0] = this->OrientationScale[0]->GetValue();
  this->RotationGUI[1] = this->OrientationScale[1]->GetValue();
  this->RotationGUI[2] = this->OrientationScale[2]->GetValue();
  return this->RotationGUI;
}

//----------------------------------------------------------------------------
float* vtkPVBoxWidget::GetScaleFromGUI()
{
  this->ScaleGUI[0] = this->ScaleThumbWheel[0]->GetValue();
  this->ScaleGUI[1] = this->ScaleThumbWheel[1]->GetValue();
  this->ScaleGUI[2] = this->ScaleThumbWheel[2]->GetValue();
  return this->ScaleGUI;
}

//----------------------------------------------------------------------------
vtkBoxWidget* vtkPVBoxWidget::GetBoxWidget()
{
  return static_cast<vtkBoxWidget*>(this->Widget3D);
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::ExecuteEvent(vtkObject* wdg, unsigned long l, void* p)
{
  this->UpdateFromBox();
  this->Superclass::ExecuteEvent(wdg, l, p);
}

//----------------------------------------------------------------------------
int vtkPVBoxWidget::ReadXMLAttributes(vtkPVXMLElement* element,
  vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }  
  return 1;
}

//----------------------------------------------------------------------------
vtkClientServerID vtkPVBoxWidget::GetObjectByName(const char* name)
{
  if(!strcmp(name, "Box"))
    {
    return this->BoxID;
    }
  if(!strcmp(name, "BoxTransform"))
    {
    return this->BoxTransformID;
    }
  if(!strcmp(name, "BoxMatrix"))
    {
    return this->BoxMatrixID;
    }
  vtkClientServerID id = {0};
  vtkErrorMacro("GetObjectByName called with invalid object name: " << name);
  return id;
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();


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
