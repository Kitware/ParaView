/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#ifndef vtkSQPlaneSourceConfigurationFileInfo_h
#define vtkSQPlaneSourceConfigurationFileInfo_h

#include "vtkObject.h"

class VTK_EXPORT vtkSQPlaneSourceConfigurationFileInfo
{
public:
  vtkSQPlaneSourceConfigurationFileInfo()
    : FileIdentifier("SQPlaneSourceConfiguration")
    , FileDescription("SciberQuest plane source configuration")
    , FileExtension(".sqpsc")
  {
  }
  virtual ~vtkSQPlaneSourceConfigurationFileInfo() {}

  virtual void PrintSelf(ostream& os, vtkIndent indent)
  {
    os << indent << "FileIdentifier: " << this->FileIdentifier << std::endl
       << indent << "FileDescription: " << this->FileDescription << std::endl
       << indent << "FileExtension: " << this->FileExtension << std::endl;
  }

  const char* FileIdentifier;
  const char* FileDescription;
  const char* FileExtension;
};

#endif
