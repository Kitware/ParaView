/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputFixedTypeRequirement.cxx
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
#include "vtkPVInputFixedTypeRequirement.h"

#include "vtkObjectFactory.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVSource.h"
#include "vtkPVPart.h"
#include "vtkDataSet.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLPackageParser.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInputFixedTypeRequirement);
vtkCxxRevisionMacro(vtkPVInputFixedTypeRequirement, "1.2");

//----------------------------------------------------------------------------
vtkPVInputFixedTypeRequirement::vtkPVInputFixedTypeRequirement()
{
}

//----------------------------------------------------------------------------
int vtkPVInputFixedTypeRequirement::ReadXMLAttributes(vtkPVXMLElement*,
                                                      vtkPVXMLPackageParser*)
{
  return 1;
}



//----------------------------------------------------------------------------
int vtkPVInputFixedTypeRequirement::GetIsValidInput(vtkPVData* pvd, vtkPVSource* pvs)
{
  if (pvd == NULL)
    {
    return 0;
    }
  if (pvs == NULL)
    {
    return 1;
    }

  vtkPVData *currentData = pvs->GetPVOutput();
  if (currentData == NULL)
    {
    return 1;
    }

  // Only accept inputs which match part for part.
  // Well I do not really like this hack, but it will work for DataSetToDataSetFilters.
  // We really need the old input, but I do not know which input it was.
  // Better would have been to passed the input property as an argument.
  // I already spent too long on this, so it will have to wait.
  int partIdx, numParts;
  numParts = currentData->GetNumberOfPVParts(); 
  if (pvd->GetNumberOfPVParts() != numParts)
    {
    return 0;
    }
  for (partIdx = 0; partIdx < numParts; ++partIdx)
    {
    vtkPVDataInformation *info1;
    vtkPVDataInformation *info2;
    info1 = pvd->GetPVPart(partIdx)->GetDataInformation();
    info2 = currentData->GetPVPart(partIdx)->GetDataInformation();
    if (info1->GetDataSetType() != info2->GetDataSetType())
      {
      return 0;
      }
    }
 
  return 1;
}



//----------------------------------------------------------------------------
void vtkPVInputFixedTypeRequirement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


  



