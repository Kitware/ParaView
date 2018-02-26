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
 *
 * @deprecated in ParaView 5.5. The information is now indirectly available via
 * vtkPVOpenGLInformation. See `vtkPVOpenGLInformation::GetCapabilities`.
*/

#ifndef vtkPVOpenGLExtensionsInformation_h
#define vtkPVOpenGLExtensionsInformation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVInformation.h"

#if !defined(VTK_LEGACY_REMOVE)

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
  void CopyFromObject(vtkObject*) VTK_OVERRIDE;

  /**
   * Returns if the given extension is supported.
   */
  bool ExtensionSupported(const char* ext);

  //@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) VTK_OVERRIDE;
  void CopyFromStream(const vtkClientServerStream*) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Merge another information object.
   */
  void AddInformation(vtkPVInformation*) VTK_OVERRIDE;

protected:
  vtkPVOpenGLExtensionsInformation();
  ~vtkPVOpenGLExtensionsInformation() override;
  //@}

private:
  vtkPVOpenGLExtensionsInformation(const vtkPVOpenGLExtensionsInformation&) = delete;
  void operator=(const vtkPVOpenGLExtensionsInformation&) = delete;

  vtkPVOpenGLExtensionsInformationInternal* Internal;
};

#endif // !defined(VTK_LEGACY_REMOVE)
#endif
