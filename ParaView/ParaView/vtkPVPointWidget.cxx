/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPointWidget.cxx
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
#include "vtkPVPointWidget.h"

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
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVDataInformation.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkPointWidget.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkPVPointWidget);
vtkCxxRevisionMacro(vtkPVPointWidget, "1.22");

int vtkPVPointWidgetCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVPointWidget::vtkPVPointWidget()
{
  int cc;
  this->Labels[0] = vtkKWLabel::New();
  this->Labels[1] = vtkKWLabel::New();  
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->PositionEntry[cc] = vtkKWEntry::New();
    this->CoordinateLabel[cc] = vtkKWLabel::New();
   }
  this->PositionResetButton = vtkKWPushButton::New();
  
  this->LastAcceptedPosition[0] = this->LastAcceptedPosition[1] =
    this->LastAcceptedPosition[2] = 0;
}

//----------------------------------------------------------------------------
vtkPVPointWidget::~vtkPVPointWidget()
{
  int i;
  this->Labels[0]->Delete();
  this->Labels[1]->Delete();
  for (i=0; i<3; i++)
    {
    this->PositionEntry[i]->Delete();
    this->CoordinateLabel[i]->Delete();
    }
  this->PositionResetButton->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::PositionResetCallback()
{
  vtkPVSource *input;
  double bds[6];

  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource has not been set.");
    return;
    }

  input = this->PVSource->GetPVInput(0);
  if (input == NULL)
    {
    return;
    }
  input->GetDataInformation()->GetBounds(bds);
  this->SetPosition(0.5*(bds[0]+bds[1]),
                  0.5*(bds[2]+bds[3]),
                  0.5*(bds[4]+bds[5]));

  this->SetPosition();
}


//----------------------------------------------------------------------------
void vtkPVPointWidget::ResetInternal()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }
  // Reset point
  this->SetPosition(this->LastAcceptedPosition[0],
                    this->LastAcceptedPosition[1],
                    this->LastAcceptedPosition[2]);
  
  this->Superclass::ResetInternal();
}


//----------------------------------------------------------------------------
void vtkPVPointWidget::AcceptInternal(const char* sourceTclName)  
{
  this->SetPositionInternal(this->PositionEntry[0]->GetValueAsFloat(),
                            this->PositionEntry[1]->GetValueAsFloat(),
                            this->PositionEntry[2]->GetValueAsFloat());

  this->UpdateVTKObject();
  
  this->Superclass::AcceptInternal(sourceTclName);
}

//---------------------------------------------------------------------------
void vtkPVPointWidget::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetPosition "
        << this->PositionEntry[0]->GetValue() << " "
        << this->PositionEntry[1]->GetValue() << " "
        << this->PositionEntry[2]->GetValue() << endl;
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::UpdateVTKObject()  
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  // Accept point
  char acceptCmd[1024];
  if ( this->VariableName && this->ObjectTclName )
    {    
    sprintf(acceptCmd, "%s Set%s %f %f %f", this->ObjectTclName, 
            this->VariableName,
            this->PositionEntry[0]->GetValueAsFloat(),
            this->PositionEntry[1]->GetValueAsFloat(),
            this->PositionEntry[2]->GetValueAsFloat());
    pvApp->GetProcessModule()->ServerScript(acceptCmd);
    }
  
  this->SetLastAcceptedPosition(this->PositionEntry[0]->GetValueAsFloat(),
                                this->PositionEntry[1]->GetValueAsFloat(),
                                this->PositionEntry[2]->GetValueAsFloat());
}


//----------------------------------------------------------------------------
void vtkPVPointWidget::SaveInBatchScriptForPart(ofstream *, 
                                                const char*)
{
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkPVPointWidget* vtkPVPointWidget::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVPointWidget::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::ChildCreate(vtkPVApplication* pvApp)
{
  static int instanceCount = 0;
  char tclName[256];

  if ((this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName("Point");
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }

  ++instanceCount;
  sprintf(tclName, "pvPointWidget%d", instanceCount);
  pvApp->BroadcastScript("vtkPointWidget %s", tclName);
  pvApp->BroadcastScript("%s AllOff", tclName);
  this->SetWidget3DTclName(tclName);

  this->SetFrameLabel("Point Widget");
  this->Labels[0]->SetParent(this->Frame->GetFrame());
  this->Labels[0]->Create(pvApp, "");
  this->Labels[0]->SetLabel("Position");

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
    this->PositionEntry[i]->SetParent(this->Frame->GetFrame());
    this->PositionEntry[i]->Create(pvApp, "");
    }

  this->Script("grid propagate %s 1",
               this->Frame->GetFrame()->GetWidgetName());

  this->Script("grid x %s %s %s -sticky ew",
               this->CoordinateLabel[0]->GetWidgetName(),
               this->CoordinateLabel[1]->GetWidgetName(),
               this->CoordinateLabel[2]->GetWidgetName());
  this->Script("grid %s %s %s %s -sticky ew",
               this->Labels[0]->GetWidgetName(),
               this->PositionEntry[0]->GetWidgetName(),
               this->PositionEntry[1]->GetWidgetName(),
               this->PositionEntry[2]->GetWidgetName());

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
                 this->PositionEntry[i]->GetWidgetName(),
                 this->GetTclName());
    this->Script("bind %s <FocusOut> {%s SetPosition}",
                 this->PositionEntry[i]->GetWidgetName(),
                 this->GetTclName());
    this->Script("bind %s <KeyPress-Return> {%s SetPosition}",
                 this->PositionEntry[i]->GetWidgetName(),
                 this->GetTclName());
    }
  this->PositionResetButton->SetParent(this->Frame->GetFrame());
  this->PositionResetButton->Create(pvApp, "");
  this->PositionResetButton->SetLabel("Set Point Position to Center of Bounds");
  this->PositionResetButton->SetCommand(this, "PositionResetCallback"); 
  this->Script("grid %s - - - - -sticky ew", 
               this->PositionResetButton->GetWidgetName());

  // This appears to be doing nothing.  Take it out !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // Initialize the center of the point based on the input bounds.
  //if (this->PVSource)
  //  {
  //  vtkPVData *input = this->PVSource->GetPVInput();
  //  if (input)
  //    {
  //    float bds[6];
  //    input->GetBounds(bds);
  //    }
  //  }
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::ExecuteEvent(vtkObject* wdg, unsigned long l, void* p)
{
  vtkPointWidget *widget = vtkPointWidget::SafeDownCast(wdg);
  if ( !widget )
    {
    vtkErrorMacro( "This is not a point widget" );
    return;
    }
  float val[3];
  widget->GetPosition(val); 
  this->SetPosition(val[0], val[1], val[2]);

  this->Superclass::ExecuteEvent(wdg, l, p);
}

//----------------------------------------------------------------------------
int vtkPVPointWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                        vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::ActualPlaceWidget()
{
  this->Superclass::ActualPlaceWidget();

  double bounds[6];
  this->PVSource->GetPVInput(0)->GetDataInformation()->GetBounds(bounds);

  this->SetPosition((bounds[0]+bounds[1])/2,(bounds[2]+bounds[3])/2, 
                    (bounds[4]+bounds[5])/2);
  this->UpdateVTKObject();
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::SetPositionInternal(float x, float y, float z)
{
  this->PositionEntry[0]->SetValue(x);
  this->PositionEntry[1]->SetValue(y);
  this->PositionEntry[2]->SetValue(z);  
  if ( this->Widget3DTclName )
    {
    this->GetPVApplication()->BroadcastScript(
      "%s SetPosition %f %f %f", this->Widget3DTclName, x, y, z);
    }
  this->Render();
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::SetPosition(float x, float y, float z)
{
  this->SetPositionInternal(x, y, z);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::GetPosition(float pt[3])
{
  if (pt == NULL || this->Application == NULL)
    {
    vtkErrorMacro("Cannot get your point.");
    return;
    }
  pt[0] = this->PositionEntry[0]->GetValueAsFloat(); 
  pt[1] = this->PositionEntry[1]->GetValueAsFloat(); 
  pt[2] = this->PositionEntry[2]->GetValueAsFloat(); 
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::SetPosition()
{
  float val[3];
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof(this->PositionEntry[cc]->GetValue());
    }
  this->GetPVApplication()->BroadcastScript(
    "%s SetPosition %f %f %f", this->Widget3DTclName, val[0], val[1], val[2]);
  this->ModifiedCallback();
  this->ValueChanged = 0;
}

 
