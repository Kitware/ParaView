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

#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkKWPushButton.h"
#include "vtkKWCheckButton.h"
#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkArrayMap.txx"
#include "vtkPVXMLElement.h"
#include "vtkKWLabeledFrame.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVArraySelection);

//----------------------------------------------------------------------------
int vtkPVArraySelectionCommand(ClientData cd, Tcl_Interp *interp,
                               int argc, char *argv[]);

vtkPVArraySelection::vtkPVArraySelection()
{
  this->CommandFunction = vtkPVArraySelectionCommand;
  
  this->VTKReaderTclName = NULL;
  this->AttributeName = NULL;

  this->LabeledFrame = vtkKWLabeledFrame::New();
  this->ButtonFrame = vtkKWWidget::New();
  this->AllOnButton = vtkKWPushButton::New();
  this->AllOffButton = vtkKWPushButton::New();

  this->CheckFrame = vtkKWWidget::New();
  this->ArrayCheckButtons = vtkCollection::New();

  this->FileName = NULL;
}

//----------------------------------------------------------------------------
vtkPVArraySelection::~vtkPVArraySelection()
{
  this->SetVTKReaderTclName(NULL);
  this->SetAttributeName(NULL);

  this->LabeledFrame->Delete();
  this->LabeledFrame = NULL;

  this->ButtonFrame->Delete();
  this->ButtonFrame = NULL;

  this->AllOnButton->Delete();
  this->AllOnButton = NULL;

  this->AllOffButton->Delete();
  this->AllOffButton = NULL;

  this->CheckFrame->Delete();
  this->CheckFrame = NULL;

  this->ArrayCheckButtons->Delete();
  this->ArrayCheckButtons = NULL;

  this->SetFileName(NULL);
}

//----------------------------------------------------------------------------
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
  
  this->LabeledFrame->SetParent(this);
  this->LabeledFrame->ShowHideFrameOn();
  this->LabeledFrame->Create(app);
  if (strcmp(this->AttributeName, "Point") == 0)
    {
    this->LabeledFrame->SetLabel("Point Arrays");
    }
  else
    {
    this->LabeledFrame->SetLabel("Cell Arrays");
    }
  app->Script("pack %s -fill x -expand t -side top",
              this->LabeledFrame->GetWidgetName());

  this->ButtonFrame->SetParent(this->LabeledFrame->GetFrame());
  this->ButtonFrame->Create(app, "frame", "");
  app->Script("pack %s -fill x -side top -expand t",
              this->ButtonFrame->GetWidgetName());

  this->AllOnButton->SetParent(this->ButtonFrame);
  this->AllOnButton->Create(app, "");
  this->AllOnButton->SetLabel("All On");
  this->AllOnButton->SetCommand(this, "AllOnCallback");
  app->Script("pack %s -fill x -side left -expand t",
              this->AllOnButton->GetWidgetName());

  this->AllOffButton->SetParent(this->ButtonFrame);
  this->AllOffButton->Create(app, "");
  this->AllOffButton->SetLabel("All Off");
  this->AllOffButton->SetCommand(this, "AllOffCallback");
  app->Script("pack %s -fill x -side left -expand t",
              this->AllOffButton->GetWidgetName());

  this->CheckFrame->SetParent(this->LabeledFrame->GetFrame());
  this->CheckFrame->Create(app, "frame", "");
  app->Script("pack %s -fill both -side top -expand t",
              this->CheckFrame->GetWidgetName());

  // This creates the check buttons and packs the button frame.
  this->Reset();
}


//----------------------------------------------------------------------------
void vtkPVArraySelection::Reset()
{
  vtkKWCheckButton* checkButton;
  int row = 0;

  // See if we need to create new check buttons.
  this->Script("%s GetFileName", this->VTKReaderTclName);

  // Filename not set
  if (this->Application->GetMainInterp()->result[0] == '\0')
    {
    return;
    }

  if (this->FileName == NULL || 
      strcmp(this->FileName, this->Application->GetMainInterp()->result) != 0)
    {
    this->SetFileName(this->Application->GetMainInterp()->result);

    // Clear out any old check buttons.
    this->Script("catch {eval pack forget [pack slaves %s]}",
                 this->CheckFrame->GetWidgetName());
    this->ArrayCheckButtons->RemoveAllItems();
      
    // Create new check buttons.
    if (this->VTKReaderTclName)
      {
      int numArrays, idx;
      this->Script("%s UpdateInformation", this->VTKReaderTclName);
      this->Script("%s GetNumberOf%sArrays", 
                   this->VTKReaderTclName, this->AttributeName);
      numArrays = this->GetIntegerResult(this->Application);
      for (idx = 0; idx < numArrays; ++idx)
        {
        checkButton = vtkKWCheckButton::New();
        checkButton->SetParent(this->CheckFrame);
        checkButton->Create(this->Application, "");
        this->Script("%s SetText [%s Get%sArrayName %d]", 
                     checkButton->GetTclName(), 
                     this->VTKReaderTclName, this->AttributeName, idx);
        this->Script("grid %s -row %d -sticky w", checkButton->GetWidgetName(), row);
        ++row;
        checkButton->SetCommand(this, "ModifiedCallback");
        this->ArrayCheckButtons->AddItem(checkButton);
        checkButton->Delete();
        }
      }
    }

  // Now set the state of the check buttons.
  this->ArrayCheckButtons->InitTraversal();
  while ( (checkButton = (vtkKWCheckButton*)(this->ArrayCheckButtons->GetNextItemAsObject())) )
    {
    //this->Script("%s SetState [%s Get%sArrayStatus {%s}]", 
    //             checkButton->GetTclName(), this->VTKReaderTclName,
    //             this->AttributeName, checkButton->GetText());
    this->Script("%s Get%sArrayStatus {%s}", this->VTKReaderTclName,
                 this->AttributeName, checkButton->GetText());
    checkButton->SetState(this->GetIntegerResult(this->Application));
    }
}


//----------------------------------------------------------------------------
void vtkPVArraySelection::Accept()
{
  vtkKWCheckButton *check;
  vtkPVApplication *pvApp = this->GetPVApplication();

  // Create new check buttons.
  if (this->VTKReaderTclName == NULL)
    {
    vtkErrorMacro("VTKREader has not been set.");
    }

  if (this->ModifiedFlag == 0)
    {
    return;
    }

  this->ArrayCheckButtons->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->ArrayCheckButtons->GetNextItemAsObject())) )
    {
    // This is only here to try to avoid extra lines in the trace file.
    // We could make every check button a pv widget.
    this->Script("%s Get%sArrayStatus {%s}", this->VTKReaderTclName,
                 this->AttributeName, check->GetText());
    if (this->GetIntegerResult(this->Application) != check->GetState())
      {
      pvApp->BroadcastScript("%s Set%sArrayStatus {%s} %d", 
                             this->VTKReaderTclName, this->AttributeName, 
                             check->GetText(), check->GetState());    

      this->AddTraceEntry("$kw(%s) SetArrayStatus {%s} %d", this->GetTclName(), 
                          check->GetText(), check->GetState());
      }
    }
}



//----------------------------------------------------------------------------
void vtkPVArraySelection::AllOnCallback()
{
  vtkKWCheckButton *check;
  int modified = 0;;

  this->ArrayCheckButtons->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->ArrayCheckButtons->GetNextItemAsObject())) )
    {
    if (check->GetState() == 0)
      {
      modified = 1;
      check->SetState(1);
      }
    }
   
  if (modified)
    {
    this->ModifiedCallback();
    }   
}


//----------------------------------------------------------------------------
void vtkPVArraySelection::AllOffCallback()
{
  vtkKWCheckButton *check;
  int modified = 0;;

  this->ArrayCheckButtons->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->ArrayCheckButtons->GetNextItemAsObject())) )
    {
    if (check->GetState() != 0)
      {
      modified = 1;
      check->SetState(0);
      }
    }
   
  if (modified)
    {
    this->ModifiedCallback();
    }   
}

//----------------------------------------------------------------------------
void vtkPVArraySelection::SetArrayStatus(const char *name, int status)
{
  vtkKWCheckButton *check;

  this->ArrayCheckButtons->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->ArrayCheckButtons->GetNextItemAsObject())) )
    {
    if ( strcmp(check->GetText(), name) == 0)
      {
      check->SetState(status);
      return;
      }
    }  
  vtkErrorMacro("Could not find array: " << name);
}


//----------------------------------------------------------------------------
void vtkPVArraySelection::SaveInTclScript(ofstream *file)
{
  vtkKWCheckButton *check;
  int state;

  // Create new check buttons.
  if (this->VTKReaderTclName == NULL)
    {
    vtkErrorMacro("VTKReader has not been set.");
    }

  this->ArrayCheckButtons->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->ArrayCheckButtons->GetNextItemAsObject())) )
    {
    this->Script("%s Get%sArrayStatus {%s}", this->VTKReaderTclName,
                 this->AttributeName, check->GetText());
    state = this->GetIntegerResult(this->Application);
    // Since they default to on.
    if (state == 0)
      {
      *file << "\t";
      *file << this->VTKReaderTclName << " Set" << this->AttributeName << "ArrayStatus {" 
            << check->GetText() << "} " << state << endl;
       
      }
    }
}

vtkPVArraySelection* vtkPVArraySelection::ClonePrototype(vtkPVSource* pvSource,
				 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVArraySelection::SafeDownCast(clone);
}

void vtkPVArraySelection::CopyProperties(vtkPVWidget* clone, 
					 vtkPVSource* pvSource,
			      vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVArraySelection* pvas = vtkPVArraySelection::SafeDownCast(clone);
  if (pvas)
    {
    pvas->SetAttributeName(this->AttributeName);
    pvas->SetVTKReaderTclName(pvSource->GetVTKSourceTclName());
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVArraySelection.");
    }
}

//----------------------------------------------------------------------------
int vtkPVArraySelection::ReadXMLAttributes(vtkPVXMLElement* element,
                                           vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  const char* attribute_name = element->GetAttribute("attribute_name");
  if(attribute_name)
    {
    this->SetAttributeName(attribute_name);
    }
  else
    {
    vtkErrorMacro("No attribute_name specified.");
    return 0;
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVArraySelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "AttributeName: " 
     << (this->AttributeName?this->AttributeName:"none") << endl;
  os << indent << "VTKReaderTclName: " 
     << (this->VTKReaderTclName?this->VTKReaderTclName:"none") << endl;
}
