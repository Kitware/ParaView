/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVLineWidget.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

vtkStandardNewMacro(vtkPVLineWidget);
vtkCxxRevisionMacro(vtkPVLineWidget, "1.38.2.10");

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

  this->LastAcceptedPoint1[0] = -0.5;
  this->LastAcceptedPoint1[1] = this->LastAcceptedPoint1[2] = 0;
  this->LastAcceptedPoint2[0] = 0.5;
  this->LastAcceptedPoint2[1] = this->LastAcceptedPoint2[2] = 0;
  this->LastAcceptedResolution = 1;
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
void vtkPVLineWidget::SetPoint1Internal(float x, float y, float z)
{
  this->Point1[0]->SetValue(x);
  this->Point1[1]->SetValue(y);
  this->Point1[2]->SetValue(z);
 
  int i;
  float pos[3];
  for (i=0; i<3; i++)
    {
    pos[i] = this->Point1[i]->GetValueAsFloat();
    }
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  pm->GetStream() << vtkClientServerStream::Invoke <<  this->Widget3DID
                  << "SetPoint1" << pos[0] << pos[1] <<  pos[2]
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke <<  this->Widget3DID
                  << "SetAlignToNone" <<  vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
  this->Render();
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint1(float x, float y, float z)
{
  this->SetPoint1Internal(x, y, z);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::GetPoint1(float pt[3])
{
  if (this->Application == NULL)
    {
    vtkErrorMacro("Not created yet.");
    return;
    }
  pt[0] = this->Point1[0]->GetValueAsFloat();
  pt[1] = this->Point1[1]->GetValueAsFloat();
  pt[2] = this->Point1[2]->GetValueAsFloat();
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint2Internal(float x, float y, float z)
{
  this->Point2[0]->SetValue(x);
  this->Point2[1]->SetValue(y);
  this->Point2[2]->SetValue(z);
 
  int i;
  float pos[3];
  for (i=0; i<3; i++)
    {
    pos[i] = this->Point2[i]->GetValueAsFloat();
    }

  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  pm->GetStream() << vtkClientServerStream::Invoke <<  this->Widget3DID
                  << "SetPoint2" << pos[0] << pos[1] <<  pos[2]
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke <<  this->Widget3DID
                  << "SetAlignToNone" <<  vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
  this->Render();
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint2(float x, float y, float z)
{
  this->SetPoint2Internal(x, y, z);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::GetPoint2(float pt[3])
{
  if (this->Application == NULL)
    {
    vtkErrorMacro("Not created yet.");
    return;
    }
  pt[0] = this->Point2[0]->GetValueAsFloat();
  pt[1] = this->Point2[1]->GetValueAsFloat();
  pt[2] = this->Point2[2]->GetValueAsFloat();
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint1()
{
  if ( !this->ValueChanged )
    {
    return;
    }
  float pos[3];
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
  float pos[3];
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
  this->ResolutionEntry->SetValue(i);
  int res = this->ResolutionEntry->GetValueAsInt();


  if ( !this->GetPVApplication() )
    {
    return;
    } 
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  pm->GetStream() << vtkClientServerStream::Invoke <<  this->Widget3DID
                  << "SetResolution" << res
                  << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
  this->Render();
}

//----------------------------------------------------------------------------
int vtkPVLineWidget::GetResolution()
{
  if (this->Application == NULL)
    {
    vtkErrorMacro("Not created yet.");
    return 0;
    }

  return this->ResolutionEntry->GetValueAsInt();
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
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->SetPoint1Internal(this->Point1[0]->GetValueAsFloat(),
                          this->Point1[1]->GetValueAsFloat(),
                          this->Point1[2]->GetValueAsFloat());
  this->SetPoint2Internal(this->Point2[0]->GetValueAsFloat(),
                          this->Point2[1]->GetValueAsFloat(),
                          this->Point2[2]->GetValueAsFloat());

  this->SetLastAcceptedPoint1(this->Point1[0]->GetValueAsFloat(),
                              this->Point1[1]->GetValueAsFloat(),
                              this->Point1[2]->GetValueAsFloat());
  this->SetLastAcceptedPoint2(this->Point2[0]->GetValueAsFloat(),
                              this->Point2[1]->GetValueAsFloat(),
                              this->Point2[2]->GetValueAsFloat());
  this->SetLastAcceptedResolution(this->ResolutionEntry->GetValueAsFloat());
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  char acceptCmd[1024];
  if ( this->Point1Variable && sourceID.ID )    
    {
    sprintf(acceptCmd, "Set%s", this->Point1Variable);
    pm->GetStream() << vtkClientServerStream::Invoke << sourceID
                    << acceptCmd << this->Point1[0]->GetValueAsFloat()
                    << this->Point1[1]->GetValueAsFloat()
                    << this->Point1[2]->GetValueAsFloat()
                    << vtkClientServerStream::End;
    }
  if ( this->Point2Variable && sourceID.ID )
    {
    sprintf(acceptCmd, "Set%s", this->Point2Variable);
    pm->GetStream() << vtkClientServerStream::Invoke << sourceID
                    << acceptCmd << this->Point1[0]->GetValueAsFloat()
                    << this->Point2[1]->GetValueAsFloat()
                    << this->Point2[2]->GetValueAsFloat()
                    << vtkClientServerStream::End;
    }
  if ( this->ResolutionVariable && sourceID.ID )
    {
    sprintf(acceptCmd, "Set%s", this->ResolutionVariable);
    pm->GetStream() << vtkClientServerStream::Invoke << sourceID
                    << acceptCmd
                    << this->ResolutionEntry->GetValueAsInt()
                    << vtkClientServerStream::End;
    }
  pm->SendStreamToServer();
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::ActualPlaceWidget()
{
  float bds[6];
  float x, y, z;

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
  
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  pm->GetStream() << vtkClientServerStream::Invoke <<  this->Widget3DID
                  << "PlaceWidget" 
                  <<  bds[0] << bds[1] << bds[2] << bds[3] << bds[4] << bds[5] 
                  << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
  this->UpdateVTKObject(this->ObjectID);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SaveInBatchScriptForPart(ofstream *file,
                                               vtkClientServerID sourceID)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();

  // Point1
  if (this->Point1Variable)
    {  
    *file << "\t" << "pvTemp" << sourceID << " Set" << this->Point1Variable;
    ostrstream str;
    str << "Get" << this->Point1Variable << ends;
    pm->GetStream() << vtkClientServerStream::Invoke << sourceID
                    << str.str()
                    << vtkClientServerStream::End; 
    ostrstream result;
    pm->GetLastClientResult().PrintArgumentValue(result, 0,0);
    result << ends;
    *file << " " << result.str() << "\n"; 
    delete [] result.str();
    delete [] str.str();
    }
  // Point2
  if (this->Point2Variable)
    {
    *file << "\t" << "pvTemp" << sourceID << " Set" << this->Point2Variable; 
    ostrstream str;
    str << "Get" << this->Point2Variable << ends;
    pm->GetStream() << vtkClientServerStream::Invoke << sourceID
                    << str.str()
                    << vtkClientServerStream::End; 
    pm->SendStreamToClient();
    ostrstream result;
    pm->GetLastClientResult().PrintArgumentValue(result, 0,0);
    result << ends;
    *file << " " << result.str() << "\n";
    delete [] result.str();
    delete [] str.str();
    }

  // Resolution
  if (this->ResolutionVariable)
    {
    *file << "\t" << "pvTemp" << sourceID << " Set" << this->ResolutionVariable;
    ostrstream str;
    str << "Get" << this->ResolutionVariable << ends;
    pm->GetStream() << vtkClientServerStream::Invoke << sourceID
                    << str.str()
                    << vtkClientServerStream::End; 
    pm->SendStreamToClient();
    ostrstream result;
    pm->GetLastClientResult().PrintArgumentValue(result, 0,0); 
    result << ends;
   *file << " " << result.str() << "\n";
    delete [] result.str();
    delete [] str.str();
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

  this->SetPoint1(this->LastAcceptedPoint1[0], this->LastAcceptedPoint1[1],
                  this->LastAcceptedPoint1[2]);
  this->SetPoint2(this->LastAcceptedPoint2[0], this->LastAcceptedPoint2[1],
                  this->LastAcceptedPoint2[2]);
  this->SetResolution(static_cast<int>(this->LastAcceptedResolution));

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
  vtkLineWidget* widget = vtkLineWidget::SafeDownCast(wdg);
  if (!widget)
    {
    return;
    }
  float val[3];
  int i;
  widget->GetPoint1(val);
  for (i=0; i<3; i++)
    {
    this->Point1[i]->SetValue(val[i]);
    }
  
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  pm->GetStream() << vtkClientServerStream::Invoke <<  this->Widget3DID
                  << "SetPoint1" << val[0] << val[1] <<  val[2]
                  << vtkClientServerStream::End;
  
  widget->GetPoint2(val);
  for (i=0; i<3; i++)
    {
    this->Point2[i]->SetValue(val[i],5);
    } 
  pm->GetStream() << vtkClientServerStream::Invoke <<  this->Widget3DID
                  << "SetPoint2" << val[0] << val[1] <<  val[2]
                  << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
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
  
  if ( this->Application && !this->BalloonHelpInitialized )
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
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  this->Widget3DID = pm->NewStreamObject("vtkLineWidget");
  pm->GetStream() << vtkClientServerStream::Invoke <<  this->Widget3DID
                  << "SetAlignToNone" << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
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
