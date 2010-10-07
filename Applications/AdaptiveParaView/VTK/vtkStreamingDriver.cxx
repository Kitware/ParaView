/*=========================================================================

  Program:   ParaView
  Module:    vtkStreamingDriver.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStreamingDriver.h"

#include "vtkCallbackCommand.h"
#include "vtkMapper.h"
#include "vtkMapperCollection.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkStreamAlgorithm.h"

vtkStandardNewMacro(vtkStreamingDriver);

class Internals
{
public:
  Internals(vtkStreamingDriver *owner)
  {
    this->Owner = owner;
    this->RenderWindow = NULL;
    this->WindowWatcher = NULL;
    this->StreamAlgorithm = vtkStreamAlgorithm::New();
    this->MapperCollection = vtkMapperCollection::New();
  }
  ~Internals()
  {
  this->Owner->SetRenderWindow(NULL);
  if (this->WindowWatcher)
    {
    this->WindowWatcher->Delete();
    }
  this->StreamAlgorithm->Delete();
  this->MapperCollection->Delete();
  }
  vtkStreamingDriver *Owner;
  vtkStreamAlgorithm *StreamAlgorithm;
  vtkRenderWindow *RenderWindow;
  vtkCallbackCommand *WindowWatcher;
  vtkMapperCollection *MapperCollection;
};

static void VTKSD_RenderEvent(vtkObject *vtkNotUsed(caller),
                              unsigned long vtkNotUsed(eventid),
                              void *who,
                              void *)
{
  vtkStreamingDriver *self = reinterpret_cast<vtkStreamingDriver*>(who);
  self->RenderEvent();
}

//----------------------------------------------------------------------------
vtkStreamingDriver::vtkStreamingDriver()
{
  this->Internal = new Internals(this);
}

//----------------------------------------------------------------------------
vtkStreamingDriver::~vtkStreamingDriver()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkStreamingDriver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkStreamingDriver::SetRenderWindow(vtkRenderWindow *rw)
{
  if (this->Internal->RenderWindow)
    {
    this->Internal->RenderWindow->Delete();
    }
  if (!rw)
    {
    return;
    }
  rw->Register(this);
  this->Internal->RenderWindow = rw;
  //TODO: Intialize swap off

  if (this->Internal->WindowWatcher)
    {
    this->Internal->WindowWatcher->Delete();
    }
  vtkCallbackCommand *cbc = vtkCallbackCommand::New();
  cbc->SetCallback(VTKSD_RenderEvent);
  cbc->SetClientData((void*)this);
  rw->AddObserver(vtkCommand::EndEvent,cbc);
  this->Internal->WindowWatcher = cbc;
}

//----------------------------------------------------------------------------
void vtkStreamingDriver::RenderEvent()
{
  cerr << "RENDER CALLED" << endl;
  //TODO:
  //for every mapper, set to next piece
  //call render again
}

//----------------------------------------------------------------------------
void vtkStreamingDriver::AddMapper(vtkMapper *mapper)
{
  if (!mapper)
    {
    return;
    }
  if (this->Internal->MapperCollection->IsItemPresent(a))
    {
    return;
    }
  this->Internal->MapperCollection->AddItem(mapper);
  //TODO: Initialize mapper to first piece
}

//----------------------------------------------------------------------------
void vtkStreamingDriver::RemoveMapper(vtkMapper *mapper)
{
  if (!mapper)
    {
    return;
    }
  this->Internal->MapperCollection->RemoveItem(mapper);
}

//----------------------------------------------------------------------------
void vtkStreamingDriver::RemoveAllMappers()
{
  this->Internal->MapperCollection->RemoveAllItems();
}
