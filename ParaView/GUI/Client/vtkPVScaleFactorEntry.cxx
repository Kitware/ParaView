/*=========================================================================

  Program:   ParaView
  Module:    vtkPVScaleFactorEntry.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVScaleFactorEntry.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMBoundsDomain.h"

vtkStandardNewMacro(vtkPVScaleFactorEntry);
vtkCxxRevisionMacro(vtkPVScaleFactorEntry, "1.15");
//-----------------------------------------------------------------------------
vtkPVScaleFactorEntry::vtkPVScaleFactorEntry()
{
}

//-----------------------------------------------------------------------------
vtkPVScaleFactorEntry::~vtkPVScaleFactorEntry()
{
}

//-----------------------------------------------------------------------------
void vtkPVScaleFactorEntry::Initialize()
{
  this->Superclass::Initialize();
  this->Update();
}

//-----------------------------------------------------------------------------
void vtkPVScaleFactorEntry::Update()
{
  this->UpdateScaleFactor();
  this->vtkPVWidget::Update();
}

//-----------------------------------------------------------------------------
void vtkPVScaleFactorEntry::UpdateScaleFactor()
{
  vtkSMProperty *prop = this->GetSMProperty();
  vtkSMBoundsDomain *dom = 0;
  
  if (prop)
    {
    dom = vtkSMBoundsDomain::SafeDownCast(prop->GetDomain("bounds"));
    }
  
  if (!prop || !dom)
    {
    if (prop)
      {
      vtkErrorMacro("Could not find required domain (bounds)");
      }
    this->Superclass::Update();
    return;
    }
  int exists;
  double max = dom->GetMaximum(0, exists);
  char value[1000];
  sprintf(value, "%g", max);
  this->SetValue(value);
}

//-----------------------------------------------------------------------------
int vtkPVScaleFactorEntry::ReadXMLAttributes(vtkPVXMLElement *element,
                                              vtkPVXMLPackageParser *parser)
{
  if (!this->Superclass::ReadXMLAttributes(element, parser))
    {
    return 0;
    }

  const char* input_menu = element->GetAttribute("input_menu");
  if (input_menu)
    {
    vtkPVXMLElement *ime = element->LookupElement(input_menu);
    if (!ime)
      {
      vtkErrorMacro("Couldn't find InputMenu element " << input_menu);
      return 0;
      }
    
    vtkPVWidget *w = this->GetPVWidgetFromParser(ime, parser);
    vtkPVInputMenu *imw = vtkPVInputMenu::SafeDownCast(w);
    if (!imw)
      {
      if (w)
        {
        w->Delete();
        }
      vtkErrorMacro("Couldn't get InputMenu widget " << input_menu);
      return 0;
      }
    imw->AddDependent(this);
    imw->Delete();
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVScaleFactorEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
