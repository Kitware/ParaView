/*=========================================================================

  Module:    vtkKWWidgetLabeled.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWWidgetLabeled - an abstract class widget with a label
// .SECTION Description
// This class implements an abstract superclass for composite widgets
//  associating a label to a widget.

#ifndef __vtkKWWidgetLabeled_h
#define __vtkKWWidgetLabeled_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWLabel;

class VTK_EXPORT vtkKWWidgetLabeled : public vtkKWWidget
{
public:
  static vtkKWWidgetLabeled* New();
  vtkTypeRevisionMacro(vtkKWWidgetLabeled, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Get the internal label.
  // IMPORTANT: the internal label is "lazy created", i.e. it is neither
  // allocated nor created until GetLabel() is called. This allows 
  // for a lower footprint and faster UI startup. Therefore, do *not* use
  // GetLabel() to check if the label exists, as it will automatically
  // allocate the label. Use HasLabel() instead. 
  virtual vtkKWLabel* GetLabel();
  virtual int HasLabel();

  // Description:
  // Show/Hide the internal label (On by default).
  // IMPORTANT: if you know you may not show the label, try to
  // set that flag as early as possible (ideally, before calling Create()) 
  // in order to lower the footprint of the widget: the label will not be
  // allocated and created if there is no need to show it.
  // Later on, you can still use that option to show the label: it will be
  // allocated and created on the fly.
  virtual void SetShowLabel(int);
  vtkBooleanMacro(ShowLabel, int);
  vtkGetMacro(ShowLabel, int);

  // Description:
  // Convenience method to set the contents label.
  // IMPORTANT: this method will create the label on the fly, use it only if
  // you are confident that you will indeed display the label.
  virtual void SetLabel(const char *);
  
  // Description:
  // Convenience method to set/get the label width.
  // IMPORTANT: this method will create the label on the fly, use it only if
  // you are confident that you will indeed display the label.
  virtual void SetLabelWidth(int width);
  virtual int GetLabelWidth();

  // Description:
  // If supported, set the label position in regards to the rest of
  // the composite widget. Check the subclass for more information about
  // what the Default position is, and if specific positions are supported.
  //BTX
  enum
  {
    LabelPositionDefault = 0,
    LabelPositionTop,
    LabelPositionBottom,
    LabelPositionLeft,
    LabelPositionRight
  };
  //ETX
  virtual void SetLabelPosition(int);
  vtkGetMacro(LabelPosition, int);
  virtual void SetLabelPositionToDefault()
    { this->SetLabelPosition(vtkKWWidgetLabeled::LabelPositionDefault); };
  virtual void SetLabelPositionToTop()
    { this->SetLabelPosition(vtkKWWidgetLabeled::LabelPositionTop); };
  virtual void SetLabelPositionToBottom()
    { this->SetLabelPosition(vtkKWWidgetLabeled::LabelPositionBottom); };
  virtual void SetLabelPositionToLeft()
    { this->SetLabelPosition(vtkKWWidgetLabeled::LabelPositionLeft); };
  virtual void SetLabelPositionToRight()
    { this->SetLabelPosition(vtkKWWidgetLabeled::LabelPositionRight); };
  
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
  vtkKWWidgetLabeled();
  ~vtkKWWidgetLabeled();

  // Description:
  // Show/Hide the label
  int ShowLabel;

  // Description:
  // Label position
  int LabelPosition;

  // Description:
  // Pack or repack the widget. To be implemented by subclasses.
  virtual void Pack() {};

  // Description:
  // Create the label
  virtual void CreateLabel(vtkKWApplication *app, const char *args = 0);

private:

  // Description:
  // Internal label
  // In 'private:' to allow lazy evaluation. GetLabel() will create the
  // label if it does not exist. This allow the object to remain lightweight. 
  vtkKWLabel *Label;

  vtkKWWidgetLabeled(const vtkKWWidgetLabeled&); // Not implemented
  void operator=(const vtkKWWidgetLabeled&); // Not implemented
};

#endif
