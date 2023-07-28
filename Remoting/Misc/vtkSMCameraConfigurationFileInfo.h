// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
