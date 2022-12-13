/*=========================================================================

  Plugin:   DigitalSignalProcessing
  Module:   vtkSoundQuantitiesCalculator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkSoundQuantitiesCalculator
 *
 * Compute the pressure RMS value (Pa and dB) as well as the acoustic power
 * from a sound pressure (Pa) array. Could be improved by adding more conversions.
 *
 * This filter has 2 inputs:
 * - port 0 is the input geometry for cell connectivity.
 * - port 1 is a multi-block dataset representing the data through time. Block flat index
 *   corresponds to the index of the point in the input mesh. This kind of dataset
 *   can be obtained e.g. by applying the filter Plot Data Over Time (vtkExtractDataArraysOverTime)
 *   with the option "Only Report Selection Statistics" turned off.
 *
 * The output is the input geometry with the computed sound quantities attached to
 * it i.e. the mean pressure, the RMS pressure and the acoustic power.
 */

#ifndef vtkSoundQuantitiesCalculator_h
#define vtkSoundQuantitiesCalculator_h

#include "vtkDSPFiltersPluginModule.h"
#include "vtkDataSetAlgorithm.h"

class vtkMultiBlockDataSet;

class VTKDSPFILTERSPLUGIN_EXPORT vtkSoundQuantitiesCalculator : public vtkDataSetAlgorithm
{
public:
  static vtkSoundQuantitiesCalculator* New();
  vtkTypeMacro(vtkSoundQuantitiesCalculator, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the source object containing the temporal data for each point.
   * The source is a vtkMultiBlockDataSet where each block is a vtkTable
   * holding temporal data for one point.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetSourceConnection for connecting the pipeline.
   */
  void SetSourceData(vtkMultiBlockDataSet* source);
  vtkMultiBlockDataSet* GetSource();
  ///@}

  /**
   * Specify the source object containing the temporal data for each point.
   * The source is a vtkMultiBlockDataSet where each block is a vtkTable
   * holding temporal data for one point.
   * This method connects to the pipeline.
   */
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  //@{
  /**
   * Get/set the name of the sound pressure array from which to compute derived quantities.
   * Default is empty.
   */
  vtkGetMacro(PressureArrayName, std::string);
  vtkSetMacro(PressureArrayName, std::string);
  //@}

  //@{
  /**
   * Density of the medium, in kg/m3.
   * Default is the density of the air = 1.2
   */
  vtkGetMacro(MediumDensity, double);
  vtkSetMacro(MediumDensity, double);
  //@}

  //@{
  /**
   * Velocity of the sound in the medium, in m/s.
   * Default is the velocity in the air = 340
   */
  vtkGetMacro(MediumSoundVelocity, double);
  vtkSetMacro(MediumSoundVelocity, double);
  //@}

  /**
   * Specify if mean pressure value through time needs to be computed.
   * Produce a point data array.
   *
   * Default is true.
   */
  vtkGetMacro(ComputeMeanPressure, bool);
  vtkSetMacro(ComputeMeanPressure, bool);

  /**
   * Specify if root mean squared pressure value through time needs to be computed.
   * RMS pressure needs ComputeMeanPressure to be true.
   * Produce a point data array.
   *
   * Default is true.
   */
  vtkGetMacro(ComputeRMSPressure, bool);
  vtkSetMacro(ComputeRMSPressure, bool);

  /**
   * Specify if the sound power over time and over the surface needs to be computed.
   * Acoustic power needs ComputeRMSPressure to be true.
   * Produce a field data array.
   *
   * Default is true.
   */
  vtkGetMacro(ComputePower, bool);
  vtkSetMacro(ComputePower, bool);

protected:
  vtkSoundQuantitiesCalculator();
  ~vtkSoundQuantitiesCalculator() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  std::string PressureArrayName;
  double MediumDensity = 1.2;
  double MediumSoundVelocity = 340.0;
  bool ComputeMeanPressure = true;
  bool ComputeRMSPressure = true;
  bool ComputePower = true;

  /**
   * Fill @c output with the required array.
   * This method only use internals variables for computing the result.
   *
   * The input is internally triangulated to be able to compute the
   * acoustic power (if needed).
   */
  int ProcessData(vtkDataSet* inputMesh, vtkMultiBlockDataSet* inputTables, vtkDataSet* output);

  vtkSoundQuantitiesCalculator(const vtkSoundQuantitiesCalculator&) = delete;
  void operator=(const vtkSoundQuantitiesCalculator&) = delete;
};

#endif // vtkSoundQuantitiesCalculator_h
