/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClipDataSet.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVClipDataSet - Clip filter
//
// .SECTION Description
// This is a subclass of vtkTableBasedClipDataSet that allows selection of input
// scalars.

#ifndef __vtkPVClipDataSet_h
#define __vtkPVClipDataSet_h

#include "vtkTableBasedClipDataSet.h"

class VTK_EXPORT vtkPVClipDataSet : public vtkTableBasedClipDataSet
{
public:
  vtkTypeMacro(vtkPVClipDataSet,vtkTableBasedClipDataSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  static vtkPVClipDataSet* New();

  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  // Description:
  // This filter uses vtkAMRDualClip for clipping AMR datasets. Do disable that
  // behavior, turn this flag off.
  vtkSetMacro(UseAMRDualClipForAMR, bool);
  vtkGetMacro(UseAMRDualClipForAMR, bool);
  vtkBooleanMacro(UseAMRDualClipForAMR, bool);

//BTX
protected:
  vtkPVClipDataSet(vtkImplicitFunction *cf=NULL);
  ~vtkPVClipDataSet();

  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**, vtkInformationVector* );

  virtual int RequestDataObject(vtkInformation*,
                                vtkInformationVector**,
                                vtkInformationVector*);


  virtual int FillInputPortInformation(int, vtkInformation* info);
  virtual int FillOutputPortInformation(int, vtkInformation* info);

  // Description:
  // Uses superclass to clip the input. This also handles composite datasets
  // (since superclass does not handle composite datasets). This method loops
  // over the composite dataset calling superclass repeatedly.
  int ClipUsingSuperclass(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  int ClipUsingThreshold(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  bool UseAMRDualClipForAMR;
private:
  vtkPVClipDataSet(const vtkPVClipDataSet&);  // Not implemented.
  void operator=(const vtkPVClipDataSet&);  // Not implemented.
//ETX
};

#endif
