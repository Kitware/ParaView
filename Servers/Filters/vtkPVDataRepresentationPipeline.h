/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDataRepresentationPipeline - executive for
// vtkPVDataRepresentation.
// .SECTION Description
// vtkPVDataRepresentationPipeline is an executive for vtkPVDataRepresentation.
// In works in collaboration with the vtkPVView and vtkPVDataRepresentation to
// ensure appropriate time/piece is requested from the upstream. This also helps
// when caching is employed by the view.

#ifndef __vtkPVDataRepresentationPipeline_h
#define __vtkPVDataRepresentationPipeline_h

#include "vtkCompositeDataPipeline.h"

class VTK_EXPORT vtkPVDataRepresentationPipeline : public vtkCompositeDataPipeline
{
public:
  static vtkPVDataRepresentationPipeline* New();
  vtkTypeMacro(vtkPVDataRepresentationPipeline, vtkCompositeDataPipeline);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPVDataRepresentationPipeline();
  ~vtkPVDataRepresentationPipeline();

  virtual int ForwardUpstream(int i, int j, vtkInformation* request);
  virtual int ForwardUpstream(vtkInformation* request);

  virtual void ExecuteDataEnd(vtkInformation* request,
    vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec);

  // Override this check to account for update extent.
  virtual int NeedToExecuteData(int outputPort,
    vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec);

  virtual int ProcessRequest(vtkInformation* request,
    vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec);

private:
  vtkPVDataRepresentationPipeline(const vtkPVDataRepresentationPipeline&); // Not implemented
  void operator=(const vtkPVDataRepresentationPipeline&); // Not implemented
//ETX
};

#endif
