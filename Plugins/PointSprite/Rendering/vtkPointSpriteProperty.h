/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSpriteProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPointSpriteProperty
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
// vtkPointSpriteProperty is a property to be used for drawing point sprites instead of points.
// It manages the point sprite shader programs, and link the radius parameter to
// the VertexAttribute in the mapper if the radius is varying.

#ifndef __vtkPointSpriteProperty_h_
#define __vtkPointSpriteProperty_h_

#include "vtkOpenGLProperty.h"

class vtkRenderWindow;
class vtkActor;
class vtkRenderer;

class VTK_EXPORT vtkPointSpriteProperty : public vtkOpenGLProperty
{
public :
  static vtkPointSpriteProperty* New();
  vtkTypeMacro(vtkPointSpriteProperty, vtkOpenGLProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Call Superclass Render method to set shaders
  // and add OpenGL calls for the point parameters.
  virtual void Render(vtkActor *a, vtkRenderer *ren);

  //BTX
  // Description:
  // Restore point parameters and call Superclass PostRender method.
  virtual void PostRender(vtkActor *a,
                          vtkRenderer *r);
  //ETX

  //BTX
  enum
  {
    FixedRadius = 0, AttributeRadius = 1
  };
  //ETX

  // Description:
  // The radius is always given in World coordinates.
  // FixedRadius : the radius is constant for all vertices,
  //    its value is DefaultRadius*RadiusMultiplier
  // AttributeRadius : the radius is proportional to the vertex attribute Radius.
  //    the value is RadiusSpan.x + Radius*RadiusSpan.y;
  //    the array bound to the Radius attribute should be set by setting the RadiusArrayName.
  //    It must be a 1 component point array.
  virtual void  SetRadiusMode(int);
  vtkGetMacro(RadiusMode, int);

  virtual void SetRadiusModeToFixedRadius()
  {
    this->SetRadiusMode(FixedRadius);
  }
  virtual void SetRadiusModeToAttributeRadius()
  {
    this->SetRadiusMode(AttributeRadius);
  }

  // Description:
  // The ConstantRadius represents the radius of each point if
  // the eConstantRadius mode is used.
  // It should be specified in World units.
  vtkGetMacro(ConstantRadius, float);
  vtkSetMacro(ConstantRadius, float);

  // Description:
  // If the eScalarRadius mode is enabled, this painter uses the texture coordinates as radius values.
  // Those values are modified by the RadiusRange parameter as follow :
  // if the texture coordinates are in the range [0, 1]
  // the real radius values range from RadiusRange[0] to RadiusRange[1].
  // It should be specified in World units.
  vtkGetVector2Macro(RadiusRange, float);
  vtkSetVector2Macro(RadiusRange, float);

  //BTX
  enum eRenderMode
  {
  Quadrics=0, TexturedSprite = 1, SimplePoint = 2
  };
  //ETX

  // Description:
  // Set/Get the RenderMode for this mapper.
  // Currently 4 modes are supported : (TexturedSprite=0, FastSphere=1, ExactSphere=2, SimplePoint=3).
  // SimplePoint mode is a backup mode that does nothing :
  //    OpenGL points are rendered.
  // TexturedSprite renders textured point sprites using the GL_ARB_point_sprite extension.
  //    if the ScalarRadius mode is activated, it also uses a vertex shader to modify the
  //    point radius per vertex.
  // Quadrics renders the point sprite as a sphere, with exact depth by doing raytracing on the GPU.
  virtual void  SetRenderMode(int);
  vtkGetMacro(RenderMode, int);
  virtual void SetRenderModeToSimplePoint()
  {
    this->SetRenderMode(SimplePoint);
  }
  virtual void SetRenderModeToTexturedSprite()
  {
    this->SetRenderMode(TexturedSprite);
  }
  virtual void SetRenderModeToQuadrics()
  {
    this->SetRenderMode(Quadrics);
  }

  // Description:
  // This parameter limits the size of the point sprite to a maximum size in pixels.
  // This is a safety parameters, because some systems crash if
  // point sprites with a too large radius are rendered
  // this parameter is given in pixels, not in world coordinates.
  vtkSetMacro(MaxPixelSize, float);
  vtkGetMacro(MaxPixelSize, float);

  // Description:
  // This is the name of the array to map to the radius.
  // It has to be a point array. If the array has more than 1 component, the first component will be used.
  vtkSetStringMacro(RadiusArrayName);
  vtkGetStringMacro(RadiusArrayName);

protected :
  vtkPointSpriteProperty();
  ~vtkPointSpriteProperty();

  // Description:
  // some graphic cards may not support the extensions needed by the choosen mode.
  // in this case, a less demanding mode is choosen and a warning is sent.
  // the order of the choice is :
  // ExactSphere -> FastSphere -> TexturedSprite
  // if the TextureRadius mode is on, it is then turned off, and
  // if no compatible mode is found, use the SimplePoint mode.
  // Returns 1 if the real mode have changed, 0 if the mode where not modified.
  //virtual int SelectRealModes(vtkRenderWindow* renWin);

  // Description:
  // Load OpenGL extensions for point sprites.
  virtual void LoadPointSpriteExtensions(vtkRenderWindow* ren);

  // Description:
  // returns if the given mode is supported.
  // this must be called after LoadPointSpriteExtensions.
  virtual bool  IsSupported(vtkRenderWindow* renWin, int RenderMode, int RadiusMode);

  // Description:
  // This method is called by the Render method.
  // It updates the shader program if needed.
  virtual void PrepareForRendering(/*vtkActor*, vtkRenderer**/);

  int RenderMode;
  int RadiusMode;
  float ConstantRadius;
  float RadiusRange[2];
  float MaxPixelSize;
  char* RadiusArrayName;

private:
  vtkPointSpriteProperty(const vtkPointSpriteProperty&); // Not implemented.
  void operator=(const vtkPointSpriteProperty&); // Not implemented.

  //BTX
  class vtkInternal;
  vtkInternal* Internal;
  //ETX
};

#endif// __vtkPointSpriteFilter_h_
