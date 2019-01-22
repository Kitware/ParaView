#include "vtkPVConfig.h"

#include "vtkClientServerInterpreterInitializer.h"
#include "vtkCommandOptions.h"
#include "vtkCommandOptionsXMLParser.h"
#include "vtkPVTestUtilities.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkStringList.h"

#define PRINT_SELF(classname)                                                                      \
  c = classname::New();                                                                            \
  c->Print(cout);                                                                                  \
  c->Delete();

int ParaViewCoreCorePrintSelf(int, char* [])
{
  vtkObject* c;
  PRINT_SELF(vtkCommandOptions);
  PRINT_SELF(vtkCommandOptionsXMLParser);
  PRINT_SELF(vtkPVTestUtilities);
  PRINT_SELF(vtkPVXMLElement);
  PRINT_SELF(vtkPVXMLParser);
  PRINT_SELF(vtkStringList);
  return 0;
}
