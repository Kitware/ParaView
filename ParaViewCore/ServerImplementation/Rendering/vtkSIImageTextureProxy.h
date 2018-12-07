/*=========================================================================

  Program:   ParaView
  Module:    vtkSIImageTextureProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSIImageTextureProxy
 * @brief   proxy for a vtkTexture.
 *
 * Combines vtkTexture and vtkNetworkImageSource. This makes it simple for
 * applications to create textures from image files where the image file is
 * present on only one process, say client, but is needed for rendering on the
 * render server and the data server.
*/

#ifndef vtkSIImageTextureProxy_h
#define vtkSIImageTextureProxy_h

#include "vtkPVServerImplementationRenderingModule.h" //needed for exports
#include "vtkSISourceProxy.h"

class VTKPVSERVERIMPLEMENTATIONRENDERING_EXPORT vtkSIImageTextureProxy : public vtkSISourceProxy
{
public:
  static vtkSIImageTextureProxy* New();
  vtkTypeMacro(vtkSIImageTextureProxy, vtkSISourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSIImageTextureProxy();
  ~vtkSIImageTextureProxy() override;

  /**
   * Overridden to hookup the image source with the Texture, if available.
   */
  void OnCreateVTKObjects() override;

private:
  vtkSIImageTextureProxy(const vtkSIImageTextureProxy&) = delete;
  void operator=(const vtkSIImageTextureProxy&) = delete;
};

#endif
