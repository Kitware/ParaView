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

#include "vtkKWLabeledWidget.h"

#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledWidget);
vtkCxxRevisionMacro(vtkKWLabeledWidget, "1.10");

int vtkKWLabeledWidgetCommand(ClientData cd, Tcl_Interp *interp,
                              int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledWidget::vtkKWLabeledWidget()
{
  this->CommandFunction = vtkKWLabeledWidgetCommand;

  this->ShowLabel = 1;

  this->Label = vtkKWLabel::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledWidget::~vtkKWLabeledWidget()
{
  if (this->Label)
    {
    this->Label->Delete();
    this->Label = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledWidget::Create(vtkKWApplication *app, const char *args)
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledWidget widget already created");
    return;
    }

  this->SetApplication(app);

  // Create the container frame

  this->Script("frame %s -relief flat -bd 0 -highlightthickness 0 %s", 
               this->GetWidgetName(), args ? args : "");

  // Create the label. If the parent has been set before (i.e. by the subclass)
  // do not set it.
  
  if (!this->Label->GetParent())
    {
    this->Label->SetParent(this);
    }

  this->Label->Create(app, "-anchor w");
  // -bd 0 -highlightthickness 0 -padx 0 -pady 0");

  // Subclasses will call this->Pack() here
  // this->Pack();

  // Update enable state
  
  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWLabeledWidget::SetLabel(const char *text)
{
  if (this->Label)
    {
    this->Label->SetLabel(text);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledWidget::SetLabelWidth(int width)
{
  if (this->Label)
    {
    this->Label->SetWidth(width);
    }
}

//----------------------------------------------------------------------------
int vtkKWLabeledWidget::GetLabelWidth()
{
  if (this->Label)
    {
    return this->Label->GetWidth();
    }
  return 0;
}

// ----------------------------------------------------------------------------
void vtkKWLabeledWidget::SetShowLabel(int _arg)
{
  if (this->ShowLabel == _arg)
    {
    return;
    }
  this->ShowLabel = _arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWLabeledWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->Label)
    {
    this->Label->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledWidget::SetBalloonHelpString(const char *string)
{
  if (this->Label)
    {
    this->Label->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledWidget::SetBalloonHelpJustification(int j)
{
  if (this->Label)
    {
    this->Label->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Label: " << this->Label << endl;

  os << indent << "ShowLabel: " 
     << (this->ShowLabel ? "On" : "Off") << endl;
}

