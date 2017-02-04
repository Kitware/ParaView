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

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVHistogramChartRepresentation
  : public vtkXYChartRepresentation
{
public:
  static vtkPVHistogramChartRepresentation* New();
  vtkTypeMacro(vtkPVHistogramChartRepresentation, vtkXYChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

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
   * Method to be overrided to transform input data to a vtkTable.
   */
  virtual vtkDataObject* TransformInputData(
    vtkInformationVector** inputVector, vtkDataObject* data) VTK_OVERRIDE;

  //@{
  /**
   * Overload the vtkAlgorithm method to update after the change
   */
  virtual void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, const char* name) VTK_OVERRIDE;
  using Superclass::SetInputArrayToProcess;
  //@}

  /**
   * Overridden to transform id-based selection produced by the histogram view
   * to a threshold-based selection.
   */
  virtual bool MapSelectionToInput(vtkSelection*) VTK_OVERRIDE;

  /**
   * Inverse of MapSelectionToInput().
   */
  virtual bool MapSelectionToView(vtkSelection* sel) VTK_OVERRIDE;

protected:
  vtkPVHistogramChartRepresentation();
  ~vtkPVHistogramChartRepresentation();

  virtual void PrepareForRendering() VTK_OVERRIDE;

  vtkPExtractHistogram* ExtractHistogram;

private:
  vtkPVHistogramChartRepresentation(const vtkPVHistogramChartRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVHistogramChartRepresentation&) VTK_DELETE_FUNCTION;

  std::string ArrayName;
  int AttributeType;
};

#endif
