/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContourEntry.cxx
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
#include "vtkPVContourEntry.h"
#include "vtkPVApplication.h"
#include "vtkObjectFactory.h"
#include "vtkContourFilter.h"
#include "vtkKWListBox.h"
#include "vtkKWEntry.h"
#include "vtkKWPushButton.h"

int vtkPVContourEntryCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVContourEntry::vtkPVContourEntry()
{
  this->CommandFunction = vtkPVContourEntryCommand;
  
  this->ContourValuesLabel = vtkKWLabel::New();
  this->ContourValuesList = vtkKWListBox::New();
  this->NewValueFrame = vtkKWWidget::New();
  this->NewValueLabel = vtkKWLabel::New();
  this->NewValueEntry = vtkKWEntry::New();
  this->AddValueButton = vtkKWPushButton::New();
  this->DeleteValueButton = vtkKWPushButton::New();
}

//----------------------------------------------------------------------------
vtkPVContourEntry::~vtkPVContourEntry()
{
  this->ContourValuesLabel->Delete();
  this->ContourValuesLabel = NULL;
  this->ContourValuesList->Delete();
  this->ContourValuesList = NULL;
  this->NewValueLabel->Delete();
  this->NewValueLabel = NULL;
  this->NewValueEntry->Delete();
  this->NewValueEntry = NULL;
  this->AddValueButton->Delete();
  this->AddValueButton = NULL;
  this->NewValueFrame->Delete();
  this->NewValueFrame = NULL;
  this->DeleteValueButton->Delete();
  this->DeleteValueButton = NULL;
}

//----------------------------------------------------------------------------
vtkPVContourEntry* vtkPVContourEntry::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVContourEntry");
  if(ret)
    {
    return (vtkPVContourEntry*)ret;
    }
  return new vtkPVContourEntry;
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::SetLabel(const char* str)
{
  this->ContourValuesLabel->SetLabel(str);
  this->SetName(str);
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::Create(vtkKWApplication *app)
{
  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    return;
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());
  
  this->ContourValuesLabel->SetParent(this);
  this->ContourValuesLabel->Create(app, "");
  this->ContourValuesLabel->SetLabel("Contour Values");
  
  this->ContourValuesList->SetParent(this);
  this->ContourValuesList->Create(app, "");
  this->ContourValuesList->SetHeight(5);
  this->ContourValuesList->SetBalloonHelpString("List of the current contour values");
  this->Script("bind %s <Delete> {%s DeleteValueCallback}",
               this->ContourValuesList->GetWidgetName(),
               this->GetTclName());
  // We need focus for delete binding.
  this->Script("bind %s <Enter> {focus %s}",
               this->ContourValuesList->GetWidgetName(),
               this->ContourValuesList->GetWidgetName());
  
  this->NewValueFrame->SetParent(this);
  this->NewValueFrame->Create(app, "frame", "");
  
  
  this->Script("pack %s %s %s",
               this->ContourValuesLabel->GetWidgetName(),
               this->ContourValuesList->GetWidgetName(),
               this->NewValueFrame->GetWidgetName());
  
  this->NewValueLabel->SetParent(this->NewValueFrame);
  this->NewValueLabel->Create(app, "");
  this->NewValueLabel->SetLabel("New Value:");
  this->NewValueLabel->SetBalloonHelpString("Enter a new contour value");
  
  this->NewValueEntry->SetParent(this->NewValueFrame);
  this->NewValueEntry->Create(app, "");
  this->NewValueEntry->SetValue("");
  this->NewValueEntry->SetBalloonHelpString("Enter a new contour value");
  this->Script("bind %s <KeyPress-Return> {%s AddValueCallback}",
               this->NewValueEntry->GetWidgetName(),
               this->GetTclName());
  
  this->AddValueButton->SetParent(this->NewValueFrame);
  this->AddValueButton->Create(app, "-text {Add Value}");
  this->AddValueButton->SetCommand(this, "AddValueCallback");
  this->AddValueButton->SetBalloonHelpString("Add the new contour value to the contour values list");
  
  this->Script("pack %s %s %s -side left",
               this->NewValueLabel->GetWidgetName(),
               this->NewValueEntry->GetWidgetName(),
               this->AddValueButton->GetWidgetName());
  
  this->DeleteValueButton->SetParent(this);
  this->DeleteValueButton->Create(app, "-text {Delete Value}");
  this->DeleteValueButton->SetCommand(this, "DeleteValueCallback");
  this->DeleteValueButton->SetBalloonHelpString("Remove the currently selected contour value from the list");
  

  this->Script("pack %s -anchor w -padx 10",
               this->DeleteValueButton->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::AddValueCallback()
{
  if (strcmp(this->NewValueEntry->GetValue(), "") == 0)
    {
    return;
    }

  this->ContourValuesList->AppendUnique(this->NewValueEntry->GetValue());
  this->NewValueEntry->SetValue("");
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::DeleteValueCallback()
{
  int index;
  
  index = this->ContourValuesList->GetSelectionIndex();
  this->ContourValuesList->DeleteRange(index, index);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::AddValue(char *val)
{
  this->ContourValuesList->AppendUnique(val);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::RemoveAllValues()
{
  this->ContourValuesList->DeleteAll();
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::Accept()
{
  int i;
  float value;
  int numContours = this->ContourValuesList->GetNumberOfItems();
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource not set.");
    return;
    }

  // Here is another way we could see if the widget has been modified.
  // First lets see if anything has changed.
  //if (numContours != this->ContourValuesList->GetNumberOfItems());
  //  {
  //  modifiedFlag = 1;
  //  }
  //for (i = 0; i < numContours && ! modifiedFlag; i++)
  //  {
  //  value = atof(this->ContourValuesList->GetItem(i));
  //  if (value != contour->GetValue(i))
  //    {
  //    modifiedFlag = 1;
  //    }
  //  }
  //if (! modifiedFlag)
  //  {
  //  return;
  //  }


  if (this->ModifiedFlag)
    {  
    if ( ! this->TraceInitialized)
      {
      pvApp->AddTraceEntry("set pv(%s) [$pv(%s) GetPVWidget {%s}]",
                           this->GetTclName(), this->PVSource->GetTclName(),
                           this->Name);
      this->TraceInitialized = 1;
      }

    pvApp->AddTraceEntry("$pv(%s) RemoveAllValues", 
                         this->GetTclName());
    pvApp->BroadcastScript("%s SetNumberOfContours %d",
                           this->PVSource->GetVTKSourceTclName(), numContours);
  
    for (i = 0; i < numContours; i++)
      {
      value = atof(this->ContourValuesList->GetItem(i));
      pvApp->BroadcastScript("%s SetValue %d %f",
                             this->PVSource->GetVTKSourceTclName(),
                             i, value);
      pvApp->AddTraceEntry("$pv(%s) AddValue %f", 
                           this->GetTclName(), value);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::Reset()
{
  int i;
  vtkContourFilter* contour;
  int numContours;
  char newValue[256];
  
  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource not set.");
    return;
    }
  contour = (vtkContourFilter*)this->PVSource->GetVTKSource();
  numContours = contour->GetNumberOfContours();  

  // The widget has bee modified.  
  // Now set the widget back to reflect the contours in the filter.
  this->ContourValuesList->DeleteAll();
  for (i = 0; i < numContours; i++)
    {
    sprintf(newValue, "%f", contour->GetValue(i));
    this->ContourValuesList->AppendUnique(newValue);
    }

  // Since the widget now matches the fitler, it is no longer modified.
  this->ModifiedFlag = 0;
}

