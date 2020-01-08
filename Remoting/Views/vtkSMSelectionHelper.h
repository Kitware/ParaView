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
   * Given the ContentType for an output vtkSelection, this create a new source
   * proxy generating the selection, the input selectionSourceProxy is used to
   * fill the default values for created selection source.
   */
  static vtkSMProxy* ConvertSelection(
    int outputType, vtkSMProxy* selectionSourceProxy, vtkSMSourceProxy* dataSource, int outputport);

  /**
   * Updates output to be a combination of (input | output) if the two selection
   * sources are mergeable. Returns true if merge successful.
   * dataSource and outputport are needed if a conversion is needed to make the
   * input expandable to the type of the output.
   */
  static bool MergeSelection(vtkSMSourceProxy* output, vtkSMSourceProxy* input,
    vtkSMSourceProxy* dataSource, int outputport);

  /**
   * Updates output to be a subtraction of input and output (input - output) if the two selection
   * sources are mergeable. Returns true if the subtraction is successful.
   * dataSource and outputport are needed if a conversion is needed to make the
   * input expandable to the type of the output.
   */
  static bool SubtractSelection(vtkSMSourceProxy* output, vtkSMSourceProxy* input,
    vtkSMSourceProxy* dataSource, int outputport);

  /**
   * Updates output to be a toggle of input and output
   * (input + (input | output ) - (input & output ) ) if the two selection
   * sources are mergeable. Returns true if the toggling is successful.
   * dataSource and outputport are needed if a conversion is needed to make the
   * input expandable to the type of the output.
   */
  static bool ToggleSelection(vtkSMSourceProxy* output, vtkSMSourceProxy* input,
    vtkSMSourceProxy* dataSource, int outputport);

protected:
  vtkSMSelectionHelper(){};
  ~vtkSMSelectionHelper() override{};

  static void ConvertSurfaceSelectionToVolumeSelectionInternal(
    vtkIdType connectionID, vtkSelection* input, vtkSelection* output, int global_ids);

private:
  vtkSMSelectionHelper(const vtkSMSelectionHelper&) = delete;
  void operator=(const vtkSMSelectionHelper&) = delete;

  static vtkSMProxy* NewSelectionSourceFromSelectionInternal(
    vtkSMSession*, vtkSelectionNode* selection, vtkSMProxy* selSource, bool ignore_composite_keys);

  static vtkSMProxy* ConvertInternal(
    vtkSMSourceProxy* inSource, vtkSMSourceProxy* dataSource, int dataPort, int outputType);
};

#endif
