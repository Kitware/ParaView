/*=========================================================================

  Program:   ParaView
  Module:    vtkPVArrayMenu.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVArrayMenu.h"

#include "vtkArrayMap.txx"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkKWLabel.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVFieldMenu.h"
#include "vtkPVInputMenu.h"
#include "vtkPVProcessModule.h"
#include "vtkPVStringAndScalarListWidgetProperty.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMStringVectorProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVArrayMenu);
vtkCxxRevisionMacro(vtkPVArrayMenu, "1.58.2.2");

vtkCxxSetObjectMacro(vtkPVArrayMenu, InputMenu, vtkPVInputMenu);
vtkCxxSetObjectMacro(vtkPVArrayMenu, FieldMenu, vtkPVFieldMenu);

//----------------------------------------------------------------------------
vtkPVArrayMenu::vtkPVArrayMenu()
{
  this->ArrayName = NULL;

  this->InputName = NULL;
  this->AttributeType = 0;

  this->Label = vtkKWLabel::New();
  this->ArrayMenu = vtkKWOptionMenu::New();

  this->InputMenu = NULL;
  this->FieldMenu = NULL;

  this->Property = NULL;
}

//----------------------------------------------------------------------------
vtkPVArrayMenu::~vtkPVArrayMenu()
{
  this->SetArrayName(NULL);

  this->SetInputName(NULL);

  this->Label->Delete();
  this->Label = NULL;
  if (this->FieldMenu)
    {
    this->FieldMenu->Delete();
    this->FieldMenu = NULL;
    }
  if (this->ArrayMenu)
    {
    this->ArrayMenu->Delete();
    this->ArrayMenu = NULL;
    }

  this->SetInputMenu(NULL);
  this->SetFieldMenu(NULL);

  this->SetProperty(NULL);
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::SetLabel(const char* label)
{
  this->Label->SetLabel(label);
  if (label && label[0] &&
      (this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName(label);
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  vtkKWWidget *extraFrame;

  // Extra frame is needed because of the range label.
  extraFrame = vtkKWWidget::New();
  extraFrame->SetParent(this);
  extraFrame->Create(app, "frame", "");
  this->Script("pack %s -side top -fill x -expand t",
               extraFrame->GetWidgetName());

  this->Label->SetParent(extraFrame);
  this->Label->Create(app, "-width 18 -justify right");
  this->Script("pack %s -side left", this->Label->GetWidgetName());

  this->ArrayMenu->SetParent(extraFrame);
  this->ArrayMenu->Create(app, "");
  this->Script("pack %s -side left", this->ArrayMenu->GetWidgetName());

  extraFrame->Delete();
  extraFrame = NULL;
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::ArrayMenuEntryCallback(const char* name)
{
  if (this->ArrayName && strcmp(name, this->ArrayName) == 0)
    {
    return;
    }

  this->SetArrayName(name);
  this->ModifiedCallback();
  this->UpdateProperty();
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::SetValue(const char* name)
{
  if (this->ArrayName && strcmp(name, this->ArrayName) == 0)
    {
    return;
    }

  if (name)
    {
    ostrstream aname;
    aname << name;
    vtkSMProperty* property = this->GetSMProperty();
    if (property)
      {
      vtkSMArrayListDomain* dom = vtkSMArrayListDomain::SafeDownCast(
        property->GetDomain("array_list"));
      
      unsigned int numStrings = dom->GetNumberOfStrings();
      for (unsigned int i=0; i<numStrings; i++)
        {
        const char* arrayName = dom->GetString(i);
        if (strcmp(arrayName, this->ArrayName) == 0)
          {
          if (dom->IsArrayPartial(i))
            {
            aname << " (partial)"; 
            }
          break;
          }
        }
      }
    aname << ends;
    this->ArrayMenu->SetValue(aname.str());
    delete[] aname.str();
    }
  else
    {
    this->ArrayMenu->SetValue("None");
    }
  this->SetArrayName(name);
  this->ModifiedCallback();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::Accept()
{
  int modFlag = this->GetModifiedFlag();

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (svp)
    {
    svp->SetNumberOfElements(1);
    svp->SetElement(0, this->ArrayName);
    }
  else
    {
    vtkErrorMacro(
      "Could not find property of name: "
      << (this->GetSMPropertyName()?this->GetSMPropertyName():"(null)")
      << " for widget: " << this->GetTraceName());
    }

  this->ModifiedFlag = 0;

  // I put this after the accept internal, because
  // vtkPVGroupWidget inactivates and builds an input list ...
  // Putting this here simplifies subclasses AcceptInternal methods.
  if (modFlag)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    ofstream* file = pvApp->GetTraceFile();
    if (file)
      {
      this->Trace(file);
      }
    }

  this->AcceptCalled = 1;
}

//---------------------------------------------------------------------------
void vtkPVArrayMenu::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  if (this->ArrayName)
    {
    *file << "$kw(" << this->GetTclName() << ") SetValue {"
          << this->ArrayName << "}" << endl;
    }
  else
    {
    *file << "$kw(" << this->GetTclName() << ") SetValue {}\n";
    }

}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::ResetInternal()
{

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());

  if (svp)
    {
    // Get the selected array.
    this->SetValue(svp->GetElement(0));
    }

  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::SaveInBatchScript(ofstream *file)
{
  const char* attributeName;

  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);

  attributeName = vtkDataSetAttributes::GetAttributeTypeAsString(
    this->AttributeType);

  if (sourceID.ID == 0)
    {
    vtkErrorMacro("Sanity check failed. " 
                  << this->GetClassName());
    return;
    }

  ostrstream cmd_with_warning_C4701;
  cmd_with_warning_C4701 << "  [$pvTemp" << sourceID << " GetProperty "
                         << "Select" << this->InputName << attributeName
                         << "] SetElement 0 " ;
  if (this->ArrayName)
    {
    cmd_with_warning_C4701 << "{" << this->ArrayName << "}" << endl;
    }
  else
    {
    cmd_with_warning_C4701 << "{}"<< endl;
    }

  cmd_with_warning_C4701 << ends;
  *file << cmd_with_warning_C4701.str();
  delete[] cmd_with_warning_C4701.str();
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::UpdateProperty()
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (svp)
    {
    svp->SetUncheckedElement(0, this->ArrayName);
    svp->UpdateDependentDomains();
    }

}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::Update()
{
  vtkSMProperty* property = this->GetSMProperty();
  if (property)
    {
    vtkSMArrayListDomain* dom = vtkSMArrayListDomain::SafeDownCast(
      property->GetDomain("array_list"));
    unsigned int numStrings = dom->GetNumberOfStrings();

    int arrayFound = 0;

    for (unsigned int i=0; i<numStrings; i++)
      {
      const char* arrayName = dom->GetString(i);
      if (this->ArrayName && strcmp(this->ArrayName, arrayName) == 0)
        {
        arrayFound = 1;
        }
      }
    if (arrayFound == 0)
      {
      if (dom->GetNumberOfStrings() > 0)
        {
        const char* arrayName = dom->GetString(dom->GetDefaultElement());
        this->SetArrayName(arrayName);
        }
      else
        {
        this->SetArrayName(NULL);
        }
      }
    }
  this->UpdateProperty();
  this->UpdateArrayMenu();
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::UpdateArrayMenu()
{
  char methodAndArgs[1024];

  // Regenerate the menu, and look for the specified array.
  this->ArrayMenu->ClearEntries();

  vtkSMProperty* property = this->GetSMProperty();
  if (property)
    {
    vtkSMArrayListDomain* dom = vtkSMArrayListDomain::SafeDownCast(
      property->GetDomain("array_list"));
    unsigned int numStrings = dom->GetNumberOfStrings();
    
    for (unsigned int i=0; i<numStrings; i++)
      {
      ostrstream aname;
      const char* arrayName = dom->GetString(i);
      aname << arrayName;
      if (dom->IsArrayPartial(i))
        {
        aname << " (partial)"; 
        }
      aname << ends;
      sprintf(methodAndArgs, "ArrayMenuEntryCallback {%s}", arrayName);
      this->ArrayMenu->AddEntryWithCommand(aname.str(), this, methodAndArgs);
      delete[] aname.str();
      }

    if (this->ArrayName)
      {
      ostrstream aname;

      aname << this->ArrayName;

      unsigned int numStrings = dom->GetNumberOfStrings();
      for (unsigned int i=0; i<numStrings; i++)
        {
        const char* arrayName = dom->GetString(i);
        if (strcmp(arrayName, this->ArrayName) == 0)
          {
          if (dom->IsArrayPartial(i))
            {
            aname << " (partial)"; 
            }
          break;
          }
        }
      aname << ends;
      this->ArrayMenu->SetValue(aname.str());
      delete[] aname.str();
      }
    else
      {
      this->ArrayMenu->SetValue("None");
      }
    }
  else
    {
    this->ArrayMenu->SetValue("None");
    }
}

//----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation *vtkPVArrayMenu::GetFieldInformation()
{

  if (this->ArrayMenu)
    {
    vtkPVSource *input;
    if ( !this->InputMenu )
      {
      return NULL;
      }
    input = this->InputMenu->GetCurrentValue();
    if (input == NULL)
      {
      return NULL;
      }
    // TODO Fix
    return input->GetDataInformation()->GetPointDataInformation();
    }

  vtkErrorMacro("No input menu or field menu.");
  return NULL;
}

//----------------------------------------------------------------------------
vtkPVArrayInformation *vtkPVArrayMenu::GetArrayInformation()
{
  vtkPVDataSetAttributesInformation* fieldInfo;

  fieldInfo = this->GetFieldInformation();
  if (fieldInfo == NULL)
    {
    return NULL;
    }

  return fieldInfo->GetArrayInformation(this->ArrayName);
}

//----------------------------------------------------------------------------
vtkPVArrayMenu* vtkPVArrayMenu::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVArrayMenu::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVArrayMenu::ClonePrototypeInternal(vtkPVSource* pvSource,
                                vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* pvWidget = 0;
  // Check if a clone of this widget has already been created
  if ( map->GetItem(this, pvWidget) != VTK_OK )
    {
    // If not, create one and add it to the map
    pvWidget = this->NewInstance();
    map->SetItem(this, pvWidget);
    // Now copy all the properties
    this->CopyProperties(pvWidget, pvSource, map);

    vtkPVArrayMenu* pvArrayMenu = vtkPVArrayMenu::SafeDownCast(pvWidget);
    if (!pvArrayMenu)
      {
      vtkErrorMacro("Internal error. Could not downcast pointer.");
      pvWidget->Delete();
      return 0;
      }
    }
  else
    {
    // Increment the reference count. This is necessary
    // to make the behavior same whether a widget is created
    // or returned from the map. Always call Delete() after
    // cloning.
    pvWidget->Register(this);
    }

  // note pvSelect == pvWidget
  return pvWidget;
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVArrayMenu* pvam = vtkPVArrayMenu::SafeDownCast(clone);
  if (pvam)
    {
    pvam->SetInputName(this->InputName);
    pvam->SetAttributeType(this->AttributeType);
    pvam->SetLabel(this->Label->GetLabel());
    if (this->InputMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVInputMenu* im = this->InputMenu->ClonePrototype(pvSource, map);
      pvam->SetInputMenu(im);
      im->Delete();
      }
    if (this->FieldMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVFieldMenu* im = this->FieldMenu->ClonePrototype(pvSource, map);
      pvam->SetFieldMenu(im);
      im->Delete();
      }
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVArrayMenu.");
    }
}

//----------------------------------------------------------------------------
int vtkPVArrayMenu::ReadXMLAttributes(vtkPVXMLElement* element,
                                      vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(!label)
    {
    vtkErrorMacro("No label attribute.");
    return 0;
    }
  this->Label->SetLabel(label);
  
  // Setup the InputMenu.
  const char* input_menu = element->GetAttribute("input_menu");
  if (input_menu)
    {
    vtkPVXMLElement* ime = element->LookupElement(input_menu);
    if (!ime)
      {
      vtkErrorMacro("Couldn't find InputMenu element " << input_menu);
      return 0;
      }
    vtkPVWidget* w = this->GetPVWidgetFromParser(ime, parser);
    vtkPVInputMenu* imw = vtkPVInputMenu::SafeDownCast(w);
    if(!imw)
      {
      if(w) { w->Delete(); }
      vtkErrorMacro("Couldn't get InputMenu widget " << input_menu);
      return 0;
      }
    imw->AddDependent(this);
    this->SetInputMenu(imw);
    imw->Delete();
    }

  // Setup the FieldMenu.
  const char* field_menu = element->GetAttribute("field_menu");
  if (field_menu)
    {
    vtkPVXMLElement* ime = element->LookupElement(field_menu);
    vtkPVWidget* w = this->GetPVWidgetFromParser(ime, parser);
    vtkPVFieldMenu* imw = vtkPVFieldMenu::SafeDownCast(w);
    if(!imw)
      {
      if(w) { w->Delete(); }
      vtkErrorMacro("Couldn't get FieldMenu widget " << field_menu);
      return 0;
      }
    imw->AddDependent(this);
    this->SetFieldMenu(imw);
    imw->Delete();
    }
  
  // Setup the InputName.
  const char* input_name = element->GetAttribute("input_name");
  if(input_name)
    {
    this->SetInputName(input_name);
    }
  else
    {
    this->SetInputName("Input");
    }
  
  // Search for attribute type with matching name.
  const char* attribute_type = element->GetAttribute("attribute_type");
  unsigned int i = vtkDataSetAttributes::NUM_ATTRIBUTES;
  if(attribute_type)
    {
    for(i=0;i < vtkDataSetAttributes::NUM_ATTRIBUTES;++i)
      {
      if(strcmp(vtkDataSetAttributes::GetAttributeTypeAsString(i),
                attribute_type) == 0)
        {
        this->SetAttributeType(i);
        break;
        }
      }
    }
  if(i == vtkDataSetAttributes::NUM_ATTRIBUTES)
    {
    this->SetAttributeType(vtkDataSetAttributes::SCALARS);
    }
  
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkPVArrayMenu::GetLabel() 
{
  return this->Label->GetLabel();
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->InputMenu);
  this->PropagateEnableState(this->FieldMenu);
  this->PropagateEnableState(this->Label);
  this->PropagateEnableState(this->ArrayMenu);
}
//----------------------------------------------------------------------------
void vtkPVArrayMenu::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ArrayName: " << (this->ArrayName?this->ArrayName:"none") 
     << endl;
  os << indent << "AttributeType: " << this->GetAttributeType() << endl;
  os << indent << "InputName: " << (this->InputName?this->InputName:"none") 
     << endl;
  if (this->InputMenu)
    {
    os << indent << "InputMenu: " << this->InputMenu << endl;
    }
  else
    {
    os << indent << "InputMenu: NULL\n";
    }
  if (this->FieldMenu)
    {
    os << indent << "FieldMenu: " << this->FieldMenu << endl;
    }
  else
    {
    os << indent << "FieldMenu: NULL\n";
    }
}
