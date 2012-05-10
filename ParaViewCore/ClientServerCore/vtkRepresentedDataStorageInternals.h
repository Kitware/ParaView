/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRepresentedDataStorageInternals
// .SECTION Description

#ifndef __vtkRepresentedDataStorageInternals_h
#define __vtkRepresentedDataStorageInternals_h

#include "vtkDataObject.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVTrivialProducer.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <map>
#include <queue>
#include <utility>

class vtkRepresentedDataStorage::vtkInternals
{
public:

  class vtkPriorityQueueItem
    {
  public:
    unsigned int RepresentationId;
    unsigned int BlockId;
    unsigned int Level;
    unsigned int Index;
    double Priority;

    vtkPriorityQueueItem() :
      RepresentationId(0), BlockId(0),
      Level(0), Index(0), Priority(0)
    {
    }

    bool operator < (const vtkPriorityQueueItem& other) const
      {
      return this->Priority < other.Priority;
      }
    };

  typedef std::priority_queue<vtkPriorityQueueItem> PriorityQueueType;
  PriorityQueueType PriorityQueue;

  class vtkItem
    {
    vtkSmartPointer<vtkPVTrivialProducer> Producer;
    vtkWeakPointer<vtkDataObject> DataObject;
    unsigned long TimeStamp;
    unsigned long ActualMemorySize;
  public:
    vtkWeakPointer<vtkPVDataRepresentation> Representation;
    bool AlwaysClone;
    bool Redistributable;
    bool Streamable;

    vtkItem() :
      Producer(vtkSmartPointer<vtkPVTrivialProducer>::New()),
      TimeStamp(0),
      ActualMemorySize(0),
      AlwaysClone(false),
      Redistributable(false),
      Streamable(false)
    { }

    void SetDataObject(vtkDataObject* data)
      {
      if (data && data->GetMTime() > this->TimeStamp)
        {
        this->DataObject = data;
        this->Producer->SetOutput(data);
        // the vtkTrivialProducer's MTime is is changed whenever the data itself
        // changes or the data's mtime changes and hence we get a good indication
        // of when we have a new piece for real.
        this->TimeStamp = this->Producer->GetMTime();
        this->ActualMemorySize = data? data->GetActualMemorySize() : 0;
        }
      }

    vtkPVTrivialProducer* GetProducer() const
      { return this->Producer.GetPointer(); }
    vtkDataObject* GetDataObject() const
      { return this->DataObject.GetPointer(); }
    unsigned long GetTimeStamp() const
      { return this->TimeStamp; }
    unsigned long GetVisibleDataSize()
      {
      if (this->Representation && this->Representation->GetVisibility())
        {
        return this->ActualMemorySize;
        }
      return 0;
      }
    };

  typedef std::map<unsigned int, std::pair<vtkItem, vtkItem> > ItemsMapType;

  vtkItem* GetItem(unsigned int index, bool use_second)
    {
    if (this->ItemsMap.find(index) != this->ItemsMap.end())
      {
      return use_second? &(this->ItemsMap[index].second) :
        &(this->ItemsMap[index].first);
      }
    return NULL;
    }

  vtkItem* GetItem(vtkPVDataRepresentation* repr, bool use_second)
    {
    RepresentationToIdMapType::iterator iter =
      this->RepresentationToIdMap.find(repr);
    if (iter != this->RepresentationToIdMap.end())
      {
      unsigned int index = iter->second;
      return use_second? &(this->ItemsMap[index].second) :
        &(this->ItemsMap[index].first);
      }

    return NULL;
    }

  unsigned long GetVisibleDataSize(bool use_second_if_available)
    {
    unsigned long size = 0;
    ItemsMapType::iterator iter;
    for (iter = this->ItemsMap.begin(); iter != this->ItemsMap.end(); ++iter)
      {
      if (use_second_if_available && iter->second.second.GetDataObject())
        {
        size += iter->second.second.GetVisibleDataSize();
        }
      else
        {
        size += iter->second.first.GetVisibleDataSize();
        }
      }
    return size;
    }

  typedef std::map<vtkPVDataRepresentation*, unsigned int>
    RepresentationToIdMapType;

  RepresentationToIdMapType RepresentationToIdMap;
  ItemsMapType ItemsMap;
};
#endif
