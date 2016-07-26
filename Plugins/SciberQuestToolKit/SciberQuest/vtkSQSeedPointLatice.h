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
// .NAME vtkSQSeedPointLatice - create a set of points on a cartesian latice
// .SECTION Description
// Create a set of points on a cartesian latice, Latice spacing can be linear
// or non-linear. The nonlinearity has the affect of making the seed points
// more dense in the center of the dataset.


#ifndef vtkSQSeedPointLatice_h
#define vtkSQSeedPointLatice_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"

#undef vtkGetVector2Macro
#define vtkGetVector2Macro(a,b) /*noop*/

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

  enum
    {
    TRANSFORM_NONE=0,
    TRANSFORM_LOG=1
    };

  int Transform[3];
  double Power[3];

private:
  vtkSQSeedPointLatice(const vtkSQSeedPointLatice&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQSeedPointLatice&) VTK_DELETE_FUNCTION;
};

#endif
