/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMSpriteTextureProxy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkSMSpriteTextureProxy
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>
// .SECTION Description
// Combines vtkTexture and vtkImageSpriteSource. This makes it simple for
// applications to create textures for point sprites.

#ifndef __vtkSMSpriteTextureProxy_h
#define __vtkSMSpriteTextureProxy_h

#include "vtkSMSourceProxy.h"

class vtkImageData;

class VTK_EXPORT vtkSMSpriteTextureProxy : public vtkSMSourceProxy
{
public:
  static vtkSMSpriteTextureProxy* New();
  vtkTypeMacro(vtkSMSpriteTextureProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the client side image if one has been loaded successfully.
  vtkImageData* GetLoadedImage();

//BTX
protected:
  vtkSMSpriteTextureProxy();
  ~vtkSMSpriteTextureProxy();

private:
  vtkSMSpriteTextureProxy(const vtkSMSpriteTextureProxy&); // Not implemented
  void operator=(const vtkSMSpriteTextureProxy&); // Not implemented
//ETX
};

#endif

