/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqCheckBox_h
#define _pqCheckBox_h

#include <QCheckBox>
#include <vtkCommand.h>

class vtkSMProxy;
class vtkSMProperty;
class vtkSMIntVectorProperty;

class pqCheckBox :
  public QCheckBox,
  public vtkCommand
{
  Q_OBJECT
  
public:
  pqCheckBox(vtkSMProxy* const Proxy, vtkSMProperty* const Property, const QString& Text, QWidget* Parent, const char* Name);
  ~pqCheckBox();
  
  virtual void Execute(vtkObject* caller, unsigned long eventId, void* callData);

private:
  vtkSMProxy* const proxy;
  vtkSMProperty* const property;
  vtkSMIntVectorProperty* const concrete_property;
  
  void updateState();
  
private slots:
  void onStateChanged(int);
};

#endif // !_pqCheckBox_h

