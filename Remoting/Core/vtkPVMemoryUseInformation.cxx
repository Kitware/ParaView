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
#include "vtkPVMemoryUseInformation.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

#include <vtksys/SystemInformation.hxx>

//#define vtkPVMemoryUseInformationDEBUG

#define vtkVerifyParseMacro(_call, _field)                                                         \
  if (!(_call))                                                                                    \
  {                                                                                                \
    vtkErrorMacro("Error parsing " _field ".");                                                    \
    return;                                                                                        \
  }

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMemoryUseInformation);

//----------------------------------------------------------------------------
vtkPVMemoryUseInformation::vtkPVMemoryUseInformation() = default;

//----------------------------------------------------------------------------
vtkPVMemoryUseInformation::~vtkPVMemoryUseInformation() = default;

//----------------------------------------------------------------------------
void vtkPVMemoryUseInformation::CopyFromObject(vtkObject* obj)
{
  (void)obj;

  this->MemInfos.clear();

  MemInfo info;

  vtksys::SystemInformation sysInfo;

  info.ProcessType = vtkProcessModule::GetProcessType();
  info.Rank = vtkProcessModule::GetProcessModule()->GetPartitionId();
  info.ProcMemUse = sysInfo.GetProcMemoryUsed();
  info.HostMemUse = sysInfo.GetHostMemoryUsed();

#ifdef vtkPVMemoryUseInformationDEBUG
  info.Print();
#endif

  this->MemInfos.push_back(info);
}

//----------------------------------------------------------------------------
void vtkPVMemoryUseInformation::AddInformation(vtkPVInformation* pvinfo)
{
  vtkPVMemoryUseInformation* info = dynamic_cast<vtkPVMemoryUseInformation*>(pvinfo);

  if (!info)
  {
    return;
  }

  this->MemInfos.insert(this->MemInfos.end(), info->MemInfos.begin(), info->MemInfos.end());
}

//----------------------------------------------------------------------------
void vtkPVMemoryUseInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();

  size_t count = this->MemInfos.size();

  *css << vtkClientServerStream::Reply << count;

  for (size_t i = 0; i < count; ++i)
  {
    *css << this->MemInfos[i].ProcessType << this->MemInfos[i].Rank << this->MemInfos[i].ProcMemUse
         << this->MemInfos[i].HostMemUse;
  }

  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVMemoryUseInformation::CopyFromStream(const vtkClientServerStream* css)
{
  int offset = 0;
  size_t count = 0;

  vtkVerifyParseMacro(css->GetArgument(0, offset, &count), "count");
  ++offset;

  this->MemInfos.resize(count);

  for (size_t i = 0; i < count; ++i)
  {
    vtkVerifyParseMacro(css->GetArgument(0, offset, &MemInfos[i].ProcessType), "ProcessType");
    ++offset;

    vtkVerifyParseMacro(css->GetArgument(0, offset, &MemInfos[i].Rank), "Rank");
    ++offset;

    vtkVerifyParseMacro(css->GetArgument(0, offset, &MemInfos[i].ProcMemUse), "ProcMemUse");
    ++offset;

    vtkVerifyParseMacro(css->GetArgument(0, offset, &MemInfos[i].HostMemUse), "HostMemUse");
    ++offset;
  }
}

//----------------------------------------------------------------------------
void vtkPVMemoryUseInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVMemoryUseInformation::MemInfo::Print()
{
  cerr << "ProcessType=" << this->ProcessType << endl
       << "Rank=" << this->Rank << endl
       << "ProcMemUse=" << this->ProcMemUse << endl
       << "HostMemUse=" << this->HostMemUse << endl;
}
