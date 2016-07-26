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
// .NAME vtkSQBOVReader -- Connects the VTK pipeline to BOVReader class.
// .SECTION Description
//
// Implements the VTK style pipeline and manipulates and instance of
// BOVReader so that "brick of values" datasets, including time series,
// can be read in parallel.
//
// .SECTION See Also
// BOVReader

#ifndef vtkSQBOVReader_h
#define vtkSQBOVReader_h
// #define vtkSQBOVReaderDEBUG

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkSQBOVReaderBase.h"

#include <vector> // for vector
#include <string> // for string

class vtkPVXMLElement;

class VTKSCIBERQUEST_EXPORT vtkSQBOVReader : public vtkSQBOVReaderBase
{
public:
  static vtkSQBOVReader *New();
  vtkTypeMacro(vtkSQBOVReader,vtkSQBOVReaderBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Iitialize the reader from an XML document. You also need to
  // pass in the bov file name so that subsetting and array selection
  // can be applied which has to occur after the file has been opened.

  virtual int Initialize(
        vtkPVXMLElement *root,
        const char *fileName,
        std::vector<std::string> &arrays);

protected:
  virtual int RequestInformation(
        vtkInformation *req,
        vtkInformationVector **inInfos,
        vtkInformationVector *outInfos);

  virtual int RequestData(
        vtkInformation *req,
        vtkInformationVector **inInfos,
        vtkInformationVector *outInfos);

  vtkSQBOVReader();
  virtual ~vtkSQBOVReader();

  virtual void Clear();

private:
  vtkSQBOVReader(const vtkSQBOVReader &) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQBOVReader &) VTK_DELETE_FUNCTION;

private:

};

#endif
