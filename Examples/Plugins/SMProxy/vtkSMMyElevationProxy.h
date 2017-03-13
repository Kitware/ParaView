
#ifndef vtkSMMyElevationProxy_h
#define vtkSMMyElevationProxy_h

#include "vtkSMSourceProxy.h"

/// useless but just showing we can do it
class vtkSMMyElevationProxy : public vtkSMSourceProxy
{
public:
  static vtkSMMyElevationProxy* New();
  vtkTypeMacro(vtkSMMyElevationProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkSMMyElevationProxy();
  ~vtkSMMyElevationProxy();
};

#endif // vtkSMMyElevationProxy_h
