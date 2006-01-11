#ifndef __vtkKWMedicalImageViewerExample_h
#define __vtkKWMedicalImageViewerExample_h

#include "vtkKWObject.h"

class vtkKWRenderWidget;
class vtkImageViewer2;
class vtkKWScale;
class vtkKWWindowLevelPresetSelector;

class vtkKWMedicalImageViewerExample : public vtkKWObject
{
public:
  static vtkKWMedicalImageViewerExample* New();
  vtkTypeRevisionMacro(vtkKWMedicalImageViewerExample,vtkKWObject);

  // Description:
  // Run the example.
  int Run(int argc, char *argv[]);

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
  vtkKWMedicalImageViewerExample() {};
  ~vtkKWMedicalImageViewerExample() {};

  vtkImageViewer2                *ImageViewer; 
  vtkKWScale                     *SliceScale;
  vtkKWWindowLevelPresetSelector *WindowLevelPresetSelector;
  vtkKWRenderWidget              *RenderWidget;

  virtual void UpdateSliceScale();

private:
  vtkKWMedicalImageViewerExample(const vtkKWMedicalImageViewerExample&);   // Not implemented.
  void operator=(const vtkKWMedicalImageViewerExample&);  // Not implemented.
};

#endif
