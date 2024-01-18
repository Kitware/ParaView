// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVProbeLineFilter
 * @brief Filter to simplify probe line usage when probing over a simple line.
 *
 * Internal Paraview filters for API backward compatibilty and ease of use.
 * Internally build a line source as well as a vtkProbeLineFilter and exposes
 * their properties.
 */

#ifndef vtkPVProbeLineFilter_h
#define vtkPVProbeLineFilter_h

#include "vtkNew.h"                                      // needed for internal filters
#include "vtkPVVTKExtensionsFiltersParallelDIY2Module.h" //needed for exports
#include "vtkPolyDataAlgorithm.h"

class vtkLineSource;
class vtkProbeLineFilter;

class VTKPVVTKEXTENSIONSFILTERSPARALLELDIY2_EXPORT vtkPVProbeLineFilter
  : public vtkPolyDataAlgorithm
{
public:
  static vtkPVProbeLineFilter* New();
  vtkTypeMacro(vtkPVProbeLineFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Shallow copy the input cell data arrays to the output.
   * Off by default.
   */
  vtkSetMacro(PassCellArrays, bool);
  vtkBooleanMacro(PassCellArrays, bool);
  vtkGetMacro(PassCellArrays, bool);
  ///@}

  ///@{
  /**
   * Shallow copy the input point data arrays to the output
   * Off by default.
   */
  vtkSetMacro(PassPointArrays, bool);
  vtkBooleanMacro(PassPointArrays, bool);
  vtkGetMacro(PassPointArrays, bool);
  ///@}

  ///@{
  /**
   * Set whether to pass the field-data arrays from the Input i.e. the input
   * providing the geometry to the output. On by default.
   */
  vtkSetMacro(PassFieldArrays, bool);
  vtkBooleanMacro(PassFieldArrays, bool);
  vtkGetMacro(PassFieldArrays, bool);
  ///@}

  ///@{
  /**
   * Set the tolerance used to compute whether a point in the
   * source is in a cell of the input.  This value is only used
   * if ComputeTolerance is off.
   */
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);
  ///@}

  ///@{
  /**
   * Set whether to use the Tolerance field or precompute the tolerance.
   * When on, the tolerance will be computed and the field
   * value is ignored. On by default.
   */
  vtkSetMacro(ComputeTolerance, bool);
  vtkBooleanMacro(ComputeTolerance, bool);
  vtkGetMacro(ComputeTolerance, bool);
  ///@}

  ///@{
  /**
   * When dealing with composite datasets, partial arrays are common i.e.
   * data-arrays that are not available in all of the blocks. By default, this
   * filter only passes those point and cell data-arrays that are available in
   * all the blocks i.e. partial array are removed.  When PassPartialArrays is
   * turned on, this behavior is changed to take a union of all arrays present
   * thus partial arrays are passed as well. However, for composite dataset
   * input, this filter still produces a non-composite output. For all those
   * locations in a block of where a particular data array is missing, this
   * filter uses vtkMath::Nan() for double and float arrays, while 0 for all
   * other types of arrays i.e int, char etc.
   */
  vtkSetMacro(PassPartialArrays, bool);
  vtkGetMacro(PassPartialArrays, bool);
  vtkBooleanMacro(PassPartialArrays, bool);
  ///@}

  ///@{
  /**
   * Setter and getter for `SamplingPattern` (values to be taken from the enumeration
   * vtkProbeLineFilter::SamplingPattern).
   */
  vtkGetMacro(SamplingPattern, int);
  vtkSetClampMacro(SamplingPattern, int, 0, 2);
  ///@}

  ///@{
  /**
   * Setter and getter for `LineResolution`. This attribute is only used if sampling
   * using `SamplingPattern::SAMPLE_LINE_UNIFORMLY`. It sets the number of points
   * in the sampling line.
   */
  vtkGetMacro(LineResolution, int);
  vtkSetMacro(LineResolution, int);
  ///@}

  ///@{
  /**
   * Get/Set the begin and end points for the line to probe against.
   */
  vtkGetVector3Macro(Point1, double);
  vtkSetVector3Macro(Point1, double);
  vtkGetVector3Macro(Point2, double);
  vtkSetVector3Macro(Point2, double);
  ///@}

protected:
  vtkPVProbeLineFilter();
  ~vtkPVProbeLineFilter() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

  int SamplingPattern = 0;
  int LineResolution = 1000;
  bool PassPartialArrays = false;
  bool PassCellArrays = false;
  bool PassPointArrays = false;
  bool PassFieldArrays = false;
  bool ComputeTolerance = true;
  double Tolerance = 1.0;
  double Point1[3] = { 0, 0, 0 };
  double Point2[3] = { 1, 1, 1 };

  vtkNew<vtkLineSource> LineSource;
  vtkNew<vtkProbeLineFilter> Prober;

private:
  vtkPVProbeLineFilter(const vtkPVProbeLineFilter&) = delete;
  void operator=(const vtkPVProbeLineFilter&) = delete;
};

#endif // vtkPVProbeLineFilter_h
