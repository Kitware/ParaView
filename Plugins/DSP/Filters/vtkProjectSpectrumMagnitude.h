// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

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
#include "vtkFFT.h" // for Octave and OctaveSubdivision
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

  ///@{
  /**
   * Get the lower/upper bound of the frequency range to project when using octave bands.
   * This parameter is automatically computed.
   * Default is 0.0 Hz.
   */
  vtkGetMacro(ComputedLowerFrequency, double);
  vtkGetMacro(ComputedUpperFrequency, double);
  ///@}

  ///@{
  /**
   * Get/set whether to compute lower/upper frequency from band octave band number.
   * Default is true.
   */
  vtkGetMacro(FreqFromOctave, bool);
  void SetFreqFromOctave(bool freqFromOctave);
  vtkBooleanMacro(FreqFromOctave, bool);
  ///@}

  ///@{
  /**
   * Get/set whether to use base-two (or base-ten) when computing frequencies with octave.
   * Base-two when true, base-ten when false.
   * Has no effect if FreqFromOctave is false.
   * Default is true.
   */
  vtkGetMacro(BaseTwoOctave, bool);
  void SetBaseTwoOctave(bool baseTwoOctave);
  vtkBooleanMacro(BaseTwoOctave, bool);
  ///@}

  ///@{
  /**
   * Get/set the octave used to compute frequencies.
   * Setter clamps value to enum.
   * Has no effect if FreqFromOctave is false.
   * Default is Hz_500.
   */
  vtkGetMacro(Octave, int);
  void SetOctave(int octave);
  ///@}

  ///@{
  /**
   * Get/set which subdivision of octave used to compute octave frequency range.
   * Setter clamps value to enum.
   * Has no effect if FreqFromOctave is false.
   * Default is Full.
   */
  vtkGetMacro(OctaveSubdivision, int);
  void SetOctaveSubdivision(int octaveSubdivision);
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

  /**
   * Compute frequencies from octave parameters.
   * This must be called in setters because frequencies are gotten in ParaView as information.
   */
  void ComputeFreqFromOctave();

private:
  vtkProjectSpectrumMagnitude(const vtkProjectSpectrumMagnitude&) = delete;
  void operator=(const vtkProjectSpectrumMagnitude&) = delete;

  vtkNew<vtkDataArraySelection> ColumnSelection;
  double LowerFrequency = 0.0;
  double UpperFrequency = 0.0;
  double ComputedLowerFrequency = 0.0;
  double ComputedUpperFrequency = 0.0;
  bool FreqFromOctave = false;
  bool BaseTwoOctave = true;
  int Octave = vtkFFT::Octave::Hz_500;
  int OctaveSubdivision = vtkFFT::OctaveSubdivision::Full;
};

#endif // vtkProjectSpectrumMagnitude_h
