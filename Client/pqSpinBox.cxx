/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqCommand.h"
#include "pqCommandDispatcher.h"
#include "pqCommandDispatcherManager.h"
#include "pqSpinBox.h"

#include <vtkSMIntRangeDomain.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxy.h>

namespace
{

class pqSpinBoxCommand :
  public pqCommand
{
public:
  pqSpinBoxCommand(vtkSMProxy& Proxy, vtkSMIntVectorProperty& ConcreteProperty, int OldValue, int NewValue) :
    proxy(Proxy),
    concrete_property(ConcreteProperty),
    old_value(OldValue),
    new_value(NewValue)
  {
  }

  void undoCommand()
  {
    concrete_property.SetElement(0, old_value);
    proxy.UpdateVTKObjects();
    proxy.MarkConsumersAsModified();
  }
  
  void redoCommand()
  {
    concrete_property.SetElement(0, new_value);
    proxy.UpdateVTKObjects();
    proxy.MarkConsumersAsModified();
  }

private:
  vtkSMProxy& proxy;
  vtkSMIntVectorProperty& concrete_property;
  const int old_value;
  const int new_value;
};

} // namespace


pqSpinBox::pqSpinBox(vtkSMProxy* const Proxy, vtkSMProperty* const Property, QWidget* Parent, const char* Name) :
  QSpinBox(Parent),
  proxy(Proxy),
  property(Property),
  concrete_property(vtkSMIntVectorProperty::SafeDownCast(Property))
{
  this->setObjectName(Name);

  updateState(); 
  
  QObject::connect(this, SIGNAL(valueChanged(int)), this, SLOT(onValueChanged(int)));
  
  if(property)
    property->AddObserver(vtkCommand::ModifiedEvent, this);
  
  /** \todo Need to observe changes to the property domain, once that capability is available */
}

pqSpinBox::~pqSpinBox()
{
  if(property)
    property->RemoveObserver(this);
  this->SetReferenceCount(0);
}

void pqSpinBox::onValueChanged(int Value)
{
  if(proxy && concrete_property)
    {
    const int old_value = concrete_property->GetElement(0);
    const int new_value = Value;
    
    if(old_value != new_value)
      pqCommandDispatcherManager::instance().getDispatcher().dispatchCommand(new pqSpinBoxCommand(*proxy, *concrete_property, old_value, new_value));
    }
}

void pqSpinBox::Execute(vtkObject* caller, unsigned long eventId, void* callData)
{
  updateState();
}

void pqSpinBox::updateState()
{
  if(!concrete_property)
    return;

  if(vtkSMIntRangeDomain* const domain = vtkSMIntRangeDomain::SafeDownCast(concrete_property->GetDomain("range")))
    {
    int exists;
    const int min_value = domain->GetMinimum(0, exists);
    const int max_value = domain->GetMaximum(0, exists);
    
    if(this->minimum() != min_value)
      this->setMinimum(min_value);
      
    if(this->maximum() != max_value)
      this->setMaximum(max_value);
    }
  
  const int new_value = concrete_property->GetElement(0);
  if(this->value() != new_value)
    this->setValue(new_value);
}

