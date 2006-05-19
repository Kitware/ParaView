#ifndef __vtkKWMyWindow_h
#define __vtkKWMyWindow_h

#include "vtkKWWindow.h"

class vtkKWRenderWidget;
class vtkImageViewer2;
class vtkKWScale;
class vtkKWWindowLevelPresetSelector;
class vtkKWSimpleAnimationWidget;

class vtkKWMyWindow : public vtkKWWindow
{
public:
  static vtkKWMyWindow* New();
  vtkTypeRevisionMacro(vtkKWMyWindow,vtkKWWindow);

  // Description:
  // Callbacks
  virtual void SetSliceFromScaleCallback(double value);
  virtual void SetSliceCallback(int slice);
  virtual int  GetSliceCallback();
  virtual int  GetSliceMinCallback();
  virtual int  GetSliceMaxCallback();
  virtual void SetSliceOrientationToXYCallback();
  virtual void SetSliceOrientationToXZCallback();
  virtual void SetSliceOrientationToYZCallback();
  virtual void WindowLevelPresetApplyCallback(int id);
  virtual void WindowLevelPresetAddCallback();
  virtual void WindowLevelPresetUpdateCallback(int id);
  virtual void WindowLevelPresetHasChangedCallback(int id);

protected:
  vtkKWMyWindow();
  ~vtkKWMyWindow();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  vtkImageViewer2                *ImageViewer; 
  vtkKWScale                     *SliceScale;
  vtkKWWindowLevelPresetSelector *WindowLevelPresetSelector;
  vtkKWRenderWidget              *RenderWidget;
  vtkKWSimpleAnimationWidget     *AnimationWidget;

  virtual void UpdateSliceRanges();

private:
  vtkKWMyWindow(const vtkKWMyWindow&);   // Not implemented.
  void operator=(const vtkKWMyWindow&);  // Not implemented.
};

#endif
