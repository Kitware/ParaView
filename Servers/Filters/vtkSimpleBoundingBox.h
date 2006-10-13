/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSimpleBoundingBox.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSimpleBoundingBox - Fast Simple Class for dealing with 3D bounds
// .SECTION Description
// vtkSimpleBoundingBox maintains a 3D axis aligned bounding box.  It is very
// lite weight and most of the member functions are in-lined so its very fast
// It is not derived from vtkObject so it can be allocated on the stack
//
// .SECTION see also
// vtkBox

#ifndef __vtkSimpleBoundingBox_h
#define __vtkSimpleBoundingBox_h
#include "vtkType.h"

class VTK_EXPORT vtkSimpleBoundingBox 
{
public:
  //Description:
  // Construct a bounding box with the min point set to 
  // VTK_DOUBLE_MAX and the max point set to VTK_DOUBLE_MIN
  vtkSimpleBoundingBox();
  
  //Description:
  // Change bounding box so it includes the point p
  // Note that the bounding box may have 0 volume if its bounds
  // were just initialized
  void AddPoint(double p[3]);
  
  //Description:
  // Change the bouding box to be the union of itself and bbox
  void AddBox(const vtkSimpleBoundingBox &bbox);
  
  // Description:
  // Change the bounding box so it includes bounds (defined by vtk standard)
  void AddBounds(double bounds[6]);
  
  // Description:
  // Get the bounds of the box (defined by vtk style)
  void GetBounds(double bounds[6]) const;
    
  // Description:
  // Returns 1 if the bounds have been set and 0 if the box is in its
  // initialized state
  int IsValid() const;
  
  // Description:
  // Returns the box to its initialized state
  void Reset();
protected:
  double Bounds[6];
};

inline vtkSimpleBoundingBox::vtkSimpleBoundingBox()
{
  this->Reset();
}

inline void vtkSimpleBoundingBox::Reset()
{
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;    
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = VTK_DOUBLE_MIN;
}

inline void vtkSimpleBoundingBox::AddPoint(double p[3])
{
  if (p[0] < this->Bounds[0])
    {
    this->Bounds[0] = p[0];
    }
  if (p[0] > this->Bounds[1])
    {
    this->Bounds[1] = p[0];
    }
  if (p[1] < this->Bounds[2])
    {
    this->Bounds[2] = p[1];
    }
  if (p[1] > this->Bounds[3])
    {
    this->Bounds[3] = p[1];
    }
  if (p[2] < this->Bounds[4])
    {
    this->Bounds[4] = p[2];
    }
  if (p[2] > this->Bounds[5])
    {
    this->Bounds[5] = p[2];
    }
}

inline void vtkSimpleBoundingBox::AddBox(const vtkSimpleBoundingBox &bbox)
{
  if (bbox.Bounds[0] < this->Bounds[0])
    {
    this->Bounds[0] = bbox.Bounds[0];
    }
  
  if (bbox.Bounds[1] > this->Bounds[1])
    {
    this->Bounds[1] = bbox.Bounds[1];
    }
  
  if (bbox.Bounds[2] < this->Bounds[2])
    {
    this->Bounds[2] = bbox.Bounds[2];
    }
  
  if (bbox.Bounds[3] > this->Bounds[3])
    {
    this->Bounds[3] = bbox.Bounds[3];
    }
  
  if (bbox.Bounds[4] < this->Bounds[4])
    {
    this->Bounds[4] = bbox.Bounds[4];
    }
  
  if (bbox.Bounds[5] > this->Bounds[5])
    {
    this->Bounds[5] = bbox.Bounds[5];
    }
}

inline void vtkSimpleBoundingBox::AddBounds(double bounds[6])
{
  if (bounds[0] < this->Bounds[0])
    {
    this->Bounds[0] = bounds[0];
    }

  if (bounds[1] > this->Bounds[1])
    {
    this->Bounds[1] = bounds[1];
    }

  if (bounds[2] < this->Bounds[2])
    {
    this->Bounds[2] = bounds[2];
    }

  if (bounds[3] > this->Bounds[3])
    {
    this->Bounds[3] = bounds[3];
    }

  if (bounds[4] < this->Bounds[4])
    {
    this->Bounds[4] = bounds[4];
    }

  if (bounds[5] > this->Bounds[5])
    {
    this->Bounds[5] = bounds[5];
    }
}
    
inline void vtkSimpleBoundingBox::GetBounds(double bounds[6]) const
{
  bounds[0] = this->Bounds[0];
  bounds[1] = this->Bounds[1];
  bounds[2] = this->Bounds[2];
  bounds[3] = this->Bounds[3];
  bounds[4] = this->Bounds[4];
  bounds[5] = this->Bounds[5];
}

inline int vtkSimpleBoundingBox::IsValid() const
{
  return ((this->Bounds[0] <= this->Bounds[1]) && 
          (this->Bounds[2] <= this->Bounds[3]) && 
          (this->Bounds[4] <= this->Bounds[5]));
} 

#endif
