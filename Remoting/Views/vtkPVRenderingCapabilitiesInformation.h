/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderingCapabilitiesInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVRenderingCapabilitiesInformation
 * @brief provides information about rendering capabilities.
 *
 */

#ifndef vtkPVRenderingCapabilitiesInformation_h
#define vtkPVRenderingCapabilitiesInformation_h

#include "vtkPVInformation.h"
#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSmartPointer.h"        // for vtkSmartPointer
#include <string>                   // for std::string

class vtkRenderWindow;

class VTKREMOTINGVIEWS_EXPORT vtkPVRenderingCapabilitiesInformation : public vtkPVInformation
{
public:
  static vtkPVRenderingCapabilitiesInformation* New();
  vtkTypeMacro(vtkPVRenderingCapabilitiesInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum CapabilitiesMask
  {
    NONE = 0,

    /**
     * Indicates if onscreen rendering is possible.
     */
    ONSCREEN_RENDERING = 0x01,

    /**
     * Indicates if headless rendering using OSMesa is possible.
     */
    HEADLESS_RENDERING_USES_OSMESA = 0x04,

    /**
     * Indicates if headless rendering using EGL is possible.
     */
    HEADLESS_RENDERING_USES_EGL = 0x08,

    /**
     * Indicates if any headless rendering is possible.
     */
    HEADLESS_RENDERING = HEADLESS_RENDERING_USES_OSMESA | HEADLESS_RENDERING_USES_EGL,

    /**
     * Indicates if any rendering is possible.
     */
    RENDERING = ONSCREEN_RENDERING | HEADLESS_RENDERING,

    /**
     * If rendering is possible, this indicates that that OpenGL version
     * is adequate for basic rendering requirements.
     * This flag can only be set if `RENDERING` is set too.
     */
    OPENGL = 0x10,
  };

  /**
   * Returns a 32-bit unsigned integer which represents the capabilities.
   * Use CapabilitiesMask to determine which capabilities are supported by all
   * the processes from which the information was gathered.
   */
  vtkGetMacro(Capabilities, vtkTypeUInt32);

  /**
   * Convenience method to check is any of the requested capabilities are supported.
   */
  bool Supports(vtkTypeUInt32 capability) { return (this->Capabilities & capability) != 0; }

  static bool Supports(vtkTypeUInt32 capabilities, vtkTypeUInt32 mask)
  {
    return (mask & capabilities) != 0;
  }

  /**
   * Return local process' capabilities.
   */
  static vtkTypeUInt32 GetLocalCapabilities();

  /**
   * This creates an off-screen render window based on capabilities of the local
   * process. This is useful to query additional OpenGL information, for
   * example.
   *
   * This may create a non-mapped onscreen render window, if ParaView was built
   * with onscreen GL support) or headless off-screen render window, if ParaView
   * was built with headless GL support (e.g. EGL or OSMesa). Headless is
   * preferred, if available.
   *
   * Note the returned window hasn't been created yet, i.e. one may need to call
   * vtkRenderWindow::Render() on it before querying OpenGL information, for
   * example.
   */
  static vtkSmartPointer<vtkRenderWindow> NewOffscreenRenderWindow();

  //@}
  void CopyFromObject(vtkObject*) override;
  void AddInformation(vtkPVInformation*) override;
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;

protected:
  vtkPVRenderingCapabilitiesInformation();
  ~vtkPVRenderingCapabilitiesInformation();

  vtkTypeUInt32 Capabilities;

private:
  vtkPVRenderingCapabilitiesInformation(const vtkPVRenderingCapabilitiesInformation&) = delete;
  void operator=(const vtkPVRenderingCapabilitiesInformation&) = delete;
};

#endif
