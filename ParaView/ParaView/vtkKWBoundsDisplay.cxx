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

#include "vtkKWBoundsDisplay.h"

#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWBoundsDisplay);
vtkCxxRevisionMacro(vtkKWBoundsDisplay, "1.11");

int vtkKWBoundsDisplayCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWBoundsDisplay::vtkKWBoundsDisplay()
{
  this->CommandFunction = vtkKWBoundsDisplayCommand;

  this->XRangeLabel = vtkKWLabel::New();
  this->YRangeLabel = vtkKWLabel::New();
  this->ZRangeLabel = vtkKWLabel::New();

  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;
  this->Extent[0] = this->Extent[2] = this->Extent[4] = 0;
  this->Extent[1] = this->Extent[3] = this->Extent[5] = 0;

  this->ExtentMode = 1;
}

//----------------------------------------------------------------------------
vtkKWBoundsDisplay::~vtkKWBoundsDisplay()
{
  this->XRangeLabel->Delete();
  this->XRangeLabel = NULL;
  this->YRangeLabel->Delete();
  this->YRangeLabel = NULL;
  this->ZRangeLabel->Delete();
  this->ZRangeLabel = NULL;
}

//----------------------------------------------------------------------------
void vtkKWBoundsDisplay::Create(vtkKWApplication *app,
                                const char* vtkNotUsed(args))
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("BoundsDisplay already created");
    return;
    }

  this->vtkKWLabeledFrame::Create(app, 0);
  this->SetLabel("Bounds");

  this->XRangeLabel->SetParent(this->GetFrame());
  this->XRangeLabel->Create(app, "");
  this->YRangeLabel->SetParent(this->GetFrame());
  this->YRangeLabel->Create(app, "");
  this->ZRangeLabel->SetParent(this->GetFrame());
  this->ZRangeLabel->Create(app, "");

  this->Script("pack %s %s %s -side top -anchor w", 
               this->XRangeLabel->GetWidgetName(),
               this->YRangeLabel->GetWidgetName(),
               this->ZRangeLabel->GetWidgetName());

  this->UpdateWidgets();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWBoundsDisplay::SetBounds(double bounds[6])
{
  int i;

  this->ExtentMode = 0;
  // Copy to our ivar.
  for (i = 0; i < 6; ++i)
    {
    this->Bounds[i] = bounds[i];
    }
    
  this->UpdateWidgets();
}

//----------------------------------------------------------------------------
void vtkKWBoundsDisplay::SetExtent(int ext[6])
{
  int i;

  this->ExtentMode = 1;
  // Copy to our ivar.
  for (i = 0; i < 6; ++i)
    {
    this->Extent[i] = ext[i];
    }
    
  this->UpdateWidgets();
}

//----------------------------------------------------------------------------
void vtkKWBoundsDisplay::UpdateWidgets()
{
  if (this->ExtentMode)
    {
    if (this->Extent[0] > this->Extent[1] || 
        this->Extent[2] > this->Extent[3] || 
        this->Extent[4] > this->Extent[5]) 
      {
      this->XRangeLabel->SetLabel("Empty extent");
      this->YRangeLabel->SetLabel("");
      this->ZRangeLabel->SetLabel("");
      }
    else
      {
      char tmp[350];
      sprintf(tmp, "X extent: %d to %d (dimension: %d)", 
              this->Extent[0], this->Extent[1], 
              this->Extent[1]-this->Extent[0]+1);
      this->XRangeLabel->SetLabel(tmp);
      sprintf(tmp, "Y extent: %d to %d (dimension: %d)", 
              this->Extent[2], this->Extent[3],
              this->Extent[3]-this->Extent[2]+1);
      this->YRangeLabel->SetLabel(tmp);
      sprintf(tmp, "Z extent: %d to %d (dimension: %d)", 
              this->Extent[4], this->Extent[5],
              this->Extent[5]-this->Extent[4]+1);
      this->ZRangeLabel->SetLabel(tmp);
      }
    }
  else
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
      sprintf(tmp, "X range: %.3f to %.3f (delta: %.3f)", 
              this->Bounds[0], this->Bounds[1], 
              this->Bounds[1] - this->Bounds[0]);
      this->XRangeLabel->SetLabel(tmp);
      sprintf(tmp, "Y range: %.3f to %.3f (delta: %.3f)", 
              this->Bounds[2], this->Bounds[3],
              this->Bounds[3] - this->Bounds[2]);
      this->YRangeLabel->SetLabel(tmp);
      sprintf(tmp, "Z range: %.3f to %.3f (delta: %.3f)", 
              this->Bounds[4], this->Bounds[5],
              this->Bounds[5] - this->Bounds[4]);
      this->ZRangeLabel->SetLabel(tmp);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWBoundsDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if (this->ExtentMode)
    {
    os << indent << "Mode: Extent\n";
    os << indent << "Extent: " << this->Extent[0] << ", " 
                 << this->Extent[1] << ", " << this->Extent[2] << ", "
                 << this->Extent[3] << ", " << this->Extent[4] << ", "
                 << this->Extent[5] << endl;
    }
  else
    {
    os << indent << "Mode: Bounds\n";
    os << indent << "Bounds: " << this->Bounds[0] << ", " 
                 << this->Bounds[1] << ", " << this->Bounds[2] << ", "
                 << this->Bounds[3] << ", " << this->Bounds[4] << ", "
                 << this->Bounds[5] << endl;
    }
}
