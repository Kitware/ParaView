#include "vtkKWMyApplication.h"

#include "vtkObjectFactory.h"
#include "vtkKWWindowBase.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWMyApplication );
vtkCxxRevisionMacro(vtkKWMyApplication, "1.1");

//----------------------------------------------------------------------------
vtkKWMyApplication::vtkKWMyApplication()
{
  this->MyValue = 0.0;
}

//----------------------------------------------------------------------------
void vtkKWMyApplication::SetMyValue(double value)
{
  if (this->MyValue == value)
    {
    return;
    }

  this->MyValue = value;
  this->Modified(); // generate a ModifiedEvent automatically

  // Just for kicks and debugging purposes, if we have a window registered,
  // display our value

  vtkKWWindowBase *win = this->GetNthWindow(0);
  if (win)
    {
    char buffer[256];
    sprintf(buffer, "MyValue Changed: %g", this->MyValue);
    win->SetStatusText(buffer);
    }
}
