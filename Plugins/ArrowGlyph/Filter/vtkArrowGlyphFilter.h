/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkArrowGlyphFilter.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkArrowGlyphFilter - A Glyph filter for arrows
// .SECTION Description
// vtkArrowGlyphFilter glyphs arrows using the independent variables
// for radius and length

#ifndef vtkArrowGlyphFilter_h
#define vtkArrowGlyphFilter_h

#include "vtkArrowGlyphFilterModule.h"
#include "vtkPolyDataAlgorithm.h"
#include "vtkSmartPointer.h"

class vtkArrowSource;
class vtkMaskPoints;

class VTKARROWGLYPHFILTER_EXPORT vtkArrowGlyphFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkArrowGlyphFilter* New();
  vtkTypeMacro(vtkArrowGlyphFilter, vtkPolyDataAlgorithm);

  // Description:
  // Turn on/off orienting of arrows along vector/normal.
  vtkSetMacro(ScaleByOrientationVectorMagnitude, int);
  vtkBooleanMacro(ScaleByOrientationVectorMagnitude, int);
  vtkGetMacro(ScaleByOrientationVectorMagnitude, int);

  // Description:
  // Array (vector) to use to control Orientation
  vtkSetStringMacro(OrientationVectorArray);
  vtkGetStringMacro(OrientationVectorArray);

  // Description:
  // A Scaling factor to apply to the arrows in conjunction with ScaleArray
  vtkSetMacro(ScaleFactor, double);
  vtkGetMacro(ScaleFactor, double);

  // Description:
  // Array to use to control Scale
  vtkSetStringMacro(ScaleArray);
  vtkGetStringMacro(ScaleArray);

  // Description:
  // A Scaling factor to apply to the arrows in conjunction with ShaftRadiusArray
  vtkSetMacro(ShaftRadiusFactor, double);
  vtkGetMacro(ShaftRadiusFactor, double);

  // Description:
  // Array to use to control ShaftRadius
  vtkSetStringMacro(ShaftRadiusArray);
  vtkGetStringMacro(ShaftRadiusArray);

  // Description:
  // A Scaling factor to apply to the arrows in conjunction with TipRadiusArray
  vtkSetMacro(TipRadiusFactor, double);
  vtkGetMacro(TipRadiusFactor, double);

  // Description:
  // Array to use to control TipRadius
  vtkSetStringMacro(TipRadiusArray);
  vtkGetStringMacro(TipRadiusArray);

  // Description:
  // Limit the number of points to glyph
  vtkSetMacro(MaximumNumberOfPoints, int);
  vtkGetMacro(MaximumNumberOfPoints, int);

  // Description:
  // Set/get whether to mask points
  void SetUseMaskPoints(int useMaskPoints);
  vtkGetMacro(UseMaskPoints, int);

  // Description:
  // Set/get flag to cause randomization of which points to mask.
  void SetRandomMode(bool mode);
  bool GetRandomMode();

  // Description:
  // This can be overwritten by subclass to return 0 when a point is
  // blanked. Default implementation is to always return 1;
  virtual int IsPointVisible(vtkDataSet*, vtkIdType) { return 1; };

  virtual void SetArrowSourceObject(vtkArrowSource* arrowsource);

  // Description:
  // Overridden to include ArrowSourceObject's MTime.
  vtkMTimeType GetMTime() override;

protected:
  vtkArrowGlyphFilter();
  ~vtkArrowGlyphFilter() override;

  enum
  {
    GlyphNPointsGather = 738233,
    GlyphNPointsScatter = 738234
  };

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

  vtkIdType GatherTotalNumberOfPoints(vtkIdType localNumPts);
  int MaskAndExecute(vtkIdType numPts, vtkIdType maxNumPts, vtkDataSet* input,
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  int ScaleByOrientationVectorMagnitude;
  char* OrientationVectorArray;
  //
  double ScaleFactor;
  char* ScaleArray;
  //
  double ShaftRadiusFactor;
  char* ShaftRadiusArray;
  //
  double TipRadiusFactor;
  char* TipRadiusArray;
  //
  vtkMaskPoints* MaskPoints;
  int MaximumNumberOfPoints;
  int UseMaskPoints;
  int RandomMode;

  //
  //  vtkSmartPointer<vtkArrowSource> ArrowSourceObject;
  vtkArrowSource* ArrowSourceObject;

private:
  vtkArrowGlyphFilter(const vtkArrowGlyphFilter&) = delete;
  void operator=(const vtkArrowGlyphFilter&) = delete;
};

#endif
