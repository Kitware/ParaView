/* Copyright 2021 NVIDIA Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef vtknvindex_global_settings_h
#define vtknvindex_global_settings_h

#include "vtkIndeXRepresentationsModule.h"

#include <vector>

#include <vtkNew.h>
#include <vtkObject.h>
#include <vtkSMProxyInitializationHelper.h>
#include <vtkSmartPointer.h>

class vtkCallbackCommand;

class VTKINDEXREPRESENTATIONS_EXPORT vtknvindex_global_settings : public vtkObject
{
public:
  static vtknvindex_global_settings* New();
  vtkTypeMacro(vtknvindex_global_settings, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtknvindex_global_settings* GetInstance();
  static void InitializeSettings(vtkObject*, unsigned long, void* clientdata, void*);

  // Logging

  // These match mi::base::MESSAGE_SEVERITY_WARNING, etc.
  enum LogLevels
  {
    LOG_DISABLE = 0,
    LOG_ERROR = 1,
    LOG_WARNING = 2,
    LOG_INFO = 3,
    LOG_VERBOSE = 4
  };

  vtkGetMacro(LogLevel, int);
  vtkSetMacro(LogLevel, int);
  vtkGetMacro(LogLevelStandardOutput, int);
  vtkSetMacro(LogLevelStandardOutput, int);
  vtkGetMacro(LogTimestamp, bool);
  vtkSetMacro(LogTimestamp, bool);
  vtkGetMacro(LogHostname, bool);
  vtkSetMacro(LogHostname, bool);

  // Network Configuration

  enum ClusterModes
  {
    CLUSTER_OFF = 0,
    CLUSTER_TCP = 1,
    CLUSTER_UDP = 2,
    CLUSTER_TCP_WITH_DISCOVERY = 3
  };

  vtkGetMacro(ClusterMode, int);
  vtkSetMacro(ClusterMode, int);

  vtkGetStringMacro(ClusterInterfaceAddress);
  vtkSetStringMacro(ClusterInterfaceAddress);

  vtkGetStringMacro(MulticastAddress);
  vtkSetStringMacro(MulticastAddress);

  vtkGetStringMacro(DiscoveryAddress);
  vtkSetStringMacro(DiscoveryAddress);

  vtkGetMacro(UseRDMA, bool);
  vtkSetMacro(UseRDMA, bool);

  vtkGetStringMacro(RDMAInterface);
  vtkSetStringMacro(RDMAInterface);
  vtkGetStringMacro(RDMAInterfaceByName);
  vtkSetStringMacro(RDMAInterfaceByName);

  // Performance Tracking

  vtkGetMacro(OutputPerformanceValues, bool);
  vtkSetMacro(OutputPerformanceValues, bool);

  vtkGetStringMacro(OutputPerformanceValuesFilename);
  vtkSetStringMacro(OutputPerformanceValuesFilename);

  vtkGetMacro(ExportSession, bool);
  vtkSetMacro(ExportSession, bool);

  vtkGetStringMacro(ExportSessionFilename);
  vtkSetStringMacro(ExportSessionFilename);

  // Extra Configuration

  virtual void SetNumberOfExtraConfigurations(int n);
  virtual int GetNumberOfExtraConfigurations();

  virtual void SetExtraConfiguration(int i, const char* expression);
  virtual const char* GetExtraConfiguration(int i);

  // License

  vtkGetStringMacro(VendorKey);
  vtkSetStringMacro(VendorKey);

  vtkGetStringMacro(SecretKey);
  vtkSetStringMacro(SecretKey);

protected:
  vtknvindex_global_settings();
  ~vtknvindex_global_settings();

  int LogLevel = LOG_WARNING;
  int LogLevelStandardOutput = LOG_INFO;
  bool LogTimestamp = false;
  bool LogHostname = false;

  int ClusterMode = CLUSTER_TCP;
  char* ClusterInterfaceAddress = nullptr;
  char* MulticastAddress = nullptr;
  char* DiscoveryAddress = nullptr;
  bool UseRDMA = false;
  char* RDMAInterface = nullptr;
  char* RDMAInterfaceByName = nullptr;

  bool OutputPerformanceValues = false;
  char* OutputPerformanceValuesFilename = nullptr;

  bool ExportSession = false;
  char* ExportSessionFilename = nullptr;

  std::vector<std::string> ExtraConfigurations;

  char* VendorKey = nullptr;
  char* SecretKey = nullptr;

private:
  vtknvindex_global_settings(const vtknvindex_global_settings&) = delete;
  void operator=(const vtknvindex_global_settings&) = delete;

  vtkNew<vtkCallbackCommand> SettingsObserver;
  unsigned long ObserverTag;

  static vtkSmartPointer<vtknvindex_global_settings> Instance;
};

#endif
