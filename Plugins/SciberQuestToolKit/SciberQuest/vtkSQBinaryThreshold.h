/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
// .NAME vtkSQBinaryThreshold - Color an array using a threshold, low, and high value.
// .SECTION Description
// Given a threshold value and an array replace entries below the threshold
// with a low value and entries above the partition with a high value.

#ifndef vtkSQBinaryThreshold_h
#define vtkSQBinaryThreshold_h

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
  vtkSQBinaryThreshold(const vtkSQBinaryThreshold&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQBinaryThreshold&) VTK_DELETE_FUNCTION;

private:
  double Threshold;
  double LowValue;
  double HighValue;
  int UseLowValue;
  int UseHighValue;
  int LogLevel;
};

#endif
