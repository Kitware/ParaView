/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUncertaintySurfacePainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkUncertaintySurfacePainter.h"

#include <limits>

#include "vtkDataObject.h"
#include "vtkGLSLShaderDeviceAdapter2.h"
#include "vtkGenericVertexAttributeMapping.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPolyData.h"
#include "vtkPolyDataPainter.h"
#include "vtkRenderer.h"
#include "vtkShader2Collection.h"
#include "vtkUniformVariables.h"
#include "vtkgl.h"

#include "vtkCompositeDataIterator.h"
#include "vtkDataSetAttributes.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkScalarsToColors.h"
#include "vtkScalarsToColorsPainter.h"

// vertex and fragment shader source code for the uncertainty surface
extern const char* vtkUncertaintySurfacePainter_fs;
extern const char* vtkUncertaintySurfacePainter_vs;

namespace
{

/*
 * Adapted from Stefan Gustavson (stegu@itn.liu.se) 2004
 * Original by Ken Perlin
 */
int perm[256] = { 151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36,
  103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94,
  252, 219, 203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168,
  68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133,
  230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209,
  76, 132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173,
  186, 3, 64, 52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206,
  59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70,
  221, 153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178,
  185, 112, 104, 218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81,
  51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115,
  121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195,
  78, 66, 215, 61, 156, 180 };

/* These are Ken Perlin's proposed gradients for 3D noise. I kept them for
   better consistency with the reference implementation, but there is really
   no need to pad this to 16 gradients for this particular implementation.
   If only the "proper" first 12 gradients are used, they can be extracted
   from the grad4[][] array: grad3[i][j] == grad4[i*2][j], 0<=i<=11, j=0,1,2
*/
int grad3[16][3] = { { 0, 1, 1 }, { 0, 1, -1 }, { 0, -1, 1 }, { 0, -1, -1 }, { 1, 0, 1 },
  { 1, 0, -1 }, { -1, 0, 1 }, { -1, 0, -1 }, { 1, 1, 0 }, { 1, -1, 0 }, { -1, 1, 0 },
  { -1, -1, 0 },                                            // 12 cube edges
  { 1, 0, -1 }, { -1, 0, -1 }, { 0, -1, 1 }, { 0, 1, 1 } }; // 4 more to make 16

/* These are my own proposed gradients for 4D noise. They are the coordinates
   of the midpoints of each of the 32 edges of a tesseract, just like the 3D
   noise gradients are the midpoints of the 12 edges of a cube.
*/
int grad4[32][4] = { { 0, 1, 1, 1 }, { 0, 1, 1, -1 }, { 0, 1, -1, 1 },
  { 0, 1, -1, -1 }, // 32 tesseract edges
  { 0, -1, 1, 1 }, { 0, -1, 1, -1 }, { 0, -1, -1, 1 }, { 0, -1, -1, -1 }, { 1, 0, 1, 1 },
  { 1, 0, 1, -1 }, { 1, 0, -1, 1 }, { 1, 0, -1, -1 }, { -1, 0, 1, 1 }, { -1, 0, 1, -1 },
  { -1, 0, -1, 1 }, { -1, 0, -1, -1 }, { 1, 1, 0, 1 }, { 1, 1, 0, -1 }, { 1, -1, 0, 1 },
  { 1, -1, 0, -1 }, { -1, 1, 0, 1 }, { -1, 1, 0, -1 }, { -1, -1, 0, 1 }, { -1, -1, 0, -1 },
  { 1, 1, 1, 0 }, { 1, 1, -1, 0 }, { 1, -1, 1, 0 }, { 1, -1, -1, 0 }, { -1, 1, 1, 0 },
  { -1, 1, -1, 0 }, { -1, -1, 1, 0 }, { -1, -1, -1, 0 } };

/* This is a look-up table to speed up the decision on which simplex we
   are in inside a cube or hypercube "cell" for 3D and 4D simplex noise.
   It is used to avoid complicated nested conditionals in the GLSL code.
   The table is indexed in GLSL with the results of six pair-wise
   comparisons beween the components of the P=(x,y,z,w) coordinates
   within a hypercube cell.
   c1 = x>=y ? 32 : 0;
   c2 = x>=z ? 16 : 0;
   c3 = y>=z ? 8 : 0;
   c4 = x>=w ? 4 : 0;
   c5 = y>=w ? 2 : 0;
   c6 = z>=w ? 1 : 0;
   offsets = simplex[c1+c2+c3+c4+c5+c6];
   o1 = step(160,offsets);
   o2 = step(96,offsets);
   o3 = step(32,offsets);
   (For the 3D case, c4, c5, c6 and o3 are not needed.)
*/
unsigned char simplex4[][4] = { { 0, 64, 128, 192 }, { 0, 64, 192, 128 }, { 0, 0, 0, 0 },
  { 0, 128, 192, 64 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 64, 128, 192, 0 },
  { 0, 128, 64, 192 }, { 0, 0, 0, 0 }, { 0, 192, 64, 128 }, { 0, 192, 128, 64 }, { 0, 0, 0, 0 },
  { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 64, 192, 128, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
  { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
  { 64, 128, 0, 192 }, { 0, 0, 0, 0 }, { 64, 192, 0, 128 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
  { 0, 0, 0, 0 }, { 128, 192, 0, 64 }, { 128, 192, 64, 0 }, { 64, 0, 128, 192 },
  { 64, 0, 192, 128 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 128, 0, 192, 64 },
  { 0, 0, 0, 0 }, { 128, 64, 192, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
  { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
  { 128, 0, 64, 192 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 192, 0, 64, 128 },
  { 192, 0, 128, 64 }, { 0, 0, 0, 0 }, { 192, 64, 128, 0 }, { 128, 64, 0, 192 }, { 0, 0, 0, 0 },
  { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 192, 64, 0, 128 }, { 0, 0, 0, 0 }, { 192, 128, 0, 64 },
  { 192, 128, 64, 0 } };

/*
 * initPermTexture(GLuint *texID) - create and load a 2D texture for
 * a combined index permutation and gradient lookup table.
 * This texture is used for 2D and 3D noise, both classic and simplex.
 */
void initPermTexture(GLuint* texID)
{
  char* pixels;
  int i, j;
  vtkgl::ActiveTexture(vtkgl::TEXTURE1);

  glGenTextures(1, texID);              // Generate a unique texture ID
  glBindTexture(GL_TEXTURE_2D, *texID); // Bind the texture to texture unit 0

  pixels = (char*)malloc(256 * 256 * 4);
  for (i = 0; i < 256; i++)
    for (j = 0; j < 256; j++)
    {
      int offset = (i * 256 + j) * 4;
      char value = perm[(j + perm[i]) & 0xFF];
      pixels[offset] = grad3[value & 0x0F][0] * 64 + 64;     // Gradient x
      pixels[offset + 1] = grad3[value & 0x0F][1] * 64 + 64; // Gradient y
      pixels[offset + 2] = grad3[value & 0x0F][2] * 64 + 64; // Gradient z
      pixels[offset + 3] = value;                            // Permuted index
    }

  // GLFW texture loading functions won't work here - we need GL_NEAREST lookup.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  vtkgl::ActiveTexture(vtkgl::TEXTURE0); // Switch active texture unit back to 0 again
}

/*
 * initSimplexTexture(GLuint *texID) - create and load a 1D texture for a
 * simplex traversal order lookup table. This is used for simplex noise only,
 * and only for 3D and 4D noise where there are more than 2 simplices.
 * (3D simplex noise has 6 cases to sort out, 4D simplex noise has 24 cases.)
 */
void initSimplexTexture(GLuint* texID)
{
  vtkgl::ActiveTexture(vtkgl::TEXTURE2); // Activate a different texture unit (unit 1)

  glGenTextures(1, texID);              // Generate a unique texture ID
  glBindTexture(GL_TEXTURE_1D, *texID); // Bind the texture to texture unit 1

  // GLFW texture loading functions won't work here - we need GL_NEAREST lookup.
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, simplex4);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  vtkgl::ActiveTexture(vtkgl::TEXTURE0); // Switch active texture unit back to 0 again
}

/*
 * initGradTexture(GLuint *texID) - create and load a 2D texture
 * for a 4D gradient lookup table. This is used for 4D noise only.
 */
void initGradTexture(GLuint* texID)
{
  char* pixels;
  int i, j;

  vtkgl::ActiveTexture(vtkgl::TEXTURE3); // Activate a different texture unit (unit 2)

  glGenTextures(1, texID);              // Generate a unique texture ID
  glBindTexture(GL_TEXTURE_2D, *texID); // Bind the texture to texture unit 2

  pixels = (char*)malloc(256 * 256 * 4);
  for (i = 0; i < 256; i++)
    for (j = 0; j < 256; j++)
    {
      int offset = (i * 256 + j) * 4;
      char value = perm[(j + perm[i]) & 0xFF];
      pixels[offset] = grad4[value & 0x1F][0] * 64 + 64;     // Gradient x
      pixels[offset + 1] = grad4[value & 0x1F][1] * 64 + 64; // Gradient y
      pixels[offset + 2] = grad4[value & 0x1F][2] * 64 + 64; // Gradient z
      pixels[offset + 3] = grad4[value & 0x1F][3] * 64 + 64; // Gradient z
    }

  // GLFW texture loading functions won't work here - we need GL_NEAREST lookup.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  vtkgl::ActiveTexture(vtkgl::TEXTURE0); // Switch active texture unit back to 0 again
}

} // end anonymous namespace

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkUncertaintySurfacePainter)

  //----------------------------------------------------------------------------
  vtkUncertaintySurfacePainter::vtkUncertaintySurfacePainter()
{
  this->Enabled = 1;
  this->Output = 0;
  this->LastRenderWindow = 0;
  this->LightingHelper = vtkSmartPointer<vtkLightingHelper>::New();
  this->TransferFunction = vtkPiecewiseFunction::New();
  this->TransferFunction->AddPoint(0, 0);
  this->TransferFunction->AddPoint(1, 1);
  this->UncertaintyArrayName = 0;
  this->UncertaintyScaleFactor = 10.0f;
  this->ScalarValueRange = 100.0f;
  this->PermTextureId = 0;
  this->SimplexTextureId = 0;
  this->GradTextureId = 0;
}

//----------------------------------------------------------------------------
vtkUncertaintySurfacePainter::~vtkUncertaintySurfacePainter()
{
  this->ReleaseGraphicsResources(this->LastRenderWindow);
  this->SetTransferFunction(0);

  if (this->Output)
  {
    this->Output->Delete();
  }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkUncertaintySurfacePainter::GetOutput()
{
  if (this->Enabled)
  {
    return this->Output;
  }

  return this->Superclass::GetOutput();
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfacePainter::ReleaseGraphicsResources(vtkWindow* window)
{
  if (this->Shader)
  {
    this->Shader->ReleaseGraphicsResources();
    this->Shader->Delete();
    this->Shader = 0;
  }

  this->LightingHelper->Initialize(0, VTK_SHADER_TYPE_VERTEX);

  this->LastRenderWindow = 0;
  this->Superclass::ReleaseGraphicsResources(window);
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfacePainter::ProcessInformation(vtkInformation* info)
{
  if (info->Has(vtkScalarsToColorsPainter::LOOKUP_TABLE()))
  {
    vtkScalarsToColors* lut =
      vtkScalarsToColors::SafeDownCast(info->Get(vtkScalarsToColorsPainter::LOOKUP_TABLE()));

    double* range = lut->GetRange();
    this->ScalarValueRange = range[1] - range[0];
  }
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfacePainter::PrepareForRendering(vtkRenderer* renderer, vtkActor* actor)
{
  if (!this->PrepareOutput())
  {
    vtkWarningMacro(<< "failed to prepare output");
    this->RenderingPreparationSuccess = 0;
    return;
  }

  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());

  if (!vtkShaderProgram2::IsSupported(renWin))
  {
    vtkWarningMacro(<< "vtkShaderProgram2 is not supported.");
    this->RenderingPreparationSuccess = 0;
    return;
  }

  // cleanup previous resources if targeting a different render window
  if (this->LastRenderWindow && this->LastRenderWindow != renWin)
  {
    this->ReleaseGraphicsResources(this->LastRenderWindow);
  }

  if (!vtkgl::ActiveTexture)
  {
    // try to load the multitexture extensions
    vtkOpenGLExtensionManager* extensions = vtkOpenGLExtensionManager::New();
    extensions->SetRenderWindow(renWin);

    if (!(extensions->LoadSupportedExtension("GL_ARB_multitexture") &&
          extensions->LoadSupportedExtension("GL_VERSION_1_2")))
    {
      vtkWarningMacro(<< "GL_ARB_multitexture is not supported.");
      this->RenderingPreparationSuccess = 0;
      extensions->Delete();
      return;
    }

    if (!vtkgl::ActiveTexture)
    {
      // try to load the function directly
      vtkgl::ActiveTexture =
        (vtkgl::PFNGLACTIVETEXTUREPROC)extensions->GetProcAddress("glActiveTextureARB");
    }

    if (!vtkgl::ActiveTexture)
    {
      vtkWarningMacro(<< "vtkgl::ActiveTexture() not found.");
      this->RenderingPreparationSuccess = 0;
      extensions->Delete();
      return;
    }

    extensions->Delete();
  }

  // store current render window
  this->LastRenderWindow = renWin;

  // setup textures
  initPermTexture(&this->PermTextureId);
  initSimplexTexture(&this->SimplexTextureId);
  initGradTexture(&this->GradTextureId);

  // setup shader
  if (!this->Shader)
  {
    // create new shader program
    this->Shader = vtkShaderProgram2::New();
    this->Shader->SetContext(renWin);

    // setup vertex shader
    vtkShader2* vertexShader = vtkShader2::New();
    vertexShader->SetType(VTK_SHADER_TYPE_VERTEX);
    vertexShader->SetSourceCode(vtkUncertaintySurfacePainter_vs);
    vertexShader->SetContext(this->Shader->GetContext());
    this->Shader->GetShaders()->AddItem(vertexShader);
    vertexShader->Delete();

    // setup fragment shader
    vtkShader2* fragmentShader = vtkShader2::New();
    fragmentShader->SetType(VTK_SHADER_TYPE_FRAGMENT);
    fragmentShader->SetSourceCode(vtkUncertaintySurfacePainter_fs);
    fragmentShader->SetContext(this->Shader->GetContext());
    this->Shader->GetShaders()->AddItem(fragmentShader);
    fragmentShader->Delete();

    // setup lighting helper
    this->LightingHelper->Initialize(this->Shader, VTK_SHADER_TYPE_VERTEX);
    this->LightingHelper->PrepareForRendering();

    // build and use the shader
    this->Shader->Build();
    if (this->Shader->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
      vtkErrorMacro("Shader building failed.");
      abort();
    }
    this->Shader->GetUniformVariables()->SetUniformf(
      "uncertaintyScaleFactor", 1, &this->UncertaintyScaleFactor);
    this->Shader->GetUniformVariables()->SetUniformf(
      "scalarValueRange", 1, &this->ScalarValueRange);
    this->Shader->GetUniformVariables()->SetUniformi(
      "permTexture", 1, reinterpret_cast<int*>(&this->PermTextureId));
    this->Shader->GetUniformVariables()->SetUniformi(
      "simplexTexture", 1, reinterpret_cast<int*>(&this->SimplexTextureId));
    this->Shader->GetUniformVariables()->SetUniformi(
      "gradTexture", 1, reinterpret_cast<int*>(&this->GradTextureId));
  }

  // superclass prepare for rendering
  this->Superclass::PrepareForRendering(renderer, actor);

  this->RenderingPreparationSuccess = 1;
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfacePainter::RenderInternal(
  vtkRenderer* renderer, vtkActor* actor, unsigned long typeFlags, bool forceCompileOnly)
{
  if (!this->RenderingPreparationSuccess)
  {
    this->Superclass::RenderInternal(renderer, actor, typeFlags, forceCompileOnly);
    return;
  }

  vtkOpenGLClearErrorMacro();

  // vtkOpenGLRenderWindow *renWin =
  //  vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());

  glPushAttrib(GL_ALL_ATTRIB_BITS);

  this->Shader->Use();
  if (!this->Shader->IsValid())
  {
    vtkErrorMacro(<< " validation of the program failed: " << this->Shader->GetLastValidateLog());
  }

  vtkOpenGLCheckErrorMacro("failed in vtkUncertaintySurfacePainter::RenderInternal (Step 1)");
  vtkOpenGLClearErrorMacro();

  // superclass render
  this->Superclass::RenderInternal(renderer, actor, typeFlags, forceCompileOnly);

  vtkOpenGLCheckErrorMacro("failed in vtkUncertaintySurfacePainter::RenderInternal (Step 2)");
  glFinish();
  this->Shader->Restore();

  //  renWin->MakeCurrent();
  glFinish();

  glPopAttrib();
  vtkOpenGLCheckErrorMacro("failed after vtkUncertaintySurfacePainter::RenderInternal");
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfacePainter::PassInformation(vtkPainter* toPainter)
{
  if (!this->RenderingPreparationSuccess)
  {
    this->Superclass::PassInformation(toPainter);
    return;
  }

  this->Superclass::PassInformation(toPainter);

  vtkInformation* info = this->GetInformation();

  // add uncertainties array mapping
  vtkGenericVertexAttributeMapping* mappings = vtkGenericVertexAttributeMapping::New();
  mappings->AddMapping("uncertainty", "Uncertainties", vtkDataObject::FIELD_ASSOCIATION_POINTS, 0);
  info->Set(vtkPolyDataPainter::DATA_ARRAY_TO_VERTEX_ATTRIBUTE(), mappings);
  mappings->Delete();

  // add shader device adaptor
  vtkShaderDeviceAdapter2* shaderAdaptor = vtkGLSLShaderDeviceAdapter2::New();
  shaderAdaptor->SetShaderProgram(this->Shader.GetPointer());
  info->Set(vtkPolyDataPainter::SHADER_DEVICE_ADAPTOR(), shaderAdaptor);
  shaderAdaptor->Delete();

  toPainter->SetInformation(info);
}

//----------------------------------------------------------------------------
bool vtkUncertaintySurfacePainter::PrepareOutput()
{
  if (!this->Enabled)
  {
    return false;
  }

  vtkDataObject* input = this->GetInput();
  vtkDataSet* inputDS = vtkDataSet::SafeDownCast(input);
  vtkCompositeDataSet* inputCD = vtkCompositeDataSet::SafeDownCast(input);

  if (!this->Output || !this->Output->IsA(input->GetClassName()) ||
    (this->Output->GetMTime() < this->GetMTime()) ||
    (this->Output->GetMTime() < input->GetMTime()) ||
    this->TransferFunction->GetMTime() > this->Output->GetMTime())
  {
    if (this->Output)
    {
      this->Output->Delete();
      this->Output = 0;
    }

    if (inputCD)
    {
      vtkCompositeDataSet* outputCD = inputCD->NewInstance();
      outputCD->ShallowCopy(inputCD);

      this->Output = outputCD;
    }
    else if (inputDS)
    {
      vtkDataSet* outputDS = inputDS->NewInstance();
      outputDS->ShallowCopy(inputDS);

      this->Output = outputDS;
    }

    this->GenerateUncertaintiesArray(input, this->Output);
    this->Output->Modified();
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfacePainter::GenerateUncertaintiesArray(
  vtkDataObject* input, vtkDataObject* output)
{
  vtkCompositeDataSet* inputCD = vtkCompositeDataSet::SafeDownCast(input);
  if (inputCD)
  {
    vtkCompositeDataSet* outputCD = vtkCompositeDataSet::SafeDownCast(output);

    vtkCompositeDataIterator* iter = inputCD->NewIterator();
    for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      this->GenerateUncertaintiesArray(inputCD->GetDataSet(iter), outputCD->GetDataSet(iter));
    }
    iter->Delete();
  }

  vtkDataSet* inputDS = vtkDataSet::SafeDownCast(input);
  if (inputDS)
  {
    vtkDataSet* outputDS = vtkDataSet::SafeDownCast(output);

    vtkAbstractArray* inputUnceratintiesArray =
      inputDS->GetPointData()->GetAbstractArray(this->UncertaintyArrayName);
    if (!inputUnceratintiesArray)
    {
      return;
    }

    vtkFloatArray* outputUncertaintiesArray = vtkFloatArray::New();
    outputUncertaintiesArray->SetNumberOfComponents(1);
    outputUncertaintiesArray->SetNumberOfValues(inputUnceratintiesArray->GetNumberOfTuples());
    outputUncertaintiesArray->SetName("Uncertainties");

    if (this->TransferFunction)
    {
      // use transfer function
      for (vtkIdType i = 0; i < inputUnceratintiesArray->GetNumberOfTuples(); i++)
      {
        vtkVariant inputValue = inputUnceratintiesArray->GetVariantValue(i);
        double outputValue =
          inputValue.ToDouble() * this->TransferFunction->GetValue(inputValue.ToDouble());
        outputUncertaintiesArray->SetValue(i, static_cast<float>(outputValue));
      }
    }
    else
    {
      // pass values through directly
      for (vtkIdType i = 0; i < outputUncertaintiesArray->GetNumberOfTuples(); i++)
      {
        vtkVariant inputValue = inputUnceratintiesArray->GetVariantValue(i);
        outputUncertaintiesArray->SetValue(i, inputValue.ToFloat());
      }
    }

    // add array to output
    outputDS->GetPointData()->AddArray(outputUncertaintiesArray);
    outputUncertaintiesArray->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfacePainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
