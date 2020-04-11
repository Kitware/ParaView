// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExodusFileSeriesReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkExodusFileSeriesReader.h"

#include "vtkDirectory.h"
#include "vtkObjectFactory.h"
#include "vtkPExodusIIReader.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <vector>
#include <vtksys/RegularExpression.hxx>

static const int ExodusArrayTypeIndices[] = { vtkExodusIIReader::GLOBAL, vtkExodusIIReader::NODAL,
  vtkExodusIIReader::EDGE_BLOCK, vtkExodusIIReader::FACE_BLOCK, vtkExodusIIReader::ELEM_BLOCK,
  vtkExodusIIReader::NODE_SET, vtkExodusIIReader::SIDE_SET, vtkExodusIIReader::EDGE_SET,
  vtkExodusIIReader::FACE_SET, vtkExodusIIReader::ELEM_SET };
static const int NumExodusArrayTypeIndices =
  sizeof(ExodusArrayTypeIndices) / sizeof(ExodusArrayTypeIndices[0]);

static const int ExodusObjectTypeIndices[] = { vtkExodusIIReader::EDGE_BLOCK,
  vtkExodusIIReader::FACE_BLOCK, vtkExodusIIReader::ELEM_BLOCK, vtkExodusIIReader::NODE_SET,
  vtkExodusIIReader::SIDE_SET, vtkExodusIIReader::EDGE_SET, vtkExodusIIReader::FACE_SET,
  vtkExodusIIReader::ELEM_SET, vtkExodusIIReader::NODE_MAP, vtkExodusIIReader::EDGE_MAP,
  vtkExodusIIReader::FACE_MAP, vtkExodusIIReader::ELEM_MAP };
static const int NumExodusObjectTypeIndices =
  sizeof(ExodusObjectTypeIndices) / sizeof(ExodusObjectTypeIndices[0]);

//=============================================================================
class vtkExodusFileSeriesReaderStatus
{
public:
  void RecordStatus(vtkExodusIIReader* reader);
  void RestoreStatus(vtkExodusIIReader* reader);

protected:
  class ObjectStatus
  {
  public:
    ObjectStatus(const char* n, int s)
      : name(n)
      , status(s)
    {
    }
    std::string name;
    int status;
  };
  typedef std::vector<ObjectStatus> ObjectStatusList;
  ObjectStatusList ArrayStatuses[NumExodusArrayTypeIndices];
  ObjectStatusList ObjectStatuses[NumExodusObjectTypeIndices];
};

//-----------------------------------------------------------------------------
void vtkExodusFileSeriesReaderStatus::RecordStatus(vtkExodusIIReader* reader)
{
  int i;

  for (i = 0; i < NumExodusArrayTypeIndices; i++)
  {
    int arrayType = ExodusArrayTypeIndices[i];
    this->ArrayStatuses[i].clear();
    for (int j = 0; j < reader->GetNumberOfObjectArrays(arrayType); j++)
    {
      this->ArrayStatuses[i].push_back(ObjectStatus(
        reader->GetObjectArrayName(arrayType, j), reader->GetObjectArrayStatus(arrayType, j)));
    }
  }

  for (i = 0; i < NumExodusObjectTypeIndices; i++)
  {
    int objectType = ExodusObjectTypeIndices[i];
    this->ObjectStatuses[i].clear();
    for (int j = 0; j < reader->GetNumberOfObjects(objectType); j++)
    {
      this->ObjectStatuses[i].push_back(
        ObjectStatus(reader->GetObjectName(objectType, j), reader->GetObjectStatus(objectType, j)));
    }
  }
}

//-----------------------------------------------------------------------------
void vtkExodusFileSeriesReaderStatus::RestoreStatus(vtkExodusIIReader* reader)
{
  int i;

  for (i = 0; i < NumExodusArrayTypeIndices; i++)
  {
    int arrayType = ExodusArrayTypeIndices[i];
    for (ObjectStatusList::iterator j = this->ArrayStatuses[i].begin();
         j != this->ArrayStatuses[i].end(); j++)
    {
      reader->SetObjectArrayStatus(arrayType, j->name.c_str(), j->status);
    }
  }

  for (i = 0; i < NumExodusObjectTypeIndices; i++)
  {
    int objectType = ExodusObjectTypeIndices[i];
    for (ObjectStatusList::iterator j = this->ObjectStatuses[i].begin();
         j != this->ObjectStatuses[i].end(); j++)
    {
      reader->SetObjectStatus(objectType, j->name.c_str(), j->status);
    }
  }
}

//=============================================================================
vtkStandardNewMacro(vtkExodusFileSeriesReader);

//-----------------------------------------------------------------------------
vtkExodusFileSeriesReader::vtkExodusFileSeriesReader()
{
}

vtkExodusFileSeriesReader::~vtkExodusFileSeriesReader()
{
}

void vtkExodusFileSeriesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkExodusFileSeriesReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->UseMetaFile)
  {
    this->FindRestartedResults();
  }

  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}

//-----------------------------------------------------------------------------
int vtkExodusFileSeriesReader::RequestInformationForInput(
  int index, vtkInformation* request, vtkInformationVector* outputVector)
{
  if (index != this->_FileIndex)
  {
    vtkExodusIIReader* reader = vtkExodusIIReader::SafeDownCast(this->Reader);
    if (!reader)
    {
      vtkWarningMacro(<< "Using a non-exodus reader (" << reader->GetClassName()
                      << ") with vtkExodusFileSeriesReader.");
      return this->Superclass::RequestInformationForInput(index, request, outputVector);
    }

    // Save the state of what to read in.
    vtkExodusFileSeriesReaderStatus readerStatus;
    readerStatus.RecordStatus(reader);

    // It is sometimes the case that the server manager state mechanism will
    // push values to FilePattern and FilePrefix when in fact these should be
    // set internally (bug #10570).  This is a problem when we really have a
    // time file series.  Since the FilePattern/Prefix don't work with a time
    // file series, just set them to NULL.
    if (this->GetNumberOfFileNames() > 1)
    {
      vtkPExodusIIReader* preader = vtkPExodusIIReader::SafeDownCast(reader);
      if (preader)
      {
        preader->SetFilePattern(NULL);
        preader->SetFilePrefix(NULL);
      }
    }

    int retVal = this->Superclass::RequestInformationForInput(index, request, outputVector);

    // Restore the state.
    readerStatus.RestoreStatus(reader);

    return retVal;
  }

  return this->Superclass::RequestInformationForInput(index, request, outputVector);
}

//-----------------------------------------------------------------------------
void vtkExodusFileSeriesReader::FindRestartedResults()
{
  if (this->GetNumberOfFileNames() < 1)
  {
    vtkWarningMacro(<< "No files given.");
    return;
  }

  std::string originalFile = this->GetFileName(0);
  this->RemoveAllFileNamesInternal();

  std::string path;
  std::string baseName;
  std::string::size_type dirseppos = originalFile.find_last_of("/\\");
  if (dirseppos == std::string::npos)
  {
    path = "./";
    baseName = originalFile;
  }
  else
  {
    path = originalFile.substr(0, dirseppos + 1);
    baseName = originalFile.substr(dirseppos + 1);
  }

  // We are looking for files following the convention of
  //
  //   <file>.e-s.<rs#>.<numproc>.<rank>
  //
  // When the simulation was run on a single process, the .<numproc>.<rank> is
  // optional.  If it exists, we will look exclusively for files with that
  // extension as all restarts should have been run on that process.
  //
  // Additionally, the -s.<rs#> (which captures the restart number) is often not
  // there on the first file set (before the first restart occurred).  Thus it is
  // optional (although there can only be one as this is the only place we check
  // numbering).
  //
  // The `e` in the above pattern can be any of exodus supported extensions e.g.
  // exo, ex2, etc. Hence we keep the regex generic (ref: paraview/paraview#19056,
  // paraview/paraview#19202).
  vtksys::RegularExpression regEx("^(.*\\.[eg][^-.]*)(-s.[0-9]+)?(\\.[0-9]+\\.[0-9]+)?$");
  if (!regEx.find(baseName))
  {
    // Filename does not follow convention.  Just use it.
    this->AddFileNameInternal(originalFile.c_str());
    return;
  }

  std::string prefix = regEx.match(1);
  std::string suffix = regEx.match(3);

  VTK_CREATE(vtkDirectory, dir);
  if (!dir->Open(path.c_str()))
  {
    vtkWarningMacro(<< "Could not open directory " << originalFile.c_str()
                    << " is supposed to be from (" << path.c_str() << ")");
    this->AddFileNameInternal(originalFile.c_str());
    return;
  }

  for (vtkIdType i = 0; i < dir->GetNumberOfFiles(); i++)
  {
    const char* file = dir->GetFile(i);
    if (!regEx.find(file))
      continue;
    if (regEx.match(1) != prefix)
      continue;
    if (regEx.match(3) != suffix)
      continue;
    this->AddFileNameInternal((path + file).c_str());
  }
  this->CopyRealFileNamesFromFileNames();

  // Check to make sure we found something.
  if (this->GetNumberOfFileNames() < 1)
  {
    vtkWarningMacro(<< "Could not find any actual files matching " << originalFile.c_str());
    this->AddFileNameInternal(originalFile.c_str());
    this->CopyRealFileNamesFromFileNames();
  }
}
