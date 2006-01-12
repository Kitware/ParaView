#ifndef __vtkKWMyApplication_h
#define __vtkKWMyApplication_h

#include "vtkKWApplication.h"

class vtkKWMyApplication : public vtkKWApplication
{
public:
  static vtkKWMyApplication* New();
  vtkTypeRevisionMacro(vtkKWMyApplication,vtkKWApplication);

  // Description:
  // Set/Get a value
  vtkGetMacro(MyValue, double);
  virtual void SetMyValue(double);
 
protected:
  vtkKWMyApplication();
  ~vtkKWMyApplication() {};

  double MyValue;

private:
  vtkKWMyApplication(const vtkKWMyApplication&);   // Not implemented.
  void operator=(const vtkKWMyApplication&);  // Not implemented.
};

#endif
