/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPassThrough.cxx
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
#include "vtkPVPassThrough.h"
#include "vtkObjectFactory.h"
#include "vtkSource.h"
#include "vtkPVData.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPassThrough);
vtkCxxRevisionMacro(vtkPVPassThrough, "1.2");

int vtkPVPassThroughCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVPassThrough::vtkPVPassThrough()
{
  this->OutputNumber = 0;
}

//----------------------------------------------------------------------------
vtkPVPassThrough::~vtkPVPassThrough()
{

}



//----------------------------------------------------------------------------
void vtkPVPassThrough::SaveInTclScript(ofstream *file, int interactiveFlag, 
                                 int vtkFlag)
{
  int i;

  // Detect special sources we do not handle yet.
  if (this->GetVTKSource() == NULL)
    {
    return;
    }

  // This should not be needed, but We can check anyway.
  if (this->VisitedFlag != 0)
    {
    return;
    }


  // This is the recursive part.
  this->VisitedFlag = 1;
  // Loop through all of the inputs
  for (i = 0; i < this->NumberOfPVInputs; ++i)
    {
    if (this->PVInputs[i] && this->PVInputs[i]->GetPVSource()->GetVisitedFlag() != 2)
      {
      this->PVInputs[i]->GetPVSource()->SaveInTclScript(file, interactiveFlag, vtkFlag);
      }
    }
   
  // Save the object in the script.
  *file << "\n" << this->GetVTKSource()->GetClassName()
        << " " << this->GetVTKSourceTclName() << "\n";
 
  // Set the input to the passthrough filter.
  vtkPVData *inputData = this->GetPVInput();
  if (inputData)
    { // This test should not be necessary, but lets be extra careful.
    vtkPVSource *inputSource = inputData->GetPVSource();
    if (inputSource)
      { // This test should not be necessary, but lets be extra careful.
      *file << "\t" << this->GetVTKSourceTclName() << " SetInput ["
            << inputSource->GetVTKSourceTclName() << " GetOutput " 
            << this->OutputNumber << "]\n";
      }
    }

 // Add the mapper, actor, scalar bar actor ...
  this->GetPVOutput()->SaveInTclScript(file, interactiveFlag, vtkFlag);
}

//----------------------------------------------------------------------------
void vtkPVPassThrough::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "OutputNumber: " << this->OutputNumber << endl;
}

