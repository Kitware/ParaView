#include "vtkKWProcessStatistics.h"

int main()
{
  vtkKWProcessStatistics *pr = vtkKWProcessStatistics::New();

  float dev = 1024.0 * 1024.0; // Let's display in MB.  
  cout 
    << "Total physical: " << pr->GetTotalPhysicalMemory() / dev << endl
    << "     Available: " << pr->GetAvailablePhysicalMemory() / dev << endl
    << " Total virtual: " << pr->GetTotalVirtualMemory() / dev << endl
    << "     Available: " << pr->GetAvailableVirtualMemory() / dev << endl;
  
  pr->Delete();
  return 0;
}
