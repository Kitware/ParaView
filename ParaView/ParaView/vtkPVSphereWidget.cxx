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
#include "vtkPVSource.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVVectorEntry.h"
#include "vtkKWCompositeCollection.h"

int vtkPVSphereWidgetCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVSphereWidget::vtkPVSphereWidget()
{
  this->CommandFunction = vtkPVSphereWidgetCommand;

  this->CenterEntry = vtkPVVectorEntry::New();
  this->CenterEntry->SetTraceReferenceObject(this);
  this->CenterEntry->SetTraceReferenceCommand("GetCenterEntry");

  this->RadiusEntry = vtkPVVectorEntry::New();
  this->RadiusEntry->SetTraceReferenceObject(this);
  this->RadiusEntry->SetTraceReferenceCommand("GetRadiusEntry");

  this->SphereTclName = NULL;

  this->ObjectTclName = NULL;
  this->VariableName = NULL;
}

//----------------------------------------------------------------------------
vtkPVSphereWidget::~vtkPVSphereWidget()
{
  this->CenterEntry->Delete();
  this->CenterEntry = NULL;
  this->RadiusEntry->Delete();
  this->RadiusEntry = NULL;

  if (this->SphereTclName)
    {
    this->GetPVApplication()->BroadcastScript("%s Delete", this->SphereTclName);
    this->SetSphereTclName(NULL);
    }

  this->SetObjectTclName(NULL);
  this->SetVariableName(NULL);
}

//----------------------------------------------------------------------------
vtkPVSphereWidget* vtkPVSphereWidget::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVSphereWidget");
  if(ret)
    {
    return (vtkPVSphereWidget*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVSphereWidget;
}


//----------------------------------------------------------------------------
void vtkPVSphereWidget::Create(vtkKWApplication *app)
{
  static int instanceCount = 0;
  char sphereTclName[256];
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);

  if (pvApp == NULL)
    {
    vtkErrorMacro("Expecting a PVApplication.");
    return;
    }

  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    return;
    }
  this->SetApplication(app);
  
  // Create the implicit sphere associated with this widget.
  ++instanceCount;
  sprintf(sphereTclName, "pvSphere%d", instanceCount);
  pvApp->BroadcastScript("vtkSphere %s", sphereTclName);
  this->SetSphereTclName(sphereTclName);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());
 
  this->CenterEntry->SetParent(this);
  this->CenterEntry->SetObjectVariable(sphereTclName, "Center");
  this->CenterEntry->SetModifiedCommand(this->GetTclName(), 
                                        "ModifiedCallback");
  this->CenterEntry->SetLabel("Center");
  this->CenterEntry->SetVectorLength(3);
  this->CenterEntry->Create(this->Application);
  this->Script("pack %s -side top -fill x",
               this->CenterEntry->GetWidgetName());

  this->RadiusEntry->SetParent(this);
  this->RadiusEntry->SetObjectVariable(sphereTclName, "Radius");
  this->RadiusEntry->SetModifiedCommand(this->GetTclName(), 
                                        "ModifiedCallback");
  this->RadiusEntry->SetLabel("Radius");
  this->RadiusEntry->Create(this->Application);
  this->Script("pack %s -side top -fill x",
               this->RadiusEntry->GetWidgetName());

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
void vtkPVSphereWidget::Reset()
{
  this->CenterEntry->Reset();
  this->RadiusEntry->Reset();

  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if ( this->CenterEntry->GetModifiedFlag())
    {
    this->CenterEntry->Accept();
    }
  if ( this->RadiusEntry->GetModifiedFlag())
    {
    this->RadiusEntry->Accept();
    }

  // Set this here to keep this widget like others.
  if (this->ObjectTclName && this->VariableName && this->SphereTclName)
    {
    pvApp->BroadcastScript("%s Set%s %s", this->ObjectTclName,
                           this->VariableName, this->SphereTclName);
    }
  this->ModifiedFlag = 0;
}


//----------------------------------------------------------------------------
void vtkPVSphereWidget::SaveInTclScript(ofstream *file)
{
  *file << "vtkSphere " << this->SphereTclName << endl;

  if (this->ObjectTclName && this->VariableName)
    {
    *file << "\t" << this->ObjectTclName << " Set" << this->VariableName
          << " " << this->SphereTclName << endl;
    }

  this->CenterEntry->SaveInTclScript(file);
  this->RadiusEntry->SaveInTclScript(file);
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CenterEntry: " << this->GetCenterEntry() << endl;
  os << indent << "RadiusEntry: " << this->GetRadiusEntry() << endl;
  os << indent << "SphereTclName: " << this->GetSphereTclName() << endl;
}

vtkPVSphereWidget* vtkPVSphereWidget::ClonePrototype(vtkPVSource* pvSource,
				 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVSphereWidget::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
int vtkPVSphereWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                         vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  return 1;
}
