#include "vtkKWEvent.h"

int main()
{
  int res = 0;
  const char *event = vtkKWEvent::GetStringFromEventId(2001);
  if (!event || strcmp("MessageDialogInvokeEvent", event))
    {
    cout << "Problem with vtkKWEvent::GetStringFromEventId. Requested:"
         << 2001 << "(MessageDialogInvokeEvent) got: " 
         << event << endl;
    res = 1;
    }
  return res;
}
