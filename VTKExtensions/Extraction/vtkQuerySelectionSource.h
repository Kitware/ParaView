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

#include "vtkDataObject.h"                      // for vtkDataObject
#include "vtkPVVTKExtensionsExtractionModule.h" //needed for exports
#include "vtkSelectionAlgorithm.h"
#include <string> // for std::string
#include <vector> // for std::vector

class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSEXTRACTION_EXPORT vtkQuerySelectionSource : public vtkSelectionAlgorithm
{
public:
  static vtkQuerySelectionSource* New();
  vtkTypeMacro(vtkQuerySelectionSource, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the parallel controller. Initialized to
   * `vtkMultiProcessController::GetGlobalController` in the constructor.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  //@{
  /**
   * Set/get the query expression string.
   */
  vtkSetStringMacro(QueryString);
  vtkGetStringMacro(QueryString);
  //@}

  //@{
  /**
   * AssemblyName to choose an assembly and Selectors for that assembly to limit
   * selection to specific blocks.
   */
  vtkSetStringMacro(AssemblyName);
  vtkGetStringMacro(AssemblyName);
  void AddSelector(const char*);
  void ClearSelectors();
  //@}

  //@{
  /**
   * Get/Set AMR level and index to use. Note, while VTK uses `unsigned int` for
   * amr level and index numbers we use `int`s in this API  and use -1 to
   * indicate that no AMR level/index has been specified.
   */
  vtkSetClampMacro(AMRLevel, int, -1, VTK_INT_MAX);
  vtkGetMacro(AMRLevel, int);
  vtkSetClampMacro(AMRIndex, int, -1, VTK_INT_MAX);
  vtkGetMacro(AMRIndex, int);
  //@}

  //@{
  /**
   * Get/Set which process to limit the selection to. `-1` is treated as
   * all processes.
   */
  vtkSetClampMacro(ProcessID, int, -1, VTK_INT_MAX);
  vtkGetMacro(ProcessID, int);
  //@}

  //@{
  /**
   * Set/Get which types of elements are being selected.
   * Accepted values are defined in `vtkDataObject::AttributeTypes`. Note,
   * `vtkDataObject::FIELD` and `vtkDataObject::POINT_THEN_CELL` are not
   * supported.
   */
  vtkSetClampMacro(ElementType, int, vtkDataObject::POINT, vtkDataObject::ROW);
  vtkGetMacro(ElementType, int);
  //@}

  /**
   * This merely reconstructs the query as a user friendly text eg. "IDs >= 12".
   */
  const char* GetUserFriendlyText();

  //@{
  /**
   * Set/get the invert selection flag.
   */
  vtkSetMacro(Inverse, bool);
  vtkGetMacro(Inverse, bool);
  //@}

  //@{
  /**
   * Specify number of layers to extract connected to the selected elements.
   */
  vtkSetClampMacro(NumberOfLayers, int, 0, VTK_INT_MAX);
  vtkGetMacro(NumberOfLayers, int);
  //@}
protected:
  vtkQuerySelectionSource();
  ~vtkQuerySelectionSource() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkQuerySelectionSource(const vtkQuerySelectionSource&) = delete;
  void operator=(const vtkQuerySelectionSource&) = delete;

  vtkMultiProcessController* Controller;
  char* QueryString;
  int ElementType;
  char* AssemblyName;
  std::vector<std::string> Selectors;
  int AMRLevel;
  int AMRIndex;
  int ProcessID;
  int NumberOfLayers;
  bool Inverse;
};

#endif
