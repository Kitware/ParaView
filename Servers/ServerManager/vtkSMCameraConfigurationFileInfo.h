/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMCameraConfigurationFileInfo.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCameraConfigurationFileInfo - Camera configuration file meta data.
//
// .SECTION Description
// Organizes meta-data that is used by both reader and writer in a single
// location.
//
// .SECTION See Also
// vtkSMCameraConfigurationReader, vtkSMCameraConfigurationWriter
//
// .SECTION Thanks
// This class was contributed by SciberQuest Inc.
#ifndef __vtkSMCameraConfigurationFileInfo_h
#define __vtkSMCameraConfigurationFileInfo_h

#include "vtkObject.h"

class vtkSMCameraConfigurationFileInfo
{
public:
  vtkSMCameraConfigurationFileInfo()
        :
    FileIdentifier("PVCameraConfiguration"),
    FileDescription("ParaView camera configuration"),
    FileExtension(".pvcc")
      { }

  void PrintSelf(ostream &os, vtkIndent indent)
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

