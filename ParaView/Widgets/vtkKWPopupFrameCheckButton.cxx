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
#include "vtkKWPopupFrameCheckButton.h"

#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWPopupButton.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWPopupFrameCheckButton );
vtkCxxRevisionMacro(vtkKWPopupFrameCheckButton, "1.1");

int vtkKWPopupFrameCheckButtonCommand(ClientData cd, Tcl_Interp *interp,
                                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWPopupFrameCheckButton::vtkKWPopupFrameCheckButton()
{
  this->CommandFunction = vtkKWPopupFrameCheckButtonCommand;

  this->LinkPopupButtonStateToCheckButton = 0;

  // GUI

  this->CheckButton = vtkKWCheckButton::New();
}

//----------------------------------------------------------------------------
vtkKWPopupFrameCheckButton::~vtkKWPopupFrameCheckButton()
{
  // GUI

  if (this->CheckButton)
    {
    this->CheckButton->Delete();
    this->CheckButton = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWPopupFrameCheckButton::Create(vtkKWApplication *app, 
                                        const char *args)
{
  // Create the superclass widgets

  if (this->IsCreated())
    {
    vtkErrorMacro("PopupFrameCheckButton already created");
    return;
    }

  this->Superclass::Create(app, args);

  // --------------------------------------------------------------
  // Annotation visibility

  if (this->PopupMode)
    {
    this->CheckButton->SetParent(this);
    }
  else
    {
    this->CheckButton->SetParent(this->Frame->GetFrame());
    }

  this->CheckButton->Create(this->Application, "");

  this->CheckButton->SetCommand(this, "CheckButtonCallback");

  if (this->PopupMode)
    {
    this->Script("pack %s -side left -anchor w",
                 this->CheckButton->GetWidgetName());
    this->Script("pack %s -side left -anchor w -fill x -expand t -padx 2",
                 this->PopupButton->GetWidgetName());
    }
  else
    {
    this->Script("pack %s -side top -padx 2 -anchor nw",
                 this->CheckButton->GetWidgetName());
    }

  // --------------------------------------------------------------
  // Update the GUI according to the Ivars

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWPopupFrameCheckButton::Update()
{
  this->UpdateEnableState();

  if (!this->IsCreated())
    {
    return;
    }

  // Check button (GetCheckButtonState() is overriden is subclasses)

  if (this->CheckButton)
    {
    this->CheckButton->SetState(this->GetCheckButtonState());
    }

  // Disable the popup button if not checked

  if (this->LinkPopupButtonStateToCheckButton && 
      this->PopupButton && 
      this->CheckButton && 
      this->CheckButton->IsCreated())
    {
    this->PopupButton->SetEnabled(
      this->CheckButton->GetState() ? this->Enabled : 0);
    }
}

// ----------------------------------------------------------------------------
void vtkKWPopupFrameCheckButton::SetLinkPopupButtonStateToCheckButton(
  int _arg)
{
  if (this->LinkPopupButtonStateToCheckButton == _arg)
    {
    return;
    }

  this->LinkPopupButtonStateToCheckButton = _arg;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWPopupFrameCheckButton::CheckButtonCallback() 
{
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWPopupFrameCheckButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->CheckButton)
    {
    this->CheckButton->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWPopupFrameCheckButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CheckButton: " 
     << this->CheckButton << endl;
  os << indent << "LinkPopupButtonStateToCheckButton: " 
     << (this->LinkPopupButtonStateToCheckButton ? "On" : "Off") << endl;
}
