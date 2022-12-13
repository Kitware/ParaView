/*=========================================================================

  Plugin:   DigitalSignalProcessing
  Module:   vtkProjectSpectrumMagnitude.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkProjectSpectrumMagnitude
 *
 * This filter computes the mean of some data arrays within a specified range of
 * frequency, and project it on the targeted mesh. The mesh to project into is
 * connected to input 1. Input 0 is  a multiblock of tables containing the FFT
 * of some data for each point of the  mesh. This filter expects that the number
 * of point in the mesh is equal to the number of blocks in the multiblock of table.
 * First block will be projected onto the first mesh point, and so on.
 *
 * Output is a non-temporal dataset with the means as new point datas.
 *
 * Frequency array is selected using the `SetInputArrayToProcess` interface.
 */

#ifndef vtkProjectSpectrumMagnitude_h
#define vtkProjectSpectrumMagnitude_h

#include "vtkDSPFiltersPluginModule.h"
#include "vtkDataSetAlgorithm.h"
#include "vtkNew.h" // for vtkNew

class vtkDataArraySelection;

class VTKDSPFILTERSPLUGIN_EXPORT vtkProjectSpectrumMagnitude : public vtkDataSetAlgorithm
{
public:
  static vtkProjectSpectrumMagnitude* New();
  vtkTypeMacro(vtkProjectSpectrumMagnitude, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the source object corresponding to the destination mesh.
   * The source is a vtkDataSet where the number of points matches the number
   * of input table blocks.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetSourceConnection for connecting the pipeline.
   */
  void SetSourceData(vtkDataSet* source);

  /**
   * Set the source object corresponding to the destination mesh.
   * The source is a vtkDataSet where the number of points matches the number
   * of input table blocks.
   * This method connects to the pipeline.
   */
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  /**
   * Get the current selection of columns to project.
   */
  vtkGetNewMacro(ColumnSelection, vtkDataArraySelection);

  ///@{
  /**
   * Get/set the lower bound of the frequency range to project.
   * Default is 0.0 Hz.
   */
  vtkGetMacro(LowerFrequency, double);
  vtkSetMacro(LowerFrequency, double);
  ///@}

  ///@{
  /**
   * Get/set the upper bound of the frequency range to project.
   * Default is 0.0 Hz.
   */
  vtkGetMacro(UpperFrequency, double);
  vtkSetMacro(UpperFrequency, double);
  ///@}

protected:
  vtkProjectSpectrumMagnitude();
  ~vtkProjectSpectrumMagnitude() override = default;

  int FillInputPortInformation(int, vtkInformation*) override;
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkProjectSpectrumMagnitude(const vtkProjectSpectrumMagnitude&) = delete;
  void operator=(const vtkProjectSpectrumMagnitude&) = delete;

  vtkNew<vtkDataArraySelection> ColumnSelection;
  double LowerFrequency = 0.0;
  double UpperFrequency = 0.0;
};

#endif // vtkProjectSpectrumMagnitude_h
