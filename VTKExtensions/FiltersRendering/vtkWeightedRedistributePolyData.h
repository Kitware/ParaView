// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Los Alamos National Laboratory
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWeightedRedistributePolyData
 * @brief   do weighted balance of cells on processors
 */

#ifndef vtkWeightedRedistributePolyData_h
#define vtkWeightedRedistributePolyData_h

#include "vtkPVVTKExtensionsFiltersRenderingModule.h" // needed for export macro
#include "vtkRedistributePolyData.h"

class vtkMultiProcessController;

//*******************************************************************

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkWeightedRedistributePolyData
  : public vtkRedistributePolyData
{
public:
  vtkTypeMacro(vtkWeightedRedistributePolyData, vtkRedistributePolyData);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object.
   */
  static vtkWeightedRedistributePolyData* New();

  void SetWeights(int, int, float);

protected:
  vtkWeightedRedistributePolyData();
  ~vtkWeightedRedistributePolyData() override;

  enum
  {
    NUM_LOC_CELLS_TAG = 70,

    SCHED_LEN_1_TAG = 300,
    SCHED_LEN_2_TAG = 301,
    SCHED_1_TAG = 310,
    SCHED_2_TAG = 311
  };

  void MakeSchedule(vtkPolyData* input, vtkCommSched*) override;
  float* Weights;

private:
  vtkWeightedRedistributePolyData(const vtkWeightedRedistributePolyData&) = delete;
  void operator=(const vtkWeightedRedistributePolyData&) = delete;
};

//****************************************************************

#endif
