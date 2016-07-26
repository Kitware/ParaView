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
// .NAME vtkSQVolumeSource -Create volume of hexahedral cells.
// .SECTION Description
// Creates a volume composed of hexahedra cells on a latice.
// This is the 3D counterpart to the plane source.

#ifndef vtkSQVolumeSource_h
#define vtkSQVolumeSource_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkPVXMLElement;

class VTKSCIBERQUEST_EXPORT vtkSQVolumeSource : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkSQVolumeSource *New();
  vtkTypeMacro(vtkSQVolumeSource,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the object from an xml document.
  int Initialize(vtkPVXMLElement *root);

  // Description:
  // Set the points defining edges of a 3D quarilateral.
  vtkSetVector3Macro(Origin,double);
  vtkGetVector3Macro(Origin,double);

  vtkSetVector3Macro(Point1,double);
  vtkGetVector3Macro(Point1,double);

  vtkSetVector3Macro(Point2,double);
  vtkGetVector3Macro(Point2,double);

  vtkSetVector3Macro(Point3,double);
  vtkGetVector3Macro(Point3,double);

  // Description:
  // Set the latice resolution in the given direction.
  vtkSetVector3Macro(Resolution,int);
  vtkGetVector3Macro(Resolution,int);

  // Description:
  // Toggle between immediate mode and demand mode. In immediate
  // mode requested geometry is gernerated and placed in the output
  // in demand mode a cell generator is placed in the pipeline and
  // a single cell is placed in the output.
  vtkSetMacro(ImmediateMode,int);

  // Description:
  // Set the log level.
  // 0 -- no logging
  // 1 -- basic logging
  // .
  // n -- advanced logging
  vtkSetMacro(LogLevel,int);
  vtkGetMacro(LogLevel,int);
  vtkGetMacro(ImmediateMode,int);

protected:
  /// Pipeline internals.
  //int FillInputPortInformation(int port,vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);

  vtkSQVolumeSource();
  ~vtkSQVolumeSource();

private:
  int ImmediateMode;
  double Origin[3];
  double Point1[3];
  double Point2[3];
  double Point3[3];
  int Resolution[3];
  int LogLevel;

private:
  vtkSQVolumeSource(const vtkSQVolumeSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQVolumeSource&) VTK_DELETE_FUNCTION;
};

#endif
