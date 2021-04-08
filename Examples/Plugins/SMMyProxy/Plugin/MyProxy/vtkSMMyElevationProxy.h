
#ifndef vtkSMMyElevationProxy_h
#define vtkSMMyElevationProxy_h

#include "MyProxyModule.h" // for export macro
#include "vtkSMSourceProxy.h"

/// useless but just showing we can do it
class MYPROXY_EXPORT vtkSMMyElevationProxy : public vtkSMSourceProxy
{
public:
  static vtkSMMyElevationProxy* New();
  vtkTypeMacro(vtkSMMyElevationProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSMMyElevationProxy(const vtkSMMyElevationProxy&) = delete;
  void operator=(const vtkSMMyElevationProxy&) = delete;

protected:
  vtkSMMyElevationProxy();
  ~vtkSMMyElevationProxy();
};

#endif // vtkSMMyElevationProxy_h
