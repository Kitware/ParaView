/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSystemConfigInformation.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

#include <sstream>
using std::ostringstream;
#include <algorithm>
using std::sort;

#include <vtksys/SystemInformation.hxx>

// #define vtkPVSystemConfigInformationDEBUG

#define vtkVerifyParseMacro(_call, _field)                                                         \
  if (!(_call))                                                                                    \
  {                                                                                                \
    vtkErrorMacro("Error parsing " _field ".");                                                    \
    return;                                                                                        \
  }

void vtkPVSystemConfigInformation::ConfigInfo::Print()
{
  cerr << "OSDescriptor=" << this->OSDescriptor << endl
       << "CPUDescriptor=" << this->CPUDescriptor << endl
       << "MemDescriptor=" << this->MemDescriptor << endl
       << "HostName=" << this->HostName << endl
       << "ProcessType=" << this->ProcessType << endl
       << "SystemType=" << this->SystemType << endl
       << "Rank=" << this->Rank << endl
       << "Pid=" << this->Pid << endl
       << "HostMemoryTotal=" << this->HostMemoryTotal << endl
       << "HostMemoryAvailable=" << this->HostMemoryAvailable << endl
       << "ProcMemoryAvailable=" << this->ProcMemoryAvailable << endl
       << "ProcMemoryAvailable=" << this->ProcMemoryAvailable << endl;
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSystemConfigInformation);

//----------------------------------------------------------------------------
vtkPVSystemConfigInformation::vtkPVSystemConfigInformation()
{
#ifdef vtkPVSystemConfigInformationDEBUG
  cerr << "=====vtkPVSystemConfigInformation::vtkPVSystemConfigInformation" << endl;
#endif
}

//----------------------------------------------------------------------------
vtkPVSystemConfigInformation::~vtkPVSystemConfigInformation()
{
#ifdef vtkPVSystemConfigInformationDEBUG
  cerr << "=====vtkPVSystemConfigInformation::~vtkPVSystemConfigInformation" << endl;
#endif
}

//----------------------------------------------------------------------------
void vtkPVSystemConfigInformation::CopyFromObject(vtkObject* obj)
{
#ifdef vtkPVSystemConfigInformationDEBUG
  cerr << "=====vtkPVSystemConfigInformation::CopyFromObject" << endl;
#endif

  (void)obj;

  this->Configs.clear();

  ConfigInfo info;

  vtksys::SystemInformation sysInfo;
  sysInfo.RunCPUCheck();
  sysInfo.RunOSCheck();
  sysInfo.RunMemoryCheck();

  info.OSDescriptor = sysInfo.GetOSDescription();
  info.CPUDescriptor = sysInfo.GetCPUDescription();
  info.MemDescriptor = sysInfo.GetMemoryDescription();
  info.HostName = sysInfo.GetHostname();
  info.ProcessType = vtkProcessModule::GetProcessType();
  info.SystemType = sysInfo.GetOSIsWindows();
  info.Rank = vtkProcessModule::GetProcessModule()->GetPartitionId();
  info.Pid = sysInfo.GetProcessId();
  info.HostMemoryTotal = sysInfo.GetHostMemoryTotal();
  // the folloowing envornment variables are querried, if set they
  // are the means of reporting limits that are applied in a non-
  // standard way.
  info.HostMemoryAvailable = sysInfo.GetHostMemoryAvailable("PV_HOST_MEMORY_LIMIT");
  info.ProcMemoryAvailable =
    sysInfo.GetProcMemoryAvailable("PV_HOST_MEMORY_LIMIT", "PV_PROC_MEMORY_LIMIT");

#ifdef vtkPVSystemConfigInformationDEBUG
  info.Print();
#endif

  this->Configs.push_back(info);
}

//----------------------------------------------------------------------------
void vtkPVSystemConfigInformation::AddInformation(vtkPVInformation* pvinfo)
{
#ifdef vtkPVSystemConfigInformationDEBUG
  cerr << "=====vtkPVSystemConfigInformation::AddInformation" << endl;
#endif
  vtkPVSystemConfigInformation* info = dynamic_cast<vtkPVSystemConfigInformation*>(pvinfo);

  if (!info)
  {
    return;
  }

  this->Configs.insert(this->Configs.end(), info->Configs.begin(), info->Configs.end());
}

//----------------------------------------------------------------------------
void vtkPVSystemConfigInformation::CopyToStream(vtkClientServerStream* css)
{
#ifdef vtkPVSystemConfigInformationDEBUG
  cerr << "=====vtkPVSystemConfigInformation::CopyToStream" << endl;
#endif

  css->Reset();

  size_t numberOfConfigs = this->Configs.size();

  *css << vtkClientServerStream::Reply << numberOfConfigs;

  for (size_t i = 0; i < numberOfConfigs; ++i)
  {
    *css << this->Configs[i].OSDescriptor.c_str() << this->Configs[i].CPUDescriptor.c_str()
         << this->Configs[i].MemDescriptor.c_str() << this->Configs[i].HostName.c_str()
         << this->Configs[i].ProcessType << this->Configs[i].SystemType << this->Configs[i].Rank
         << this->Configs[i].Pid << this->Configs[i].HostMemoryTotal
         << this->Configs[i].HostMemoryAvailable << this->Configs[i].ProcMemoryAvailable;
  }

  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVSystemConfigInformation::CopyFromStream(const vtkClientServerStream* css)
{
#ifdef vtkPVSystemConfigInformationDEBUG
  cerr << "=====vtkPVSystemConfigInformation::CopyFromStream" << endl;
#endif

  int offset = 0;
  size_t numberOfConfigs = 0;

  vtkVerifyParseMacro(css->GetArgument(0, offset, &numberOfConfigs), "numberOfConfigs");
  ++offset;

  this->Configs.resize(numberOfConfigs);

  for (size_t i = 0; i < numberOfConfigs; ++i)
  {
    char* osDescr;
    vtkVerifyParseMacro(css->GetArgument(0, offset, &osDescr), "OSDescriptor");
    ++offset;
    this->Configs[i].OSDescriptor = osDescr;

    char* cpuDescr;
    vtkVerifyParseMacro(css->GetArgument(0, offset, &cpuDescr), "CPUDescriptor");
    ++offset;
    this->Configs[i].CPUDescriptor = cpuDescr;

    char* memDescr;
    vtkVerifyParseMacro(css->GetArgument(0, offset, &memDescr), "MemDescriptor");
    ++offset;
    this->Configs[i].MemDescriptor = memDescr;

    char* hostName;
    vtkVerifyParseMacro(css->GetArgument(0, offset, &hostName), "HostName");
    ++offset;
    this->Configs[i].HostName = hostName;

    vtkVerifyParseMacro(css->GetArgument(0, offset, &this->Configs[i].ProcessType), "ProcessType");
    ++offset;

    vtkVerifyParseMacro(css->GetArgument(0, offset, &this->Configs[i].SystemType), "SystemType");
    ++offset;

    vtkVerifyParseMacro(css->GetArgument(0, offset, &this->Configs[i].Rank), "Rank");
    ++offset;

    vtkVerifyParseMacro(css->GetArgument(0, offset, &this->Configs[i].Pid), "Pid");
    ++offset;

    vtkVerifyParseMacro(
      css->GetArgument(0, offset, &this->Configs[i].HostMemoryTotal), "HostMemoryTotal");
    ++offset;

    vtkVerifyParseMacro(
      css->GetArgument(0, offset, &this->Configs[i].HostMemoryAvailable), "HostMemoryAvailable");
    ++offset;

    vtkVerifyParseMacro(
      css->GetArgument(0, offset, &this->Configs[i].ProcMemoryAvailable), "ProcMemoryAvailable");
    ++offset;
  }
}

//----------------------------------------------------------------------------
void vtkPVSystemConfigInformation::Sort()
{
#ifdef vtkPVSystemConfigInformationDEBUG
  cerr << "=====vtkPVSystemConfigInformation::Sort" << endl;
#endif

  sort(this->Configs.begin(), this->Configs.end());
}

//----------------------------------------------------------------------------
void vtkPVSystemConfigInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
