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
/**
 * @class   vtkPVOpenGLExtensionsInformation
 * @brief   Information object
 * to obtain information about OpenGL extensions.
 *
 * Information object that can be used to obtain OpenGL extension
 * information. The object from which the information is obtained
 * should be a render window.
*/

#ifndef vtkPVOpenGLExtensionsInformation_h
#define vtkPVOpenGLExtensionsInformation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVInformation.h"

class vtkPVOpenGLExtensionsInformationInternal;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVOpenGLExtensionsInformation
  : public vtkPVInformation
{
public:
  static vtkPVOpenGLExtensionsInformation* New();
  vtkTypeMacro(vtkPVOpenGLExtensionsInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Transfer information about a single object into this object.
   */
  virtual void CopyFromObject(vtkObject*) VTK_OVERRIDE;

  /**
   * Returns if the given extension is supported.
   */
  bool ExtensionSupported(const char* ext);

  //@{
  /**
   * Manage a serialized version of the information.
   */
  virtual void CopyToStream(vtkClientServerStream*) VTK_OVERRIDE;
  virtual void CopyFromStream(const vtkClientServerStream*) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Merge another information object.
   */
  virtual void AddInformation(vtkPVInformation*) VTK_OVERRIDE;

protected:
  vtkPVOpenGLExtensionsInformation();
  ~vtkPVOpenGLExtensionsInformation();
  //@}

private:
  vtkPVOpenGLExtensionsInformation(const vtkPVOpenGLExtensionsInformation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVOpenGLExtensionsInformation&) VTK_DELETE_FUNCTION;

  vtkPVOpenGLExtensionsInformationInternal* Internal;
};

#endif
