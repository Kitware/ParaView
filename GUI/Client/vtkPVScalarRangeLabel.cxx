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
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVScalarRangeLabel);
vtkCxxRevisionMacro(vtkPVScalarRangeLabel, "1.25");

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
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->Label->SetParent(this);
  this->Label->SetLabel("");
  this->Label->Create(app, "");
  this->Script("pack %s -side top -expand t -fill x", 
               this->Label->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVScalarRangeLabel::Update()
{
  this->Range[0] = VTK_LARGE_FLOAT;
  this->Range[1] = -VTK_LARGE_FLOAT;

  vtkSMProperty* prop = this->GetSMProperty();
  vtkSMDoubleRangeDomain* dom = 0;
  if (prop)
    {
    dom = vtkSMDoubleRangeDomain::SafeDownCast(prop->GetDomain("scalar_range"));
    }
  if (!prop || !dom)
    {
    vtkErrorMacro("Could not find required domain (scalar_range)");
    this->Label->SetLabel("Missing Array");
    this->Superclass::Update();
    return;
    }

  int exists;
  double rg = dom->GetMinimum(0, exists);
  if (exists)
    {
    this->Range[0] = rg;
    }
  rg = dom->GetMaximum(0, exists);
  if (exists)
    {
    this->Range[1] = rg;
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
