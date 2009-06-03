/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqViewUpdater.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "pqViewUpdater.h"

#include <vtkAnnotationLink.h>
#include <vtkCallbackCommand.h>
#include <vtkSMProxy.h>

#include <pqView.h>

class pqViewUpdater::Implementation
{
public:
  Implementation() :
    View(0)
  {
  }

  static void RawExecute(vtkObject* object, unsigned long event, void* client_data, void* call_data)
  {
    reinterpret_cast<pqViewUpdater::Implementation*>(client_data)->Execute(object, event, call_data);
  }

  virtual void Execute(vtkObject* object, unsigned long event, void* call_data)
  {
    if(!this->View)
      {
      vtkGenericWarningMacro(<< "View not set.");
      return;
      }

    this->View->render();
  }

  pqView* View;
};

pqViewUpdater::pqViewUpdater()
{
  this->Internal = new Implementation();
}

pqViewUpdater::~pqViewUpdater()
{
  delete this->Internal;
}

void pqViewUpdater::SetView(pqView* const view)
{
  this->Internal->View = view;
}

void pqViewUpdater::AddLink(pqProxy* annotation_link)
{
  vtkAnnotationLink* const client_annotation_link = vtkAnnotationLink::SafeDownCast(annotation_link->getProxy()->GetClientSideObject());
  if(!client_annotation_link)
    {
    vtkGenericWarningMacro(<< "Not a client-side vtkAnnotationLink.");
    return;
    }

  vtkCallbackCommand* const command = vtkCallbackCommand::New();
  command->SetCallback(&pqViewUpdater::Implementation::RawExecute);
  command->SetClientData(this->Internal);

  client_annotation_link->AddObserver(vtkCommand::AnnotationChangedEvent, command);

  command->Delete();
}

