/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMSelectionHelper
 * @brief   Utility class to help with selection tasks
 *
 * This class contains several static methods that help with the
 * complicated selection task.
 */

#ifndef vtkSMSelectionHelper_h
#define vtkSMSelectionHelper_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkCollection;
class vtkSelection;
class vtkSelectionNode;
class vtkSMProxy;
class vtkSMSession;
class vtkSMSourceProxy;

class VTKREMOTINGVIEWS_EXPORT vtkSMSelectionHelper : public vtkSMObject
{
public:
  static vtkSMSelectionHelper* New();
  vtkTypeMacro(vtkSMSelectionHelper, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create an append selection proxy from a selection source.
   */
  static vtkSMProxy* NewAppendSelectionsFromSelectionSource(vtkSMSourceProxy* selectionSource);

  enum class CombineOperation
  {
    DEFAULT = 0,
    ADDITION = 1,
    SUBTRACTION = 2,
    TOGGLE = 3
  };

  ///@{
  /**
   * Combine appendSelections1 with appendSelections2 using a combine operation and
   * store the result into appendSelections2. appendSelections1 and appendSelections2 can be
   * combined if they have the same fieldType and containing cells qualifiers. The returned value
   * indicates if the selections could/have be combined.
   *
   * If operation is DEFAULT,     appendSelections2 = appendSelections2
   * if operation is ADDITION,    appendSelections2 = appendSelections1 | appendSelections2
   * if operation is SUBTRACTION, appendSelections2 = appendSelections1 & !appendSelections2
   * if operation is TOGGLE,      appendSelections2 = appendSelections1 ^ appendSelections2
   *
   * Note: appendSelections2 must have at least one selection source, but appendSelections1 can be
   * empty (but valid).
   */
  static bool CombineSelection(vtkSMSourceProxy* appendSelections1,
    vtkSMSourceProxy* appendSelections2, CombineOperation operation, bool deepCopy = false);
  static bool IgnoreSelection(
    vtkSMSourceProxy* appendSelections1, vtkSMSourceProxy* appendSelections2, bool deepCopy = false)
  {
    return CombineSelection(
      appendSelections1, appendSelections2, CombineOperation::DEFAULT, deepCopy);
  }
  static bool AddSelection(
    vtkSMSourceProxy* appendSelections1, vtkSMSourceProxy* appendSelections2, bool deepCopy = false)
  {
    return CombineSelection(
      appendSelections1, appendSelections2, CombineOperation::ADDITION, deepCopy);
  }
  static bool SubtractSelection(
    vtkSMSourceProxy* appendSelections1, vtkSMSourceProxy* appendSelections2, bool deepCopy = false)
  {
    return CombineSelection(
      appendSelections1, appendSelections2, CombineOperation::SUBTRACTION, deepCopy);
  }
  static bool ToggleSelection(
    vtkSMSourceProxy* appendSelections1, vtkSMSourceProxy* appendSelections2, bool deepCopy = false)
  {
    return CombineSelection(
      appendSelections1, appendSelections2, CombineOperation::TOGGLE, deepCopy);
  }
  ///@}

  /**
   * Given a selection, returns a proxy for a selection source that has
   * the ids specified by it. This source can then be used as input
   * to a vtkExtractSelection filter.
   * CAVEAT: Make sure to specify the connection id for the server on which
   * the selection was performed. This method can only handle 3 types of
   * selection FRUSTUM, INDICES and GLOBALIDS. We can easily change this to
   * handle all other types of selection but that's not required currently and
   * hence we not adding that code.
   */
  static vtkSMProxy* NewSelectionSourceFromSelection(
    vtkSMSession* session, vtkSelection* selection, bool ignore_composite_keys = false);

  static void NewSelectionSourcesFromSelection(vtkSelection* selection, vtkSMProxy* view,
    vtkCollection* selSources, vtkCollection* selRepresentations);

  /**
   * Given the ContentType for an output vtkSelection, this create a new append selections
   * proxy generating the selection, the input appendSelections is used to
   * fill the default values for created selection source.
   */
  static vtkSMProxy* ConvertAppendSelections(int outputType, vtkSMSourceProxy* appendSelections,
    vtkSMSourceProxy* dataSource, int dataPort, bool& selectionChanged);

  /**
   * Given the ContentType for an output vtkSelection, this create a new source
   * proxy generating the selection, the input selectionSourceProxy is used to
   * fill the default values for created selection source.
   */
  static vtkSMProxy* ConvertSelectionSource(int outputType, vtkSMSourceProxy* selectionSourceProxy,
    vtkSMSourceProxy* dataSource, int dataPort);

protected:
  vtkSMSelectionHelper() = default;
  ~vtkSMSelectionHelper() override = default;

private:
  vtkSMSelectionHelper(const vtkSMSelectionHelper&) = delete;
  void operator=(const vtkSMSelectionHelper&) = delete;

  static vtkSMProxy* NewSelectionSourceFromSelectionInternal(
    vtkSMSession*, vtkSelectionNode* selection, vtkSMProxy* selSource, bool ignore_composite_keys);

  static vtkSMProxy* ConvertInternal(
    vtkSMSourceProxy* inSource, vtkSMSourceProxy* dataSource, int dataPort, int outputType);

  static const std::string SubSelectionBaseName;
};

#endif
