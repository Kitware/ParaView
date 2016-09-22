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
// .NAME vtkSMChartSeriesSelectionDomain - extends vtkSMChartSeriesListDomain to
// add logic to better handle default values suitable for series-parameter type
// properties such as SeriesVisibility, SeriesLabel, etc.
// .SECTION Description
// vtkSMChartSeriesSelectionDomain extends vtkSMChartSeriesListDomain to
// add logic to better handle default values suitable for series-parameter type
// properties such as SeriesVisibility, SeriesLabel, etc.
//
// This domain also supports an experimental feature (we can generalize this to
// vtkSMDomain is found useful in other places). Generally, a vtkSMProperty
// never changes unless the application/user updates it. However for things like
// series parameters, it is useful if the property is updated to handle
// changed/newly added series consistently in the Qt application and the Python.
// To support that, this domain resets the property value to default every time
// the domain changes preserving status for existing series i.e. it won't affect
// the state for any series that already set on the property. Thus, it's not a
// true "reset", but more like "update".

#ifndef vtkSMChartSeriesSelectionDomain_h
#define vtkSMChartSeriesSelectionDomain_h

#include "vtkPVServerManagerRenderingModule.h" // needed for exports
#include "vtkSMStringListDomain.h"

#include <set> // For std::set

class vtkPVDataInformation;
class vtkPVArrayInformation;
class vtkChartRepresentation;

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
  virtual int SetDefaultValues(vtkSMProperty*, bool use_unchecked_values);

  // Description:
  // Get the default-mode that controls how SetDefaultValues() behaves.
  vtkGetMacro(DefaultMode, int);

  // Description:
  // Add/Remove series names to hide by default. These are regular expressions.
  static void AddSeriesVisibilityDefault(const char* regex, bool value);

  // Description:
  // Global flag to toggle between (a) the default behavior and
  // (b) setting default visibility to off.
  static void SetLoadNoChartVariables(bool choice)
  { vtkSMChartSeriesSelectionDomain::LoadNoVariables = choice; }
  static bool GetLoadNoChartVariables()
  { return vtkSMChartSeriesSelectionDomain::LoadNoVariables; }

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

  // Description:
  // Get the default value that will be used for the series with the given name
  // by this domain.
  virtual std::vector<vtkStdString> GetDefaultValue(const char* series);

  // Description:
  // Build up the domain with available series names.
  // Add arrays from dataInfo to strings. If blockName is non-empty, then it's
  // used to "uniquify" the array names.
  virtual void PopulateAvailableArrays(
    const std::string& blockName,
    std::vector<vtkStdString>& strings,
    vtkPVDataInformation* dataInfo, int fieldAssociation, bool flattenTable);

  // Description:
  // Build up the domain with provided array.
  // Add array component from dataArray to strings. If blockName is non-empty, then it's
  // used to "uniquify" the array names.
  virtual void PopulateArrayComponents(
    vtkChartRepresentation* chartRepr,
    const std::string& blockName, 
    std::vector<vtkStdString>& strings,
    std::set<vtkStdString>& unique_strings, 
    vtkPVArrayInformation* dataInfo, bool flattenTable);

  // Description:
  // Call this method in PopulateAvailableArrays() to override a specific array's
  // default visibility. Used for hiding array components, by default, for
  // example.
  virtual void SetDefaultVisibilityOverride(const vtkStdString& arrayname, bool visibility);

  int DefaultMode;

  // Description:
  // Value used when DefaultMode==VALUE
  char* DefaultValue;
  vtkSetStringMacro(DefaultValue);

  // Description:
  // Specify if table components should be split.
  bool FlattenTable;

  static bool LoadNoVariables;

private:
  vtkSMChartSeriesSelectionDomain(const vtkSMChartSeriesSelectionDomain&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMChartSeriesSelectionDomain&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;

  // The EXPERIMENTAL feature: everytime domain is modified we update the
  // property's value.
  void OnDomainModified();
  void UpdateDefaultValues(vtkSMProperty*, bool preserve_previous_values);

};

#endif
