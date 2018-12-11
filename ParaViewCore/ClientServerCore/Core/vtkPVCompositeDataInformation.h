/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeDataInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVCompositeDataInformation
 * @brief   Light object for holding composite data information.
 *
 * vtkPVCompositeDataInformation is used to copy the meta information of
 * a composite dataset from server to client. It holds a vtkPVDataInformation
 * for each block of the composite dataset.
 * @sa
 * vtkHierarchicalBoxDataSet vtkPVDataInformation
*/

#ifndef vtkPVCompositeDataInformation_h
#define vtkPVCompositeDataInformation_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkPVInformation.h"

class vtkPVDataInformation;
class vtkUniformGridAMR;

struct vtkPVCompositeDataInformationInternals;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVCompositeDataInformation : public vtkPVInformation
{
public:
  static vtkPVCompositeDataInformation* New();
  vtkTypeMacro(vtkPVCompositeDataInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object.
   */
  void AddInformation(vtkPVInformation*) override;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  //@}

  /**
   * Clears all internal data structures.
   */
  virtual void Initialize();

  /**
   * Returns the number of children.
   */
  unsigned int GetNumberOfChildren();

  /**
   * Returns the information for the data object at the given index. If the
   * child is a composite dataset itself, then the return vtkPVDataInformation
   * will have the CompositeDataInformation set appropriately.
   */
  vtkPVDataInformation* GetDataInformation(unsigned int idx);

  /**
   * Return the name of the child node at the given index, if any. This is the
   * value for the key vtkCompositeDataSet::NAME() in the meta-data associated
   * with the node.
   */
  const char* GetName(unsigned int idx);

  //@{
  /**
   * Get/Set if the data is multipiece. If so, then GetDataInformation() will
   * always return NULL. For vtkMultiblockDataSet, we don't collect information
   * about individual pieces. One can however, query the number of pieces by
   * using GetNumberOfChildren().
   */
  vtkGetMacro(DataIsMultiPiece, int);
  //@}

  //@{
  /**
   * Returns if the dataset is a composite dataset.
   */
  vtkGetMacro(DataIsComposite, int);
  //@}

  //@{
  /**
   * Returns the number of levels in the AMR dataset. Only valid for
   * vtkUniformGridAMR datasets.
   */
  vtkGetMacro(NumberOfAMRLevels, unsigned int);
  //@}

  // TODO:
  // Add API to obtain meta data information for each of the children.

protected:
  vtkPVCompositeDataInformation();
  ~vtkPVCompositeDataInformation() override;

  /**
   * Copy information from an amr dataset.
   */
  void CopyFromAMR(vtkUniformGridAMR* amr);

  int DataIsMultiPiece;
  int DataIsComposite;
  unsigned int FlatIndexMax;

  unsigned int NumberOfPieces;
  vtkSetMacro(NumberOfPieces, unsigned int);

  unsigned int NumberOfAMRLevels;

  friend class vtkPVDataInformation;
  vtkPVDataInformation* GetDataInformationForCompositeIndex(int* index);

private:
  vtkPVCompositeDataInformationInternals* Internal;

  vtkPVCompositeDataInformation(const vtkPVCompositeDataInformation&) = delete;
  void operator=(const vtkPVCompositeDataInformation&) = delete;
};

#endif
