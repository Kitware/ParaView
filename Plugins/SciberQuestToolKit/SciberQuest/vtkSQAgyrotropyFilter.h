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
// .NAME vtkSQAgyrotropyFilter -
// .SECTION Description
// Compute agyrotropy as described in:
//
// JOURNAL OF GEOPHYSICAL RESEARCH, VOL. 113, A06222, 2008,
// "Illuminating electron diffusion regions of collisionless
// magnetic reconnection using electron agyrotropy",
// Jack Scudder and William Daughton

#ifndef vtkSQAgyrotropyFilter_h
#define vtkSQAgyrotropyFilter_h

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
  vtkSQAgyrotropyFilter(const vtkSQAgyrotropyFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQAgyrotropyFilter&) VTK_DELETE_FUNCTION;

private:
  double NoiseThreshold;
  int LogLevel;
};

#endif
