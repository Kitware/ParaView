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
#include "vtkLineWidget.h"
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

#include "vtkKWEvent.h"
#include "vtkSMLineWidgetProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkPVLineWidget);
vtkCxxRevisionMacro(vtkPVLineWidget, "1.54");

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
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint1(double x, double y, double z)
{
  this->SetPoint1Internal(x, y, z);
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
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint2(double x, double y, double z)
{
  this->SetPoint2Internal(x, y, z);
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
  this->ModifiedCallback();
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
  this->ModifiedCallback();
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetResolution(int i)
{
  if(!this->IsCreated())
    {
    vtkErrorMacro("Not created yet");
    return;
    }
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Resolution"));
  ivp->SetElements1(i);
  this->WidgetProxy->UpdateVTKObjects();
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
    this->WidgetProxy->GetProperty("ResolutionInfo"));
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
  this->ModifiedCallback();
  this->ValueChanged = 0; 
}

//---------------------------------------------------------------------------
void vtkPVLineWidget::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
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
    }
  else
    {
    bds[0] = bds[2] = bds[4] = 0.0;
    bds[1] = bds[3] = bds[5] = 1.0;
    }

  if (this->WidgetProxy)
    {
    this->WidgetProxy->PlaceWidget(bds);
    }
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SaveInBatchScript(ofstream *file)
{
  //TODO: this method is incorrect. Why was this method incorrect even
  //before the changes were made? Is this never invoked?
  //I believe it is so, since PVLineWidget is used only(?) while
  //creating a line source, so, it is not saved in batch script.

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
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::ResetInternal()
{
  if (this->SuppressReset)
    {
    return;
    }
  if ( ! this->ModifiedFlag || !this->AcceptCalled)
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
void vtkPVLineWidget::ExecuteEvent(vtkObject* wdg, unsigned long l, void* p)
{
  if(l == vtkKWEvent::WidgetModifiedEvent)
    {
    double pos[3];
    int res;
    this->WidgetProxy->UpdateInformation();
    this->GetPoint1Internal(pos);
    this->Point1[0]->SetValue(pos[0]);
    this->Point1[1]->SetValue(pos[1]);
    this->Point1[2]->SetValue(pos[2]);

    this->GetPoint2Internal(pos);
    this->Point2[0]->SetValue(pos[0]);
    this->Point2[1]->SetValue(pos[1]);
    this->Point2[2]->SetValue(pos[2]);

    res = this->GetResolutionInternal();
    this->ResolutionEntry->SetValue(res);

    this->Render();
    }
  this->Superclass::ExecuteEvent(wdg, l, p);
}


//----------------------------------------------------------------------------
void vtkPVLineWidget::SetBalloonHelpString(const char *str)
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

    this->ResolutionLabel->SetBalloonHelpString(this->BalloonHelpString);
    this->ResolutionEntry->SetBalloonHelpString(this->BalloonHelpString);
    for (int i=0; i<3; i++)
      {
      this->CoordinateLabel[i]->SetBalloonHelpString(this->BalloonHelpString);
      this->Point1[i]->SetBalloonHelpString(this->BalloonHelpString);
      this->Point2[i]->SetBalloonHelpString(this->BalloonHelpString);
      }

    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::ChildCreate(vtkPVApplication* pvApp)
{
  if ((this->TraceNameState == vtkPVWidget::Uninitialized ||
      this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName("Line");
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }

  this->SetFrameLabel("Line Widget");
  this->Labels[0]->SetParent(this->Frame->GetFrame());
  this->Labels[0]->Create(pvApp, "");
  this->Labels[0]->SetLabel(this->GetPoint1LabelText());
  this->Labels[1]->SetParent(this->Frame->GetFrame());
  this->Labels[1]->Create(pvApp, "");
  this->Labels[1]->SetLabel(this->GetPoint2LabelText());
  int i;
  for (i=0; i<3; i++)
    {
    this->CoordinateLabel[i]->SetParent(this->Frame->GetFrame());
    this->CoordinateLabel[i]->Create(pvApp, "");
    char buffer[3];
    sprintf(buffer, "%c", "xyz"[i]);
    this->CoordinateLabel[i]->SetLabel(buffer);
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
  this->ResolutionLabel->SetLabel(this->GetResolutionLabelText());
  this->ResolutionEntry->SetParent(this->Frame->GetFrame());
  this->ResolutionEntry->Create(pvApp, "");
  this->ResolutionEntry->SetValue(0);

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

  this->SetBalloonHelpString(this->BalloonHelpString);
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
