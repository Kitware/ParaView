// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkDSPTableFFT
 *
 * This filter acts as a replacement for the vtkTableFFT when dealing with
 * data stemming from the vtkTemporalMultiplexing filter.
 */

#ifndef vtkDSPTableFFT_h
#define vtkDSPTableFFT_h

#include "vtkDSPFiltersPluginModule.h" // for export macro
#include "vtkTableFFT.h"

#include <memory> // for std::unique_ptr

class vtkInformation;
class vtkInformationVector;
class VTKDSPFILTERSPLUGIN_EXPORT vtkDSPTableFFT : public vtkTableFFT
{

public:
  static vtkDSPTableFFT* New();
  vtkTypeMacro(vtkDSPTableFFT, vtkTableFFT);

protected:
  vtkDSPTableFFT() = default;
  ~vtkDSPTableFFT() override = default;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkDSPTableFFT(const vtkDSPTableFFT&) = delete;
  void operator=(const vtkDSPTableFFT&) = delete;
};

#endif // vtkDSPTableFFT_h
