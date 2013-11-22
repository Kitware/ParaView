/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQRandomSeedPoints - create a random cloud of points
// .SECTION Description
// vtkSQRandomSeedPoints is a source object that creates a user-specified number
// of points within a specified radius about a specified center point.
// By default location of the points is random within the sphere. It is
// also possible to generate random points only on the surface of the
// sphere.

#ifndef __vtkSQRandomSeedPoints_h
#define __vtkSQRandomSeedPoints_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"

class VTKSCIBERQUEST_EXPORT vtkSQRandomSeedPoints : public vtkPolyDataAlgorithm
{
public:
  static vtkSQRandomSeedPoints *New();
  vtkTypeMacro(vtkSQRandomSeedPoints,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of points to generate.
  vtkSetClampMacro(NumberOfPoints,int,1,VTK_INT_MAX);
  vtkGetMacro(NumberOfPoints,int);

  // Description:
  // Set the bounding box the seed points are generated
  // inside.
  vtkSetVector6Macro(Bounds,double);
  vtkGetVector6Macro(Bounds,double);


protected:
  /// Pipeline internals.
  int FillInputPortInformation(int port,vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);

  vtkSQRandomSeedPoints();
  ~vtkSQRandomSeedPoints();

  int NumberOfPoints;
  double Bounds[6];

private:
  vtkSQRandomSeedPoints(const vtkSQRandomSeedPoints&);  // Not implemented.
  void operator=(const vtkSQRandomSeedPoints&);  // Not implemented.
};

#endif
