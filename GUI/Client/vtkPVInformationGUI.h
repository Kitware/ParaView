/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInformationGUI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVInformationGUI - Meta widget that displays data information.
// .SECTION Description
// This widget displays information about the output of a source.
// Call update with the source as a parameter to set the information
// displayed by the widget.

#ifndef __vtkPVInformationGUI_h
#define __vtkPVInformationGUI_h


#include "vtkKWFrame.h"
class vtkPVSource;
class vtkKWLabeledFrame;
class vtkKWLabel;
class vtkKWBoundsDisplay;

class VTK_EXPORT vtkPVInformationGUI : public vtkKWFrame
{
public:
  static vtkPVInformationGUI* New();
  vtkTypeRevisionMacro(vtkPVInformationGUI, vtkKWFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the user interface.
  void Create(vtkKWApplication* app, const char* options);
  
  // Description:
  // This updates the user interface.  It checks first to see if the
  // data has changed.  If nothing has changes, it is smart enough
  // to do nothing.
  void Update(vtkPVSource* source);
      
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  // Not needed here, but implemented for consistency.
  virtual void UpdateEnableState();
        
protected:
  vtkPVInformationGUI();
  ~vtkPVInformationGUI();
  
  vtkKWLabeledFrame *StatsFrame;

  vtkKWLabel *TypeLabel;
  vtkKWLabel *NumDataSetsLabel;
  vtkKWLabel *NumCellsLabel;
  vtkKWLabel *NumPointsLabel;
  vtkKWLabel *MemorySizeLabel;
  
  vtkKWBoundsDisplay *BoundsDisplay;
  vtkKWBoundsDisplay *ExtentDisplay;

  vtkPVInformationGUI(const vtkPVInformationGUI&); // Not implemented
  void operator=(const vtkPVInformationGUI&); // Not implemented
};

#endif
