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
/**
 * @class   vtkQuartileChartRepresentation
 *
 * vtkQuartileChartRepresentation extends to vtkXYChartRepresentation to add
 * support for combining quartile plots. A quartile plot is created by treating
 * multiple input arrays are ranges for area plots. All properties, like color,
 * label etc. are specified collectively.
*/

#ifndef vtkQuartileChartRepresentation_h
#define vtkQuartileChartRepresentation_h

#include "vtkXYChartRepresentation.h"

class VTKREMOTINGVIEWS_EXPORT vtkQuartileChartRepresentation : public vtkXYChartRepresentation
{
public:
  static vtkQuartileChartRepresentation* New();
  vtkTypeMacro(vtkQuartileChartRepresentation, vtkXYChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to format the series name to remove the operation applied e.g.
   * a columnName of "min(EQPS)" and "max(EQPS)" both will return "EQPS".
   */
  std::string GetDefaultSeriesLabel(
    const std::string& tableName, const std::string& columnName) override;

  //@{
  /**
   * When set to true, q1/q3 region is rendered.
   */
  vtkSetMacro(QuartileVisibility, bool);
  vtkGetMacro(QuartileVisibility, bool);
  //@}

  //@{
  /**
   * When set to true, min/max region is rendered.
   */
  vtkSetMacro(RangeVisibility, bool);
  vtkGetMacro(RangeVisibility, bool);
  //@}

  //@{
  /**
   * When set to true, the avg curve is rendered.
   */
  vtkSetMacro(AverageVisibility, bool);
  vtkGetMacro(AverageVisibility, bool);
  //@}

  //@{
  /**
   * When set to true, the med curve is rendered.
   */
  vtkSetMacro(MedianVisibility, bool);
  vtkGetMacro(MedianVisibility, bool);
  //@}

  //@{
  /**
   * When set to true, the min curve is rendered.
   */
  vtkSetMacro(MinVisibility, bool);
  vtkGetMacro(MinVisibility, bool);
  //@}

  //@{
  /**
   * When set to true, the max curve is rendered.
   */
  vtkSetMacro(MaxVisibility, bool);
  vtkGetMacro(MaxVisibility, bool);
  //@}

protected:
  vtkQuartileChartRepresentation();
  ~vtkQuartileChartRepresentation() override;

  bool AverageVisibility;
  bool HasOnlyOnePoint;
  bool MaxVisibility;
  bool MedianVisibility;
  bool MinVisibility;
  bool QuartileVisibility;
  bool RangeVisibility;

  void PrepareForRendering() override;

private:
  vtkQuartileChartRepresentation(const vtkQuartileChartRepresentation&) = delete;
  void operator=(const vtkQuartileChartRepresentation&) = delete;

  class vtkQCRInternals;
  friend class vtkQCRInternals;
};

#endif
