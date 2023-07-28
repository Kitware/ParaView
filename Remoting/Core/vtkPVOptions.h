// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVOptions
 * @brief   ParaView options storage
 *
 * An object of this class represents a storage for ParaView options
 *
 * These options can be retrieved during run-time, set using configuration file
 * or using Command Line Arguments.
 *
 * @deprecated in ParaView 5.12.0. See `vtkCLIOptions` instead.
 * See https://gitlab.kitware.com/paraview/paraview/-/merge_requests/4951 for
 * developer guidelines.
 */

#ifndef vtkPVOptions_h
#define vtkPVOptions_h

#include "vtkCommandOptions.h"
#include "vtkParaViewDeprecation.h" // for PARAVIEW_DEPRECATED_IN_5_12_0
#include "vtkRemotingCoreModule.h"  //needed for exports

#include <string>  // used for ivar
#include <utility> // needed for pair
#include <vector>  // needed for vector

class vtkPVOptionsInternal;

class VTKREMOTINGCORE_EXPORT vtkPVOptions : public vtkCommandOptions
{
protected:
  friend class vtkPVOptionsXMLParser;

public:
  PARAVIEW_DEPRECATED_IN_5_12_0("Use `vtkCLIOptions` instead")
  static vtkPVOptions* New();
  vtkTypeMacro(vtkPVOptions, vtkCommandOptions);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Returns the verbosity level for stderr output chosen.
   * Is set to vtkLogger::VERBOSITY_INVALID if not specified.
   */
  ///@}

  enum ProcessTypeEnum
  {
    PARAVIEW = 0x2,
    PVCLIENT = 0x4,
    PVSERVER = 0x8,
    PVRENDER_SERVER = 0x10,
    PVDATA_SERVER = 0x20,
    PVBATCH = 0x40,
    ALLPROCESS = PARAVIEW | PVCLIENT | PVSERVER | PVRENDER_SERVER | PVDATA_SERVER | PVBATCH
  };

protected:
  vtkPVOptions();
  ~vtkPVOptions() override;

private:
  vtkPVOptions(const vtkPVOptions&) = delete;
  void operator=(const vtkPVOptions&) = delete;
};

#endif
