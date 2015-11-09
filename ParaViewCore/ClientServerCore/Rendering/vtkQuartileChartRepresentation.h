/*=========================================================================

  Program:   ParaView
  Module:    vtkQuartileChartRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQuartileChartRepresentation
// .SECTION Description
// vtkQuartileChartRepresentation extends to vtkXYChartRepresentation to add
// support for combining quartile plots. A quartile plot is created by treating
// multiple input arrays are ranges for area plots. All properties, like color,
// label etc. are specified collectively.

#ifndef vtkQuartileChartRepresentation_h
#define vtkQuartileChartRepresentation_h

#include "vtkXYChartRepresentation.h"

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkQuartileChartRepresentation : public vtkXYChartRepresentation
{
public:
  static vtkQuartileChartRepresentation* New();
  vtkTypeMacro(vtkQuartileChartRepresentation, vtkXYChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overridden to format the series name to remove the operation applied e.g.
  // a columnName of "min(EQPS)" and "max(EQPS)" both will return "EQPS".
  virtual vtkStdString GetDefaultSeriesLabel(
    const vtkStdString& tableName, const vtkStdString& columnName);

  // Description:
  // When set to true, q1/q3 region is rendered.
  vtkSetMacro(QuartileVisibility, bool);
  vtkGetMacro(QuartileVisibility, bool);

  // Description:
  // When set to true, min/max region is rendered.
  vtkSetMacro(RangeVisibility, bool);
  vtkGetMacro(RangeVisibility, bool);

  // Description:
  // When set to true, the avg curve is rendered.
  vtkSetMacro(AverageVisibility, bool);
  vtkGetMacro(AverageVisibility, bool);

  // Description:
  // When set to true, the med curve is rendered.
  vtkSetMacro(MedianVisibility, bool);
  vtkGetMacro(MedianVisibility, bool);

//BTX
protected:
  vtkQuartileChartRepresentation();
  ~vtkQuartileChartRepresentation();

  bool QuartileVisibility;
  bool RangeVisibility;
  bool AverageVisibility;
  bool MedianVisibility;

private:
  vtkQuartileChartRepresentation(const vtkQuartileChartRepresentation&); // Not implemented
  void operator=(const vtkQuartileChartRepresentation&); // Not implemented

  class vtkQCRInternals;
  friend class vtkQCRInternals;
//ETX
};

#endif
