/*=========================================================================

  Program:   ParaView
  Module:    vtkWeightedRedistributePolyData.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Los Alamos National Laboratory
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkWeightedRedistributePolyData - do weighted balance of cells on processors

#ifndef __vtkWeightedRedistributePolyData_h
#define __vtkWeightedRedistributePolyData_h

#include "vtkRedistributePolyData.h"

class vtkMultiProcessController;

//*******************************************************************

class VTK_EXPORT vtkWeightedRedistributePolyData : public vtkRedistributePolyData
{
public:
  vtkTypeMacro(vtkWeightedRedistributePolyData, vtkRedistributePolyData);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object.
  static vtkWeightedRedistributePolyData *New();

  void SetWeights (int, int, float );


protected:
  vtkWeightedRedistributePolyData();
  ~vtkWeightedRedistributePolyData();

//BTX
  enum
  {
    NUM_LOC_CELLS_TAG  = 70,
    
    SCHED_LEN_1_TAG    = 300,
    SCHED_LEN_2_TAG    = 301,
    SCHED_1_TAG        = 310,
    SCHED_2_TAG        = 311
  };
//ETX


  virtual void MakeSchedule (vtkCommSched*);
  float* Weights;

private:
  vtkWeightedRedistributePolyData(const vtkWeightedRedistributePolyData&); // Not implemented
  void operator=(const vtkWeightedRedistributePolyData&); // Not implemented
};

//****************************************************************

#endif


