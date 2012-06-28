/*=========================================================================

  Program:   ParaView
  Module:    vtkPVOpenGLExtensionsInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVOpenGLExtensionsInformation - Information object
// to obtain information about OpenGL extensions.
// .SECTION Description
// Information object that can be used to obtain OpenGL extension
// information. The object from which the information is obtained
// should be a render window.
#ifndef __vtkPVOpenGLExtensionsInformation_h
#define __vtkPVOpenGLExtensionsInformation_h

#include "vtkPVInformation.h"

class vtkPVOpenGLExtensionsInformationInternal;

class VTK_EXPORT vtkPVOpenGLExtensionsInformation : public vtkPVInformation
{
public:
  static vtkPVOpenGLExtensionsInformation* New();
  vtkTypeMacro(vtkPVOpenGLExtensionsInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Returns if the given extension is supported.
  bool ExtensionSupported(const char* ext);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);
protected:
  vtkPVOpenGLExtensionsInformation();
  ~vtkPVOpenGLExtensionsInformation();

private:
  vtkPVOpenGLExtensionsInformation(const vtkPVOpenGLExtensionsInformation&); // Not implemented.
  void operator=(const vtkPVOpenGLExtensionsInformation&); // Not implemented.

  vtkPVOpenGLExtensionsInformationInternal* Internal;

};

#endif
