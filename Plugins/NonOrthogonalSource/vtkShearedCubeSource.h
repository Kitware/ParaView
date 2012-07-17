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
  // Set the origin of the cub axis
  vtkSetVector3Macro(AxisOrigin,double);
  vtkGetVectorMacro(AxisOrigin,double,3);

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
  vtkSetMacro(EnableCustomBounds, int);
  vtkGetMacro(EnableCustomBounds, int);

  // Description:
  // Enable/Disable field generation for oriented bounding box annotation
  vtkSetMacro(EnableCustomTitle, int);
  vtkGetMacro(EnableCustomTitle, int);

  // Description:
  // Enable/Disable field generation for oriented bounding box annotation
  vtkSetMacro(EnableCustomOrigin, int);
  vtkGetMacro(EnableCustomOrigin, int);


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
  double OrientedBoundingBox[6];
  double AxisOrigin[3];
  char* AxisUTitle;
  char* AxisVTitle;
  char* AxisWTitle;
  char* TimeLabel;

  int EnableCustomBase;
  int EnableCustomBounds;
  int EnableCustomTitle;
  int EnableCustomOrigin;
  int EnableTimeLabel;

private:
  vtkShearedCubeSource(const vtkShearedCubeSource&);  // Not implemented.
  void operator=(const vtkShearedCubeSource&);  // Not implemented.
};

#endif
