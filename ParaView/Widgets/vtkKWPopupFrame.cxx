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
#include "vtkKWPopupFrame.h"

#include "vtkKWLabeledFrame.h"
#include "vtkKWPopupButton.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWPopupFrame );
vtkCxxRevisionMacro(vtkKWPopupFrame, "1.1");

int vtkKWPopupFrameCommand(ClientData cd, Tcl_Interp *interp,
                           int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWPopupFrame::vtkKWPopupFrame()
{
  this->CommandFunction = vtkKWPopupFrameCommand;

  // GUI

  this->PopupMode               = 0;

  this->PopupButton             = NULL;
  this->Frame                   = vtkKWLabeledFrame::New();
}

//----------------------------------------------------------------------------
vtkKWPopupFrame::~vtkKWPopupFrame()
{
  // GUI

  if (this->PopupButton)
    {
    this->PopupButton->Delete();
    this->PopupButton = NULL;
    }

  if (this->Frame)
    {
    this->Frame->Delete();
    this->Frame = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWPopupFrame::Create(vtkKWApplication *app, 
                             const char* vtkNotUsed(args))
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("PopupFrame already created");
    return;
    }

  this->SetApplication(app);

  // --------------------------------------------------------------
  // Create the container

  this->Script("frame %s -borderwidth 0 -relief flat", this->GetWidgetName());

  // --------------------------------------------------------------
  // If in popup mode, create the popup button

  if (this->PopupMode)
    {
    if (!this->PopupButton)
      {
      this->PopupButton = vtkKWPopupButton::New();
      }
    
    this->PopupButton->SetParent(this);
    this->PopupButton->Create(app, 0);
    }

  // --------------------------------------------------------------
  // Create the labeled frame

  if (this->PopupMode)
    {
    this->Frame->ShowHideFrameOff();
    this->Frame->SetParent(this->PopupButton->GetPopupFrame());
    }
  else
    {
    this->Frame->SetParent(this);
    }

  this->Frame->Create(app, 0);

  this->Script("pack %s -side top -anchor nw -fill both -expand y",
               this->Frame->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWPopupFrame::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->PopupButton)
    {
    this->PopupButton->SetEnabled(this->Enabled);
    }

  if (this->Frame)
    {
    this->Frame->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWPopupFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Frame: " << this->Frame << endl;
  os << indent << "PopupMode: " 
     << (this->PopupMode ? "On" : "Off") << endl;
  os << indent << "PopupButton: " << this->PopupButton << endl;
}

