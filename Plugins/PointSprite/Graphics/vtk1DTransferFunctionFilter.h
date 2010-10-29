/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk1DTransferFunctionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtk1DTransferFunctionFilter
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>
// .SECTION Description
// This filters takes an array as input and map it through a transfer function.
// The mapped array will have as many tuples as the input array, and one component.
// The name and type of the output array can be specified.
// by default, the output array will have the same type as the input array.
// The output array will have the same association as the input array : point data
// will be mapped to point data, cell data to cell data...

#ifndef vtk1DTransferFunctionFilter_H_
#define vtk1DTransferFunctionFilter_H_

#include "vtkPassInputTypeAlgorithm.h"
class vtk1DTransferFunction;

class VTK_EXPORT vtk1DTransferFunctionFilter : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtk1DTransferFunctionFilter, vtkPassInputTypeAlgorithm);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtk1DTransferFunctionFilter *New();

  // Description:
  // Set/Get the transfer function that will be used by this filter
  // to map an array from the input to the output.
  // The array to use as input has to be selected by the SelectInputArrayToProcess methods.
  // The output array will have 1 component and as many tuples as the input array.
  // It will have the same fieldAssociation than the input (ie point data will be
  // mapped to point data, cell data to cell data...)
  virtual void  SetTransferFunction(vtk1DTransferFunction*);
  vtkGetObjectMacro(TransferFunction, vtk1DTransferFunction);

  // Description:
  // Enable/Disable this filter.
  // When disabled, this filter shallow copies the input to the output.
  vtkSetMacro(Enabled, int);
  vtkGetMacro(Enabled, int);
  vtkBooleanMacro(Enabled, int);

  // Description:
  // Set/Get the output array name.
  // This name will be given to the output array.
  vtkSetStringMacro(OutputArrayName);
  vtkGetStringMacro(OutputArrayName);

  // Description:
  // If this option is turned on, the name of the output will be the concatenation
  // of the input array name and the OutputArrayName ivar.
  // this can be useful the trace the mapped arrays or to avoid name clashes.
  vtkSetMacro(ConcatenateOutputNameWithInput, int);
  vtkGetMacro(ConcatenateOutputNameWithInput, int);
  vtkBooleanMacro(ConcatenateOutputNameWithInput, int);

  // Description:
  // Set the output array type (see vtkAbstractArray::CreateArray method)
  vtkSetMacro(OutputArrayType, int);
  vtkGetMacro(OutputArrayType, int);

  // Description:
  // If this flag is set, the filter will ignore the OutputArrayType ivar and
  // the output array will always have the same type as the input array.
  vtkSetMacro(ForceSameTypeAsInputArray, int);
  vtkGetMacro(ForceSameTypeAsInputArray, int);
  vtkBooleanMacro(ForceSameTypeAsInputArray, int);

  // Description:
  // Return this object's modified time.
  // overloaded to reflect the TransferFunction MTime too.
  virtual unsigned long GetMTime();

protected:
  vtk1DTransferFunctionFilter();
  virtual ~vtk1DTransferFunctionFilter();

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // Description:
  // Set the array to the output, with the same
  // association that the one used as input.
  // return 1 if the array was added to output, 0 if on error.
  virtual int  SetOutputArray(vtkDataObject*, vtkDataArray*);

  vtk1DTransferFunction* TransferFunction;
  int   Enabled;
  char* OutputArrayName;
  int   OutputArrayType;
  int   ForceSameTypeAsInputArray;
  int   ConcatenateOutputNameWithInput;

private:
  vtk1DTransferFunctionFilter(const vtk1DTransferFunctionFilter&); // Not implemented.
  void operator=(const vtk1DTransferFunctionFilter&); // Not implemented.
};

#endif /* vtk1DTransferFunctionFilter_H_ */
