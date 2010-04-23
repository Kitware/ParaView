/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTwoScalarsToColorsPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkTwoScalarsToColorsPainter
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

#include "vtkTwoScalarsToColorsPainter.h"

#include "vtkObjectFactory.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkLookupTable.h"
#include "vtkAbstractMapper.h"
#include "vtkFloatArray.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkImageData.h"
#include "vtkOpenGL.h"
#include "vtkMapper.h"

vtkStandardNewMacro(vtkTwoScalarsToColorsPainter)

vtkTwoScalarsToColorsPainter::vtkTwoScalarsToColorsPainter()
{
  this->OpacityArrayName = NULL;
  this->EnableOpacity = false;
  this->OpacityScalarMode = VTK_SCALAR_MODE_USE_POINT_FIELD_DATA;

  this->InterpolateScalarsBeforeMapping = 0;
}

vtkTwoScalarsToColorsPainter::~vtkTwoScalarsToColorsPainter()
{
  this->SetOpacityArrayName(NULL);
}

void vtkTwoScalarsToColorsPainter::PrepareForRendering(
  vtkRenderer* vtkNotUsed(renderer),
    vtkActor* actor)
{
  vtkDataObject* input = this->GetInput();
  if (!input)
    {
    vtkErrorMacro("No input present.");
    return;
    }

  // If the input polydata has changed, the output should also reflect
  if (!this->OutputData || !this->OutputData->IsA(input->GetClassName())
      || this->OutputUpdateTime < this->MTime || this->OutputUpdateTime
      < this->GetInput()->GetMTime())
    {
    if (this->OutputData)
      {
      this->OutputData->Delete();
      this->OutputData = 0;
      }
    // Create a shallow-copied clone with no output scalars.
    this->OutputData = this->NewClone(input);
    this->OutputUpdateTime.Modified();
    }

  if (!this->ScalarVisibility && !this->EnableOpacity)
    {
    // Nothing to do here.
    this->ColorTextureMap = 0;
    return;
    }

  // Build the colors.
  // As per the vtkOpenGLPolyDataMapper's claim, this
  // it not a very expensive task, as the colors are cached
  // and hence we do this always.

  // Determine if we are going to use a texture for coloring or use vertex
  // colors. This need to be determine before we iterate over all the blocks in
  // the composite dataset to ensure that we emply the technique for all the
  // blocks.
  this->ScalarsLookupTable = 0;
  int useTexture = this->CanUseTextureMapForColoring(input);
  if (useTexture)
    {
    // Ensure that the ColorTextureMap has been created and updated correctly.
    // ColorTextureMap depends on the LookupTable. Hence it can be generated
    // independent of the input.
    this->UpdateColorTextureMap(actor->GetProperty()->GetOpacity(),
        this->GetPremultiplyColorsWithAlpha(actor));
    }
  else
    {
    // Remove texture map if present.
    this->ColorTextureMap = 0;
    }

  this->UsingScalarColoring = 0;

  // Now if we have composite data, we need to MapScalars for all leaves.
  if (input->IsA("vtkCompositeDataSet"))
    {
    vtkCompositeDataSet* cdInput = vtkCompositeDataSet::SafeDownCast(input);
    vtkCompositeDataSet* cdOutput = vtkCompositeDataSet::SafeDownCast(
        this->OutputData);
    vtkCompositeDataIterator* iter = cdInput->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkDataSet* pdInput = vtkDataSet::SafeDownCast(
          iter->GetCurrentDataObject());
      vtkDataSet* pdOutput = vtkDataSet::SafeDownCast(
          cdOutput->GetDataSet(iter));
      if (pdInput && pdOutput)
        {
        this->MapScalars(pdOutput, actor->GetProperty()->GetOpacity(),
            this->GetPremultiplyColorsWithAlpha(actor), pdInput, actor);
        }
      }

    iter->Delete();
    }
  else
    {
    this->MapScalars(vtkDataSet::SafeDownCast(this->OutputData),
        actor->GetProperty()->GetOpacity(),
        this->GetPremultiplyColorsWithAlpha(actor), vtkDataSet::SafeDownCast(
            input), actor);
    }
  this->LastUsedAlpha = actor->GetProperty()->GetOpacity();
  this->GetLookupTable()->SetAlpha(this->LastUsedAlpha);
}

void vtkTwoScalarsToColorsPainter::MapScalars(vtkDataSet* output,
    double alpha,
    int multiply_with_alpha,
    vtkDataSet* input,
    vtkActor* actor)
{
  this->InterpolateScalarsBeforeMapping = 0;
  this->ColorTextureMap = NULL;

  this->Superclass::MapScalars(output, alpha, multiply_with_alpha, input);

  if (!this->EnableOpacity)
    {
    return;
    }

  int cellFlag;
  //double orig_alpha;

  vtkDataArray* opacity;

  if (input == 0)
    {
    return;
    }

  vtkPointData* oppd = output->GetPointData();
  //vtkCellData* opcd = output->GetCellData();
  vtkFieldData* opfd = output->GetFieldData();

  vtkDataArray* originalColors = NULL;
  if (this->ScalarVisibility)
    {
    // if we map scalars to colors, then the opacity array has to
    // have the same scalarmode.
    opacity = vtkAbstractMapper::GetScalars(input, this->ScalarMode,
        VTK_GET_ARRAY_BY_NAME, -1, this->OpacityArrayName, cellFlag);

    }
  else
    { // no scalar color array, let us build one with constant color
    opacity = vtkAbstractMapper::GetScalars(input, this->OpacityScalarMode,
        VTK_GET_ARRAY_BY_NAME, -1, this->OpacityArrayName, cellFlag);

    }

  if (!opacity)
    return;

  if (cellFlag == 0)
    {
    originalColors = oppd->GetScalars();
    }
  else if (cellFlag == 1)
    {
    originalColors = oppd->GetScalars();
    }
  else
    {
    originalColors = opfd->GetArray("Color");
    }

  if (originalColors && (this->GetMTime() < this->BlendTime
      && input->GetMTime() < this->BlendTime && originalColors->GetMTime()
      < this->BlendTime) && actor->GetProperty()->GetMTime() < this->BlendTime)
    {
    //re-use old colors
    return;
    }

  if (!this->ScalarVisibility)
    {
    vtkUnsignedCharArray* constantColor = vtkUnsignedCharArray::New();
    constantColor->SetNumberOfComponents(4);
    constantColor->SetNumberOfTuples(opacity->GetNumberOfTuples());
    if (cellFlag == 0)
      {
      oppd->SetScalars(constantColor);
      }
    else if (cellFlag == 1)
      {
      oppd->SetScalars(constantColor);
      }
    else
      {
      opfd->AddArray(constantColor);
      }
    constantColor->Delete();

    double col[4];
    actor->GetProperty()->GetColor(col);
    unsigned char ucol[4];
    if(multiply_with_alpha)
      {
      ucol[0] = static_cast<unsigned char> (col[0] * alpha * 255);
      ucol[1] = static_cast<unsigned char> (col[1] * alpha * 255);
      ucol[2] = static_cast<unsigned char> (col[2] * alpha * 255);
      }
    else
      {
      ucol[0] = static_cast<unsigned char> (col[0] * 255);
      ucol[1] = static_cast<unsigned char> (col[1] * 255);
      ucol[2] = static_cast<unsigned char> (col[2] * 255);
      }
    ucol[3] = static_cast<unsigned char> (alpha * 255);
    unsigned char* pointer = constantColor->GetPointer(0);

    for (vtkIdType id = 0; id < constantColor->GetNumberOfTuples(); id++)
      {
      pointer[0] = ucol[0];
      pointer[1] = ucol[1];
      pointer[2] = ucol[2];
      pointer[3] = ucol[3];
      pointer += 4;
      }
    originalColors = constantColor;
    }

  if (!originalColors || originalColors->GetNumberOfTuples()
      != opacity->GetNumberOfTuples()
      || originalColors->GetNumberOfComponents() != 4)
    {
    this->BlendTime.Modified();
    return;
    }

  /// we then blend the two arrays together
  vtkIdType id;
  //int comp;
  //double tuple[4];
  int floatMode = opacity->GetDataType() == VTK_FLOAT || opacity->GetDataType()
      == VTK_DOUBLE;

  double amin = opacity->GetDataTypeMin();
  double amax = opacity->GetDataTypeMax();
  double arange = amax - amin;

  for (id = 0; id < opacity->GetNumberOfTuples(); id++)
    {
    double *tuple = originalColors->GetTuple(id);
    //double orig = tuple[3];
    double val = opacity->GetTuple1(id);
    // in the float Mode, the values are supposed to vary between 0 and 1
    if (floatMode)
      {
      if (val < 0.0)
        val = 0.0;
      if (val > 1.0)
        val = 1.0;
      tuple[3] = val * alpha * 255;
      }
    else // if the type is not float or double, values are supposed to vary from DataTypeMin to DataTypeMax
      {
      tuple[3] = ((val - amin) / arange) * alpha * 255;
      }
    originalColors->SetTuple(id, tuple);
    }
  // the primitive painter looks at the name to decide if the colors are opaque or not.
  // we use the translucent path, which means setting the name to NULL.
  if (cellFlag == 0 || cellFlag == 1)
    {
    originalColors->SetName(NULL);
    }
  this->BlendTime.Modified();
}

vtkDataObject* vtkTwoScalarsToColorsPainter::NewClone(vtkDataObject* data)
{
  if (data->IsA("vtkDataSet"))
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(data);
    vtkDataSet* clone = ds->NewInstance();
    clone->ShallowCopy(ds);
    // scalars passed thru this filter are colors, which will be built in
    // the pre-rendering stage.
    // SetActiveScalars is used so that the SetScalars
    // call afterwards do not remove the scalar array.
    clone->GetCellData()->SetActiveScalars(0);
    clone->GetPointData()->SetActiveScalars(0);
    clone->GetCellData()->SetScalars(0);
    clone->GetPointData()->SetScalars(0);
    return clone;
    }
  else if (data->IsA("vtkCompositeDataSet"))
    {
    vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(data);
    vtkCompositeDataSet* clone = cd->NewInstance();
    clone->CopyStructure(cd);
    vtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkDataObject* leafClone = this->NewClone(iter->GetCurrentDataObject());
      clone->SetDataSet(iter, leafClone);
      leafClone->Delete();
      }
    iter->Delete();
    return clone;
    }
  return 0;

}

void vtkTwoScalarsToColorsPainter::RenderInternals(vtkRenderer* renderer,
    vtkActor* actor,
    unsigned long typeflags,
    bool forceCompileOnly)
{
  vtkProperty* prop = actor->GetProperty();

  // if we are doing vertex colors then set lmcolor to adjust
  // the current materials ambient and diffuse values using
  // vertex color commands otherwise tell it not to.
  glDisable(GL_COLOR_MATERIAL);
  if (this->ScalarVisibility || this->EnableOpacity)
    {
    GLenum lmcolorMode;
    if (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT)
      {
      if (prop->GetAmbient() > prop->GetDiffuse())
        {
        lmcolorMode = GL_AMBIENT;
        }
      else
        {
        lmcolorMode = GL_DIFFUSE;
        }
      }
    else if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT_AND_DIFFUSE)
      {
      lmcolorMode = GL_AMBIENT_AND_DIFFUSE;
      }
    else if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT)
      {
      lmcolorMode = GL_AMBIENT;
      }
    else // if (this->ScalarMaterialMode == VTK_MATERIALMODE_DIFFUSE)
      {
      lmcolorMode = GL_DIFFUSE;
      }

    glColorMaterial(GL_FRONT_AND_BACK, lmcolorMode);
    glEnable(GL_COLOR_MATERIAL);

    }

  int pre_multiplied_by_alpha = this->GetPremultiplyColorsWithAlpha(actor);

  // We colors were premultiplied by alpha then we change the blending
  // function to one that will compute correct blended destination alpha
  // value, otherwise we stick with the default.
  if (pre_multiplied_by_alpha)
    {
    // save the blend function.
    glPushAttrib(GL_COLOR_BUFFER_BIT);

    // the following function is not correct with textures because there are
    // not premultiplied by alpha.
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }
  this->ColorTextureMap = 0;
  this->Superclass::RenderInternal(renderer, actor, typeflags, forceCompileOnly);

  if (pre_multiplied_by_alpha)
    {
    // restore the blend function
    glPopAttrib();
    }
}

void vtkTwoScalarsToColorsPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
