/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqProperties.h"

#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProperty.h>
#include <vtkSMProxy.h>
#include <vtkSMStringVectorProperty.h>

#include <QString>

template<typename PropertyT, typename ValueT>
void pqSetProperty(vtkSMProxy* Proxy, const QString& Name, const ValueT& Value)
{
  if(!Proxy)
    {
    vtkOutputWindowDisplayErrorText("Invalid proxy\n");
    return;
    }
  
  if(Name.isEmpty())
    {
    vtkOutputWindowDisplayErrorText("Empty property name\n");
    return;
    }
  
  vtkSMProperty* const abstract_property = Proxy->GetProperty(Name.ascii());
  if(!abstract_property)
    {
    QString message = "Property [" + Name + "] not found\n";
    vtkOutputWindowDisplayErrorText(message.ascii());
    return;
    }
  
  PropertyT* const concrete_property = PropertyT::SafeDownCast(abstract_property);
  if(!concrete_property)
    {
    QString message = "Property [" + Name + "] type mismatch\n";
    vtkOutputWindowDisplayErrorText(message.ascii());
    return;
    }
  
  concrete_property->SetElement(0, Value);
}

void pqSetProperty(vtkSMProxy* Proxy, const QString& Name, const double Value)
{
  pqSetProperty<vtkSMDoubleVectorProperty, double>(Proxy, Name, Value);
}

void pqSetProperty(vtkSMProxy* Proxy, const QString& Name, const int Value)
{
  pqSetProperty<vtkSMIntVectorProperty, int>(Proxy, Name, Value);
}

void pqSetProperty(vtkSMProxy* Proxy, const QString& Name, const QString& Value)
{
  pqSetProperty<vtkSMStringVectorProperty, const char*>(Proxy, Name, Value.ascii());
}

