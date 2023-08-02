// SPDX-FileCopyrightText: Copyright (c) Copyright 2021 NVIDIA Corporation
// SPDX-License-Identifier: BSD-3-Clause

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
