/*=========================================================================

  Program:   ParaView
  Module:    vtkPVArrayInformation.h
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
// .NAME vtkPVArrayInformation - Data array information like type.
// .SECTION Description
// This objects is for eliminating direct access to vtkDataObjects
// by the "client".  Only vtkPVPart and vtkPVProcessModule should access
// the data directly.  At the moment, this object is only a container
// and has no useful methods for operating on data.
// Note:  I could just use vtkDataArray objects and store the range
// as values in the array.  This would eliminate this object.

#ifndef __vtkPVArrayInformation_h
#define __vtkPVArrayInformation_h

#include "vtkPVInformation.h"

class vtkClientServerStream;

class VTK_EXPORT vtkPVArrayInformation : public vtkPVInformation
{
public:
  static vtkPVArrayInformation* New();
  vtkTypeRevisionMacro(vtkPVArrayInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // DataType is the string name of the data type: VTK_FLOAT ...
  // the value "VTK_VOID" means that different processes have different types.
  vtkSetMacro(DataType, int);
  vtkGetMacro(DataType, int);

  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  // Description:
  // Changing the number of components clears the ranges back to the default.
  void SetNumberOfComponents(int numComps);
  vtkGetMacro(NumberOfComponents, int);

  // Description:
  // There is a range for each component.
  // Range for component -1 is the range of the vector magnitude.
  // The number of components should be set before these ranges.
  void SetComponentRange(int comp, double min, double max);
  void SetComponentRange(int comp, double *range)
    { this->SetComponentRange(comp, range[0], range[1]);}
  double *GetComponentRange(int component);
  void GetComponentRange(int comp, double *range);

  // Description:
  // Returns 1 if the array can be combined.
  // It must have the same name and number of components.
  int Compare(vtkPVArrayInformation *info);

  // Description:
  // Merge (union) ranges into this object.
  void AddRanges(vtkPVArrayInformation *info);

  void DeepCopy(vtkPVArrayInformation *info);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*) const;
  virtual void CopyFromStream(const vtkClientServerStream*);

protected:
  vtkPVArrayInformation();
  ~vtkPVArrayInformation();

  int DataType;
  int NumberOfComponents;
  char *Name;
  double *Ranges;

  vtkPVArrayInformation(const vtkPVArrayInformation&); // Not implemented
  void operator=(const vtkPVArrayInformation&); // Not implemented
};

#endif
