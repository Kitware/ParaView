/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectionSource.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVSelectionSource
 * @brief   selection source used to produce different types
 * of vtkSelections.
 *
 * vtkPVSelectionSource is used to create different types of selections. It
 * provides different APIs for different types of selections to create.
 * The output selection type depends on the API used most recently.
*/

#ifndef vtkPVSelectionSource_h
#define vtkPVSelectionSource_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkSelectionAlgorithm.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPVSelectionSource : public vtkSelectionAlgorithm
{
public:
  static vtkPVSelectionSource* New();
  vtkTypeMacro(vtkPVSelectionSource, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set a frustum to choose within.
   */
  void AddFrustum(double vertices[32]);

  //@{
  /**
   * Add global IDs.
   */
  void AddGlobalID(vtkIdType id);
  void RemoveAllGlobalIDs();
  //@}

  //@{
  /**
   * Add integer pedigree IDs in a particular domain.
   */
  void AddPedigreeID(const char* domain, vtkIdType id);
  void RemoveAllPedigreeIDs();
  //@}

  //@{
  /**
   * Add string pedigree IDs in a particular domain.
   */
  void AddPedigreeStringID(const char* domain, const char* id);
  void RemoveAllPedigreeStringIDs();
  //@}

  //@{
  /**
   * Add a (piece, id) to the selection set. The source will generate
   * only the ids for which piece == UPDATE_PIECE_NUMBER.
   * If piece == -1, the id applies to all pieces.
   */
  void AddID(vtkIdType piece, vtkIdType id);
  void RemoveAllIDs();
  //@}

  //@{
  /**
   * Add IDs that will be added to the selection produced by the
   * selection source.
   * The source will generate
   * only the ids for which piece == UPDATE_PIECE_NUMBER.
   * If piece == -1, the id applies to all pieces.
   */
  void AddCompositeID(unsigned int composite_index, vtkIdType piece, vtkIdType id);
  void RemoveAllCompositeIDs();
  //@}

  //@{
  /**
   * The list of IDs that will be added to the selection produced by the
   * selection source.
   */
  void AddHierarhicalID(unsigned int level, unsigned int dataset, vtkIdType id);
  void RemoveAllHierarchicalIDs();
  //@}

  //@{
  /**
   * Add a value range to threshold within.
   */
  void AddThreshold(double min, double max);
  void RemoveAllThresholds();
  //@}

  //@{
  /**
   * Add the flat-index/composite index for a block.
   */
  void AddBlock(vtkIdType blockno);
  void RemoveAllBlocks();
  //@}

  /**
   * For threshold and value selection, this controls the name of the
   * scalar array that will be thresholded within.
   */
  void SetArrayName(const char* arrayName);

  //@{
  /**
   * Add a point in world space to probe at.
   */
  void AddLocation(double x, double y, double z);
  void RemoveAllLocations();
  //@}

  //@{
  /**
   * Set the field type for the generated selection.
   * Possible values are as defined by
   * vtkSelection::SelectionField.
   */
  vtkSetMacro(FieldType, int);
  vtkGetMacro(FieldType, int);
  //@}

  //@{
  /**
   * When extracting by points, extract the cells that contain the
   * passing points.
   */
  vtkSetMacro(ContainingCells, int);
  vtkGetMacro(ContainingCells, int);
  //@}

  //@{
  vtkSetMacro(Inverse, int);
  vtkGetMacro(Inverse, int);
  //@}

  //@{
  /**
   * Set/get the query expression string.
   */
  vtkSetStringMacro(QueryString);
  vtkGetStringMacro(QueryString);
  //@}

protected:
  vtkPVSelectionSource();
  ~vtkPVSelectionSource() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  enum Modes
  {
    FRUSTUM,
    GLOBALIDS,
    ID,
    COMPOSITEID,
    HIERARCHICALID,
    THRESHOLDS,
    LOCATIONS,
    BLOCKS,
    PEDIGREEIDS,
    QUERY
  };

  Modes Mode;
  int FieldType;
  int ContainingCells;
  int Inverse;
  double Frustum[32];
  char* ArrayName;
  char* QueryString;

private:
  vtkPVSelectionSource(const vtkPVSelectionSource&) = delete;
  void operator=(const vtkPVSelectionSource&) = delete;

  class vtkInternal;
  vtkInternal* Internal;
};

#endif
