// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSMDoubleMapPropertyIterator_h
#define vtkSMDoubleMapPropertyIterator_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkSMDoubleMapProperty;
class vtkSMDoubleMapPropertyIteratorInternals;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMDoubleMapPropertyIterator : public vtkSMObject
{
public:
  static vtkSMDoubleMapPropertyIterator* New();
  vtkTypeMacro(vtkSMDoubleMapPropertyIterator, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Set/get the property to iterate over.
  virtual void SetProperty(vtkSMDoubleMapProperty* property);
  vtkGetObjectMacro(Property, vtkSMDoubleMapProperty);

  // Description:
  // Go to the first item.
  virtual void Begin();

  // Description:
  // Returns true if iterator points past the end of the collection.
  virtual int IsAtEnd();

  // Description:
  // Move to the next item.
  virtual void Next();

  // Description:
  // Returns the key (index) at the current iterator position.
  virtual vtkIdType GetKey();

  // Description:
  // Returns the value of the component for the current value.
  virtual double GetElementComponent(unsigned int component);

protected:
  vtkSMDoubleMapPropertyIterator();
  ~vtkSMDoubleMapPropertyIterator() override;

private:
  vtkSMDoubleMapPropertyIterator(const vtkSMDoubleMapPropertyIterator&) = delete;
  void operator=(const vtkSMDoubleMapPropertyIterator&) = delete;

  vtkSMDoubleMapProperty* Property;
  vtkSMDoubleMapPropertyIteratorInternals* Internals;
};

#endif
