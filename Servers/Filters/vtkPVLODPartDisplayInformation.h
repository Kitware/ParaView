/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODPartDisplayInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVLODPartDisplayInformation - Holds memory size of geometry.
// .SECTION Description
// This information object collects the memory size of the high res geometry
// and the LOD geometry.  They are used to determine when to use the
// LOD and when to collect geometry for local rendering.

#ifndef __vtkPVLODPartDisplayInformation_h
#define __vtkPVLODPartDisplayInformation_h

#include "vtkPVInformation.h"

class VTK_EXPORT vtkPVLODPartDisplayInformation : public vtkPVInformation
{
public:
  static vtkPVLODPartDisplayInformation* New();
  vtkTypeRevisionMacro(vtkPVLODPartDisplayInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Memory sizes of full resolution geometry and decimated geometry
  // summed over all processes.
  vtkGetMacro(GeometryMemorySize, int);
  vtkGetMacro(LODGeometryMemorySize, int);

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
  vtkPVLODPartDisplayInformation();
  ~vtkPVLODPartDisplayInformation();

  unsigned long GeometryMemorySize;
  unsigned long LODGeometryMemorySize;

  vtkPVLODPartDisplayInformation(const vtkPVLODPartDisplayInformation&); // Not implemented
  void operator=(const vtkPVLODPartDisplayInformation&); // Not implemented
};

#endif
