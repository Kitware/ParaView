/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputProperty.h
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
// .NAME vtkPVInputProperty - Holds description of a VTK filters input.
// .SECTION Description
// This is a first attempt at separating a fitlers property from the
// user interface (in this case an input menu).
// Inputs are different from other properties in that they are stored
// in both vtkPVSource and vtkSource.  I may not need an input menu,
// but the input still needs to be set in batch scripts.

// Properties for UI (Properties Page) is an unfortunate KW naming convention.

#ifndef __vtkPVInputProperty_h
#define __vtkPVInputProperty_h


#include "vtkObject.h"
class vtkCollection;
class vtkPVInputRequirement;
class vtkPVDataSetAttributesInformation;
class vtkPVSource;

class VTK_EXPORT vtkPVInputProperty : public vtkObject
{
public:
  static vtkPVInputProperty* New();
  vtkTypeRevisionMacro(vtkPVInputProperty, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  void Copy(vtkPVInputProperty* in);

  // Description:
  // This method return 1 if the PVData matches the property.
  // The pvSource pointer is only used by one requirement so far.
  // vtkDataToDataSetFilters cannot change input types.
  int GetIsValidInput(vtkPVSource *input, vtkPVSource *pvs);

  // Description:
  // This are used by the field menu to determine is a field
  // should be selectable.
  int GetIsValidField(int field, vtkPVDataSetAttributesInformation* info);

  // Description:
  // The name is used to construct methods for setting/adding/getting the input.
  // It is most commonly "Input", but can also be "Source" ...
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);
  
  // Description:
  // The type describes which describes the set on input classes
  // which the input will accept.  The value here is taken from
  // VTK definitions: VTK_DATA_SET, VTK_POINT_DATA, VTK_STRUCTURED_DATA,
  // VTK_POINT_SET, VTK_IMAGE_DATA, VTK_RECTILINEAR_GRID ...
  vtkSetStringMacro(Type);
  vtkGetStringMacro(Type);

  // Description:
  // To restrict inputs by attributes.
  void AddRequirement(vtkPVInputRequirement* ir);

protected:
  vtkPVInputProperty();
  ~vtkPVInputProperty();

  char* Name;
  char* Type;
  vtkCollection* Requirements;

  vtkPVInputProperty(const vtkPVInputProperty&); // Not implemented
  void operator=(const vtkPVInputProperty&); // Not implemented
};

#endif
