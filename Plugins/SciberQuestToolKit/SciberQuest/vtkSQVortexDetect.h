/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQVortexDetect.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSQVortexDetect -
// .SECTION Description
// .SECTION Caveats
// .SECTION See Also

#ifndef __vtkSQVortexDetect_h
#define __vtkSQVortexDetect_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkDataSetAlgorithm.h"

class vtkInformation;
class vtkInformationVector;
class vtkPVXMLElement;

class VTKSCIBERQUEST_EXPORT vtkSQVortexDetect : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkSQVortexDetect,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct to compute the gradient of the scalars and vectors.
  static vtkSQVortexDetect *New();

  // Description:
  // Initialize from an xml document.
  int Initialize(vtkPVXMLElement *root);

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
  // Coompute the magnitude of all multi-component results.
  vtkSetMacro(ResultMagnitude,int);
  vtkGetMacro(ResultMagnitude,int);

  // Description:
  // Compute the rotation, curl(v).
  vtkSetMacro(ComputeVorticity,int);
  vtkGetMacro(ComputeVorticity,int);

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
  // Compute FTLE from a displacement map. See vtkSQFieldTracer for
  // information on generating a displacement map.
  vtkSetMacro(ComputeFTLE,int);
  vtkGetMacro(ComputeFTLE,int);

protected:
  vtkSQVortexDetect();
  ~vtkSQVortexDetect(){}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  // controls to turn on/off array generation
  int PassInput;
  int SplitComponents;
  int ResultMagnitude;
  int ComputeGradient;
  int ComputeVorticity;
  int ComputeHelicity;
  int ComputeNormalizedHelicity;
  int ComputeQ;
  int ComputeLambda2;
  int ComputeDivergence;
  int ComputeFTLE;
  int ComputeEigenvalueDiagnostic;

private:
  vtkSQVortexDetect(const vtkSQVortexDetect&);  // Not implemented.
  void operator=(const vtkSQVortexDetect&);  // Not implemented.
};

#endif
