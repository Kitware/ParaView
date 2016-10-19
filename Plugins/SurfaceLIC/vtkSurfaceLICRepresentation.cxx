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

#ifndef VTKGL2
#include "vtkCompositePolyDataMapper2.h"
#include "vtkSurfaceLICDefaultPainter.h"
#include "vtkSurfaceLICPainter.h"
#else
#include "vtkCompositeSurfaceLICMapper.h"
#include "vtkSurfaceLICInterface.h"
#endif

// send LOD painter parameters that let it run faster.
// but lic result will be slightly degraded.
//#define vtkSurfaceLICRepresentationFASTLOD

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSurfaceLICRepresentation);

//----------------------------------------------------------------------------
vtkSurfaceLICRepresentation::vtkSurfaceLICRepresentation()
{
#ifndef VTKGL2
  vtkCompositePolyDataMapper2* mapper;
  vtkSurfaceLICDefaultPainter* painter;
  // painter chain
  painter = vtkSurfaceLICDefaultPainter::New();
  mapper = dynamic_cast<vtkCompositePolyDataMapper2*>(this->Mapper);

  painter->SetDelegatePainter(mapper->GetPainter()->GetDelegatePainter());
  mapper->SetPainter(painter);
  painter->Delete();

  this->Painter = painter->GetSurfaceLICPainter();
  this->Painter->Register(NULL);

  // lod painter chain
  painter = vtkSurfaceLICDefaultPainter::New();
  mapper = dynamic_cast<vtkCompositePolyDataMapper2*>(this->LODMapper);

  painter->SetDelegatePainter(mapper->GetPainter()->GetDelegatePainter());
  mapper->SetPainter(painter);
  painter->Delete();

  this->LODPainter = painter->GetSurfaceLICPainter();
  this->LODPainter->Register(NULL);
#else
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
#endif
}

//----------------------------------------------------------------------------
vtkSurfaceLICRepresentation::~vtkSurfaceLICRepresentation()
{
#ifndef VTKGL2
  this->Painter->Delete();
  this->LODPainter->Delete();
#endif
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SetUseLICForLOD(bool val)
{
  this->UseLICForLOD = val;
#ifndef VTKGL2
  this->LODPainter->SetEnable(this->Painter->GetEnable() && this->UseLICForLOD);
#else
  this->SurfaceLICLODMapper->GetLICInterface()->SetEnable(
    (this->SurfaceLICMapper->GetLICInterface()->GetEnable() && this->UseLICForLOD) ? 1 : 0);
#endif
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
  // MPI global collective comunication calls
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
#ifndef VTKGL2
  this->Painter->SetEnable(val);
  this->LODPainter->SetEnable(this->Painter->GetEnable() && this->UseLICForLOD);
#else
  this->SurfaceLICMapper->GetLICInterface()->SetEnable(val ? 1 : 0);
  this->SurfaceLICLODMapper->GetLICInterface()->SetEnable((val && this->UseLICForLOD) ? 1 : 0);
#endif
}

// These are some settings that would help lod painter run faster.
// If the user really cares about speed then best to use a wireframe
// durring interaction
#if defined(vtkSurfaceLICRepresentationFASTLOD)
//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SetStepSize(double val)
{

  // when interacting take half the number of steps at twice the
  // step size.
  double twiceVal = val * 2.0;

#ifndef VTKGL2
  this->Painter->SetStepSize(val);
  this->LODPainter->SetStepSize(twiceVal);
#else
  this->SurfaceLICMapper->GetLICInterface()->SetStepSize(val);
  this->SurfaceLICLODMapper->GetLICInterface()->SetStepSize(twiceVal);
#endif
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SetNumberOfSteps(int val)
{
#ifndef VTKGL2
  this->Painter->SetNumberOfSteps(val);
#else
  this->SurfaceLICMapper->SetNumberOfSteps(val);
#endif

  // when interacting take half the number of steps at twice the
  // step size.
  int halfVal = val / 2;
  if (halfVal < 1)
  {
    halfVal = 1;
  }
#ifndef VTKGL2
  this->LODPainter->GetLICInterface()->SetNumberOfSteps(halfVal);
#else
  this->SurfaceLICLODMapper->GetLICInterface()->SetNumberOfSteps(halfVal);
#endif
}

//----------------------------------------------------------------------------
#define vtkSurfaceLICRepresentationPassParameterMacro(_name, _type)                                \
  void vtkSurfaceLICRepresentation::Set##_name(_type val)                                          \
  {                                                                                                \
    #ifndef VTKGL2 this->Painter->Set##_name(val);                                                 \
    #else this->SurfaceLICMapper->GetLICInterface()->Set##_name(val);                              \
    #endif                                                                                         \
  }
vtkSurfaceLICRepresentationPassParameterMacro(
  EnhancedLIC, int) vtkSurfaceLICRepresentationPassParameterMacro(EnhanceContrast,
  int) vtkSurfaceLICRepresentationPassParameterMacro(LowLICContrastEnhancementFactor,
  double) vtkSurfaceLICRepresentationPassParameterMacro(HighLICContrastEnhancementFactor,
  double) vtkSurfaceLICRepresentationPassParameterMacro(LowColorContrastEnhancementFactor,
  double) vtkSurfaceLICRepresentationPassParameterMacro(HighColorContrastEnhancementFactor,
  double) vtkSurfaceLICRepresentationPassParameterMacro(AntiAlias, int)
#endif

//----------------------------------------------------------------------------
#ifndef VTKGL2
#define vtkSurfaceLICRepresentationPassParameterWithLODMacro(_name, _type)                         \
  void vtkSurfaceLICRepresentation::Set##_name(_type val)                                          \
  {                                                                                                \
    this->Painter->Set##_name(val);                                                                \
    this->LODPainter->Set##_name(val);                                                             \
  }
#else
#define vtkSurfaceLICRepresentationPassParameterWithLODMacro(_name, _type)                         \
  void vtkSurfaceLICRepresentation::Set##_name(_type val)                                          \
  {                                                                                                \
    this->SurfaceLICMapper->GetLICInterface()->Set##_name(val);                                    \
    this->SurfaceLICLODMapper->GetLICInterface()->Set##_name(val);                                 \
  }
#endif

#if !defined(vtkSurfaceLICRepresentationFASTLOD)
  vtkSurfaceLICRepresentationPassParameterWithLODMacro(
    StepSize, double) vtkSurfaceLICRepresentationPassParameterWithLODMacro(NumberOfSteps,
    int) vtkSurfaceLICRepresentationPassParameterWithLODMacro(EnhancedLIC,
    int) vtkSurfaceLICRepresentationPassParameterWithLODMacro(EnhanceContrast,
    int) vtkSurfaceLICRepresentationPassParameterWithLODMacro(LowLICContrastEnhancementFactor,
    double) vtkSurfaceLICRepresentationPassParameterWithLODMacro(HighLICContrastEnhancementFactor,
    double) vtkSurfaceLICRepresentationPassParameterWithLODMacro(LowColorContrastEnhancementFactor,
    double) vtkSurfaceLICRepresentationPassParameterWithLODMacro(HighColorContrastEnhancementFactor,
    double) vtkSurfaceLICRepresentationPassParameterWithLODMacro(AntiAlias, int)
#endif
    vtkSurfaceLICRepresentationPassParameterWithLODMacro(
      NormalizeVectors, int) vtkSurfaceLICRepresentationPassParameterWithLODMacro(ColorMode,
      int) vtkSurfaceLICRepresentationPassParameterWithLODMacro(MapModeBias, double)
      vtkSurfaceLICRepresentationPassParameterWithLODMacro(LICIntensity, double)
        vtkSurfaceLICRepresentationPassParameterWithLODMacro(MaskOnSurface, int)
          vtkSurfaceLICRepresentationPassParameterWithLODMacro(MaskThreshold, double)
            vtkSurfaceLICRepresentationPassParameterWithLODMacro(MaskColor, double*)
              vtkSurfaceLICRepresentationPassParameterWithLODMacro(MaskIntensity, double)
                vtkSurfaceLICRepresentationPassParameterWithLODMacro(GenerateNoiseTexture, int)
                  vtkSurfaceLICRepresentationPassParameterWithLODMacro(NoiseType, int)
                    vtkSurfaceLICRepresentationPassParameterWithLODMacro(NoiseTextureSize, int)
                      vtkSurfaceLICRepresentationPassParameterWithLODMacro(MinNoiseValue, double)
                        vtkSurfaceLICRepresentationPassParameterWithLODMacro(MaxNoiseValue, double)
                          vtkSurfaceLICRepresentationPassParameterWithLODMacro(NoiseGrainSize, int)
                            vtkSurfaceLICRepresentationPassParameterWithLODMacro(
                              NumberOfNoiseLevels, int)
                              vtkSurfaceLICRepresentationPassParameterWithLODMacro(
                                ImpulseNoiseProbability, double)
                                vtkSurfaceLICRepresentationPassParameterWithLODMacro(
                                  ImpulseNoiseBackgroundValue, double)
                                  vtkSurfaceLICRepresentationPassParameterWithLODMacro(
                                    NoiseGeneratorSeed, int)
                                    vtkSurfaceLICRepresentationPassParameterWithLODMacro(
                                      CompositeStrategy, int)

  //----------------------------------------------------------------------------
  void vtkSurfaceLICRepresentation::SelectInputVectors(
    int a, int b, int c, int attributeMode, const char* name)
{
#ifndef VTKGL2
  (void)a;
  (void)b;
  (void)c;
  this->Painter->SetInputArrayToProcess(attributeMode, name);
  this->LODPainter->SetInputArrayToProcess(attributeMode, name);
#else
  this->SurfaceLICMapper->SetInputArrayToProcess(a, b, c, attributeMode, name);
  this->SurfaceLICLODMapper->SetInputArrayToProcess(a, b, c, attributeMode, name);
#endif
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::WriteTimerLog(const char* fileName)
{
#if !defined(vtkSurfaceLICPainterTIME) && !defined(vtkLineIntegralConvolution2DTIME)
  (void)fileName;
#else
#ifndef VTKGL2
  this->Painter->WriteTimerLog(fileName);
#else
  this->SurfaceLICMapper->WriteTimerLog(fileName);
#endif
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
