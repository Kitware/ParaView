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
vtkCxxRevisionMacro(vtkPVScaleFactorEntry, "1.13");

vtkPVScaleFactorEntry::vtkPVScaleFactorEntry()
{
  this->ScaleFactor = 0.1;
}

vtkPVScaleFactorEntry::~vtkPVScaleFactorEntry()
{
}

void vtkPVScaleFactorEntry::ResetInternal()
{
  if (this->AcceptCalled)
    {
    this->Superclass::ResetInternal();
    return;
    }
  this->Update();
}

void vtkPVScaleFactorEntry::Update()
{
  this->UpdateScaleFactor();
  this->vtkPVWidget::Update();
}

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
  
  int exists, i;
  
  double bnds[6];
  
  for (i = 0; i < 3; i++)
    {
    bnds[2*i] = dom->GetMinimum(i, exists);
    if (!exists)
      {
      bnds[2*i] = 0;
      }
    bnds[2*i+1] = dom->GetMaximum(i, exists);
    if (!exists)
      {
      bnds[2*i+1] = 1;
      }
    } 
  
  double maxBnds = bnds[1] - bnds[0];
  maxBnds = (bnds[3] - bnds[2] > maxBnds) ? (bnds[3] - bnds[2]) : maxBnds;
  maxBnds = (bnds[5] - bnds[4] > maxBnds) ? (bnds[5] - bnds[4]) : maxBnds;
  maxBnds *= this->ScaleFactor;
  
  char value[1000];
  sprintf(value, "%g", maxBnds);
  this->SetValue(value);
}

int vtkPVScaleFactorEntry::ReadXMLAttributes(vtkPVXMLElement *element,
                                              vtkPVXMLPackageParser *parser)
{
  if (!this->Superclass::ReadXMLAttributes(element, parser))
    {
    return 0;
    }

  // Setup the scaling.
  if(!element->GetScalarAttribute("scale_factor",
                                  &this->ScaleFactor))
    {
    this->ScaleFactor = 0.1;
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

void vtkPVScaleFactorEntry::CopyProperties(
  vtkPVWidget *clone, vtkPVSource *source,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, source, map);
  vtkPVScaleFactorEntry *pvsfe = vtkPVScaleFactorEntry::SafeDownCast(clone);
  if (pvsfe)
    {
    pvsfe->ScaleFactor = this->ScaleFactor;
    }
}                        

//----------------------------------------------------------------------------
void vtkPVScaleFactorEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "ScaleFactor: " << this->ScaleFactor << endl;
}
