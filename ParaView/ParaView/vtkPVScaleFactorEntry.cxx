/*=========================================================================

  Program:   ParaView
  Module:    vtkPVScaleFactorEntry.cxx
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
#include "vtkPVScaleFactorEntry.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"

vtkStandardNewMacro(vtkPVScaleFactorEntry);
vtkCxxRevisionMacro(vtkPVScaleFactorEntry, "1.4.4.3");

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
  
  float bnds[6];
  input->GetDataInformation()->GetBounds(bnds);
  
  float maxBnds = bnds[1] - bnds[0];
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
