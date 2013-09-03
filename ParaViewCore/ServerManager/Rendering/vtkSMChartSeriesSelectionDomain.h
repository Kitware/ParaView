/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMChartSeriesSelectionDomain
// .SECTION Description
//

#ifndef __vtkSMChartSeriesSelectionDomain_h
#define __vtkSMChartSeriesSelectionDomain_h

#include "vtkSMStringListDomain.h"
#include "vtkPVServerManagerRenderingModule.h" // needed for exports

class vtkPVDataInformation;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMChartSeriesSelectionDomain :
  public vtkSMStringListDomain
{
public:
  static vtkSMChartSeriesSelectionDomain* New();
  vtkTypeMacro(vtkSMChartSeriesSelectionDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Update self checking the "unchecked" values of all required
  // properties.
  virtual void Update(vtkSMProperty*);

  enum DefaultModes
    {
    UNDEFINED,
    VISIBILITY,
    LABEL,
    COLOR,
    VALUE
    };

  // Description:
  // Set the property's default value based on the domain. How the value is
  // determined using the range is controlled by DefaultMode.
  virtual int SetDefaultValues(vtkSMProperty*);

  // Description:
  // Get the default-mode that controls how SetDefaultValues() behaves.
  vtkGetMacro(DefaultMode, int);

  // Description:
  // Add/Remove series names to hide by default. These are regular expressions.
  static void AddSeriesVisibilityDefault(const char* regex, bool value);

//BTX
protected:
  vtkSMChartSeriesSelectionDomain();
  ~vtkSMChartSeriesSelectionDomain();

  // Description:
  // Returns the datainformation from the current input, if possible.
  vtkPVDataInformation* GetInputInformation();

  // Description:
  // Process any specific XML definition tags.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  // Description:
  // Returns the default visibility for a series given its name.
  virtual bool GetDefaultSeriesVisibility(const char*);

  int DefaultMode;

  // Description:
  // Value used when DefaultMode==VALUE
  char* DefaultValue;
  vtkSetStringMacro(DefaultValue);

private:
  vtkSMChartSeriesSelectionDomain(const vtkSMChartSeriesSelectionDomain&); // Not implemented
  void operator=(const vtkSMChartSeriesSelectionDomain&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
