/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataInformation.h
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
// .NAME vtkPVDataInformation - Light object for holding data information.
// .SECTION Description
// This object is a light weight object.  It has no user interface and
// does not necessarily last a long time.  It is meant to help collect
// information about data object and collections of data objects.  It
// has a PV in the class name because it should never be moved into
// VTK.

#ifndef __vtkPVDataInformation_h
#define __vtkPVDataInformation_h

#include "vtkPVInformation.h"

class vtkDataSet;
class vtkPVDataSetAttributesInformation;
class vtkCollection;

class VTK_EXPORT vtkPVDataInformation : public vtkPVInformation
{
public:
  static vtkPVDataInformation* New();
  vtkTypeRevisionMacro(vtkPVDataInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

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

  // Description:
  // Remove all information.  The next add will be like a copy.
  // I might want to put this in the PVInformation superclass.
  void Initialize();

  // Description:
  // Access to information.
  vtkGetMacro(DataSetType, int);
  const char *GetDataSetTypeAsString();
  int DataSetTypeIsA(const char* type);
  vtkGetMacro(NumberOfPoints, vtkIdType);
  vtkGetMacro(NumberOfCells, vtkIdType);
  vtkGetMacro(MemorySize, int);
  vtkGetVector6Macro(Bounds, double);
  void GetBounds(float* bds);

  // Description:
  // Of course Extent is only valid for structured data sets.
  // Extent is the largest extent that contains all the parts.
  vtkGetVector6Macro(Extent, int);

  // Description:
  // Access to information about point and cell data.
  vtkGetObjectMacro(PointDataInformation,vtkPVDataSetAttributesInformation);
  vtkGetObjectMacro(CellDataInformation,vtkPVDataSetAttributesInformation);

  // Description:
  // Name stored in field data.
  vtkGetStringMacro(Name);

protected:
  vtkPVDataInformation();
  ~vtkPVDataInformation();

  void DeepCopy(vtkPVDataInformation *dataInfo);

  // Data information collected from remote processes.
  int            DataSetType;
  vtkIdType      NumberOfPoints;
  vtkIdType      NumberOfCells;
  int            MemorySize;
  double         Bounds[6];
  int            Extent[6];
  char*          Name;
  vtkSetStringMacro(Name);

  vtkPVDataSetAttributesInformation* PointDataInformation;
  vtkPVDataSetAttributesInformation* CellDataInformation;

  vtkPVDataInformation(const vtkPVDataInformation&); // Not implemented
  void operator=(const vtkPVDataInformation&); // Not implemented
};

#endif
