/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectCustomReader.cxx
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
#include "vtkPVSelectCustomReader.h"

#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWListBox.h"
#include "vtkKWPushButton.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.h"
#include "vtkObjectFactory.h"
#include "vtkPVReaderModule.h"
#include "vtkPVWindow.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSelectCustomReader);
vtkCxxRevisionMacro(vtkPVSelectCustomReader, "1.4.4.1");

//----------------------------------------------------------------------------
vtkPVSelectCustomReader::vtkPVSelectCustomReader() 
{
}

//----------------------------------------------------------------------------
vtkPVSelectCustomReader::~vtkPVSelectCustomReader() 
{
}

//----------------------------------------------------------------------------
vtkPVReaderModule* vtkPVSelectCustomReader::SelectReader(vtkPVWindow* win, 
                                                         const char* openFileName) 
{
  ostrstream str2;
  str2 << "Opening file " << openFileName << " with a custom reader "
       << "may results in unpredictable result such as ParaView may "
       << "crash. Make sure to pick the right reader." << ends;
  this->SetDialogText(str2.str());
  str2.rdbuf()->freeze(0);
  vtkKWApplication* app = win->GetApplication();
  this->SetStyleToOkCancel();
  this->SetOptions( vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault );
  this->Create(app, 0);
  vtkKWWidget* frame = this->GetTopFrame();
  this->SetMasterWindow(win);
  this->SetTitle("Open Data With...");
  vtkKWLabel* label = vtkKWLabel::New();
  label->SetParent(frame);
  ostrstream str1;
  str1 << "Open " << openFileName << " with:" << ends;
  label->SetLabel(str1.str());
  label->Create(app, 0);
  str1.rdbuf()->freeze(0);

  vtkKWListBox* listbox = vtkKWListBox::New();
  listbox->SetParent(frame);
  listbox->Create(app, 0);
  int num = 5;
  if ( win->GetReaderList()->GetNumberOfItems() < num )
    {
    num = win->GetReaderList()->GetNumberOfItems();
    }
  if ( num < 1 )
    {
    num = 1;
    }
  listbox->SetHeight(num);      
      
  vtkPVReaderModule* result = 0;

  this->Script("pack %s %s -padx 5 -pady 5 -side top", 
               label->GetWidgetName(),
               listbox->GetWidgetName());

  vtkLinkedListIterator<vtkPVReaderModule*>* it = 
    win->GetReaderList()->NewIterator();
  while(!it->IsDoneWithTraversal())
    {
    vtkPVReaderModule* rm = 0;
    if ( it->GetData(rm) == VTK_OK && rm && rm->GetLabel() )
      {
      ostrstream str;
      str << rm->GetLabel() << " Reader" << ends;
      listbox->AppendUnique(str.str());
      str.rdbuf()->freeze(0);
      }
    it->GoToNextItem();
    }
  it->Delete();
  listbox->SetSelectionIndex(0);
  listbox->SetDoubleClickCallback(this, "OK");

  // Set the width to that of the longest string
  listbox->SetWidth(0);      

  // invoke
  int res = this->Invoke();
  if ( res == 1 )
    {
    vtkPVReaderModule* reader = 0;
    if ( win->GetReaderList()->GetItem(listbox->GetSelectionIndex(),
                                   reader) == VTK_OK && reader )
      {
      result = reader;
      }
    }

  // Cleanup
  listbox->Delete();
  label->Delete();

  return result;
}

//----------------------------------------------------------------------------
void vtkPVSelectCustomReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
