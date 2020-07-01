/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSurfaceLICRepresentation.h"

#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationRequestKey.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"

#include "vtkCompositeSurfaceLICMapper.h"
#include "vtkSurfaceLICInterface.h"

// send LOD painter parameters that let it run faster.
// but lic result will be slightly degraded.
//#define vtkSurfaceLICRepresentationFASTLOD

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSurfaceLICRepresentation);

//----------------------------------------------------------------------------
vtkSurfaceLICRepresentation::vtkSurfaceLICRepresentation()
{
  this->Mapper->Delete();
  this->LODMapper->Delete();

  this->SurfaceLICMapper = vtkCompositeSurfaceLICMapper::New();
  this->SurfaceLICLODMapper = vtkCompositeSurfaceLICMapper::New();
  this->Mapper = this->SurfaceLICMapper;
  this->LODMapper = this->SurfaceLICLODMapper;

  // setup composite display attributes
  vtkCompositeDataDisplayAttributes* compositeAttributes = vtkCompositeDataDisplayAttributes::New();
  this->SurfaceLICMapper->SetCompositeDataDisplayAttributes(compositeAttributes);
  this->SurfaceLICLODMapper->SetCompositeDataDisplayAttributes(compositeAttributes);
  compositeAttributes->Delete();

  // This will add the new mappers to the pipeline.
  this->SetupDefaults();
}

//----------------------------------------------------------------------------
vtkSurfaceLICRepresentation::~vtkSurfaceLICRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SetUseLICForLOD(bool val)
{
  this->UseLICForLOD = val;
  this->SurfaceLICLODMapper->GetLICInterface()->SetEnable(
    (this->SurfaceLICMapper->GetLICInterface()->GetEnable() && this->UseLICForLOD) ? 1 : 0);
}

//----------------------------------------------------------------------------
int vtkSurfaceLICRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    // i.e. this->GetVisibility() == false, hence nothing to do.
    return 0;
  }

  // the Surface LIC painter will make use of
  // MPI global collective communication calls
  // need to disable IceT's empty image
  // optimization
  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    outInfo->Set(vtkPVRenderView::RENDER_EMPTY_IMAGES(), 1);
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SetEnable(bool val)
{
  this->SurfaceLICMapper->GetLICInterface()->SetEnable(val ? 1 : 0);
  this->SurfaceLICLODMapper->GetLICInterface()->SetEnable((val && this->UseLICForLOD) ? 1 : 0);
}

// These are some settings that would help lod painter run faster.
// If the user really cares about speed then best to use a wireframe
// during interaction
#if defined(vtkSurfaceLICRepresentationFASTLOD)
//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SetStepSize(double val)
{

  // when interacting take half the number of steps at twice the
  // step size.
  double twiceVal = val * 2.0;

  this->SurfaceLICMapper->GetLICInterface()->SetStepSize(val);
  this->SurfaceLICLODMapper->GetLICInterface()->SetStepSize(twiceVal);
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SetNumberOfSteps(int val)
{
  this->SurfaceLICMapper->SetNumberOfSteps(val);

  // when interacting take half the number of steps at twice the
  // step size.
  int halfVal = val / 2;
  if (halfVal < 1)
  {
    halfVal = 1;
  }
  this->SurfaceLICLODMapper->GetLICInterface()->SetNumberOfSteps(halfVal);
}

//----------------------------------------------------------------------------
#define vtkSurfaceLICRepresentationPassParameterMacro(_name, _type)                                \
  void vtkSurfaceLICRepresentation::Set##_name(_type val)                                          \
  {                                                                                                \
    this->SurfaceLICMapper->GetLICInterface()->Set##_name(val);                                    \
  }
vtkSurfaceLICRepresentationPassParameterMacro(EnhancedLIC, int);
vtkSurfaceLICRepresentationPassParameterMacro(EnhanceContrast, int);
vtkSurfaceLICRepresentationPassParameterMacro(LowLICContrastEnhancementFactor, double);
vtkSurfaceLICRepresentationPassParameterMacro(HighLICContrastEnhancementFactor, double);
vtkSurfaceLICRepresentationPassParameterMacro(LowColorContrastEnhancementFactor, double);
vtkSurfaceLICRepresentationPassParameterMacro(HighColorContrastEnhancementFactor, double);
vtkSurfaceLICRepresentationPassParameterMacro(AntiAlias, int);
#endif

//----------------------------------------------------------------------------
#define vtkSurfaceLICRepresentationPassParameterWithLODMacro(_name, _type)                         \
  void vtkSurfaceLICRepresentation::Set##_name(_type val)                                          \
  {                                                                                                \
    this->SurfaceLICMapper->GetLICInterface()->Set##_name(val);                                    \
    this->SurfaceLICLODMapper->GetLICInterface()->Set##_name(val);                                 \
  }

#if !defined(vtkSurfaceLICRepresentationFASTLOD)
vtkSurfaceLICRepresentationPassParameterWithLODMacro(StepSize, double);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(NumberOfSteps, int);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(EnhancedLIC, int);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(EnhanceContrast, int);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(LowLICContrastEnhancementFactor, double);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(HighLICContrastEnhancementFactor, double);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(LowColorContrastEnhancementFactor, double);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(HighColorContrastEnhancementFactor, double);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(AntiAlias, int);
#endif
vtkSurfaceLICRepresentationPassParameterWithLODMacro(NormalizeVectors, int);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(ColorMode, int);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(MapModeBias, double);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(LICIntensity, double);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(MaskOnSurface, int);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(MaskThreshold, double);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(MaskColor, double*);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(MaskIntensity, double);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(GenerateNoiseTexture, int);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(NoiseType, int);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(NoiseTextureSize, int);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(MinNoiseValue, double);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(MaxNoiseValue, double);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(NoiseGrainSize, int);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(NumberOfNoiseLevels, int);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(ImpulseNoiseProbability, double);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(ImpulseNoiseBackgroundValue, double);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(NoiseGeneratorSeed, int);
vtkSurfaceLICRepresentationPassParameterWithLODMacro(CompositeStrategy, int);

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SelectInputVectors(
  int a, int b, int c, int attributeMode, const char* name)
{
  this->SurfaceLICMapper->SetInputArrayToProcess(a, b, c, attributeMode, name);
  this->SurfaceLICLODMapper->SetInputArrayToProcess(a, b, c, attributeMode, name);
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::WriteTimerLog(const char* fileName)
{
#if !defined(vtkSurfaceLICPainterTIME) && !defined(vtkLineIntegralConvolution2DTIME)
  (void)fileName;
#else
  this->SurfaceLICMapper->WriteTimerLog(fileName);
#endif
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::UpdateColoringParameters()
{
  this->Superclass::UpdateColoringParameters(); // check if this is still relevant.
  // never interpolate scalars for surface LIC
  // because geometry shader is not expecting it.
  this->Superclass::SetInterpolateScalarsBeforeMapping(0);
}
