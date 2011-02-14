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
// .NAME vtkPVExtractPieces
// .SECTION Description
// vtkPVExtractPieces is a composite filter that deals with different types of
// datasets using internal filters. It's main goal is add data-distribution
// capabilities to non-parallel aware data sources.
// .SECTION Notes
// In older versions of ParaView this was done by the
// vtkSMOutputPort::InsertExtractPiecesIfNecessary() method. Now, we always
// simply insert this filter in the pipeline and leave the decision making for
// run-time. This overcomes the need to have inputs setup correctly before
// calling CreateOutputPorts() on vtkSMSourceProxy.

#ifndef __vtkPVExtractPieces_h
#define __vtkPVExtractPieces_h

#include "vtkPassInputTypeAlgorithm.h"
#include "vtkSmartPointer.h"
class VTK_EXPORT vtkPVExtractPieces : public vtkPassInputTypeAlgorithm
{
public:
  static vtkPVExtractPieces* New();
  vtkTypeMacro(vtkPVExtractPieces, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPVExtractPieces();
  ~vtkPVExtractPieces();
  int FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation* info);

  virtual int RequestDataObject(vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);
  virtual int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  virtual int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  virtual int RequestUpdateExtent(vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  // Description:
  // Ensure that correct RealAlgorithm is setup.
  bool EnsureRealAlgorithm(vtkInformation* inputInformation);

  vtkSmartPointer<vtkAlgorithm> RealAlgorithm;

private:
  vtkPVExtractPieces(const vtkPVExtractPieces&); // Not implemented
  void operator=(const vtkPVExtractPieces&); // Not implemented
//ETX
};

#endif
