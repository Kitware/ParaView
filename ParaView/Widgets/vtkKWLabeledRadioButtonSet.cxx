/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWLabeledRadioButtonSet.cxx
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

#include "vtkKWLabeledRadioButtonSet.h"

#include "vtkKWImageLabel.h"
#include "vtkKWRadioButtonSet.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledRadioButtonSet);
vtkCxxRevisionMacro(vtkKWLabeledRadioButtonSet, "1.4");

int vtkKWLabeledRadioButtonSetCommand(ClientData cd, Tcl_Interp *interp,
                                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledRadioButtonSet::vtkKWLabeledRadioButtonSet()
{
  this->CommandFunction = vtkKWLabeledRadioButtonSetCommand;

  this->PackHorizontally = 0;

  this->RadioButtonSet = vtkKWRadioButtonSet::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledRadioButtonSet::~vtkKWLabeledRadioButtonSet()
{
  if (this->RadioButtonSet)
    {
    this->RadioButtonSet->Delete();
    this->RadioButtonSet = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledRadioButtonSet::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledRadioButtonSet already created");
    return;
    }

  // Call the superclass, this will set the application and create the Label

  this->Superclass::Create(app, args);

  // Create the radiobutton set

  this->RadioButtonSet->SetParent(this);
  this->RadioButtonSet->Create(app, 0);

  // Pack the label and the checkbutton

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledRadioButtonSet::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->Label->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  if (this->PackHorizontally)
    {
    tk_cmd << "pack ";
    if (this->ShowLabel)
      {
      tk_cmd << this->Label->GetWidgetName() << " ";
      }
    tk_cmd << this->RadioButtonSet->GetWidgetName() 
           << " -side left -anchor nw" << endl;
    }
  else
    {
    if (this->ShowLabel)
      {
      tk_cmd << "pack " << this->Label->GetWidgetName() 
             << " -side top -anchor nw" << endl;
      }
    tk_cmd << "pack " << this->RadioButtonSet->GetWidgetName() 
           << " -side top -anchor nw -padx 10" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

// ----------------------------------------------------------------------------
void vtkKWLabeledRadioButtonSet::SetPackHorizontally(int _arg)
{
  if (this->PackHorizontally == _arg)
    {
    return;
    }
  this->PackHorizontally = _arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWLabeledRadioButtonSet::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->RadioButtonSet)
    {
    this->RadioButtonSet->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledRadioButtonSet::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->RadioButtonSet)
    {
    this->RadioButtonSet->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledRadioButtonSet::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->RadioButtonSet)
    {
    this->RadioButtonSet->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledRadioButtonSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "RadioButtonSet: " << this->RadioButtonSet << endl;

  os << indent << "PackHorizontally: " 
     << (this->PackHorizontally ? "On" : "Off") << endl;
}
