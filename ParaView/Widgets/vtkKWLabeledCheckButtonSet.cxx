/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWLabeledCheckButtonSet.cxx
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

#include "vtkKWLabeledCheckButtonSet.h"

#include "vtkKWLabel.h"
#include "vtkKWCheckButtonSet.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLabeledCheckButtonSet );
vtkCxxRevisionMacro(vtkKWLabeledCheckButtonSet, "1.1");

int vtkKWLabeledCheckButtonSetCommand(ClientData cd, Tcl_Interp *interp,
                                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledCheckButtonSet::vtkKWLabeledCheckButtonSet()
{
  this->CommandFunction = vtkKWLabeledCheckButtonSetCommand;

  this->Label = vtkKWLabel::New();
  this->CheckButtonSet = vtkKWCheckButtonSet::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledCheckButtonSet::~vtkKWLabeledCheckButtonSet()
{
  if (this->Label)
    {
    this->Label->Delete();
    this->Label = NULL;
    }

  if (this->CheckButtonSet)
    {
    this->CheckButtonSet->Delete();
    this->CheckButtonSet = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledCheckButtonSet::Create(vtkKWApplication *app)
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledLabel already created");
    return;
    }

  this->SetApplication(app);

  // Create the frame

  this->Script("frame %s", this->GetWidgetName());

  // Create the label and the checkbutton set

  this->Label->SetParent(this);
  this->Label->Create(app, 0);

  this->CheckButtonSet->SetParent(this);
  this->CheckButtonSet->Create(app, 0);

  this->Script("pack %s -side top -anchor nw", 
               this->Label->GetWidgetName());

  this->Script("pack %s -side top -anchor nw -padx 10", 
               this->CheckButtonSet->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWLabeledCheckButtonSet::SetEnabled(int e)
{
  // Propagate first (since objects can be modified externally, they might
  // not be in synch with this->Enabled)

  if (this->IsCreated())
    {
    this->Label->SetEnabled(e);
    this->CheckButtonSet->SetEnabled(e);
    }

  // Then update internal Enabled ivar, although it is not of much use here

  if (this->Enabled == e)
    {
    return;
    }

  this->Enabled = e;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWLabeledCheckButtonSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Label: " << this->Label << endl;
  os << indent << "CheckButtonSet: " << this->CheckButtonSet << endl;
}
