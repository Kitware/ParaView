
#ifndef __vtkSMPrismDoubleRangeDomain_h
#define __vtkSMPrismDoubleRangeDomain_h

#include "vtkSMDoubleRangeDomain.h"

//BTX
struct vtkSMPrismDoubleRangeDomainInternals;
//ETX

class VTK_EXPORT vtkSMPrismDoubleRangeDomain : public vtkSMDoubleRangeDomain
{
public:
  static vtkSMPrismDoubleRangeDomain* New();
  vtkTypeMacro(vtkSMPrismDoubleRangeDomain, vtkSMDoubleRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Update self checking the "unchecked" values of all required
  // properties. Overwritten by sub-classes.
  virtual void Update(vtkSMProperty*);
    virtual int SetDefaultValues(vtkSMProperty*);

protected:
  vtkSMPrismDoubleRangeDomain();
  ~vtkSMPrismDoubleRangeDomain();


  vtkSMPrismDoubleRangeDomainInternals* DRInternals;

private:
  vtkSMPrismDoubleRangeDomain(const vtkSMPrismDoubleRangeDomain&); // Not implemented
  void operator=(const vtkSMPrismDoubleRangeDomain&); // Not implemented
};

#endif
