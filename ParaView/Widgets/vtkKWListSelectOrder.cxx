/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkKWListSelectOrder.h"

#include "vtkKWApplication.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWListBox.h"
#include "vtkObjectFactory.h"
#include "vtkKWPushButton.h"
#include "vtkKWFrame.h"

#include <vtkstd/string>
#include <vtkstd/vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWListSelectOrder );
vtkCxxRevisionMacro(vtkKWListSelectOrder, "1.2");

//----------------------------------------------------------------------------
vtkKWListSelectOrder::vtkKWListSelectOrder()
{
  this->SourceList = vtkKWListBox::New();
  this->FinalList = vtkKWListBox::New();

  this->AddButton = vtkKWPushButton::New();
  this->AddAllButton = vtkKWPushButton::New();
  this->RemoveButton = vtkKWPushButton::New();
  this->RemoveAllButton = vtkKWPushButton::New();
  this->UpButton = vtkKWPushButton::New();
  this->DownButton = vtkKWPushButton::New();
}

//----------------------------------------------------------------------------
vtkKWListSelectOrder::~vtkKWListSelectOrder()
{
  this->SourceList->Delete();
  this->FinalList->Delete();

  this->AddButton->Delete();
  this->AddAllButton->Delete();
  this->RemoveButton->Delete();
  this->RemoveAllButton->Delete();
  this->UpButton->Delete();
  this->DownButton->Delete();
}

//----------------------------------------------------------------------------
void vtkKWListSelectOrder::Create(vtkKWApplication *app, const char *args)
{
  const char *wname;

  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("Entry already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();

  this->Script("frame %s %s",wname, (args?args:""));
  vtkKWFrame* frame;
  this->SourceList->SetParent(this);
  this->SourceList->Create(app, 0);
  this->SourceList->GetListbox()->Configure("-selectmode multiple");
  this->Script("pack %s -side left -expand true -fill both",
    this->SourceList->GetWidgetName());
  frame = vtkKWFrame::New();
  frame->SetParent(this);
  frame->Create(app, 0);
  this->AddButton->SetParent(frame->GetFrame());
  this->AddButton->Create(app,0);
  this->AddButton->SetLabel("Add");
  this->AddButton->SetCommand(this, "AddCallback");
  this->AddAllButton->SetParent(frame->GetFrame());
  this->AddAllButton->Create(app,0);
  this->AddAllButton->SetLabel("Add All");
  this->AddAllButton->SetCommand(this, "AddAllCallback");
  this->RemoveButton->SetParent(frame->GetFrame());
  this->RemoveButton->Create(app,0);
  this->RemoveButton->SetLabel("Remove");
  this->RemoveButton->SetCommand(this, "RemoveCallback");
  this->RemoveAllButton->SetParent(frame->GetFrame());
  this->RemoveAllButton->Create(app,0);
  this->RemoveAllButton->SetLabel("RemoveAll");
  this->RemoveAllButton->SetCommand(this, "RemoveAllCallback");
  this->Script("pack %s %s %s %s -side top -fill x",
    this->AddButton->GetWidgetName(),
    this->AddAllButton->GetWidgetName(),
    this->RemoveButton->GetWidgetName(),
    this->RemoveAllButton->GetWidgetName());
  this->Script("pack %s -side left -expand false -fill y",
    frame->GetWidgetName());
  frame->Delete();
  frame = vtkKWFrame::New();
  frame->SetParent(this);
  frame->Create(app, 0);
  this->FinalList->SetParent(frame->GetFrame());
  this->FinalList->Create(app, 0);
  this->FinalList->GetListbox()->Configure("-selectmode multiple");
  this->Script("pack %s -side top -expand true -fill both",
    this->FinalList->GetWidgetName());
  vtkKWFrame* btframe = vtkKWFrame::New();
  btframe->SetParent(frame->GetFrame());
  btframe->Create(app, 0);

  this->UpButton->SetParent(btframe->GetFrame());
  this->UpButton->Create(app, 0);
  this->UpButton->SetLabel("Up");
  this->UpButton->SetCommand(this, "UpCallback");
  this->DownButton->SetParent(btframe->GetFrame());
  this->DownButton->Create(app, 0);
  this->DownButton->SetLabel("Down");
  this->DownButton->SetCommand(this, "DownCallback");
  this->Script("pack %s %s -side left -fill x",
    this->UpButton->GetWidgetName(),
    this->DownButton->GetWidgetName());
  this->Script("pack %s -side top -expand false -fill x",
    btframe->GetWidgetName());
  btframe->Delete();
  
  this->Script("pack %s -side left -expand true -fill both",
    frame->GetWidgetName());
  frame->Delete();
}

//----------------------------------------------------------------------------
void vtkKWListSelectOrder::AddElement(vtkKWListBox* l1, vtkKWListBox* l2, 
  const char* element, int force)
{
  int idx = l2->GetItemIndex(element);
  if ( idx >= 0 )
    {
    if ( force )
      {
      l2->DeleteRange(idx, idx);
      l1->AppendUnique(element);
      }
    }
  else
    {
    l1->AppendUnique(element);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWListSelectOrder::AddSourceElement(const char* element, int force /* = 0 */)
{
  this->AddElement(this->SourceList, this->FinalList, element, force);
}

//----------------------------------------------------------------------------
void vtkKWListSelectOrder::AddFinalElement(const char* element, int force /* = 0 */)
{
  this->AddElement(this->FinalList, this->SourceList, element, force);
}

//----------------------------------------------------------------------------
void vtkKWListSelectOrder::AddCallback()
{
  this->MoveSelectedList(this->SourceList, this->FinalList);
}

//----------------------------------------------------------------------------
void vtkKWListSelectOrder::AddAllCallback()
{
  this->MoveWholeList(this->SourceList, this->FinalList);
}

//----------------------------------------------------------------------------
void vtkKWListSelectOrder::RemoveCallback() 
{
  this->MoveSelectedList(this->FinalList, this->SourceList);
}

//----------------------------------------------------------------------------
void vtkKWListSelectOrder::RemoveAllCallback()
{
  this->MoveWholeList(this->FinalList, this->SourceList);
}

//----------------------------------------------------------------------------
void vtkKWListSelectOrder::ShiftItems(vtkKWListBox* l1, int down)
{
  const char* list = this->Script("lsort -integer %s [ %s curselection ]", 
    down?"-decreasing":"",
    l1->GetListbox()->GetWidgetName());
  char* selection = new char[ strlen(list) + 1 ];
  strcpy(selection, list);
  int idx = -1;
  int size = l1->GetNumberOfItems();
  istrstream sel(selection);
  vtkstd::string item;
  while(sel >> idx)
    {
    if ( idx < 0 )
      {
      break;
      }
    int newidx = idx - 1;
    if ( down )
      {
      newidx = idx + 1;
      }
    if ( newidx >= 0 && newidx < size )
      {
      item = l1->GetItem(idx);
      l1->DeleteRange(idx, idx);
      l1->InsertEntry(newidx, item.c_str());
      this->Script("%s selection set %d %d",
        l1->GetListbox()->GetWidgetName(), newidx, newidx);
      }

    idx = -1;
    }
  delete [] selection;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWListSelectOrder::UpCallback()
{
  this->ShiftItems(this->FinalList, 0);
}

//----------------------------------------------------------------------------
void vtkKWListSelectOrder::DownCallback()
{
  this->ShiftItems(this->FinalList, 1);
}

//----------------------------------------------------------------------------
void vtkKWListSelectOrder::MoveWholeList(vtkKWListBox* l1, vtkKWListBox* l2)
{
  this->Script("%s selection set 0 end",
    l1->GetListbox()->GetWidgetName());
  this->MoveSelectedList(l1, l2);
}

//----------------------------------------------------------------------------
void vtkKWListSelectOrder::MoveSelectedList(vtkKWListBox* l1, vtkKWListBox* l2)
{
  const char* cselected = this->Script("%s curselection", 
    l1->GetListbox()->GetWidgetName());
  this->MoveList(l1, l2, cselected);
}

//----------------------------------------------------------------------------
void vtkKWListSelectOrder::MoveList(vtkKWListBox* l1, vtkKWListBox* l2, 
  const char* list)
{
  char* selection = new char[ strlen(list) + 1 ];
  strcpy(selection, list);
  int idx = -1;
  vtkstd::string str;
  vtkstd::vector<int> vec;
  istrstream sel(selection);
  while(sel >> idx)
    {
    if ( idx < 0 )
      {
      break;
      }
    str = l1->GetItem(idx);
    l2->AppendUnique(str.c_str());
    vec.push_back(idx);
    idx = -1;
    }
  while( vec.size() )
    {
    idx = vec[vec.size() - 1];
    l1->DeleteRange(idx,idx);
    vec.erase(vec.end()-1);
    }
  delete [] selection;
  
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkKWListSelectOrder::GetNumberOfElementsOnSourceList()
{
  return this->SourceList->GetNumberOfItems();
}

//----------------------------------------------------------------------------
int vtkKWListSelectOrder::GetNumberOfElementsOnFinalList()
{
  return this->FinalList->GetNumberOfItems();
}

//----------------------------------------------------------------------------
const char* vtkKWListSelectOrder::GetElementFromSourceList(int idx)
{
  return this->SourceList->GetItem(idx);
}

//----------------------------------------------------------------------------
const char* vtkKWListSelectOrder::GetElementFromFinalList(int idx)
{
  return this->FinalList->GetItem(idx);
}

//----------------------------------------------------------------------------
int vtkKWListSelectOrder::GetElementIndexFromSourceList(const char* element)
{
  return this->SourceList->GetItemIndex(element);
}

//----------------------------------------------------------------------------
int vtkKWListSelectOrder::GetElementIndexFromFinalList(const char* element)
{
  return this->FinalList->GetItemIndex(element);
}

//----------------------------------------------------------------------------
void vtkKWListSelectOrder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

