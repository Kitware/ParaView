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

#include <cmath>

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
    if (this->PositionSpline->GetPoints()->GetNumberOfPoints() > 1)
    {
      // This is assuming that the camera is passed in having the first
      // timestep's view up.
      vtkVector3<double> firstPos, secondPos;
      this->PositionSpline->GetPoints()->GetPoint(0, firstPos.GetData());
      this->PositionSpline->GetPoints()->GetPoint(1, secondPos.GetData());
      vtkVector3<double> delta;
      for (int i = 0; i < 3; ++i)
      {
        delta[i] = secondPos[i] - firstPos[i];
      }
      vtkVector3<double> initialViewUp;
      camera->GetViewUp(initialViewUp.GetData());
      delta.Normalize();
      initialViewUp.Normalize();
      // If the initial motion is within 45 degrees of the view up,
      // assume the view up should be the tangent of the direction of
      // motion.  (Roughly, this is a pretty shoddy derivative
      // calculation but it should prevent the view up from lining up
      // with the view plane normal in many cases such as an orbit about
      // the x-axis)
      if (std::abs(delta.Dot(initialViewUp)) > sqrt(0.5))
      {
        vtkVector3<double> p1, p2;
        double t1 = u, t2 = u + 0.05;
        if (t2 > 1)
        {
          t1 = u - 0.05;
          t2 = u;
        }
        this->Evaluate(t1, this->PositionSpline, p1.GetData());
        this->Evaluate(t2, this->PositionSpline, p2.GetData());
        for (int i = 0; i < 3; ++i)
        {
          delta[i] = p2[i] - p1[i];
        }
        delta.Normalize();
        camera->SetViewUp(delta.GetData());
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator2::Evaluate(double u, vtkParametricSpline* spline, double tuple[3])
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

  double temp[3] = { u, 0, 0 };
  spline->Evaluate(temp, tuple, temp);
}

//----------------------------------------------------------------------------
void vtkCameraInterpolator2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
