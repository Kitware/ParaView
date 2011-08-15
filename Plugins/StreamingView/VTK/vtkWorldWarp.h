/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWorldWarp.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWorldWarp - Warps flat data onto the surface of the globe
// .SECTION Description
// This filter warps geometry defined on a flat coordinate space
// onto a spherical one. It was developed to project LANL's climate data
// onto the surface of the earth.
// The specific mapping is controllable and can use either raw point
// coordinates (which are expected to be in degrees for lon and lat and
// meters for altitude) or point coordinates mapped through lon/lat/alt
// lookup tables as input (for which the coordinates must be in the domain
// of the appropriate lookup table). The lookup tables are read from an
// auxiallary file.
// The filter properly warps piece bounding box meta information so that
// streaming will be able to make view dependent culling and prioritization
// of warped pieces.
// The filter makes no attempt to warp vector and normal field data.

#ifndef __vtkWorldWarp_h
#define __vtkWorldWarp_h

#include "vtkPolyDataAlgorithm.h"

class VTK_EXPORT vtkWorldWarp : public vtkPolyDataAlgorithm
{
public:
  static vtkWorldWarp *New();
  vtkTypeMacro(vtkWorldWarp, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //Specifies whether X, Y or Z input varies the output longitude
  //Default is 0, meaning X.
  vtkSetClampMacro(LonInput, int, 0, 2);
  vtkGetMacro(LonInput, int);

  //Description:
  //Specifies whether X, Y or Z input varies the output latitude
  //Default is 1, meaning Y.
  vtkSetClampMacro(LatInput, int, 0, 2);
  vtkGetMacro(LatInput, int);

  //Description:
  //Specifies whether X, Y or Z input varies the output alt
  //Default is 2, meaning Z.
  vtkSetClampMacro(AltInput, int, 0, 2);
  vtkGetMacro(AltInput, int);

  //Description:
  //Specifies pre-multiplier for X input so that it can be adjusted before
  //mapping. Assuming loninput is unchanged, aim for -180..180, or
  //0..length(lonmap) if a lonmap is provided.
  vtkSetMacro(XScale, double);
  vtkGetMacro(XScale, double);
  // Description:
  //Specifies offset for X input
  vtkSetMacro(XBias, double);
  vtkGetMacro(XBias, double);

  //Description:
  //Specifies pre-multiplier for Y input so that it can be adjusted before
  //mapping. Assuming loninput is unchanged, aim for -90..90, or
  //0..length(latmap) if a latmap is provided.
  vtkSetMacro(YScale, double);
  vtkGetMacro(YScale, double);
  //Description:
  //Specifies offset for Y input.
  vtkSetMacro(YBias, double);
  vtkGetMacro(YBias, double);

  //Description:
  //Specifies pre-multiplier for Z input so that it can be adjusted before
  //mapping. Assuming loninput is unchanged, aim for meters or
  //0..length(altmap) if a altmap is provided.
  vtkSetMacro(ZScale, double);
  vtkGetMacro(ZScale, double);
  //Description:
  //Specifies offset for Z input.
  vtkSetMacro(ZBias, double);
  vtkGetMacro(ZBias, double);

  //Description:
  //Specifies base level of altitude.
  //Default = 6371000, earth sea level in meters
  vtkSetMacro(BaseAltitude, double);
  vtkGetMacro(BaseAltitude, double);

  //Description:
  //Specifies post-multiplier for altitude so that it can be exagerated.
  //Default = 1.0;
  vtkSetMacro(AltitudeScale, double);
  vtkGetMacro(AltitudeScale, double);

  // Description:
  // Set/Get the name of the file that contains the lon,lat,alt map tables.
  void SetMapFileName(const char *MapFileName);
  vtkGetStringMacro(MapFileName);

protected:
  vtkWorldWarp();
  ~vtkWorldWarp();

  virtual int ProcessRequest(
    vtkInformation *,
    vtkInformationVector **,
    vtkInformationVector *);

  virtual int RequestData(
    vtkInformation *,
    vtkInformationVector **,
    vtkInformationVector *);

  int LatInput;
  int LonInput;
  int AltInput;

  double XScale;
  double XBias;
  double YScale;
  double YBias;
  double ZScale;
  double ZBias;

  double BaseAltitude;
  double AltitudeScale;

  char *MapFileName;
  enum {ON_NONE, ON_LON, ON_LAT, ON_ALT};
  double *LonMap;
  int LonMapSize;
  double *LatMap;
  int LatMapSize;
  double *AltMap;
  int AltMapSize;

  void SwapPoint(double inPoint[3], double outPoint[3]);

private:
  vtkWorldWarp(const vtkWorldWarp&);  // Not implemented.
  void operator=(const vtkWorldWarp&);  // Not implemented.
};


#endif
