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

#include "vtkRMBoxWidget.h"
#include "vtkKWEvent.h"

#include "vtkDoubleArray.h"
#include "vtkPoints.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"

vtkStandardNewMacro(vtkPVBoxWidget);
vtkCxxRevisionMacro(vtkPVBoxWidget, "1.36");

int vtkPVBoxWidgetCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVBoxWidget::vtkPVBoxWidget()
{
  this->CommandFunction = vtkPVBoxWidgetCommand;

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

  this->RM3DWidget = vtkRMBoxWidget::New();
  this->Initialized = 0;
  
  this->BoxProxy = 0;
  this->BoxTransformProxy = 0;
  this->BoxMatrixProxy = 0;
  this->BoxPointsProxy = 0;
  this->BoxNormalsProxy = 0;
  
  this->BoxProxyName = 0;
  this->BoxTransformProxyName = 0;
  this->BoxMatrixProxyName = 0;
  this->BoxPointsProxyName = 0;
  this->BoxNormalsProxyName = 0;
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
  this->RM3DWidget->Delete();
  
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
  
  if (this->BoxMatrixProxyName)
    {
    vtkSMObject::GetProxyManager()->UnRegisterProxy("matrices",
                                                    this->BoxMatrixProxyName);
    }
  this->SetBoxMatrixProxyName(0);
  if (this->BoxMatrixProxy)
    {
    this->BoxMatrixProxy->Delete();
    this->BoxMatrixProxy = 0;
    }
  
  if (this->BoxPointsProxyName)
    {
    vtkSMObject::GetProxyManager()->UnRegisterProxy("points",
                                                    this->BoxPointsProxyName);
    }
  this->SetBoxPointsProxyName(0);
  if (this->BoxPointsProxy)
    {
    this->BoxPointsProxy->Delete();
    this->BoxPointsProxy = 0;
    }
  
  if (this->BoxNormalsProxyName)
    {
    vtkSMObject::GetProxyManager()->UnRegisterProxy("data_arrays",
                                                    this->BoxNormalsProxyName);
    }
  this->SetBoxNormalsProxyName(0);
  if (this->BoxNormalsProxy)
    {
    this->BoxNormalsProxy->Delete();
    this->BoxNormalsProxy = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::ResetInternal()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }
  static_cast<vtkRMBoxWidget*>(this->RM3DWidget)->ResetInternal();
  this->Superclass::ResetInternal();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::ActualPlaceWidget()
{
  this->Superclass::ActualPlaceWidget();
  double bds[6];
  this->PVSource->GetPVInput(0)->GetDataInformation()->GetBounds(bds);
  this->RM3DWidget->PlaceWidget(bds);  
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::AcceptInternal(vtkClientServerID sourceID)  
{
  this->PlaceWidget(); 
  if ( ! this->ModifiedFlag)
    {
    return;
    }
  static_cast<vtkRMBoxWidget*>(this->RM3DWidget)->UpdateVTKObject();
  
  vtkBoxWidget *bw = vtkBoxWidget::SafeDownCast(this->Widget3D);
  if (bw)
    {
    vtkSMDoubleVectorProperty *dcProp =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->BoxMatrixProxy->GetProperty("DeepCopy"));
    vtkTransform *t = vtkTransform::New();
    bw->GetTransform(t);
    vtkMatrix4x4 *mat = t->GetMatrix();
    if (dcProp)
      {
      dcProp->SetElement(0, mat->Element[0][0]);
      dcProp->SetElement(1, mat->Element[0][1]);
      dcProp->SetElement(2, mat->Element[0][2]);
      dcProp->SetElement(3, mat->Element[0][3]);
      dcProp->SetElement(4, mat->Element[1][0]);
      dcProp->SetElement(5, mat->Element[1][1]);
      dcProp->SetElement(6, mat->Element[1][2]);
      dcProp->SetElement(7, mat->Element[1][3]);
      dcProp->SetElement(8, mat->Element[2][0]);
      dcProp->SetElement(9, mat->Element[2][1]);
      dcProp->SetElement(10, mat->Element[2][2]);
      dcProp->SetElement(11, mat->Element[2][3]);
      dcProp->SetElement(12, mat->Element[3][0]);
      dcProp->SetElement(13, mat->Element[3][1]);
      dcProp->SetElement(14, mat->Element[3][2]);
      dcProp->SetElement(15, mat->Element[3][3]);
      }

    vtkSMDoubleVectorProperty *matProp =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->BoxTransformProxy->GetProperty("Matrix"));
    if (matProp)
      {
      matProp->SetElement(0, mat->Element[0][0]);
      matProp->SetElement(1, mat->Element[0][1]);
      matProp->SetElement(2, mat->Element[0][2]);
      matProp->SetElement(3, mat->Element[0][3]);
      matProp->SetElement(4, mat->Element[1][0]);
      matProp->SetElement(5, mat->Element[1][1]);
      matProp->SetElement(6, mat->Element[1][2]);
      matProp->SetElement(7, mat->Element[1][3]);
      matProp->SetElement(8, mat->Element[2][0]);
      matProp->SetElement(9, mat->Element[2][1]);
      matProp->SetElement(10, mat->Element[2][2]);
      matProp->SetElement(11, mat->Element[2][3]);
      matProp->SetElement(12, mat->Element[3][0]);
      matProp->SetElement(13, mat->Element[3][1]);
      matProp->SetElement(14, mat->Element[3][2]);
      matProp->SetElement(15, mat->Element[3][3]);
      }
    
    vtkPlanes *p = vtkPlanes::New();
    bw->GetPlanes(p);
    vtkSMProxyProperty *ptsProp = vtkSMProxyProperty::SafeDownCast(
      this->BoxProxy->GetProperty("Points"));
    vtkSMProxyProperty *nProp = vtkSMProxyProperty::SafeDownCast(
      this->BoxProxy->GetProperty("Normals"));
    int i;
    vtkSMDoubleVectorProperty *dvpPt = vtkSMDoubleVectorProperty::SafeDownCast(
      this->BoxPointsProxy->GetProperty("Points"));
    vtkSMDoubleVectorProperty *dvpN = vtkSMDoubleVectorProperty::SafeDownCast(
      this->BoxNormalsProxy->GetProperty("Values"));
    vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->BoxNormalsProxy->GetProperty("Components"));
    if (dvpPt)
      {
      vtkPoints *pts = p->GetPoints();
      double *pt;
      for (i = 0; i < 6; i++)
        {
        pt = pts->GetPoint(i);
        dvpPt->SetElement(3*i, pt[0]);
        dvpPt->SetElement(3*i+1, pt[1]);
        dvpPt->SetElement(3*i+2, pt[2]);
        }
      }
    if (dvpN)
      {
      vtkDoubleArray *normals = vtkDoubleArray::SafeDownCast(p->GetNormals());
      for (i = 0; i < 18; i++)
        {
        dvpN->SetElement(i, normals->GetValue(i));
        }
      }
    if (ivp)
      {
      ivp->SetElement(0, 3);
      }
    if (ptsProp)
      {
      ptsProp->RemoveAllProxies();
      ptsProp->AddProxy(this->BoxPointsProxy);
      }
    if (nProp)
      {
      nProp->RemoveAllProxies();
      nProp->AddProxy(this->BoxNormalsProxy);
      }
    this->BoxPointsProxy->UpdateVTKObjects();
    this->BoxNormalsProxy->UpdateVTKObjects();
    this->BoxProxy->UpdateVTKObjects();
    this->BoxTransformProxy->UpdateVTKObjects();
    this->BoxMatrixProxy->UpdateVTKObjects();
    t->Delete();
    p->Delete();
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
  vtkTransform* trans = static_cast<vtkRMBoxWidget*>(this->RM3DWidget)->GetBoxTransform();
  trans->Identity();
  trans->Translate(this->GetPositionFromGUI());
  this->GetRotationFromGUI();
  trans->RotateZ(this->RotationGUI[2]);
  trans->RotateX(this->RotationGUI[0]);
  trans->RotateY(this->RotationGUI[1]);
  trans->Scale(this->GetScaleFromGUI());
  vtkMatrix4x4* mat = trans->GetMatrix();
  
  vtkBoxWidget *bw = vtkBoxWidget::SafeDownCast(this->Widget3D);
  vtkPlanes *p = vtkPlanes::New();
  bw->GetPlanes(p);
  vtkPoints *pts = p->GetPoints();
  vtkDoubleArray *normals = vtkDoubleArray::SafeDownCast(p->GetNormals());
  
  vtkClientServerID boxMatrixID = this->BoxMatrixProxy->GetID(0);

  *file << endl;
  *file << "set pvTemp" << boxMatrixID.ID
        << " [$proxyManager NewProxy matrices Matrix4x4]"
        << endl;
  *file << "  $proxyManager RegisterProxy matrices pvTemp" << boxMatrixID.ID
        << " $pvTemp" << boxMatrixID.ID << endl;
  *file << "  $pvTemp" << boxMatrixID.ID << " UnRegister {}" << endl;

  for(int i=0; i<16; i++)
    {
    *file << "  [$pvTemp" << boxMatrixID.ID
          << " GetProperty DeepCopy] SetElement " << i
          << " " << *(&mat->Element[0][0] + i)
          << endl;
    }
  *file << "  $pvTemp" << boxMatrixID.ID
        << " UpdateVTKObjects" << endl;

  *file << endl;
  vtkClientServerID boxTransformID = this->BoxTransformProxy->GetID(0);

  *file << "set pvTemp" << boxTransformID.ID
        << " [$proxyManager NewProxy transforms Transform]"
        << endl;
  *file << "$proxyManager RegisterProxy transforms pvTemp" << boxTransformID.ID
        << " $pvTemp" << boxTransformID.ID << endl;
  *file << "  $pvTemp" << boxTransformID.ID << " UnRegister {}" << endl;
  *file << "  [$pvTemp" << boxTransformID.ID
        << " GetProperty MatrixProxy] AddProxy $pvTemp" << boxMatrixID.ID
        << endl;
  *file << "  $pvTemp" << boxTransformID.ID
        << " UpdateVTKObjects"  << endl;
  *file << endl;

  vtkClientServerID boxPointsID = this->BoxPointsProxy->GetID(0);
  *file << "set pvTemp" << boxPointsID.ID
        << " [$proxyManager NewProxy points Points]" << endl;
  *file << "$proxyManager RegisterProxy points pvTemp" << boxPointsID.ID
        << " $pvTemp" << boxPointsID.ID << endl;
  *file << "  $pvTemp" << boxPointsID.ID << " UnRegister {}" << endl;
  double *pt;
  for (i = 0; i < 6; i++)
    {
    pt = pts->GetPoint(i);
    *file << "  [$pvTemp" << boxPointsID.ID
          << " GetProperty Points] SetElement " << 3*i << " " << pt[0] << endl;
    *file << "  [$pvTemp" << boxPointsID.ID
          << " GetProperty Points] SetElement " << 3*i+1 << " " << pt[1]
          << endl;
    *file << "  [$pvTemp" << boxPointsID.ID
          << " GetProperty Points] SetElement " << 3*i+2 << " " << pt[2]
          << endl;
    }
  *file << "  $pvTemp" << boxPointsID.ID << " UpdateVTKObjects" << endl;
  *file << endl;
  
  vtkClientServerID boxNormalsID = this->BoxNormalsProxy->GetID(0);
  *file << "set pvTemp" << boxNormalsID.ID
        << " [$proxyManager NewProxy data_arrays DoubleArray]" << endl;
  *file << "$proxyManager RegisterProxy data_arrays pvTemp" << boxNormalsID.ID
        << " $pvTemp" << boxNormalsID.ID << endl;
  *file << "  $pvTemp" << boxNormalsID.ID << " UnRegister {}" << endl;
  *file << "  [$pvTemp" << boxNormalsID.ID
        << " GetProperty Components] SetElement 0 3" << endl;
  for (i = 0; i < 18; i++)
    {
    *file << "  [$pvTemp" << boxNormalsID.ID
          << " GetProperty Values] SetElement " << i << " "
          << normals->GetValue(i) << endl;
    }
  *file << "  $pvTemp" << boxNormalsID.ID << " UpdateVTKObjects" << endl;
  *file << endl;
  
  vtkClientServerID boxID = this->BoxProxy->GetID(0);
  *file << "set pvTemp" << boxID.ID
        << " [$proxyManager NewProxy implicit_functions Planes]" << endl;
  *file << "$proxyManager RegisterProxy implicit_functions pvTemp" << boxID.ID
        << " $pvTemp" << boxID.ID << endl;
  *file << "  $pvTemp" << boxID.ID << " UnRegister {}" << endl;
  *file << "  [$pvTemp" << boxID.ID << " GetProperty Points] AddProxy $pvTemp"
        << boxPointsID.ID << endl;
  *file << "  [$pvTemp" << boxID.ID << " GetProperty Normals] AddProxy $pvTemp"
        << boxNormalsID.ID << endl;
  *file << "  $pvTemp" << boxID.ID << " UpdateVTKObjects" << endl;
  *file << endl;
  
  p->Delete();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
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

  static int instanceCount = 0;
  vtkSMProxyManager *pm = vtkSMObject::GetProxyManager();
  this->BoxProxy = pm->NewProxy("implicit_functions", "Planes");
  ostrstream str1;
  str1 << "Box" << instanceCount << ends;
  this->SetBoxProxyName(str1.str());
  pm->RegisterProxy("implicit_functions", this->BoxProxyName, this->BoxProxy);
  this->BoxProxy->CreateVTKObjects(1);
  str1.rdbuf()->freeze(0);
  
  this->BoxTransformProxy = pm->NewProxy("transforms", "Transform");
  ostrstream str2;
  str2 << "BoxTransform" << instanceCount << ends;
  this->SetBoxTransformProxyName(str2.str());
  pm->RegisterProxy("transforms", this->BoxTransformProxyName,
                    this->BoxTransformProxy);
  this->BoxTransformProxy->CreateVTKObjects(1);
  str2.rdbuf()->freeze(0);
  
  this->BoxMatrixProxy = pm->NewProxy("matrices", "Matrix4x4");
  ostrstream str3;
  str3 << "BoxMatrix" << instanceCount << ends;
  this->SetBoxMatrixProxyName(str3.str());
  pm->RegisterProxy("matrices", this->BoxMatrixProxyName,
                    this->BoxMatrixProxy);
  this->BoxMatrixProxy->CreateVTKObjects(1);
  str3.rdbuf()->freeze(0);
  
  this->BoxPointsProxy = pm->NewProxy("points", "Points");
  ostrstream str4;
  str4 << "BoxPoints" << instanceCount << ends;
  this->SetBoxPointsProxyName(str4.str());
  pm->RegisterProxy("points", this->BoxPointsProxyName, this->BoxPointsProxy);
  this->BoxPointsProxy->CreateVTKObjects(1);
  str4.rdbuf()->freeze(0);
  
  this->BoxNormalsProxy = pm->NewProxy("data_arrays", "DoubleArray");
  ostrstream str5;
  str5 << "BoxNormals" << instanceCount << ends;
  this->SetBoxNormalsProxyName(str5.str());
  pm->RegisterProxy("data_arrays", this->BoxNormalsProxyName,
                    this->BoxNormalsProxy);
  this->BoxNormalsProxy->CreateVTKObjects(1);
  str5.rdbuf()->freeze(0);
  
  instanceCount++;
}
//----------------------------------------------------------------------------
void vtkPVBoxWidget::EnableCallbacks()
{
  int cc;
  for(cc=0 ; cc < 3 ; cc++)
    {
    this->TranslateThumbWheel[cc]->SetCommand(this, "TranslateCallback");
    this->TranslateThumbWheel[cc]->SetEndCommand(this, "TranslateEndCallback");
    this->TranslateThumbWheel[cc]->SetEntryCommand(this,"TranslateEndCallback");

    this->ScaleThumbWheel[cc]->SetCommand(this, "ScaleCallback");
    this->ScaleThumbWheel[cc]->SetEndCommand(this, "ScaleEndCallback");
    this->ScaleThumbWheel[cc]->SetEntryCommand(this, "ScaleEndCallback");

    this->OrientationScale[cc]->SetCommand(this, "OrientationCallback");
    this->OrientationScale[cc]->SetEndCommand(this, 
                                              "OrientationEndCallback");
    this->OrientationScale[cc]->SetEntryCommand(this, 
                                                "OrientationEndCallback");
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
  static_cast<vtkRMBoxWidget*>(this->RM3DWidget)->SetScaleNoEvent(
    this->ScaleGUI[0],this->ScaleGUI[1],this->ScaleGUI[2]);
  this->SetValueChanged();

  if ( this->GetPVSource()->GetPVRenderView() )
    {
    this->GetPVSource()->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::TranslateCallback()
{
  this->GetPositionFromGUI();
  static_cast<vtkRMBoxWidget*>(this->RM3DWidget)->SetPositionNoEvent(
    this->PositionGUI[0], this->PositionGUI[1], this->PositionGUI[2]);
  this->SetValueChanged();
  if ( this->GetPVSource()->GetPVRenderView() )
    {
    this->GetPVSource()->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::OrientationCallback()
{
  this->GetRotationFromGUI();
  static_cast<vtkRMBoxWidget*>(this->RM3DWidget)->SetRotationNoEvent(
    this->RotationGUI[0],this->RotationGUI[1],this->RotationGUI[2]);
  this->SetValueChanged();
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
void vtkPVBoxWidget::SetOrientationInternal(double x, double y, double z)
{
  static_cast<vtkRMBoxWidget*>(this->RM3DWidget)->SetRotation(x,y,z);
  this->SetValueChanged();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetTranslateInternal(double x, double y, double z)
{
  static_cast<vtkRMBoxWidget*>(this->RM3DWidget)->SetPosition(x,y,z);
  this->SetValueChanged();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetScaleInternal(double x, double y, double z)
{
  static_cast<vtkRMBoxWidget*>(this->RM3DWidget)->SetScale(x,y,z);
  this->SetValueChanged();
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetOrientation(double px, double py, double pz)
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
    this->SetOrientationInternal(px, py, pz);
    }
  this->AddTraceEntry("$kw(%s) SetOrientation %f %f %f",
                      this->GetTclName(), px, py, pz);  
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetScale(double px, double py, double pz)
{
  this->GetScaleFromGUI();
  if ( !( px == this->ScaleGUI[0] && 
      py == this->ScaleGUI[1] && 
      pz == this->ScaleGUI[2] ) )
    {
    this->SetScaleInternal(px, py, pz);
    }
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::SetTranslate(double px, double py, double pz)
{
  this->GetPositionFromGUI();
  if ( !( px == this->PositionGUI[0] && 
      py == this->PositionGUI[1] && 
      pz == this->PositionGUI[2] ) )
    {
    this->SetTranslateInternal(px, py, pz);
    }

  this->AddTraceEntry("$kw(%s) SetTranslate %f %f %f",
                      this->GetTclName(), px, py, pz);  
}
//----------------------------------------------------------------------------
void vtkPVBoxWidget::UpdateFromBox()
{
  double orientation[3];
  double scale[3];
  double position[3];

  this->DisableCallbacks();
  static_cast<vtkRMBoxWidget*>(this->RM3DWidget)->GetScale(scale);
  static_cast<vtkRMBoxWidget*>(this->RM3DWidget)->GetRotation(orientation);
  static_cast<vtkRMBoxWidget*>(this->RM3DWidget)->GetPosition(position);
  
  this->GetScaleFromGUI();
  if ( !( scale[0] == this->ScaleGUI[0] && 
      scale[1] == this->ScaleGUI[1] && 
      scale[2] == this->ScaleGUI[2] ) )
    {
    this->ScaleThumbWheel[0]->SetValue(scale[0]);
    this->ScaleThumbWheel[1]->SetValue(scale[1]);
    this->ScaleThumbWheel[2]->SetValue(scale[2]);
    }
  
  this->GetPositionFromGUI();
  if ( !( position[0] == this->PositionGUI[0] && 
      position[1] == this->PositionGUI[1] && 
      position[2] == this->PositionGUI[2] ) )
    {
    this->TranslateThumbWheel[0]->SetValue(position[0]);
    this->TranslateThumbWheel[1]->SetValue(position[1]);
    this->TranslateThumbWheel[2]->SetValue(position[2]);
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
vtkBoxWidget* vtkPVBoxWidget::GetBoxWidget()
{
  return static_cast<vtkBoxWidget*>(this->Widget3D);
}

//----------------------------------------------------------------------------
void vtkPVBoxWidget::ExecuteEvent(vtkObject* wdg, unsigned long l, void* p)
{
  this->UpdateFromBox();

  if(l == vtkKWEvent::WidgetModifiedEvent && wdg == this->RM3DWidget)
    {//case to update the display values from iVars
    if ( this->GetPVSource()->GetPVRenderView() )
      {
      this->GetPVSource()->GetPVRenderView()->EventuallyRender();
      }
    }
  else
    {//case to update iVars 
    this->Superclass::ExecuteEvent(wdg, l, p);
    }
}

//----------------------------------------------------------------------------
int vtkPVBoxWidget::ReadXMLAttributes(vtkPVXMLElement* element,
  vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }  
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
  if (!strcmp(name, "BoxMatrix"))
    {
    return this->BoxMatrixProxy;
    }
  if (!strcmp(name, "BoxPoints"))
    {
    return this->BoxPointsProxy;
    }
  if (!strcmp(name, "BoxNormals"))
    {
    return this->BoxNormalsProxy;
    }
  vtkErrorMacro("GetProxyByName called with invalid proxy name: " << name);
  return 0;
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
