/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContour.cxx
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
#include "vtkPVContour.h"

#include "vtkContourFilter.h"
#include "vtkDataSetAttributes.h"
#include "vtkKWCompositeCollection.h"
#include "vtkKWFrame.h"
#include "vtkKWMessageDialog.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVWindow.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVContour);
vtkCxxRevisionMacro(vtkPVContour, "1.60");

//----------------------------------------------------------------------------
int vtkPVContourCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVContour::vtkPVContour()
{
  this->CommandFunction = vtkPVContourCommand;
  
  this->ReplaceInputOff();
}

//----------------------------------------------------------------------------
vtkPVContour::~vtkPVContour()
{
}

//----------------------------------------------------------------------------
void vtkPVContour::CreateProperties()
{
  vtkContourFilter* contour = vtkContourFilter::SafeDownCast(this->GetVTKSource());
  if (contour)
    {
    contour->SetNumberOfContours(0);
    }

  this->Superclass::CreateProperties();

  // We update the input menu to make sure it has a valid value.
  // This value is used by the array menu when checking for arrays
  // (in VerifyInput())
  vtkPVWidget* input = this->GetPVWidget("Input");
  if (input)
    {
    input->Reset();
    }
  this->VerifyInput();
}

//----------------------------------------------------------------------------
// Print a warning if input has no scalars
void vtkPVContour::VerifyInput()
{
  vtkPVData* input = this->GetPVInput(0);
  if (!input)
    {
    return;
    }

  vtkPVArrayMenu* arrayMenu = vtkPVArrayMenu::SafeDownCast(
    this->GetPVWidget("Scalars"));
  if (arrayMenu && arrayMenu->GetApplication())
    {
    arrayMenu->Reset();
    if (arrayMenu->GetValue() == NULL)
      {
      vtkKWMessageDialog::PopupMessage(
        this->Application, this->GetPVApplication()->GetMainWindow(), 
        "Warning", 
        "Input does not have scalars to contour.",
        vtkKWMessageDialog::WarningIcon);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVContour::SetPVInput(int idx, vtkPVData *input)
{
  if (idx != 0)
    {
    vtkErrorMacro("Contour has only one input.");
    return;
    }
  if (input == this->GetPVInput(0))
    {
    return;
    }

  this->vtkPVSource::SetPVInput(idx, input);
  this->VerifyInput();
}

//----------------------------------------------------------------------------
void vtkPVContour::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
