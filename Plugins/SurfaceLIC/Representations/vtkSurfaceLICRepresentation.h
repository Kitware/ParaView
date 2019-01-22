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
// .NAME vtkSurfaceLICRepresentation
// .SECTION Description
// vtkSurfaceLICRepresentation extends vtkGeometryRepresentation to use surface
// lic when rendering surfaces.

#ifndef vtkSurfaceLICRepresentation_h
#define vtkSurfaceLICRepresentation_h

#include "vtkGeometryRepresentation.h"
#include "vtkSurfaceLICRepresentationsModule.h" // for export macro

class vtkInformation;
class vtkInformationRequestKey;

class vtkCompositeSurfaceLICMapper;

class VTKSURFACELICREPRESENTATIONS_EXPORT vtkSurfaceLICRepresentation
  : public vtkGeometryRepresentation
{
public:
  static vtkSurfaceLICRepresentation* New();
  vtkTypeMacro(vtkSurfaceLICRepresentation, vtkGeometryRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
  // typically called by the vtkView to request meta-data from the
  // representations or ask them to perform certain tasks e.g.
  // PrepareForRendering.
  virtual int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

  // Description:
  // Indicates whether LIC should be used when doing LOD rendering.
  void SetUseLICForLOD(bool val);

  //***************************************************************************
  // Forwarded to vtkSurfaceLICPainter
  void SetEnable(bool val);

  void SetNumberOfSteps(int val);
  void SetStepSize(double val);
  void SetNormalizeVectors(int val);

  void SetEnhancedLIC(int val);

  void SetEnhanceContrast(int val);
  void SetLowLICContrastEnhancementFactor(double val);
  void SetHighLICContrastEnhancementFactor(double val);
  void SetLowColorContrastEnhancementFactor(double val);
  void SetHighColorContrastEnhancementFactor(double val);
  void SetAntiAlias(int val);

  void SetColorMode(int val);
  void SetMapModeBias(double val);
  void SetLICIntensity(double val);

  void SetMaskOnSurface(int val);
  void SetMaskThreshold(double val);
  void SetMaskColor(double* val);
  void SetMaskColor(double r, double g, double b)
  {
    double rgb[3] = { r, g, b };
    this->SetMaskColor(rgb);
  }
  void SetMaskIntensity(double val);

  void SetGenerateNoiseTexture(int val);
  void SetNoiseType(int val);
  void SetNoiseTextureSize(int val);
  void SetNoiseGrainSize(int val);
  void SetMinNoiseValue(double val);
  void SetMaxNoiseValue(double val);
  void SetNumberOfNoiseLevels(int val);
  void SetImpulseNoiseProbability(double val);
  void SetImpulseNoiseBackgroundValue(double val);
  void SetNoiseGeneratorSeed(int val);

  void SetCompositeStrategy(int val);

  void WriteTimerLog(const char* fileName);

  void SelectInputVectors(int, int, int, int attributeMode, const char* name);

protected:
  vtkSurfaceLICRepresentation();
  ~vtkSurfaceLICRepresentation();

  // Description:
  // Overridden method to set parameters on vtkProperty and vtkMapper.
  void UpdateColoringParameters() override;

  vtkCompositeSurfaceLICMapper* SurfaceLICMapper;
  vtkCompositeSurfaceLICMapper* SurfaceLICLODMapper;

  bool UseLICForLOD;

private:
  vtkSurfaceLICRepresentation(const vtkSurfaceLICRepresentation&) = delete;
  void operator=(const vtkSurfaceLICRepresentation&) = delete;
};

#endif
