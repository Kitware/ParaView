/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQBinaryThreshold - Color an array using a threshold, low, and high value.
// .SECTION Description
// Given a threshold value and an array replace entries below the threshold
// with a low value and entries above the partition with a high value.

#ifndef __vtkSQBinaryThreshold_h
#define __vtkSQBinaryThreshold_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkDataSetAlgorithm.h"

class vtkPVXMLElement;

class VTKSCIBERQUEST_EXPORT vtkSQBinaryThreshold : public vtkDataSetAlgorithm
{
public:
  static vtkSQBinaryThreshold *New();

  vtkTypeMacro(vtkSQBinaryThreshold,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the object from an xml document.
  int Initialize(vtkPVXMLElement *root);

  // Description:
  // Set/Get the noise threshold, above which negative discriminant
  // is corrected for. eg: if (-nt < val < 0) then val=0
  vtkSetMacro(Threshold,double);
  vtkGetMacro(Threshold,double);

  // Description:
  // Set/get the value to substitute for values below the threshold
  vtkSetMacro(LowValue,double);
  vtkGetMacro(LowValue,double);

  // Description:
  // Set/get the value to substitute for values above the threshold
  vtkSetMacro(HighValue,double);
  vtkGetMacro(HighValue,double);

  // Description:
  // Set/Get the flag that controls if the low value is applied.
  vtkSetMacro(UseLowValue,int);
  vtkGetMacro(UseLowValue,int);

  // Description:
  // Set/Get the flag that controls if the high value is applied.
  vtkSetMacro(UseHighValue,int);
  vtkGetMacro(UseHighValue,int);

  // Description:
  // Set the log level.
  // 0 -- no logging
  // 1 -- basic logging
  // .
  // n -- advanced logging
  vtkSetMacro(LogLevel,int);
  vtkGetMacro(LogLevel,int);

protected:
  vtkSQBinaryThreshold();
  ~vtkSQBinaryThreshold();

  // VTK Pipeline
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);

private:
  vtkSQBinaryThreshold(const vtkSQBinaryThreshold&);  // Not implemented.
  void operator=(const vtkSQBinaryThreshold&);  // Not implemented.

private:
  double Threshold;
  double LowValue;
  double HighValue;
  int UseLowValue;
  int UseHighValue;
  int LogLevel;
};

#endif
