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

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVArrayMenu);
vtkCxxRevisionMacro(vtkPVArrayMenu, "1.52");

vtkCxxSetObjectMacro(vtkPVArrayMenu, InputMenu, vtkPVInputMenu);
vtkCxxSetObjectMacro(vtkPVArrayMenu, FieldMenu, vtkPVFieldMenu);

//----------------------------------------------------------------------------
vtkPVArrayMenu::vtkPVArrayMenu()
{
  this->FieldSelection = vtkDataSet::POINT_DATA_FIELD;

  this->ArrayName = NULL;
  this->ArrayNumberOfComponents = 1;
  this->SelectedComponent = 0;

  this->NumberOfComponents = 1;
  this->ShowComponentMenu = 0;

  this->InputName = NULL;
  this->AttributeType = 0;

  this->Label = vtkKWLabel::New();
  this->ArrayMenu = vtkKWOptionMenu::New();
  this->ComponentMenu = vtkKWOptionMenu::New();

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

  this->ComponentMenu->Delete();
  this->ComponentMenu = NULL;

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
void vtkPVArrayMenu::SetFieldSelection(int field)
{
  if (this->FieldSelection == field)
    {
    return;
    }
  this->FieldSelection = field;

  if (this->Application == NULL)
    {
    return;
    }
  this->ModifiedCallback();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::SetNumberOfComponents(int num)
{
  if (this->NumberOfComponents == num)
    {
    return;
    }
  this->Modified();

  this->NumberOfComponents = num;
  if (num != 1)
    {
    this->ShowComponentMenu = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::SetShowComponentMenu(int flag)
{
  if (this->ShowComponentMenu == flag)
    {
    return;
    }
  this->Modified();

  this->ShowComponentMenu = flag;
  if (flag)
    {
    this->NumberOfComponents = 1;
    }
  this->UpdateComponentMenu();
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::Create(vtkKWApplication *app)
{
  vtkKWWidget *extraFrame;

  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    return;
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());

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

  this->ComponentMenu->SetParent(extraFrame);
  this->ComponentMenu->Create(app, "");
  if (this->ShowComponentMenu)
    {
    this->Script("pack %s -side left", this->ComponentMenu->GetWidgetName());
    }

  extraFrame->Delete();
  extraFrame = NULL;
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::ArrayMenuEntryCallback(const char* name)
{
  if (strcmp(name, this->ArrayName) == 0)
    {
    return;
    }

  this->SetArrayName(name);
  this->UpdateComponentMenu();
  this->ModifiedCallback();
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::ComponentMenuEntryCallback(int comp)
{
  if (comp == this->SelectedComponent)
    {
    return;
    }

  this->SelectedComponent = comp;
  this->ModifiedCallback();
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::SetValue(const char* name)
{
  if (this->ArrayName && strcmp(name, this->ArrayName) == 0)
    {
    return;
    }

  this->ArrayMenu->SetValue(name);
  this->SetArrayName(name);
  this->ModifiedCallback();
  this->Update();
}

//----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation *vtkPVArrayMenu::GetFieldInformation()
{

  if (this->FieldMenu)
    {
    return this->FieldMenu->GetFieldInformation();
    }
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
    switch (this->FieldSelection)
      {
      case vtkDataSet::DATA_OBJECT_FIELD:
        vtkErrorMacro("We do not handle data object fields yet.");
        return NULL;
      case vtkDataSet::POINT_DATA_FIELD:
        return input->GetDataInformation()->GetPointDataInformation();
        break;
      case vtkDataSet::CELL_DATA_FIELD:
        return input->GetDataInformation()->GetCellDataInformation();
        break;
      }
    vtkErrorMacro("Unknown field.");
    return NULL; 
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
void vtkPVArrayMenu::SetSelectedComponent(int comp)
{
  char label[128];

  if (comp == this->SelectedComponent)
    {
    return;
    }
  sprintf(label, "%d", comp);
  this->ComponentMenu->SetValue(label);
  this->SelectedComponent = comp;
  this->ModifiedCallback();
  this->vtkPVWidget::Update();
} 

//----------------------------------------------------------------------------
void vtkPVArrayMenu::AcceptInternal(vtkClientServerID sourceID)
{
  const char* attributeName;

  attributeName = vtkDataSetAttributes::GetAttributeTypeAsString(
    this->AttributeType);
  if (attributeName == NULL)
    {
    //vtkErrorMacro("Could not find attribute name.");
    return;
    }

  if (this->InputName == NULL || sourceID.ID == 0)
    {
    vtkDebugMacro("Access names have not all been set.");
    return;
    }

  if (this->ArrayName)
    {
    int numScalars = this->ShowComponentMenu;
    int numStrings = 1;
    char* cmd = new char[strlen(this->InputName)+strlen(attributeName)+7];
    sprintf(cmd, "Select%s%s", this->InputName, attributeName);
    this->Property->SetVTKCommands(1, &cmd, &numStrings, &numScalars);
    this->Property->SetStrings(1, &this->ArrayName);
    delete [] cmd;
    }
  else
    {
    this->Property->SetStrings(0, NULL);
    }

  if (this->ShowComponentMenu)
    {
    float scalar = this->SelectedComponent;
    this->Property->SetScalars(1, &scalar);
    }

  this->Property->SetVTKSourceID(sourceID);
  this->Property->AcceptInternal();
  
  this->ModifiedFlag = 0;
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

  if (this->ShowComponentMenu)
    {
    *file << "$kw(" << this->GetTclName() << ") "
          << "SetSelectedComponent " << this->SelectedComponent << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::ResetInternal()
{
  const char* attributeName;

  attributeName = vtkDataSetAttributes::GetAttributeTypeAsString(
    this->AttributeType);
  if (attributeName == NULL)
    {
    vtkErrorMacro("Could not find attribute name.");
    return;
    }

  if (this->InputName == NULL || this->Property->GetString(0) == NULL)
    {
    vtkDebugMacro("Access names have not all been set.");
    return;
    }

  // Get the selected array.
  this->SetValue(this->Property->GetString(0));

  // Get the selected array form the VTK filter.
  if (this->ShowComponentMenu)
    {
    this->SetSelectedComponent(static_cast<int>(this->Property->GetScalar(0)));
    }

  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::SaveInBatchScriptForPart(ofstream *file,
                                              vtkClientServerID sourceID)
{
  const char* attributeName;

  attributeName = vtkDataSetAttributes::GetAttributeTypeAsString(
    this->AttributeType);

  if (sourceID.ID == 0)
    {
    vtkErrorMacro("Sanity check failed. " 
                  << this->GetClassName());
    return;
    }

  if (this->ArrayName)
    {
    *file << "\t";
    *file << "pvTemp" << sourceID << " Select" << this->InputName
          << attributeName << " {" << this->ArrayName << "}\n";
    }
  else
    {
    *file << "\t";
    *file << "pvTemp" << sourceID << " Select" << this->InputName
          << attributeName << " {}\n";
    }
  if (this->ShowComponentMenu)
    {
    *file << "\t";
    *file << "pvTemp" << sourceID << " Select" << this->InputName
          << attributeName << "Component "  << this->SelectedComponent << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::Update()
{
  this->UpdateArrayMenu();
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::UpdateArrayMenu()
{
  int i, num;
  char methodAndArgs[1024];
  vtkPVDataSetAttributesInformation *attrInfo;
  vtkPVArrayInformation *ai;
  int arrayFound = 0;
  const char *first = NULL;
  const char* attributeName;

  attributeName = 
    vtkDataSetAttributes::GetAttributeTypeAsString(this->AttributeType);
  if (attributeName == NULL)
    {
    vtkErrorMacro("Could not find attribute name.");
    return;
    }

  // Regenerate the menu, and look for the specified array.
  this->ArrayMenu->ClearEntries();

  attrInfo = this->GetFieldInformation();
  if (attrInfo == NULL)
    {
    this->ArrayMenu->SetValue("None");
    return;
    }

  num = attrInfo->GetNumberOfArrays();
  for (i = 0; i < num; ++i)
    {
    ai = attrInfo->GetArrayInformation(i);
    // If the array does not have a name, then we can do nothing with it.
    if (ai->GetName())
      {
      // Match the requested number of componenets.
      if (this->NumberOfComponents <= 0 || this->ShowComponentMenu ||
          ai->GetNumberOfComponents() == this->NumberOfComponents) 
        {
        sprintf(methodAndArgs, "ArrayMenuEntryCallback {%s}", ai->GetName());
        this->ArrayMenu->AddEntryWithCommand(ai->GetName(), 
                                      this, methodAndArgs);
        if (first == NULL)
          {
          first = ai->GetName();
          }
        if (this->ArrayName && strcmp(this->ArrayName, ai->GetName()) == 0)
          {
          arrayFound = 1;
          }
        }
      }
    }

  // If the filter has not specified a valid array, then use the default
  // attribute.
  if (arrayFound == 0)
    { // If the current value is not in the menu, then look for another to use.
    // First look for a default attribute.
    ai = attrInfo->GetAttributeInformation(this->AttributeType);
    if (ai == NULL || ai->GetName() == NULL)
      { // lets just use the first in the menu.
      if (first)
        {
        ai = attrInfo->GetArrayInformation(first);
        }
      else
        {
        // Here we may want to keep the previous value.
        this->SetArrayName(NULL);
        this->ArrayMenu->SetValue("None");
        }
      }

    if (ai)
      {
      this->SetArrayName(ai->GetName());

      // In this case, the widget does not match the object.
      this->ModifiedCallback();
      }
    // Now set the menu's value.
    this->ArrayMenu->SetValue(this->ArrayName);

    if (!this->AcceptCalled && this->ArrayName)
      {
      char *str = new char[strlen(this->ArrayName)+1];
      strcpy(str, this->ArrayName);
      this->Property->SetStrings(1, &str);
      delete [] str;
      }
    }

  this->UpdateComponentMenu();
}


//----------------------------------------------------------------------------
void vtkPVArrayMenu::UpdateComponentMenu()
{
  int i;
  char methodAndArgs[1024];
  char label[124];
  vtkPVArrayInformation *ai;
  int currentComponent;

  if (this->Application == NULL)
    {
    this->SelectedComponent = 0;
    return;
    }

  this->Script("pack forget %s", this->ComponentMenu->GetWidgetName()); 
  currentComponent = this->SelectedComponent;
  this->ArrayNumberOfComponents = 1;
  this->SelectedComponent = 0;

  ai = this->GetArrayInformation();
  if (ai == NULL)
    {
    this->SelectedComponent = 0;
    return;
    }

  this->ArrayNumberOfComponents = ai->GetNumberOfComponents();

  if ( ! this->ShowComponentMenu || this->ArrayNumberOfComponents == 1)
    {
    return;
    }

  if (currentComponent < 0 || currentComponent >= this->ArrayNumberOfComponents)
    {
    currentComponent = 0;
    this->ModifiedCallback();
    }
  this->SelectedComponent = currentComponent;
  this->Script("pack %s -side left", this->ComponentMenu->GetWidgetName());

  // Clear current values.
  this->ComponentMenu->ClearEntries();

  // Regenerate the menu.
  for (i = 0; i < this->ArrayNumberOfComponents; ++i)
    {
    sprintf(label, "%d", i);
    sprintf(methodAndArgs, "ComponentMenuEntryCallback %d", i);
    this->ComponentMenu->AddEntryWithCommand(label, this, methodAndArgs);
    }

  // Now set the menu's value.
  sprintf(label, "%d", this->SelectedComponent);
  this->ComponentMenu->SetValue(label);
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
    pvam->SetNumberOfComponents(this->NumberOfComponents);
    pvam->SetInputName(this->InputName);
    pvam->SetFieldSelection(this->FieldSelection);
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
  
  // Setup the NumberOfComponents.
  if(!element->GetScalarAttribute("number_of_components",
                                  &this->NumberOfComponents))
    {
    this->NumberOfComponents = 1;
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
  
  // Search for field selection with matching name.
  const char* field_selection = element->GetAttribute("field_selection");
  if(field_selection)
    {
    if(strcmp("PointData", field_selection) == 0)
      {
      this->SetFieldSelection(vtkDataSet::POINT_DATA_FIELD);
      }
    else if(strcmp("CellData", field_selection) == 0)
      {
      this->SetFieldSelection(vtkDataSet::CELL_DATA_FIELD);
      }
    else
      {
      vtkErrorMacro("Unknown field selection.");
      this->SetFieldSelection(vtkDataSet::POINT_DATA_FIELD);
      }
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
void vtkPVArrayMenu::SetProperty(vtkPVWidgetProperty *prop)
{
  this->Property = vtkPVStringAndScalarListWidgetProperty::SafeDownCast(prop);
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVArrayMenu::GetProperty()
{
  return this->Property;
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVArrayMenu::CreateAppropriateProperty()
{
  return vtkPVStringAndScalarListWidgetProperty::New();
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->InputMenu);
  this->PropagateEnableState(this->FieldMenu);
  this->PropagateEnableState(this->Label);
  this->PropagateEnableState(this->ArrayMenu);
  this->PropagateEnableState(this->ComponentMenu);
}
//----------------------------------------------------------------------------
void vtkPVArrayMenu::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ArrayName: " << (this->ArrayName?this->ArrayName:"none") 
     << endl;
  os << indent << "ArrayNumberOfComponents: " << this->ArrayNumberOfComponents
     << endl;
  os << indent << "AttributeType: " << this->GetAttributeType() << endl;
  os << indent << "InputName: " << (this->InputName?this->InputName:"none") 
     << endl;
  os << indent << "NumberOfComponents: " << this->GetNumberOfComponents() << endl;
  os << indent << "SelectedComponent: " << this->GetSelectedComponent() << endl;
  os << indent << "ShowComponentMenu: " << this->GetShowComponentMenu() << endl;
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
