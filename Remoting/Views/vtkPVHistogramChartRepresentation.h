/*=========================================================================

  Program:   ParaView
  Module:    vtkPVHistogramChartRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVHistogramChartRepresentation
 * @brief   representation for "Histogram
 * View".
 *
 * vtkPVHistogramChartRepresentation is the vtkPVDataRepresentation subclass for
 * showing an data in the "Histogram View". The representation pipeline extracts
 * histogram from the input dataset and then shows that in the view.
*/

#ifndef vtkPVHistogramChartRepresentation_h
#define vtkPVHistogramChartRepresentation_h

#include "vtkXYChartRepresentation.h"

class vtkDataObject;
class vtkInformationVector;
class vtkPExtractHistogram;
class vtkSelection;

class VTKREMOTINGVIEWS_EXPORT vtkPVHistogramChartRepresentation : public vtkXYChartRepresentation
{
public:
  static vtkPVHistogramChartRepresentation* New();
  vtkTypeMacro(vtkPVHistogramChartRepresentation, vtkXYChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Controls which input data component should be binned, for input arrays
   * with more-than-one component
   */
  void SetComponent(int);
  int GetComponent();
  //@}

  //@{
  /**
   * Controls the number of bins N in the output histogram data
   */
  void SetBinCount(int);
  int GetBinCount();
  //@}

  //@{
  /**
   * Get/Set if first and last bins must be centered around the min and max
   * data. This is only used when UseCustomBinRanges is set to false.
   * Default is false.
   */
  void SetCenterBinsAroundMinAndMax(bool);
  bool GetCenterBinsAroundMinAndMax();
  //@}

  //@{
  /**
   * Get/Set custom bin ranges to use. These are used only when
   * UseCustomBinRanges is set to true.
   */
  void SetCustomBinRanges(double min, double max);
  double* GetCustomBinRanges();
  //@}

  //@{
  /**
   * When set to true, CustomBinRanges will  be used instead of using the full
   * range for the selected array. By default, set to false.
   */
  void SetUseCustomBinRanges(bool);
  bool GetUseCustomBinRanges();
  //@}

  /**
   * Sets the color for the histograms.
   */
  void SetHistogramColor(double r, double g, double b);

  /**
   * Sets the histogram to be color mapped by the scalar.
   */
  void SetUseColorMapping(bool colorMapping);

  /**
   * Sets the lookup table that is used for color mapping by the scalar.
   */
  void SetLookupTable(vtkScalarsToColors* lut);

  /**
   * Set the line style for the histogram.
   */
  void SetHistogramLineStyle(int style);

  /**
   * Method to be overridden to transform input data to a vtkTable.
   */
  vtkSmartPointer<vtkDataObject> TransformInputData(vtkDataObject* data) override;

  //@{
  /**
   * Overload the vtkAlgorithm method to update after the change
   */
  void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, const char* name) override;
  using Superclass::SetInputArrayToProcess;
  //@}

  /**
   * Overridden to transform id-based selection produced by the histogram view
   * to a threshold-based selection.
   */
  bool MapSelectionToInput(vtkSelection*) override;

  /**
   * Inverse of MapSelectionToInput().
   */
  bool MapSelectionToView(vtkSelection* sel) override;

protected:
  vtkPVHistogramChartRepresentation();
  ~vtkPVHistogramChartRepresentation() override;

  void PrepareForRendering() override;

  vtkPExtractHistogram* ExtractHistogram;

private:
  vtkPVHistogramChartRepresentation(const vtkPVHistogramChartRepresentation&) = delete;
  void operator=(const vtkPVHistogramChartRepresentation&) = delete;

  std::string ArrayName;
  int AttributeType;
};

#endif
