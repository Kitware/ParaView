/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVArraySelection.cxx
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
#include "vtkPVArraySelection.h"
#include "vtkKWRadioButton.h"
#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"
#include "vtkFieldData.h"
#include "vtkPVData.h"

//----------------------------------------------------------------------------
vtkPVArraySelection* vtkPVArraySelection::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVArraySelection");
  if (ret)
    {
    return (vtkPVArraySelection*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVArraySelection;
}

int vtkPVArraySelectionCommand(ClientData cd, Tcl_Interp *interp,
                               int argc, char *argv[]);

vtkPVArraySelection::vtkPVArraySelection()
{
  this->CommandFunction = vtkPVArraySelectionCommand;
  
  this->ArraySelectionFrame = vtkKWWidget::New();
  this->ArraySelectionFrame->SetParent(this);
  this->ArraySelectionLabel = vtkKWLabel::New();
  this->ArraySelectionLabel->SetParent(this->ArraySelectionFrame);
  this->ArraySelectionMenu = vtkKWOptionMenu::New();
  this->ArraySelectionMenu->SetParent(this->ArraySelectionFrame);
  
  this->NumberOfComponents = 1;
  this->UsePointData = 1;
  
  this->PVSource = NULL;
  this->EntryCallback = NULL;
  this->VTKData = NULL;
}

vtkPVArraySelection::~vtkPVArraySelection()
{
  this->ArraySelectionLabel->Delete();
  this->ArraySelectionLabel = NULL;
  this->ArraySelectionMenu->Delete();
  this->ArraySelectionMenu = NULL;
  this->ArraySelectionFrame->Delete();
  this->ArraySelectionFrame = NULL;
  
  if (this->EntryCallback)
    {
    delete [] this->EntryCallback;
    }
}

void vtkPVArraySelection::Create(vtkKWApplication *app)
{
  const char *wname;
  
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("ArraySelection already created");
    return;
    }
  
  this->SetApplication(app);
  
  if (!app)
    {
    return;
    }
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);
  
  this->ArraySelectionFrame->Create(app, "frame", "");
  this->ArraySelectionLabel->Create(app, "");
  this->ArraySelectionLabel->SetLabel("Arrays:");
  this->ArraySelectionMenu->Create(app, "");
  app->Script("pack %s -fill x -side top",
              this->ArraySelectionFrame->GetWidgetName());
  app->Script("pack %s %s -side left",
              this->ArraySelectionLabel->GetWidgetName(),
              this->ArraySelectionMenu->GetWidgetName());
}

void vtkPVArraySelection::FillMenu()
{
  vtkFieldData *fd;
  int numArrays, i, callbackLength, defaultSet = 0;
  const char *arrayName, *defaultArrayName;
  vtkDataArray *dataArray;

  if ( ! this->PVSource || ! this->EntryCallback)
    {
    vtkErrorMacro("Must call SetCommand before FillMenu");
    return;
    }
  
  vtkPVData *pvInput = this->PVSource->GetNthPVInput(0);

  if ( ! pvInput && ! this->VTKData)
    {
    vtkErrorMacro("Can't get list of data arrays");
    return;
    }

  callbackLength = strlen(this->EntryCallback);
  
  this->ArraySelectionMenu->ClearEntries();

  if (this->UsePointData)
    {
    if (pvInput)
      {
      fd = pvInput->GetVTKData()->GetPointData();
      }
    else
      {
      fd = this->VTKData->GetPointData();
      }
    }
  else
    {
    if (pvInput)
      {
      fd = pvInput->GetVTKData()->GetCellData();
      }
    else
      {
      fd = this->VTKData->GetCellData();
      }
    }
  
  defaultArrayName = this->GetValue();
  if (defaultArrayName && defaultArrayName[0] != '\0' &&
      fd->GetArray(defaultArrayName))
    {
    defaultSet = 1;
    }
  
  numArrays = fd->GetNumberOfArrays();
  for (i = 0; i < numArrays; i++)
    {
    dataArray = fd->GetArray(i);
    arrayName = fd->GetArrayName(i);
    if (!arrayName)
      {
      continue;
      }
    
    if (dataArray->GetNumberOfComponents() == this->NumberOfComponents)
      {
      this->ArraySelectionMenu->AddEntryWithCommand(fd->GetArrayName(i),
                                                    this->PVSource,
                                                    this->EntryCallback);
      if (!defaultSet && arrayName[0] != '\0')
        {
        defaultArrayName = arrayName;
        defaultSet = 1;
        }
      }
    }
  
  if (defaultSet)
    {
    this->ArraySelectionMenu->SetValue(defaultArrayName);
    this->Application->Script("%s %s", this->PVSource->GetTclName(),
                              this->EntryCallback);
    }
}

void vtkPVArraySelection::SetMenuEntryCommand(const char *methodString)
{
  this->SetEntryCallback(methodString);
}

void vtkPVArraySelection::SetUsePointData(int val)
{
  if (val == this->UsePointData)
    {
    return;
    }
  
  this->UsePointData = val;
}

void vtkPVArraySelection::SetPVSource(vtkPVSource *source)
{
  // avoiding circular reference counting
  this->PVSource = source;
}

void vtkPVArraySelection::SetVTKData(vtkDataSet *dataSet)
{
  this->VTKData = dataSet;
}
