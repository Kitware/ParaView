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
#include "vtkPVData.h"
#include "vtkObjectFactory.h"

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
  this->ArrayName = NULL;

  this->PVSource = NULL;
  this->NumberOfComponents = 1;

  this->InputName = NULL;
  this->AttributeName = NULL;
  this->ObjectTclName = NULL;

  this->Label = vtkKWLabel::New();
  this->Menu = vtkKWOptionMenu::New();
}

//----------------------------------------------------------------------------
vtkPVArrayMenu::~vtkPVArrayMenu()
{
  this->SetArrayName(NULL);

  this->SetPVSource(NULL);

  this->SetInputName(NULL);
  this->SetAttributeName(NULL);
  this->SetObjectTclName(NULL);

  this->Label->Delete();
  this->Label = NULL;
  this->Menu->Delete();
  this->Menu = NULL;
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::SetLabel(const char* label)
{
  this->Label->SetLabel(label);
  this->SetName(label);
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

  this->Menu->SetParent(this);
  this->Menu->Create(app, "");
  this->Script("pack %s -side left", this->Menu->GetWidgetName());
}



//----------------------------------------------------------------------------
void vtkPVArrayMenu::MenuEntryCallback(const char* name)
{
  if (strcmp(name, this->ArrayName) == 0)
    {
    return;
    }

  this->SetArrayName(name);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::SetValue(const char* name)
{
  if (strcmp(name, this->ArrayName) == 0)
    {
    return;
    }

  this->Menu->SetValue(name);
  this->SetArrayName(name);
  this->ModifiedCallback();
}


//----------------------------------------------------------------------------
void vtkPVArrayMenu::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if ( ! this->ModifiedFlag)
    {
    return;
    }

  if (this->InputName == NULL || this->AttributeName == NULL 
          || this->ObjectTclName == NULL)
    {
    vtkErrorMacro("Access names have not all been set.");
    return;
    }

  if (this->ArrayName)
    {
    pvApp->BroadcastScript("%s Select%s%s %s", 
                           this->ObjectTclName,
                           this->InputName,
                           this->AttributeName,
                           this->ArrayName);
    pvApp->AddTraceEntry("$pv(%s) SetValue %s", 
                         this->GetTclName(), 
                         this->ArrayName);
    }
  else
    {
    pvApp->BroadcastScript("%s Select%s%s {}", 
                           this->ObjectTclName,
                           this->InputName,
                           this->AttributeName);
    pvApp->AddTraceEntry("$pv(%s) SetValue {}", this->GetTclName());
    }

  this->vtkPVWidget::Accept();
}

//----------------------------------------------------------------------------
void vtkPVArrayMenu::Reset()
{
  int i, num;
  vtkPVData *pvd;
  vtkDataSet *data;
  char methodAndArgs[1024];
  vtkDataArray *array;
  int arrayFound = 0;
  const char *first = NULL;

  if (this->InputName == NULL || this->AttributeName == NULL 
          || this->ObjectTclName == NULL)
    {
    vtkErrorMacro("Access names have not all been set.");
    return;
    }

  // Get the selected array form the VTK filter.
  this->Script("%s SetArrayName [%s Get%s%sSelection]",
               this->GetTclName(), 
               this->ObjectTclName,
               this->InputName,
               this->AttributeName);

  // Regenerate the menu, and look for the specified array.
  this->Menu->ClearEntries();
  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource has not been set.");
    return;
    }
  pvd = this->PVSource->GetPVInput();
  if (pvd == NULL)
    {
    vtkErrorMacro("Could not get the input of my source.");
    return;
    }
  data = pvd->GetVTKData();
  if (data == NULL)
    { // Lets be anal.
    vtkErrorMacro("Could not find vtk data.");
    return;
    }
  num = data->GetPointData()->GetNumberOfArrays();
  for (i = 0; i < num; ++i)
    {
    array = data->GetPointData()->GetArray(i);
    // It the array does not have a name, then we can do nothing with it.
    if (array->GetName())
      {
      // Match the requested number of componenets.
      if (this->NumberOfComponents <= 0 || 
          array->GetNumberOfComponents() == this->NumberOfComponents) 
        {
        sprintf(methodAndArgs, "MenuEntryCallback %s", array->GetName());
        this->Menu->AddEntryWithCommand(array->GetName(), 
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

  this->ModifiedFlag = 0;    

  // If the filter has not specified a valid array, then use the default attribute.
  if (arrayFound == 0)
    { // If the current value is not in the menu, then look for another to use.
    // First look for a default attribute.
    // What a pain !!!  Is using the PVSource is a good idea?
    this->SetArrayName(NULL);
    this->Script("catch {%s SetArrayName [[[%s GetPointData] Get%s] GetName]}",
                 this->GetTclName(), pvd->GetVTKDataTclName(), this->AttributeName);
    if (this->ArrayName == NULL || this->ArrayName[0] == '\0')
      { // lets just use the first in the menu.
      if (first)
        {
        this->SetArrayName(first);
        }
      else
        {
        vtkWarningMacro("Could not find " << this->AttributeName);
        // Here we may want to keep the previous value.
        this->SetArrayName(NULL);
        }
      }

      // In this case, the widget does not match the object.
      this->ModifiedFlag = 1;
    }

  // Now set the menu's value.
  this->Menu->SetValue(this->ArrayName);

}
