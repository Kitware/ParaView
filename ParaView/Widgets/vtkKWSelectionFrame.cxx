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
#include "vtkKWSelectionFrame.h"
#include "vtkObjectFactory.h"

#include "vtkKWApplication.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"

#include "Resources/vtkKWArrowDown.h"

vtkStandardNewMacro(vtkKWSelectionFrame);
vtkCxxRevisionMacro(vtkKWSelectionFrame, "1.12");

//----------------------------------------------------------------------------
vtkKWSelectionFrame::vtkKWSelectionFrame()
{
  this->TitleBar              = vtkKWWidget::New();
  this->Title                 = vtkKWLabel::New();
  this->SelectionList         = vtkKWMenuButton::New();
  this->TitleBarRightSubframe = vtkKWWidget::New();
  
  this->BodyFrame             = vtkKWWidget::New();
  
  this->SelectObject = NULL;
  this->SelectMethod = NULL;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame::~vtkKWSelectionFrame()
{
  this->SetSelectObject(NULL);
  this->SetSelectMethod(NULL);

  this->TitleBar->Delete();
  this->Title->Delete();
  this->SelectionList->Delete();
  this->TitleBarRightSubframe->Delete();
  this->BodyFrame->Delete();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::Create(vtkKWApplication *app, const char *args)
{
  const char *wname;
  
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("Selection frame already created");
    return;
    }
  
  this->SetApplication(app);
  
  // create the top level

  wname = this->GetWidgetName();
  this->Script("frame %s %s -bd 3 -relief ridge", wname, args);

  this->TitleBar->SetParent(this);
  this->TitleBar->Create(app, "frame", "-bg #008");
  
  this->SelectionList->SetParent(this->TitleBar);
  this->SelectionList->Create(app, "-indicatoron 0");
  this->SelectionList->SetImageOption(image_KWArrowDown, 
                                      image_KWArrowDown_width,
                                      image_KWArrowDown_height,
                                      image_KWArrowDown_pixel_size,
                                      image_KWArrowDown_buffer_length);

  this->Title->SetParent(this->TitleBar);
  this->Title->Create(app, "-bg #008 -fg #fff");
  this->Title->SetLabel("<Click to Select>");
  
  this->Script("pack %s %s -side left -anchor w -fill y",
               this->SelectionList->GetWidgetName(),
               this->Title->GetWidgetName());
  
  this->TitleBarRightSubframe->SetParent(this->TitleBar);
  this->TitleBarRightSubframe->Create(app, "frame", "-bg #008");
  this->Script("pack %s -side right -anchor e -padx 4",
               this->TitleBarRightSubframe->GetWidgetName());
  
  this->BodyFrame->SetParent(this);
  this->BodyFrame->Create(app, "frame", "-bg white");
  this->Script("pack %s -side top -fill x -expand no",
               this->TitleBar->GetWidgetName());
  this->Script("pack %s -side top -fill both -expand yes",
               this->BodyFrame->GetWidgetName());
  
  this->SetSelectObject(NULL);
  this->SetSelectMethod(NULL);

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitle(const char *title)
{
  if ( ! this->Title->IsCreated() )
    {
    vtkErrorMacro("Selection frame must be created before title can be set");
    return;
    }
  
  this->Title->SetLabel(title);
}

//----------------------------------------------------------------------------
const char* vtkKWSelectionFrame::GetTitle()
{
  return this->Title->GetLabel();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetSelectionList(int num, const char **list)
{
  if ( ! this->SelectionList->IsCreated() )
    {
    vtkErrorMacro("Selection frame must be created before selection list can be set");
    return;
    }
  
  this->SelectionList->GetMenu()->DeleteAllMenuItems();
  
  int i;
  char *cbk;
  
  for (i = 0; i < num; i++)
    {
    cbk = new char[strlen(list[i]) + 25];
    sprintf(cbk, "SelectionMenuCallback {%s}", list[i]);
    this->SelectionList->AddCommand(list[i], this, cbk);
    delete [] cbk;
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetSelectCommand(vtkKWObject *object,
                                           const char *methodAndArgString)
{
  this->SetSelectObject(object);
  this->SetSelectMethod(methodAndArgString);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SelectionMenuCallback(const char *menuItem)
{
  if ( ! (this->SelectObject && this->SelectMethod) )
    {
    return;
    }
  
  this->Script("%s %s {%s} %s",
               this->SelectObject->GetTclName(), this->SelectMethod,
               menuItem, this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetSelectObject(vtkKWObject *object)
{
  // avoiding reference-counting loops
  this->SelectObject = object;
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->TitleBar)
    {
    this->TitleBar->SetEnabled(this->Enabled);
    }

  if (this->SelectionList)
    {
    this->SelectionList->SetEnabled(this->Enabled);
    }

  if (this->Title)
    {
    this->Title->SetEnabled(this->Enabled);
    }

  if (this->TitleBarRightSubframe)
    {
    this->TitleBarRightSubframe->SetEnabled(this->Enabled);
    }

  if (this->BodyFrame)
    {
    this->BodyFrame->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "BodyFrame: " << this->BodyFrame << endl;
  os << indent << "TitleBarRightSubframe: " << this->TitleBarRightSubframe
     << endl;
}

