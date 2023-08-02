// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVLogInformation.h"

#include "vtkClientServerStream.h"
#include "vtkLogRecorder.h"
#include "vtkLogger.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVLogInformation);

//----------------------------------------------------------------------------
void vtkPVLogInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Rank: " << this->Rank << endl;
  os << indent << "Logs: " << this->Logs << endl;
  os << indent << "Starting Logs: " << this->StartingLogs << endl;
  os << indent << "Verbosity" << this->Verbosity << endl;
}

//----------------------------------------------------------------------------
void vtkPVLogInformation::CopyFromObject(vtkObject* obj)
{
  if (!obj)
  {
    vtkErrorMacro("Cannot get class name from NULL object.");
    return;
  }
  vtkLogRecorder* logRecorder = vtkLogRecorder::SafeDownCast(obj);
  if (logRecorder == nullptr)
  {
    vtkErrorMacro("Cannot down cast to vtkLogRecorder.");
  }
  else
  {
    if (this->Rank == logRecorder->GetMyRank())
    {
      this->Logs = logRecorder->GetLogs();
      this->StartingLogs = logRecorder->GetStartingLog();
    }
    this->Verbosity = vtkLogger::GetCurrentVerbosityCutoff();
  }
}

//----------------------------------------------------------------------------
void vtkPVLogInformation::AddInformation(vtkPVInformation* info)
{
  if (vtkPVLogInformation::SafeDownCast(info))
  {
    this->Logs += vtkPVLogInformation::SafeDownCast(info)->Logs;
    this->StartingLogs += vtkPVLogInformation::SafeDownCast(info)->StartingLogs;
    if (this->Verbosity == 20)
    {
      this->Verbosity = vtkPVLogInformation::SafeDownCast(info)->Verbosity;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVLogInformation::CopyFromStream(const vtkClientServerStream* css)
{
  css->GetArgument(0, 0, &Logs);
  css->GetArgument(0, 1, &StartingLogs);
  css->GetArgument(0, 2, &Verbosity);
}

//----------------------------------------------------------------------------
void vtkPVLogInformation::CopyParametersToStream(vtkMultiProcessStream& str)
{
  str << 557499 << this->Rank;
}

//----------------------------------------------------------------------------
void vtkPVLogInformation::CopyParametersFromStream(vtkMultiProcessStream& str)
{
  int magic_number;
  str >> magic_number >> this->Rank;
  if (magic_number != 557499)
  {
    vtkErrorMacro("Magic number mismatch.");
  }
}

//----------------------------------------------------------------------------
void vtkPVLogInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply << this->Logs << this->StartingLogs << this->Verbosity
       << vtkClientServerStream::End;
}
