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
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"

vtkStandardNewMacro(vtkPVLineWidget);

//----------------------------------------------------------------------------
vtkPVLineWidget::vtkPVLineWidget()
{
  this->Widget3D = vtkLineWidget::New();
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
  this->Point1Object = 0;
  this->Point2Object = 0;
  this->ResolutionObject = 0;
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
  this->SetPoint1Object(0);
  this->SetPoint2Object(0);
  this->SetResolutionObject(0);
}


//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint1Method(const char* wname, const char* varname)
{
  this->SetPoint1Object(wname);
  this->SetPoint1Variable(varname);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint2Method(const char* wname, const char* varname)
{
  this->SetPoint2Object(wname);
  this->SetPoint2Variable(varname);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetResolutionMethod(const char* wname, const char* varname)
{
  this->SetResolutionObject(wname);
  this->SetResolutionVariable(varname);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint1(float x, float y, float z)
{
  this->Point1[0]->SetValue(x,5);
  this->Point1[1]->SetValue(y,5);
  this->Point1[2]->SetValue(z,5);
 
  int i;
  float pos[3];
  for (i=0; i<3; i++)
    {
    pos[i] = this->Point1[i]->GetValueAsFloat();
    }
  vtkLineWidget *line = static_cast<vtkLineWidget*>( this->Widget3D );
  line->SetPoint1(pos);
  line->SetAlignToNone();
  vtkPVGenericRenderWindowInteractor* iren = 
    this->PVSource->GetPVWindow()->GetGenericInteractor();
  if(iren)
    {
    iren->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint2(float x, float y, float z)
{
  this->Point2[0]->SetValue(x,5);
  this->Point2[1]->SetValue(y,5);
  this->Point2[2]->SetValue(z,5);
 
  int i;
  float pos[3];
  for (i=0; i<3; i++)
    {
    pos[i] = this->Point2[i]->GetValueAsFloat();
    }
  vtkLineWidget *line = static_cast<vtkLineWidget*>( this->Widget3D );
  line->SetPoint2(pos);
  line->SetAlignToNone();
  vtkPVGenericRenderWindowInteractor* iren = 
    this->PVSource->GetPVWindow()->GetGenericInteractor();
  if(iren)
    {
    iren->Render();
    }
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
  vtkLineWidget *line = static_cast<vtkLineWidget*>( this->Widget3D );
  line->SetResolution(res);
  vtkPVGenericRenderWindowInteractor* iren = 
    this->PVSource->GetPVWindow()->GetGenericInteractor();
  if(iren)
    {
    iren->Render();
    }
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

//----------------------------------------------------------------------------
void vtkPVLineWidget::Accept()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }

  vtkPVApplication *pvApp = this->GetPVApplication();
  ofstream *traceFile = pvApp->GetTraceFile();
  int traceFlag = 0;

  // Start the trace entry and the accept command.
  if (traceFile && this->InitializeTrace())
    {
    traceFlag = 1;
    }

  if (traceFlag)
    {
    *traceFile << "$kw(" << this->GetTclName() << ") SetPoint1 "
	       << this->Point1[0]->GetValue() << " "
	       << this->Point1[1]->GetValue() << " "
	       << this->Point1[2]->GetValue() << endl;
    *traceFile << "$kw(" << this->GetTclName() << ") SetPoint2 "
	       << this->Point2[0]->GetValue() << " "
	       << this->Point2[1]->GetValue() << " "
	       << this->Point2[2]->GetValue() << endl;
    *traceFile << "$kw(" << this->GetTclName() << ") SetResolution "
	       << this->ResolutionEntry->GetValue() << endl;
    }

  char acceptCmd[1024];
  if ( this->Point1Variable && this->Point1Object )
    {
    sprintf(acceptCmd, "%s Set%s %f %f %f", this->Point1Object, 
	    this->Point1Variable,
	    this->Point1[0]->GetValueAsFloat(),
	    this->Point1[1]->GetValueAsFloat(),
	    this->Point1[2]->GetValueAsFloat());
    pvApp->BroadcastScript(acceptCmd);
    }
  if ( this->Point2Variable && this->Point2Object )
    {
    sprintf(acceptCmd, "%s Set%s %f %f %f", this->Point2Object, 
	    this->Point2Variable,
	    this->Point2[0]->GetValueAsFloat(),
	    this->Point2[1]->GetValueAsFloat(),
	    this->Point2[2]->GetValueAsFloat());
    pvApp->BroadcastScript(acceptCmd);
    }
  if ( this->ResolutionVariable && this->ResolutionObject )
    {
    sprintf(acceptCmd, "%s Set%s %i", this->ResolutionObject, 
	    this->ResolutionVariable,
	    this->ResolutionEntry->GetValueAsInt());
    pvApp->BroadcastScript(acceptCmd);
    }
  this->Superclass::Accept();
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::Reset()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }
  if ( this->Point1Variable && this->Point1Object )
    {
    this->Script("eval %s SetPoint1 [ %s Get%s ]",
		 this->GetTclName(), this->Point1Object, 
		 this->Point1Variable);
    }
  if ( this->Point2Variable && this->Point2Object )
    {
    this->Script("eval %s SetPoint2 [ %s Get%s ]",
		 this->GetTclName(), this->Point2Object, 
		 this->Point2Variable);
    }
  if ( this->ResolutionVariable && this->ResolutionObject )
    {
    this->Script("%s SetResolution [ %s Get%s ]",
		 this->GetTclName(), this->ResolutionObject, 
		 this->ResolutionVariable);
    }
  this->Superclass::Reset();
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
    this->Point1[i]->SetValue(val[i],5);
    }
  widget->GetPoint2(val);
  for (i=0; i<3; i++)
    {
    this->Point2[i]->SetValue(val[i],5);
    }
  this->Superclass::ExecuteEvent(wdg, l, p);
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::ChildCreate(vtkPVApplication* pvApp)
{
  this->SetTraceName("Line");

  this->SetFrameLabel("Line Widget");
  this->Labels[0]->SetParent(this->Frame->GetFrame());
  this->Labels[0]->Create(pvApp, "");
  this->Labels[0]->SetLabel("Point 0");
  this->Labels[1]->SetParent(this->Frame->GetFrame());
  this->Labels[1]->Create(pvApp, "");
  this->Labels[1]->SetLabel("Point 1");
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
  this->ResolutionLabel->SetLabel("Resolution");
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
  this->Script("grid %s %s - - -sticky ew",
	       this->ResolutionLabel->GetWidgetName(),
	       this->ResolutionEntry->GetWidgetName());

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
  
}
