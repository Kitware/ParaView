/*=========================================================================

  Module:    vtkKWSimpleAnimationWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWSimpleAnimationWidget - a simple animation widget
// .SECTION Description
// This widget provides some simple controls and means to create an
// animation for either a 3D or a 2D scene. It supports two animation
// type. The first one, 'Camera' provides a user interface to rotate the
// camera in the scene. The second one, 'Slice' provides a different user
// interface to slice through a volume for example (say, display all the
// slices along the saggital axis of a medical dataset). No explicit reference
// is made to the dataset, but callbacks must be set so that this widget
// can set or get the slice value on the approriate external resource.

#ifndef __vtkKWSimpleAnimationWidget_h
#define __vtkKWSimpleAnimationWidget_h

#include "vtkKWCompositeWidget.h"

class vtkKWLabelWithLabel;
class vtkKWPushButtonSet;
class vtkKWScaleWithEntrySet;
class vtkKWRenderWidget;

class KWWIDGETS_EXPORT vtkKWSimpleAnimationWidget : public vtkKWCompositeWidget
{
public:
  static vtkKWSimpleAnimationWidget* New();
  vtkTypeRevisionMacro(vtkKWSimpleAnimationWidget, vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the renderwidget to perform the animation on
  vtkGetObjectMacro(RenderWidget, vtkKWRenderWidget);
  virtual void SetRenderWidget(vtkKWRenderWidget*);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);
  
  // Description:
  // Set/Get the animation type.
  // If set to 'camera', the widget will display controls to rotate the
  // camera only the 3-axes.
  // If set to 'slice', the widget will display controls to iterate over
  // a range of slice. It is meant to actually slice through a 3D volume.
  // This 'slice' modes requires several callbacks to be also defined.
  //BTX
  enum 
  {
    AnimationTypeCamera = 0,
    AnimationTypeSlice
  };
  //ETX
  virtual void SetAnimationType(int);
  vtkGetMacro(AnimationType, int);
  virtual void SetAnimationTypeToCamera()
    { this->SetAnimationType(
      vtkKWSimpleAnimationWidget::AnimationTypeCamera); };
  virtual void SetAnimationTypeToSlice()
    { this->SetAnimationType(
      vtkKWSimpleAnimationWidget::AnimationTypeSlice); };

  // Description:
  // Set the command to invoke to set the slice value on an external
  // object when the animation is in 'slice' mode. This command is passed
  // an int (the slice value).
  // This command is mandatory for the slice animation to work.
  virtual void SetSliceSetCommand(vtkObject *object, const char *method);
  virtual void InvokeSliceSetCommand(int);

  // Description:
  // Set the command to invoke to get the slice value from an external
  // object when the animation is in 'slice' mode. This command should return
  // an int (the slice value).
  // This command is optional for the slice animation to work but will
  // guarantee that the slice is set back to its proper value once
  // the animation has been performed.
  virtual void SetSliceGetCommand(vtkObject *object, const char *method);
  virtual int InvokeSliceGetCommand();

  // Description:
  // Set the command to invoke to get the minimum and maximum value of the
  // slice range from an external object when the animation is in 'slice' mode.
  // These commands should return an int (the min and max).
  // These commands are mandatory for the slice animation to work.
  virtual void SetSliceGetMinCommand(vtkObject *object, const char *method);
  virtual int InvokeSliceGetMinCommand();
  virtual void SetSliceGetMaxCommand(vtkObject *object, const char *method);
  virtual int InvokeSliceGetMaxCommand();

  // Description:
  // Set a command to be invoked after the slice animation has been
  // created/previewed
  // This command is optional.
  virtual void SetSlicePostAnimationCommand(
    vtkObject *object, const char *method);
  virtual void InvokeSlicePostAnimationCommand();

  // Description:
  // Set a command to be invoked after the camera animation has been
  // created/previewed
  // This command is optional.
  virtual void SetCameraPostAnimationCommand(
    vtkObject *object, const char *method);
  virtual void InvokeCameraPostAnimationCommand();

  // Description:
  // Update the whole UI depending on the value of the Ivars
  virtual void Update();

  // Description:
  // Callbacks
  virtual void PreviewAnimationCallback();
  virtual void CreateAnimationCallback();
  virtual void CancelAnimationCallback();
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWSimpleAnimationWidget();
  ~vtkKWSimpleAnimationWidget();
  
  vtkKWRenderWidget *RenderWidget;

  // GUI

  vtkKWScaleWithEntrySet *Parameters;
  vtkKWPushButtonSet     *AnimationButtonSet;
  vtkKWLabelWithLabel    *HelpLabel;

  int AnimationType;

  // Description:
  // Animation status
  //BTX
  enum
  {
    AnimationStopped = 0,
    AnimationPreviewing,
    AnimationCreating,
    AnimationCancelled
  };
  //ETX
  int AnimationStatus;

  char *CameraPostAnimationCommand;
  char *SlicePostAnimationCommand;
  char *SliceGetCommand;
  char *SliceGetMinCommand;
  char *SliceGetMaxCommand;
  char *SliceSetCommand;
  
  // Description:
  // Preview and create camera animation
  virtual void PreviewCameraAnimation();
  virtual void CreateCameraAnimation(
    const char *file_root, const char *ext, int width, int height);
  virtual void PerformCameraAnimation(
    const char *file_root, const char *ext, int width, int height);
  
  // Description:
  // Preview and create slice animation
  virtual void PreviewSliceAnimation();
  virtual void CreateSliceAnimation(
    const char *file_root, const char *ext, int width, int height);
  virtual void PerformSliceAnimation(
    const char *file_root, const char *ext, int width, int height);

  // Description:
  // Enable/disable animation buttons
  virtual void DisableButtonsButCancel();
  virtual void EnableButtonsButCancel();

private:
  vtkKWSimpleAnimationWidget(const vtkKWSimpleAnimationWidget&);  // Not implemented
  void operator=(const vtkKWSimpleAnimationWidget&);  // Not implemented
};

#endif

