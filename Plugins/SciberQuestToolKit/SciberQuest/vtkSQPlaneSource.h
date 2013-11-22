/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQPlaneSource.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSQPlaneSource - create an array of quadrilaterals located in a plane
// .SECTION Description
// vtkSQPlaneSource creates an m x n array of quadrilaterals arranged as
// a regular tiling in a plane. The plane is defined by specifying an
// origin point, and then two other points that, together with the
// origin, define two axes for the plane. These axes do not have to be
// orthogonal - so you can create a parallelogram. (The axes must not
// be parallel.) The resolution of the plane (i.e., number of subdivisions) is
// controlled by the ivars XResolution and YResolution.
//
// By default, the plane is centered at the origin and perpendicular to the
// z-axis, with width and height of length 1 and resolutions set to 1.
//
// There are three convenience methods that allow you to easily move the
// plane.  The first, SetNormal(), allows you to specify the plane
// normal. The effect of this method is to rotate the plane around the center
// of the plane, aligning the plane normal with the specified normal. The
// rotation is about the axis defined by the cross product of the current
// normal with the new normal. The second, SetCenter(), translates the center
// of the plane to the specified center point. The third method, Push(),
// allows you to translate the plane along the plane normal by the distance
// specified. (Negative Push values translate the plane in the negative
// normal direction.)  Note that the SetNormal(), SetCenter() and Push()
// methods modify the Origin, Point1, and/or Point2 instance variables.

// .SECTION Caveats
// The normal to the plane will point in the direction of the cross product
// of the first axis (Origin->Point1) with the second (Origin->Point2). This
// also affects the normals to the generated polygons.

#ifndef __vtkSQPlaneSource_h
#define __vtkSQPlaneSource_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPVXMLElement;

class VTKSCIBERQUEST_EXPORT vtkSQPlaneSource : public vtkPolyDataAlgorithm
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkSQPlaneSource,vtkPolyDataAlgorithm);

  // Description:
  // Construct plane perpendicular to z-axis, resolution 1x1, width
  // and height 1.0, and centered at the origin.
  static vtkSQPlaneSource *New();

  // Description:
  // Initialize the object from an xml document.
  int Initialize(vtkPVXMLElement *root);

  // Description:
  // Mode controls how data is generated. Demand mode generates
  // minimal pseudo dataset and places an object in the pipeline
  // that can be accessed downstream and generate data as needed.
  //BTX
  enum {
    MODE_IMMEDIATE=0,
    MODE_DEMAND=1,
    };
  //ETX
  vtkSetMacro(ImmediateMode,int);
  vtkGetMacro(ImmediateMode,int);

  // Description:
  // Specify the resolution of the plane along the first axes.
  vtkSetMacro(XResolution,int);
  vtkGetMacro(XResolution,int);

  // Description:
  // Specify the resolution of the plane along the second axes.
  vtkSetMacro(YResolution,int);
  vtkGetMacro(YResolution,int);

  // Description:
  // Set the number of x-y subdivisions in the plane.
  void SetResolution(const int xR, const int yR);
  void SetResolution(int res[2]) {
    this->SetResolution(res[0],res[1]); }
  void GetResolution(int& xR,int& yR) {
    xR=this->XResolution; yR=this->YResolution; }

  //BTX
  enum {
    DECOMP_TYPE_PATCHES=0,
    DECOMP_TYPE_STRIPS=1
    };
  //ETX
  // Description:
  // Specify the somain decomposition.
  vtkSetMacro(DecompType,int);
  vtkGetMacro(DecompType,int);

  // Description:
  // Specify a point defining the origin of the plane.
  void SetOrigin(double x, double y, double z);
  void SetOrigin(double pnt[3]);
  vtkGetVectorMacro(Origin,double,3);

  // Description:
  // Specify a point defining the first axis of the plane.
  void SetPoint1(double x, double y, double z);
  void SetPoint1(double pnt[3]);
  vtkGetVectorMacro(Point1,double,3);

  // Description:
  // Specify a point defining the second axis of the plane.
  void SetPoint2(double x, double y, double z);
  void SetPoint2(double pnt[3]);
  vtkGetVectorMacro(Point2,double,3);

  // Description:
  // Set/Get the center of the plane. Works in conjunction with the plane
  // normal to position the plane. Don't use this method to define the plane.
  // Instead, use it to move the plane to a new center point.
  void SetCenter(double x, double y, double z);
  void SetCenter(double center[3]);
  vtkGetVectorMacro(Center,double,3);

  // Description:
  // Set/Get the plane normal. Works in conjunction with the plane center to
  // orient the plane. Don't use this method to define the plane. Instead, use
  // it to rotate the plane around the current center point.
  void SetNormal(double nx, double ny, double nz);
  void SetNormal(double n[3]);
  vtkGetVectorMacro(Normal,double,3);

  // Desciption:
  // Set/Get the constraint, one of: NONE, XY, XZ, or YZ.
  void SetConstraint(int type);
  vtkGetMacro(Constraint,int);

  // Description:
  // Applies the current constraint, by fixing one of the
  // corrdinate values.
  void ApplyConstraint();

  // Description:
  // Set a name that will be placed into the output vtkInformation object
  // in the vtkSQMetaDataKeys::DESCRIPTIVE_NAME() key.
  vtkSetStringMacro(DescriptiveName);
  vtkGetStringMacro(DescriptiveName);

  // Description:
  // Translate the plane in the direction of the normal by the
  // distance specified.  Negative values move the plane in the
  // opposite direction.
  void Push(double distance);

  // Description:
  // Set the log level.
  // 0 -- no logging
  // 1 -- basic logging
  // .
  // n -- advanced logging
  vtkSetMacro(LogLevel,int);
  vtkGetMacro(LogLevel,int);

protected:
  vtkSQPlaneSource();
  virtual ~vtkSQPlaneSource();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *,vtkInformationVector **,vtkInformationVector *outInfos);

  int UpdatePlane(double v1[3], double v2[3], bool quiet=false);

private:
  int ImmediateMode;
  int XResolution;
  int YResolution;
  int DecompType;
  double Origin[3];
  double Point1[3];
  double Point2[3];
  double Normal[3];
  double Center[3];
  int Constraint;
  char *DescriptiveName;
  int LogLevel;

private:
  vtkSQPlaneSource(const vtkSQPlaneSource&);  // Not implemented.
  void operator=(const vtkSQPlaneSource&);  // Not implemented.
};

#endif
