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
/**
 * @class   vtkSMProxyConfigurationFileInfo
 * @brief   Proxy configuration file meta data.
 *
 *
 * Organizes meta-data that is used by both reader and writer in a single
 * location.
 *
 * @sa
 * vtkSMProxyConfigurationReader, vtkSMProxyConfigurationWriter
 *
 * @par Thanks:
 * This class was contributed by SciberQuest Inc.
*/

#ifndef vtkSMConfigurationFileInfo_h
#define vtkSMConfigurationFileInfo_h

#include "vtkObject.h"

class vtkSMProxyConfigurationFileInfo
{
public:
  vtkSMProxyConfigurationFileInfo()
    : FileIdentifier("SMProxyConfiguration")
    , FileDescription("ParaView server manager proxy configuration")
    , FileExtension(".pvpc")
  {
  }

  void PrintSelf(ostream& os, vtkIndent indent)
  {
    os << indent << "FileIdentifier: " << this->FileIdentifier << endl
       << indent << "FileDescription: " << this->FileDescription << endl
       << indent << "FileExtension: " << this->FileExtension << endl;
  }

  const char* FileIdentifier;
  const char* FileDescription;
  const char* FileExtension;
};

#endif

// VTK-HeaderTest-Exclude: vtkSMProxyConfigurationFileInfo.h
