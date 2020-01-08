/*=========================================================================

  Program:   ParaView
  Module:    vtkBalancedRedistributePolyData.h

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
 * @class   vtkBalancedRedistributePolyData
 * @brief   do balance of cells on processors
*/

#ifndef vtkBalancedRedistributePolyData_h
#define vtkBalancedRedistributePolyData_h

#include "vtkPVVTKExtensionsFiltersRenderingModule.h" // needed for export macro
#include "vtkWeightedRedistributePolyData.h"
class vtkMultiProcessController;

//*******************************************************************

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkBalancedRedistributePolyData
  : public vtkWeightedRedistributePolyData
{
public:
  vtkTypeMacro(vtkBalancedRedistributePolyData, vtkWeightedRedistributePolyData);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkBalancedRedistributePolyData* New();

protected:
  vtkBalancedRedistributePolyData();
  ~vtkBalancedRedistributePolyData();
  void MakeSchedule(vtkPolyData*, vtkCommSched*) override;

private:
  vtkBalancedRedistributePolyData(const vtkBalancedRedistributePolyData&) = delete;
  void operator=(const vtkBalancedRedistributePolyData&) = delete;
};

//****************************************************************

#endif
