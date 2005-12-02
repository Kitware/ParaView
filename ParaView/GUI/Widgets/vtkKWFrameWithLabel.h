/*=========================================================================

  Module:    vtkKWFrameWithLabel.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWFrameWithLabel - a frame with a grooved border and a label
// .SECTION Description
// The vtkKWFrameWithLabel creates a frame with a grooved border, and a label
// embedded in the upper left corner of the grooved border.


#ifndef __vtkKWFrameWithLabel_h
#define __vtkKWFrameWithLabel_h

#include "vtkKWCompositeWidget.h"

class vtkKWFrame;
class vtkKWIcon;
class vtkKWLabel;
class vtkKWLabelWithLabel;

class KWWIDGETS_EXPORT vtkKWFrameWithLabel : public vtkKWCompositeWidget
{
public:
  static vtkKWFrameWithLabel* New();
  vtkTypeRevisionMacro(vtkKWFrameWithLabel,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Set the label for the frame.
  void SetLabelText(const char *);
  
  // Description:
  // Get the internal frame.
  vtkGetObjectMacro(Frame, vtkKWFrame);

  // Description:
  // Get the internal frame containing the label.
  vtkGetObjectMacro(LabelFrame, vtkKWFrame);

  // Description:
  // Get the label (frame title).
  virtual vtkKWLabel *GetLabel();

  // Description:
  // Collapse/expand the frame.
  virtual void CollapseFrame();
  virtual void ExpandFrame();
  virtual int IsFrameCollapsed();

  // Description:
  // Set/Get if the frame can be collapsed, i.e. display a button that will
  // let the user collapse the frame. On by default.
  vtkSetMacro(AllowFrameToCollapse, int);
  vtkBooleanMacro(AllowFrameToCollapse, int);
  vtkGetMacro(AllowFrameToCollapse, int);

  // Description:
  // Globally set/get if the frame can be collapsed.
  // By default it is globally enabled.
  static int GetDefaultAllowFrameToCollapse();
  static void SetDefaultAllowFrameToCollapse(int);
  static void DefaultAllowFrameToCollapseOn()
    { vtkKWFrameWithLabel::SetDefaultAllowFrameToCollapse(1); };
  static void DefaultAllowFrameToCollapseOff()
    { vtkKWFrameWithLabel::SetDefaultAllowFrameToCollapse(0); };

  // Description:
  // Globally override the case of the label to ensure GUI consistency.
  // This will change the label when SetLabelText() is called.
  // Defaults to LabelCaseUppercaseFirst.
  //BTX
  enum
  {
    LabelCaseUserSpecified = 0,
    LabelCaseUppercaseFirst,
    LabelCaseLowercaseFirst
  };
  //ETX
  static void SetDefaultLabelCase(int v);
  static int GetDefaultLabelCase();
  static void SetDefaultLabelCaseToUserSpecified() 
    { vtkKWFrameWithLabel::SetDefaultLabelCase(
      vtkKWFrameWithLabel::LabelCaseUserSpecified);};
  static void SetDefaultLabelCaseToUppercaseFirst() 
    {vtkKWFrameWithLabel::SetDefaultLabelCase(
      vtkKWFrameWithLabel::LabelCaseUppercaseFirst);};
  static void SetDefaultLabelCaseToLowercaseFirst() 
    {vtkKWFrameWithLabel::SetDefaultLabelCase(
      vtkKWFrameWithLabel::LabelCaseLowercaseFirst);};

  // Description:
  // Globally enable or disable the font weight of the label.
  // By default it is set to bold.
  //BTX
  enum
  {
    LabelFontWeightNormal = 0,
    LabelFontWeightBold
  };
  //ETX
  static void SetDefaultLabelFontWeight(int v);
  static int GetDefaultLabelFontWeight();
  static void SetDefaultLabelFontWeightToNormal() 
    { vtkKWFrameWithLabel::SetDefaultLabelFontWeight(
      vtkKWFrameWithLabel::LabelFontWeightNormal);};
  static void SetDefaultLabelFontWeightToBold() 
    {vtkKWFrameWithLabel::SetDefaultLabelFontWeight(
      vtkKWFrameWithLabel::LabelFontWeightBold);};

  // Description:
  // Set/Get the visibility of a special icon (lock) when the application
  // is in Limited Edition Mode and the label frame is disabled.
  virtual void SetLimitedEditionModeIconVisibility(int);
  vtkBooleanMacro(LimitedEditionModeIconVisibility, int);
  vtkGetMacro(LimitedEditionModeIconVisibility, int);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Get the drag and drop framework.
  // Override the superclass to allow the frame to be dragged using
  // the label.
  virtual vtkKWDragAndDropTargetSet* GetDragAndDropTargetSet();

  // Description:
  // Callback
  // Ask the frame to readjust its tops margin according to the content of
  // the LabelFrame. This method if bound to a "Configure" event, so
  // the widget should adjust itself automatically most of the time.
  virtual void AdjustMarginCallback();
  virtual void CollapseButtonCallback();
 
protected:

  vtkKWFrameWithLabel();
  ~vtkKWFrameWithLabel();

  vtkKWFrame          *Frame;
  vtkKWFrame          *LabelFrame;
  vtkKWLabelWithLabel *Label;
  vtkKWFrame          *Border;
  vtkKWFrame          *Border2;
  vtkKWFrame          *Groove;
  vtkKWLabel          *Icon;
  vtkKWIcon           *IconData;

  int AllowFrameToCollapse;
  int LimitedEditionModeIconVisibility;

  static int DefaultAllowFrameToCollapse;
  static int DefaultLabelFontWeight;
  static int DefaultLabelCase;

  virtual vtkKWLabel *GetLabelIcon();

private:
  vtkKWFrameWithLabel(const vtkKWFrameWithLabel&); // Not implemented
  void operator=(const vtkKWFrameWithLabel&); // Not implemented
};

#endif

