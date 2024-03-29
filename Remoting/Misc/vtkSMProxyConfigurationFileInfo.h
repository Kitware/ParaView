// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#ifndef vtkSMProxyConfigurationFileInfo_h
#define vtkSMProxyConfigurationFileInfo_h

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
