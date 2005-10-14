/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqSpinBox.h"

#include <vtkSMIntRangeDomain.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxy.h>

pqSpinBox::pqSpinBox(vtkSMProxy* const Proxy, vtkSMProperty* const Property, QWidget* Parent, const char* Name) :
  QSpinBox(Parent),
  proxy(Proxy),
  property(Property),
  concrete_property(vtkSMIntVectorProperty::SafeDownCast(Property))
{
  this->setName(Name);

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
  if(concrete_property)
    {
    if(concrete_property->GetElement(0) != Value) 
      {
      concrete_property->SetElement(0, Value);
      if(proxy)
        {
        proxy->UpdateVTKObjects();
        proxy->MarkConsumersAsModified();
        }
      }
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

