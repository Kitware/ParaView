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

#include "vtkExodusIIReader.h"
#include "vtkObjectFactory.h"
#include "vtkStdString.h"

#include <vtkstd/vector>

static const int ExodusArrayTypeIndices[] = {
  vtkExodusIIReader::GLOBAL,
  vtkExodusIIReader::NODAL,
  vtkExodusIIReader::EDGE_BLOCK,
  vtkExodusIIReader::FACE_BLOCK,
  vtkExodusIIReader::ELEM_BLOCK,
  vtkExodusIIReader::NODE_SET,
  vtkExodusIIReader::SIDE_SET,
  vtkExodusIIReader::EDGE_SET,
  vtkExodusIIReader::FACE_SET,
  vtkExodusIIReader::ELEM_SET
};
static const int NumExodusArrayTypeIndices
  = sizeof(ExodusArrayTypeIndices)/sizeof(ExodusArrayTypeIndices[0]);

static const int ExodusObjectTypeIndices[] = {
  vtkExodusIIReader::EDGE_BLOCK,
  vtkExodusIIReader::FACE_BLOCK,
  vtkExodusIIReader::ELEM_BLOCK,
  vtkExodusIIReader::NODE_SET,
  vtkExodusIIReader::SIDE_SET,
  vtkExodusIIReader::EDGE_SET,
  vtkExodusIIReader::FACE_SET,
  vtkExodusIIReader::ELEM_SET,
  vtkExodusIIReader::NODE_MAP,
  vtkExodusIIReader::EDGE_MAP,
  vtkExodusIIReader::FACE_MAP,
  vtkExodusIIReader::ELEM_MAP
};
static const int NumExodusObjectTypeIndices
  = sizeof(ExodusObjectTypeIndices)/sizeof(ExodusObjectTypeIndices[0]);

//=============================================================================
class vtkExodusFileSeriesReaderStatus
{
public:
  void RecordStatus(vtkExodusIIReader *reader);
  void RestoreStatus(vtkExodusIIReader *reader);
protected:
  class ObjectStatus {
  public:
    ObjectStatus(const char *n, int s) : name(n), status(s) { }
    vtkStdString name;
    int status;
  };
  typedef vtkstd::vector<ObjectStatus> ObjectStatusList;
  ObjectStatusList ArrayStatuses[NumExodusArrayTypeIndices];
  ObjectStatusList ObjectStatuses[NumExodusObjectTypeIndices];
};

//-----------------------------------------------------------------------------
void vtkExodusFileSeriesReaderStatus::RecordStatus(vtkExodusIIReader *reader)
{
  int i;

  for (i = 0; i < NumExodusArrayTypeIndices; i++)
    {
    int arrayType = ExodusArrayTypeIndices[i];
    this->ArrayStatuses[i].clear();
    for (int j = 0; j < reader->GetNumberOfObjectArrays(arrayType); j++)
      {
      this->ArrayStatuses[i].push_back(
                       ObjectStatus(reader->GetObjectArrayName(arrayType, j),
                                    reader->GetObjectArrayStatus(arrayType,j)));
      }
    }

  for (i = 0; i < NumExodusObjectTypeIndices; i++)
    {
    int objectType = ExodusObjectTypeIndices[i];
    this->ObjectStatuses[i].clear();
    for (int j = 0; j < reader->GetNumberOfObjects(objectType); j++)
      {
      this->ObjectStatuses[i].push_back(
                           ObjectStatus(reader->GetObjectName(objectType, j),
                                        reader->GetObjectStatus(objectType,j)));
      }
    }
}

//-----------------------------------------------------------------------------
void vtkExodusFileSeriesReaderStatus::RestoreStatus(vtkExodusIIReader *reader)
{
  int i;

  for (i = 0; i < NumExodusArrayTypeIndices; i++)
    {
    int arrayType = ExodusArrayTypeIndices[i];
    for (ObjectStatusList::iterator j = this->ArrayStatuses[i].begin();
         j != this->ArrayStatuses[i].end(); j++)
      {
      reader->SetObjectArrayStatus(arrayType, j->name, j->status);
      }
    }

  for (i = 0; i < NumExodusObjectTypeIndices; i++)
    {
    int objectType = ExodusObjectTypeIndices[i];
    for (ObjectStatusList::iterator j = this->ObjectStatuses[i].begin();
         j != this->ObjectStatuses[i].end(); j++)
      {
      reader->SetObjectStatus(objectType, j->name, j->status);
      }
    }
}

//=============================================================================
vtkCxxRevisionMacro(vtkExodusFileSeriesReader, "1.1");
vtkStandardNewMacro(vtkExodusFileSeriesReader);

//-----------------------------------------------------------------------------
vtkExodusFileSeriesReader::vtkExodusFileSeriesReader()
{
}

vtkExodusFileSeriesReader::~vtkExodusFileSeriesReader()
{
}

void vtkExodusFileSeriesReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkExodusFileSeriesReader::RequestInformationForInput(
                                             int index,
                                             vtkInformation *request,
                                             vtkInformationVector *outputVector)
{
  if (index != this->LastRequestInformationIndex)
    {
    vtkExodusIIReader *reader = vtkExodusIIReader::SafeDownCast(this->Reader);
    if (!reader)
      {
      vtkWarningMacro(<< "Using a non-exodus reader (" << reader->GetClassName()
                      << ") with vtkExodusFileSeriesReader.");
      return this->Superclass::RequestInformationForInput(index, request,
                                                          outputVector);
      }

    // Save the state of what to read in.
    vtkExodusFileSeriesReaderStatus readerStatus;
    readerStatus.RecordStatus(reader);
      
    int retVal = this->Superclass::RequestInformationForInput(index, request,
                                                              outputVector);

    // Restore the state.
    readerStatus.RestoreStatus(reader);

    return retVal;
    }

  return this->Superclass::RequestInformationForInput(index,
                                                      request, outputVector);
}
