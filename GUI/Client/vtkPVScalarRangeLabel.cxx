/*=========================================================================

  Program:   ParaView
  Module:    vtkPVScalarRangeLabel.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVScalarRangeLabel.h"

#include "vtkDataArray.h"
#include "vtkKWApplication.h"
#include "vtkKWLabel.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVScalarRangeLabel);
vtkCxxRevisionMacro(vtkPVScalarRangeLabel, "1.20");

vtkCxxSetObjectMacro(vtkPVScalarRangeLabel, ArrayMenu, vtkPVArrayMenu);

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
  if (this->IsCreated())
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
  vtkPVArrayInformation *ai;

  if (this->ArrayMenu == NULL)
    {
    vtkErrorMacro("Array menu has not been set.");
    return;
    }

  ai = this->ArrayMenu->GetArrayInformation();
  if (ai == NULL || ai->GetName() == NULL)
    {
    this->Range[0] = VTK_LARGE_FLOAT;
    this->Range[1] = -VTK_LARGE_FLOAT;
    this->Label->SetLabel("Missing Array");
    this->Superclass::Update();
    return;
    }

  ai->GetComponentRange(0, this->Range);

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
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void vtkPVScalarRangeLabel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ArrayMenu: " << this->GetArrayMenu() << endl;
  os << indent << "Range: " << this->GetRange() << endl;
}

//----------------------------------------------------------------------------
vtkPVScalarRangeLabel* vtkPVScalarRangeLabel::ClonePrototype(
  vtkPVSource* pvSource, vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVScalarRangeLabel::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
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
