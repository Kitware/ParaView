/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGlyphScaleFactorEntry.cxx
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
#include "vtkPVGlyphScaleFactorEntry.h"

#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVSelectionList.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"

vtkStandardNewMacro(vtkPVGlyphScaleFactorEntry);
vtkCxxRevisionMacro(vtkPVGlyphScaleFactorEntry, "1.1");

vtkCxxSetObjectMacro(vtkPVGlyphScaleFactorEntry, ScalarMenu, vtkPVArrayMenu);
vtkCxxSetObjectMacro(vtkPVGlyphScaleFactorEntry, VectorMenu, vtkPVArrayMenu);
vtkCxxSetObjectMacro(vtkPVGlyphScaleFactorEntry, ScaleModeList,
                     vtkPVSelectionList);

vtkPVGlyphScaleFactorEntry::vtkPVGlyphScaleFactorEntry()
{
  this->ScalarMenu = NULL;
  this->VectorMenu = NULL;
  this->ScaleModeList = NULL;
}

vtkPVGlyphScaleFactorEntry::~vtkPVGlyphScaleFactorEntry()
{
  this->SetScalarMenu(NULL);
  this->SetVectorMenu(NULL);
  this->SetScaleModeList(NULL);
}

void vtkPVGlyphScaleFactorEntry::CopyProperties(
  vtkPVWidget *clone, vtkPVSource *pvSource,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVGlyphScaleFactorEntry *gsfe =
    vtkPVGlyphScaleFactorEntry::SafeDownCast(clone);
  if (gsfe)
    {
    if (this->ScalarMenu)
      {
      vtkPVArrayMenu *am = this->ScalarMenu->ClonePrototype(pvSource, map);
      gsfe->SetScalarMenu(am);
      am->Delete();
      }
    if (this->VectorMenu)
      {
      vtkPVArrayMenu *am = this->VectorMenu->ClonePrototype(pvSource, map);
      gsfe->SetVectorMenu(am);
      am->Delete();
      }
    if (this->ScaleModeList)
      {
      vtkPVSelectionList *sl =
        this->ScaleModeList->ClonePrototype(pvSource, map);
      gsfe->SetScaleModeList(sl);
      sl->Delete();
      }
    }
}

int vtkPVGlyphScaleFactorEntry::ReadXMLAttributes(
  vtkPVXMLElement *element, vtkPVXMLPackageParser *parser)
{
  if (!this->Superclass::ReadXMLAttributes(element, parser))
    {
    return 0;
    }
  
  const char *scalar_menu = element->GetAttribute("scalar_menu");
  const char *vector_menu = element->GetAttribute("vector_menu");
  const char *scale_mode_list = element->GetAttribute("scale_mode_list");
  
  vtkPVXMLElement *ime;
  vtkPVWidget *w;
  
  if (scalar_menu)
    {
    ime = element->LookupElement(scalar_menu);
    w = this->GetPVWidgetFromParser(ime, parser);
    vtkPVArrayMenu *am = vtkPVArrayMenu::SafeDownCast(w);
    if (!am)
      {
      if (w)
        {
        w->Delete();
        }
      vtkErrorMacro("Couldn't get scalar menu " << scalar_menu);
      return 0;
      }
    am->AddDependent(this);
    this->SetScalarMenu(am);
    am->Delete();
    }
  if (vector_menu)
    {
    ime = element->LookupElement(vector_menu);
    w = this->GetPVWidgetFromParser(ime, parser);
    vtkPVArrayMenu *am = vtkPVArrayMenu::SafeDownCast(w);
    if (!am)
      {
      if (w)
        {
        w->Delete();
        }
      vtkErrorMacro("Couldn't get vector menu " << vector_menu);
      return 0;
      }
    am->AddDependent(this);
    this->SetVectorMenu(am);
    am->Delete();
    }
  if (scale_mode_list)
    {
    ime = element->LookupElement(scale_mode_list);
    w = this->GetPVWidgetFromParser(ime, parser);
    vtkPVSelectionList *list = vtkPVSelectionList::SafeDownCast(w);
    if (!list)
      {
      if (w)
        {
        w->Delete();
        }
      vtkErrorMacro("Couldn't get scale mode selection list "
                    << scale_mode_list);
      return 0;
      }
    list->AddDependent(this);
    this->SetScaleModeList(list);
    list->Delete();
    }
    
  return 1;
}

void vtkPVGlyphScaleFactorEntry::UpdateScaleFactor()
{
  if (!this->InputMenu || !this->ScalarMenu || !this->VectorMenu ||
      !this->ScaleModeList)
    {
    return;
    }
  
  vtkPVSource *input = this->InputMenu->GetCurrentValue();
  if (!input || (input == this->Input && this->AcceptCalled &&
                 !this->ScalarMenu->GetModifiedFlag() &&
                 !this->VectorMenu->GetModifiedFlag() &&
                 !this->ScaleModeList->GetModifiedFlag()))
    {
    return;
    }
  
  this->Input = input;
  
  float bnds[6];
  vtkPVDataInformation *dInfo = input->GetDataInformation();
  dInfo->GetBounds(bnds);
  vtkPVDataSetAttributesInformation *pdInfo = dInfo->GetPointDataInformation();
  vtkPVArrayInformation *aInfo;
  
  double maxBnds = bnds[1] - bnds[0];
  maxBnds = (bnds[3] - bnds[2] > maxBnds) ? (bnds[3] - bnds[2]) : maxBnds;
  maxBnds = (bnds[5] - bnds[4] > maxBnds) ? (bnds[5] - bnds[4]) : maxBnds;
  maxBnds *= 0.1;

  int scaleMode = this->ScaleModeList->GetCurrentValue();
  switch (scaleMode)
    {
    case 0: // Scalar
    {
    const char *arrayName = this->ScalarMenu->GetValue();
    if (arrayName)
      {
      aInfo = pdInfo->GetArrayInformation(arrayName);
      if (aInfo)
        {
        double *range = aInfo->GetComponentRange(0);
        if (range[0]+range[1] != 0)
          {
          maxBnds /= (range[1] + range[0])*0.5;
          }
        }
      }
    break;
    }
    case 1:
    {
    const char *arrayName = this->VectorMenu->GetValue();
    if (arrayName)
      {
      aInfo = pdInfo->GetArrayInformation(arrayName);
      if (aInfo)
        {
        double *range0 = aInfo->GetComponentRange(0);
        double *range1 = aInfo->GetComponentRange(1);
        double *range2 = aInfo->GetComponentRange(2);
        double avg[3];
        avg[0] = (range0[0] + range0[1]) * 0.5;
        avg[1] = (range1[0] + range1[1]) * 0.5;
        avg[2] = (range2[0] + range2[1]) * 0.5;
        if (avg[0] != 0 || avg[1] != 0 || avg[2] != 0)
          {
          double mag = sqrt(avg[0]*avg[0] + avg[1]*avg[1] + avg[2]*avg[2]);
          maxBnds /= mag;
          }
        }
      }
    break;
    }
    case 2:
    {
    const char *arrayName = this->VectorMenu->GetValue();
    if (arrayName)
      {
      aInfo = pdInfo->GetArrayInformation(arrayName);
      if (aInfo)
        {
        double *range0 = aInfo->GetComponentRange(0);
        double *range1 = aInfo->GetComponentRange(1);
        double *range2 = aInfo->GetComponentRange(2);
        double avg[3];
        avg[0] = (range0[0] + range0[1]) * 0.5;
        avg[1] = (range1[0] + range1[1]) * 0.5;
        avg[2] = (range2[0] + range2[1]) * 0.5;
        if (avg[0] + avg[1] + avg[2] != 0)
          {
          maxBnds /= (avg[0] + avg[1] + avg[2])/3.0;
          }
        }
      }
    break;
    }
    default:
      break;
    }
  
  char value[1000];
  sprintf(value, "%f", maxBnds);
  this->SetValue(value);
}

void vtkPVGlyphScaleFactorEntry::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ScalarMenu: " << this->ScalarMenu << endl;
  os << indent << "VectorMenu: " << this->VectorMenu << endl;
  os << indent << "ScaleModeList: " << this->ScaleModeList << endl;
}
