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
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkPVProcessModule.h"

#include "vtkKWEvent.h"
#include "vtkRMLineWidget.h"

vtkStandardNewMacro(vtkPVLineWidget);
vtkCxxRevisionMacro(vtkPVLineWidget, "1.51");

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
  this->RM3DWidget = vtkRMLineWidget::New();
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
  this->RM3DWidget->Delete();

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
  static_cast<vtkRMLineWidget*>(this->RM3DWidget)->SetPoint1(x,y,z);
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
  static_cast<vtkRMLineWidget*>(this->RM3DWidget)->GetPoint1(pt);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint2Internal(double x, double y, double z)
{
  static_cast<vtkRMLineWidget*>(this->RM3DWidget)->SetPoint2(x,y,z);
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
  static_cast<vtkRMLineWidget*>(this->RM3DWidget)->GetPoint2(pt);
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
  static_cast<vtkRMLineWidget*>(this->RM3DWidget)->SetResolution(i);
}

//----------------------------------------------------------------------------
int vtkPVLineWidget::GetResolution()
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Not created yet.");
    return 0;
    }

  return static_cast<vtkRMLineWidget*>(this->RM3DWidget)->GetResolution();
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
void vtkPVLineWidget::AcceptInternal(vtkClientServerID sourceID)
{
  this->UpdateVTKObject(sourceID);
  this->Superclass::AcceptInternal(sourceID);
}


//----------------------------------------------------------------------------
void vtkPVLineWidget::UpdateVTKObject(vtkClientServerID sourceID)
{
  static_cast<vtkRMLineWidget*>(this->RM3DWidget)->UpdateVTKObject(sourceID,
    this->Point1Variable, this->Point2Variable, this->ResolutionVariable);
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
  
  this->RM3DWidget->PlaceWidget(bds);
  this->UpdateVTKObject(this->ObjectID);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SaveInBatchScript(ofstream *file)
{
  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);

  // Point1
  if (this->Point1Variable)
    {  
    *file << "  " << "[$pvTemp" << sourceID << " GetProperty " 
          << this->Point1Variable << "] SetElements3 "
          << this->Point1[0]->GetValueAsFloat() << " "
          << this->Point1[1]->GetValueAsFloat() << " "
          << this->Point1[2]->GetValueAsFloat() << endl;
    }
  // Point2
  if (this->Point2Variable)
    {
    *file << "  " << "[$pvTemp" << sourceID << " GetProperty " 
          << this->Point2Variable << "] SetElements3 "
          << this->Point2[0]->GetValueAsFloat() << " "
          << this->Point2[1]->GetValueAsFloat() << " "
          << this->Point2[2]->GetValueAsFloat() << endl;

    }

  // Resolution
  if (this->ResolutionVariable)
    {
    *file << "  " << "[$pvTemp" << sourceID << " GetProperty " 
          << this->ResolutionVariable << "] SetElements1 "
          << this->ResolutionEntry->GetValueAsFloat() << endl;
    }
}


//----------------------------------------------------------------------------
void vtkPVLineWidget::ResetInternal()
{
  if (this->SuppressReset)
    {
    return;
    }
  if ( ! this->ModifiedFlag)
    {
    return;
    }
  static_cast<vtkRMLineWidget*>(this->RM3DWidget)->ResetInternal();
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
    static_cast<vtkRMLineWidget*>(this->RM3DWidget)->GetPoint1(pos);
    this->Point1[0]->SetValue(pos[0]);
    this->Point1[1]->SetValue(pos[1]);
    this->Point1[2]->SetValue(pos[2]);

    static_cast<vtkRMLineWidget*>(this->RM3DWidget)->GetPoint2(pos);
    this->Point2[0]->SetValue(pos[0]);
    this->Point2[1]->SetValue(pos[1]);
    this->Point2[2]->SetValue(pos[2]);
    
    res = static_cast<vtkRMLineWidget*>(this->RM3DWidget)->GetResolution();
    this->ResolutionEntry->SetValue(res);

    this->Render();
  
    }
  else
    {
    vtkLineWidget* widget = vtkLineWidget::SafeDownCast(wdg);
    if (!widget)
      {
      return;
      }
    double val[3];
    
    widget->GetPoint1(val);
    this->SetPoint1(val[0],val[1],val[2]);
    
    widget->GetPoint2(val);
    this->SetPoint2(val[0],val[1],val[2]);
    
    this->Superclass::ExecuteEvent(wdg, l, p);
    }
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
