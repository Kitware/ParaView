/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQSphereSource.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSQSphereSource - create a polygonal sphere centered at the origin
// .SECTION Description
// vtkSQSphereSource creates a sphere (represented by polygons) of specified
// radius centered at the origin. The resolution (polygonal discretization)
// in both the latitude (phi) and longitude (theta) directions can be
// specified. It also is possible to create partial spheres by specifying
// maximum phi and theta angles. By default, the surface tessellation of
// the sphere uses triangles; however you can set LatLongTessellation to
// produce a tessellation using quadrilaterals.
// .SECTION Caveats
// Resolution means the number of latitude or longitude lines for a complete
// sphere. If you create partial spheres the number of latitude/longitude
// lines may be off by one.

#ifndef __vtkSQSphereSource_h
#define __vtkSQSphereSource_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_MAX_SPHERE_RESOLUTION 1024

class VTKSCIBERQUEST_EXPORT vtkSQSphereSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSQSphereSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct sphere with radius=0.5 and default resolution 8 in both Phi
  // and Theta directions. Theta ranges from (0,360) and phi (0,180) degrees.
  static vtkSQSphereSource *New();

  // Description:
  // Set radius of sphere. Default is .5.
  vtkSetClampMacro(Radius,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(Radius,double);

  // Description:
  // Set the center of the sphere. Default is 0,0,0.
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);

  // Description:
  // Set the number of points in the longitude direction (ranging from
  // StartTheta to EndTheta).
  vtkSetClampMacro(ThetaResolution,int,3,VTK_MAX_SPHERE_RESOLUTION);
  vtkGetMacro(ThetaResolution,int);

  // Description:
  // Set the number of points in the latitude direction (ranging
  // from StartPhi to EndPhi).
  vtkSetClampMacro(PhiResolution,int,3,VTK_MAX_SPHERE_RESOLUTION);
  vtkGetMacro(PhiResolution,int);

  // Description:
  // Set the starting longitude angle. By default StartTheta=0 degrees.
  vtkSetClampMacro(StartTheta,double,0.0,360.0);
  vtkGetMacro(StartTheta,double);

  // Description:
  // Set the ending longitude angle. By default EndTheta=360 degrees.
  vtkSetClampMacro(EndTheta,double,0.0,360.0);
  vtkGetMacro(EndTheta,double);

  // Description:
  // Set the starting latitude angle (0 is at north pole). By default
  // StartPhi=0 degrees.
  vtkSetClampMacro(StartPhi,double,0.0,360.0);
  vtkGetMacro(StartPhi,double);

  // Description:
  // Set the ending latitude angle. By default EndPhi=180 degrees.
  vtkSetClampMacro(EndPhi,double,0.0,360.0);
  vtkGetMacro(EndPhi,double);

  // Description:
  // Cause the sphere to be tessellated with edges along the latitude
  // and longitude lines. If off, triangles are generated at non-polar
  // regions, which results in edges that are not parallel to latitude and
  // longitude lines. If on, quadrilaterals are generated everywhere
  // except at the poles. This can be useful for generating a wireframe
  // sphere with natural latitude and longitude lines.
  vtkSetMacro(LatLongTessellation,int);
  vtkGetMacro(LatLongTessellation,int);
  vtkBooleanMacro(LatLongTessellation,int);

  // Description:
  // Set a name that will be placed into the output vtkInformation object
  // in the vtkSQMetaDataKeys::DESCRIPTIVE_NAME() key.
  vtkSetStringMacro(DescriptiveName);
  vtkGetStringMacro(DescriptiveName);

protected:
  vtkSQSphereSource(int res=8);
  virtual ~vtkSQSphereSource();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  double Radius;
  double Center[3];
  int ThetaResolution;
  int PhiResolution;
  double StartTheta;
  double EndTheta;
  double StartPhi;
  double EndPhi;
  int LatLongTessellation;
  char *DescriptiveName;

private:
  vtkSQSphereSource(const vtkSQSphereSource&);  // Not implemented.
  void operator=(const vtkSQSphereSource&);  // Not implemented.
};

#endif
