/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQPlaneSourceConfigurationReader - A reader for XML camera configuration.
//
// .SECTION Description
// A reader for XML camera configuration. Reades camera configuration files.
// writen by the vtkSQPlaneSourceConfigurationWriter.
//
// .SECTION See Also
// vtkSQPlaneSourceConfigurationWriter, vtkSMProxyConfigurationReader
//
// .SECTION Thanks
// This class was contributed by SciberQuest Inc.
#ifndef __vtkSQPlaneSourceConfigurationReader_h
#define __vtkSQPlaneSourceConfigurationReader_h

#include "vtkSMProxyConfigurationReader.h"

class vtkSMProxy;
class vtkSMRenderViewProxy;
class vtkPVXMLElement;

class VTK_EXPORT vtkSQPlaneSourceConfigurationReader : public vtkSMProxyConfigurationReader
{
public:
  vtkTypeMacro(vtkSQPlaneSourceConfigurationReader,vtkSMProxyConfigurationReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQPlaneSourceConfigurationReader *New();


  // Description:
  // Read the named file, and push the properties into the underying
  // managed render view proxy. This will make sure the renderview is
  // updated after the read.
  virtual int ReadConfiguration(const char *filename);
  virtual int ReadConfiguration(vtkPVXMLElement *x);
  // unhide
  virtual int ReadConfiguration()
    {
    return this->Superclass::ReadConfiguration();
    }

protected:
  vtkSQPlaneSourceConfigurationReader();
  virtual ~vtkSQPlaneSourceConfigurationReader();

private:
  vtkSQPlaneSourceConfigurationReader(const vtkSQPlaneSourceConfigurationReader&);  // Not implemented.
  void operator=(const vtkSQPlaneSourceConfigurationReader&);  // Not implemented.
};

#endif
