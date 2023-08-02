// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Los Alamos National Laboratory
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAllToNRedistributePolyData
 * @brief   do balanced redistribution of cells on from all to n processors
 */

#ifndef vtkAllToNRedistributePolyData_h
#define vtkAllToNRedistributePolyData_h

#include "vtkPVVTKExtensionsFiltersRenderingModule.h" // needed for export macro
#include "vtkWeightedRedistributePolyData.h"

class vtkMultiProcessController;

//*******************************************************************

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkAllToNRedistributePolyData
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
  ~vtkAllToNRedistributePolyData() override;

  void MakeSchedule(vtkPolyData*, vtkCommSched*) override;

  int NumberOfProcesses;

private:
  vtkAllToNRedistributePolyData(const vtkAllToNRedistributePolyData&) = delete;
  void operator=(const vtkAllToNRedistributePolyData&) = delete;
};

//****************************************************************

#endif
