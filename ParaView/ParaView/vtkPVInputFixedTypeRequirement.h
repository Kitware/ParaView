/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputFixedTypeRequirement.h
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
// .NAME vtkPVInputFixedTypeRequirement - Type cannot change after input is set.
// .SECTION Description
// Used for vtkDataSetToDataSetFilter. Input type cannot change
// after it is set because the output will change type


    // Well I do not really like this hack, but it will work for DataSetToDataSetFilters.
    // We really need the old input, but I do not know which input it was.
    // Better would have been to passed the input property as an argument.
    // I already spent too long on this, so it will have to wait.

#ifndef __vtkPVInputFixedTypeRequirement_h
#define __vtkPVInputFixedTypeRequirement_h


class vtkDataSet;
class vtkPVData;
class vtkPVDataSetAttributesInformation;

#include "vtkPVInputRequirement.h"

class VTK_EXPORT vtkPVInputFixedTypeRequirement : public vtkPVInputRequirement
{
public:
  static vtkPVInputFixedTypeRequirement* New();
  vtkTypeRevisionMacro(vtkPVInputFixedTypeRequirement, vtkPVInputRequirement);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This method return 1 if the PVData matches the requirement.
  virtual int GetIsValidInput(vtkPVData *pvd, vtkPVSource *pvs);

  // Description:
  // Called by vtkPVXMLPackageParser to configure the widget from XML
  // attributes.
  virtual int ReadXMLAttributes(vtkPVXMLElement* element,
                                vtkPVXMLPackageParser* parser);
  
protected:
  vtkPVInputFixedTypeRequirement();
  ~vtkPVInputFixedTypeRequirement() {};

  vtkPVInputFixedTypeRequirement(const vtkPVInputFixedTypeRequirement&); // Not implemented
  void operator=(const vtkPVInputFixedTypeRequirement&); // Not implemented
};

#endif
