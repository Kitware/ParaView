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
#include "vtkPVSystemInformation.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <vtksys/SystemInformation.hxx>

vtkStandardNewMacro(vtkPVSystemInformation);
//----------------------------------------------------------------------------
vtkPVSystemInformation::vtkPVSystemInformation()
{
}

//----------------------------------------------------------------------------
vtkPVSystemInformation::~vtkPVSystemInformation()
{
}

//----------------------------------------------------------------------------
void vtkPVSystemInformation::CopyFromObject(vtkObject*)
{
  this->SystemInformations.clear();

  vtksys::SystemInformation sys_info;
  sys_info.RunCPUCheck();
  sys_info.RunOSCheck();
  sys_info.RunMemoryCheck();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  SystemInformationType self;
  self.ProcessType = vtkProcessModule::GetProcessType();
  self.ProcessId = pm->GetPartitionId();
  self.NumberOfProcesses = pm->GetNumberOfLocalPartitions();
  self.Hostname = sys_info.GetHostname();
  self.OSName = sys_info.GetOSName();
  self.OSRelease = sys_info.GetOSRelease();
  self.OSVersion = sys_info.GetOSVersion();
  self.OSPlatform = sys_info.GetOSPlatform();
  self.Is64Bits = sys_info.Is64Bits();
  self.NumberOfPhyicalCPUs = sys_info.GetNumberOfPhysicalCPU();
  self.NumberOfLogicalCPUs = sys_info.GetNumberOfLogicalCPU();
  self.TotalPhysicalMemory = sys_info.GetTotalPhysicalMemory();
  self.AvailablePhysicalMemory = sys_info.GetAvailablePhysicalMemory();
  self.TotalVirtualMemory = sys_info.GetTotalVirtualMemory();
  self.AvailableVirtualMemory = sys_info.GetAvailableVirtualMemory();
  this->SystemInformations.push_back(self);
}

//----------------------------------------------------------------------------
void vtkPVSystemInformation::AddInformation(vtkPVInformation* pvother)
{
  vtkPVSystemInformation* other = vtkPVSystemInformation::SafeDownCast(pvother);
  if (other)
  {
    this->SystemInformations.insert(this->SystemInformations.end(),
      other->SystemInformations.begin(), other->SystemInformations.end());
  }
}
//----------------------------------------------------------------------------
namespace
{
class vtkPVSystemInformationSave
{
  vtkClientServerStream& Stream;

public:
  vtkPVSystemInformationSave(vtkClientServerStream& stream)
    : Stream(stream)
  {
  }
  vtkPVSystemInformationSave(const vtkPVSystemInformationSave& other)
    : Stream(other.Stream)
  {
  }
  void operator()(const vtkPVSystemInformation::SystemInformationType& data)
  {
    this->Stream << static_cast<int>(data.ProcessType) << data.ProcessId << data.NumberOfProcesses
                 << data.Hostname.c_str() << data.OSName.c_str() << data.OSRelease.c_str()
                 << data.OSVersion.c_str() << data.OSPlatform.c_str() << data.Is64Bits
                 << data.NumberOfPhyicalCPUs << data.NumberOfLogicalCPUs << data.TotalPhysicalMemory
                 << data.AvailablePhysicalMemory << data.TotalVirtualMemory
                 << data.AvailableVirtualMemory;
  }

private:
  void operator=(const vtkPVSystemInformationSave&);
};

class vtkPVSystemInformationLoad
{
  const vtkClientServerStream& Stream;
  bool LoadSuccessful;
  int& Offset;

public:
  vtkPVSystemInformationLoad(const vtkClientServerStream& stream, int& offset)
    : Stream(stream)
    , LoadSuccessful(true)
    , Offset(offset)
  {
  }
  vtkPVSystemInformationLoad(const vtkPVSystemInformationLoad& other)
    : Stream(other.Stream)
    , LoadSuccessful(other.LoadSuccessful)
    , Offset(other.Offset)
  {
  }
  operator bool() { return this->LoadSuccessful; }
  void operator()(vtkPVSystemInformation::SystemInformationType& data)
  {
    if (!this->LoadSuccessful)
    {
      return;
    }
    try
    {
      const char* temp_ptr;
      int temp_int;
      const vtkClientServerStream& stream = this->Stream;
      if (!stream.GetArgument(0, this->Offset++, &temp_int))
      {
        throw false;
      }
      data.ProcessType = static_cast<vtkProcessModule::ProcessTypes>(temp_int);
      if (!stream.GetArgument(0, this->Offset++, &data.ProcessId))
      {
        throw false;
      }
      if (!stream.GetArgument(0, this->Offset++, &data.NumberOfProcesses))
      {
        throw false;
      }
      if (!stream.GetArgument(0, this->Offset++, &temp_ptr))
      {
        throw false;
      }
      data.Hostname = temp_ptr;
      if (!stream.GetArgument(0, this->Offset++, &temp_ptr))
      {
        throw false;
      }
      data.OSName = temp_ptr;
      if (!stream.GetArgument(0, this->Offset++, &temp_ptr))
      {
        throw false;
      }
      data.OSRelease = temp_ptr;
      if (!stream.GetArgument(0, this->Offset++, &temp_ptr))
      {
        throw false;
      }
      data.OSVersion = temp_ptr;
      if (!stream.GetArgument(0, this->Offset++, &temp_ptr))
      {
        throw false;
      }
      data.OSPlatform = temp_ptr;
      if (!stream.GetArgument(0, this->Offset++, &data.Is64Bits))
      {
        throw false;
      }
      if (!stream.GetArgument(0, this->Offset++, &data.NumberOfPhyicalCPUs))
      {
        throw false;
      }
      if (!stream.GetArgument(0, this->Offset++, &data.NumberOfLogicalCPUs))
      {
        throw false;
      }
      if (!stream.GetArgument(0, this->Offset++, &data.TotalPhysicalMemory))
      {
        throw false;
      }
      if (!stream.GetArgument(0, this->Offset++, &data.AvailablePhysicalMemory))
      {
        throw false;
      }
      if (!stream.GetArgument(0, this->Offset++, &data.TotalVirtualMemory))
      {
        throw false;
      }
      if (!stream.GetArgument(0, this->Offset++, &data.AvailableVirtualMemory))
      {
        throw false;
      }
    }
    catch (bool)
    {
      this->LoadSuccessful = false;
    }
  }

private:
  void operator=(const vtkPVSystemInformationLoad&);
};
}

//----------------------------------------------------------------------------
void vtkPVSystemInformation::CopyToStream(vtkClientServerStream* stream)
{
  stream->Reset();
  *stream << vtkClientServerStream::Reply
          << static_cast<unsigned int>(this->SystemInformations.size());
  std::for_each(this->SystemInformations.begin(), this->SystemInformations.end(),
    vtkPVSystemInformationSave(*stream));
  *stream << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVSystemInformation::CopyFromStream(const vtkClientServerStream* stream)
{
  int offset = 0;
  unsigned int count;
  if (!stream->GetArgument(0, offset++, &count))
  {
    vtkErrorMacro("Error parsing count.");
    return;
  }
  this->SystemInformations.clear();
  this->SystemInformations.resize(count);
  if (std::for_each(this->SystemInformations.begin(), this->SystemInformations.end(),
        vtkPVSystemInformationLoad(*stream, offset)) == false)
  {
    vtkErrorMacro("Failed to parse stream correctly.");
  }
}

//----------------------------------------------------------------------------
void vtkPVSystemInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
