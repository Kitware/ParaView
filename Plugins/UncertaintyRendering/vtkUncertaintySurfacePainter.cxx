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

#include "vtkPolyDataPainter.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkPolyData.h"
#include "vtkShader2Collection.h"
#include "vtkGLSLShaderDeviceAdapter2.h"
#include "vtkInformation.h"
#include "vtkgl.h"
#include "vtkGenericVertexAttributeMapping.h"
#include "vtkUniformVariables.h"

#include "vtkIdTypeArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkFloatArray.h"
#include "vtkPolyDataMapper.h"
#include "vtkPointData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkScalarsToColors.h"
#include "vtkScalarsToColorsPainter.h"

// vertex and fragment shader source code for the uncertainty surface
extern const char* vtkUncertaintySurfacePainter_fs;
extern const char* vtkUncertaintySurfacePainter_vs;

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
}

//----------------------------------------------------------------------------
vtkUncertaintySurfacePainter::~vtkUncertaintySurfacePainter()
{
  this->ReleaseGraphicsResources(this->LastRenderWindow);
  this->SetTransferFunction(0);

  if(this->Output)
    {
    this->Output->Delete();
    }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkUncertaintySurfacePainter::GetOutput()
{
  if(this->Enabled)
    {
    return this->Output;
    }

  return this->Superclass::GetOutput();
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfacePainter::ReleaseGraphicsResources(vtkWindow *window)
{
  if(this->Shader)
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
    if(info->Has(vtkScalarsToColorsPainter::LOOKUP_TABLE()))
      {
      vtkScalarsToColors *lut =
        vtkScalarsToColors::SafeDownCast(
          info->Get(vtkScalarsToColorsPainter::LOOKUP_TABLE()));

      double *range = lut->GetRange();
      this->ScalarValueRange = range[1] - range[0];
      }
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfacePainter::PrepareForRendering(vtkRenderer *renderer,
                                                       vtkActor *actor)
{
  if(!this->PrepareOutput())
    {
    vtkWarningMacro(<< "failed to prepare output");
    this->RenderingPreparationSuccess = 0;
    return;
    }

  vtkOpenGLRenderWindow *renWin =
    vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());

  if(!vtkShaderProgram2::IsSupported(renWin))
    {
    vtkWarningMacro(<< "vtkShaderProgram2 is not supported.");
    this->RenderingPreparationSuccess = 0;
    return;
    }

  // cleanup previous resources if targeting a different render window
  if(this->LastRenderWindow && this->LastRenderWindow != renWin)
    {
    this->ReleaseGraphicsResources(this->LastRenderWindow);
    }

  // store current render window
  this->LastRenderWindow = renWin;

  // setup shader
  if(!this->Shader)
    {
    // create new shader program
    this->Shader = vtkShaderProgram2::New();
    this->Shader->SetContext(renWin);

    // setup vertex shader
    vtkShader2 *vertexShader = vtkShader2::New();
    vertexShader->SetType(VTK_SHADER_TYPE_VERTEX);
    vertexShader->SetSourceCode(vtkUncertaintySurfacePainter_vs);
    vertexShader->SetContext(this->Shader->GetContext());
    this->Shader->GetShaders()->AddItem(vertexShader);
    vertexShader->Delete();

    // setup fragment shader
    vtkShader2 *fragmentShader = vtkShader2::New();
    fragmentShader->SetType(VTK_SHADER_TYPE_FRAGMENT);
    fragmentShader->SetSourceCode(vtkUncertaintySurfacePainter_fs);
    fragmentShader->SetContext(this->Shader->GetContext());
    this->Shader->GetShaders()->AddItem(fragmentShader);
    fragmentShader->Delete();

    // setup lighting helper
    this->LightingHelper->Initialize(this->Shader, VTK_SHADER_TYPE_VERTEX);
    this->LightingHelper->PrepareForRendering();
    }

  // superclass prepare for rendering
  this->Superclass::PrepareForRendering(renderer, actor);

  this->RenderingPreparationSuccess = 1;
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfacePainter::RenderInternal(vtkRenderer *renderer,
                                                  vtkActor *actor,
                                                  unsigned long typeFlags,
                                                  bool forceCompileOnly)
{
  if(!this->RenderingPreparationSuccess)
    {
    this->Superclass::RenderInternal(renderer,
                                     actor,
                                     typeFlags,
                                     forceCompileOnly);
    return;
    }

  vtkOpenGLRenderWindow *renWin =
    vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());

  glPushAttrib(GL_ALL_ATTRIB_BITS);

  // build and use the shader
  this->Shader->Build();
  if(this->Shader->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro("Shader building failed.");
    abort();
    }
  this->Shader->GetUniformVariables()->SetUniformf("uncertaintyScaleFactor", 1, &this->UncertaintyScaleFactor);
  this->Shader->GetUniformVariables()->SetUniformf("scalarValueRange", 1, &this->ScalarValueRange);

  this->Shader->Use();
  if(!this->Shader->IsValid())
    {
    vtkErrorMacro(<< " validation of the program failed: "
                  << this->Shader->GetLastValidateLog());
    }

  // superclass render
  this->Superclass::RenderInternal(renderer,
                                   actor,
                                   typeFlags,
                                   forceCompileOnly);
  glFinish();
  this->Shader->Restore();

  renWin->MakeCurrent();
  glFinish();

  glPopAttrib();
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfacePainter::PassInformation(vtkPainter *toPainter)
{
  if(!this->RenderingPreparationSuccess)
    {
    this->Superclass::PassInformation(toPainter);
    return;
    }

  this->Superclass::PassInformation(toPainter);

  vtkInformation *info = this->GetInformation();

  // add uncertainties array mapping
  vtkGenericVertexAttributeMapping *mappings = vtkGenericVertexAttributeMapping::New();
  mappings->AddMapping("uncertainty",
                       "Uncertainties",
                       vtkDataObject::FIELD_ASSOCIATION_POINTS,
                       0);
  info->Set(vtkPolyDataPainter::DATA_ARRAY_TO_VERTEX_ATTRIBUTE(), mappings);
  mappings->Delete();

  // add shader device adaptor
  vtkShaderDeviceAdapter2 *shaderAdaptor = vtkGLSLShaderDeviceAdapter2::New();
  shaderAdaptor->SetShaderProgram(this->Shader.GetPointer());
  info->Set(vtkPolyDataPainter::SHADER_DEVICE_ADAPTOR(), shaderAdaptor);
  shaderAdaptor->Delete();

  toPainter->SetInformation(info);
}

//----------------------------------------------------------------------------
bool vtkUncertaintySurfacePainter::PrepareOutput()
{
  if(!this->Enabled)
    {
    return false;
    }

  vtkDataObject *input = this->GetInput();
  vtkDataSet *inputDS = vtkDataSet::SafeDownCast(input);
  vtkCompositeDataSet *inputCD = vtkCompositeDataSet::SafeDownCast(input);

  if(!this->Output ||
     !this->Output->IsA(input->GetClassName()) ||
     (this->Output->GetMTime() < this->GetMTime()) ||
     (this->Output->GetMTime() < input->GetMTime()) ||
     this->TransferFunction->GetMTime() > this->Output->GetMTime())
    {
    if(this->Output)
      {
      this->Output->Delete();
      this->Output = 0;
      }

    if(inputCD)
      {
      vtkCompositeDataSet *outputCD = inputCD->NewInstance();
      outputCD->ShallowCopy(inputCD);

      this->Output = outputCD;
      }
    else if(inputDS)
      {
      vtkDataSet *outputDS = inputDS->NewInstance();
      outputDS->ShallowCopy(inputDS);

      this->Output = outputDS;
      }

    this->GenerateUncertaintiesArray(input, this->Output);
    this->Output->Modified();
    }

  return true;
}

//----------------------------------------------------------------------------
void vtkUncertaintySurfacePainter::GenerateUncertaintiesArray(vtkDataObject *input,
                                                              vtkDataObject *output)
{
  vtkCompositeDataSet *inputCD = vtkCompositeDataSet::SafeDownCast(input);
  if(inputCD)
    {
    vtkCompositeDataSet *outputCD = vtkCompositeDataSet::SafeDownCast(output);

    vtkCompositeDataIterator *iter = inputCD->NewIterator();
    for(iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      this->GenerateUncertaintiesArray(inputCD->GetDataSet(iter),
                                       outputCD->GetDataSet(iter));
      }
    iter->Delete();
    }

  vtkDataSet *inputDS = vtkDataSet::SafeDownCast(input);
  if(inputDS)
    {
    vtkDataSet *outputDS = vtkDataSet::SafeDownCast(output);

    vtkAbstractArray *inputUnceratintiesArray =
      inputDS->GetPointData()->GetAbstractArray(this->UncertaintyArrayName);
    if(!inputUnceratintiesArray)
      {
      return;
      }

    vtkFloatArray *outputUncertaintiesArray = vtkFloatArray::New();
    outputUncertaintiesArray->SetNumberOfComponents(1);
    outputUncertaintiesArray->SetNumberOfValues(
      inputUnceratintiesArray->GetNumberOfTuples());
    outputUncertaintiesArray->SetName("Uncertainties");

    if(this->TransferFunction)
      {
      // use transfer function
      for(vtkIdType i = 0; i < inputUnceratintiesArray->GetNumberOfTuples(); i++)
        {
        vtkVariant inputValue = inputUnceratintiesArray->GetVariantValue(i);
        double outputValue = inputValue.ToDouble() * 
                             this->TransferFunction->GetValue(inputValue.ToDouble());
        outputUncertaintiesArray->SetValue(i, static_cast<float>(outputValue));
        }
      }
    else
      {
      // pass values through directly
      for(vtkIdType i = 0; i < outputUncertaintiesArray->GetNumberOfTuples(); i++)
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
void vtkUncertaintySurfacePainter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
