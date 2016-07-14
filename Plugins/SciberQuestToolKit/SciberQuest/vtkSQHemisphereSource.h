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
// .NAME vtkSQHemisphereSource - Source/Reader that provides a polydata sphere as 2 hemispheres.
// .SECTION Description
// Source that provides a polydata sphere as 2 hemispheres on 2 outputs.
//

#ifndef vtkSQHemisphereSource_h
#define vtkSQHemisphereSource_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPVXMLElement;

class VTKSCIBERQUEST_EXPORT vtkSQHemisphereSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSQHemisphereSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQHemisphereSource *New();

  // Description:
  // Initialize from and xml document
  int Initialize(vtkPVXMLElement *root);

  // Description:
  // Set/Get location of the sphere.
  vtkSetVector3Macro(Center,double);
  vtkGetVector3Macro(Center,double);

  // Description:
  // Set/Get the vector along the north pole.
  vtkSetVector3Macro(North,double);
  vtkGetVector3Macro(North,double);

  // Description:
  // Set/Get the radius of the sphere.
  vtkSetMacro(Radius,double);
  vtkGetMacro(Radius,double);

  // Description:
  // Set/Get the resolution (number of polys) used in the output.
  vtkSetMacro(Resolution,int);
  vtkGetMacro(Resolution,int);

  // Description:
  // Set/Get descriptive names attached to each of the outputs.
  // The defaults are "north" and "south".
  vtkSetStringMacro(NorthHemisphereName);
  vtkGetStringMacro(NorthHemisphereName);
  vtkSetStringMacro(SouthHemisphereName);
  vtkGetStringMacro(SouthHemisphereName);

  // Description:
  // Set the log level.
  // 0 -- no logging
  // 1 -- basic logging
  // .
  // n -- advanced logging
  vtkSetMacro(LogLevel,int);
  vtkGetMacro(LogLevel,int);

protected:
  vtkSQHemisphereSource();
  ~vtkSQHemisphereSource();

  // VTK Pipeline
  int FillInputPortInformation(int port,vtkInformation *info);
  //int FillOutputPortInformation(int port,vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation* req, vtkInformationVector** input, vtkInformationVector* output);

private:
  vtkSQHemisphereSource(const vtkSQHemisphereSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQHemisphereSource&) VTK_DELETE_FUNCTION;

private:
  double North[3];
  double Center[3];
  double Radius;
  int Resolution;
  char *NorthHemisphereName;
  char *SouthHemisphereName;
  int LogLevel;
};

#endif
