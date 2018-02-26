/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDisplayInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVDisplayInformation
 * @brief   provides information about the rendering display and OpenGL context.
 *
 * @deprecated in ParaView 5.5. Please use vtkPVRenderingCapabilitiesInformation
 * instead.
 */

#ifndef vtkPVDisplayInformation_h
#define vtkPVDisplayInformation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVInformation.h"

#if !defined(VTK_LEGACY_REMOVE)

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVDisplayInformation : public vtkPVInformation
{
public:
  VTK_LEGACY(static vtkPVDisplayInformation* New());
  vtkTypeMacro(vtkPVDisplayInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns if the display can be opened up on the current processes.
   */
  VTK_LEGACY(static bool CanOpenDisplayLocally());

  /**
   * Returns true if OpenGL context supports core features required for
   * rendering.
   */
  VTK_LEGACY(static bool SupportsOpenGLLocally());

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) VTK_OVERRIDE;

  /**
   * Merge another information object. Calls AddInformation(info, 0).
   */
  void AddInformation(vtkPVInformation* info) VTK_OVERRIDE;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) VTK_OVERRIDE;
  void CopyFromStream(const vtkClientServerStream*) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * CanOpenDisplay is set to 1 if a window can be opened on
   * the display.
   */
  vtkGetMacro(CanOpenDisplay, int);
  //@}

  //@{
  /**
   * SupportsOpenGL is set to 1 if the OpenGL context available supports core
   * features needed for rendering.
   */
  vtkGetMacro(SupportsOpenGL, int);
  //@}

protected:
  vtkPVDisplayInformation();
  ~vtkPVDisplayInformation() override;

  int CanOpenDisplay;
  int SupportsOpenGL;

private:
  vtkPVDisplayInformation(const vtkPVDisplayInformation&) = delete;
  void operator=(const vtkPVDisplayInformation&) = delete;

  static int GlobalCanOpenDisplayLocally;
  static int GlobalSupportsOpenGL;
};

#endif // !defined(VTK_LEGACY_REMOVE)
#endif
