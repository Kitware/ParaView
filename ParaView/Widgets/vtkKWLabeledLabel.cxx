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

#include "vtkKWLabeledLabel.h"

#include "vtkKWImageLabel.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledLabel);
vtkCxxRevisionMacro(vtkKWLabeledLabel, "1.12");

int vtkKWLabeledLabelCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledLabel::vtkKWLabeledLabel()
{
  this->CommandFunction = vtkKWLabeledLabelCommand;

  this->PackHorizontally = 1;
  this->ExpandLabel2     = 0;
  this->LabelAnchor      = vtkKWWidget::ANCHOR_NW;

  this->Label2 = vtkKWLabel::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledLabel::~vtkKWLabeledLabel()
{
  if (this->Label2)
    {
    this->Label2->Delete();
    this->Label2 = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabel::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledLabel already created");
    return;
    }

  // Call the superclass, this will set the application and create the Label

  this->Superclass::Create(app, args);

  // Create the 2nd label

  this->Label2->SetParent(this);
  this->Label2->Create(app, "-anchor nw");

  // Pack the labels

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledLabel::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->Label->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  tk_cmd << this->Label2->GetWidgetName() << " config -anchor " 
         << this->GetAnchorAsString(this->LabelAnchor) << endl;

  if (this->PackHorizontally)
    {
    if (this->ShowLabel)
      {
      tk_cmd << "pack " << this->Label->GetWidgetName() 
             << " -side left -anchor " 
             << this->GetAnchorAsString(this->LabelAnchor) << endl;
      }
    tk_cmd << "pack " << this->Label2->GetWidgetName() 
           << " -side left -anchor nw -fill both -expand " 
           << (this->ExpandLabel2 ? "y" : "n") << endl;
    }
  else
    {
    if (this->ShowLabel)
      {
      tk_cmd << "pack " << this->Label->GetWidgetName() 
             << " -side top -anchor " 
             << this->GetAnchorAsString(this->LabelAnchor) << endl;
      }
    tk_cmd << "pack " << this->Label2->GetWidgetName() 
           << " -side top -anchor nw -padx 10 -fill both -expand "
           << (this->ExpandLabel2 ? "y" : "n") << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

// ----------------------------------------------------------------------------
void vtkKWLabeledLabel::SetPackHorizontally(int _arg)
{
  if (this->PackHorizontally == _arg)
    {
    return;
    }
  this->PackHorizontally = _arg;
  this->Modified();

  this->Pack();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledLabel::SetExpandLabel2(int _arg)
{
  if (this->ExpandLabel2 == _arg)
    {
    return;
    }
  this->ExpandLabel2 = _arg;
  this->Modified();

  this->Pack();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledLabel::SetLabelAnchor(int _arg)
{
  if (this->LabelAnchor == _arg)
    {
    return;
    }
  this->LabelAnchor = _arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabel::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->Label2)
    {
    this->Label2->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabel::SetLabel2(const char *text)
{
  if (this->Label2)
    {
    this->Label2->SetLabel(text);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledLabel::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->Label2)
    {
    this->Label2->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledLabel::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->Label2)
    {
    this->Label2->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Label2: " << this->Label2 << endl;

  os << indent << "PackHorizontally: " 
     << (this->PackHorizontally ? "On" : "Off") << endl;

  os << indent << "ExpandLabel2: " 
     << (this->ExpandLabel2 ? "On" : "Off") << endl;

  os << indent << "LabelAnchor: " 
     << this->GetAnchorAsString(this->LabelAnchor) << endl;
}

