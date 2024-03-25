// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkEmulatedTimeDummySource_h
#define vtkEmulatedTimeDummySource_h

#include "vtkEmulatedTimeAlgorithm.h"

#include "EmulatedTimeDummySourcesModule.h"

/**
 * @class vtkEmulatedTimeDummySource
 *
 * Dummy source to demonstrate how to use vtkEmulatedTimeAlgorithm.
 * This dummy source create either an animated sphere, cone or cube
 * for different pre-configured timesteps.
 */
class EMULATEDTIMEDUMMYSOURCES_EXPORT vtkEmulatedTimeDummySource : public vtkEmulatedTimeAlgorithm
{
public:
  vtkTypeMacro(vtkEmulatedTimeDummySource, vtkEmulatedTimeAlgorithm);
  static vtkEmulatedTimeDummySource* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum SourcePreset
  {
    SPHERE = 0,
    CONE,
    CUBE
  };

  ///@{
  /**
   * Set/Get a preset configuration with various time steps and sources.
   * The `SourcePreset` enum list all available presets.
   */
  vtkSetMacro(SourcePresets, int);
  vtkGetMacro(SourcePresets, int);
  ///@}

protected:
  vtkEmulatedTimeDummySource();
  ~vtkEmulatedTimeDummySource() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

private:
  int SourcePresets = SourcePreset::SPHERE;

  vtkEmulatedTimeDummySource(const vtkEmulatedTimeDummySource&) = delete;
  void operator=(const vtkEmulatedTimeDummySource&) = delete;
};

#endif
