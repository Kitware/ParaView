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
// This object is a light weight object. It has no user interface and
// does not necessarily last a long time.  It is meant to help
// collect information about data object and collections of data objects.
// It has a PV in the class name because it should never be moved into
// VTK.

// Note:  It would be nice to use a vtkDataSet object to store all of this
// information.  It already has the structure.  I do not know how I would
// store the information (number of points, bounds ...) with out storing
// the actual points and cells.


#ifndef __vtkPVDataInformation_h
#define __vtkPVDataInformation_h


#include "vtkObject.h"

class vtkDataSet;
class vtkPVDataSetAttributesInformation;
class vtkCollection;

class VTK_EXPORT vtkPVDataInformation : public vtkObject
{
public:
  static vtkPVDataInformation* New();
  vtkTypeRevisionMacro(vtkPVDataInformation, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single vtk data object into
  // this object. Note:  CopyFromData does not set 
  // the GeometryMemorySize or LODMemorySize.
  void CopyFromData(vtkDataSet* data);
  void DeepCopy(vtkPVDataInformation* info);

  // Description:
  // Intersect information of argument with information currently
  // in this object.  Arrays must be in both 
  // (same name and number of components)to be in final.        
  void AddInformation(vtkDataSet* data);
  void AddInformation(vtkPVDataInformation* info);
  
  // Description:
  // Remove all infommation. next add will be like a copy.
  void Initialize();

  // Description:
  // Access to information.
  vtkGetMacro(DataSetType, int);
  const char *GetDataSetTypeAsString();
  int DataSetTypeIsA(const char* type);
  vtkGetMacro(NumberOfPoints, vtkIdType);
  vtkGetMacro(NumberOfCells, vtkIdType);
  vtkGetMacro(MemorySize, unsigned long);
  vtkGetMacro(GeometryMemorySize, unsigned long);
  vtkGetMacro(LODMemorySize, unsigned long);
  vtkGetVector6Macro(Bounds, double);
  void GetBounds(float* bds);

  // Description:
  // These values are not copied from data so they have to be set
  // by the process module.
  vtkSetMacro(GeometryMemorySize,unsigned long);
  vtkSetMacro(LODMemorySize,unsigned long);

  // Description:
  // Of course Extent is only valid for structured data sets.
  // Extent is the largest extent that contains all the parts.
  vtkGetVector6Macro(Extent, int);

  // Description:
  // Access to information about point and cell data.
  vtkGetObjectMacro(PointDataInformation,vtkPVDataSetAttributesInformation);
  vtkGetObjectMacro(CellDataInformation,vtkPVDataSetAttributesInformation);

  // Description:
  // Methods used to send and receive these objects.
  // The user must delete the string returned by new message.
  // The argument length returns the length of the binary message.
  // No byte swapping is currently implemented.
  unsigned char *NewMessage(int &length);
  void CopyFromMessage(unsigned char *msg);

  // Description:
  // Name stored in field data.
  vtkGetStringMacro(Name);

protected:
  vtkPVDataInformation();
  ~vtkPVDataInformation();

  // Data information collected from remote processes.
  int            DataSetType;
  vtkIdType      NumberOfPoints;
  vtkIdType      NumberOfCells;
  unsigned long  MemorySize;
  unsigned long  GeometryMemorySize;
  unsigned long  LODMemorySize;
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
