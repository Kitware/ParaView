/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputRequirement.h
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
// .NAME vtkPVInputRequirement - Restrict allowable input.
// .SECTION Description
// Some filters should not accept inputs without specific attributes.
// An example is Contour requires point scalars.  
// This is a supperclass for objects that describe input requirments.
// New subclasses can be added (and created through XML) for any
// crazy restriction.

#ifndef __vtkPVInputRequirement_h
#define __vtkPVInputRequirement_h

class vtkPVData;
class vtkPVXMLElement;
class vtkPVXMLPackageParser;
class vtkPVDataSetAttributesInformation;
class vtkPVSource;

#include "vtkObject.h"

class VTK_EXPORT vtkPVInputRequirement : public vtkObject
{
public:
  static vtkPVInputRequirement* New();
  vtkTypeRevisionMacro(vtkPVInputRequirement, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method return 1 if the PVData matches the requirement.
  // The pvSource pointer is only used by one requirement so far.
  // vtkDataToDataSetFilters cannot change input types.
  virtual int GetIsValidInput(vtkPVData *pvd, vtkPVSource *pvs);

  // Description:
  // This are used by the field menu to determine is a field
  // should be selectable.
  virtual int GetIsValidField(int field, 
                              vtkPVDataSetAttributesInformation* info);

  // Description:
  // Called by vtkPVXMLPackageParser to configure the widget from XML
  // attributes.
  virtual int ReadXMLAttributes(vtkPVXMLElement* element,
                                vtkPVXMLPackageParser* parser);

protected:
  vtkPVInputRequirement() {};
  ~vtkPVInputRequirement() {};

  vtkPVInputRequirement(const vtkPVInputRequirement&); // Not implemented
  void operator=(const vtkPVInputRequirement&); // Not implemented
};

#endif
