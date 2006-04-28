/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtractDataSetsWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVExtractDataSetsWidget - widget that lists block of a composite dataset
// .SECTION Description
// vtkPVExtractDataSetsWidget is basically a vtkKWListBox that lists
// all blocks in a vtkHierarchicalDataSet (through vtkPVDataInformation).
// It allows the user to select zero or more blocks and assigns them
// in a SM property in pairs: (group, id), (group, id) ...
// The property should look like:
// @verbatim
//     <IntVectorProperty 
//        name="SelectedDataSets" 
//        command="AddDataSet"
//        clean_command="ClearDataSetList"
//        repeat_command="1"
//        number_of_elements_per_command="2"/>
// @endverbatim
// .SECTION See Also
// vtkPVCompositeDataInformation

#ifndef __vtkPVExtractDataSetsWidget_h
#define __vtkPVExtractDataSetsWidget_h

#include "vtkPVWidget.h"

class vtkKWPushButton;
class vtkKWWidget;
class vtkKWListBox;
class vtkKWFrame;

//BTX
struct vtkPVExtractDataSetsWidgetInternals;
//ETX

class VTK_EXPORT vtkPVExtractDataSetsWidget : public vtkPVWidget
{
public:
  static vtkPVExtractDataSetsWidget* New();
  vtkTypeRevisionMacro(vtkPVExtractDataSetsWidget, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Save this source to a file.
  void SaveInBatchScript(ofstream *file);

  // Description:
  // Button callbacks.
  void AllOnCallback();
  void AllOffCallback();

  // Description:
  // Access metod necessary for scripting.
  void SetSelectState(int idx, int val);

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Called when the Accept button is pressed. It moves the widget state to
  // the SM property.
  virtual void Accept();

  // Description:
  // This method resets the widget values from the VTK filter.
  virtual void ResetInternal();
  virtual void Initialize();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
  // Description:
  // Called when an item is selected. This checks if the user clicked
  // on a group item and selects/unselects the whole group.
  void PartSelectionCallback();

protected:
  vtkPVExtractDataSetsWidget();
  ~vtkPVExtractDataSetsWidget();

  // Description:
  // Set up the UI for this source
  void CreateWidget();

  vtkKWFrame* ButtonFrame;
  vtkKWPushButton* AllOnButton;
  vtkKWPushButton* AllOffButton;

  vtkKWListBox* PartSelectionList;

  void CommonInit();

private:
  vtkPVExtractDataSetsWidgetInternals* Internal;
  vtkPVExtractDataSetsWidget(const vtkPVExtractDataSetsWidget&); // Not implemented
  void operator=(const vtkPVExtractDataSetsWidget&); // Not implemented
};

#endif
