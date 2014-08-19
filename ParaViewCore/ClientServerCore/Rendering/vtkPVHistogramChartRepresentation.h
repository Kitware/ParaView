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
// .NAME vtkPVHistogramChartRepresentation
// .SECTION Description

#ifndef __vtkPVHistogramChartRepresentation_h
#define __vtkPVHistogramChartRepresentation_h

#include "vtkXYChartRepresentation.h"

class vtkDataObject;
class vtkInformationVector;
class vtkPExtractHistogram;
class vtkSelection;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVHistogramChartRepresentation : public vtkXYChartRepresentation
{
public:
  static vtkPVHistogramChartRepresentation* New();
  vtkTypeMacro(vtkPVHistogramChartRepresentation, vtkXYChartRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controls which input data component should be binned, for input arrays
  // with more-than-one component
  void SetComponent(int);
  int GetComponent();

  // Description:
  // Controls the number of bins N in the output histogram data
  void SetBinCount(int);
  int GetBinCount();

  // Description:
  // Get/Set custom bin ranges to use. These are used only when
  // UseCustomBinRanges is set to true.
  void SetCustomBinRanges(double min, double max);
  double* GetCustomBinRanges();

  // Description:
  // When set to true, CustomBinRanges will  be used instead of using the full
  // range for the selected array. By default, set to false.
  void SetUseCustomBinRanges(bool);
  bool GetUseCustomBinRanges();

  // Description:
  // Sets the color for the histograms.
  void SetHistogramColor(double r, double g, double b);

  // Description:
  // Set the line style for the histogram.
  void SetHistogramLineStyle(int style);

  // Description:
  // Method to be overrided to transform input data to a vtkTable.
  virtual vtkDataObject* TransformInputData(vtkInformationVector** inputVector,
                                            vtkDataObject* data);

  // Description:
  // Overload the vtkAlgorithm method to update after the change
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
    int fieldAssociation, const char *name);
  using Superclass::SetInputArrayToProcess;

  // Description:
  // Overridden to transform id-based selection produced by the histogram view
  // to a threshold-based selection.
  virtual bool MapSelectionToInput(vtkSelection*);

  // Description:
  // Inverse of MapSelectionToInput().
  virtual bool MapSelectionToView(vtkSelection* sel);

//BTX
protected:
  vtkPVHistogramChartRepresentation();
  ~vtkPVHistogramChartRepresentation();


  virtual void PrepareForRendering();

  vtkPExtractHistogram* ExtractHistogram;

private:
  vtkPVHistogramChartRepresentation(const vtkPVHistogramChartRepresentation&); // Not implemented
  void operator=(const vtkPVHistogramChartRepresentation&); // Not implemented

  std::string ArrayName;
  int AttributeType;
//ETX
};

#endif
