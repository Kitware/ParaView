/*=========================================================================

  Program:   ParaView
  Module:    vtkShearedWaveletSource.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkShearedWaveletSource
// .SECTION Description
//

#ifndef vtkShearedWaveletSource_h
#define vtkShearedWaveletSource_h

#include "vtkUnstructuredGridAlgorithm.h"

class VTK_EXPORT vtkShearedWaveletSource : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkShearedWaveletSource* New();
  vtkTypeMacro(vtkShearedWaveletSource, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // Set bounding box is model-space.
  // Specified as (xmin, xmax, ymin, ymax, zmin, zmax).
  vtkSetVector6Macro(ModelBoundingBox, double);
  vtkGetVector6Macro(ModelBoundingBox, double);

  // Set basis vectors.
  vtkSetVector3Macro(BasisU, double);
  vtkSetVector3Macro(BasisV, double);
  vtkSetVector3Macro(BasisW, double);

  // Description:
  // Set title that should be used by the CubeAxis for a given direction
  vtkSetStringMacro(AxisUTitle);
  vtkGetStringMacro(AxisUTitle);
  vtkSetStringMacro(AxisVTitle);
  vtkGetStringMacro(AxisVTitle);
  vtkSetStringMacro(AxisWTitle);
  vtkGetStringMacro(AxisWTitle);

  // Description:
  // Enable/Disable field generation for oriented bounding box annotation
  vtkSetMacro(EnableAxisTitles, bool);
  vtkGetMacro(EnableAxisTitles, bool);

  // Description:
  // Enable/Disable field generation for label that will be used for "Time:"
  vtkSetMacro(EnableTimeLabel, bool);
  vtkGetMacro(EnableTimeLabel, bool);

  // Description:
  // Specify custom Time label
  vtkSetStringMacro(TimeLabel);
  vtkGetStringMacro(TimeLabel);
  const char* GetTimeLabelAnnotation() { return this->EnableTimeLabel ? this->TimeLabel : "Time"; }

protected:
  vtkShearedWaveletSource();
  ~vtkShearedWaveletSource();

  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  bool EnableAxisTitles;
  bool EnableTimeLabel;

  char* AxisUTitle;
  char* AxisVTitle;
  char* AxisWTitle;
  char* TimeLabel;

  double ModelBoundingBox[6];
  double BasisU[3];
  double BasisV[3];
  double BasisW[3];

private:
  vtkShearedWaveletSource(const vtkShearedWaveletSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkShearedWaveletSource&) VTK_DELETE_FUNCTION;
};

#endif
