/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPiecewiseFunction.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPiecewiseFunction.h"

#include "vtkObjectFactory.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkPVPiecewiseFunction);
vtkCxxRevisionMacro(vtkPVPiecewiseFunction, "1.1");
//-----------------------------------------------------------------------------
vtkPVPiecewiseFunction::vtkPVPiecewiseFunction()
{
  this->ScalePointsWithRange = 1;
}

//-----------------------------------------------------------------------------
vtkPVPiecewiseFunction::~vtkPVPiecewiseFunction()
{
}

struct vtkPVPiecewiseFunctionNode
{
  double Value[4];
};

//-----------------------------------------------------------------------------
void vtkPVPiecewiseFunction::SetRange(double min, double max)
{
  if (!this->ScalePointsWithRange)
    {
    // Nothing to do.
    return;
    }

  // Scale the control points over the new range.
  double oldrange[2];
  this->GetRange(oldrange);

  if (oldrange[0] == min && oldrange[1] == max)
    {
    // no range change, nothing to do.
    return;
    }


  // Adjust control points to the new range.
  double dold = (oldrange[1] - oldrange[0]);
  dold = (dold > 0) ? dold : 1;

  double dnew = (max -min);
  dnew = (dnew > 0) ? dnew : 1;

  double scale = dnew/dold;

  // Get the current control points.
  vtkstd::vector<vtkPVPiecewiseFunctionNode> controlPoints;
  int num_points = this->GetSize();
  for (int cc=0; cc < num_points; cc++)
    {
    vtkPVPiecewiseFunctionNode node;
    this->GetNodeValue(cc, node.Value);
    node.Value[0] = scale*(node.Value[0] - oldrange[0]) + min;
    controlPoints.push_back(node);
    }

  // Remove old points and add the moved points.
  this->RemoveAllPoints();

  // Now added the control points again.
  vtkstd::vector<vtkPVPiecewiseFunctionNode>::iterator iter;
  for (iter = controlPoints.begin(); iter != controlPoints.end(); ++iter)
    {
    this->AddPoint(iter->Value[0], iter->Value[1], iter->Value[2], 
      iter->Value[3]);
    }
  
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVPiecewiseFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ScalePointsWithRange: " 
    << this->ScalePointsWithRange << endl;
}
