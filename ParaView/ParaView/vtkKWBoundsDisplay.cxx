/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWBoundsDisplay.cxx
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
#include "vtkKWApplication.h"
#include "vtkKWBoundsDisplay.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkKWBoundsDisplay* vtkKWBoundsDisplay::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWBoundsDisplay");
  if(ret)
    {
    return (vtkKWBoundsDisplay*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWBoundsDisplay;
}

int vtkKWBoundsDisplayCommand(ClientData cd, Tcl_Interp *interp,
			     int argc, char *argv[]);

vtkKWBoundsDisplay::vtkKWBoundsDisplay()
{
  this->CommandFunction = vtkKWBoundsDisplayCommand;

  this->XRangeLabel = vtkKWLabel::New();
  this->YRangeLabel = vtkKWLabel::New();
  this->ZRangeLabel = vtkKWLabel::New();

  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_LARGE_FLOAT;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;
}

vtkKWBoundsDisplay::~vtkKWBoundsDisplay()
{
  this->XRangeLabel->Delete();
  this->XRangeLabel = NULL;
  this->YRangeLabel->Delete();
  this->YRangeLabel = NULL;
  this->ZRangeLabel->Delete();
  this->ZRangeLabel = NULL;
}


void vtkKWBoundsDisplay::Create(vtkKWApplication *app)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("BoundsDisplay already created");
    return;
    }

  this->vtkKWLabeledFrame::Create(app);
  this->SetLabel("Bounds");

  this->XRangeLabel->SetParent(this->GetFrame());
  this->XRangeLabel->Create(app, "");
  this->YRangeLabel->SetParent(this->GetFrame());
  this->YRangeLabel->Create(app, "");
  this->ZRangeLabel->SetParent(this->GetFrame());
  this->ZRangeLabel->Create(app, "");

  this->Script("pack %s %s %s -side top -expand 1 -fill x", 
               this->XRangeLabel->GetWidgetName(),
               this->YRangeLabel->GetWidgetName(),
               this->ZRangeLabel->GetWidgetName());

  this->UpdateWidgets();
}

void vtkKWBoundsDisplay::SetBounds(float bounds[6])
{
  int i;

  // Copy to our ivar.
  for (i = 0; i < 6; ++i)
    {
    this->Bounds[i] = bounds[i];
    }
    
  this->UpdateWidgets();
}

void vtkKWBoundsDisplay::UpdateWidgets()
{
  if (this->Bounds[0] > this->Bounds[1] || 
      this->Bounds[2] > this->Bounds[3] || 
      this->Bounds[4] > this->Bounds[5]) 
    {
    this->XRangeLabel->SetLabel("Empty bounds");
    this->YRangeLabel->SetLabel("");
    this->ZRangeLabel->SetLabel("");
    }
  else
    {
    char tmp[350];
    sprintf(tmp, "x range: %f to %f", this->Bounds[0],this->Bounds[1]);
    this->XRangeLabel->SetLabel(tmp);
    sprintf(tmp, "y range: %f to %f", this->Bounds[2],this->Bounds[3]);
    this->YRangeLabel->SetLabel(tmp);
    sprintf(tmp, "z range: %f to %f", this->Bounds[4],this->Bounds[5]);
    this->ZRangeLabel->SetLabel(tmp);
    }
}





