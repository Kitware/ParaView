/*=========================================================================

  Program:   ParaView
  Module:    vtkSpreadSheetRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSpreadSheetRepresentation
 *
 * Representation for showing data in a vtkSpreadSheetView. Unlike typical
 * ParaView representations, this one does not do any data movement, it merely
 * updates the input and provides access to the input data objects (rather
 * clones of those). This filer has 3 input ports:
 * \li 0: Data (vtkDataObject)
 * \li 1: Extracted Data (vtkUnstructruedGrid or multi-block of it)
 * \li 2: Extracted vtkSelection (vtkSelection)
 * @warning
 * This representation doesn't support caching currently.
*/

#ifndef vtkSpreadSheetRepresentation_h
#define vtkSpreadSheetRepresentation_h

#include "vtkNew.h"                               // needed for vtkNew.
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVDataRepresentation.h"

class vtkBlockDeliveryPreprocessor;
class vtkCleanArrays;
class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkSpreadSheetRepresentation
  : public vtkPVDataRepresentation
{
public:
  static vtkSpreadSheetRepresentation* New();
  vtkTypeMacro(vtkSpreadSheetRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Since this has no delivery, just mark ourselves modified.
   */
  virtual void MarkModified() VTK_OVERRIDE { this->Superclass::MarkModified(); }

  vtkAlgorithmOutput* GetDataProducer();
  vtkAlgorithmOutput* GetExtractedDataProducer();
  vtkAlgorithmOutput* GetSelectionProducer();

  //@{
  /**
   * Allow user to enable/disable cell connectivity generation in the datamodel
   */
  void SetGenerateCellConnectivity(bool);
  bool GetGenerateCellConnectivity();
  //@}

  //***************************************************************************
  // Forwarded to vtkBlockDeliveryPreprocessor.
  void SetFieldAssociation(int val);
  int GetFieldAssociation();

  //@{
  /**
   * Select the block indices to extract.
   * Each node in the multi-block tree is identified by an \c index. The index can
   * be obtained by performing a preorder traversal of the tree (including empty
   * nodes). eg. A(B (D, E), C(F, G)).
   * Inorder traversal yields: A, B, D, E, C, F, G
   * Index of A is 0, while index of C is 4.
   */
  void AddCompositeDataSetIndex(unsigned int index);
  void RemoveAllCompositeDataSetIndices();
  //@}

protected:
  vtkSpreadSheetRepresentation();
  ~vtkSpreadSheetRepresentation();

  /**
   * Fill input port information.
   */
  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  /**
   * Overridden to invoke vtkCommand::UpdateDataEvent.
   */
  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  vtkNew<vtkCleanArrays> CleanArrays;
  vtkNew<vtkBlockDeliveryPreprocessor> DataConditioner;

  vtkNew<vtkCleanArrays> ExtractedCleanArrays;
  vtkNew<vtkBlockDeliveryPreprocessor> ExtractedDataConditioner;

private:
  vtkSpreadSheetRepresentation(const vtkSpreadSheetRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSpreadSheetRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
