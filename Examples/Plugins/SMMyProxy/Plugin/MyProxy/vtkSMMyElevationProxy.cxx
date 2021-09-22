
#include "vtkSMMyElevationProxy.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMMyElevationProxy);

vtkSMMyElevationProxy::vtkSMMyElevationProxy() = default;

vtkSMMyElevationProxy::~vtkSMMyElevationProxy() = default;

void vtkSMMyElevationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
