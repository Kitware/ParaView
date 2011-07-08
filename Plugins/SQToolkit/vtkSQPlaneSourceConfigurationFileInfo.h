/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
// .NAME vtkSQPlaneSourceConfigurationFileInfo - Camera configuration file meta data.
//
// .SECTION Description
// Organizes meta-data that is used by both reader and writer in a single
// location.
//
// .SECTION See Also
// vtkSQPlaneSourceConfigurationReader, vtkSQPlaneSourceConfigurationWriter
//
// .SECTION Thanks
// This class was contributed by SciberQuest Inc.
#ifndef __vtkSQPlaneSourceConfigurationFileInfo_h
#define __vtkSQPlaneSourceConfigurationFileInfo_h

#include "vtkObject.h"

class vtkSQPlaneSourceConfigurationFileInfo
{
public:
  vtkSQPlaneSourceConfigurationFileInfo()
        :
    FileIdentifier("SQPlaneSourceConfiguration"),
    FileDescription("SciberQuest plane source configuration"),
    FileExtension(".sqpsc")
       {}
  virtual ~vtkSQPlaneSourceConfigurationFileInfo(){}

  virtual void PrintSelf(ostream &os, vtkIndent indent)
    {
    os
      << indent << "FileIdentifier: " << this->FileIdentifier << endl
      << indent << "FileDescription: " << this->FileDescription << endl
      << indent << "FileExtension: " << this->FileExtension << endl;
    }

  const char *FileIdentifier;
  const char *FileDescription;
  const char *FileExtension;
};

#endif

