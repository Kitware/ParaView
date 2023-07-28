// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVFilePathEncodingHelper.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVFileInformationHelper.h"
#include "vtkPVSessionBase.h"
#include "vtkPVSessionCore.h"
#include "vtkProcessModule.h"
#include "vtkSIProxy.h"

vtkStandardNewMacro(vtkPVFilePathEncodingHelper);
//-----------------------------------------------------------------------------
vtkPVFilePathEncodingHelper::vtkPVFilePathEncodingHelper()
{
  this->Path = nullptr;
  this->SecondaryPath = nullptr;
  this->ActiveGlobalId = 0;
}

//-----------------------------------------------------------------------------
vtkPVFilePathEncodingHelper::~vtkPVFilePathEncodingHelper()
{
  this->SetPath(nullptr);
  this->SetSecondaryPath(nullptr);
  this->SetActiveGlobalId(0);
}

//-----------------------------------------------------------------------------
bool vtkPVFilePathEncodingHelper::MakeDirectory()
{
  return this->CallObjectMethod("MakeDirectory");
}

//-----------------------------------------------------------------------------
bool vtkPVFilePathEncodingHelper::DeleteDirectory()
{
  return this->CallObjectMethod("DeleteDirectory");
}

//-----------------------------------------------------------------------------
bool vtkPVFilePathEncodingHelper::OpenDirectory()
{
  return this->CallObjectMethod("Open");
}

//-----------------------------------------------------------------------------
bool vtkPVFilePathEncodingHelper::RenameDirectory()
{
  return this->CallObjectMethod("Rename");
}

//-----------------------------------------------------------------------------
bool vtkPVFilePathEncodingHelper::GetActiveFileIsReadable()
{
  return this->CallObjectMethod("CanReadFile", true);
}

//-----------------------------------------------------------------------------
bool vtkPVFilePathEncodingHelper::IsDirectory()
{
  // note: called on vtkDirectory instance.
  return this->CallObjectMethod("FileIsDirectory", false);
}

//-----------------------------------------------------------------------------
bool vtkPVFilePathEncodingHelper::CallObjectMethod(const char* method, bool ignoreErrors)
{
  vtkPVSessionBase* session =
    vtkPVSessionBase::SafeDownCast(vtkProcessModule::GetProcessModule()->GetActiveSession());

  auto siProxy =
    vtkSIProxy::SafeDownCast(session->GetSessionCore()->GetSIObject(this->ActiveGlobalId));
  auto object = vtkObject::SafeDownCast(siProxy->GetVTKObject());
  auto interpreter = siProxy->GetInterpreter();

  // Build stream request
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << object << method;
  stream << this->Path;
  if (this->SecondaryPath != nullptr)
  {
    stream << this->SecondaryPath;
  }
  stream << vtkClientServerStream::End;

  // Process stream and get result
  int temp = interpreter->GetGlobalWarningDisplay();
  interpreter->SetGlobalWarningDisplay(ignoreErrors ? 0 : 1);
  interpreter->ProcessStream(stream);
  interpreter->SetGlobalWarningDisplay(temp);

  int ret = 1;
  interpreter->GetLastResult().GetArgument(0, 0, &ret);
  return ret != 0;
}

//-----------------------------------------------------------------------------
void vtkPVFilePathEncodingHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Path: " << (this->Path ? this->Path : "(null)") << endl;
  os << indent << "SecondaryPath: " << (this->SecondaryPath ? this->SecondaryPath : "(null)")
     << endl;
  os << indent << "ActiveGlobalId: " << this->ActiveGlobalId << endl;
}
