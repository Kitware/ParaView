/*=========================================================================

  Program:   ParaView
  Module:    vtkQuerySelectionSource.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkQuerySelectionSource
 * @brief   a selection source that uses a "query" to
 * generate the selection.
 *
 * vtkQuerySelectionSource is a selection source that uses a "query" to generate
 * the vtkSelection object.
 * A query has the following form: "TERM OPERATOR VALUE(s)"
 * eg. "GLOBALID is_in_range (0, 10)" here GLOBALID is the TERM, is_in_range is
 * the operator and (0,10) are the values. A query can have additional
 * qualifiers such as the process id, block id, amr level, amr block.
*/

#ifndef vtkQuerySelectionSource_h
#define vtkQuerySelectionSource_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkSelectionAlgorithm.h"

class vtkAbstractArray;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkQuerySelectionSource : public vtkSelectionAlgorithm
{
public:
  static vtkQuerySelectionSource* New();
  vtkTypeMacro(vtkQuerySelectionSource, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/get the query expression string.
   */
  vtkSetStringMacro(QueryString);
  vtkGetStringMacro(QueryString);
  //@}

  //@{
  vtkSetMacro(CompositeIndex, int);
  vtkGetMacro(CompositeIndex, int);
  //@}

  vtkSetMacro(HierarchicalLevel, int);
  vtkGetMacro(HierarchicalLevel, int);

  vtkSetMacro(HierarchicalIndex, int);
  vtkGetMacro(HierarchicalIndex, int);

  vtkSetMacro(ProcessID, int);
  vtkGetMacro(ProcessID, int);

  // Possible values are as defined by
  // vtkSelectionNode::SelectionField.
  vtkSetMacro(FieldType, int);
  vtkGetMacro(FieldType, int);

  /**
   * This merely reconstructs the query as a user friendly text eg. "IDs >= 12".
   * ( Makes you want to wonder if we should support parsing input query text as
   * well ;) )
   */
  const char* GetUserFriendlyText();

  //@{
  /**
   * Set/get the invert selection flag.
   */
  vtkSetMacro(Inverse, int);
  vtkGetMacro(Inverse, int);
  //@}

protected:
  vtkQuerySelectionSource();
  ~vtkQuerySelectionSource() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FieldType;

  char* QueryString;

  int CompositeIndex;
  int HierarchicalIndex;
  int HierarchicalLevel;
  int ProcessID;

private:
  vtkQuerySelectionSource(const vtkQuerySelectionSource&) = delete;
  void operator=(const vtkQuerySelectionSource&) = delete;

  class vtkInternals;
  vtkInternals* Internals;

  int Inverse;
};

#endif
