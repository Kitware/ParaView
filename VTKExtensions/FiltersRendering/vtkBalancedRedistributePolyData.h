// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Los Alamos National Laboratory
// SPDX-License-Identifier: BSD-3-Clause
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
  ~vtkBalancedRedistributePolyData() override;
  void MakeSchedule(vtkPolyData*, vtkCommSched*) override;

private:
  vtkBalancedRedistributePolyData(const vtkBalancedRedistributePolyData&) = delete;
  void operator=(const vtkBalancedRedistributePolyData&) = delete;
};

//****************************************************************

#endif
