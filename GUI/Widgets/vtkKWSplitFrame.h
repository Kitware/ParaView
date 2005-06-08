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

#include "vtkKWWidget.h"
class vtkKWApplication;
class vtkKWFrame;

class KWWIDGETS_EXPORT vtkKWSplitFrame : public vtkKWWidget
{
public:
  static vtkKWSplitFrame* New();
  vtkTypeRevisionMacro(vtkKWSplitFrame,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app);
  
  // Description:
  // Get the the left/bottom internal frame.
  vtkKWFrame *GetFrame1() {return this->Frame1;};

  // Description:
  // Get the the right/top internal frame.
  vtkKWFrame *GetFrame2() {return this->Frame2;};

  // Description:
  // Horizontal orientation has first frame to left of second.
  // Vertical orientation has the first frame at the bottom of 
  // the second frame.
  // At the moment, this state has to be set before the
  // widget is created.  I should extend it in the future to be more flexible.
  //BTX
  enum 
  {
    Horizontal = 0,
    Vertical
  };
  //ETX
  vtkSetMacro(Orientation, int);
  vtkGetMacro(Orientation, int);
  virtual void SetOrientationToHorizontal()
    { this->SetOrientation(vtkKWSplitFrame::Horizontal); };
  virtual void SetOrientationToVertical()
    { this->SetOrientation(vtkKWSplitFrame::Vertical); };

  // Description:
  // Set/Get which frame is automatically expanded when the whole widget
  // is resized. By default, Frame2 (i.e. right or top frame)
  //BTX
  enum 
  {
    ExpandFrame1 = 0,
    ExpandFrame2
  };
  //ETX
  vtkSetMacro(ExpandFrame, int);
  vtkGetMacro(ExpandFrame, int);
  virtual void SetExpandFrameToFrame1()
    { this->SetExpandFrame(vtkKWSplitFrame::ExpandFrame1); };
  virtual void SetExpandFrameToFrame2()
    { this->SetExpandFrame(vtkKWSplitFrame::ExpandFrame2); };

  // Description:
  // Set/Get The minimum size for the two frames.
  vtkGetMacro(Frame1MinimumSize, int);
  vtkGetMacro(Frame2MinimumSize, int);
  virtual void SetFrame1MinimumSize(int minSize);
  virtual void SetFrame2MinimumSize(int minSize);

  // Description:
  // Set/Get the size of the frames.  
  vtkGetMacro(Frame1Size, int);
  vtkGetMacro(Frame2Size, int);
  virtual void SetFrame1Size(int size);
  virtual void SetFrame2Size(int size);

  // Description:
  // Set/Get the visibility of the left or right frame.  
  virtual void SetFrame1Visibility(int flag);
  vtkGetMacro(Frame1Visibility, int);
  vtkBooleanMacro(Frame1Visibility, int);  
  virtual void SetFrame2Visibility(int flag);
  vtkGetMacro(Frame2Visibility, int);
  vtkBooleanMacro(Frame2Visibility, int);  

  // Description:
  // This sets the separators narrow dimension.
  // If the size is 0, then the two frames cannot be adjusted by the user.
  virtual void SetSeparatorSize(int size);
  vtkGetMacro(SeparatorSize, int);

  // Description:
  // This sets the separators narrow margin, i.e. the empty space around the
  // separator itself.
  virtual void SetSeparatorMargin(int size);
  vtkGetMacro(SeparatorMargin, int);

  // Callbacks used internally to adjust the size,
  virtual void DragCallback();
  virtual void ConfigureCallback();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

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

  int Frame1Visibility;
  int Frame2Visibility;

  int Frame1MinimumSize;
  int Frame2MinimumSize;

  int Orientation;
  int ExpandFrame;

  // Reset the actual windows to match our size IVars.

  virtual void Update();
  virtual void AddBindings();
  virtual void RemoveBindings();
  virtual void ReConfigure();

private:
  vtkKWSplitFrame(const vtkKWSplitFrame&); // Not implemented
  void operator=(const vtkKWSplitFrame&); // Not implemented
};


#endif



