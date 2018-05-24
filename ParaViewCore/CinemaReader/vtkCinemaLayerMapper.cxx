/*=========================================================================

  Program:   ParaView
  Module:    vtkCinemaLayerMapper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCinemaLayerMapper.h"

#include "vtkActor2D.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkPixelBufferObject.h"
#include "vtkPointData.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkShaderProgram.h"
#include "vtkSmartPointer.h"
#include "vtkTextureObject.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVector.h"
#include "vtkViewport.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <vector>

namespace
{
enum TEXTURE_OBJECT_TYPES
{
  VALUES = 0,
  DEPTH = 1,
  LUMINANCE = 2,
  COLORS = 3,
  NUMBER_OF_TEXTURE_OBJECT_TYPES = 4,
};

static const char* vtkGetArrayName(int textureObjectType)
{
  switch (textureObjectType)
  {
    case VALUES:
      return "Values";
      break;
    case DEPTH:
      return "Depth";
      break;
    case LUMINANCE:
      return "Luminance";
      break;
    case COLORS:
      return "Colors";
      break;
  }
  return NULL;
}

/**
 * Class to manage textures for a layer (along with other supporting rendering
 * objects)
 */
class LayerTexturesType
{
  vtkSmartPointer<vtkPixelBufferObject> PBOs[NUMBER_OF_TEXTURE_OBJECT_TYPES];
  vtkSmartPointer<vtkTextureObject> TextureObjects[NUMBER_OF_TEXTURE_OBJECT_TYPES];

  void Update(int index, const int dims[3], vtkDataArray* darray, vtkOpenGLRenderWindow* context)
  {
    if (darray == NULL)
    {
      this->PBOs[index] = NULL;
      this->TextureObjects[index] = NULL;
      return;
    }
    if (!this->PBOs[index])
    {
      this->PBOs[index] = vtkSmartPointer<vtkPixelBufferObject>::New();
    }
    vtkIdType incs[2] = { 0, 0 };
    unsigned int udims2[2] = { static_cast<unsigned int>(dims[0]),
      static_cast<unsigned int>(dims[1]) };
    this->PBOs[index]->SetContext(context);
    this->PBOs[index]->Upload2D(darray->GetDataType(), darray->GetVoidPointer(0), udims2,
      darray->GetNumberOfComponents(), incs);
    if (!this->TextureObjects[index])
    {
      this->TextureObjects[index] = vtkSmartPointer<vtkTextureObject>::New();
      this->TextureObjects[index]->SetRequireTextureFloat(true);
      this->TextureObjects[index]->SetRequireDepthBufferFloat(true);
      this->TextureObjects[index]->SetWrapS(vtkTextureObject::ClampToEdge);
      this->TextureObjects[index]->SetWrapT(vtkTextureObject::ClampToEdge);
      this->TextureObjects[index]->SetWrapR(vtkTextureObject::ClampToEdge);
    }
    this->TextureObjects[index]->SetContext(context);
    this->TextureObjects[index]->Create2D(
      dims[0], dims[1], darray->GetNumberOfComponents(), this->PBOs[index], false);
  }

public:
  LayerTexturesType() {}
  ~LayerTexturesType() {}

  /**
   * Setup textures to point to the data array from the layer. Existing
   * texture objects may be reused.
   */
  void Initialize(vtkCinemaLayerMapper* mapper, vtkOpenGLRenderWindow* context, vtkActor2D* actor,
    vtkImageData* layer)
  {
    if (!layer || !context)
    {
      for (int cc = 0; cc < NUMBER_OF_TEXTURE_OBJECT_TYPES; ++cc)
      {
        this->PBOs[cc] = NULL;
        this->TextureObjects[cc] = NULL;
      }
      return;
    }

    vtkProperty2D* property = actor->GetProperty();
    vtkScalarsToColors* lut = mapper->GetLookupTable();

    // update texture for each type of data array in the layer.
    for (int ttype = 0; ttype < NUMBER_OF_TEXTURE_OBJECT_TYPES; ++ttype)
    {
      int dims[3];
      layer->GetDimensions(dims);

      vtkSmartPointer<vtkDataArray> darray =
        layer->GetPointData()->GetArray(vtkGetArrayName(ttype));
      if (!darray)
      {
        continue;
      }
      if (ttype == VALUES)
      {
        if (mapper->GetScalarVisibility() && (lut != NULL))
        {
          double prev = lut->GetAlpha();
          lut->SetAlpha(property->GetOpacity());
          darray.TakeReference(lut->MapScalars(darray, VTK_COLOR_MODE_MAP_SCALARS, -1));
          lut->SetAlpha(prev);
        }
        else
        {
          // showing solid color. we just pass a single pixel 2D texture that
          // has the color of interest instead of values.
          vtkNew<vtkUnsignedCharArray> colors;
          colors->SetNumberOfComponents(4);
          colors->SetNumberOfTuples(1);

          unsigned char rgba[4];
          rgba[0] = static_cast<unsigned char>(property->GetColor()[0] * 255.0);
          rgba[1] = static_cast<unsigned char>(property->GetColor()[1] * 255.0);
          rgba[2] = static_cast<unsigned char>(property->GetColor()[2] * 255.0);
          rgba[3] = static_cast<unsigned char>(property->GetOpacity() * 255.0);
          colors->SetTypedTuple(0, rgba);
          darray = colors.Get();
          dims[0] = dims[1] = dims[2] = 1;
        }
      }
      this->Update(ttype, dims, darray, context);
    }
  }

  vtkTextureObject* GetTextureObject(int type) const
  {
    return (type >= 0 && type < NUMBER_OF_TEXTURE_OBJECT_TYPES) ? this->TextureObjects[type] : NULL;
  }
};
}

class vtkCinemaLayerMapper::vtkInternals
{
  vtkCinemaLayerMapper* Mapper;
  std::vector<LayerTexturesType> LayerTextures;
  vtkNew<vtkMatrix4x4> InverseProjMat;
  vtkNew<vtkMatrix4x4> ViewUpRoll;

  vtkVector3d PrevCameraViewUp; // used to avoid recomputing ViewUpRoll unless needed.

  vtkSmartPointer<vtkShaderProgram> ShaderProgram;
  vtkSmartPointer<vtkOpenGLVertexArrayObject> VAO;
  vtkVector2i LayerDimensions; // size of each layer in pixels.
public:
  vtkInternals(vtkCinemaLayerMapper* mapper)
    : Mapper(mapper)
  {
  }

  void ReleaseGraphicsResources(vtkWindow*)
  {
    this->ShaderProgram = NULL;
    this->VAO = NULL;
    this->LayerTextures.clear();
  }

  /**
   * Updates all texture objects to have vtkTextureObject object corresponding
   * to various data arrays in each of the layers.
   */
  void UpdateTextureObjects(
    vtkOpenGLRenderWindow* context, vtkRenderer* vtkNotUsed(ren), vtkActor2D* actor)
  {
    vtkCinemaLayerMapper* const self = this->Mapper;

    const std::vector<vtkSmartPointer<vtkImageData> >& layers = self->GetLayers();

    const size_t numLayers = layers.size();
    this->LayerTextures.resize(numLayers);
    for (size_t cc = 0; cc < numLayers; ++cc)
    {
      vtkImageData* layer = layers[cc];
      assert(layer != NULL);
      int dims[3];
      layer->GetDimensions(dims);
      if (cc == 0)
      {
        this->LayerDimensions = vtkVector2i(dims[0], dims[1]);
      }
      else
      {
        // we're assuming this until we start keeping separate VBOs for each
        // layer.
        assert(this->LayerDimensions[0] == dims[0] && this->LayerDimensions[1] == dims[1]);
      }
      this->LayerTextures[cc].Initialize(self, context, actor, layer);
    }

    if (vtkMatrix4x4* projMat = self->GetLayerProjectionMatrix())
    {
      this->InverseProjMat->DeepCopy(projMat);

      // Since we want to convert form layer projection space back to
      // model-view in the vertex-shader, we invert it.
      this->InverseProjMat->Invert();

      // Convert to GLSL ordering.
      this->InverseProjMat->Transpose();
    }
    else
    {
      this->InverseProjMat->Identity();
    }

    // update the viewup
    this->ViewUpRoll->Identity();
    this->PrevCameraViewUp = vtkVector3d(0, 0, 0);
  }

  vtkMatrix4x4* GetRollMatrix(const vtkVector3d& cameraViewUp, const vtkVector3d& projDirection)
  {
    if (this->PrevCameraViewUp != cameraViewUp)
    {
      this->PrevCameraViewUp = cameraViewUp;
      vtkVector3d vLayer(this->Mapper->GetLayerCameraViewUp());
      vtkVector3d vCurrent(cameraViewUp);
      if (vLayer.Norm() != 0 && vCurrent.Norm() != 0)
      {
        vLayer.Normalize();
        vCurrent.Normalize();

        const vtkVector3d crossV = vLayer.Cross(vCurrent);
        double cosT = vLayer.Dot(vCurrent);
        double sinT = crossV.Norm();
        if (crossV.Dot(projDirection) < 0)
        {
          // angle is negative, so flip sign of sine
          sinT *= -1;
        }

        this->ViewUpRoll->Element[0][0] = cosT;
        this->ViewUpRoll->Element[0][1] = -sinT;
        this->ViewUpRoll->Element[1][0] = sinT;
        this->ViewUpRoll->Element[1][1] = cosT;

        // convert to GLSL order.
        this->ViewUpRoll->Transpose();
      }
    }
    return this->ViewUpRoll.Get();
  }

  void Render(vtkOpenGLRenderWindow* renWin, vtkRenderer* ren)
  {
    if (this->LayerTextures.size() == 0)
    {
      return;
    }

    vtkOpenGLState* ostate = renWin->GetState();

    typedef vtkOpenGLRenderUtilities GLUtil;
    if (!this->ShaderProgram)
    {
      // Prep fragment shader source:
      std::string fragShader = GLUtil::GetFullScreenQuadFragmentShaderTemplate();
      vtkShaderProgram::Substitute(fragShader, "//VTK::FSQ::Decl",
        "uniform bool hasLUT;\n"
        "uniform sampler2D texDepth;\n"
        "uniform sampler2D texLUTs;\n"
        "uniform sampler2D texValues;\n"
        "uniform sampler2D texLuminance;\n");
      vtkShaderProgram::Substitute(fragShader, "//VTK::FSQ::Impl",
        "vec2 tc = vec2(texCoord.x, 1.0 - texCoord.y);\n"
        "float depth = texture2D(texDepth, tc.xy).x;\n"
        "vec3 luminance = texture2D(texLuminance, tc.xy).xyz;\n"
        "if (depth >= 256.0){discard;}\n"
        "if (hasLUT) {\n"
        "  gl_FragData[0] = texture2D(texLUTs, tc.xy).rgba;\n"
        "} else {\n"
        "  vec4 valueMapped = texture2D(texValues, tc.xy).rgba;\n"
        "  gl_FragData[0] = vec4(vec3(valueMapped.xyz * luminance.xyz), valueMapped.a);\n"
        "}\n"
        "gl_FragDepth = depth/256.0;\n;");

      std::string vertexShader =
        "//VTK::System::Dec\n"
        "attribute vec4 ndCoordIn;\n"
        "attribute vec2 texCoordIn;\n"
        "varying vec2 texCoord;\n"
        "uniform mat4 invProjMat;\n"
        "uniform mat4 projMat;\n"
        "uniform mat4 rollMat;\n"
        "uniform int plopPixels;\n"
        "void main()\n"
        "{\n"
        "  if (plopPixels == 0) \n"
        "  {\n"
        "    gl_Position = projMat * rollMat * invProjMat * vec4(ndCoordIn.xy, 0, 1);\n"
        "  }\n"
        "  else\n"
        "  {\n"
        "    gl_Position = vec4(ndCoordIn.xy, 0, 1);\n"
        "  }\n"
        "  texCoord = texCoordIn;\n"
        "}\n";

      // Create shader program:
      this->ShaderProgram = renWin->GetShaderCache()->ReadyShaderProgram(
        // GLUtil::GetFullScreenQuadVertexShader().c_str(),
        vertexShader.c_str(), fragShader.c_str(),
        GLUtil::GetFullScreenQuadGeometryShader().c_str());
      // this->ShaderProgram->SetFileNameForDebugging("/tmp/cinema_layer_mapper");

      // Initialize new VAO/vertex buffer. This is only done once:
      this->VAO = vtkSmartPointer<vtkOpenGLVertexArrayObject>::New();
      GLUtil::PrepFullScreenVAO(renWin, this->VAO, this->ShaderProgram);
    }
    else
    {
      renWin->GetShaderCache()->ReadyShaderProgram(this->ShaderProgram);
    }

    this->VAO->Bind();
    ostate->vtkglEnable(GL_DEPTH_TEST);
    ostate->vtkglDepthMask(GL_TRUE);
    ostate->vtkglDepthFunc(GL_LEQUAL);

    vtkOpenGLCamera* camera = vtkOpenGLCamera::SafeDownCast(ren->GetActiveCamera());

    vtkMatrix4x4 *wcvc, *vcdc, *wcdc;
    vtkMatrix3x3* normMat;
    camera->GetKeyMatrices(ren, wcvc, normMat, vcdc, wcdc);

    vtkMatrix4x4* rollMat = this->GetRollMatrix(
      vtkVector3d(camera->GetViewUp()), vtkVector3d(camera->GetDirectionOfProjection()));
    for (size_t cc = 0; cc < this->LayerTextures.size(); ++cc)
    {
      const LayerTexturesType& layerTexture = this->LayerTextures[cc];

      vtkTextureObject* values = layerTexture.GetTextureObject(VALUES);
      vtkTextureObject* depth = layerTexture.GetTextureObject(DEPTH);
      vtkTextureObject* luminance = layerTexture.GetTextureObject(LUMINANCE);
      vtkTextureObject* luts = layerTexture.GetTextureObject(COLORS);
      if (!depth || (!(values && luminance) && !luts))
      {
        cerr << "Unsupported case encountered. Skipping for now.";
        continue;
      }

      depth->Activate();
      this->ShaderProgram->SetUniformi("texDepth", depth->GetTextureUnit());
      if (values)
      {
        values->Activate();
        this->ShaderProgram->SetUniformi("texValues", values->GetTextureUnit());
      }
      if (luminance)
      {
        luminance->Activate();
        this->ShaderProgram->SetUniformi("texLuminance", luminance->GetTextureUnit());
      }
      if (luts)
      {
        luts->Activate();
        this->ShaderProgram->SetUniformi("hasLUT", true);
        this->ShaderProgram->SetUniformi("texLUTs", luts->GetTextureUnit());
      }
      else
      {
        this->ShaderProgram->SetUniformi("hasLUT", false);
      }

      this->ShaderProgram->SetUniformi(
        "plopPixels", (this->Mapper->GetRenderLayersAsImage() == true) ? 1 : 0);
      this->ShaderProgram->SetUniformMatrix("invProjMat", this->InverseProjMat.Get());
      this->ShaderProgram->SetUniformMatrix("rollMat", rollMat);
      this->ShaderProgram->SetUniformMatrix("projMat", vcdc);

      GLUtil::DrawFullScreenQuad();

      depth->Deactivate();
      if (values)
      {
        values->Deactivate();
      }
      if (luminance)
      {
        luminance->Deactivate();
      }
      if (luts)
      {
        luts->Deactivate();
      }
    }
    this->VAO->Release();
  }
};

vtkStandardNewMacro(vtkCinemaLayerMapper);
vtkCxxSetObjectMacro(vtkCinemaLayerMapper, LookupTable, vtkScalarsToColors);
//----------------------------------------------------------------------------
vtkCinemaLayerMapper::vtkCinemaLayerMapper()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(0);
  this->ScalarVisibility = true;
  this->LookupTable = NULL;
  this->Internals = new vtkCinemaLayerMapper::vtkInternals(this);
  this->LayerCameraViewUp[0] = this->LayerCameraViewUp[1] = this->LayerCameraViewUp[2] = 0.0;
  this->RenderLayersAsImage = false;
}

//----------------------------------------------------------------------------
vtkCinemaLayerMapper::~vtkCinemaLayerMapper()
{
  this->SetLookupTable(NULL);
  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkCinemaLayerMapper::GetMTime()
{
  vtkMTimeType mtime = this->Superclass::GetMTime();
  if (this->ScalarVisibility && (this->LookupTable != NULL))
  {
    mtime = std::max(mtime, this->LookupTable->GetMTime());
  }
  mtime = std::max(mtime, this->LayerProjectionMatrix->GetMTime());
  return mtime;
}

//----------------------------------------------------------------------------
void vtkCinemaLayerMapper::SetLayerProjectionMatrix(vtkMatrix4x4* matrix)
{
  if (matrix == NULL)
  {
    this->LayerProjectionMatrix->Identity();
    this->Modified();
  }
  else
  {
    this->LayerProjectionMatrix->DeepCopy(matrix);
    this->Modified();
  }
}
//----------------------------------------------------------------------------
vtkMatrix4x4* vtkCinemaLayerMapper::GetLayerProjectionMatrix()
{
  return this->LayerProjectionMatrix.Get();
}

//----------------------------------------------------------------------------
void vtkCinemaLayerMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Superclass::ReleaseGraphicsResources(win);
  this->Internals->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
void vtkCinemaLayerMapper::SetLayers(const std::vector<vtkSmartPointer<vtkImageData> >& layers)
{
  if (this->Layers != layers)
  {
    this->Layers = layers;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkCinemaLayerMapper::ClearLayers()
{
  this->SetLayers(std::vector<vtkSmartPointer<vtkImageData> >());
}

//----------------------------------------------------------------------------
void vtkCinemaLayerMapper::RenderOpaqueGeometry(vtkViewport* viewport, vtkActor2D* actor)
{
  this->Superclass::RenderOpaqueGeometry(viewport, actor);

  if (this->GetMTime() < this->BuildTime)
  {
    return;
  }

  vtkInternals* internals = this->Internals;
  // If the mapper changed, we need to update the OpenGL resources for the
  // layers.
  vtkRenderer* ren = vtkRenderer::SafeDownCast(viewport);
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(viewport->GetVTKWindow());
  internals->UpdateTextureObjects(renWin, ren, actor);
  this->BuildTime.Modified();
}

//----------------------------------------------------------------------------
void vtkCinemaLayerMapper::RenderOverlay(vtkViewport* viewport, vtkActor2D*)
{
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(viewport->GetVTKWindow());
  this->Internals->Render(renWin, vtkRenderer::SafeDownCast(viewport));
}

//----------------------------------------------------------------------------
void vtkCinemaLayerMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
