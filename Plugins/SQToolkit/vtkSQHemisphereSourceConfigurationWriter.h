/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
// .NAME vtkSQHemisphereSourceConfigurationWriter - A writer for XML camera configuration.
//
// .SECTION Description
// A writer for XML camera configuration. Writes camera configuration files
// using ParaView state file machinery.
//
// .SECTION See Also
// vtkSQHemisphereSourceConfigurationReader, vtkSMProxyConfigurationWriter
//
// .SECTION Thanks
// This class was contributed by SciberQuest Inc.
#ifndef __vtkSQHemisphereSourceConfigurationWriter_h
#define __vtkSQHemisphereSourceConfigurationWriter_h

#include "vtkSMProxyConfigurationWriter.h"

class vtkSMRenderViewProxy;
class vtkSMProxy;

class VTK_EXPORT vtkSQHemisphereSourceConfigurationWriter : public vtkSMProxyConfigurationWriter
{
public:
  vtkTypeRevisionMacro(vtkSQHemisphereSourceConfigurationWriter,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQHemisphereSourceConfigurationWriter *New();

  // Description:
  // Override sets iterator proxy.
  virtual void SetProxy(vtkSMProxy *proxy);

protected:
  vtkSQHemisphereSourceConfigurationWriter();
  ~vtkSQHemisphereSourceConfigurationWriter();

private:
  vtkSQHemisphereSourceConfigurationWriter(const vtkSQHemisphereSourceConfigurationWriter&);  // Not implemented.
  void operator=(const vtkSQHemisphereSourceConfigurationWriter&);  // Not implemented.
};

#endif

