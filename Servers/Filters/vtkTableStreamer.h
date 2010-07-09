/*=========================================================================

  Program:   ParaView
  Module:    vtkTableStreamer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTableStreamer - block-based vtkTable streaming filter.
// .SECTION Description
// vtkTableStreamer is a block-based vtkTable streaming filter. 

#ifndef __vtkTableStreamer_h
#define __vtkTableStreamer_h

#include "vtkDataObjectAlgorithm.h"
#include <vtkstd/vector> // needed for vtkstd::vector

class vtkMultiProcessController;

class VTK_EXPORT vtkTableStreamer : public vtkDataObjectAlgorithm
{
public:
  static vtkTableStreamer* New();
  vtkTypeMacro(vtkTableStreamer, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the number of rows that fit within one block. The output of this
  // filter will have at most BlockSize rows.
  vtkSetMacro(BlockSize, vtkIdType);
  vtkGetMacro(BlockSize, vtkIdType);

  // Description:
  // Get/Set the block to pass.
  vtkSetMacro(Block, vtkIdType);
  vtkGetMacro(Block, vtkIdType);

  // Description:
  // Get/Set the MPI controller used for gathering.
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

//BTX
protected:
  vtkTableStreamer();
  ~vtkTableStreamer();

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual vtkExecutive* CreateDefaultExecutive();

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestDataObject(vtkInformation*,
                                vtkInformationVector**,
                                vtkInformationVector*);

  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*);

  // Description:
  // Fills up \c result with (start offset, count) pairs for each leaf node in
  // the input dObj that should be passed through this filter.
  bool DetermineIndicesToPass(vtkDataObject* dObj,
    vtkstd::vector<vtkstd::pair<vtkIdType, vtkIdType> >& result);


  vtkIdType Block;
  vtkIdType BlockSize;
  vtkMultiProcessController* Controller;
private:
  vtkTableStreamer(const vtkTableStreamer&); // Not implemented
  void operator=(const vtkTableStreamer&); // Not implemented

  // Description:
  // Fills up counts with the number of rows in each leaf node (for composite
  // input. For non-composite input, counts[0] is filled with the number of
  // rows). This works in parallel collecting information across all processes.
  bool CountRows(vtkDataObject* dObj, vtkstd::vector<vtkIdType>& counts,
    vtkstd::vector<vtkIdType>& offsets);
//ETX
};

#endif

