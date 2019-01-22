#include "vtkPVConfig.h"

#include "vtkPythonCalculator.h"
#include "vtkPythonProgrammableFilter.h"

#define PRINT_SELF(classname)                                                                      \
  cout << "------------------------------------" << endl;                                          \
  cout << "Class: " << #classname << endl;                                                         \
  c = classname::New();                                                                            \
  c->Print(cout);                                                                                  \
  c->Delete();

int ParaViewCoreClientServerPythonPrintSelf(int, char* [])
{
  vtkObject* c;

  PRINT_SELF(vtkPythonCalculator);
  PRINT_SELF(vtkPythonProgrammableFilter);

  return 0;
}
