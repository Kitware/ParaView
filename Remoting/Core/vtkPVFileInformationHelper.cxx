// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVFileInformationHelper.h"

#include "vtkObjectFactory.h"

#if defined(_WIN32)
#include <wchar.h>
#include <windows.h>
#endif

#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkPVFileInformationHelper);
//-----------------------------------------------------------------------------
vtkPVFileInformationHelper::vtkPVFileInformationHelper()
  : Path(nullptr)
  , WorkingDirectory(nullptr)
  , DirectoryListing(0)
  , SpecialDirectories(0)
  , ExamplesInSpecialDirectories(true)
  , FastFileTypeDetection(1)
  , GroupFileSequences(true)
  , ReadDetailedFileInformation(false)
  , PathSeparator(nullptr)
{
  this->SetPath(".");
#if defined(_WIN32) && !defined(__CYGWIN__)
  this->SetPathSeparator("\\");
#else
  this->SetPathSeparator("/");
#endif
}

//-----------------------------------------------------------------------------
vtkPVFileInformationHelper::~vtkPVFileInformationHelper()
{
  this->SetPath(nullptr);
  this->SetPathSeparator(nullptr);
  this->SetWorkingDirectory(nullptr);
}

//-----------------------------------------------------------------------------
bool vtkPVFileInformationHelper::GetActiveFileIsReadable()
{
  return vtksys::SystemTools::FileExists(this->Path);
}

//-----------------------------------------------------------------------------
bool vtkPVFileInformationHelper::GetActiveFileIsDirectory()
{
  return vtksys::SystemTools::FileIsDirectory(this->Path);
}

//-----------------------------------------------------------------------------
void vtkPVFileInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Path: " << (this->Path ? this->Path : "(null)") << endl;
  os << indent
     << "WorkingDirectory: " << (this->WorkingDirectory ? this->WorkingDirectory : "(null)")
     << endl;
  os << indent << "DirectoryListing: " << this->DirectoryListing << endl;
  os << indent << "SpecialDirectories: " << this->SpecialDirectories << endl;
  os << indent << "ExamplesInSpecialDirectories: " << this->ExamplesInSpecialDirectories << endl;
  os << indent << "PathSeparator: " << (this->PathSeparator ? this->PathSeparator : "(null)")
     << endl;
  os << indent << "FastFileTypeDetection: " << this->FastFileTypeDetection << endl;
}
