/*=========================================================================

  Program:   ParaView
  Module:    vtkSMImageTextureProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMImageTextureProxy - proxy for a vtkTexture.
// .SECTION Description
// Combines vtkTexture and vtkNetworkImageSource. This makes it simple for
// applications to create textures from image files where the image file is
// present on only one process, say client, but is needed for rendering on the
// render server and the data server.
#ifndef __vtkSMImageTextureProxy_h
#define __vtkSMImageTextureProxy_h

#include "vtkSMSourceProxy.h"

class vtkImageData;

class VTK_EXPORT vtkSMImageTextureProxy : public vtkSMSourceProxy
{
public:
  static vtkSMImageTextureProxy* New();
  vtkTypeMacro(vtkSMImageTextureProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the client side image if one has been loaded successfully.
  vtkImageData* GetLoadedImage();

//BTX
protected:
  vtkSMImageTextureProxy();
  ~vtkSMImageTextureProxy();

  virtual void CreateVTKObjects();

private:
  vtkSMImageTextureProxy(const vtkSMImageTextureProxy&); // Not implemented
  void operator=(const vtkSMImageTextureProxy&); // Not implemented
//ETX
};

#endif

