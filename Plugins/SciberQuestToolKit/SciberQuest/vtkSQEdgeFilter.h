/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#ifndef __vtkSQEdgeFilter_h
#define __vtkSQEdgeFilter_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkDataSetAlgorithm.h"

class vtkInformation;
class vtkInformationVector;

class VTKSCIBERQUEST_EXPORT  vtkSQEdgeFilter : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkSQEdgeFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQEdgeFilter *New();

  // Description:
  // Deep copy input arrays to the output. A shallow copy is not possible
  // due to the presence of ghost layers.
  vtkSetMacro(PassInput,int);
  vtkGetMacro(PassInput,int);

  // Description:
  // Split vector results into component arrays.
  vtkSetMacro(SplitComponents,int);
  vtkGetMacro(SplitComponents,int);

  // Description:
  // Compute the gradient
  vtkSetMacro(ComputeGradient,int);
  vtkGetMacro(ComputeGradient,int);

  // Description:
  // Compute laplacian
  vtkSetMacro(ComputeLaplacian,int);
  vtkGetMacro(ComputeLaplacian,int);

protected:
  int RequestDataObject(vtkInformation*,vtkInformationVector** inInfoVec,vtkInformationVector* outInfoVec);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestUpdateExtent(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  vtkSQEdgeFilter();
  virtual ~vtkSQEdgeFilter();

private:
  // controls to turn on/off array generation
  int PassInput;
  int SplitComponents;
  int ComputeGradient;
  int ComputeLaplacian;

  //
  int OutputExt[6];
  int DomainExt[6];

  //
  int Mode;

private:
  vtkSQEdgeFilter(const vtkSQEdgeFilter &); // Not implemented
  void operator=(const vtkSQEdgeFilter &); // Not implemented
};

#endif
