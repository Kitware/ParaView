/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVScalarRangeLabel.cxx
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
#include "vtkPVScalarRangeLabel.h"

#include "vtkKWApplication.h"
#include "vtkPVInputMenu.h"
#include "vtkPVData.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkPVArrayMenu.h"

vtkCxxSetObjectMacro(vtkPVScalarRangeLabel, ArrayMenu, vtkPVArrayMenu);

//----------------------------------------------------------------------------
vtkPVScalarRangeLabel* vtkPVScalarRangeLabel::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVScalarRangeLabel");
  if(ret)
    {
    return (vtkPVScalarRangeLabel*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVScalarRangeLabel;
}

//----------------------------------------------------------------------------
int vtkPVScalarRangeLabelCommand(ClientData cd, Tcl_Interp *interp,
			     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVScalarRangeLabel::vtkPVScalarRangeLabel()
{
  this->CommandFunction = vtkPVScalarRangeLabelCommand;

  this->Label = vtkKWLabel::New();
  this->ArrayMenu = NULL;

  this->Range[0] = VTK_LARGE_FLOAT;
  this->Range[1] = -VTK_LARGE_FLOAT;
}

//----------------------------------------------------------------------------
vtkPVScalarRangeLabel::~vtkPVScalarRangeLabel()
{
  this->Label->Delete();
  this->Label = NULL;
  this->SetArrayMenu(NULL);
}


//----------------------------------------------------------------------------
void vtkPVScalarRangeLabel::Create(vtkKWApplication *app)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("ScalarRangeLabel already created");
    return;
    }
  this->SetApplication(app);

  this->Script("frame %s", this->GetWidgetName());
  this->Label->SetParent(this);
  this->Label->SetLabel("");
  this->Label->Create(app, "");
  this->Script("pack %s -side top -expand t -fill x", 
               this->Label->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVScalarRangeLabel::Update()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  int id, num;
  vtkPVInputMenu *inputMenu;
  vtkPVData *pvd;
  vtkDataArray *array;
  float temp[2];

  if (this->ArrayMenu == NULL)
    {
    vtkErrorMacro("Array menu has not been set.");
    return;
    }

  array = this->ArrayMenu->GetVTKArray();
  if (array == NULL || array->GetName() == NULL)
    {
    this->Range[0] = VTK_LARGE_FLOAT;
    this->Range[1] = -VTK_LARGE_FLOAT;
    this->Label->SetLabel("Missing Array");
    return;
    }

  inputMenu = this->ArrayMenu->GetInputMenu();
  if (inputMenu == NULL)
    {
    vtkErrorMacro("Could not find input menu.");
    return;
    }

  pvd = inputMenu->GetPVData();
  if (pvd == NULL)
    {
    vtkErrorMacro("Could not find PVData.");
    return;
    }

  this->Range[0] = VTK_LARGE_FLOAT;
  this->Range[1] = -VTK_LARGE_FLOAT;
  pvApp->BroadcastScript("Application SendDataArrayRange %s {%s}",
                         pvd->GetVTKDataTclName(),
                         array->GetName());
  
  array->GetRange(this->Range, 0);  
  num = controller->GetNumberOfProcesses();
  for (id = 1; id < num; id++)
    {
    controller->Receive(temp, 2, id, 1976);
    // try to protect against invalid ranges.
    if (this->Range[0] > this->Range[1])
      {
      this->Range[0] = temp[0];
      this->Range[1] = temp[1];
      }
    else if (temp[0] < temp[1])
      {
      if (temp[0] < this->Range[0])
        {
        this->Range[0] = temp[0];
        }
      if (temp[1] > this->Range[1])
        {
        this->Range[1] = temp[1];
        }
      }
    }

  char str[512];
  if (this->Range[0] > this->Range[1])
    {
    sprintf(str, "Invalid Data Range");
    }
  else
    {
    sprintf(str, "Scalar Range: %f to %f", this->Range[0], this->Range[1]);
    }

  this->Label->SetLabel(str);
}

//----------------------------------------------------------------------------
void vtkPVScalarRangeLabel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ArrayMenu: " << this->GetArrayMenu() << endl;
  os << indent << "Range: " << this->GetRange() << endl;
}

vtkPVScalarRangeLabel* vtkPVScalarRangeLabel::ClonePrototype(
  vtkPVSource* pvSource, vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVScalarRangeLabel::SafeDownCast(clone);
}

void vtkPVScalarRangeLabel::CopyProperties(vtkPVWidget* clone, 
					   vtkPVSource* pvSource,
			      vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVScalarRangeLabel* pvsrl = vtkPVScalarRangeLabel::SafeDownCast(clone);
  if (pvsrl)
    {
    if (this->ArrayMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVArrayMenu* am = this->ArrayMenu->ClonePrototype(pvSource, map);
      pvsrl->SetArrayMenu(am);
      am->Delete();
      }
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVScalarRangeLabel.");
    }
}

//----------------------------------------------------------------------------
int vtkPVScalarRangeLabel::ReadXMLAttributes(vtkPVXMLElement* element,
                                             vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }  
  
  // Setup the ArrayMenu.
  const char* array_menu = element->GetAttribute("array_menu");
  if(!array_menu)
    {
    vtkErrorMacro("No array_menu attribute.");
    return 0;
    }
  
  vtkPVXMLElement* ame = element->LookupElement(array_menu);
  if (!ame)
    {
    vtkErrorMacro("Couldn't find ArrayMenu element " << array_menu);
    return 0;
    }
  vtkPVWidget* w = this->GetPVWidgetFromParser(ame, parser);
  vtkPVArrayMenu* amw = vtkPVArrayMenu::SafeDownCast(w);
  if(!amw)
    {
    if(w) { w->Delete(); }
    vtkErrorMacro("Couldn't get ArrayMenu widget " << array_menu);
    return 0;
    }
  amw->AddDependent(this);
  this->SetArrayMenu(amw);
  amw->Delete();  
  
  return 1;
}
