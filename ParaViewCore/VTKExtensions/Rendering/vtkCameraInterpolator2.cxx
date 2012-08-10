/*=========================================================================

  Program:   ParaView
  Module:    vtkCameraInterpolator2.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCameraInterpolator2.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkParametricSpline.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkCameraInterpolator2);
//----------------------------------------------------------------------------
vtkCameraInterpolator2::vtkCameraInterpolator2()
{
  this->FocalPathPoints = vtkPoints::New();
  this->FocalPathPoints->SetDataTypeToDouble();
  this->PositionPathPoints = vtkPoints::New();
  this->PositionPathPoints->SetDataTypeToDouble();

  this->FocalSpline = vtkParametricSpline::New();
  this->FocalSpline->SetPoints(this->FocalPathPoints);
  this->FocalSpline->ParameterizeByLengthOn();

  this->PositionSpline = vtkParametricSpline::New();
  this->PositionSpline->SetPoints(this->PositionPathPoints);
  this->PositionSpline->ParameterizeByLengthOn();

  this->FocalPointMode = PATH;
  this->PositionMode = PATH;
  this->PositionPathInterpolationMode = SPLINE;
  this->FocalPathInterpolationMode = SPLINE;
  this->ClosedFocalPath = false;
  this->ClosedPositionPath = false;
}

//----------------------------------------------------------------------------
vtkCameraInterpolator2::~vtkCameraInterpolator2()
{
  this->FocalPathPoints->Delete();
  this->PositionPathPoints->Delete();
  this->FocalSpline->Delete();
  this->PositionSpline->Delete();
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator2::AddPositionPathPoint(double x, double y, double z)
{
  this->PositionPathPoints->InsertNextPoint(x, y, z);
  this->PositionSpline->Modified();
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator2::ClearPositionPath()
{
  this->PositionPathPoints->Initialize();
  this->PositionSpline->Modified();
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator2::AddFocalPathPoint(double x, double y, double z)
{
  this->FocalPathPoints->InsertNextPoint(x, y, z);
  this->FocalSpline->Modified();
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator2::ClearFocalPath()
{
  this->FocalPathPoints->Initialize();
  this->FocalSpline->Modified();
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator2::InterpolateCamera(double u, vtkCamera* camera)
{
  double tuple[3];

  this->FocalSpline->SetClosed(this->ClosedFocalPath);
  this->PositionSpline->SetClosed(this->ClosedPositionPath);

  if (this->FocalPointMode == PATH)
    {
    this->Evaluate(u, this->FocalSpline, tuple);
    camera->SetFocalPoint(tuple);
    }

  if (this->PositionMode == PATH)
    {
    this->Evaluate(u, this->PositionSpline, tuple);
    camera->SetPosition(tuple);
    }
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator2::Evaluate(double u,
  vtkParametricSpline* spline,
  double tuple[3])
{
  if (spline->GetPoints()->GetNumberOfPoints() <= 0)
    {
    vtkWarningMacro("No path specified.");
    return;
    }

  if (spline->GetPoints()->GetNumberOfPoints() == 1)
    {
    // Fixed point.
    spline->GetPoints()->GetPoint(0, tuple);
    return;
    }

  double temp[3] = {u, 0, 0 };
  spline->Evaluate(temp, tuple, temp);
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


