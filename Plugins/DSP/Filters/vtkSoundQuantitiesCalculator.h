// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSoundQuantitiesCalculator
 *
 * Compute the pressure RMS value (Pa and dB) as well as the acoustic power
 * from a sound pressure (Pa) array. Could be improved by adding more conversions.
 *
 * This filter has 2 inputs:
 * - port 0 is a multiblock dataset or multidimensional table representing the data through time.
 * - port 1 is the input geometry for cell connectivity.
 *
 * The output is the input geometry with the computed sound quantities attached to
 * it i.e. the mean pressure, the RMS pressure and the acoustic power.
 * Input geometry arrays are copied except for the pressure array.
 */

#ifndef vtkSoundQuantitiesCalculator_h
#define vtkSoundQuantitiesCalculator_h

#include "vtkDSPFiltersPluginModule.h"
#include "vtkDataSetAlgorithm.h"

class VTKDSPFILTERSPLUGIN_EXPORT vtkSoundQuantitiesCalculator : public vtkDataSetAlgorithm
{
public:
  static vtkSoundQuantitiesCalculator* New();
  vtkTypeMacro(vtkSoundQuantitiesCalculator, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the source object corresponding to the destination mesh.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetSourceConnection for connecting the pipeline.
   */
  void SetSourceData(vtkDataSet* source);

  /**
   * Set the source object corresponding to the destination mesh.
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
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
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
  int ProcessData(vtkDataSet* inputMesh, vtkDataObject* inputTables, vtkDataSet* output);

  vtkSoundQuantitiesCalculator(const vtkSoundQuantitiesCalculator&) = delete;
  void operator=(const vtkSoundQuantitiesCalculator&) = delete;
};

#endif // vtkSoundQuantitiesCalculator_h
