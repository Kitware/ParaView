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

class vtkCollection;
class vtkSMProxy;
class vtkSMRenderModuleProxy;
class vtkSelection;

class VTK_EXPORT vtkSMSelectionHelper : public vtkSMObject
{
public:
  static vtkSMSelectionHelper* New();
  vtkTypeRevisionMacro(vtkSMSelectionHelper, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Given a selection object, creates a server side representation which
  // can be accessed using the given proxy.
  static void SendSelection(vtkSelection* sel, vtkSMProxy* proxy);

  // Description:
  // This method finds the source and geometry proxies corresponding to a
  // prop id and puts their ids as properties in the selection.
  static void AddSourceIDs(vtkSelection* sel,
                           vtkSMRenderModuleProxy* rmp);

  // Description:
  // Convert a polydata selection on the surface to an unstructured
  // selection on the surface. The input selection belongs to the output of
  // the geometry filter, the output selection belongs to the input of the
  // geometry filter. 
  // vtkSelectionConverter requires certain properties to
  // be set for this to work. See vtkSelectionConverter documentation.
  // Make sure to specify the connection id for the server on which
  // the selection was performed.
  static void ConvertSurfaceSelectionToVolumeSelection(vtkIdType connectionID,
                                                       vtkSelection* input,
                                                       vtkSelection* output);

  // Description:
  // Same as ConvertSurfaceSelectionToVolumeSelection except that the
  // converted selection is in terms on GlobalIDs.
  static void ConvertSurfaceSelectionToGlobalIDVolumeSelection(vtkIdType connectionID,
    vtkSelection* input, vtkSelection* output);

  // Description:
  // Given a render module proxy and a selection rectange (xmin, ymin,
  // xmax, ymax), returns selected proxies and associated selection objects
  // in selectedProxies and selections collections.  The two collections
  // are guarenteed to be in sync (i.e. the proxies and selection objects
  // are stored in the same order) so that you can traverse them in the
  // same order.
  static void SelectOnSurface(vtkSMRenderModuleProxy* rmP,
                              int rectangle[4],
                              vtkCollection* selectedProxies,
                              vtkCollection* selections);

  // Description:
  // Given a selection, returns a proxy for a selection source that has
  // the ids specified by it. This source can then be used as input
  // to a vtkExtractSelection filter.
  // Make sure to specify the connection id for the server on which
  // the selection was performed.
  static vtkSMProxy* NewSelectionSourceFromSelection(vtkIdType connectionID,
                                                     vtkSelection* selection);

protected:
  vtkSMSelectionHelper() {};
  ~vtkSMSelectionHelper() {};

  static void ConvertSurfaceSelectionToVolumeSelectionInternal(
    vtkIdType connectionID, vtkSelection* input, vtkSelection* output,
    int global_ids);

private:
  vtkSMSelectionHelper(const vtkSMSelectionHelper&); // Not implemented.
  void operator=(const vtkSMSelectionHelper&); // Not implemented.
};

#endif

