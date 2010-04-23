/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSpriteProperty.cxx

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

#include "vtkPointSpriteProperty.h"

#include "vtkObjectFactory.h"
#include "vtkShaderProgram2.h"
#include "vtkShader2.h"
#include "vtkTimeStamp.h"
#include "vtkRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkShader2Collection.h"
#include "vtkCamera.h"
#include "vtkUniformVariables.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkgl.h"
#include "vtkSmartPointer.h"
#include "vtkPainterPolyDataMapper.h"
#include "vtkDataObject.h"
#include <cmath>

vtkStandardNewMacro(vtkPointSpriteProperty)

// the shader strings
extern const char* Texture_vs;
extern const char* Quadrics_vs;
extern const char* Quadrics_fs;
extern const char* FixedRadiusHelper;
extern const char* AttributeRadiusHelper;

class vtkPointSpriteProperty::vtkInternal
{
public:
  bool SpriteExtensionsOk;
  bool VertexShaderExtensionsOk;
  bool FragmentShaderExtensionsOk;

  bool NeedRadiusAttributeMapping;
  int CachedPointSmoothState;
  int PushedAttrib;

  vtkWeakPointer<vtkRenderWindow> CachedRenderWindow;
  vtkSmartPointer<vtkShader2> RadiusShader;
  vtkSmartPointer<vtkShader2> VertexShader;
  vtkSmartPointer<vtkShader2> FragmentShader;

  vtkInternal()
  {
    this->SpriteExtensionsOk = false;
    this->VertexShaderExtensionsOk = false;
    this->FragmentShaderExtensionsOk = false;
    this->PushedAttrib = 0;
    this->NeedRadiusAttributeMapping = false;
  }
};

vtkPointSpriteProperty::vtkPointSpriteProperty()
{
  this->ConstantRadius = 1;
  this->RadiusRange[0] = 0;
  this->RadiusRange[1] = 1;
  this->RadiusMode = FixedRadius;
  this->RenderMode = TexturedSprite;
  this->Internal = new vtkPointSpriteProperty::vtkInternal();
  this->MaxPixelSize = 1024.0;
  this->RadiusArrayName = NULL;
  this->PrepareForRendering();
}

vtkPointSpriteProperty::~vtkPointSpriteProperty()
{
  delete this->Internal;
  this->SetRadiusArrayName(NULL);
}

void vtkPointSpriteProperty::SetRenderMode(int renderMode)
{
  if (this->RenderMode == renderMode)
    return;
  this->RenderMode = renderMode;
  this->Modified();
  this->PrepareForRendering();
}

void vtkPointSpriteProperty::SetRadiusMode(int radiusMode)
{
  if (this->RadiusMode == radiusMode)
    return;
  this->RadiusMode = radiusMode;
  this->Modified();
  this->PrepareForRendering();
}

void vtkPointSpriteProperty::Render(vtkActor *act, vtkRenderer *ren)
{
  if (this->GetRepresentation() == VTK_POINTS)
    {
    this->LoadPointSpriteExtensions(ren->GetRenderWindow());
    // force the Shading ivar depending on the chosen modes
    if (this->RenderMode == Quadrics || (this->RenderMode == TexturedSprite
        && this->RadiusMode == AttributeRadius))
      {
      this->ShadingOn();
      }
    else
      {
      this->ShadingOff();
      }

    if (this->Internal->NeedRadiusAttributeMapping == true)
      {
      vtkPainterPolyDataMapper* mapper =
          vtkPainterPolyDataMapper::SafeDownCast(act->GetMapper());
      if (mapper)
        {
        mapper->RemoveVertexAttributeMapping("Radius");
        mapper->MapDataArrayToVertexAttribute("Radius", this->RadiusArrayName,
            vtkDataObject::FIELD_ASSOCIATION_POINTS, 0);
        }
      }

    if (this->Internal->PushedAttrib == 0)
      {
      glPushAttrib(GL_ALL_ATTRIB_BITS);
      this->Internal->PushedAttrib = 1;
      }

    // first test if only points are rendered
    if (this->RenderMode == SimplePoint)
      {
      glEnable(GL_POINT_SMOOTH);
      this->Superclass::Render(act, ren);
      return;
      }

    int *rensize = ren->GetSize();

    //
    // test if we only need the texture point sprite mode without shader
    //
    if (this->RenderMode == TexturedSprite && this->RadiusMode == FixedRadius)
      {

      //
      // Get the max point size : we need it to control Point Fading
      //
      float quadraticPointDistanceAttenuation[3];
      float maxSize;
      glGetFloatv(vtkgl::POINT_SIZE_MAX_ARB, &maxSize);
      if (this->MaxPixelSize < maxSize)
        {
        maxSize = this->MaxPixelSize;
        }

      // if the Radius Mode is set to Scalar radius, then
      // I have to compute the point size in the shader
      float factor = this->ConstantRadius * rensize[1] / this->GetPointSize();

      if (ren->GetActiveCamera()->GetParallelProjection())
        {
        factor /= ren->GetActiveCamera()->GetParallelScale();
        quadraticPointDistanceAttenuation[0] = 1.0 / (factor * factor);
        quadraticPointDistanceAttenuation[1] = 0.0;
        quadraticPointDistanceAttenuation[2] = 0.0;
        }
      else
        {
        factor *= 4.0;
        quadraticPointDistanceAttenuation[0] = 0.0;
        quadraticPointDistanceAttenuation[1] = 0.0;
        quadraticPointDistanceAttenuation[2] = 1.0 / (factor * factor);
        }

      vtkgl::PointParameterfvARB(vtkgl::POINT_DISTANCE_ATTENUATION_ARB,
          quadraticPointDistanceAttenuation);

      //
      // Set Point Fade Threshold size
      //
      // The alpha of a point is calculated to allow the fading of points
      // instead of shrinking them past a defined threshold size. The threshold
      // is defined by GL_POINT_FADE_THRESHOLD_SIZE_ARB and is not clamped to
      // the minimum and maximum point sizes.
      vtkgl::PointParameterfARB(vtkgl::POINT_FADE_THRESHOLD_SIZE_ARB, 1.0f);
      vtkgl::PointParameterfARB(vtkgl::POINT_SIZE_MIN_ARB, 1.0f);
      vtkgl::PointParameterfARB(vtkgl::POINT_SIZE_MAX_ARB, maxSize);

      }
    else // in all other cases, we need shaders
      {

      glEnable(vtkgl::VERTEX_PROGRAM_POINT_SIZE_ARB);

      //
      // send uniforms for the radius helper.
      //
      float factor = 1.0;

      if (ren->GetActiveCamera()->GetParallelProjection() && this->RenderMode
          != Quadrics)
        {
        factor = 0.25 / ren->GetActiveCamera()->GetParallelScale();
        }

      if (this->RadiusMode == AttributeRadius)
        {
        float radiusSpan[2];
        radiusSpan[0] = this->RadiusRange[0] * factor;
        radiusSpan[1] = (this->RadiusRange[1] - this->RadiusRange[0]) * factor;
        this->AddShaderVariable("RadiusSpan", 2, radiusSpan);
        }
      else if (this->RadiusMode == FixedRadius)
        {
        float radius = this->ConstantRadius * factor;
        this->AddShaderVariable("ConstantRadius", 1, &radius);
        }

      //
      // send uniforms for the vertex shader
      //
      float pointSizeThreshold = 0.0;
      float viewport[2] = { rensize[0], rensize[1] };

      this->AddShaderVariable("viewport", 2, viewport);
      this->AddShaderVariable("pointSizeThreshold", 1, &pointSizeThreshold);
      this->AddShaderVariable("MaxPixelSize", 1, &this->MaxPixelSize);

      }
    }

  this->Superclass::Render(act, ren);

  if (this->GetRepresentation() == VTK_POINTS)
    {
    //
    // Setup the specific texture parameters if the TexturedSprite mode in on
    //
    if (this->RenderMode == TexturedSprite)
      {
      glEnable(vtkgl::POINT_SPRITE_ARB);
      //
      // Specify point sprite texture coordinate replacement mode for each texture unit
      //
      glTexEnvf(vtkgl::POINT_SPRITE_ARB, vtkgl::COORD_REPLACE_ARB, GL_TRUE);

      //
      //
      //
      glEnable(GL_ALPHA_TEST);
      glAlphaFunc(GL_GREATER, 0.0);

      //
      // Blend the texture with the color.
      //
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      }
    }
}

void vtkPointSpriteProperty::PostRender(vtkActor *act, vtkRenderer *ren)
{
  if (this->GetRepresentation() == VTK_POINTS)
    {
    if (this->Internal->PushedAttrib == 1)
      {
      glPopAttrib();
      this->Internal->PushedAttrib = 0;
      }
    }

  this->Superclass::PostRender(act, ren);
}

void vtkPointSpriteProperty::PrepareForRendering()
{
  vtkShaderProgram2* prog = vtkShaderProgram2::New();

  const char* radius_shader = NULL;
  const char* vertex_shader = NULL;
  const char* fragment_shader = NULL;

  // setup the shaders and initialize the texture if necessary
  switch (this->RenderMode)
    {
    case SimplePoint:
      break;
    case TexturedSprite:
      if (this->RadiusMode == AttributeRadius)
        {
        radius_shader = AttributeRadiusHelper;
        vertex_shader = Texture_vs;
        }
      break;
    case Quadrics:
      if (this->RadiusMode == AttributeRadius)
        radius_shader = AttributeRadiusHelper;
      else if (this->RadiusMode == FixedRadius)
        radius_shader = FixedRadiusHelper;

      vertex_shader = Quadrics_vs;
      fragment_shader = Quadrics_fs;
      break;
    }

  if (radius_shader != NULL || vertex_shader != NULL || fragment_shader != NULL)
    {
    if (radius_shader != NULL)
      {
      vtkShader2* shader = vtkShader2::New();
      shader->SetSourceCode(radius_shader);
      shader->SetType(VTK_SHADER_TYPE_VERTEX);
      prog->GetShaders()->AddItem(shader);
      shader->Delete();
      }

    if (vertex_shader != NULL)
      {
      vtkShader2* shader = vtkShader2::New();
      shader->SetSourceCode(vertex_shader);
      shader->SetType(VTK_SHADER_TYPE_VERTEX);
      prog->GetShaders()->AddItem(shader);
      shader->Delete();
      }

    if (fragment_shader != NULL)
      {
      vtkShader2* shader = vtkShader2::New();
      shader->SetSourceCode(fragment_shader);
      shader->SetType(VTK_SHADER_TYPE_FRAGMENT);
      prog->GetShaders()->AddItem(shader);
      shader->Delete();
      }

    if (this->PropProgram != NULL)
      {
      this->PropProgram->ReleaseGraphicsResources();
      }
    this->SetPropProgram(prog);

    if (radius_shader == AttributeRadiusHelper)
      {
      this->Internal->NeedRadiusAttributeMapping = true;
      }
    else
      {
      this->Internal->NeedRadiusAttributeMapping = false;
      }
    this->ShadingOn();
    }
  else
    {
    if (this->PropProgram)
      {
      this->PropProgram->ReleaseGraphicsResources();
      this->SetPropProgram(NULL);
      }
    this->ShadingOff();
    this->Internal->NeedRadiusAttributeMapping = false;
    }
  prog->Delete();

}

bool vtkPointSpriteProperty::IsSupported(vtkRenderWindow* renWin,
    int renderMode,
    int radiusMode)
{
  this->LoadPointSpriteExtensions(renWin);
  if (renderMode == Quadrics)
    {
    return this->Internal->VertexShaderExtensionsOk
        && this->Internal->FragmentShaderExtensionsOk;
    }
  if (renderMode == TexturedSprite && radiusMode == AttributeRadius)
    {
    return this->Internal->VertexShaderExtensionsOk
        && this->Internal->SpriteExtensionsOk;
    }
  if (renderMode == TexturedSprite && radiusMode == FixedRadius)
    {
    return this->Internal->SpriteExtensionsOk;
    }
  if (renderMode == SimplePoint)
    {
    return true;
    }
  return false;
}

void vtkPointSpriteProperty::LoadPointSpriteExtensions(vtkRenderWindow *renWin)
{

  if (this->Internal->CachedRenderWindow == renWin)
    return;

  this->Internal->CachedRenderWindow = renWin;

  this->Internal->SpriteExtensionsOk = false;
  this->Internal->VertexShaderExtensionsOk = false;
  this->Internal->FragmentShaderExtensionsOk = false;

  //
  // Create Extension Manager
  //
  vtkSmartPointer<vtkOpenGLExtensionManager> extensions = vtkSmartPointer<
      vtkOpenGLExtensionManager>::New();
  extensions->SetRenderWindow(renWin);

  //
  // Test for point sprite extensions
  //
  int supports_GL_ARB_point_sprite = extensions->ExtensionSupported(
      "GL_ARB_point_sprite");
  int supports_GL_ARB_point_parameters = extensions->ExtensionSupported(
      "GL_ARB_point_parameters");
  //
  // Set Sprite flags depending on test results
  //
  if (!supports_GL_ARB_point_sprite || !supports_GL_ARB_point_parameters)
    {
    this->Internal->SpriteExtensionsOk = false;
    }
  else
    {
    this->Internal->SpriteExtensionsOk = true;
    extensions->LoadExtension("GL_ARB_point_sprite");
    extensions->LoadExtension("GL_ARB_point_parameters");
    }

  //
  // Test support for GLSL
  //
  int supports_GL_2_0 = extensions->ExtensionSupported("GL_VERSION_2_0");
  int supports_vertex_shader;
  int supports_fragment_shader;
  int supports_shader_objects;
  int supports_GL_ARB_vertex_program;

  if (supports_GL_2_0)
    {
    supports_vertex_shader = 1;
    supports_fragment_shader = 1;
    supports_shader_objects = 1;
    }
  else
    {
    supports_vertex_shader = extensions->ExtensionSupported(
        "GL_ARB_vertex_shader");
    supports_fragment_shader = extensions->ExtensionSupported(
        "GL_ARB_fragment_shader");
    supports_shader_objects = extensions->ExtensionSupported(
        "GL_ARB_shader_objects");
    }
  supports_GL_ARB_vertex_program = extensions->ExtensionSupported(
      "GL_ARB_vertex_program");

  //
  // Set vertex and fragment shader flags depending on test results
  //
  if (!supports_shader_objects || !supports_vertex_shader
      || !supports_GL_ARB_vertex_program)
    {
    this->Internal->VertexShaderExtensionsOk = false;
    }
  else
    {
    this->Internal->VertexShaderExtensionsOk = true;
    if (supports_GL_2_0)
      {
      extensions->LoadExtension("GL_VERSION_2_0");
      }
    else
      {
      extensions->LoadCorePromotedExtension("GL_ARB_vertex_shader");
      extensions->LoadCorePromotedExtension("GL_ARB_shader_objects");
      }
    extensions->LoadExtension("GL_ARB_vertex_program");
    extensions->LoadExtension("GL_ARB_shading_language_100");
    }

  if (!supports_shader_objects || !supports_fragment_shader)
    {
    this->Internal->FragmentShaderExtensionsOk = false;
    }
  else
    {
    this->Internal->FragmentShaderExtensionsOk = true;
    if (supports_GL_2_0)
      {
      extensions->LoadExtension("GL_VERSION_2_0");
      }
    else
      {
      extensions->LoadCorePromotedExtension("GL_ARB_fragment_shader");
      extensions->LoadCorePromotedExtension("GL_ARB_shader_objects");
      }
    extensions->LoadExtension("GL_ARB_shading_language_100");
    }
}

void vtkPointSpriteProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

