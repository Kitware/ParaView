/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSphereWidget.cxx
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
#include "vtkPVSphereWidget.h"

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
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkSphereWidget.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkPVSphereWidget);

int vtkPVSphereWidgetCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVSphereWidget::vtkPVSphereWidget()
{
  int cc;
  this->Widget3D = vtkSphereWidget::New();
  this->Labels[0] = vtkKWLabel::New();
  this->Labels[1] = vtkKWLabel::New();  
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->CenterEntry[cc] = vtkKWEntry::New();
    this->CoordinateLabel[cc] = vtkKWLabel::New();
   }
  this->RadiusEntry = vtkKWEntry::New();
  this->CenterResetButton = vtkKWPushButton::New();
  this->SphereTclName = 0;
}

//----------------------------------------------------------------------------
vtkPVSphereWidget::~vtkPVSphereWidget()
{
  if (this->SphereTclName)
    {
    this->GetPVApplication()->BroadcastScript("%s Delete", 
					      this->SphereTclName);
    this->SetSphereTclName(NULL);
    }
  int i;
  this->Labels[0]->Delete();
  this->Labels[1]->Delete();
  for (i=0; i<3; i++)
    {
    this->CenterEntry[i]->Delete();
    this->CoordinateLabel[i]->Delete();
    }
  this->RadiusEntry->Delete();
  this->CenterResetButton->Delete();
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::CenterResetCallback()
{
  vtkPVData *input;
  float bds[6];

  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource has not been set.");
    return;
    }

  input = this->PVSource->GetPVInput();
  if (input == NULL)
    {
    return;
    }
  input->GetBounds(bds);
  this->SetCenter(0.5*(bds[0]+bds[1]),
		  0.5*(bds[2]+bds[3]),
		  0.5*(bds[4]+bds[5]));

  this->SetCenter();
}


//----------------------------------------------------------------------------
void vtkPVSphereWidget::Reset()
{
  if ( this->SphereTclName )
    {
    this->Script("eval %s SetCenter [ %s GetCenter ]", 
		 this->GetTclName(), this->SphereTclName);
    this->Script("eval %s SetRadius [ %s GetRadius ]", 
		 this->GetTclName(), this->SphereTclName);
    }
  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::Accept()  
{
  if ( this->SphereTclName )
    {
    vtkPVApplication *pvApp = static_cast<vtkPVApplication*>(
      this->Application);
    float val[3];
    int cc;
    for ( cc = 0; cc < 3; cc ++ )
      {
      val[cc] = atof( this->CenterEntry[cc]->GetValue() );
      }
    float rad = atof(this->RadiusEntry->GetValue());
    pvApp->BroadcastScript("%s SetCenter %f %f %f", this->SphereTclName,
			   val[0], val[1], val[2]);
    this->AddTraceEntry("$kw(%s) SetCenter %f %f %f", 
			this->GetTclName(), val[0], val[1], val[2]);
    pvApp->BroadcastScript("%s SetRadius %f", this->SphereTclName,
			   rad);
    this->AddTraceEntry("$kw(%s) SetRadius %f", 
			this->GetTclName(), rad);
    }
  this->ModifiedFlag = 0;
}


//----------------------------------------------------------------------------
void vtkPVSphereWidget::SaveInTclScript(ofstream *file)
{
  *file << "vtkSphere " << this->SphereTclName << endl;
  *file << "\t" << this->SphereTclName << " SetCenter ";
  this->Script("%s GetCenter", this->SphereTclName);
  *file << this->Application->GetMainInterp()->result << endl;
  *file << "\t" << this->SphereTclName << " SetRadius ";
  this->Script("%s GetRadius", this->SphereTclName);
  *file << this->Application->GetMainInterp()->result << endl;
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

vtkPVSphereWidget* vtkPVSphereWidget::ClonePrototype(vtkPVSource* pvSource,
				 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVSphereWidget::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::ChildCreate(vtkPVApplication* pvApp)
{
  static int instanceCount = 0;
  char sphereTclName[256];

  ++instanceCount;
  sprintf(sphereTclName, "pvSphere%d", instanceCount);
  this->SetTraceName(sphereTclName);
  pvApp->BroadcastScript("vtkSphere %s", sphereTclName);
  this->SetSphereTclName(sphereTclName);
  
  this->SetFrameLabel("Sphere Widget");
  this->Labels[0]->SetParent(this->Frame->GetFrame());
  this->Labels[0]->Create(pvApp, "");
  this->Labels[0]->SetLabel("Center");
  this->Labels[1]->SetParent(this->Frame->GetFrame());
  this->Labels[1]->Create(pvApp, "");
  this->Labels[1]->SetLabel("Radius");

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
    this->CenterEntry[i]->SetParent(this->Frame->GetFrame());
    this->CenterEntry[i]->Create(pvApp, "");
    }
  this->RadiusEntry->SetParent(this->Frame->GetFrame());
  this->RadiusEntry->Create(pvApp, "");

  this->Script("grid propagate %s 1",
	       this->Frame->GetFrame()->GetWidgetName());

  this->Script("grid x %s %s %s -sticky ew",
	       this->CoordinateLabel[0]->GetWidgetName(),
	       this->CoordinateLabel[1]->GetWidgetName(),
	       this->CoordinateLabel[2]->GetWidgetName());
  this->Script("grid %s %s %s %s -sticky ew",
	       this->Labels[0]->GetWidgetName(),
	       this->CenterEntry[0]->GetWidgetName(),
	       this->CenterEntry[1]->GetWidgetName(),
	       this->CenterEntry[2]->GetWidgetName());
  this->Script("grid %s %s - - -sticky ew",
	       this->Labels[1]->GetWidgetName(),
	       this->RadiusEntry->GetWidgetName());

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
		 this->CenterEntry[i]->GetWidgetName(),
		 this->GetTclName());
    this->Script("bind %s <FocusOut> {%s SetCenter}",
		 this->CenterEntry[i]->GetWidgetName(),
		 this->GetTclName());
    this->Script("bind %s <KeyPress-Return> {%s SetCenter}",
		 this->CenterEntry[i]->GetWidgetName(),
		 this->GetTclName());
    }
  this->Script("bind %s <Key> {%s SetValueChanged}",
	       this->RadiusEntry->GetWidgetName(),
	       this->GetTclName());
  this->Script("bind %s <FocusOut> {%s SetRadius}",
	       this->RadiusEntry->GetWidgetName(),
		 this->GetTclName());
  this->Script("bind %s <KeyPress-Return> {%s SetRadius}",
	       this->RadiusEntry->GetWidgetName(),
	       this->GetTclName());
  this->CenterResetButton->SetParent(this->Frame->GetFrame());
  this->CenterResetButton->Create(pvApp, "");
  this->CenterResetButton->SetLabel("Set Sphere Center to Center of Bounds");
  this->CenterResetButton->SetCommand(this, "CenterResetCallback"); 
  this->Script("grid %s - - - - -sticky ew", 
	       this->CenterResetButton->GetWidgetName());
  // Initialize the center of the sphere based on the input bounds.
  if (this->PVSource)
    {
    vtkPVData *input = this->PVSource->GetPVInput();
    if (input)
      {
      float bds[6];
      input->GetBounds(bds);
      pvApp->BroadcastScript("%s SetCenter %f %f %f", sphereTclName,
                             0.5*(bds[0]+bds[1]), 0.5*(bds[2]+bds[3]),
                             0.5*(bds[4]+bds[5]));
      pvApp->BroadcastScript("%s SetRadius %f", sphereTclName,
                             0.5*(bds[1]-bds[0]));
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::ExecuteEvent(vtkObject* wdg, unsigned long, void*)
{
  vtkSphereWidget *widget = vtkSphereWidget::SafeDownCast(wdg);
  if ( !widget )
    {
    cout << "This is not a sphere widget" << endl;
    return;
    }
  float val[3];
  widget->GetCenter(val); 
  this->SetCenter(val[0], val[1], val[2]);
  float rad = widget->GetRadius();
  this->SetRadius(rad);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
int vtkPVSphereWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                        vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetCenter(float x, float y, float z)
{
  this->CenterEntry[0]->SetValue(x, 5);
  this->CenterEntry[1]->SetValue(y, 5);
  this->CenterEntry[2]->SetValue(z, 5);  
  if ( this->Widget3D )
    {
    vtkSphereWidget *sphere = static_cast<vtkSphereWidget*>(this->Widget3D);
    sphere->SetCenter(x, y, z);
    }
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetCenter()
{
  float val[3];
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof(this->CenterEntry[cc]->GetValue());
    }
  vtkSphereWidget *sphere = static_cast<vtkSphereWidget*>(this->Widget3D);
  sphere->SetCenter(val);
  vtkPVGenericRenderWindowInteractor* iren = 
    this->PVSource->GetPVWindow()->GetGenericInteractor();
  if(iren)
    {
    iren->Render();
    }
  this->ModifiedCallback();
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetRadius(float r)
{
  this->RadiusEntry->SetValue(r, 5); 
  if ( this->Widget3D )
    {
    vtkSphereWidget *sphere = static_cast<vtkSphereWidget*>(this->Widget3D);
    sphere->SetRadius(r);
    }
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetRadius()
{
  float val;
  val = atof(this->RadiusEntry->GetValue());

  vtkSphereWidget *sphere = static_cast<vtkSphereWidget*>(this->Widget3D);
  sphere->SetRadius(val);
  vtkPVGenericRenderWindowInteractor* iren = 
    this->PVSource->GetPVWindow()->GetGenericInteractor();
  if(iren)
    {
    iren->Render();
    }
  this->ModifiedCallback();
  this->ValueChanged = 0;
}

