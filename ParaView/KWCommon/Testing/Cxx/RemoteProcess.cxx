#include "vtkKWRemoteExecute.h"

int main()
{
  const char* args[] = {
    "find",
    "/usr",
    "-type",
    "f",
    0
  };
  vtkKWRemoteExecute* re = vtkKWRemoteExecute::New();
  re->SetRemoteHost("public");
  re->RunRemoteCommand("/usr/bin/find", args);
  
  re->Delete();
  return 0;
}
