/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVFileEntry.cxx
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
#include "vtkPVFileEntry.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkPVFileEntry* vtkPVFileEntry::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVFileEntry");
  if (ret)
    {
    return (vtkPVFileEntry*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVFileEntry;
}

//----------------------------------------------------------------------------
vtkPVFileEntry::vtkPVFileEntry()
{
  this->Label = vtkKWLabel::New();
  this->Label->SetParent(this);
  this->Entry = vtkKWEntry::New();
  this->Entry->SetParent(this);
  this->BrowseButton = vtkKWPushButton::New();
  this->BrowseButton->SetParent(this);
  this->Extension = NULL;
}

//----------------------------------------------------------------------------
vtkPVFileEntry::~vtkPVFileEntry()
{
  this->BrowseButton->Delete();
  this->BrowseButton = NULL;
  this->Entry->Delete();
  this->Entry = NULL;
  this->Label->Delete();
  this->Label = NULL;
  this->SetExtension(NULL);
}

//----------------------------------------------------------------------------
void vtkPVFileEntry::Create(vtkKWApplication *pvApp, char *label,
                            char *ext, char *help)
{
  const char* wname;
  
  if (this->Application)
    {
    vtkErrorMacro("FileEntry already created");
    return;
    }
  
  // For getting the widget in a script.
  this->SetTraceName(label);
  this->SetApplication(pvApp);

  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);
  
  // Now a label
  if (label && label[0] != '\0')
    {
    this->Label->Create(pvApp, "-width 18 -justify right");
    this->Label->SetLabel(label);
    if (help)
      {
      this->Label->SetBalloonHelpString(help);
      }
    this->Script("pack %s -side left", this->Label->GetWidgetName());
    }
  
  // Now the entry
  this->Entry->Create(pvApp, "");
  this->Script("bind %s <KeyPress> {%s ModifiedCallback}",
               this->Entry->GetWidgetName(), this->GetTclName());
  if (help)
    { 
    this->Entry->SetBalloonHelpString(help);
   }
  this->Script("pack %s -side left -fill x -expand t",
               this->Entry->GetWidgetName());
  
  // Now the push button
  this->BrowseButton->Create(pvApp, "");
  this->BrowseButton->SetLabel("Browse");
  this->BrowseButton->SetCommand(this, "BrowseCallback");
  if (help)
    {
    this->BrowseButton->SetBalloonHelpString(help);
    }
  this->Script("pack %s -side left", this->BrowseButton->GetWidgetName());

  this->SetExtension(ext);
}


//----------------------------------------------------------------------------
void vtkPVFileEntry::BrowseCallback()
{
  if (this->Extension)
    {
    this->Script("%s SetValue [tk_getOpenFile -filetypes {{{} {.%s}}}]", 
                 this->GetTclName(), this->Extension);
    }
  else
    {
    this->Script("%s SetValue [tk_getOpenFile]", this->GetTclName());
    }
}

//----------------------------------------------------------------------------
void vtkPVFileEntry::SetValue(const char* fileName)
{
  const char *old;
  
  if (fileName == NULL)
    {
    fileName = "";
    }

  old = this->Entry->GetValue();
  if (strcmp(old, fileName) == 0)
    {
    return;
    }

  this->Entry->SetValue(fileName); 
  this->ModifiedCallback();
}



//----------------------------------------------------------------------------
void vtkPVFileEntry::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->ModifiedFlag)
    {  
    pvApp->AddTraceEntry("$kw(%s) SetValue %s", this->GetTclName(), 
                         this->GetValue());
    }

  pvApp->BroadcastScript("%s Set%s %s",
                         this->ObjectTclName, this->VariableName, 
                         this->Entry->GetValue());

  // The supper does nothing but turn the modified flag off.
  this->vtkPVWidget::Accept();
}


//----------------------------------------------------------------------------
void vtkPVFileEntry::Reset()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  pvApp->BroadcastScript("%s SetValue [%s Get%s]",
                         this->Entry->GetTclName(),
                         this->ObjectTclName, this->VariableName); 

  // The supper does nothing but turn the modified flag off.
  this->vtkPVWidget::Reset();
}
