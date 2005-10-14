/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqSpinBox_h
#define _pqSpinBox_h

#include <QSpinBox>
#include <vtkCommand.h>

class vtkSMProxy;
class vtkSMProperty;
class vtkSMIntVectorProperty;

class pqSpinBox :
  public QSpinBox,
  public vtkCommand
{
  Q_OBJECT
  
public:
  pqSpinBox(vtkSMProxy* const Proxy, vtkSMProperty* const Property, QWidget* Parent, const char* Name);
  ~pqSpinBox();
  
  virtual void Execute(vtkObject* caller, unsigned long eventId, void* callData);

private:
  vtkSMProxy* const proxy;
  vtkSMProperty* const property;
  vtkSMIntVectorProperty* const concrete_property;
  
  void updateState();
  
private slots:
  void onValueChanged(int);
};

#endif // !_pqSpinBox_h

