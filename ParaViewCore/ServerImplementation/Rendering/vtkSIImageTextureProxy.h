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
// .NAME vtkSIImageTextureProxy - proxy for a vtkTexture.
// .SECTION Description
// Combines vtkTexture and vtkNetworkImageSource. This makes it simple for
// applications to create textures from image files where the image file is
// present on only one process, say client, but is needed for rendering on the
// render server and the data server.
#ifndef __vtkSIImageTextureProxy_h
#define __vtkSIImageTextureProxy_h

#include "vtkSISourceProxy.h"

class vtkImageData;

class VTK_EXPORT vtkSIImageTextureProxy : public vtkSISourceProxy
{
public:
  static vtkSIImageTextureProxy* New();
  vtkTypeMacro(vtkSIImageTextureProxy, vtkSISourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSIImageTextureProxy();
  ~vtkSIImageTextureProxy();

  // Description:
  // Creates the VTKObjects. Overridden to add post-filters to the pipeline.
  virtual bool CreateVTKObjects(vtkSMMessage* message);

private:
  vtkSIImageTextureProxy(const vtkSIImageTextureProxy&); // Not implemented
  void operator=(const vtkSIImageTextureProxy&); // Not implemented
//ETX
};

#endif
