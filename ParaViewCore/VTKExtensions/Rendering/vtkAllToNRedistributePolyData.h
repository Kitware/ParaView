/*=========================================================================

  Program:   ParaView
  Module:    vtkAllToNRedistributePolyData.h

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

// .NAME vtkAllToNRedistributePolyData - do balanced redistribution of cells on from all to n processors

#ifndef __vtkAllToNRedistributePolyData_h
#define __vtkAllToNRedistributePolyData_h

#include "vtkWeightedRedistributePolyData.h"
class vtkMultiProcessController;

//*******************************************************************

class VTK_EXPORT vtkAllToNRedistributePolyData : public vtkWeightedRedistributePolyData
{
public:
  vtkTypeMacro(vtkAllToNRedistributePolyData, vtkWeightedRedistributePolyData);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  static vtkAllToNRedistributePolyData *New();

  vtkSetMacro(NumberOfProcesses, int);
  vtkGetMacro(NumberOfProcesses, int);


protected:
  vtkAllToNRedistributePolyData();
  ~vtkAllToNRedistributePolyData();

  void MakeSchedule (vtkPolyData*, vtkCommSched*);

  int NumberOfProcesses;

private:
  vtkAllToNRedistributePolyData(const vtkAllToNRedistributePolyData&); // Not implemented
  void operator=(const vtkAllToNRedistributePolyData&); // Not implemented
};

//****************************************************************

#endif


