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
// .NAME vtkSpreadSheetRepresentation
// .SECTION Description
// Representation for showing data in a vtkSpreadSheetView. Unlike typical
// ParaView representations, this one does not do any data movement, it merely
// updates the input and provides access to the input data objects (rather
// clones of those). This filer has 3 input ports:
// \li 0: Data (vtkDataObject)
// \li 1: Extracted Data (vtkUnstructruedGrid or multi-block of it)
// \li 2: Extracted vtkSelection (vtkSelection)
// .SECTION Caveats
// This representation doesn't support caching currently.

#ifndef __vtkSpreadSheetRepresentation_h
#define __vtkSpreadSheetRepresentation_h

#include "vtkPVDataRepresentation.h"

class vtkBlockDeliveryPreprocessor;
class VTK_EXPORT vtkSpreadSheetRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkSpreadSheetRepresentation* New();
  vtkTypeMacro(vtkSpreadSheetRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Since this has no delivery, just mark ourselves modified.
  virtual void MarkModified() { this->Superclass::MarkModified(); }

  vtkAlgorithmOutput* GetDataProducer();
  vtkAlgorithmOutput* GetExtractedDataProducer();
  vtkAlgorithmOutput* GetSelectionProducer();

  //***************************************************************************
  // Forwarded to vtkBlockDeliveryPreprocessor.
  void SetFieldAssociation(int val);
  int GetFieldAssociation();
  void SetCompositeDataSetIndex(int val);

//BTX
protected:
  vtkSpreadSheetRepresentation();
  ~vtkSpreadSheetRepresentation();

  // Description:
  // Fill input port information.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Overridden to invoke vtkCommand::UpdateDataEvent.
  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  vtkBlockDeliveryPreprocessor* DataConditioner;
  vtkBlockDeliveryPreprocessor* ExtractedDataConditioner;

private:
  vtkSpreadSheetRepresentation(const vtkSpreadSheetRepresentation&); // Not implemented
  void operator=(const vtkSpreadSheetRepresentation&); // Not implemented
//ETX
};

#endif
