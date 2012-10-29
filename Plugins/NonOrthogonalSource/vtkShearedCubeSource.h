/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShearedCubeSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkShearedCubeSource - A sheared version of the Sphere source

#ifndef __vtkShearedCubeSource_h
#define __vtkShearedCubeSource_h

#include "vtkCubeSource.h"

class VTK_EXPORT vtkShearedCubeSource : public vtkCubeSource
{
public:
  vtkTypeMacro(vtkShearedCubeSource,vtkCubeSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct sphere with radius=0.5 and default resolution 8 in both Phi
  // and Theta directions. Theta ranges from (0,360) and phi (0,180) degrees.
  static vtkShearedCubeSource *New();

  // Description:
  // Set the unit vector for X
  vtkSetVector3Macro(BaseU,double);
  vtkGetVectorMacro(BaseU,double,3);

  // Description:
  // Set the unit vector for Y
  vtkSetVector3Macro(BaseV,double);
  vtkGetVectorMacro(BaseV,double,3);

  // Description:
  // Set the unit vector for Z
  vtkSetVector3Macro(BaseW,double);
  vtkGetVectorMacro(BaseW,double,3);

  // Description:
  // Set the range that will be used for cube axis label along the vector for X
  vtkSetVector2Macro(LabelRangeU,double);
  vtkGetVectorMacro(LabelRangeU,double,2);

  // Description:
  // Set the range that will be used for cube axis label along the vector for Y
  vtkSetVector2Macro(LabelRangeV,double);
  vtkGetVectorMacro(LabelRangeV,double,2);

  // Description:
  // Set the range that will be used for cube axis label along the vector for Z
  vtkSetVector2Macro(LabelRangeW,double);
  vtkGetVectorMacro(LabelRangeW,double,2);

  // Description:
  // Set the oriented bounding box
  vtkSetVector6Macro(OrientedBoundingBox,double);
  vtkGetVectorMacro(OrientedBoundingBox,double,6);

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
  vtkSetMacro(EnableCustomBase, int);
  vtkGetMacro(EnableCustomBase, int);

  // Description:
  // Enable/Disable field generation for oriented bounding box annotation
  vtkSetMacro(EnableCustomTitle, int);
  vtkGetMacro(EnableCustomTitle, int);

  // Description:
  // Enable/Disable LabelRange annotation
  vtkSetMacro(EnableCustomLabelRange, int);
  vtkGetMacro(EnableCustomLabelRange, int);

  // Description:
  // Enable/Disable field generation for label that will be used for "Time:"
  vtkSetMacro(EnableTimeLabel, int);
  vtkGetMacro(EnableTimeLabel, int);

  // Description:
  // Specify custom Time label
  vtkSetStringMacro(TimeLabel);
  vtkGetStringMacro(TimeLabel);

protected:
  vtkShearedCubeSource();
  ~vtkShearedCubeSource();

  void UpdateMetaData(vtkDataSet* ds);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // Needed for time label annotation
  virtual int RequestInformation(vtkInformation *info,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector);

  double BaseU[3];
  double BaseV[3];
  double BaseW[3];
  double LabelRangeU[2];
  double LabelRangeV[2];
  double LabelRangeW[2];
  double LinearTransformU[2];
  double LinearTransformV[2];
  double LinearTransformW[2];
  double OrientedBoundingBox[6];
  char* AxisUTitle;
  char* AxisVTitle;
  char* AxisWTitle;
  char* TimeLabel;

  int EnableCustomBase;
  int EnableCustomTitle;
  int EnableCustomLabelRange;
  int EnableTimeLabel;

private:
  vtkShearedCubeSource(const vtkShearedCubeSource&);  // Not implemented.
  void operator=(const vtkShearedCubeSource&);  // Not implemented.
};

#endif
