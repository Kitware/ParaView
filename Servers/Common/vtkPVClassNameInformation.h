/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClassNameInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVClassNameInformation - Holds class name
// .SECTION Description
// This information object gets the class name of the input VTK object.  This
// is separate from vtkPVDataInformation because it can be determined before
// Update is called and because it operates on any VTK object.

#ifndef __vtkPVClassNameInformation_h
#define __vtkPVClassNameInformation_h

#include "vtkPVInformation.h"

class VTK_EXPORT vtkPVClassNameInformation : public vtkPVInformation
{
public:
  static vtkPVClassNameInformation* New();
  vtkTypeMacro(vtkPVClassNameInformation, vtkPVInformation);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Get class name of VTK object.
  vtkGetStringMacro(VTKClassName);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

protected:
  vtkPVClassNameInformation();
  ~vtkPVClassNameInformation();

  char* VTKClassName;
  vtkSetStringMacro(VTKClassName);
private:
  vtkPVClassNameInformation(const vtkPVClassNameInformation&); // Not implemented
  void operator=(const vtkPVClassNameInformation&); // Not implemented
};

#endif
