/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectionList.cxx
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
#include "vtkPVSelectionList.h"
#include "vtkStringList.h"
#include "vtkObjectFactory.h"

int vtkPVSelectionListCommand(ClientData cd, Tcl_Interp *interp,
		     int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVSelectionList::vtkPVSelectionList()
{
  this->CommandFunction = vtkPVSelectionListCommand;

  this->CurrentValue = 0;
  this->CurrentName = NULL;
  
  this->Label = vtkKWLabel::New();
  this->MenuButton = vtkKWMenuButton::New();

  this->Names = vtkStringList::New();
}

//----------------------------------------------------------------------------
vtkPVSelectionList::~vtkPVSelectionList()
{
  this->SetCurrentName(NULL);
  
  this->Label->Delete();
  this->Label = NULL;
  this->MenuButton->Delete();
  this->MenuButton = NULL;
  this->Names->Delete();
  this->Names = NULL;
}

//----------------------------------------------------------------------------
vtkPVSelectionList* vtkPVSelectionList::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVSelectionList");
  if(ret)
    {
    return (vtkPVSelectionList*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVSelectionList;
}

//----------------------------------------------------------------------------
int vtkPVSelectionList::Create(vtkKWApplication *app)
{
  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    return 0;
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());

  this->Label->SetParent(this);
  this->Label->Create(app, "-width 18 -justify right");
  this->Script("pack %s -side left", this->Label->GetWidgetName());

  this->MenuButton->SetParent(this);
  this->MenuButton->Create(app, "");
  this->Script("pack %s -side left", this->MenuButton->GetWidgetName());

  return 1;
}


//----------------------------------------------------------------------------
void vtkPVSelectionList::SetLabel(const char* label) 
{
  // For getting the widget in a script.
  this->SetName(label);
  this->Label->SetLabel(label);
}
  


//----------------------------------------------------------------------------
void vtkPVSelectionList::SetAccessMethods(const char* setCmd, 
                                          const char* getCmd)
{
  this->ResetCommands->RemoveAllItems();
  this->AcceptCommands->RemoveAllItems();
  
  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource not set.");
    return;
    }

  if (this->Application == NULL)
    {
    vtkErrorMacro("Create widget before setting access methods.");
    return;
    }

  this->SetSetCommand(setCmd);
  this->SetGetCommand(getCmd);
  
  // Command to update the UI.
  this->ResetCommands->AddString("%s SetCurrentValue [%s %s]",
                                 this->GetTclName(), 
                                 this->PVSource->GetVTKSourceTclName(), 
                                 getCmd); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s %s [%s GetCurrentValue]",
                                  this->PVSource->GetVTKSourceTclName(),
                                  setCmd,
                                  this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->ModifiedFlag && this->PVSource)
    {  
    if ( ! this->TraceInitialized)
      {
      pvApp->AddTraceEntry("set pv(%s) [$pv(%s) GetPVWidget {%s}]",
                           this->GetTclName(), this->PVSource->GetTclName(),
                           this->Name);
      this->TraceInitialized = 1;
      }

    pvApp->AddTraceEntry("$pv(%s) SetCurrentValue {%d}", this->GetTclName(), 
                         this->GetCurrentValue());
    }

  this->vtkPVWidget::Accept();
}



//----------------------------------------------------------------------------
void vtkPVSelectionList::AddItem(const char *name, int value)
{
  char tmp[1024];
  
  // Save for internal use
  this->Names->SetString(value, name);

  sprintf(tmp, "SelectCallback {%s} %d", name, value);
  this->MenuButton->AddCommand(name, this, tmp);
  
  if (value == this->CurrentValue)
    {
    this->MenuButton->SetButtonText(name);
    }
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::SetCurrentValue(int value)
{
  char *name;

  if (this->CurrentValue == value)
    {
    return;
    }

  this->CurrentValue = value;
  name = this->Names->GetString(value);
  if (name)
    {
    this->SelectCallback(name, value);
    }
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::SelectCallback(const char *name, int value)
{
  this->CurrentValue = value;
  this->SetCurrentName(name);
  
  this->MenuButton->SetButtonText(name);
  this->ModifiedCallback();
}

