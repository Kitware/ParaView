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
// .NAME vtkSMSelectionHelper - Utility class to help with selection tasks
// .SECTION Description
// This class contains several static methods that help with the
// complicated selection task.

#ifndef __vtkSMSelectionHelper_h
#define __vtkSMSelectionHelper_h

#include "vtkSMObject.h"

class vtkSelection;
class vtkSelectionNode;
class vtkSMProxy;
class vtkSMSourceProxy;
class vtkCollection;

class VTK_EXPORT vtkSMSelectionHelper : public vtkSMObject
{
public:
  static vtkSMSelectionHelper* New();
  vtkTypeMacro(vtkSMSelectionHelper, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Given a selection object, creates a server side representation which
  // can be accessed using the given proxy.
  static void SendSelection(vtkSelection* sel, vtkSMProxy* proxy);

  // Description:
  // Given a selection, returns a proxy for a selection source that has
  // the ids specified by it. This source can then be used as input
  // to a vtkExtractSelection filter.
  // CAVEAT: Make sure to specify the connection id for the server on which
  // the selection was performed. This method can only handle 3 types of
  // selection FRUSTUM, INDICES and GLOBALIDS. We can easily change this to
  // handle all other types of selection but that's not required currently and
  // hence we not adding that code.
  static vtkSMProxy* NewSelectionSourceFromSelection(vtkIdType connectionID,
                                                     vtkSelection* selection);

  static void NewSelectionSourcesFromSelection(
    vtkSelection* selection, vtkSMProxy* view,
    vtkCollection* selSources, vtkCollection* selRepresentations);

  // Description:
  // Given the ContentType for an output vtkSelection, this create a new source
  // proxy generating the selection, the input selectionSourceProxy is used to
  // fill the default values for created selection source.
  static vtkSMProxy* ConvertSelection(int outputType,
    vtkSMProxy* selectionSourceProxy,
    vtkSMSourceProxy* dataSource, int outputport);

  // Description:
  // Updates output to be a combination of (input | output) if the two selection
  // sources are mergeable. Returns true if merge successful.
  // dataSource and outputport are needed if a conversion is needed to make the
  // input expandable to the type of the output.
  static bool MergeSelection(
    vtkSMSourceProxy* output, vtkSMSourceProxy* input,
    vtkSMSourceProxy* dataSource, int outputport);

protected:
  vtkSMSelectionHelper() {};
  ~vtkSMSelectionHelper() {};

  static void ConvertSurfaceSelectionToVolumeSelectionInternal(
    vtkIdType connectionID, vtkSelection* input, vtkSelection* output,
    int global_ids);

private:
  vtkSMSelectionHelper(const vtkSMSelectionHelper&); // Not implemented.
  void operator=(const vtkSMSelectionHelper&); // Not implemented.

  static vtkSMProxy* NewSelectionSourceFromSelectionInternal(
    vtkIdType connectionId, vtkSelectionNode* selection, vtkSMProxy* selSource=0);

  static vtkSMProxy* ConvertInternal(
    vtkSMSourceProxy* inSource, vtkSMSourceProxy* dataSource,
    int dataPort, int outputType);
};

#endif

