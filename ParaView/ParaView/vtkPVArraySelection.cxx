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
#include "vtkKWPushButton.h"
#include "vtkKWCheckButton.h"
#include "vtkCollection.h"
#include "vtkObjectFactory.h"

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

  this->ArrayCheckButtons = vtkCollection::New();
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

  this->ArrayCheckButtons->Delete();
  this->ArrayCheckButtons = NULL;
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
  if (strcmp(this->AttributeName, "Point") == 0)
    {
    this->LabeledFrame->SetLabel("Point Arrays");
    }
  else
    {
    this->LabeledFrame->SetLabel("Cell Arrays");
    }
  this->LabeledFrame->Create(app);
  app->Script("pack %s -fill x -side top",
              this->LabeledFrame->GetWidgetName());

  this->ButtonFrame->SetParent(this->LabeledFrame->GetFrame());
  this->ButtonFrame->Create(app, "frame", "");

  this->AllOnButton->SetParent(this->ButtonFrame);
  this->AllOnButton->SetLabel("All On");
  this->AllOnButton->Create(app, "");
  this->AllOnButton->SetCommand(this, "AllOnCallback");
  app->Script("pack %s -fill x -side left -expand t",
              this->AllOnButton->GetWidgetName());

  this->AllOffButton->SetParent(this->ButtonFrame);
  this->AllOffButton->SetLabel("All On");
  this->AllOffButton->Create(app, "");
  this->AllOffButton->SetCommand(this, "AllOffCallback");
  app->Script("pack %s -fill x -side left -expand t",
              this->AllOffButton->GetWidgetName());

  // This creates the check buttons and packs the button frame.
  this->Reset();
}


//----------------------------------------------------------------------------
void vtkPVArraySelection::Reset()
{
  // Clear out any old check buttons.
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->LabeledFrame->GetWidgetName());
  this->ArrayCheckButtons->RemoveAllItems();
  
  // Create new check buttons.
  if (this->VTKReaderTclName)
    {
    int numArrays, idx;
    vtkKWCheckButton* checkButton;
    this->Script("%s GetNumberOf%sArrays", 
                 this->VTKReaderTclName, this->AttributeName);
    numArrays = this->GetIntegerResult(this->Application);
    for (idx = 0; idx < numArrays; ++idx)
      {
      checkButton = vtkKWCheckButton::New();
      checkButton->SetParent(this->LabeledFrame->GetFrame());
      checkButton->Create(this->Application, "");
      this->Script("%s SetLabel [%s GetNameOf%sArray %d]", 
                   checkButton->GetTclName(), 
                   this->VTKReaderTclName, this->AttributeName, idx);
      this->Script("%s SetState [%s Get%sArrayStatus %d]",
                   checkButton->GetTclName(), 
                   this->VTKReaderTclName, this->AttributeName, idx);
      this->Script("pack %s -side top", checkButton->GetWidgetName());
      checkButton->SetCommand(this, "ModifiedCallback");
      this->ArrayCheckButtons->AddItem(checkButton);
      checkButton->Delete();
      }
    }

  // Pack the button frame.
  this->Script("pack %s -side top -fill x -expand t", 
               this->ButtonFrame->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVArraySelection::Accept()
{
  vtkKWCheckButton *check;
  int idx;

  // Create new check buttons.
  if (this->VTKReaderTclName == NULL)
    {
    vtkErrorMacro("VTKREader has not been set.");
    }

  this->ArrayCheckButtons->InitTraversal();
  idx = 0;
  while ( (check = (vtkKWCheckButton*)(this->ArrayCheckButtons->GetNextItemAsObject())) )
    {
    // I could also do this by array name.
    this->Script("%s Set%sArrayStatus %d %d", this->VTKReaderTclName,
                 this->AttributeName, idx, check->GetState());
    ++idx;
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


