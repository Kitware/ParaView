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
// .NAME vtkSQLogSource - A ParaView UI for the SciberQuest Log.
// .SECTION Description
// A dummy pipeline object (produces no data) providing a server
// manager UI for the SciberQuest Log. Applications may turn logging
// on and off view the GlobalLevel ivar. The data gathered by the log
// is written by the mpi root rank when this object is destroyed. Therefor
// this object must destroyed before MPI_Finalize is invoked.

#ifndef vtkSQLogSource_h
#define vtkSQLogSource_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPVXMLElement;

class VTKSCIBERQUEST_EXPORT vtkSQLogSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSQLogSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQLogSource *New();

  // Description:
  // Initialize from and xml document
  int Initialize(vtkPVXMLElement *root);

  // Description:
  // Set/Get the global log level. Applications can enable logging
  // for all sciberquest objects by setting this.
  void SetGlobalLevel(int level);
  vtkGetMacro(GlobalLevel,int);

  // Description:
  // Set/Get the log file name.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkSQLogSource();
  ~vtkSQLogSource();

  // VTK Pipeline
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);

private:
  vtkSQLogSource(const vtkSQLogSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQLogSource&) VTK_DELETE_FUNCTION;

private:
  int GlobalLevel;
  char *FileName;
};

#endif
