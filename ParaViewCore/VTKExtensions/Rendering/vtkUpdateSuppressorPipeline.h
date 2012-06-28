/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUpdateSuppressorPipeline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUpdateSuppressorPipeline - pipeline for vtkPVUpdateSuppressor
// .SECTION Description
// vtkUpdateSuppressorPipeline is designed to for with vtkPVUpdateSuppressor.
// It stops all update extent and data requests.

#ifndef __vtkUpdateSuppressorPipeline_h
#define __vtkUpdateSuppressorPipeline_h

#include "vtkCompositeDataPipeline.h"

class VTK_EXPORT vtkUpdateSuppressorPipeline : public vtkCompositeDataPipeline
{
public:
  static vtkUpdateSuppressorPipeline* New();
  vtkTypeMacro(vtkUpdateSuppressorPipeline, vtkCompositeDataPipeline);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Generalized interface for asking the executive to fullfill update
  // requests.
  virtual int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inInfo,
                             vtkInformationVector* outInfo);

  // Description:
  // Get/Set if the update suppressions are enabled.
  // Enabled by default.
  void SetEnabled(bool e) {this->Enabled = e;}
  vtkGetMacro(Enabled, bool);
protected:
  vtkUpdateSuppressorPipeline();
  ~vtkUpdateSuppressorPipeline();

  bool Enabled;
private:
  vtkUpdateSuppressorPipeline(const vtkUpdateSuppressorPipeline&);  // Not implemented.
  void operator=(const vtkUpdateSuppressorPipeline&);  // Not implemented.
};

#endif
