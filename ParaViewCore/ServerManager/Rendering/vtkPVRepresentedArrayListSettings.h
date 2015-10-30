/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRepresentedArrayListSettings.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRepresentedArrayListSettings - singleton used to filter out undesired array names from color array list.
// 
// .SECTION Description
// vtkPVRepresentedArrayListSettings is a singleton used to keep track
// of a list of regular expressions that filter out arrays in a
// RepresentedArrayList domain. All class to
// vtkPVRepresentedArrayListSettings::New() returns a reference to the
// singleton instance.

#ifndef vtkPVRepresentedArrayListSettings_h
#define vtkPVRepresentedArrayListSettings_h

#include "vtkObject.h"
#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSmartPointer.h" // needed for vtkSmartPointer

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkPVRepresentedArrayListSettings : public vtkObject
{
public:
  static vtkPVRepresentedArrayListSettings* New();
  vtkTypeMacro(vtkPVRepresentedArrayListSettings, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Access the singleton.
  static vtkPVRepresentedArrayListSettings* GetInstance();

  // Description:
  // Set/get the number of filter expressions.
  virtual void SetNumberOfFilterExpressions(int n);
  virtual int GetNumberOfFilterExpressions();

  // Description:
  // Set/get the filter expression at index i. If the index is
  // outside the valid range, this call is a noop.
  virtual void SetFilterExpression(int i, const char* expression);
  virtual const char* GetFilterExpression(int i);

//BTX
protected:
  vtkPVRepresentedArrayListSettings();
  ~vtkPVRepresentedArrayListSettings();

private:
  vtkPVRepresentedArrayListSettings(const vtkPVRepresentedArrayListSettings&); // Not implemented
  void operator=(const vtkPVRepresentedArrayListSettings&); // Not implemented

  static vtkSmartPointer<vtkPVRepresentedArrayListSettings> Instance;

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
