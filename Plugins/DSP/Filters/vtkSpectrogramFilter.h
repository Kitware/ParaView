// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkSpectrogramFilter
 *
 * This filter computes the spectrogram of the input vtkTable column.
 * The output is a vtkImageData where the X and Y axes correspond to time and
 * frequency, respectively.
 * The spectrogram is computed by applying a FFT on temporal windows each containing
 * a subset of the input samples. The window size and type can be controlled with the
 * time resolution and window type properties, respectively. Sample overlap between
 * consecutive windows can also be specified to better capture the frequency detail
 * of the input signal.
 */

#ifndef vtkSpectrogramFilter_h
#define vtkSpectrogramFilter_h

#include "vtkDSPFiltersPluginModule.h" // For export
#include "vtkImageAlgorithm.h"
#include "vtkTableFFT.h" // For enum

class VTKDSPFILTERSPLUGIN_EXPORT vtkSpectrogramFilter : public vtkImageAlgorithm
{
public:
  static vtkSpectrogramFilter* New();
  vtkTypeMacro(vtkSpectrogramFilter, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/set the windowing function for the FFT.
   * Default is Hanning.
   */
  vtkGetMacro(WindowType, int);
  vtkSetClampMacro(WindowType, int, vtkTableFFT::HANNING, vtkTableFFT::RECTANGULAR);
  ///@}

  ///@{
  /**
   * Get/set the length of the temporal window from which to compute the spectrum.
   * Default is 100 time samples.
   */
  vtkGetMacro(TimeResolution, int);
  vtkSetMacro(TimeResolution, int);
  ///@}

  ///@{
  /**
   * Get/set the percentage of overlap between consecutive temporal windows.
   * Default is 50%.
   */
  vtkGetMacro(OverlapPercentage, int);
  vtkSetClampMacro(OverlapPercentage, int, 0, 99);
  ///@}

  ///@{
  /**
   * Get/set the default sampling rate in Hz.
   * This value is used for the FFT if no array called "Time" (case insensitive) is present.
   * Default is 10000 Hz.
   */
  vtkGetMacro(DefaultSampleRate, double);
  vtkSetMacro(DefaultSampleRate, double);
  ///@}

protected:
  vtkSpectrogramFilter() = default;
  ~vtkSpectrogramFilter() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double ComputeSampleRate(vtkTable* input);

private:
  vtkSpectrogramFilter(const vtkSpectrogramFilter&) = delete;
  void operator=(const vtkSpectrogramFilter&) = delete;

  int WindowType = vtkTableFFT::HANNING;
  int TimeResolution = 100;
  int OverlapPercentage = 50;
  double DefaultSampleRate = 1e4;
};

#endif // vtkSpectrogramFilter_h
