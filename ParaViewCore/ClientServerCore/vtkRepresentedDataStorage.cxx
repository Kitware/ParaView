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

#include "vtkObjectFactory.h"
#include "vtkPVTrivialProducer.h"
#include "vtkAlgorithmOutput.h"
#include "vtkDataObject.h"
#include "vtkPVDataRepresentation.h"
#include "vtkCompositeRepresentation.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <map>

class vtkRepresentedDataStorage::vtkInternals
{
public:
  class vtkItem
    {
  public:
    vtkWeakPointer<vtkPVDataRepresentation> Representation;
    vtkSmartPointer<vtkPVTrivialProducer> Producer;
    vtkWeakPointer<vtkDataObject> DataObject;
    };

  vtkItem* GetItem(vtkPVDataRepresentation* repr)
    {
    RepresentationToIdMapType::iterator iter =
      this->RepresentationToIdMap.find(repr);
    if (iter != this->RepresentationToIdMap.end())
      {
      return &this->ItemsMap[iter->second];
      }

    ItemsMapType::iterator iter2;
    for (iter2 = this->ItemsMap.begin();
      iter2 != this->ItemsMap.end(); ++iter2)
      {
      vtkCompositeRepresentation* composite =
        vtkCompositeRepresentation::SafeDownCast(
          iter2->second.Representation.GetPointer());
      if (composite && composite->GetActiveRepresentation() == repr)
        {
        return &iter2->second;
        }
      }
    return NULL;
    }

  typedef std::map<vtkPVDataRepresentation*, int>
    RepresentationToIdMapType;

  typedef std::map<int, vtkItem> ItemsMapType;

  RepresentationToIdMapType RepresentationToIdMap;
  ItemsMapType ItemsMap;
};

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
void vtkRepresentedDataStorage::RegisterRepresentation(
  int id, vtkPVDataRepresentation* repr)
{
  this->Internals->RepresentationToIdMap[repr] = id;

  vtkInternals::vtkItem item;
  item.Representation = repr;
  item.Producer = vtkSmartPointer<vtkPVTrivialProducer>::New();
  this->Internals->ItemsMap[id] = item;
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
void vtkRepresentedDataStorage::SetPiece(
  vtkPVDataRepresentation* repr, vtkDataObject* data)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr);
  if (item)
    {
    item->DataObject = data;
    item->Producer->SetOutput(data);
    }
  else
    {
    vtkErrorMacro("Invalid argument.");
    }
}

//----------------------------------------------------------------------------
void vtkRepresentedDataStorage::RemovePiece(vtkPVDataRepresentation* repr)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr);
  if (item)
    {
    item->DataObject = NULL;
    item->Producer->SetOutput(NULL);
    }
  else
    {
    vtkErrorMacro("Invalid arguments.");
    }
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkRepresentedDataStorage::GetProducer(vtkPVDataRepresentation* repr)
{
  vtkInternals::vtkItem* item = this->Internals->GetItem(repr);
  if (!item)
    {
    vtkErrorMacro("Invalid arguments.");
    return NULL;
    }

  return item->Producer->GetOutputPort(0);
}

//----------------------------------------------------------------------------
void vtkRepresentedDataStorage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
