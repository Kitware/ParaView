/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtractPartsWidget.cxx
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
#include "vtkPVExtractPartsWidget.h"

#include "vtkCollection.h"
#include "vtkKWLabel.h"
#include "vtkKWListBox.h"
#include "vtkKWPushButton.h"
#include "vtkKWWidget.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkUnsignedCharArray.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVExtractPartsWidget);
vtkCxxRevisionMacro(vtkPVExtractPartsWidget, "1.6.4.2");

int vtkPVExtractPartsWidgetCommand(ClientData cd, Tcl_Interp *interp,
                                int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVExtractPartsWidget::vtkPVExtractPartsWidget()
{
  this->CommandFunction = vtkPVExtractPartsWidgetCommand;
  
  this->ButtonFrame = vtkKWWidget::New();
  this->AllOnButton = vtkKWPushButton::New();
  this->AllOffButton = vtkKWPushButton::New();

  this->PartSelectionList = vtkKWListBox::New();
  this->PartLabelCollection = vtkCollection::New();
  
  this->AcceptCalled = 0;
  this->LastAcceptedPartStates = vtkUnsignedCharArray::New();
}

//----------------------------------------------------------------------------
vtkPVExtractPartsWidget::~vtkPVExtractPartsWidget()
{
  this->ButtonFrame->Delete();
  this->ButtonFrame = NULL;
  this->AllOnButton->Delete();
  this->AllOnButton = NULL;
  this->AllOffButton->Delete();
  this->AllOffButton = NULL;

  this->PartSelectionList->Delete();
  this->PartSelectionList = NULL;
  this->PartLabelCollection->Delete();
  this->PartLabelCollection = NULL;
  
  this->LastAcceptedPartStates->Delete();
}

//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::Create(vtkKWApplication *app)
{
  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);

  if (this->Application)
    {
    vtkErrorMacro("PVWidget already created");
    return;
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s -borderwidth 0 -relief flat", this->GetWidgetName());

  
  this->ButtonFrame->SetParent(this);
  this->ButtonFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x",
               this->ButtonFrame->GetWidgetName());

  this->AllOnButton->SetParent(this->ButtonFrame);
  this->AllOnButton->Create(pvApp, "");
  this->AllOnButton->SetLabel("All On");
  this->AllOnButton->SetCommand(this, "AllOnCallback");

  this->AllOffButton->SetParent(this->ButtonFrame);
  this->AllOffButton->Create(pvApp, "");
  this->AllOffButton->SetLabel("All Off");
  this->AllOffButton->SetCommand(this, "AllOffCallback");

  this->Script("pack %s %s -side left -fill x -expand t",
               this->AllOnButton->GetWidgetName(),
               this->AllOffButton->GetWidgetName());

  this->PartSelectionList->SetParent(this);
  this->PartSelectionList->ScrollbarOff();
  this->PartSelectionList->Create(app, "-selectmode extended");
  this->PartSelectionList->SetHeight(0);
  // I assume we need focus for control and alt modifiers.
  this->Script("bind %s <Enter> {focus %s}",
               this->PartSelectionList->GetWidgetName(),
               this->PartSelectionList->GetWidgetName());

  this->Script("pack %s -side top -fill both -expand t",
               this->PartSelectionList->GetWidgetName());

  int num = this->PVSource->GetPVInput(0)->GetNumberOfParts();
  int i;
  for (i = 0; i < num; i++)
    {
    this->LastAcceptedPartStates->InsertValue(i, 1);
    }
  
  // There is no current way to get a modified call back, so assume
  // the user will change the list.  This widget will only be used once anyway.
  this->ModifiedCallback();
}


//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::Inactivate()
{
  int num, idx;
  vtkKWLabel* label;

  this->Script("pack forget %s %s", this->ButtonFrame->GetWidgetName(),
               this->PartSelectionList->GetWidgetName());

  num = this->PartSelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    if (this->PartSelectionList->GetSelectState(idx))
      {
      label = vtkKWLabel::New();
      label->SetParent(this);
      label->SetLabel(this->PartSelectionList->GetItem(idx));
      label->Create(this->Application, "");
      this->Script("pack %s -side top -anchor w",
                   label->GetWidgetName());
      this->PartLabelCollection->AddItem(label);
      label->Delete();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::AcceptInternal(const char* vtkSourceTclName)
{
  int num, idx;
  int state;

  num = this->PartSelectionList->GetNumberOfItems();

  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->ModifiedFlag)
    {
    this->Inactivate();
    }

  // Now loop through the input mask setting the selection states.
  for (idx = 0; idx < num; ++idx)
    {
    state = this->PartSelectionList->GetSelectState(idx);    
    pvApp->GetProcessModule()->ServerScript("%s SetInputMask %d %d",
                                            vtkSourceTclName, idx, state);
    this->LastAcceptedPartStates->InsertValue(idx, state);
    }

  this->ModifiedFlag = 0;
  this->AcceptCalled = 1;
}


//---------------------------------------------------------------------------
void vtkPVExtractPartsWidget::SetSelectState(int idx, int val)
{
  this->PartSelectionList->SetSelectState(idx, val);
  
  if (!this->AcceptCalled)
    {
    this->LastAcceptedPartStates->InsertValue(idx, val);
    }
}


//---------------------------------------------------------------------------
void vtkPVExtractPartsWidget::Trace(ofstream *file)
{
  int idx, num;

  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  num = this->PartSelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    *file << "$kw(" << this->GetTclName() << ") SetSelectState "
          << idx << " " << this->PartSelectionList->GetSelectState(idx) << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::ResetInternal()
{
  vtkPVSource *input;
  vtkPVPart *part;
  int num, idx;

  this->PartSelectionList->DeleteAll();
  // Loop through all of the parts of the input adding to the list.
  input = this->PVSource->GetPVInput(0);
  num = input->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = input->GetPart(idx);
    this->PartSelectionList->InsertEntry(idx, part->GetName());
    }

  // Now loop through the input mask setting the selection states.
  for (idx = 0; idx < num; ++idx)
    {
//    this->Script("%s SetSelectState %d [%s GetInputMask %d]",
//                 this->PartSelectionList->GetTclName(),
//                 idx, vtkSourceTclName, idx);
    this->PartSelectionList->SetSelectState(
      idx, this->LastAcceptedPartStates->GetValue(idx));
    }

  // Because list box does not notify us when it is modified ...
  //this->ModifiedFlag = 0;
}


//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::AllOnCallback()
{
  int num, idx;

  num = this->PartSelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    this->PartSelectionList->SetSelectState(idx, 1);
    }

  // Because list box does not notify us when it is modified ...
  //this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::AllOffCallback()
{
  int num, idx;

  num = this->PartSelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    this->PartSelectionList->SetSelectState(idx, 0);
    }

  // Because list box does not notify us when it is modified ...
  //this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
// Multiple input filter has only one VTK source.
void vtkPVExtractPartsWidget::SaveInBatchScript(ofstream *file)
{
  int num, idx;
  int state;

  num = this->PartSelectionList->GetNumberOfItems();

  // Now loop through the input mask setting the selection states.
  for (idx = 0; idx < num; ++idx)
    {
    state = this->PartSelectionList->GetSelectState(idx);  
    *file << "\t" << this->PVSource->GetVTKSourceTclName(0) 
          << " SetInputMask " << idx << " " << state << endl;  
    }
}

//----------------------------------------------------------------------------
void vtkPVExtractPartsWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
