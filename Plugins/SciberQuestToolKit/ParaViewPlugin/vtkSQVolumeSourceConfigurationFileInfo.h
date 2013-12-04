/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQVolumeSourceConfigurationFileInfo - Camera configuration file meta data.
//
// .SECTION Description
// Organizes meta-data that is used by both reader and writer in a single
// location.
//
// .SECTION See Also
// vtkSQVolumeSourceConfigurationReader, vtkSQVolumeSourceConfigurationWriter
//
// .SECTION Thanks
// This class was contributed by SciberQuest Inc.
#ifndef __vtkSQVolumeSourceConfigurationFileInfo_h
#define __vtkSQVolumeSourceConfigurationFileInfo_h

#include "vtkObject.h"

class VTK_EXPORT vtkSQVolumeSourceConfigurationFileInfo
{
public:
  vtkSQVolumeSourceConfigurationFileInfo()
        :
    FileIdentifier("SQVolumeSourceConfiguration"),
    FileDescription("SciberQuest volume source configuration"),
    FileExtension(".sqvsc")
       {}
  virtual ~vtkSQVolumeSourceConfigurationFileInfo(){}

  virtual void PrintSelf(ostream &os, vtkIndent indent)
    {
    os
      << indent << "FileIdentifier: " << this->FileIdentifier << std::endl
      << indent << "FileDescription: " << this->FileDescription << std::endl
      << indent << "FileExtension: " << this->FileExtension << std::endl;
    }

  const char *FileIdentifier;
  const char *FileDescription;
  const char *FileExtension;
};

#endif
