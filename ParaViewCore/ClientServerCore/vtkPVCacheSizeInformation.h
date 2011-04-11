/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCacheSizeInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCacheSizeInformation - information obeject to 
// collect cache size information from a vtkCacheSizeKeeper.
// .SECTION Description
// Gather information about cache size from vtkCacheSizeKeeper.

#ifndef __vtkPVCacheSizeInformation_h
#define __vtkPVCacheSizeInformation_h

#include "vtkPVInformation.h"

class VTK_EXPORT vtkPVCacheSizeInformation : public vtkPVInformation
{
public:
  static vtkPVCacheSizeInformation* New();
  vtkTypeMacro(vtkPVCacheSizeInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);

  //BTX
  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);
  //ETX

  vtkGetMacro(CacheSize, unsigned long);
  vtkSetMacro(CacheSize, unsigned long);
protected:
  vtkPVCacheSizeInformation();
  ~vtkPVCacheSizeInformation();

  unsigned long CacheSize;
private:
  vtkPVCacheSizeInformation(const vtkPVCacheSizeInformation&); // Not implemented.
  void operator=(const vtkPVCacheSizeInformation&); // Not implemented.
};

#endif

