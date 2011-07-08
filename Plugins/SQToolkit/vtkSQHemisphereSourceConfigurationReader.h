/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
// .NAME vtkSQHemisphereSourceConfigurationReader - A reader for XML camera configuration.
//
// .SECTION Description
// A reader for XML camera configuration. Reades camera configuration files.
// writen by the vtkSQHemisphereSourceConfigurationWriter.
//
// .SECTION See Also
// vtkSQHemisphereSourceConfigurationWriter, vtkSMProxyConfigurationReader
//
// .SECTION Thanks
// This class was contributed by SciberQuest Inc.
#ifndef __vtkSQHemisphereSourceConfigurationReader_h
#define __vtkSQHemisphereSourceConfigurationReader_h

#include "vtkSMProxyConfigurationReader.h"

class vtkSMProxy;
class vtkSMRenderViewProxy;
class vtkPVXMLElement;

class VTK_EXPORT vtkSQHemisphereSourceConfigurationReader : public vtkSMProxyConfigurationReader
{
public:
  vtkTypeRevisionMacro(vtkSQHemisphereSourceConfigurationReader,vtkSMProxyConfigurationReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQHemisphereSourceConfigurationReader *New();


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
  vtkSQHemisphereSourceConfigurationReader();
  virtual ~vtkSQHemisphereSourceConfigurationReader();

private:
  vtkSQHemisphereSourceConfigurationReader(const vtkSQHemisphereSourceConfigurationReader&);  // Not implemented.
  void operator=(const vtkSQHemisphereSourceConfigurationReader&);  // Not implemented.
};

#endif

