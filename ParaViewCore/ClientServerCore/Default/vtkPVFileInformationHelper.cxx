/*=========================================================================

  Program:   ParaView
  Module:    vtkPVFileInformationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVFileInformationHelper.h"

#include "vtkObjectFactory.h"

#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkPVFileInformationHelper);
//-----------------------------------------------------------------------------
vtkPVFileInformationHelper::vtkPVFileInformationHelper()
{
  this->DirectoryListing = 0;
  this->Path = 0;
  this->WorkingDirectory = 0;
  this->SpecialDirectories = 0;
  this->SetPath(".");
  this->PathSeparator = 0;
  this->FastFileTypeDetection = 1;
#if defined(_WIN32) && !defined(__CYGWIN__)
  this->SetPathSeparator("\\");
#else
  this->SetPathSeparator("/");
#endif
}

//-----------------------------------------------------------------------------
vtkPVFileInformationHelper::~vtkPVFileInformationHelper()
{
  this->SetPath(0);
  this->SetPathSeparator(0);
  this->SetWorkingDirectory(0);
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
  os << indent << "Path: " << (this->Path? this->Path : "(null)") << endl;
  os << indent << "WorkingDirectory: " <<
    (this->WorkingDirectory? this->WorkingDirectory : "(null)") << endl;
  os << indent << "DirectoryListing: " << this->DirectoryListing << endl;
  os << indent << "SpecialDirectories: " << this->SpecialDirectories << endl;
  os << indent << "PathSeparator: " 
    <<  (this->PathSeparator? this->PathSeparator : "(null)") << endl;
  os << indent << "FastFileTypeDetection: "
    << this->FastFileTypeDetection << endl;
}
