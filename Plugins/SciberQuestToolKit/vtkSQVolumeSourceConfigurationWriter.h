/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQVolumeSourceConfigurationWriter - A writer for XML camera configuration.
//
// .SECTION Description
// A writer for XML camera configuration. Writes camera configuration files
// using ParaView state file machinery.
//
// .SECTION See Also
// vtkSQVolumeSourceConfigurationReader, vtkSMProxyConfigurationWriter
//
// .SECTION Thanks
// This class was contributed by SciberQuest Inc.
#ifndef __vtkSQVolumeSourceConfigurationWriter_h
#define __vtkSQVolumeSourceConfigurationWriter_h

#include "vtkSMProxyConfigurationWriter.h"

class vtkSMRenderViewProxy;
class vtkSMProxy;

class VTK_EXPORT vtkSQVolumeSourceConfigurationWriter : public vtkSMProxyConfigurationWriter
{
public:
  vtkTypeMacro(vtkSQVolumeSourceConfigurationWriter,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQVolumeSourceConfigurationWriter *New();

  // Description:
  // Override sets iterator proxy.
  virtual void SetProxy(vtkSMProxy *proxy);

protected:
  vtkSQVolumeSourceConfigurationWriter();
  ~vtkSQVolumeSourceConfigurationWriter();

private:
  vtkSQVolumeSourceConfigurationWriter(const vtkSQVolumeSourceConfigurationWriter&);  // Not implemented.
  void operator=(const vtkSQVolumeSourceConfigurationWriter&);  // Not implemented.
};

#endif
