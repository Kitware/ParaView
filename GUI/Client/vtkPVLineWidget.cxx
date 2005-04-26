/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLineWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLineWidget.h"

#include "vtkDataSet.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkSMPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkPVProcessModule.h"
#include "vtkPVTraceHelper.h"

#include "vtkCommand.h"
#include "vtkSMLineWidgetProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkPVLineWidget);
vtkCxxRevisionMacro(vtkPVLineWidget, "1.68");

//----------------------------------------------------------------------------
vtkPVLineWidget::vtkPVLineWidget()
{
  this->Labels[0] = vtkKWLabel::New();
  this->Labels[1] = vtkKWLabel::New();
  this->ResolutionLabel = vtkKWLabel::New();
  for (int i=0; i<3; i++)
    {
    this->CoordinateLabel[i] = vtkKWLabel::New();
    this->Point1[i] = vtkKWEntry::New();
    this->Point2[i] = vtkKWEntry::New();
    }
  this->LengthLabel = vtkKWLabel::New();
  this->LengthValue = vtkKWLabel::New();
  this->ResolutionEntry = vtkKWEntry::New();
  this->Point1Variable = 0;
  this->Point2Variable = 0;
  this->ResolutionVariable = 0;

  this->Point1LabelText = 0;
  this->Point2LabelText = 0;
  this->ResolutionLabelText = 0;

  this->SetPoint1LabelTextName("Point 1");
  this->SetPoint2LabelTextName("Point 2");
  this->SetResolutionLabelTextName("Resolution");

  this->ShowResolution = 1;
  this->SetWidgetProxyXMLName("LineWidgetProxy");
}

//----------------------------------------------------------------------------
vtkPVLineWidget::~vtkPVLineWidget()
{
  this->Labels[0]->Delete();
  this->Labels[1]->Delete();
  for (int i=0; i<3; i++)
    {
    this->Point1[i]->Delete();
    this->Point2[i]->Delete();
    this->CoordinateLabel[i]->Delete();
    }
  this->ResolutionLabel->Delete();
  this->ResolutionEntry->Delete();

  this->LengthLabel->Delete();
  this->LengthLabel = 0;
  this->LengthValue->Delete();
  this->LengthValue = 0;

  this->SetPoint1Variable(0);
  this->SetPoint2Variable(0);
  this->SetResolutionVariable(0);

  this->SetPoint1LabelTextName(0);
  this->SetPoint2LabelTextName(0);
  this->SetResolutionLabelTextName(0);
}


//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint1VariableName(const char* varname)
{
  this->SetPoint1Variable(varname);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint2VariableName(const char* varname)
{
  this->SetPoint2Variable(varname);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetResolutionVariableName(const char* varname)
{
  this->SetResolutionVariable(varname);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint1LabelTextName(const char* varname)
{
  this->SetPoint1LabelText(varname);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint2LabelTextName(const char* varname)
{
  this->SetPoint2LabelText(varname);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetResolutionLabelTextName(const char* varname)
{
  this->SetResolutionLabelText(varname);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint1Internal(double x, double y, double z)
{
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Point1"));
  dvp->SetElements3(x,y,z);
  this->WidgetProxy->UpdateVTKObjects();

  this->Point1[0]->SetValue(x);
  this->Point1[1]->SetValue(y);
  this->Point1[2]->SetValue(z);

  double pos2[3];
  for (int i=0; i<3; i++)
    {
    pos2[i] = this->Point2[i]->GetValueAsFloat();
    }

  double d0, d1, d2;
  d0 = pos2[0]-x;
  d1 = pos2[1]-y;
  d2 = pos2[2]-z;
  this->DisplayLength(sqrt(d0*d0 + d1*d1 + d2*d2));
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint1(double x, double y, double z)
{
  this->SetPoint1Internal(x, y, z);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetPoint1 %f %f %f",
    this->GetTclName(), x, y, z);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::GetPoint1(double pt[3])
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Not created yet.");
    return;
    }
  this->WidgetProxy->UpdateInformation();
  this->GetPoint1Internal(pt);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::GetPoint1Internal(double pt[3])
{
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Point1Info"));
  pt[0] = dvp->GetElement(0);
  pt[1] = dvp->GetElement(1);
  pt[2] = dvp->GetElement(2);
  
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint2Internal(double x, double y, double z)
{
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Point2"));
  dvp->SetElements3(x,y,z);
  this->WidgetProxy->UpdateVTKObjects();

  this->Point2[0]->SetValue(x);
  this->Point2[1]->SetValue(y);
  this->Point2[2]->SetValue(z);

  double pos1[3];
  for (int i=0; i<3; i++)
    {
    pos1[i] = this->Point1[i]->GetValueAsFloat();
    }

  double d0, d1, d2;
  d0 = pos1[0]-x;
  d1 = pos1[1]-y;
  d2 = pos1[2]-z;
  this->DisplayLength(sqrt(d0*d0 + d1*d1 + d2*d2));

}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint2(double x, double y, double z)
{
  this->SetPoint2Internal(x, y, z);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetPoint2 %f %f %f",
    this->GetTclName(), x, y, z);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::GetPoint2(double pt[3])
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Not created yet.");
    return;
    }
  this->WidgetProxy->UpdateInformation();
  this->GetPoint2Internal(pt);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::GetPoint2Internal(double pt[3])
{
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Point2Info"));
  pt[0] = dvp->GetElement(0);
  pt[1] = dvp->GetElement(1);
  pt[2] = dvp->GetElement(2);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint1()
{
  if ( !this->ValueChanged )
    {
    return;
    }
  double pos[3];
  int i;
  for (i=0; i<3; i++)
    {
    pos[i] = this->Point1[i]->GetValueAsFloat();
    }
  this->SetPoint1(pos[0], pos[1], pos[2]);
  this->Render();
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint2()
{
  if ( !this->ValueChanged )
    {
    return;
    }
  double pos[3];
  int i;
  for (i=0; i<3; i++)
    {
    pos[i] = this->Point2[i]->GetValueAsFloat();
    }
  this->SetPoint2(pos[0], pos[1], pos[2]);
  this->Render();
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetResolutionInternal(int res)
{
  if(!this->IsCreated())
    {
    vtkErrorMacro("Not created yet");
    return;
    }
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Resolution"));
  ivp->SetElements1(res);
  this->WidgetProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetResolution(int res)
{
  this->SetResolutionInternal(res);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetResolution %d", this->GetTclName(), res);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
int vtkPVLineWidget::GetResolution()
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Not created yet.");
    return 0;
    }

  this->WidgetProxy->UpdateInformation();
  return this->GetResolutionInternal();
}

//----------------------------------------------------------------------------
int vtkPVLineWidget::GetResolutionInternal()
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Resolution"));
  return ivp->GetElement(0);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetResolution()
{
  if ( !this->ValueChanged )
    {
    return;
    }
  int res = this->ResolutionEntry->GetValueAsInt();
  this->SetResolution(res);
  this->Render();
  this->ValueChanged = 0; 
}

//---------------------------------------------------------------------------
void vtkPVLineWidget::Trace(ofstream *file)
{
  if ( ! this->GetTraceHelper()->Initialize(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetPoint1 "
        << this->Point1[0]->GetValue() << " "
        << this->Point1[1]->GetValue() << " "
        << this->Point1[2]->GetValue() << endl;
  *file << "$kw(" << this->GetTclName() << ") SetPoint2 "
        << this->Point2[0]->GetValue() << " "
        << this->Point2[1]->GetValue() << " "
        << this->Point2[2]->GetValue() << endl;
  *file << "$kw(" << this->GetTclName() << ") SetResolution "
        << this->ResolutionEntry->GetValue() << endl;
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::Accept()
{
  int modFlag = this->GetModifiedFlag();
  double pt1[3],pt2[3];
  int res;
  const char* variablename;
  
  this->WidgetProxy->UpdateInformation();
  this->GetPoint1Internal(pt1);
  this->GetPoint2Internal(pt2);
  res = this->GetResolutionInternal();

  vtkSMSourceProxy* sproxy = this->GetPVSource()->GetProxy();
  variablename = (this->Point1Variable)?this->Point1Variable : "Point1";
  vtkSMDoubleVectorProperty* sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    sproxy->GetProperty(variablename));
  if (sdvp)
    {
    sdvp->SetElements3(pt1[0],pt1[1],pt1[2]);
    }
  else
    {
    vtkErrorMacro("Could not find property "<<variablename<<" for widget: "<< sproxy->GetVTKClassName());
    }
  variablename = (this->Point2Variable)? this->Point2Variable : "Point2";
  sdvp = vtkSMDoubleVectorProperty::SafeDownCast(sproxy->GetProperty(variablename));
  if(sdvp)
    {
    sdvp->SetElements3(pt2[0],pt2[1],pt2[2]);
    }
  else
    {
    vtkErrorMacro("Could not find property "<<variablename<<" for widget: "<< sproxy->GetVTKClassName());
    }

  if (this->ResolutionVariable)
    {
    vtkSMIntVectorProperty* sivp = vtkSMIntVectorProperty::SafeDownCast(
      sproxy->GetProperty(this->ResolutionVariable));
    if (sivp)
      {
      sivp->SetElements1(res);
      }
    else
      {
      vtkErrorMacro("Could not find property "<<this->ResolutionVariable
        <<" for widget: "<< sproxy->GetVTKClassName());
      }
    }
  sproxy->UpdateVTKObjects();
  sproxy->UpdatePipeline();
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

}

//----------------------------------------------------------------------------
void vtkPVLineWidget::ActualPlaceWidget()
{
  double bds[6];
  double x, y, z;

  if ( this->PVSource->GetPVInput(0) )
    {
    this->PVSource->GetPVInput(0)->GetDataInformation()->GetBounds(bds);

    x = (bds[0]+bds[1])/2; 
    y = bds[2]; 
    z = (bds[4]+bds[5])/2;
    this->SetPoint1(x, y, z);
    x = (bds[0]+bds[1])/2; 
    y = bds[3]; 
    z = (bds[4]+bds[5])/2;
    this->SetPoint2(x, y, z);
    this->PlaceWidget(bds);
    }
  else
    {
    this->SetPoint1(-0.5,0,0);
    this->SetPoint2(0.5,0,0);
    }
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::Create(vtkKWApplication* app)
{
  this->Superclass::Create(app);

  // Set up controller properties. Controller properties are set so 
  // that in the SM State, we can have a mapping from the widget to the 
  // controlled implicit function.
  if (this->Point1Variable)
    {
    vtkSMProperty* p = 
      this->GetPVSource()->GetProxy()->GetProperty(this->Point1Variable);
    p->SetControllerProxy(this->WidgetProxy);
    p->SetControllerProperty(this->WidgetProxy->GetProperty("Point1"));
    }

  if (this->Point2Variable)
    {
    vtkSMProperty* p = 
      this->GetPVSource()->GetProxy()->GetProperty(this->Point2Variable);
    p->SetControllerProxy(this->WidgetProxy);
    p->SetControllerProperty(this->WidgetProxy->GetProperty("Point2"));
    }
  // There is no need to set up control dependency for resolution.
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SaveInBatchScript(ofstream *file)
{
  // First create the Line Widget proxy.
  this->WidgetProxy->SaveInBatchScript(file);

  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);

  // Point1
  vtkSMSourceProxy* sproxy = this->GetPVSource()->GetProxy();
  const char *variablename = (this->Point1Variable)? this->Point1Variable : "Point1";
  vtkSMDoubleVectorProperty* sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    sproxy->GetProperty(variablename));
  if(sdvp)
    {  
    *file << "  " << "[$pvTemp" << sourceID << " GetProperty " 
      << variablename << "] SetElements3 "
      << sdvp->GetElement(0) << " "
      << sdvp->GetElement(1) << " "
      << sdvp->GetElement(2) << endl;
    *file << "  [$pvTemp" << sourceID << " GetProperty "
      << variablename << "] SetControllerProxy $pvTemp" << this->WidgetProxy->GetID(0)
      << endl;
    *file << "  [$pvTemp" << sourceID << " GetProperty "
      << variablename << "] SetControllerProperty [$pvTemp" << this->WidgetProxy->GetID(0)
      << " GetProperty Point1]" << endl;
    }

  // Point2
  variablename = (this->Point2Variable)? this->Point2Variable : "Point2";
  sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    sproxy->GetProperty(variablename));
  if(sdvp)
    {
    *file << "  " << "[$pvTemp" << sourceID << " GetProperty " 
      << variablename << "] SetElements3 "
      << sdvp->GetElement(0) << " "
      << sdvp->GetElement(1) << " "
      << sdvp->GetElement(2) << endl;
    *file << "  [$pvTemp" << sourceID << " GetProperty "
      << variablename << "] SetControllerProxy $pvTemp" << this->WidgetProxy->GetID(0)
      << endl;
    *file << "  [$pvTemp" << sourceID << " GetProperty "
      << variablename << "] SetControllerProperty [$pvTemp" << this->WidgetProxy->GetID(0)
      << " GetProperty Point2]" << endl;   
    }

  // Resolution
  if (this->ResolutionVariable)
    {
    vtkSMIntVectorProperty* sivp = vtkSMIntVectorProperty::SafeDownCast(
      sproxy->GetProperty(this->ResolutionVariable));
    if(sivp)
      {
      *file << "  " << "[$pvTemp" << sourceID << " GetProperty " 
        << variablename << "] SetElements1 "
        << sivp->GetElement(0) << endl;
      *file << "  [$pvTemp" << sourceID << " GetProperty "
        << variablename << "] SetControllerProxy $pvTemp" << this->WidgetProxy->GetID(0)
        << endl;
      *file << "  [$pvTemp" << sourceID << " GetProperty "
        << variablename << "] SetControllerProperty [$pvTemp" << this->WidgetProxy->GetID(0)
        << " GetProperty Resolution]" << endl;
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::Initialize()
{
  this->PlaceWidget();

  this->Accept();
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::ResetInternal()
{

  if (!this->ModifiedFlag)
    {
    return;
    }
  double pt1[3],pt2[3];
  int res;
  const char* variablename;

  vtkSMSourceProxy* sproxy = this->GetPVSource()->GetProxy();
  variablename = (this->Point1Variable)? this->Point1Variable : "Point1";
  vtkSMDoubleVectorProperty* sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    sproxy->GetProperty(variablename));
  if(sdvp)
    {
    pt1[0] = sdvp->GetElement(0);
    pt1[1] = sdvp->GetElement(1);
    pt1[2] = sdvp->GetElement(2);
    this->SetPoint1Internal(pt1[0],pt1[1],pt1[2]);
    } 
  else
    {
    vtkErrorMacro("Could not find property "<<variablename<<" for widget: "<< sproxy->GetVTKClassName());
    }

  variablename = (this->Point2Variable)? this->Point2Variable : "Point2";
  sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    sproxy->GetProperty(variablename));
  if(sdvp)
    {
    pt2[0] = sdvp->GetElement(0);
    pt2[1] = sdvp->GetElement(1);
    pt2[2] = sdvp->GetElement(2);
    this->SetPoint2Internal(pt2[0],pt2[1],pt2[2]);
    }
  else
    {
    vtkErrorMacro("Could not find property "<<variablename<<" for widget: "<< sproxy->GetVTKClassName());
    }

  if (this->ResolutionVariable)
    {
    vtkSMIntVectorProperty* sivp = vtkSMIntVectorProperty::SafeDownCast(
      sproxy->GetProperty(this->ResolutionVariable));
    if(sivp)
      {
      res = sivp->GetElement(0);
      this->SetResolution(res);
      }
    else
      {
      vtkErrorMacro("Could not find property "<<
        this->ResolutionVariable <<" for widget: "<< sproxy->GetVTKClassName());
      }
    }
  this->Superclass::ResetInternal();
}

//----------------------------------------------------------------------------
vtkPVLineWidget* vtkPVLineWidget::ClonePrototype(vtkPVSource* pvSource,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVLineWidget::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::CopyProperties(vtkPVWidget* clone, 
  vtkPVSource* pvSource,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVLineWidget* pvlw = vtkPVLineWidget::SafeDownCast(clone);
  if (pvlw)
    {
    pvlw->SetPoint1VariableName(this->GetPoint1Variable());
    pvlw->SetPoint2VariableName(this->GetPoint2Variable());
    pvlw->SetResolutionVariableName(this->GetResolutionVariable());
    pvlw->SetPoint1LabelTextName(this->GetPoint1LabelText());
    pvlw->SetPoint2LabelTextName(this->GetPoint2LabelText());
    pvlw->SetResolutionLabelTextName(this->GetResolutionLabelText());
    pvlw->SetShowResolution(this->ShowResolution);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVLineWidget.");
    }
}

//----------------------------------------------------------------------------
int vtkPVLineWidget::ReadXMLAttributes(vtkPVXMLElement* element,
  vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }

  const char* point1_variable = element->GetAttribute("point1_variable");
  if(point1_variable)
    {
    this->SetPoint1VariableName(point1_variable);
    }

  const char* point2_variable = element->GetAttribute("point2_variable");
  if(point2_variable)
    {
    this->SetPoint2VariableName(point2_variable);
    }

  const char* resolution_variable = element->GetAttribute("resolution_variable");
  if(resolution_variable)
    {
    this->SetResolutionVariableName(resolution_variable);
    }

  const char* point1_label = element->GetAttribute("point1_label");
  if(point1_label)
    {
    this->SetPoint1LabelTextName(point1_label);
    }

  const char* point2_label = element->GetAttribute("point2_label");
  if(point2_label)
    {
    this->SetPoint2LabelTextName(point2_label);
    }

  const char* resolution_label = element->GetAttribute("resolution_label");
  if(resolution_label)
    {
    this->SetResolutionLabelTextName(resolution_label);
    }

  int showResolution;
  if (element->GetScalarAttribute("show_resolution", &showResolution))
    {
    this->SetShowResolution(showResolution);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::DisplayLength(double len)
{
  char tmp[1024];
  sprintf(tmp, "%.5g", len);
  this->LengthValue->SetText(tmp);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::ExecuteEvent(vtkObject* wdg, unsigned long l, void* p)
{
  if(l == vtkCommand::WidgetModifiedEvent)
    {
    double pos1[3];
    double pos2[3];
    int res;
    this->WidgetProxy->UpdateInformation();
    this->GetPoint1Internal(pos1);
    this->Point1[0]->SetValue(pos1[0]);
    this->Point1[1]->SetValue(pos1[1]);
    this->Point1[2]->SetValue(pos1[2]);

    this->GetPoint2Internal(pos2);
    this->Point2[0]->SetValue(pos2[0]);
    this->Point2[1]->SetValue(pos2[1]);
    this->Point2[2]->SetValue(pos2[2]);

    // Display the length of the line.
    double d0, d1, d2;
    d0 = pos2[0]-pos1[0];
    d1 = pos2[1]-pos1[1];
    d2 = pos2[2]-pos1[2];
    this->DisplayLength(sqrt(d0*d0 + d1*d1 + d2*d2));

    res = this->GetResolutionInternal();
    this->ResolutionEntry->SetValue(res);

    }
  this->Superclass::ExecuteEvent(wdg, l, p);
}


//----------------------------------------------------------------------------
void vtkPVLineWidget::SetBalloonHelpString(const char *str)
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

  if (this->ResolutionLabel)
    {
    this->ResolutionLabel->SetBalloonHelpString(str);
    }
  if (this->ResolutionEntry)
    {
    this->ResolutionEntry->SetBalloonHelpString(str);
    }
  for (int i=0; i<3; i++)
    {
    if (this->CoordinateLabel[i])
      {
      this->CoordinateLabel[i]->SetBalloonHelpString(str);
      }
    if (this->Point1[i])
      {
      this->Point1[i]->SetBalloonHelpString(str);
      }
    if (this->Point2[i])
      {
      this->Point2[i]->SetBalloonHelpString(str);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::ChildCreate(vtkPVApplication* pvApp)
{
  if ((this->GetTraceHelper()->GetObjectNameState() == 
       vtkPVTraceHelper::ObjectNameStateUninitialized ||
      this->GetTraceHelper()->GetObjectNameState() == 
       vtkPVTraceHelper::ObjectNameStateDefault) )
    {
    this->GetTraceHelper()->SetObjectName("Line");
    this->GetTraceHelper()->SetObjectNameState(
      vtkPVTraceHelper::ObjectNameStateSelfInitialized);
    }


  this->SetFrameLabel("Line Widget");
  this->Labels[0]->SetParent(this->Frame->GetFrame());
  this->Labels[0]->Create(pvApp, "");
  this->Labels[0]->SetText(this->GetPoint1LabelText());
  this->Labels[1]->SetParent(this->Frame->GetFrame());
  this->Labels[1]->Create(pvApp, "");
  this->Labels[1]->SetText(this->GetPoint2LabelText());
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
    this->Point1[i]->SetParent(this->Frame->GetFrame());
    this->Point1[i]->Create(pvApp, "");
    }

  for (i=0; i<3; i++)    
    {
    this->Point2[i]->SetParent(this->Frame->GetFrame());
    this->Point2[i]->Create(pvApp, "");
    }
  this->ResolutionLabel->SetParent(this->Frame->GetFrame());
  this->ResolutionLabel->Create(pvApp, "");
  this->ResolutionLabel->SetText(this->GetResolutionLabelText());
  this->ResolutionEntry->SetParent(this->Frame->GetFrame());
  this->ResolutionEntry->Create(pvApp, "");
  this->ResolutionEntry->SetValue(0);
  this->LengthLabel->SetParent(this->Frame->GetFrame());
  this->LengthLabel->Create(pvApp, "");
  this->LengthLabel->SetText("Length:");
  this->LengthValue->SetParent(this->Frame->GetFrame());
  this->LengthValue->Create(pvApp, "");
  this->LengthValue->SetText("1.0");

  this->Script("grid propagate %s 1",
    this->Frame->GetFrame()->GetWidgetName());

  this->Script("grid x %s %s %s -sticky ew",
    this->CoordinateLabel[0]->GetWidgetName(),
    this->CoordinateLabel[1]->GetWidgetName(),
    this->CoordinateLabel[2]->GetWidgetName());
  this->Script("grid %s %s %s %s -sticky ew",
    this->Labels[0]->GetWidgetName(),
    this->Point1[0]->GetWidgetName(),
    this->Point1[1]->GetWidgetName(),
    this->Point1[2]->GetWidgetName());
  this->Script("grid %s %s %s %s -sticky ew",
    this->Labels[1]->GetWidgetName(),
    this->Point2[0]->GetWidgetName(),
    this->Point2[1]->GetWidgetName(),
    this->Point2[2]->GetWidgetName());
  if (this->ShowResolution)
    {
    this->Script("grid %s %s - - -sticky ew",
      this->ResolutionLabel->GetWidgetName(),
      this->ResolutionEntry->GetWidgetName());
    }
  this->Script("grid %s %s - - -sticky w", 
    this->LengthLabel->GetWidgetName(),
    this->LengthValue->GetWidgetName());

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
      this->Point1[i]->GetWidgetName(),
      this->GetTclName());
    this->Script("bind %s <Key> {%s SetValueChanged}",
      this->Point2[i]->GetWidgetName(),
      this->GetTclName());
    this->Script("bind %s <FocusOut> {%s SetPoint1}",
      this->Point1[i]->GetWidgetName(),
      this->GetTclName());
    this->Script("bind %s <FocusOut> {%s SetPoint2}",
      this->Point2[i]->GetWidgetName(),
      this->GetTclName());
    this->Script("bind %s <KeyPress-Return> {%s SetPoint1}",
      this->Point1[i]->GetWidgetName(),
      this->GetTclName());
    this->Script("bind %s <KeyPress-Return> {%s SetPoint2}",
      this->Point2[i]->GetWidgetName(),
      this->GetTclName());
    }
  this->Script("bind %s <Key> {%s SetValueChanged}",
    this->ResolutionEntry->GetWidgetName(),
    this->GetTclName());
  this->Script("bind %s <FocusOut> {%s SetResolution}",
    this->ResolutionEntry->GetWidgetName(),
    this->GetTclName());
  this->Script("bind %s <KeyPress-Return> {%s SetResolution}",
    this->ResolutionEntry->GetWidgetName(),
    this->GetTclName());

  this->SetResolution(20);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->PropagateEnableState(this->Point1[cc]);
    this->PropagateEnableState(this->Point2[cc]);
    this->PropagateEnableState(this->Labels[cc]);
    this->PropagateEnableState(this->CoordinateLabel[cc]);
    }
  this->PropagateEnableState(this->ResolutionLabel);
  this->PropagateEnableState(this->ResolutionEntry);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Point1Variable: " 
    << ( this->Point1Variable ? this->Point1Variable : "(none)" ) << endl;
  os << indent << "Point1LabelText: " 
    << ( this->Point1LabelText ? this->Point1LabelText : "(none)" ) << endl;
  os << indent << "Point2Variable: " 
    << ( this->Point2Variable ? this->Point2Variable : "(none)" ) << endl;
  os << indent << "Point2LabelText: " 
    << ( this->Point2LabelText ? this->Point2LabelText : "(none)" ) << endl;
  os << indent << "ResolutionVariable: " 
    << ( this->ResolutionVariable ? this->ResolutionVariable : "(none)" ) << endl;
  os << indent << "ResolutionLabelText: " 
    << ( this->ResolutionLabelText ? this->ResolutionLabelText : "(none)" ) 
    << endl;
  os << indent << "ShowResolution: " << this->ShowResolution << endl;
}
