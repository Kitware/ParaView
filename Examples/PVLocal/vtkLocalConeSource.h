/*=========================================================================
This source has no copyright.  It is intended to be copied by users
wishing to create their own ParaView plugin classes locally.
=========================================================================*/
// .NAME vtkLocalConeSource - Example ParaView Plugin Source
// .SECTION Description
// vtkLocalConeSource is a subclass of vtkConeSource that adds no
// functionality but is sufficient to show an example ParaView plugin.

#ifndef __vtkLocalConeSource_h
#define __vtkLocalConeSource_h

#include "vtkConeSource.h"

#include "vtkPVLocalConfigure.h" // VTK_PVLocal_EXPORT

class VTK_PVLocal_EXPORT vtkLocalConeSource : public vtkConeSource
{
public:
  vtkTypeRevisionMacro(vtkLocalConeSource,vtkConeSource);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkLocalConeSource* New();

protected:
  vtkLocalConeSource();
  ~vtkLocalConeSource();

private:
  vtkLocalConeSource(const vtkLocalConeSource&);  // Not implemented.
  void operator=(const vtkLocalConeSource&);  // Not implemented.
};

#endif
