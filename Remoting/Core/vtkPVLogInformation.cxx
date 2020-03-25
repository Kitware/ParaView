/*=========================================================================

   Program: ParaView
   Module:  vtkPVLogInformation.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/

#include "vtkPVLogInformation.h"

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
