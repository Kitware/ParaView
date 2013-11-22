/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQHemisphereSourceConfigurationFileInfo.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSQHemisphereSourceConfigurationFileInfo - Camera configuration file meta data.
//
// .SECTION Description
// Organizes meta-data that is used by both reader and writer in a single
// location.
//
// .SECTION See Also
// vtkSQHemisphereSourceConfigurationReader, vtkSQHemisphereSourceConfigurationWriter
//
// .SECTION Thanks
// This class was contributed by SciberQuest Inc.
#ifndef __vtkSQHemisphereSourceConfigurationFileInfo_h
#define __vtkSQHemisphereSourceConfigurationFileInfo_h

#include "vtkObject.h"

class VTK_EXPORT vtkSQHemisphereSourceConfigurationFileInfo
{
public:
  vtkSQHemisphereSourceConfigurationFileInfo()
        :
    FileIdentifier("SQHemisphereSourceSourceConfiguration"),
    FileDescription("SciberQuest HemisphereSource Source configuration"),
    FileExtension(".sqhsc")
       {}
  virtual ~vtkSQHemisphereSourceConfigurationFileInfo(){}

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
