#include "vtkKWEvent.h"
#include "vtkString.h"

int main()
{
  int res = 0;
  if ( !vtkString::Equals("MessageDialogInvokeEvent",
                          vtkKWEvent::GetStringFromEventId(2001)) )
    {
    cout << "Problem with vtkKWEvent::GetStringFromEventId. Reequested:"
         << 2001 << "(MessageDialogInvokeEvent) got: " 
         << vtkKWEvent::GetStringFromEventId(2001) << endl;
    res = 1;
    }
  return res;
}
