/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqCheckBox.h"
#include "pqCommand.h"
#include "pqCommandDispatcher.h"
#include "pqCommandDispatcherManager.h"

#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxy.h>

namespace
{

class pqCheckBoxCommand :
  public pqCommand
{
public:
  pqCheckBoxCommand(vtkSMProxy& Proxy, vtkSMIntVectorProperty& ConcreteProperty, bool OldState, bool NewState) :
    proxy(Proxy),
    concrete_property(ConcreteProperty),
    old_state(OldState),
    new_state(NewState)
  {
  }

  void undoCommand()
  {
    concrete_property.SetElement(0, old_state);
    proxy.UpdateVTKObjects();
    proxy.MarkConsumersAsModified();
  }
  
  void redoCommand()
  {
    concrete_property.SetElement(0, new_state);
    proxy.UpdateVTKObjects();
    proxy.MarkConsumersAsModified();
  }

private:
  vtkSMProxy& proxy;
  vtkSMIntVectorProperty& concrete_property;
  const bool old_state;
  const bool new_state;
};

} // namespace

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
  if(proxy && concrete_property)
    {
    const bool old_state = concrete_property->GetElement(0) ? true : false;
    const bool new_state = State == Qt::Checked ? true : false;
    
    if(old_state != new_state)
      pqCommandDispatcherManager::instance().getDispatcher().dispatchCommand(new pqCheckBoxCommand(*proxy, *concrete_property, old_state, new_state));
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

