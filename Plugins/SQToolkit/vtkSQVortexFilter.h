/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef __vtkSQVortexFilter_h
#define __vtkSQVortexFilter_h

#include "vtkDataSetAlgorithm.h"

class vtkInformation;
class vtkInformationVector;

class vtkSQVortexFilter : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkSQVortexFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQVortexFilter *New();

  // TODO documentaion
  vtkSetMacro(ComputeRotation,int);
  vtkGetMacro(ComputeRotation,int);

  // TODO documentaion
  vtkSetMacro(ComputeHelicity,int);
  vtkGetMacro(ComputeHelicity,int);

  // Description:
  // H_n is the cosine of the angle bteween velocity and vorticty. Near
  // vortex core regions this angle is small. In the limiting case 
  // H_n -> +/-1, and a streamline pasing through this point has no
  // curvature (straight). The sign of H_n indicates the direction of 
  // swirl relative to bulk velocity. Vortex cores might be found by
  // tracing streamlines from H_n maxima/minima.
  vtkSetMacro(ComputeNormalizedHelicity,int);
  vtkGetMacro(ComputeNormalizedHelicity,int);

  // Description:
  // Lambda refers to the Lambda 2 method, where the second of the sorted
  // eigenvalues of the corrected pressure hessian, derived be decomposing
  // the velocity gardient tensor into strain rate tensor(symetric) and spin
  // tensor(anti-symetric), is examined. lambda2<0 indicates the possibility
  // of a vortex.
  vtkSetMacro(ComputeLambda,int);
  vtkGetMacro(ComputeLambda,int);
  vtkSetMacro(ComputeLambda2,int);
  vtkGetMacro(ComputeLambda2,int);

protected:
  //int FillInputPortInformation(int port, vtkInformation *info);
  //int FillOutputPortInformation(int port, vtkInformation *info);
  int RequestDataObject(vtkInformation*,vtkInformationVector** inInfoVec,vtkInformationVector* outInfoVec);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestUpdateExtent(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  vtkSQVortexFilter();
  virtual ~vtkSQVortexFilter();

private:
  // controls to turn on/off array generation
  int ComputeRotation;
  int ComputeHelicity;
  int ComputeNormalizedHelicity;
  int ComputeLambda;
  int ComputeLambda2;

  //
  int OutputExt[6];
  int DomainExt[6];

  //
  int Mode;

private:
  vtkSQVortexFilter(const vtkSQVortexFilter &); // Not implemented
  void operator=(const vtkSQVortexFilter &); // Not implemented
};

#endif
