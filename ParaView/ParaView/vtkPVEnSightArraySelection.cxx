/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVEnSightArraySelection.cxx
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
#include "vtkPVEnSightArraySelection.h"
#include "vtkObjectFactory.h"
#include "vtkKWCheckButton.h"
#include "vtkCollection.h"
#include "vtkPVApplication.h"

//-------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVEnSightArraySelection);

//-------------------------------------------------------------------------
int vtkPVEnSightArraySelectionCommand(ClientData cd, Tcl_Interp *interp,
                                      int argc, char *argv[]);

vtkPVEnSightArraySelection::vtkPVEnSightArraySelection()
{
  this->CommandFunction = vtkPVEnSightArraySelectionCommand;
}

vtkPVEnSightArraySelection::~vtkPVEnSightArraySelection()
{
}

void vtkPVEnSightArraySelection::Accept()
{
  vtkKWCheckButton *check;
  vtkPVApplication *pvApp = this->GetPVApplication();
  int attributeType;
  
  if (strcmp(this->AttributeName, "Point") == 0)
    {
    attributeType = 0;
    }
  else
    {
    attributeType = 1;
    }
  
  // Create new check buttons.
  if (this->VTKReaderTclName == NULL)
    {
    vtkErrorMacro("VTKReader has not been set.");
    }

  if (this->ModifiedFlag == 0)
    {
    return;
    }

  pvApp->BroadcastScript("%s RemoveAll%sVariableNames", this->VTKReaderTclName,
                         this->AttributeName);
  
  this->ArrayCheckButtons->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->ArrayCheckButtons->GetNextItemAsObject())) )
    {
    if (check->GetState())
      {
      pvApp->BroadcastScript("%s AddVariableName {%s} %d",
                             this->VTKReaderTclName, check->GetText(),
                             attributeType);
      }

    this->AddTraceEntry("$kw(%s) SetArrayStatus {%s} %d", this->GetTclName(), 
                        check->GetText(), check->GetState());
    }
}

void vtkPVEnSightArraySelection::Reset()
{
  vtkKWCheckButton* checkButton;
  int row = 0;
  int attributeType, variableType;
  
  if (strcmp(this->AttributeName, "Point") == 0)
    {
    attributeType = 0;
    }
  else
    {
    attributeType = 1;
    }
  
  // Clear out any old check buttons.
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->CheckFrame->GetWidgetName());
  this->ArrayCheckButtons->RemoveAllItems();
  
  // Create new check buttons.
  if (this->VTKReaderTclName)
    {
    int numArrays, idx;
    this->Script("%s GetNumberOfVariables", this->VTKReaderTclName);
    numArrays = this->GetIntegerResult(this->Application);
    
    for (idx = 0; idx < numArrays; ++idx)
      {
      this->Script("%s GetVariableType %d",
                   this->VTKReaderTclName, idx);
      variableType = vtkKWObject::GetIntegerResult(this->Application);
      switch (attributeType)
        {
        case 0:
          if ((variableType > 2 && variableType < 6) || variableType > 7)
            {
            continue;
            }
          break;
        case 1:
          if (variableType < 3 || variableType > 5)
            {
            continue;
            }
          break;
        }
      
      checkButton = vtkKWCheckButton::New();
      checkButton->SetParent(this->CheckFrame);
      checkButton->Create(this->Application, "");
      this->Script("%s SetText [%s GetDescription %d]", 
                   checkButton->GetTclName(), this->VTKReaderTclName, idx);
      this->Script("grid %s -row %d -sticky w",
                   checkButton->GetWidgetName(), row);
      ++row;
      checkButton->SetCommand(this, "ModifiedCallback");
      this->ArrayCheckButtons->AddItem(checkButton);
      checkButton->Delete();
      }

    this->Script("%s GetNumberOfComplexVariables", this->VTKReaderTclName);
    numArrays = this->GetIntegerResult(this->Application);
    
    for (idx = 0; idx < numArrays; ++idx)
      {
      this->Script("%s GetComplexVariableType %d",
                   this->VTKReaderTclName, idx);
      variableType = vtkKWObject::GetIntegerResult(this->Application);
      switch (attributeType)
        {
        case 0:
          if (variableType > 9 || variableType < 8)
            {
            continue;
            }
          break;
        case 1:
          if (variableType < 10)
            {
            continue;
            }
        }
      
      checkButton = vtkKWCheckButton::New();
      checkButton->SetParent(this->CheckFrame);
      checkButton->Create(this->Application, "");
      this->Script("%s SetText [%s GetComplexDescription %d]", 
                   checkButton->GetTclName(), this->VTKReaderTclName, idx);
      this->Script("grid %s -row %d -sticky w",
                   checkButton->GetWidgetName(), row);
      ++row;
      checkButton->SetCommand(this, "ModifiedCallback");
      this->ArrayCheckButtons->AddItem(checkButton);
      checkButton->Delete();
      }    
    }

  // Now set the state of the check buttons.
  this->ArrayCheckButtons->InitTraversal();
  while ( (checkButton = (vtkKWCheckButton*)(this->ArrayCheckButtons->GetNextItemAsObject())) )
    {
    this->Script("%s IsRequestedVariable {%s} %d", this->VTKReaderTclName,
                 checkButton->GetText(), attributeType);
    checkButton->SetState(this->GetIntegerResult(this->Application));
    }
}

void vtkPVEnSightArraySelection::SaveInTclScript(ofstream *file)
{
  vtkKWCheckButton *check;
  int state;
  int attributeType;

  if (strcmp(this->AttributeName, "Point") == 0)
    {
    attributeType = 0;
    }
  else
    {
    attributeType = 1;
    }

  // Create new check buttons.
  if (this->VTKReaderTclName == NULL)
    {
    vtkErrorMacro("VTKReader has not been set.");
    }

  this->ArrayCheckButtons->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->ArrayCheckButtons->GetNextItemAsObject())) )
    {
    this->Script("%s IsRequestedVariable {%s} %d", this->VTKReaderTclName,
                 check->GetText(), attributeType);
    state = this->GetIntegerResult(this->Application);
    // Since they default to on.
    if (state == 0)
      {
      *file << "\t";
      *file << this->VTKReaderTclName << " AddVariableName {"
            << check->GetText() << "} " << endl;
       
      }
    }
}

void vtkPVEnSightArraySelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
