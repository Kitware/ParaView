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
/**
 * @class   vtkSMCameraConfigurationFileInfo
 * @brief   Camera configuration file meta data.
 *
 *
 * Organizes meta-data that is used by both reader and writer in a single
 * location.
 *
 * @sa
 * vtkSMCameraConfigurationReader, vtkSMCameraConfigurationWriter
 *
 * @par Thanks:
 * This class was contributed by SciberQuest Inc.
*/

#ifndef vtkSMCameraConfigurationFileInfo_h
#define vtkSMCameraConfigurationFileInfo_h

#include "vtkObject.h"

class vtkSMCameraConfigurationFileInfo
{
public:
  vtkSMCameraConfigurationFileInfo()
    : FileIdentifier("PVCameraConfiguration")
    , FileDescription("ParaView camera configuration")
    , FileExtension(".pvcc")
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

// VTK-HeaderTest-Exclude: vtkSMCameraConfigurationFileInfo.h
