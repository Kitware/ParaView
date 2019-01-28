
#ifndef vtkSMMyBoundsDomain_h
#define vtkSMMyBoundsDomain_h

#include "MyProxyModule.h" // for export macro
#include "vtkSMBoundsDomain.h"

/// useless but just showing we can do it
class MYPROXY_EXPORT vtkSMMyBoundsDomain : public vtkSMBoundsDomain
{
public:
  static vtkSMMyBoundsDomain* New();
  vtkTypeMacro(vtkSMMyBoundsDomain, vtkSMBoundsDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // overload setting of default values so we can orient them
  // our way
  void Update(vtkSMProperty* prop) override;

protected:
  vtkSMMyBoundsDomain();
  ~vtkSMMyBoundsDomain();
};

#endif // vtkSMMyBoundsDomain_h
