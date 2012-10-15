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

#include "vtkCompositePolyDataMapper2.h"
#include "vtkObjectFactory.h"
#include "vtkSurfaceLICDefaultPainter.h"
#include "vtkSurfaceLICPainter.h"
#include "vtkInformationRequestKey.h"
#include "vtkInformation.h"
#include "vtkPVView.h"
#include "vtkPVRenderView.h"

// send LOD painter parameters that let it run faster.
// but lic result will be slightly degraded.
//#define vtkSurfaceLICRepresentationFASTLOD

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSurfaceLICRepresentation);

//----------------------------------------------------------------------------
vtkSurfaceLICRepresentation::vtkSurfaceLICRepresentation()
{
  vtkCompositePolyDataMapper2* mapper;
  vtkSurfaceLICDefaultPainter* painter;
  // painter chain
  painter = vtkSurfaceLICDefaultPainter::New();
  mapper  = dynamic_cast<vtkCompositePolyDataMapper2*>(this->Mapper);

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
}

//----------------------------------------------------------------------------
vtkSurfaceLICRepresentation::~vtkSurfaceLICRepresentation()
{
  this->Painter->Delete();
  this->LODPainter->Delete();
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SetUseLICForLOD(bool val)
{
  this->UseLICForLOD = val;
  this->LODPainter->SetEnable(this->Painter->GetEnable() && this->UseLICForLOD);
}

//----------------------------------------------------------------------------
int vtkSurfaceLICRepresentation::ProcessViewRequest(
      vtkInformationRequestKey* request_type,
      vtkInformation* inInfo,
      vtkInformation* outInfo)
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
  this->Painter->SetEnable(val);
  this->LODPainter->SetEnable(this->Painter->GetEnable() && this->UseLICForLOD);
}

// These are some settings that would help lod painter run faster.
// If the user really cares about speed then best to use a wireframe
// durring interaction
#if defined(vtkSurfaceLICRepresentationFASTLOD)
//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SetStepSize(double val)
{
  this->Painter->SetStepSize(val);

  // when interacting take half the number of steps at twice the
  // step size.
  double twiceVal=val*2.0;
  this->LODPainter->SetStepSize(twiceVal);
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SetNumberOfSteps(int val)
{
  this->Painter->SetNumberOfSteps(val);

  // when interacting take half the number of steps at twice the
  // step size.
  int halfVal=val/2;
  if (halfVal<1) halfVal=1;
  this->LODPainter->SetNumberOfSteps(halfVal);
}

//----------------------------------------------------------------------------
#define vtkSurfaceLICRepresentationPassParameterMacro(_name, _type)          \
void vtkSurfaceLICRepresentation::Set##_name (_type val)                     \
{                                                                            \
  this->Painter->Set##_name (val);                                           \
}
vtkSurfaceLICRepresentationPassParameterMacro( EnhancedLIC, int)
vtkSurfaceLICRepresentationPassParameterMacro( EnhanceContrast, int)
vtkSurfaceLICRepresentationPassParameterMacro( LowLICContrastEnhancementFactor, double)
vtkSurfaceLICRepresentationPassParameterMacro( HighLICContrastEnhancementFactor, double)
vtkSurfaceLICRepresentationPassParameterMacro( LowColorContrastEnhancementFactor, double)
vtkSurfaceLICRepresentationPassParameterMacro( HighColorContrastEnhancementFactor, double)
vtkSurfaceLICRepresentationPassParameterMacro( AntiAlias, int)
#endif

//----------------------------------------------------------------------------
#define vtkSurfaceLICRepresentationPassParameterWithLODMacro(_name, _type)   \
void vtkSurfaceLICRepresentation::Set##_name (_type val)                     \
{                                                                            \
  this->Painter->Set##_name (val);                                           \
  this->LODPainter->Set##_name (val);                                        \
}
#if !defined(vtkSurfaceLICRepresentationFASTLOD)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( StepSize, double)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( NumberOfSteps, int)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( EnhancedLIC, int)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( EnhanceContrast, int)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( LowLICContrastEnhancementFactor, double)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( HighLICContrastEnhancementFactor, double)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( LowColorContrastEnhancementFactor, double)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( HighColorContrastEnhancementFactor, double)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( AntiAlias, int)
#endif
vtkSurfaceLICRepresentationPassParameterWithLODMacro( NormalizeVectors, int)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( ColorMode, int)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( MapModeBias, double)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( LICIntensity, double)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( MaskOnSurface, int)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( MaskThreshold, double)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( MaskColor, double*)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( MaskIntensity, double)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( GenerateNoiseTexture, int)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( NoiseType, int)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( NoiseTextureSize, int)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( MinNoiseValue, double)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( MaxNoiseValue, double)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( NoiseGrainSize, int)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( NumberOfNoiseLevels, int)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( ImpulseNoiseProbability, double)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( ImpulseNoiseBackgroundValue, double)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( NoiseGeneratorSeed, int)
vtkSurfaceLICRepresentationPassParameterWithLODMacro( CompositeStrategy, int)

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SelectInputVectors(int, int, int,
  int attributeMode, const char* name)
{
  this->Painter->SetInputArrayToProcess(attributeMode, name);
  this->LODPainter->SetInputArrayToProcess(attributeMode, name);
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::WriteTimerLog(const char *fileName)
{
  #if !defined(vtkSurfaceLICPainterTIME) && !defined(vtkLineIntegralConvolution2DTIME)
  (void)fileName;
  #else
  this->Painter->WriteTimerLog(fileName);
  #endif
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::UpdateColoringParameters()
{
  Superclass::UpdateColoringParameters();
  // never interpolate scalars for surface LIC
  // because geometry shader is not expecting it.
  Superclass::SetInterpolateScalarsBeforeMapping(0);
}
