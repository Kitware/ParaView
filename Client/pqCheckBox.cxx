/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqCheckBox.h"

#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxy.h>

pqCheckBox::pqCheckBox(vtkSMProxy* const Proxy, vtkSMProperty* const Property, const QString& Text, QWidget* Parent, const char* Name) :
  QCheckBox(Text, Parent),
  proxy(Proxy),
  property(Property),
  concrete_property(vtkSMIntVectorProperty::SafeDownCast(Property))
{
  this->setObjectName(Name);
  this->setTristate(concrete_property ? false : true);
  
  updateState(); 
  
  QObject::connect(this, SIGNAL(stateChanged(int)), this, SLOT(onStateChanged(int)));
  if(property)
    property->AddObserver(vtkCommand::ModifiedEvent, this);
}

pqCheckBox::~pqCheckBox()
{
  if(property)
    property->RemoveObserver(this);
  this->SetReferenceCount(0);
}

void pqCheckBox::onStateChanged(int State)
{
  if(concrete_property)
    {
    const bool old_state = concrete_property->GetElement(0) ? true : false;
    const bool new_state = State == Qt::Checked ? true : false;
    
    if(old_state != new_state) 
      {
      concrete_property->SetElement(0, new_state);
      if(proxy)
        {
        proxy->UpdateVTKObjects();
        proxy->MarkConsumersAsModified();
        }
      }
    }
  else
    {
    if(State != Qt::PartiallyChecked)
      this->setCheckState(Qt::PartiallyChecked);
    }
}

void pqCheckBox::Execute(vtkObject* caller, unsigned long eventId, void* callData)
{
  updateState();
}

void pqCheckBox::updateState()
{
  if(concrete_property)
    {
    const Qt::CheckState new_state = concrete_property->GetElement(0) ? Qt::Checked : Qt::Unchecked;
    if(this->checkState() != new_state)
      this->setCheckState(new_state);
      
    this->setDisabled(false);
    }
  else
    {
    if(this->checkState() != Qt::PartiallyChecked)
      this->setCheckState(Qt::PartiallyChecked);
      
    this->setDisabled(true);
    }
}

