/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectCustomReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
vtkCxxRevisionMacro(vtkPVSelectCustomReader, "1.7");

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
  label->SetText(str1.str());
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
