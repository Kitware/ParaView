/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQAgyrotropyFilter -
// .SECTION Description
// Compute agyrotropy as described in:
//
// JOURNAL OF GEOPHYSICAL RESEARCH, VOL. 113, A06222, 2008,
// "Illuminating electron diffusion regions of collisionless
// magnetic reconnection using electron agyrotropy",
// Jack Scudder and William Daughton

#ifndef __vtkSQAgyrotropyFilter_h
#define __vtkSQAgyrotropyFilter_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkDataSetAlgorithm.h"

class vtkPVXMLElement;

class VTKSCIBERQUEST_EXPORT vtkSQAgyrotropyFilter : public vtkDataSetAlgorithm
{
public:
  static vtkSQAgyrotropyFilter *New();

  vtkTypeMacro(vtkSQAgyrotropyFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the object from an xml document.
  int Initialize(vtkPVXMLElement *root);

  // Description:
  // Set/Get the noise threshold, above which negative discriminant
  // is corrected for. eg: if (-nt < val < 0) then val=0
  vtkSetMacro(NoiseThreshold,double);
  vtkGetMacro(NoiseThreshold,double);

  // Description:
  // Set the log level.
  // 0 -- no logging
  // 1 -- basic logging
  // .
  // n -- advanced logging
  vtkSetMacro(LogLevel,int);
  vtkGetMacro(LogLevel,int);

protected:
  vtkSQAgyrotropyFilter();
  ~vtkSQAgyrotropyFilter();

  // VTK Pipeline
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);

private:
  vtkSQAgyrotropyFilter(const vtkSQAgyrotropyFilter&);  // Not implemented.
  void operator=(const vtkSQAgyrotropyFilter&);  // Not implemented.

private:
  double NoiseThreshold;
  int LogLevel;
};

#endif
