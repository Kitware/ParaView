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

#include "vtkKWLabeledText.h"

#include "vtkKWImageLabel.h"
#include "vtkKWText.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledText);
vtkCxxRevisionMacro(vtkKWLabeledText, "1.5");

int vtkKWLabeledTextCommand(ClientData cd, Tcl_Interp *interp,
                            int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledText::vtkKWLabeledText()
{
  this->CommandFunction = vtkKWLabeledTextCommand;

  this->Text = vtkKWText::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledText::~vtkKWLabeledText()
{
  if (this->Text)
    {
    this->Text->Delete();
    this->Text = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledText::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledText already created");
    return;
    }

  // Call the superclass, this will set the application and create the Label

  this->Superclass::Create(app, args);

  // Let's make the label slightly smaller

  if (this->Label)
    {
    this->Script("%s configure -bd 1", this->Label->GetWidgetName());
    }

  // Create the option menu

  this->Text->SetParent(this);
  this->Text->Create(app, "");

  // Pack the label and the option menu

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledText::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->Label->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  if (this->ShowLabel)
    {
    tk_cmd << "pack " << this->Label->GetWidgetName() 
           << " -anchor nw -pady 0 -ipady 0" << endl;
    }
  tk_cmd << "pack " << this->Text->GetWidgetName() 
         << " -side top -anchor nw -fill both -expand t" << endl;
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWLabeledText::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->Text)
    {
    this->Text->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledText::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->Text)
    {
    this->Text->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledText::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->Text)
    {
    this->Text->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledText::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Text: " << this->Text << endl;
}

