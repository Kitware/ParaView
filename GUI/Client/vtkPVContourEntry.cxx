/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContourEntry.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVContourEntry.h"

#include "vtkContourValues.h"
#include "vtkKWListBox.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationInterfaceEntry.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVScalarRangeLabel.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVContourEntry);
vtkCxxRevisionMacro(vtkPVContourEntry, "1.53");

vtkCxxSetObjectMacro(vtkPVContourEntry, ArrayMenu, vtkPVArrayMenu);

//-----------------------------------------------------------------------------
int vtkPVContourEntryCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);


//-----------------------------------------------------------------------------
vtkPVContourEntry::vtkPVContourEntry()
{
  this->CommandFunction = vtkPVContourEntryCommand;

  this->ArrayMenu = NULL;
  
  this->DomainName = "scalar_range";
}

//-----------------------------------------------------------------------------
vtkPVContourEntry::~vtkPVContourEntry()
{
  this->SetArrayMenu(NULL);
}

//-----------------------------------------------------------------------------
int vtkPVContourEntry::ComputeWidgetRange()
{
  vtkSMProperty* prop = this->GetSMProperty();
  vtkSMDoubleRangeDomain* dom = 0;
  if (prop)
    {
    dom = vtkSMDoubleRangeDomain::SafeDownCast(prop->GetDomain(this->DomainName));
    }
  if (dom)
    {
    int exists;
    double rg = dom->GetMinimum(0, exists);
    if (exists)
      {
      this->WidgetRange[0] = rg;
      }
    rg = dom->GetMaximum(0, exists);
    if (exists)
      {
      this->WidgetRange[1] = rg;
      }
    this->UseWidgetRange = 1;
    return 1;
    }
  else
    {
    vtkErrorMacro(<< "Required domain " 
                  << this->DomainName 
                  << " could not be found.");
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::Accept()
{
  this->Superclass::Accept();

  vtkSMDoubleVectorProperty* prop = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (prop)
    {
    int numContours = this->ContourValues->GetNumberOfContours();
    prop->SetNumberOfElements(numContours);
    for(int i=0; i<numContours; i++)
      {
      prop->SetElement(i, this->ContourValues->GetValue(i));
      }
    }
  else
    {
    vtkErrorMacro(
      "Could not find property of name: "
      << (this->GetSMPropertyName()?this->GetSMPropertyName():"(null)")
      << " for widget: " << this->GetTraceName());
    }

  // The superclass (vtkPVValueList) uses Accept for moving value from
  // NewValueEntry to ContourValues if ContourValues has no contours, so
  // explicitly call vtkPVWidget::Accept() here.
  this->vtkPVWidget::Accept();
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::SaveInBatchScript(ofstream *file)
{
  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);

  vtkSMDoubleVectorProperty* prop = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (prop)
    {
    unsigned int numContours = prop->GetNumberOfElements();

    *file << "  [$pvTemp" << sourceID.ID << " GetProperty ContourValues] "
          << "SetNumberOfElements " << numContours << endl;
    for (unsigned int i = 0; i < numContours; i++)
      {
      *file << "  ";
      *file << "[$pvTemp" << sourceID.ID << " GetProperty ContourValues] "
            << "SetElement " << i << " " << prop->GetElement(i) << endl;
      }
    }
}

//-----------------------------------------------------------------------------
// If we had access to the ContourValues object of the filter,
// this would be much easier.  We would not have to rely on Tcl calls.
void vtkPVContourEntry::ResetInternal()
{
  // The widget has been modified.  
  // Now set the widget back to reflect the contours in the filter.
  this->ContourValuesList->DeleteAll();
  this->ContourValues->SetNumberOfContours(0);

  vtkSMDoubleVectorProperty* prop = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (prop)
    {
    unsigned int numContours = prop->GetNumberOfElements();

    for (unsigned int i = 0; i < numContours; i++)
      {
      this->AddValue(prop->GetElement(i));
      }
    }
  
  // Since the widget now matches the fitler, it is no longer modified.
  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai)
{
  if (ai->InitializeTrace(NULL))
    {
    this->AddTraceEntry("$kw(%s) AnimationMenuCallback $kw(%s)", 
                        this->GetTclName(), ai->GetTclName());
    }

  ai->SetLabelAndScript(this->GetTraceName(), NULL, this->GetTraceName());
  ai->SetAnimationElement(0);

  vtkSMProperty *prop = this->GetSMProperty();
  vtkSMDoubleRangeDomain *rangeDomain = vtkSMDoubleRangeDomain::SafeDownCast(
    prop->GetDomain(this->DomainName));
    

  if (!rangeDomain)
    {
    vtkErrorMacro("Required domain scalar_range could not be found");
    return;
    }

  ai->SetCurrentSMProperty(prop);
  ai->SetCurrentSMDomain(rangeDomain);

  int minExists, maxExists;
  double min = rangeDomain->GetMinimum(0, minExists);
  double max = rangeDomain->GetMaximum(0, maxExists);
  if (minExists && maxExists)
    {
    ai->SetTimeStart(min);
    ai->SetTimeEnd(max);
    }

  ai->Update();

}

//----------------------------------------------------------------------------
void vtkPVContourEntry::CopyProperties(
  vtkPVWidget* clone, 
  vtkPVSource* pvSource,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVContourEntry* pvce = vtkPVContourEntry::SafeDownCast(clone);
  if (pvce)
    {
    if (this->ArrayMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVArrayMenu* am = this->ArrayMenu->ClonePrototype(pvSource, map);
      pvce->SetArrayMenu(am);
      am->Delete();
      }
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVContourEntry.");
    }
}

//-----------------------------------------------------------------------------
int vtkPVContourEntry::ReadXMLAttributes(vtkPVXMLElement* element,
                                         vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the ArrayMenu.
  const char* array_menu = element->GetAttribute("array_menu");
  if(array_menu)
    {
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
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->ArrayMenu);
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ArrayMenu: " << this->GetArrayMenu() << endl;
}
