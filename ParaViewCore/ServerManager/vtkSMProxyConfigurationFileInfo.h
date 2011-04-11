/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMProxyConfigurationFileInfo.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyConfigurationFileInfo - Proxy configuration file meta data.
//
// .SECTION Description
// Organizes meta-data that is used by both reader and writer in a single
// location.
//
// .SECTION See Also
// vtkSMProxyConfigurationReader, vtkSMProxyConfigurationWriter
//
// .SECTION Thanks
// This class was contributed by SciberQuest Inc.
#ifndef __vtkSMConfigurationFileInfo_h
#define __vtkSMConfigurationFileInfo_h

#include "vtkObject.h"

class vtkSMProxyConfigurationFileInfo
{
public:
  vtkSMProxyConfigurationFileInfo()
        :
    FileIdentifier("SMProxyConfiguration"),
    FileDescription("ParaView server manager proxy configuration"),
    FileExtension(".pvpc")
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

