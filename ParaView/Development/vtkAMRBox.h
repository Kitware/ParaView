/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRBox.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAMRBox -
// .SECTION Description

#ifndef __vtkAMRBox_h
#define __vtkAMRBox_h

#include "vtkObject.h"

class VTK_EXPORT vtkAMRBox
{
public:
  int LoCorner[3];
  int HiCorner[3];

  vtkAMRBox() 
    {
      for(int i=0; i<3; i++)
        {
        this->LoCorner[i] = this->HiCorner[i] = 0;
        }
    }

  vtkAMRBox(int dimensionality, int* loCorner, int* hiCorner) 
    {
      this->LoCorner[2] = this->HiCorner[2] = 0;
      memcpy(this->LoCorner, loCorner, dimensionality*sizeof(int));
      memcpy(this->HiCorner, hiCorner, dimensionality*sizeof(int));
    }
    
  vtkIdType GetNumberOfCells()
    {
      vtkIdType numCells=1;
      for(int i=0; i<3; i++)
        {
        numCells *= HiCorner[i] - LoCorner[i] + 1;
        }
      return numCells;
    }

  void Coarsen(int refinement)
    {
      for (int i=0; i<3; i++)
        {
        this->LoCorner[i] = 
          ( this->LoCorner[i] < 0 ? 
            -abs(this->LoCorner[i]+1)/refinement - 1 :
            this->LoCorner[i]/refinement );
        this->HiCorner[i] = 
          ( this->HiCorner[i] < 0 ? 
            -abs(this->HiCorner[i]+1)/refinement - 1 :
            this->HiCorner[i]/refinement );
        }
    }

  void Refine(int refinement)
    {
      for (int i=0; i<3; i++)
        {
        this->LoCorner[i] = this->LoCorner[i]*refinement;
        this->HiCorner[i] = this->HiCorner[i]*refinement;
        }
    }

  int DoesContainCell(int i, int j, int k)
    {
      return 
        i >= this->LoCorner[0] && i <= this->HiCorner[0] &&
        j >= this->LoCorner[1] && j <= this->HiCorner[1] &&
        k >= this->LoCorner[2] && k <= this->HiCorner[2];
    }
};

#endif






