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

vtkStandardNewMacro(vtkPVScaleFactorEntry);
vtkCxxRevisionMacro(vtkPVScaleFactorEntry, "1.7");

vtkCxxSetObjectMacro(vtkPVScaleFactorEntry, InputMenu, vtkPVInputMenu);

vtkPVScaleFactorEntry::vtkPVScaleFactorEntry()
{
  this->InputMenu = NULL;
  this->Input = NULL;
  this->ScaleFactor = 0.1;
}

vtkPVScaleFactorEntry::~vtkPVScaleFactorEntry()
{
  this->SetInputMenu(NULL);
}

void vtkPVScaleFactorEntry::ResetInternal()
{
  if (this->AcceptCalled)
    {
    this->Superclass::ResetInternal();
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
  if (!this->InputMenu)
    {
    return;
    }
  
  vtkPVSource *input = this->InputMenu->GetCurrentValue();
  if (!input || (input == this->Input && this->AcceptCalled))
    {
    return;
    }
  
  this->Input = input;
  
  double bnds[6];
  input->GetDataInformation()->GetBounds(bnds);
  
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
    this->SetInputMenu(imw);
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
    if (this->InputMenu)
      {
      vtkPVInputMenu *im = this->InputMenu->ClonePrototype(source, map);
      pvsfe->SetInputMenu(im);
      im->Delete();
      }
    pvsfe->SetInput(this->Input);
    }
}                        

void vtkPVScaleFactorEntry::SetInput(vtkPVSource *input)
{
  this->Input = input;
}

void vtkPVScaleFactorEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "InputMenu: ";
  if (this->InputMenu)
    {
    os << this->InputMenu << endl;
    }
  else
    {
    os << "(none)" << endl;
    }
}
