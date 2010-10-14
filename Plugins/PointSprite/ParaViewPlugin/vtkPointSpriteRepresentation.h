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
// .NAME vtkPointSpriteRepresentation
// .SECTION Description
// vtkPointSpriteRepresentation is an extension for vtkGeometryRepresentation
// that renders point-sprites at all point locations.

#ifndef __vtkPointSpriteRepresentation_h
#define __vtkPointSpriteRepresentation_h

#include "vtkGeometryRepresentation.h"

class vtk1DGaussianTransferFunction;
class vtk1DLookupTableTransferFunction;
class vtk1DLookupTableTransferFunction;
class vtk1DTransferFunctionChooser;
class vtk1DTransferFunctionFilter;
class vtkCellPointsFilter;
class vtkDepthSortPainter;
class vtkPointSpriteDefaultPainter;
class vtkPointSpriteProperty;
class vtkTwoScalarsToColorsPainter;

class VTK_EXPORT vtkPointSpriteRepresentation : public vtkGeometryRepresentation
{
public:
  static vtkPointSpriteRepresentation* New();
  vtkTypeMacro(vtkPointSpriteRepresentation, vtkGeometryRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  //***************************************************************************
  // Forwarded to ArrayToRadiusFilter and LODArrayToRadiusFilter
  void SetRadiusTransferFunctionEnabled(int val);
  void SetRadiusArrayToProcess(int a, int b, int c, int d, const char* e);

  //***************************************************************************
  // Forwarded to ArrayToOpacityFilter and LODArrayToOpacityFilter
  void SetOpacityTransferFunctionEnabled(int val);
  void SetOpacityArrayToProcess(int a, int b, int c, int d, const char* e);

  //***************************************************************************
  // Forwarded to PSProperty (vtkPointSpriteProperty).
  void SetRenderMode(int val);
  void SetRadiusMode(int val);
  void SetConstantRadius(double val);
  void SetRadiusRange(double val0, double val1);
  void SetMaxPixelSize(double val);
  void SetRadiusArrayName(const char* val);

  //***************************************************************************
  // Forwarded to ScalarsToColorsPainter and LODScalarsToColorsPainter
  void SetEnableOpacity(double val);

  //***************************************************************************
  // Forwarded to RadiusTransferFunctionChooser
  void SetRadiusTransferFunctionMode(int val);
  void SetRadiusVectorComponent(int val);
  void SetRadiusScalarRange(double val0, double val1);
  void SetRadiusUseScalarRange(int val);

  //***************************************************************************
  // Forwarded to OpacityTransferFunctionChooser
  void SetOpacityTransferFunctionMode(int val);
  void SetOpacityVectorComponent(int val);
  void SetOpacityScalarRange(double val0, double val1);
  void SetOpacityUseScalarRange(int val);

  //***************************************************************************
  // Forwarded to RadiusTableTransferFunction
  void SetRadiusTableValues(int index, double val);
  void SetNumberOfRadiusTableValues(int val);
  void RemoveAllRadiusTableValues();

  //***************************************************************************
  // Forwarded to OpacityTableTransferFunction
  void SetOpacityTableValues(int index, double val);
  void SetNumberOfOpacityTableValues(int val);
  void RemoveAllOpacityTableValues();

  //***************************************************************************
  // Forwarded to RadiusGaussianTransferFunction
  void SetRadiusGaussianControlPoints(int index, double, double, double, double,
    double);
  void SetNumberOfRadiusGaussianControlPoints(int val);
  void RemoveAllRadiusGaussianControlPoints();

  //***************************************************************************
  // Forwarded to OpacityGaussianTransferFunction
  void SetOpacityGaussianControlPoints(int index, double, double, double,
    double, double);
  void SetNumberOfOpacityGaussianControlPoints(int val);
  void RemoveAllOpacityGaussianControlPoints();

  // Description:
  // InterpolateScalarsBeforeMapping is not supported by this representation.
  virtual void SetInterpolateScalarsBeforeMapping(int)
    {this->Superclass::SetInterpolateScalarsBeforeMapping(0); }
//BTX
protected:
  vtkPointSpriteRepresentation();
  ~vtkPointSpriteRepresentation();

  vtkCellPointsFilter* PointsFilter;
  vtk1DTransferFunctionFilter* ArrayToRadiusFilter;
  vtk1DTransferFunctionFilter* LODArrayToRadiusFilter;

  vtk1DTransferFunctionFilter* ArrayToOpacityFilter;
  vtk1DTransferFunctionFilter* LODArrayToOpacityFilter;

  vtkPointSpriteProperty* PSProperty;

  vtkPointSpriteDefaultPainter* PointSpriteDefaultPainter;
  vtkPointSpriteDefaultPainter* LODPointSpriteDefaultPainter;

  vtkDepthSortPainter* DepthSortPainter;
  vtkDepthSortPainter* LODDepthSortPainter;

  vtkTwoScalarsToColorsPainter* ScalarsToColorsPainter;
  vtkTwoScalarsToColorsPainter* LODScalarsToColorsPainter;

  vtk1DTransferFunctionChooser* RadiusTransferFunctionChooser;
  vtk1DTransferFunctionChooser* OpacityTransferFunctionChooser;

  vtk1DLookupTableTransferFunction* RadiusTableTransferFunction;
  vtk1DLookupTableTransferFunction* OpacityTableTransferFunction;

  vtk1DGaussianTransferFunction* RadiusGaussianTransferFunction;
  vtk1DGaussianTransferFunction* OpacityGaussianTransferFunction;

private:
  vtkPointSpriteRepresentation(const vtkPointSpriteRepresentation&); // Not implemented
  void operator=(const vtkPointSpriteRepresentation&); // Not implemented
//ETX
};

#endif
