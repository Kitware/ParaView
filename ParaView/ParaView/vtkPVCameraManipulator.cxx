/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraManipulator.cxx
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
#include "vtkPVCameraManipulator.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkKWApplication.h"
#include "vtkLight.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

vtkCxxRevisionMacro(vtkPVCameraManipulator, "1.5");
vtkStandardNewMacro(vtkPVCameraManipulator);

vtkCxxSetObjectMacro(vtkPVCameraManipulator,Application,vtkKWApplication);

//-------------------------------------------------------------------------
vtkPVCameraManipulator::vtkPVCameraManipulator()
{
  this->Button = 1;
  this->Shift = 0;
  this->Control = 0;

  this->LastX = this->LastY = 0;

  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->DisplayCenter[0] = this->DisplayCenter[1] = 0.0;
  this->Application = 0;

  this->ManipulatorName = 0;
}

//-------------------------------------------------------------------------
vtkPVCameraManipulator::~vtkPVCameraManipulator()
{
  this->SetApplication(0);
  this->SetManipulatorName(0);
}

//-------------------------------------------------------------------------
void vtkPVCameraManipulator::OnButtonDown(int, int, vtkRenderer*,
                                          vtkRenderWindowInteractor*)
{
}


//-------------------------------------------------------------------------
void vtkPVCameraManipulator::OnButtonUp(int, int, vtkRenderer*,
                                        vtkRenderWindowInteractor*)
{
}

//-------------------------------------------------------------------------
void vtkPVCameraManipulator::OnMouseMove(int, int, vtkRenderer*,
                                         vtkRenderWindowInteractor*)
{
}

//-------------------------------------------------------------------------
void vtkPVCameraManipulator::ComputeDisplayCenter(vtkRenderer *ren)
{
  float *pt;

  // save the center of rotation in screen coordinates
  ren->SetWorldPoint(this->Center[0],
                     this->Center[1],
                     this->Center[2], 1.0);
  ren->WorldToDisplay();
  pt = ren->GetDisplayPoint();
  this->DisplayCenter[0] = pt[0];
  this->DisplayCenter[1] = pt[1];
}

//-------------------------------------------------------------------------
void vtkPVCameraManipulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ManipulatorName: " 
     << (this->ManipulatorName?this->ManipulatorName:"none") << endl;
  os << indent << "Button: " << this->Button << endl;
  os << indent << "Shift: " << this->Shift << endl;
  os << indent << "Control: " << this->Control << endl;
  
  os << indent << "Center: " << this->Center[0] << ", " 
     << this->Center[1] << ", " << this->Center[2] << endl;
  os << indent << "Application: " << this->Application << endl;
}






