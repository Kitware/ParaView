#include "vtkKWRemoteExecute.h"

#ifndef _WIN32
#include <unistd.h>
#endif

int main()
{
  const char* args = "/usr/bin/find /usr -type f -name \\*andy\\*";
  vtkKWRemoteExecute* re = vtkKWRemoteExecute::New();
  re->SetRemoteHost("public");
  re->RunRemoteCommand(args);
  while( re->GetResult() == vtkKWRemoteExecute::RUNNING )
    {
    cout << "Waiting" << endl;
#ifdef _WIN32
    Sleep(1000);
#else
    sleep(1);
#endif
    }
  re->WaitToFinish();
  
  re->Delete();
  return 0;
}
