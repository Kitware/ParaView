#ifndef __vtkKWMyWidget_h
#define __vtkKWMyWidget_h

#include "vtkKWCompositeWidget.h"

class vtkKWScale;

class vtkKWMyWidget : public vtkKWCompositeWidget
{
public:
  static vtkKWMyWidget* New();
  vtkTypeRevisionMacro(vtkKWMyWidget,vtkKWCompositeWidget);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Callbacks
  virtual void ScaleChangeNotifiedByCommandCallback(double value);

protected:
  vtkKWMyWidget();
  ~vtkKWMyWidget();

  vtkKWScale *Scale;

  // Description:
  // Processes the events that are passed through CallbackCommand (or others).
  virtual void ProcessCallbackCommandEvents(
    vtkObject *caller, unsigned long event, void *calldata);

private:
  vtkKWMyWidget(const vtkKWMyWidget&);   // Not implemented.
  void operator=(const vtkKWMyWidget&);  // Not implemented.
};

#endif
