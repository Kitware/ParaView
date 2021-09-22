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
#include "vtkLegacy.h"             // for legacy macros.
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
  VTK_LEGACY(const char* GetHostName());
  VTK_LEGACY(int GetConnectID());
  VTK_LEGACY(void SetConnectID(int));
  VTK_LEGACY(int GetTellVersion());
  VTK_LEGACY(int GetSymmetricMPIMode());
  VTK_LEGACY(int GetEnableStackTrace());
  VTK_LEGACY(int GetTimeout());
  VTK_LEGACY(const char* GetLogFileName());
  VTK_LEGACY(int GetReverseConnection());
  VTK_LEGACY(const char* GetServerURL());
  VTK_LEGACY(const char* GetServersFileName());
  VTK_LEGACY(int GetUseStereoRendering());
  VTK_LEGACY(const char* GetStereoType());
  VTK_LEGACY(const int* GetTileDimensions());
  VTK_LEGACY(void GetTileDimensions(int[2]));
  VTK_LEGACY(const int* GetTileMullions());
  VTK_LEGACY(void GetTileMullions(int[2]));
  VTK_LEGACY(bool GetIsInTileDisplay() const);
  VTK_LEGACY(int GetForceOnscreenRendering());
  VTK_LEGACY(int GetForceOffscreenRendering());
  VTK_LEGACY(bool GetIsInCave() const);
  VTK_LEGACY(int GetDisableXDisplayTests());
  VTK_LEGACY(int GetEGLDeviceIndex());
  VTK_LEGACY(int GetMultiClientMode());
  VTK_LEGACY(int IsMultiClientModeDebug());
  VTK_LEGACY(int GetDisableFurtherConnections());
  VTK_LEGACY(int GetMultiServerMode());
  VTK_LEGACY(const char* GetParaViewDataName());
  VTK_LEGACY(const char* GetTestPlugins());
  VTK_LEGACY(const char* GetTestPluginPaths());
  VTK_LEGACY(int GetCatalystLivePort());
  VTK_LEGACY(int GetDisableRegistry());
  VTK_LEGACY(int GetPrintMonitors());
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
