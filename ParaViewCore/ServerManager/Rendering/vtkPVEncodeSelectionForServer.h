/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEncodeSelectionForServer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVEncodeSelectionForServer
 * @brief   Convert a client-side selection into proxies holding
 *          server-side selection sources.
 *
 * In ParaView, selection occurs on the client using a framebuffer
 * whose pixel-values have associated prop, cell, and point IDs
 * obtained by compositing images rendered on each server.
 * The selection is then simplified and broadcast to the server(s)
 * where representations are provided access.
 *
 * By subclassing this object and registering a vtkObjectFactory
 * override, it is possible for applications to get access to the
 * raw selection (with prop IDs and other information) that is
 * created on the client before it is simplified, serialized,
 * and sent to the server.
 */

#ifndef vtkPVEncodeSelectionForServer_h
#define vtkPVEncodeSelectionForServer_h

#include "vtkNew.h" // needed for vtkInteractorObserver.
#include "vtkObject.h"
#include "vtkPVServerManagerRenderingModule.h" //needed for exports

class vtkSMRenderViewProxy;
class vtkCollection;
class vtkSelection;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkPVEncodeSelectionForServer : public vtkObject
{
public:
  static vtkPVEncodeSelectionForServer* New();
  vtkTypeMacro(vtkPVEncodeSelectionForServer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Populate the selection sources given a raw (client-side) selection.
   *
   * This should return true on success (i.e., when \a selectionSources
   * is modified) and false on failure.
   *
   * This method is invoked by vtkSMRenderViewProxy::FetchLastSelection.
   * By default, this method will call vtkShrinkSelection if multiple_selections
   * is false and then use vtkSMSelectionHelper::NewSelectionSourcesFromSelection
   * to create a selection-source object from the selection.
   * Subclasses may override this method and register themselves with
   * the object factory to change the behavior at runtime.
   *
   * The last 2 arguments are not used by the default implementation but
   * are passed for subclasses;
   * + \a modifier may be any value in pqView::SelectionModifier but defaults
   *   to 0 (which is used when the selection should replace any prior selection).
   * + \a selectBlocks indicates whether the selected entities themselves
   *   or the blocks containing them should be selected.
   */
  virtual bool ProcessSelection(vtkSelection* rawSelection, vtkSMRenderViewProxy* viewProxy,
    bool multipleSelectionsAllowed, vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources, int modifier, bool selectBlocks);
  //@}

protected:
  vtkPVEncodeSelectionForServer();
  ~vtkPVEncodeSelectionForServer() override;

private:
  vtkPVEncodeSelectionForServer(const vtkPVEncodeSelectionForServer&) = delete;
  void operator=(const vtkPVEncodeSelectionForServer&) = delete;
};

#endif
