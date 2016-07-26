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
#ifndef vtkSQVortexFilter_h
#define vtkSQVortexFilter_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkDataSetAlgorithm.h"

#include <set> // for set
#include <string> // for string

class vtkPVXMLElement;
class vtkInformation;
class vtkInformationVector;

class VTKSCIBERQUEST_EXPORT vtkSQVortexFilter : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkSQVortexFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQVortexFilter *New();

  // Description:
  // Initialize from an xml document.
  int Initialize(vtkPVXMLElement *root);

  // Description:
  // Array selection.
  void AddInputArray(const char *name);
  void ClearInputArrays();

  // Description:
  // Deep copy input arrays to the output. A shallow copy is not possible
  // due to the presence of ghost layers.
  void AddArrayToCopy(const char *name);
  void ClearArraysToCopy();

  // Description:
  // Split vector results into component arrays.
  vtkSetMacro(SplitComponents,int);
  vtkGetMacro(SplitComponents,int);

  // Description:
  // Coompute the magnitude of all multi-component results.
  vtkSetMacro(ResultMagnitude,int);
  vtkGetMacro(ResultMagnitude,int);

  // Description:
  // Compute the rotation, curl(v).
  vtkSetMacro(ComputeRotation,int);
  vtkGetMacro(ComputeRotation,int);

  // Description:
  // Compute helicty, v.curl(v)
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
  // Compute Q criteria (using the definition for compressible flow).
  // In a vortex Q>0.
  vtkSetMacro(ComputeQ,int);
  vtkGetMacro(ComputeQ,int);

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

  // Description:
  // Compute the divergence on a centered stencil.
  vtkSetMacro(ComputeDivergence,int);
  vtkGetMacro(ComputeDivergence,int);

  // Description:
  // Compute the vector gradient on a centered stencil.
  vtkSetMacro(ComputeGradient,int);
  vtkGetMacro(ComputeGradient,int);

  // Description:
  // Compute the eigenvalue diagnostic of Haimes and Kenworth.
  vtkSetMacro(ComputeEigenvalueDiagnostic,int);
  vtkGetMacro(ComputeEigenvalueDiagnostic,int);

  // Description:
  // Compute the vector gradient on a centered stencil.
  vtkSetMacro(ComputeGradientDiagnostic,int);
  vtkGetMacro(ComputeGradientDiagnostic,int);

  // Description:
  // Set the log level.
  // 0 -- no logging
  // 1 -- basic logging
  // .
  // n -- advanced logging
  vtkSetMacro(LogLevel,int);
  vtkGetMacro(LogLevel,int);

protected:
  int RequestDataObject(vtkInformation*,vtkInformationVector** inInfoVec,vtkInformationVector* outInfoVec);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestUpdateExtent(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  vtkSQVortexFilter();
  virtual ~vtkSQVortexFilter();

private:
  // controls to turn on/off array generation
  std::set<std::string> InputArrays;
  std::set<std::string> ArraysToCopy;
  int SplitComponents;
  int ResultMagnitude;
  int ComputeRotation;
  int ComputeHelicity;
  int ComputeNormalizedHelicity;
  int ComputeQ;
  int ComputeLambda;
  int ComputeLambda2;
  int ComputeDivergence;
  int ComputeGradient;
  int ComputeEigenvalueDiagnostic;
  int ComputeGradientDiagnostic;
  //
  int OutputExt[6];
  int DomainExt[6];
  //
  int Mode;
  int LogLevel;

private:
  vtkSQVortexFilter(const vtkSQVortexFilter &) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQVortexFilter &) VTK_DELETE_FUNCTION;
};

#endif
