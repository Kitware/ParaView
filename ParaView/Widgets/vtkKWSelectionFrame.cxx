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
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWIcon.h"

vtkStandardNewMacro(vtkKWSelectionFrame);
vtkCxxRevisionMacro(vtkKWSelectionFrame, "1.15");

//----------------------------------------------------------------------------
vtkKWSelectionFrame::vtkKWSelectionFrame()
{
  this->TitleBar              = vtkKWFrame::New();
  this->Title                 = vtkKWLabel::New();
  this->SelectionList         = vtkKWMenuButton::New();
  this->TitleBarRightSubframe = vtkKWFrame::New();
  this->BodyFrame             = vtkKWFrame::New();
  
  this->SelectCommand         = NULL;

  this->TitleColor[0]                   = 1.0;
  this->TitleColor[1]                   = 1.0;
  this->TitleColor[2]                   = 1.0;

  this->TitleSelectedColor[0]           = 1.0;
  this->TitleSelectedColor[1]           = 1.0;
  this->TitleSelectedColor[2]           = 1.0;

  this->TitleBackgroundColor[0]         = 0.0;
  this->TitleBackgroundColor[1]         = 0.0;
  this->TitleBackgroundColor[2]         = 0.5;

  this->TitleBackgroundSelectedColor[0] = 0.0;
  this->TitleBackgroundSelectedColor[1] = 0.0;
  this->TitleBackgroundSelectedColor[2] = 0.5;
}

//----------------------------------------------------------------------------
vtkKWSelectionFrame::~vtkKWSelectionFrame()
{
  this->TitleBar->Delete();
  this->Title->Delete();
  this->SelectionList->Delete();
  this->TitleBarRightSubframe->Delete();
  this->BodyFrame->Delete();

  if (this->SelectCommand)
    {
    delete [] this->SelectCommand;
    this->SelectCommand = NULL;
    }
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
  this->Script("frame %s %s -bd 3 -relief ridge", wname, (args ? args : ""));

  // The title bar

  this->TitleBar->SetParent(this);
  this->TitleBar->Create(app, "");

  this->Script("pack %s -side top -fill x -expand no",
               this->TitleBar->GetWidgetName());

  // The selection button

  this->SelectionList->SetParent(this->TitleBar);
  this->SelectionList->Create(app, "");
  this->SelectionList->IndicatorOff();
  this->SelectionList->SetImageOption(vtkKWIcon::ICON_EXPAND);

  this->Script("pack %s -side left -anchor w -fill y -padx 1 -pady 1",
               this->SelectionList->GetWidgetName());

  //cout << this->SelectionList->GetTclName() << endl;

  // The title itself

  this->Title->SetParent(this->TitleBar);
  this->Title->Create(app, "");
  this->Title->SetLabel("<Click to Select>");
  
  this->Script("pack %s -side left -anchor w -fill y",
               this->Title->GetWidgetName());
  
  // The subframe on the right

  this->TitleBarRightSubframe->SetParent(this->TitleBar);
  this->TitleBarRightSubframe->Create(app, "");

  this->Script("pack %s -side right -anchor e -padx 4",
               this->TitleBarRightSubframe->GetWidgetName());
  
  // The body frame

  this->BodyFrame->SetParent(this);
  this->BodyFrame->Create(app, "-bg white");

  this->Script("pack %s -side top -fill both -expand yes",
               this->BodyFrame->GetWidgetName());

  // Update colors

  this->UpdateColors();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitle(const char *title)
{
  this->Title->SetLabel(title);
}

//----------------------------------------------------------------------------
const char* vtkKWSelectionFrame::GetTitle()
{
  return this->Title->GetLabel();
}

//----------------------------------------------------------------------------
int vtkKWSelectionFrame::SetColor(
  float *color, float r, float g, float b)
{
  if ((r == color[0] && g == color[1] &&  b == color[2]) ||
      (r < 0.0 || r > 1.0) || (g < 0.0 || g > 1.0) || (b < 0.0 || b > 1.0))
    {
    return 0;
    }

  color[0] = r;
  color[1] = g;
  color[2] = b;

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitleColor(
  float r, float g, float b)
{
  if (this->SetColor(this->TitleColor, r, g, b))
    {
    this->Modified();
    this->UpdateColors();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitleSelectedColor(
  float r, float g, float b)
{
  if (this->SetColor(this->TitleSelectedColor, r, g, b))
    {
    this->Modified();
    this->UpdateColors();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitleBackgroundColor(
  float r, float g, float b)
{
  if (this->SetColor(this->TitleBackgroundColor, r, g, b))
    {
    this->Modified();
    this->UpdateColors();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetTitleBackgroundSelectedColor(
  float r, float g, float b)
{
  if (this->SetColor(this->TitleBackgroundSelectedColor, r, g, b))
    {
    this->Modified();
    this->UpdateColors();
    }
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::UpdateColors()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->TitleBar->SetBackgroundColor(
    this->TitleBackgroundColor[0], 
    this->TitleBackgroundColor[1], 
    this->TitleBackgroundColor[2]);

  this->Title->SetBackgroundColor(
    this->TitleBackgroundColor[0], 
    this->TitleBackgroundColor[1], 
    this->TitleBackgroundColor[2]);

  this->Title->SetForegroundColor(
    this->TitleColor[0], 
    this->TitleColor[1], 
    this->TitleColor[2]);

  this->TitleBarRightSubframe->SetBackgroundColor(
    this->TitleBackgroundColor[0], 
    this->TitleBackgroundColor[1], 
    this->TitleBackgroundColor[2]);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetSelectionList(int num, const char **list)
{
  if (!this->SelectionList->IsCreated())
    {
    vtkErrorMacro(
      "Selection frame must be created before selection list can be set");
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
void vtkKWSelectionFrame::SetObjectMethodCommand(
  char **command, 
  vtkKWObject *object, 
  const char *method)
{
  if (*command)
    {
    delete [] *command;
    *command = NULL;
    }

  if (!object)
    {
    return;
    }

  ostrstream command_str;
  command_str << object->GetTclName() << " " << method << ends;
  *command = command_str.str();
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SetSelectCommand(vtkKWObject *object,
                                           const char *method)
{
  this->SetObjectMethodCommand(&this->SelectCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWSelectionFrame::SelectionMenuCallback(const char *menuItem)
{
  if (this->SelectCommand)
    {
    this->Script("eval {%s {%s} %s}",
                 this->SelectCommand, menuItem, this->GetTclName());
    }
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
  os << indent << "TitleColor: ("
     << this->TitleColor[0] << ", " 
     << this->TitleColor[1] << ", " 
     << this->TitleColor[2] << ")" << endl;
  os << indent << "TitleSelectedColor: ("
     << this->TitleSelectedColor[0] << ", " 
     << this->TitleSelectedColor[1] << ", " 
     << this->TitleSelectedColor[2] << ")" << endl;
  os << indent << "TitleBackgroundColor: ("
     << this->TitleBackgroundColor[0] << ", " 
     << this->TitleBackgroundColor[1] << ", " 
     << this->TitleBackgroundColor[2] << ")" << endl;
  os << indent << "TitleBackgroundSelectedColor: ("
     << this->TitleBackgroundSelectedColor[0] << ", " 
     << this->TitleBackgroundSelectedColor[1] << ", " 
     << this->TitleBackgroundSelectedColor[2] << ")" << endl;
}

