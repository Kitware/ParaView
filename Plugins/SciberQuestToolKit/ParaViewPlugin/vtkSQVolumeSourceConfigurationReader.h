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
// .NAME vtkSQVolumeSourceConfigurationReader - A reader for XML camera configuration.
//
// .SECTION Description
// A reader for XML camera configuration. Reades camera configuration files.
// writen by the vtkSQVolumeSourceConfigurationWriter.
//
// .SECTION See Also
// vtkSQVolumeSourceConfigurationWriter, vtkSMProxyConfigurationReader
//
// .SECTION Thanks
// This class was contributed by SciberQuest Inc.
#ifndef vtkSQVolumeSourceConfigurationReader_h
#define vtkSQVolumeSourceConfigurationReader_h

#include "vtkSMProxyConfigurationReader.h"

class vtkSMProxy;
class vtkSMRenderViewProxy;
class vtkPVXMLElement;

class VTK_EXPORT vtkSQVolumeSourceConfigurationReader : public vtkSMProxyConfigurationReader
{
public:
  vtkTypeMacro(vtkSQVolumeSourceConfigurationReader, vtkSMProxyConfigurationReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQVolumeSourceConfigurationReader* New();

  // Description:
  // Read the named file, and push the properties into the underying
  // managed render view proxy. This will make sure the renderview is
  // updated after the read.
  virtual int ReadConfiguration(const char* filename);
  virtual int ReadConfiguration(vtkPVXMLElement* x);
  // unhide
  virtual int ReadConfiguration() { return this->Superclass::ReadConfiguration(); }

protected:
  vtkSQVolumeSourceConfigurationReader();
  virtual ~vtkSQVolumeSourceConfigurationReader();

private:
  vtkSQVolumeSourceConfigurationReader(
    const vtkSQVolumeSourceConfigurationReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQVolumeSourceConfigurationReader&) VTK_DELETE_FUNCTION;
};

#endif
