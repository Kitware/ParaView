/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVisibilityPrioritizer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVisibilityPrioritizer - rejects pieces out of view frustum and sorts by
// distance.
// .SECTION Description
// This filter responds to most executive requests as a simple pass through.
// The exception is in the REQUEST_UPDATE_EXTENT_INFORMATION pass, where it can
// assign a priority based on distance to the camera and inclusion within the
// viewing frustum.
//

#ifndef __vtkVisibilityPrioritizer_h
#define __vtkVisibilityPrioritizer_h

#include "vtkPassInputTypeAlgorithm.h"

class vtkExtractSelectedFrustum;

class VTK_EXPORT vtkVisibilityPrioritizer : public vtkPassInputTypeAlgorithm
{
public:
  static vtkVisibilityPrioritizer *New();
  vtkTypeMacro(vtkVisibilityPrioritizer, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetCameraState(double *EyeUpAt);
  vtkGetVectorMacro(CameraState, double, 9);
  void SetFrustum(double *frustum);
  vtkGetVectorMacro(Frustum, double, 32);

  //an interface to use this class outside of the pipeline
  double CalculatePriority(double *piecebbox, double *pnorm=NULL);

  //Description:
  //When back piece rejection is in effect, this says how back facing
  //a piece must be to be considered hidden. Ideally it would be 0.0,
  //but we make it lower because piece norms are approximations and
  //doing so reduces popping.
  vtkSetMacro(BackFaceFactor, double);
  vtkGetMacro(BackFaceFactor, double);

protected:
  vtkVisibilityPrioritizer();
  ~vtkVisibilityPrioritizer();

  virtual int ProcessRequest(
    vtkInformation *,
    vtkInformationVector **,
    vtkInformationVector *);

  virtual int RequestUpdateExtentInformation(
    vtkInformation *,
    vtkInformationVector **,
    vtkInformationVector *);

  virtual int RequestData(
    vtkInformation *,
    vtkInformationVector **,
    vtkInformationVector *);

  vtkExtractSelectedFrustum *FrustumTester;

  double *CameraState;
  double *Frustum;
  bool Enabled;
  double BackFaceFactor;

private:
  vtkVisibilityPrioritizer(const vtkVisibilityPrioritizer&);  // Not implemented.
  void operator=(const vtkVisibilityPrioritizer&);  // Not implemented.
};


#endif
