/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputGroupRequirement.cxx
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
#include "vtkPVInputGroupRequirement.h"

#include "vtkObjectFactory.h"
#include "vtkPVSource.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkDataSet.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLPackageParser.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInputGroupRequirement);
vtkCxxRevisionMacro(vtkPVInputGroupRequirement, "1.4");

//----------------------------------------------------------------------------
vtkPVInputGroupRequirement::vtkPVInputGroupRequirement()
{
  this->Quantity = 1;
}

//----------------------------------------------------------------------------
int vtkPVInputGroupRequirement::ReadXMLAttributes(vtkPVXMLElement* element,
                                                  vtkPVXMLPackageParser*)
{
  const char* rAttr;

  rAttr = element->GetAttribute("quantity");
  if (rAttr) 
    {
    if (strcmp(rAttr, "Multiple") == 0)
      {
      this->Quantity = -1;
      }
    else if (strcmp(rAttr, "Single") == 0)
      {
      this->Quantity = 1;
      }
    else 
      {
      this->Quantity = atoi(rAttr);
      }
    } 

  return 1;
}



//----------------------------------------------------------------------------
int vtkPVInputGroupRequirement::GetIsValidInput(vtkPVSource* input, vtkPVSource*)
{
  int num = input->GetNumberOfParts();
  if (this->Quantity == -1 && num > 1)
    {
    return 1;
    }
  if (this->Quantity > 0 && this->Quantity == num)
    {
    return 1;
    }

  return 0;
}



//----------------------------------------------------------------------------
void vtkPVInputGroupRequirement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if (this->Quantity == -1)
    {
    os << indent << "Quantity: Multiple" << endl;
    }
  else
    {  
    os << indent << "Quantity: " << this->Quantity << endl;
    }
}


  



