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
#include "vtkEnsembleDataReader.h"
#include "vtkObjectFactory.h"

#include <cassert>

class vtkPVEnsembleDataReaderInformation::vtkInternal
{
public:
  std::vector<std::string> FilePaths;
};

vtkStandardNewMacro(vtkPVEnsembleDataReaderInformation);
//-----------------------------------------------------------------------------
vtkPVEnsembleDataReaderInformation::vtkPVEnsembleDataReaderInformation()
  : Internal(new vtkPVEnsembleDataReaderInformation::vtkInternal())
{
  this->SetRootOnly(1);
}

//-----------------------------------------------------------------------------
vtkPVEnsembleDataReaderInformation::~vtkPVEnsembleDataReaderInformation()
{
  delete this->Internal;
  this->Internal = nullptr;
}

//-----------------------------------------------------------------------------
void vtkPVEnsembleDataReaderInformation::CopyFromObject(vtkObject* obj)
{
  vtkEnsembleDataReader* reader = vtkEnsembleDataReader::SafeDownCast(obj);
  if (!reader)
  {
    vtkErrorMacro("vtkPVEnsembleDataReaderInformation requires a valid "
                  "vtkEnsembleDataReader instance.");
    return;
  }
  reader->UpdateMetaData();
  unsigned int count = reader->GetNumberOfMembers();
  this->Internal->FilePaths.resize(count);
  for (unsigned int rowIndex = 0; rowIndex < count; ++rowIndex)
  {
    this->Internal->FilePaths[rowIndex] = reader->GetFilePath(rowIndex);
  }
}

//-----------------------------------------------------------------------------
void vtkPVEnsembleDataReaderInformation::CopyToStream(vtkClientServerStream* stream)
{
  *stream << vtkClientServerStream::Reply;
  *stream << static_cast<unsigned int>(this->Internal->FilePaths.size());
  for (size_t i = 0; i < this->Internal->FilePaths.size(); ++i)
  {
    *stream << this->Internal->FilePaths[i];
  }
  *stream << vtkClientServerStream::End;
}

//-----------------------------------------------------------------------------
void vtkPVEnsembleDataReaderInformation::CopyFromStream(const vtkClientServerStream* stream)
{
  this->Internal->FilePaths.clear();

  int offset = 0;
  unsigned int filePathCount = 0;
  if (!stream->GetArgument(0, offset++, &filePathCount))
  {
    vtkErrorMacro("Error parsing file path count.");
    return;
  }

  this->Internal->FilePaths.resize(filePathCount);
  std::string filePath;
  for (unsigned int i = 0; i < filePathCount; ++i)
  {
    if (!stream->GetArgument(0, offset++, &filePath))
    {
      vtkErrorMacro("Error parsing file path.");
    }
    this->Internal->FilePaths[i] = filePath;
  }
}

//-----------------------------------------------------------------------------
unsigned int vtkPVEnsembleDataReaderInformation::GetFileCount()
{
  return static_cast<unsigned int>(this->Internal->FilePaths.size());
}

//-----------------------------------------------------------------------------
std::string vtkPVEnsembleDataReaderInformation::GetFilePath(int unsigned i)
{
  unsigned int count = static_cast<unsigned int>(this->Internal->FilePaths.size());
  if (i >= count)
  {
    vtkErrorMacro("Bad index sent to GetFilePath.");
    return std::string();
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
    os << indent.GetNextIndent() << this->Internal->FilePaths[i].c_str() << endl;
  }
}
