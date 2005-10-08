#ifndef __vtkKWSimpleWindowWithImageWidgetExample_h
#define __vtkKWSimpleWindowWithImageWidgetExample_h

#include "vtkKWObject.h"

class vtkKWRenderWidget;
class vtkImageViewer2;
class vtkKWScale;
class vtkKWWindowLevelPresetSelector;

class vtkKWSimpleWindowWithImageWidgetExample : public vtkKWObject
{
public:
  static vtkKWSimpleWindowWithImageWidgetExample* New();
  vtkTypeRevisionMacro(vtkKWSimpleWindowWithImageWidgetExample,vtkKWObject);

  // Description:
  // Run the example.
  int Run(int argc, char *argv[]);

  // Description:
  // Callbacks
  virtual void SetSliceFromScaleCallback();
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
  vtkKWSimpleWindowWithImageWidgetExample() {};
  ~vtkKWSimpleWindowWithImageWidgetExample() {};

  vtkImageViewer2                *ImageViewer; 
  vtkKWScale                     *SliceScale;
  vtkKWWindowLevelPresetSelector *WindowLevelPresetSelector;
  vtkKWRenderWidget              *RenderWidget;

  virtual void UpdateSliceScale();

private:
  vtkKWSimpleWindowWithImageWidgetExample(const vtkKWSimpleWindowWithImageWidgetExample&);   // Not implemented.
  void operator=(const vtkKWSimpleWindowWithImageWidgetExample&);  // Not implemented.
};

#endif
