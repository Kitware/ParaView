// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME vtkPTemporalRanges - Parallel Temporal Ranges filter
//
// .SECTION Description
//
// vtkPTemporalRanges works basically like its superclass, vtkTemporalRanges,
// except that it works in a data parallel manner.
//

#ifndef vtkPTemporalRanges_h
#define vtkPTemporalRanges_h

#include "vtkSLACFiltersModule.h" // for export macro
#include "vtkTemporalRanges.h"

class vtkMultiProcessController;

class VTKSLACFILTERS_EXPORT vtkPTemporalRanges : public vtkTemporalRanges
{
public:
  vtkTypeMacro(vtkPTemporalRanges, vtkTemporalRanges);
  static vtkPTemporalRanges* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController*);

protected:
  vtkPTemporalRanges();
  ~vtkPTemporalRanges() override;

  vtkMultiProcessController* Controller;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  virtual void Reduce(vtkTable* table);

private:
  vtkPTemporalRanges(const vtkPTemporalRanges&) = delete;
  void operator=(const vtkPTemporalRanges&) = delete;

  class vtkRangeTableReduction;
  friend class vtkRangeTableReduction;
};

#endif // vtkPTemporalRanges_h
