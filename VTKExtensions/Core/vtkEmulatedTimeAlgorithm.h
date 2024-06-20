// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkEmulatedTimeAlgorithm_h
#define vtkEmulatedTimeAlgorithm_h

#include "vtkAlgorithm.h"

#include <vector>

#include "vtkPVVTKExtensionsCoreModule.h"

/**
 * @class vtkEmulatedTimeAlgorithm
 * @brief Provide a base API for a emulated real time LiveSource.
 *
 * This class is intended to be inherited by a temporal algorithm.
 * It allows the algorithm to be updated at regular time intervals,
 * using ParaView LiveSource concept, while also seamlessly
 * functioning with standard vtk animation sequences.
 *
 * An use case to use this class would be is to replicate the
 * behavior of a LiveSource which has been recorded in a file.
 * It's important to note that time in this context is measured in seconds.
 *
 * The inherited class should then be defined as a LiveSource in
 * its XML definition, as shown below:
 * ```
 *   <Hints>
 *      <LiveSource interval="100" emulated_time="1" />
 *    </Hints>
 * ```
 *
 * If the pipeline process is taking more time that the timesteps gap, it will
 * skip frames.
 *
 * @sa pqLiveSourceBehavior, pqLiveSourceManager
 */
class VTKPVVTKEXTENSIONSCORE_EXPORT vtkEmulatedTimeAlgorithm : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkEmulatedTimeAlgorithm, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Check whether the provided time falls within the algorithm TimeRange,
   * if it does return true to update the source with the new time.
   * Also save the current time steps asked by the LiveSource behavior,
   * enabling it to invoke RequestData for the specified time through
   * the RequestUpdateExtent method.
   *
   * Required for LiveSource behavior.
   */
  virtual bool GetNeedsUpdate(double time);

  /**
   * LiveSources algorithm requires calling this->Modified() to update the data.
   * However, doing so triggers an unwanted call to RequestInformation.
   * To address this, we override the standard Modified method to ensure that
   * RequestInformation is only invoked when the parameters are actually modified.
   */
  void Modified() override;

  /**
   * Return the first and last time steps of the implemented algorithm.
   *
   * Required for LiveSource behavior.
   */
  vtkGetVector2Macro(TimeRange, double);

  /**
   * Process a request from the executive. For vtkEmulatedTimeAlgorithm, the
   * request will be delegated to one of the following methods: RequestData,
   * RequestInformation or RequestUpdateExtent.
   *
   * If the request ask for REQUEST_INFORMATION(), it invokes the subclass's RequestInformation
   * method. It copies then the time range and time steps to be able to interact with
   * the LiveSource behavior.
   *
   * See vtkAlgorithm for more details.
   */
  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkEmulatedTimeAlgorithm();
  ~vtkEmulatedTimeAlgorithm() override;

  /**
   * This is called within ProcessRequest.
   * It is a method you need to override, it should provide information about its output
   * without doing any lengthy computations.
   *
   * It is expected that the overriden method set time information such as TIME_STEPS() and
   * TIME_RANGE().
   */
  virtual int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) = 0;

  /**
   * This is called within ProcessRequest when a request asks the algorithm to do its work.
   * It is a method you need to override, to do whatever the algorithm is designed to do.
   * This happens during the final pass in the pipeline execution process.
   */
  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) = 0;

  /**
   * If an update is needed from the LiveSource behavior, update the current pipeline timestep.
   * Otherwise do nothing.
   */
  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

private:
  bool NeedsUpdate = false;
  bool NeedsInitialization = true;
  double RequestedTime = 0.;
  double TimeRange[2];
  std::vector<double> TimeSteps;

  vtkEmulatedTimeAlgorithm(const vtkEmulatedTimeAlgorithm&) = delete;
  void operator=(const vtkEmulatedTimeAlgorithm&) = delete;
};

#endif
