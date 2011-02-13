/*=========================================================================

  Program:   ParaView
  Module:    vtkPMImageTextureProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPMImageTextureProxy - proxy for a vtkTexture.
// .SECTION Description
// Combines vtkTexture and vtkNetworkImageSource. This makes it simple for
// applications to create textures from image files where the image file is
// present on only one process, say client, but is needed for rendering on the
// render server and the data server.
#ifndef __vtkPMImageTextureProxy_h
#define __vtkPMImageTextureProxy_h

#include "vtkPMSourceProxy.h"

class vtkImageData;

class VTK_EXPORT vtkPMImageTextureProxy : public vtkPMSourceProxy
{
public:
  static vtkPMImageTextureProxy* New();
  vtkTypeMacro(vtkPMImageTextureProxy, vtkPMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMImageTextureProxy();
  ~vtkPMImageTextureProxy();

  // Description:
  // Creates the VTKObjects. Overridden to add post-filters to the pipeline.
  virtual bool CreateVTKObjects(vtkSMMessage* message);

private:
  vtkPMImageTextureProxy(const vtkPMImageTextureProxy&); // Not implemented
  void operator=(const vtkPMImageTextureProxy&); // Not implemented
//ETX
};

#endif
