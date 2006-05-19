#ifndef __vtkKWMyWidget_h
#define __vtkKWMyWidget_h

#include "vtkKWCompositeWidget.h"
#include "vtkKWMyApplicationWin32Header.h"

class vtkKWScale;

class KWMyApplication_EXPORT vtkKWMyWidget : public vtkKWCompositeWidget
{
public:
  static vtkKWMyWidget* New();
  vtkTypeRevisionMacro(vtkKWMyWidget,vtkKWCompositeWidget);

  // Description:
  // Callbacks
  virtual void ScaleChangeNotifiedByCommandCallback(double value);

protected:
  vtkKWMyWidget();
  ~vtkKWMyWidget();

  vtkKWScale *Scale;

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  // Description:
  // Processes the events that are passed through CallbackCommand (or others).
  virtual void ProcessCallbackCommandEvents(
    vtkObject *caller, unsigned long event, void *calldata);

private:
  vtkKWMyWidget(const vtkKWMyWidget&);   // Not implemented.
  void operator=(const vtkKWMyWidget&);  // Not implemented.
};

#endif
