// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPTemporalRanges.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

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
  virtual void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController*);

protected:
  vtkPTemporalRanges();
  ~vtkPTemporalRanges();

  vtkMultiProcessController* Controller;

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  virtual void Reduce(vtkTable* table);

private:
  vtkPTemporalRanges(const vtkPTemporalRanges&) = delete;
  void operator=(const vtkPTemporalRanges&) = delete;

  class vtkRangeTableReduction;
  friend class vtkRangeTableReduction;
};

#endif // vtkPTemporalRanges_h
