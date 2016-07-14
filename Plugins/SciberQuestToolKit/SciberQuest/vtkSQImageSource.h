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
#ifndef vtkSQImageSource_h
#define vtkSQImageSource_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkImageAlgorithm.h"
#include "CartesianExtent.h" // for CartesianExtent

class vtkInformation;
class vtkInformationVector;
class vtkDataSetAttributes;
class vtkPVXMLElement;

class VTKSCIBERQUEST_EXPORT vtkSQImageSource : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkSQImageSource,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQImageSource *New();

  // Description:
  // Initialize from an xml document.
  int Initialize(vtkPVXMLElement *root);

  // Description:
  // Set the whole extent of the generated dataset
  vtkGetVector6Macro(Extent,int);
  vtkSetVector6Macro(Extent,int);

  // Description:
  // For PV UI. Range domains only work with arrays of size 2.
  void SetIExtent(int ilo, int ihi);
  void SetJExtent(int jlo, int jhi);
  void SetKExtent(int klo, int khi);

  // Description:
  // Set the grid spacing.
  vtkSetVector3Macro(Origin,double);
  vtkGetVector3Macro(Origin,double);

  // Description:
  // Set the dataset origin
  vtkSetVector3Macro(Spacing,double);
  vtkGetVector3Macro(Spacing,double);

protected:
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  vtkSQImageSource();
  virtual ~vtkSQImageSource();

private:
  int Extent[6];
  double Origin[3];
  double Spacing[3];

private:
  vtkSQImageSource(const vtkSQImageSource &) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQImageSource &) VTK_DELETE_FUNCTION;
};

#endif
