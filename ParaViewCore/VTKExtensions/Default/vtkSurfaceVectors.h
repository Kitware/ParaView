/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSurfaceVectors.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSurfaceVectors - Constrains vectors to surface.
// .SECTION Description
// This filter works on point vectors.  It does not work on cell vectors yet.
// A normal is conputed for a point by averaging normals of surrounding
// 2D cells.  The vector is then constrained to be perpendicular to the normal.

#ifndef __vtkSurfaceVectors_h
#define __vtkSurfaceVectors_h

#include "vtkDataSetAlgorithm.h"

class vtkFloatArray;
class vtkIdList;

class VTK_EXPORT vtkSurfaceVectors : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkSurfaceVectors,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSurfaceVectors *New();

//BTX
  enum ConstraintMode {
    Parallel = 0,
    Perpendicular,
    PerpendicularScale
  };
//ETX

  // Description:
  // This mode determines whether this filter projects vectors to be 
  // perpendicular to surface or parallel to surface.  
  // It defaults to parallel.
  vtkSetMacro(ConstraintMode,int);
  vtkGetMacro(ConstraintMode,int);
  void SetConstraintModeToParallel() 
    {this->SetConstraintMode(vtkSurfaceVectors::Parallel);}
  void SetConstraintModeToPerpendicular() 
    {this->SetConstraintMode(vtkSurfaceVectors::Perpendicular);}
  void SetConstraintModeToPerpendicularScale() 
    {this->SetConstraintMode(vtkSurfaceVectors::PerpendicularScale);}
    
protected:
  vtkSurfaceVectors();
  ~vtkSurfaceVectors();

  // Usual data generation method
  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, 
                          vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  int   ConstraintMode;

private:
  vtkSurfaceVectors(const vtkSurfaceVectors&);  // Not implemented.
  void operator=(const vtkSurfaceVectors&);  // Not implemented.
};

#endif
