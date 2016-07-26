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
// .NAME vtkSQFTLE
// .SECTION Description
// Compute the FTLE given a displacement map. For an explanation of this
// terminology see http://amath.colorado.edu/cmsms/index.php?page=ftle-of-the-standard-map
//
// .SECTION Caveats
// .SECTION See Also

#ifndef vtkSQFTLE_h
#define vtkSQFTLE_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkDataSetAlgorithm.h"

#include <set> // for set
#include <string> // for string

class vtkInformation;
class vtkInformationVector;
class vtkPVXMLElement;

class VTKSCIBERQUEST_EXPORT vtkSQFTLE : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkSQFTLE,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct to compute the gradient of the scalars and vectors.
  static vtkSQFTLE *New();

  // Description:
  // Initialize from an xml document.
  int Initialize(vtkPVXMLElement *root);

  // Description:
  // Array selection.
  void AddInputArray(const char *name);
  void ClearInputArrays();

  // Description:
  // Shallow copy input data arrays to the output.
  vtkSetMacro(PassInput,int);
  vtkGetMacro(PassInput,int);

  // Description:
  // Set/Get the time interval overwhich displacement map was
  // integrated.
  vtkSetMacro(TimeInterval,double);
  vtkGetMacro(TimeInterval,double);

  // Description:
  // Set the log level.
  // 0 -- no logging
  // 1 -- basic logging
  // .
  // n -- advanced logging
  vtkSetMacro(LogLevel,int);
  vtkGetMacro(LogLevel,int);

protected:
  vtkSQFTLE();
  ~vtkSQFTLE(){}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  std::set<std::string> InputArrays;
  int PassInput;
  double TimeInterval;
  int LogLevel;

private:
  vtkSQFTLE(const vtkSQFTLE&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQFTLE&) VTK_DELETE_FUNCTION;
};

#endif
