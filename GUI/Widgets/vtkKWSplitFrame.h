/*=========================================================================

  Module:    vtkKWSplitFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWSplitFrame - A Frame that contains two adjustable sub frames.
// .SECTION Description
// The split frame allows the use to select the size of the two frames.
// It uses a separator that can be dragged interactively.


#ifndef __vtkKWSplitFrame_h
#define __vtkKWSplitFrame_h

#include "vtkKWCompositeWidget.h"
class vtkKWFrame;

class KWWidgets_EXPORT vtkKWSplitFrame : public vtkKWCompositeWidget
{
public:
  static vtkKWSplitFrame* New();
  vtkTypeRevisionMacro(vtkKWSplitFrame,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();
  
  // Description:
  // Get Frame1. In horizontal orientation, this is the one on the left of the
  // separator. In vertical orientation, the one at the bottom.
  // Both Frame1 and Frame2 position can be swapped using the SetFrameLayout
  // method.
  vtkKWFrame *GetFrame1() {return this->Frame1;};

  // Description:
  // Get Frame2. In horizontal orientation, this is the one on the right of the
  // separator. In vertical orientation, the one at the top.
  // Both Frame1 and Frame2 position can be swapped using the SetFrameLayout
  // method.
  vtkKWFrame *GetFrame2() {return this->Frame2;};

  // Description:
  // Set/Get the orientation of the split frame.
  // If horizontal, Frame1 is on the left of the separator, Frame2 on the
  // right. If Vertical, Frame1 is below the separator, Frame2 is on top.
  // Both Frame1 and Frame2 position can be swapped using the SetFrameLayout
  // method.
  //BTX
  enum 
  {
    OrientationHorizontal = 0,
    OrientationVertical
  };
  //ETX
  virtual void SetOrientation(int);
  vtkGetMacro(Orientation, int);
  virtual void SetOrientationToHorizontal()
    { this->SetOrientation(vtkKWSplitFrame::OrientationHorizontal); };
  virtual void SetOrientationToVertical()
    { this->SetOrientation(vtkKWSplitFrame::OrientationVertical); };

  // Description:
  // Set/Get the frame layout.
  // If set to Default, depending on the orientation, Frame1 is on the left
  // (respectively bottom) of the separator, Frame2 on the right (top).
  // If set to Swapped, Frame1 and Frame2 position are exchanged.
  //BTX
  enum 
  {
    FrameLayoutDefault = 0,
    FrameLayoutSwapped
  };
  //ETX
  virtual void SetFrameLayout(int);
  vtkGetMacro(FrameLayout, int);
  virtual void SetFrameLayoutToDefault()
    { this->SetFrameLayout(vtkKWSplitFrame::FrameLayoutDefault); };
  virtual void SetFrameLayoutToSwapped()
    { this->SetFrameLayout(vtkKWSplitFrame::FrameLayoutSwapped); };

  // Description:
  // Set/Get which frame is automatically expanded when the whole widget
  // is resized. By default, Frame2 (i.e. right or top frame)
  //BTX
  enum 
  {
    ExpandableFrame1 = 0,
    ExpandableFrame2,
    ExpandableFrameBoth
  };
  //ETX
  vtkSetClampMacro(ExpandableFrame, int, 
                   vtkKWSplitFrame::ExpandableFrame1, 
                   vtkKWSplitFrame::ExpandableFrameBoth);
  vtkGetMacro(ExpandableFrame, int);
  virtual void SetExpandableFrameToFrame1()
    { this->SetExpandableFrame(vtkKWSplitFrame::ExpandableFrame1); };
  virtual void SetExpandableFrameToFrame2()
    { this->SetExpandableFrame(vtkKWSplitFrame::ExpandableFrame2); };
  virtual void SetExpandableFrameToBothFrames()
    { this->SetExpandableFrame(vtkKWSplitFrame::ExpandableFrameBoth); };

  // Description:
  // Set/Get The minimum size, size and visibility of Frame1.
  vtkGetMacro(Frame1MinimumSize, int);
  virtual void SetFrame1MinimumSize(int minSize);
  vtkGetMacro(Frame1Size, int);
  virtual void SetFrame1Size(int size);
  vtkGetMacro(Frame1Visibility, int);
  virtual void SetFrame1Visibility(int flag);
  vtkBooleanMacro(Frame1Visibility, int);  

  // Description:
  // Set/Get The minimum size, size and visibility of Frame2.
  vtkGetMacro(Frame2MinimumSize, int);
  virtual void SetFrame2MinimumSize(int minSize);
  vtkGetMacro(Frame2Size, int);
  virtual void SetFrame2Size(int size);
  vtkGetMacro(Frame2Visibility, int);
  virtual void SetFrame2Visibility(int flag);
  vtkBooleanMacro(Frame2Visibility, int);  

  // Description:
  // Set the size of both frames by setting the relative position of the
  // separator. This will not work until the widget has been mapped on screen.
  virtual void SetSeparatorPosition(double pos);
  virtual double GetSeparatorPosition();

  // Description:
  // Set/Get the separator narrow dimension.
  // If the size is 0, then the two frames cannot be adjusted by the user.
  virtual void SetSeparatorSize(int size);
  vtkGetMacro(SeparatorSize, int);

  // Description:
  // Set/Get the separator narrow margin, i.e. the empty space around the
  // separator itself.
  virtual void SetSeparatorMargin(int size);
  vtkGetMacro(SeparatorMargin, int);

  // Description:
  // Set/Get the visibility of the separator.  
  virtual void SetSeparatorVisibility(int flag);
  vtkGetMacro(SeparatorVisibility, int);
  vtkBooleanMacro(SeparatorVisibility, int);  

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Callbacks. Internal, do not use.
  virtual void DragCallback();
  virtual void ConfigureCallback();

protected:
  vtkKWSplitFrame();
  ~vtkKWSplitFrame();

  virtual int GetTotalSeparatorSize();

  vtkKWFrame *Frame1;
  vtkKWFrame *Separator;
  vtkKWFrame *Frame2;

  int Size;
  int Frame1Size;
  int Frame2Size;

  int SeparatorSize;
  int SeparatorMargin;
  int SeparatorVisibility;

  int Frame1Visibility;
  int Frame2Visibility;

  int Frame1MinimumSize;
  int Frame2MinimumSize;

  int Orientation;
  int FrameLayout;
  int ExpandableFrame;

  // Reset the actual windows to match our size IVars.

  virtual void Pack();
  virtual void AddBindings();
  virtual void RemoveBindings();
  virtual void AddSeparatorBindings();
  virtual void RemoveSeparatorBindings();
  virtual void ReConfigure();
  virtual int GetInternalMarginHorizontal();
  virtual int GetInternalMarginVertical();
  virtual void ConfigureSeparatorCursor();

private:
  vtkKWSplitFrame(const vtkKWSplitFrame&); // Not implemented
  void operator=(const vtkKWSplitFrame&); // Not implemented
};


#endif



