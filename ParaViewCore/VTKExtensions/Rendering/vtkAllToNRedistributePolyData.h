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

/**
 * @class   vtkAllToNRedistributePolyData
 * @brief   do balanced redistribution of cells on from all to n processors
*/

#ifndef vtkAllToNRedistributePolyData_h
#define vtkAllToNRedistributePolyData_h

#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro
#include "vtkWeightedRedistributePolyData.h"

class vtkMultiProcessController;

//*******************************************************************

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkAllToNRedistributePolyData
  : public vtkWeightedRedistributePolyData
{
public:
  vtkTypeMacro(vtkAllToNRedistributePolyData, vtkWeightedRedistributePolyData);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkAllToNRedistributePolyData* New();

  vtkSetMacro(NumberOfProcesses, int);
  vtkGetMacro(NumberOfProcesses, int);

protected:
  vtkAllToNRedistributePolyData();
  ~vtkAllToNRedistributePolyData();

  void MakeSchedule(vtkPolyData*, vtkCommSched*) override;

  int NumberOfProcesses;

private:
  vtkAllToNRedistributePolyData(const vtkAllToNRedistributePolyData&) = delete;
  void operator=(const vtkAllToNRedistributePolyData&) = delete;
};

//****************************************************************

#endif
