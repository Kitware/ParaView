/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVTempTessellatorEntry.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright 2003 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/
// .NAME vtkPVTempTessellatorEntry maintains a list of (field,accuracy) tuples for tessellations
// .SECTION Description
// This widget lets the user add/remove a floating point value for each point field
// defined over the input dataset. These floating point values are used as the
// maximum allowable chord error for edge subdivisions during tessellation.
//
// To use this widget as part of the GUI for a source or filter module, you create an
// XML description of the GUI that contains an entry for a TessellatorEntry widget
// like so:<code>
// <TessellatorEntry label="Max Field Error Squared" trace_name="FieldError2"
//                   property="FieldError2"
//                   help="The square of the maximum field error allowed at any edge midpoint in the output tessellation."
//                   input_menu_id="tess_source_select"
//                   />
// </code>
// The \a variable and \a type attributes specify the name and type of the underlying
// filter's member variable that is controlled by the TessellatorEntry GUI.
// The \a input_menu_id must correspond to the value of an InputMenu GUI entry's \a id
// attribute. The TessellatorEntry must reference the current input to the filter
// so that it can collect the names of point data arrays for its listbox.
// The \a property attribute specifies the server-manager property to be used
// with this widget. This property handles calling the appropriate commands
// to synchronize the filter with the GUI values when the user presses the
// Accept button.
#ifndef VTKSNL_PVTESSELLATOR_ENTRY_H
#define VTKSNL_PVTESSELLATOR_ENTRY_H

#include "vtkPVWidget.h"

class vtkPVInputMenu;
class vtkPVDataSetAttributesInformation;

class vtkTessellatorEntryData;

class VTK_EXPORT vtkPVTempTessellatorEntry : public vtkPVWidget
{
public:
  static vtkPVTempTessellatorEntry* New();
  vtkTypeRevisionMacro(vtkPVTempTessellatorEntry,vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the actual widget
  virtual void Create( vtkKWApplication* app );

  // Description:
  // Access to the label (for scripting)
  virtual void SetLabel( const char* );
  const char* GetLabel() const;
  char* GetLabel();

  // Description:
  // Actions that may performed on the widget
  void SetFieldCriterion( int fieldNumber, float criterion );
  void ResetFieldCriteria();

  // Description:
  // Callback when a field criterion is changed.
  virtual void ChangeCriterionCallback();

  // Description:
  // Callback when a field criterion is enabled/disabled.
  virtual void ToggleCriterionCallback();

  // Description:
  // Callback when a point field is selected.
  // This callback must set the widget values (CriterionEnable
  // and CriterionValue) to match those for the selected
  // field.
  virtual void PointDataSelectedCallback();

  // Description:
  // Add a script to the menu of the animation interface.
  virtual void AddAnimationScriptsToMenu( vtkKWMenu* menu, vtkPVAnimationInterfaceEntry* ai );

  // Description:
  // A function called whenever a user decides to animate tessellation criteria
  // with our animation script.
  void AnimationMenuCallback( vtkPVAnimationInterfaceEntry* ai );

  //BTX
  // Description:
  // Called when Accept is pressed
  virtual void Accept();
  //ETX

  // Description:
  // Trace calls and/or save state.
  virtual void Trace( ofstream *file );

  // Description:
  // Called when the GUI should be updated
  // using the state of the PVSource that serves as its input.
  // It is called when the user modifies the input menu associated with the filter.
  // This modifies the state of the GUI but not the \a Property, which
  // stores the values last sent to the server. See \a ResetInternal.
  virtual void Update();

  // Description:
  // Called when the widget should update its GUI entries
  // from the \a Property that stores the values last sent to the server.
  // If values have never been sent to the server (i.e., the user has never hit
  // the "Accept" button), then \a Update is called to put default entries into
  // the GUI.
  virtual void ResetInternal();

  // Description:
  // Set the names of the tessellator filter's methods used to
  // add or reset subdivision criteria.
  vtkSetStringMacro(ResetCriteriaCommand);
  vtkSetStringMacro(SetFieldCriterionCommand);

  // Description:
  // Enable/disable parts of the widget.
  // Used in this class to disable the widget when the filter's
  // input has no scalar fields (and thus no possible criteria
  // may be added).
  virtual void UpdateEnableState();

  // Description:
  // Get/Set the widget that is used to select the PVSource of the module.
  virtual void SetInputMenu( vtkPVInputMenu* );
  vtkGetObjectMacro(InputMenu,vtkPVInputMenu);
  const vtkPVInputMenu* GetInputMenu() const { return this->InputMenu; }

  //BTX
  // Description:
  // Get the point data associated with the module's source dataset.
  //
  // This isn't wrapped because vtkPVDataSetAttributesInformation is in
  // the vtkPVFilters library, which doesn't get wrapped.
  vtkPVDataSetAttributesInformation* GetPointDataInformation();
  //ETX

protected:
  vtkPVTempTessellatorEntry();
  ~vtkPVTempTessellatorEntry();

  char* ResetCriteriaCommand;
  char* SetFieldCriterionCommand;

  vtkPVInputMenu* InputMenu;

  vtkTessellatorEntryData* Data;

  virtual void UpdateProperty();

  vtkPVTempTessellatorEntry( const vtkPVTempTessellatorEntry& ); // Not implemented
  void operator = ( const vtkPVTempTessellatorEntry& ); // Not implemented

  //BTX
  virtual void CopyProperties( vtkPVWidget* clone, vtkPVSource* pvSource,
                               vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map );
  //ETX

  // Description:
  // Read attributes from the module description.
  int ReadXMLAttributes( vtkPVXMLElement* element, vtkPVXMLPackageParser* parser );

  // Description:
  // Save a script to reproduce the state of the widget in a .pvs file.
  virtual void SaveInBatchScriptForPart( ofstream *file, vtkClientServerID );
};

#endif // VTKSNL_PVTESSELLATOR_ENTRY_H
