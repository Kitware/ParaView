/*=========================================================================

  Program:   ParaView
  Module:    vtkPVOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVOptions
 * @brief   ParaView options storage
 *
 * An object of this class represents a storage for ParaView options
 *
 * These options can be retrieved during run-time, set using configuration file
 * or using Command Line Arguments.
 */

#ifndef vtkPVOptions_h
#define vtkPVOptions_h

#include "vtkCommandOptions.h"
#include "vtkRemotingCoreModule.h" //needed for exports

#include <string>  // used for ivar
#include <utility> // needed for pair
#include <vector>  // needed for vector

class vtkPVOptionsInternal;

class VTKREMOTINGCORE_EXPORT vtkPVOptions : public vtkCommandOptions
{
protected:
  friend class vtkPVOptionsXMLParser;

public:
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
