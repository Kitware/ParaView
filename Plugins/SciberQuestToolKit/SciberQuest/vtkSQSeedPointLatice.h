/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQSeedPointLatice - create a set of points on a cartesian latice
// .SECTION Description
// Create a set of points on a cartesian latice, Latice spacing can be linear
// or non-linear. The nonlinearity has the affect of making the seed points
// more dense in the center of the dataset.


#ifndef __vtkSQSeedPointLatice_h
#define __vtkSQSeedPointLatice_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"


//BTX
#undef vtkGetVector2Macro
#define vtkGetVector2Macro(a,b) /*noop*/
//ETX

class VTKSCIBERQUEST_EXPORT vtkSQSeedPointLatice : public vtkPolyDataAlgorithm
{
public:
  static vtkSQSeedPointLatice *New();
  vtkTypeMacro(vtkSQSeedPointLatice,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the bounding box the seed points are generated
  // inside.
  vtkSetVector6Macro(Bounds,double);
  vtkGetVector6Macro(Bounds,double);
  void SetIBounds(double lo, double hi);
  void SetJBounds(double lo, double hi);
  void SetKBounds(double lo, double hi);
  double *GetIBounds();
  double *GetJBounds();
  double *GetKBounds();
  vtkGetVector2Macro(IBounds,double);
  vtkGetVector2Macro(JBounds,double);
  vtkGetVector2Macro(KBounds,double);


  // Description:
  // Set the power to use in the non-linear transform.
  // Value of 0 means no transform is applied, and grid is
  // regular cartesian. When a power is set, the grid is
  // a streatched cartesian grid with higher density at the
  // center of the bounds.
  void SetTransformPower(double *tp);
  void SetTransformPower(double itp, double jtp, double ktp);
  vtkGetVector3Macro(Power,double);

  vtkSetVector3Macro(Transform,int);
  vtkGetVector3Macro(Transform,int);

  // Description:
  // Set the latice resolution in the given direction.
  vtkSetVector3Macro(NX,int);
  vtkGetVector3Macro(NX,int);

protected:
  /// Pipeline internals.
  int FillInputPortInformation(int port,vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);

  vtkSQSeedPointLatice();
  ~vtkSQSeedPointLatice();

private:
  int NumberOfPoints;

  int NX[3];
  double Bounds[6];
  double IBounds[2];

  //BTX
  enum
    {
    TRANSFORM_NONE=0,
    TRANSFORM_LOG=1
    };
  //ETX
  int Transform[3];
  double Power[3];

private:
  vtkSQSeedPointLatice(const vtkSQSeedPointLatice&);  // Not implemented.
  void operator=(const vtkSQSeedPointLatice&);  // Not implemented.
};

#endif
