/*=========================================================================

  Module:    vtkKWLabeledRadioButtonSet.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLabeledRadioButtonSet - a radiobutton set with a label
// .SECTION Description
// This class creates a radiobutton set with a label on top of it; both are
// contained in a frame
// .SECTION See Also
// vtkKWRadioButtonSet

#ifndef __vtkKWLabeledRadioButtonSet_h
#define __vtkKWLabeledRadioButtonSet_h

#include "vtkKWLabeledWidget.h"

class vtkKWRadioButtonSet;

class VTK_EXPORT vtkKWLabeledRadioButtonSet : public vtkKWLabeledWidget
{
public:
  static vtkKWLabeledRadioButtonSet* New();
  vtkTypeRevisionMacro(vtkKWLabeledRadioButtonSet, vtkKWLabeledWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Get the internal object
  vtkGetObjectMacro(RadioButtonSet, vtkKWRadioButtonSet);

  // Description:
  // Set the widget packing order to be horizontal (default is vertical).
  virtual void SetPackHorizontally(int);
  vtkBooleanMacro(PackHorizontally, int);
  vtkGetMacro(PackHorizontally, int);

  // Description:
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);
  virtual void SetBalloonHelpJustification(int j);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWLabeledRadioButtonSet();
  ~vtkKWLabeledRadioButtonSet();

  vtkKWRadioButtonSet *RadioButtonSet;

  int PackHorizontally;

  // Pack or repack the widget.

  virtual void Pack();

private:
  vtkKWLabeledRadioButtonSet(const vtkKWLabeledRadioButtonSet&); // Not implemented
  void operator=(const vtkKWLabeledRadioButtonSet&); // Not implemented
};


#endif

