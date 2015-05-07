/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEnsembleDataReaderInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVEnsembleDataReaderInformation.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

#include "vtkEnsembleDataReader.h"

#include <cassert>

struct vtkPVEnsembleDataReaderInformationInternal
{
  std::vector<vtkStdString> FilePaths;
};

vtkStandardNewMacro(vtkPVEnsembleDataReaderInformation);

//-----------------------------------------------------------------------------
vtkPVEnsembleDataReaderInformation::vtkPVEnsembleDataReaderInformation()
{
  this->SetRootOnly(1);

  this->Internal = new vtkPVEnsembleDataReaderInformationInternal;
}

//-----------------------------------------------------------------------------
vtkPVEnsembleDataReaderInformation::~vtkPVEnsembleDataReaderInformation()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkPVEnsembleDataReaderInformation::CopyFromObject(vtkObject* obj)
{
  vtkEnsembleDataReader *reader = vtkEnsembleDataReader::SafeDownCast(obj);
  if (!reader)
    {
    vtkErrorMacro(
        "vtkPVEnsembleDataReaderInformation requires a valid "
        "vtkEnsembleDataReader instance.");
    return;
    }

  reader->UpdateInformation();
  this->Internal->FilePaths.clear();
  for (int rowIndex = 0; rowIndex < reader->GetNumberOfMembers(); ++rowIndex)
    {
      this->Internal->FilePaths.push_back(reader->GetFilePath(rowIndex));
    }
}

//-----------------------------------------------------------------------------
void vtkPVEnsembleDataReaderInformation::CopyToStream(vtkClientServerStream* stream)
{
  stream->Reset();
  *stream << vtkClientServerStream::Reply;
  *stream << this->Internal->FilePaths.size();
  for (size_t i = 0; i < this->Internal->FilePaths.size(); ++i)
    {
    *stream << this->Internal->FilePaths[i];
    }
  *stream << vtkClientServerStream::End;
}

//-----------------------------------------------------------------------------
void vtkPVEnsembleDataReaderInformation::CopyFromStream(const vtkClientServerStream* stream)
{
  int filePathCount = 0;
  if (!stream->GetArgument(0, 0, &filePathCount))
    {
    vtkErrorMacro("Error parsing file path count.");
    }

  this->Internal->FilePaths.clear();
  this->Internal->FilePaths.reserve(filePathCount);
  vtkStdString filePath;
  for (int i = 0; i < filePathCount; ++i)
    {
    if (!stream->GetArgument(0, i, &filePath))
      {
      vtkErrorMacro("Error parsing file path.");
      }
    this->Internal->FilePaths.push_back(filePath);
    }
}

//-----------------------------------------------------------------------------
int vtkPVEnsembleDataReaderInformation::GetFileCount()
{
  return static_cast<int>(this->Internal->FilePaths.size());
}

//-----------------------------------------------------------------------------
vtkStdString vtkPVEnsembleDataReaderInformation::GetFilePath(const int i)
{
  int count = static_cast<int>(this->Internal->FilePaths.size());
  if (i < 0 || i >= count)
    {
    vtkErrorMacro("Bad index sent to GetFilePath.");
    }
  return this->Internal->FilePaths[i];
}

//-----------------------------------------------------------------------------
void vtkPVEnsembleDataReaderInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  // File paths
  os << indent << "File Paths: " << endl;
  for (size_t i = 0; i < this->Internal->FilePaths.size(); ++i)
    {
      os << this->Internal->FilePaths[i] << endl;
    }
}
