/*=========================================================================

  Program:   ParaView
  Module:    vtkPVArrayMenu.cxx
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

#include "vtkPVArrayMenu.h"
#include "vtkObjectFactory.h"
#include "vtkKWMessageDialog.h"

//----------------------------------------------------------------------------
vtkPVArrayMenu* vtkPVArrayMenu::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVArrayMenu");
  if(ret)
    {
    return (vtkPVArrayMenu*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVArrayMenu;
}

//----------------------------------------------------------------------------
vtkPVArrayMenu::vtkPVArrayMenu()
{
  this->FieldSelection = vtkDataSet::POINT_DATA_FIELD;
  this->ShowFieldMenu = 0;

  this->ArrayName = NULL;
  this->ArrayNumberOfComponents = 1;
  this->SelectedComponent = 0;

  this->NumberOfComponents = 1;
  this->ShowComponentMenu = 0;

  this->InputName = NULL;
  this->AttributeType = 0;
  this->ObjectTclName = NULL;

  this->Label = vtkKWLabel::New();
  this->FieldMenu = vtkKWOptionMenu::New();
  this->ArrayMenu = vtkKWOptionMenu::New();
  this->ComponentMenu = vtkKWOptionMenu::New();

  this->DataSetCommandObjectTclName = NULL;
  this->DataSetCommandMethod = NULL;
  this->DataSet = NULL;
}

//----------------------------------------------------------------------------
vtkPVArrayMenu::~vtkPVArrayMenu()
{
  this->SetArrayName(NULL);

  this->SetInputName(NULL);
  this->SetObjectTclName(NULL);

  this->Label->Delete();
  this->Label = NULL;
  this->FieldMenu->Delete();
  this->FieldMenu = NULL;
  this->ArrayMenu->Delete();
  this->ArrayMenu = NULL;
  this->ComponentMenu->Delete();
  this->ComponentMenu = NULL;

  this->SetDataSetCommandObjectTclName(NULL);
  this->SetDataSetCommandMethod(NULL);
  this->SetDataSet(NULL);
}


//----------------------------------------------------------------------------
void vtkPVArrayMenu::SetDataSetCommand(const char* objTclName, 
                                       const char* method)
{
  this->SetDataSetCommandObjectTclName(objTclName);
  this->SetDataSetCommandMethod(method);
}


//----------------------------------------------------------------------------
void vtkPVArrayMenu::SetLabel(const char* label)
{
  this->Label->SetLabel(label);
  this->SetTraceName(label);
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::SetFieldSelection(int field)
{
  if (this->FieldSelection == field)
    {
    return;
    }
  this->FieldSelection = field;
  this->ModifiedCallback();

  if (this->Application == NULL)
    {
    return;
    }

  switch (field)
    {
    case vtkDataSet::DATA_OBJECT_FIELD:
      vtkErrorMacro("We do not handle data object fields yet.");
      return;
    case vtkDataSet::POINT_DATA_FIELD:
      this->FieldMenu->SetValue("Point");
      break;
    case vtkDataSet::CELL_DATA_FIELD:
      this->FieldMenu->SetValue("Cell");
      break;
    default:
      vtkErrorMacro("Unknown field.");
      return;
    }

  this->UpdateArrayMenu();
}


//----------------------------------------------------------------------------
void vtkPVArrayMenu::SetShowFieldMenu(int flag)
{
  if (this->ShowFieldMenu == flag)
    {
    return;
    }
  this->Modified();

  this->ShowFieldMenu = flag;

  if (this->Application)
    {
    if (flag)
      {
      this->Script("pack %s -after %s -side left", 
                   this->FieldMenu->GetWidgetName(),
                   this->Label->GetWidgetName());
      }
    else
      {
      this->Script("pack forget %s", this->FieldMenu->GetWidgetName()); 
      }
    }
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
  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    return;
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());

  this->Label->SetParent(this);
  this->Label->Create(app, "-width 18 -justify right");
  this->Script("pack %s -side left", this->Label->GetWidgetName());

  this->FieldMenu->SetParent(this);
  this->FieldMenu->Create(app, "");
  // Easier to hard code the field enum 
  // than to sprintf into a method and arg string
  this->FieldMenu->AddEntryWithCommand("Point", this, "SetFieldSelection 1");
  this->FieldMenu->AddEntryWithCommand("Cell", this, "SetFieldSelection 2");
  switch (this->FieldSelection)
    {
    case vtkDataSet::DATA_OBJECT_FIELD:
      vtkErrorMacro("We do not handle data object fields yet.");
      return;
    case vtkDataSet::POINT_DATA_FIELD:
      this->FieldMenu->SetValue("Point");
      break;
    case vtkDataSet::CELL_DATA_FIELD:
      this->FieldMenu->SetValue("Cell");
      break;
    default:
      vtkErrorMacro("Unknown field.");
      return;
    }

  if (this->ShowFieldMenu)
    {
    this->Script("pack %s -side left", this->FieldMenu->GetWidgetName());
    }

  this->ArrayMenu->SetParent(this);
  this->ArrayMenu->Create(app, "");
  this->Script("pack %s -side left", this->ArrayMenu->GetWidgetName());

  this->ComponentMenu->SetParent(this);
  this->ComponentMenu->Create(app, "");
  if (this->ShowComponentMenu)
    {
    this->Script("pack %s -side left", this->ComponentMenu->GetWidgetName());
    }
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
}


//----------------------------------------------------------------------------
void vtkPVArrayMenu::SetValue(const char* name)
{
  if (strcmp(name, this->ArrayName) == 0)
    {
    return;
    }

  this->ArrayMenu->SetValue(name);
  this->SetArrayName(name);
  this->UpdateComponentMenu();
  this->ModifiedCallback();
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
}





//----------------------------------------------------------------------------
void vtkPVArrayMenu::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  const char* attributeName;

  attributeName = vtkDataSetAttributes::GetAttributeTypeAsString(this->AttributeType);
  if (attributeName == NULL)
    {
    vtkErrorMacro("Could not find attribute name.");
    return;
    }

  if ( ! this->ModifiedFlag)
    {
    return;
    }

  if (this->InputName == NULL || this->ObjectTclName == NULL)
    {
    vtkErrorMacro("Access names have not all been set.");
    return;
    }

  if (this->ArrayName)
    {
    pvApp->BroadcastScript("%s Select%s%s %s", 
                           this->ObjectTclName,
                           this->InputName,
                           attributeName,
                           this->ArrayName);
    this->AddTraceEntry("$kw(%s) SetValue %s", 
                         this->GetTclName(), 
                         this->ArrayName);
    }
  else
    {
    pvApp->BroadcastScript("%s Select%s%s {}", 
                           this->ObjectTclName,
                           this->InputName,
                           attributeName);
    this->AddTraceEntry("$kw(%s) SetValue {}", this->GetTclName());
    }

  if (this->ShowComponentMenu)
    {
    pvApp->BroadcastScript("%s Select%s%sComponent %d", 
                           this->ObjectTclName,
                           this->InputName,
                           attributeName,
                           this->SelectedComponent);
    this->AddTraceEntry("$kw(%s) SetSelectedComponent %s", 
                         this->GetTclName(), 
                         this->ArrayName);
    }


  this->vtkPVWidget::Accept();
}


//----------------------------------------------------------------------------
void vtkPVArrayMenu::Reset()
{
  const char* attributeName;

  attributeName = vtkDataSetAttributes::GetAttributeTypeAsString(this->AttributeType);
  if (attributeName == NULL)
    {
    vtkErrorMacro("Could not find attribute name.");
    return;
    }

  if (this->InputName == NULL || this->ObjectTclName == NULL)
    {
    vtkErrorMacro("Access names have not all been set.");
    return;
    }

  // Get the selected array form the VTK filter.
  this->Script("%s SetArrayName [%s Get%s%sSelection]",
               this->GetTclName(), 
               this->ObjectTclName,
               this->InputName,
               attributeName);

  // Get the selected array form the VTK filter.
  if (this->ShowComponentMenu)
    {
    this->Script("%s SetSelectedComponent [%s Get%s%sComponentSelection]",
                 this->GetTclName(), 
                 this->ObjectTclName,
                 this->InputName,
                 attributeName);
    }

  this->ModifiedFlag = 0;

  this->UpdateArrayMenu();

}





//----------------------------------------------------------------------------
void vtkPVArrayMenu::UpdateArrayMenu()
{
  int i, num;
  char methodAndArgs[1024];
  vtkDataArray *array;
  int arrayFound = 0;
  const char *first = NULL;
  const char* attributeName;
  vtkDataSetAttributes *field;

  attributeName = vtkDataSetAttributes::GetAttributeTypeAsString(this->AttributeType);
  if (attributeName == NULL)
    {
    vtkErrorMacro("Could not find attribute name.");
    return;
    }

  // Regenerate the menu, and look for the specified array.
  this->ArrayMenu->ClearEntries();

  // We have to get the data set incase an input has changed. 
  if (this->DataSetCommandMethod == NULL || this->DataSetCommandObjectTclName == NULL)
    {
    vtkErrorMacro("DataSetCommand has not been set.")
    return;
    }
  this->Script("%s SetDataSet [%s %s]", this->GetTclName(),
               this->DataSetCommandObjectTclName, this->DataSetCommandMethod);
  if (this->DataSet == NULL)
    {
    this->SetArrayName(NULL);
    this->ArrayMenu->SetValue("None");
    vtkDebugMacro("Could not find vtk data set.");
    return;
    }

  switch (this->FieldSelection)
    {
    case vtkDataSet::POINT_DATA_FIELD:
      field = this->DataSet->GetPointData();
      break;
    case vtkDataSet::CELL_DATA_FIELD:
      field = this->DataSet->GetCellData();
      break;
    default:
      vtkErrorMacro("Unknown field type.");
      return;
    }

  num = field->GetNumberOfArrays();
  for (i = 0; i < num; ++i)
    {
    array = field->GetArray(i);
    // It the array does not have a name, then we can do nothing with it.
    if (array->GetName())
      {
      // Match the requested number of componenets.
      if (this->NumberOfComponents <= 0 || this->ShowComponentMenu ||
          array->GetNumberOfComponents() == this->NumberOfComponents) 
        {
        sprintf(methodAndArgs, "ArrayMenuEntryCallback %s", array->GetName());
        this->ArrayMenu->AddEntryWithCommand(array->GetName(), 
                                      this, methodAndArgs);
        if (first == NULL)
          {
          first = array->GetName();
          }
        if (this->ArrayName && strcmp(this->ArrayName, array->GetName()) == 0)
          {
          arrayFound = 1;
          }
        }
      }
    }

  // If the filter has not specified a valid array, then use the default attribute.
  if (arrayFound == 0)
    { // If the current value is not in the menu, then look for another to use.
    // First look for a default attribute.
    array = field->GetAttribute(this->AttributeType); 
    if (array == NULL)
      { // lets just use the first in the menu.
      if (first)
        {
        array = field->GetArray(first); 
        }
      else
        {
        // Here we may want to keep the previous value.
        this->SetArrayName(NULL);
        this->ArrayMenu->SetValue("None");
        }
      }

    // In this case, the widget does not match the object.
    this->ModifiedFlag = 1;

    if (array)
      {
      this->SetArrayName(array->GetName());
      }

    // Now set the menu's value.
    this->ArrayMenu->SetValue(this->ArrayName);
    }

  this->UpdateComponentMenu();
}




//----------------------------------------------------------------------------
void vtkPVArrayMenu::UpdateComponentMenu()
{
  int i;
  char methodAndArgs[1024];
  char label[124];
  vtkDataArray *array;
  int currentComponent;
  vtkDataSetAttributes *field;

  if (this->Application == NULL)
    {
    this->SelectedComponent = 0;
    return;
    }

  this->Script("pack forget %s", this->ComponentMenu->GetWidgetName()); 
  currentComponent = this->SelectedComponent;
  this->ArrayNumberOfComponents = 1;
  this->SelectedComponent = 0;

  // Make sure the data set is the latest.
  if (this->DataSetCommandMethod == NULL || this->DataSetCommandObjectTclName == NULL)
    {
    vtkErrorMacro("DataSetCommand has not been set.")
    return;
    }
  this->Script("%s SetDataSet [%s %s]", this->GetTclName(),
               this->DataSetCommandObjectTclName, this->DataSetCommandMethod);
  if (this->DataSet == NULL)
    {
    vtkErrorMacro("Could not find vtk data set.");
    return;
    }

  // Point or cell data.
  switch (this->FieldSelection)
    {
    case vtkDataSet::POINT_DATA_FIELD:
      field = this->DataSet->GetPointData();
      break;
    case vtkDataSet::CELL_DATA_FIELD:
      field = this->DataSet->GetCellData();
      break;
    default:
      vtkErrorMacro("Unknown field type.");
      return;
    }

  array = field->GetArray(this->ArrayName);
  if (array == NULL)
    {
    return;
    }
  this->ArrayNumberOfComponents = array->GetNumberOfComponents();

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
