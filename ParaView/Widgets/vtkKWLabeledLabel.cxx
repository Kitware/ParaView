/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWLabeledLabel.cxx
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

#include "vtkKWLabeledLabel.h"

#include "vtkObjectFactory.h"
#include "vtkKWLabel.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLabeledLabel );
vtkCxxRevisionMacro(vtkKWLabeledLabel, "1.3");

int vtkKWLabeledLabelCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledLabel::vtkKWLabeledLabel()
{
  this->CommandFunction = vtkKWLabeledLabelCommand;

  this->Label1 = vtkKWLabel::New();
  this->Label2 = vtkKWLabel::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledLabel::~vtkKWLabeledLabel()
{
  this->Label1->Delete();
  this->Label1 = NULL;

  this->Label2->Delete();
  this->Label2 = NULL;
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabel::Create(vtkKWApplication *app)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledLabel already created");
    return;
    }

  this->SetApplication(app);

  // Create the top level

  const char *wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);

  this->Label1->SetParent(this);
  this->Label1->Create(app, "");

  this->Label2->SetParent(this);
  this->Label2->Create(app, "");

  this->Script("pack %s %s -side left -anchor nw", 
               this->Label1->GetWidgetName(),
               this->Label2->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWLabeledLabel::SetEnabled(int e)
{
  // Propagate first (since objects can be modified externally, they might
  // not be in synch with this->Enabled)

  if (this->IsCreated())
    {
    this->Label1->SetEnabled(e);
    this->Label2->SetEnabled(e);
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
void vtkKWLabeledLabel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Label1: " << this->Label1 << endl;
  os << indent << "Label2: " << this->Label2 << endl;
}
