// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkMeanPowerSpectralDensity
 * @brief   Computes the mean power spectral density (PSD) of temporal signals.
 *
 * This filter computes the mean power spectral density (PSD) of temporal signals.
 * The input should contain tables of data arrays where each table typically
 * corresponds to a point/cell.
 *
 * The recommended sequence of filters applied to a vtkDataSet is:
 * vtkTemporalMultiplexing -> vtkDSPTableFFT -> vtkMeanPowerSpectralDensity
 *
 * while this one is also supported:
 * vtkPExtractDataArraysOverTime (Plot Data Over Time) -> vtkTableFFT -> vtkMeanPowerSpectralDensity
 */

#ifndef vtkMeanPowerSpectralDensity_h
#define vtkMeanPowerSpectralDensity_h

#include "vtkDSPFiltersPluginModule.h"
#include "vtkTableAlgorithm.h"

class VTKDSPFILTERSPLUGIN_EXPORT vtkMeanPowerSpectralDensity : public vtkTableAlgorithm
{
public:
  static vtkMeanPowerSpectralDensity* New();
  vtkTypeMacro(vtkMeanPowerSpectralDensity, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/set the name of the FFT array from which to compute the mean PSD.
   * Default is empty.
   */
  vtkGetMacro(FFTArrayName, std::string);
  vtkSetMacro(FFTArrayName, std::string);
  ///@}

  ///@{
  /**
   * Get/set the name of the frequency array to copy to the output.
   * Default is empty.
   */
  vtkGetMacro(FrequencyArrayName, std::string);
  vtkSetMacro(FrequencyArrayName, std::string);
  ///@}

protected:
  vtkMeanPowerSpectralDensity() = default;
  ~vtkMeanPowerSpectralDensity() override = default;

  int FillInputPortInformation(int, vtkInformation*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMeanPowerSpectralDensity(const vtkMeanPowerSpectralDensity&) = delete;
  void operator=(const vtkMeanPowerSpectralDensity&) = delete;

  std::string FFTArrayName;
  std::string FrequencyArrayName;
};

#endif // vtkMeanPowerSpectralDensity_h
