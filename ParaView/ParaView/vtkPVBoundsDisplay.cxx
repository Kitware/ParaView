/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVBoundsDisplay.cxx
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
#include "vtkPVBoundsDisplay.h"
#include "vtkKWLabel.h"
#include "vtkPVInputMenu.h"
#include "vtkPVData.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkPVBoundsDisplay* vtkPVBoundsDisplay::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVBoundsDisplay");
  if(ret)
    {
    return (vtkPVBoundsDisplay*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVBoundsDisplay;
}

//----------------------------------------------------------------------------
int vtkPVBoundsDisplayCommand(ClientData cd, Tcl_Interp *interp,
			     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVBoundsDisplay::vtkPVBoundsDisplay()
{
  this->CommandFunction = vtkPVBoundsDisplayCommand;

  this->Widget = vtkKWBoundsDisplay::New();
  this->InputMenu = NULL;
}

//----------------------------------------------------------------------------
vtkPVBoundsDisplay::~vtkPVBoundsDisplay()
{
  this->Widget->Delete();
  this->Widget = NULL;
  this->SetInputMenu(NULL);
}


//----------------------------------------------------------------------------
void vtkPVBoundsDisplay::Create(vtkKWApplication *app)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("BoundsDisplay already created");
    return;
    }
  this->SetApplication(app);

  this->Script("frame %s  -bd 2 ", this->GetWidgetName());
  this->Widget->SetParent(this);
  this->Widget->Create(app);
  this->Script("pack %s -side top -expand t -fill x", 
               this->Widget->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVBoundsDisplay::Update()
{
  vtkPVData *input;
  float bds[6];
  
  return;

  if (this->InputMenu == NULL)
    {
    vtkErrorMacro("Input menu has not been set.");
    return;
    }

  input = this->InputMenu->GetCurrentValue()->GetPVOutput();
  if (input == NULL)
    {
    bds[0] = bds[2] = bds[4] = VTK_LARGE_FLOAT;
    bds[1] = bds[3] = bds[5] = -VTK_LARGE_FLOAT;
    }
  else
    {
    input->GetBounds(bds);
    }

  this->Widget->SetBounds(bds);
}




