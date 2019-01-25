// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalRanges.h

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

// .NAME vtkTemporalRanges - Get the average, min, and max of fields over time.
//
// .SECTION Description
//
// This filter takes in any data set and computes the minimum, maximum, and
// average of the field talking into account all values over all time.  This
// filter is a bit like running both vtkDescriptiveStatistics and
// vtkTemporalStatistics.
//
// In addition to giving descriptive statistics for all field values over space
// and time, it will also give a single statistics over all blocks in a data
// set.
//

#ifndef vtkTemporalRanges_h
#define vtkTemporalRanges_h

#include "vtkSLACFiltersModule.h" // for export macro
#include "vtkTableAlgorithm.h"

class vtkCompositeDataSet;
class vtkDataSet;
class vtkDoubleArray;
class vtkFieldData;

class VTKSLACFILTERS_EXPORT vtkTemporalRanges : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkTemporalRanges, vtkTableAlgorithm);
  static vtkTemporalRanges* New();
  virtual void PrintSelf(ostream& os, vtkIndent indent) override;

  enum
  {
    AVERAGE_ROW,
    MINIMUM_ROW,
    MAXIMUM_ROW,
    COUNT_ROW,
    NUMBER_OF_ROWS
  };

protected:
  vtkTemporalRanges();
  ~vtkTemporalRanges();

  int CurrentTimeIndex;

  virtual int FillInputPortInformation(int port, vtkInformation* info) override;

  virtual int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  virtual int RequestUpdateExtent(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  virtual void InitializeTable(vtkTable* output);

  virtual void AccumulateCompositeData(vtkCompositeDataSet* input, vtkTable* output);
  virtual void AccumulateDataSet(vtkDataSet* input, vtkTable* output);
  virtual void AccumulateFields(vtkFieldData* fields, vtkTable* output);
  virtual void AccumulateArray(vtkDataArray* field, vtkTable* output);

  virtual void AccumulateTable(vtkTable* source, vtkTable* target);

  virtual vtkDoubleArray* GetColumn(vtkTable* table, const char* name, int component);
  virtual vtkDoubleArray* GetColumn(vtkTable* table, const char* name);

private:
  vtkTemporalRanges(const vtkTemporalRanges&) = delete;
  void operator=(const vtkTemporalRanges&) = delete;
};

#endif // vtkTemporalRanges_h
