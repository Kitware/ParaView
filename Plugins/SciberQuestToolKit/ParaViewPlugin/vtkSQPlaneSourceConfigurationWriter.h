/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQPlaneSourceConfigurationWriter - A writer for XML camera configuration.
//
// .SECTION Description
// A writer for XML camera configuration. Writes camera configuration files
// using ParaView state file machinery.
//
// .SECTION See Also
// vtkSQPlaneSourceConfigurationReader, vtkSMProxyConfigurationWriter
//
// .SECTION Thanks
// This class was contributed by SciberQuest Inc.
#ifndef __vtkSQPlaneSourceConfigurationWriter_h
#define __vtkSQPlaneSourceConfigurationWriter_h

#include "vtkSMProxyConfigurationWriter.h"

class vtkSMRenderViewProxy;
class vtkSMProxy;

class VTK_EXPORT vtkSQPlaneSourceConfigurationWriter : public vtkSMProxyConfigurationWriter
{
public:
  vtkTypeMacro(vtkSQPlaneSourceConfigurationWriter,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQPlaneSourceConfigurationWriter *New();

  // Description:
  // Override sets iterator proxy.
  virtual void SetProxy(vtkSMProxy *proxy);

protected:
  vtkSQPlaneSourceConfigurationWriter();
  ~vtkSQPlaneSourceConfigurationWriter();

private:
  vtkSQPlaneSourceConfigurationWriter(const vtkSQPlaneSourceConfigurationWriter&);  // Not implemented.
  void operator=(const vtkSQPlaneSourceConfigurationWriter&);  // Not implemented.
};

#endif
