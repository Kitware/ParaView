/*=========================================================================

  Program:   ParaView
  Module:    vtkKWBoundsDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWBoundsDisplay.h"

#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWBoundsDisplay);
vtkCxxRevisionMacro(vtkKWBoundsDisplay, "1.12");

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
