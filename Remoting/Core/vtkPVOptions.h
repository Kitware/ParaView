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
#include "vtkParaViewDeprecation.h" // for PARAVIEW_DEPRECATED_IN_5_10_0
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
  static vtkPVOptions* New();
  vtkTypeMacro(vtkPVOptions, vtkCommandOptions);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  const char* GetHostName();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetConnectID();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  void SetConnectID(int);
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetTellVersion();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetSymmetricMPIMode();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetEnableStackTrace();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetTimeout();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  const char* GetLogFileName();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetReverseConnection();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  const char* GetServerURL();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  const char* GetServersFileName();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetUseStereoRendering();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  const char* GetStereoType();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  const int* GetTileDimensions();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  void GetTileDimensions(int[2]);
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  const int* GetTileMullions();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  void GetTileMullions(int[2]);
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  bool GetIsInTileDisplay() const;
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetForceOnscreenRendering();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetForceOffscreenRendering();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  bool GetIsInCave() const;
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetDisableXDisplayTests();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetEGLDeviceIndex();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetMultiClientMode();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int IsMultiClientModeDebug();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetDisableFurtherConnections();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetMultiServerMode();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  const char* GetParaViewDataName();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  const char* GetTestPlugins();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  const char* GetTestPluginPaths();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetCatalystLivePort();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetDisableRegistry();
  PARAVIEW_DEPRECATED_IN_5_10_0("Use `vtkCLIOptions` instead")
  int GetPrintMonitors();
  ///@}

  //@{
  /**
   * Returns the verbosity level for stderr output chosen.
   * Is set to vtkLogger::VERBOSITY_INVALID if not specified.
   */
  //@}

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
