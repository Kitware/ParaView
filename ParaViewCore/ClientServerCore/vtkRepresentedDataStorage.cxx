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
#include "vtkRepresentedDataStorage.h"
#include "vtkRepresentedDataStorageInternals.h"

#include "vtkObjectFactory.h"
#include "vtkAlgorithmOutput.h"

vtkStandardNewMacro(vtkRepresentedDataStorage);
//----------------------------------------------------------------------------
vtkRepresentedDataStorage::vtkRepresentedDataStorage()
  : Internals(new vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkRepresentedDataStorage::~vtkRepresentedDataStorage()
{
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
unsigned long vtkRepresentedDataStorage::GetVisibleDataSize(bool low_res)
{
  return this->Internals->GetVisibleDataSize(low_res);
}

//----------------------------------------------------------------------------
void vtkRepresentedDataStorage::RegisterRepresentation(
  int id, vtkPVDataRepresentation* repr)
{
  this->Internals->RepresentationToIdMap[repr] = id;

  vtkInternals::vtkItem item;
  item.Representation = repr;
  this->Internals->ItemsMap[id].first = item;

  vtkInternals::vtkItem item2;
  item2.Representation = repr;
  this->Internals->ItemsMap[id].second= item2;
}

//----------------------------------------------------------------------------
void vtkRepresentedDataStorage::UnRegisterRepresentation(
  vtkPVDataRepresentation* repr)
{
  vtkInternals::RepresentationToIdMapType::iterator iter =
    this->Internals->RepresentationToIdMap.find(repr);
  if (iter == this->Internals->RepresentationToIdMap.end())
    {
    vtkErrorMacro("Invalid argument.");
    return;
    }
  this->Internals->ItemsMap.erase(iter->second);
  this->Internals->RepresentationToIdMap.erase(iter);
}

//----------------------------------------------------------------------------
void vtkRepresentedDataStorage::SetDeliverToAllProcesses(
  vtkPVDataRepresentation* repr, bool mode, bool low_res)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, low_res);
  if (item)
    {
    item->AlwaysClone = mode;
    }
  else
    {
    vtkErrorMacro("Invalid argument.");
    }
}


//----------------------------------------------------------------------------
void vtkRepresentedDataStorage::SetPiece(
  vtkPVDataRepresentation* repr, vtkDataObject* data, bool low_res)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, low_res);
  if (item)
    {
    item->SetDataObject(data);
    }
  else
    {
    vtkErrorMacro("Invalid argument.");
    }
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkRepresentedDataStorage::GetProducer(
  vtkPVDataRepresentation* repr, bool low_res)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr, low_res);
  if (!item)
    {
    vtkErrorMacro("Invalid arguments.");
    return NULL;
    }

  return item->GetProducer()->GetOutputPort(0);
}

//----------------------------------------------------------------------------
void vtkRepresentedDataStorage::SetPiece(int id, vtkDataObject* data, bool
  low_res)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(id, low_res);
  if (item)
    {
    item->SetDataObject(data);
    }
  else
    {
    vtkErrorMacro("Invalid argument.");
    }
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkRepresentedDataStorage::GetProducer(
  int id, bool low_res)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(id, low_res);
  if (!item)
    {
    vtkErrorMacro("Invalid arguments.");
    return NULL;
    }

  return item->GetProducer()->GetOutputPort(0);
}

//----------------------------------------------------------------------------
void vtkRepresentedDataStorage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
