/*=========================================================================

  Module:    vtkKWWidgetWithLabel.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWWidgetWithLabel - an abstract class widget with a label
// .SECTION Description
// This class implements an abstract superclass for composite widgets
// associating a label to a widget.

#ifndef __vtkKWWidgetWithLabel_h
#define __vtkKWWidgetWithLabel_h

#include "vtkKWCompositeWidget.h"

class vtkKWLabel;

class KWWidgets_EXPORT vtkKWWidgetWithLabel : public vtkKWCompositeWidget
{
public:
  static vtkKWWidgetWithLabel* New();
  vtkTypeRevisionMacro(vtkKWWidgetWithLabel, vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the internal label visibility (On by default).
  // IMPORTANT: if you know you may not show the label, try to
  // set that flag as early as possible (ideally, before calling Create()) 
  // in order to lower the footprint of the widget: the label will not be
  // allocated and created if there is no need to show it.
  // Later on, you can still use that option to show the label: it will be
  // allocated and created on the fly.
  virtual void SetLabelVisibility(int);
  vtkBooleanMacro(LabelVisibility, int);
  vtkGetMacro(LabelVisibility, int);

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
  // Set/Get the contents label.
  // IMPORTANT: SetLabelText will create the label on the fly, use it only if
  // you are confident that you will indeed display the label.
  virtual void SetLabelText(const char *);
  const char* GetLabelText();
  
  // Description:
  // Set/Get the label width.
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
    { this->SetLabelPosition(vtkKWWidgetWithLabel::LabelPositionDefault); };
  virtual void SetLabelPositionToTop()
    { this->SetLabelPosition(vtkKWWidgetWithLabel::LabelPositionTop); };
  virtual void SetLabelPositionToBottom()
    { this->SetLabelPosition(vtkKWWidgetWithLabel::LabelPositionBottom); };
  virtual void SetLabelPositionToLeft()
    { this->SetLabelPosition(vtkKWWidgetWithLabel::LabelPositionLeft); };
  virtual void SetLabelPositionToRight()
    { this->SetLabelPosition(vtkKWWidgetWithLabel::LabelPositionRight); };
  
  // Description:
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWWidgetWithLabel();
  ~vtkKWWidgetWithLabel();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  // Description:
  // Label visibility
  int LabelVisibility;

  // Description:
  // Label position
  int LabelPosition;

  // Description:
  // Create the label
  virtual void CreateLabel();

  // Description:
  // Pack or repack the widget. To be implemented by subclasses.
  virtual void Pack() {};

private:

  // Description:
  // Internal label
  // In 'private:' to allow lazy evaluation. GetLabel() will create the
  // label if it does not exist. This allow the object to remain lightweight. 
  vtkKWLabel *Label;

  vtkKWWidgetWithLabel(const vtkKWWidgetWithLabel&); // Not implemented
  void operator=(const vtkKWWidgetWithLabel&); // Not implemented
};

#endif
