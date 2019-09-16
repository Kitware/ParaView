/*=========================================================================

  Program:   ParaView
  Module:    vtkCompositeRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCompositeRepresentation
 * @brief   combine multiple representations into one
 * with only 1 representation active at a time.
 *
 * vtkCompositeRepresentation makes is possible to combine multiple
 * representations into one. Only one representation can be active at a give
 * time. vtkCompositeRepresentation provides API to add the representations that
 * form the composite and to pick the active representation.
 *
 * vtkCompositeRepresentation relies on call AddToView and RemoveFromView
 * on the internal representations whenever it needs to change the active
 * representation. So it is essential that representations handle those methods
 * correctly and don't suffer from uncanny side effects when that's done
 * repeatedly.
*/

#ifndef vtkCompositeRepresentation_h
#define vtkCompositeRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVDataRepresentation.h"

class vtkStringArray;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkCompositeRepresentation
  : public vtkPVDataRepresentation
{
public:
  static vtkCompositeRepresentation* New();
  vtkTypeMacro(vtkCompositeRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

  /**
   * Methods overridden to propagate to the active representation.
   */
  void SetVisibility(bool val) override;

  //@{
  /**
   * Add/Remove representations. \c key is a unique string used to identify
   * that representation.
   */
  virtual void AddRepresentation(const char* key, vtkPVDataRepresentation* repr);
  virtual void RemoveRepresentation(vtkPVDataRepresentation* repr);
  virtual void RemoveRepresentation(const char* key);
  //@}

  //@{
  /**
   * Set the active key. If a valid key is not specified, then none of the
   * representations is treated as active.
   */
  void SetActiveRepresentation(const char* key);
  const char* GetActiveRepresentationKey();
  //@}

  /**
   * Returns the active representation if valid.
   */
  vtkPVDataRepresentation* GetActiveRepresentation();

  //@{
  /**
   * Overridden to simply pass the input to the internal representations. We
   * won't need this if vtkPVDataRepresentation correctly respected in the
   * arguments passed to it during ProcessRequest() etc.
   */
  void SetInputConnection(int port, vtkAlgorithmOutput* input) override;
  void SetInputConnection(vtkAlgorithmOutput* input) override;
  void AddInputConnection(int port, vtkAlgorithmOutput* input) override;
  void AddInputConnection(vtkAlgorithmOutput* input) override;
  void RemoveInputConnection(int port, vtkAlgorithmOutput* input) override;
  void RemoveInputConnection(int port, int idx) override;
  //@}

  /**
   * Propagate the modification to all internal representations.
   */
  void MarkModified() override;

  /**
   * Returns the data object that is rendered from the given input port.
   */
  vtkDataObject* GetRenderedDataObject(int port) override;

  /**
   * Returns the list of available representation types as a string array.
   */
  vtkStringArray* GetRepresentationTypes();

  //@{
  /**
   * Passed on to internal representations as well.
   */
  void SetUpdateTime(double time) override;
  void SetForceUseCache(bool val) override;
  void SetForcedCacheKey(double val) override;
  //@}

protected:
  vtkCompositeRepresentation();
  ~vtkCompositeRepresentation() override;

  int FillInputPortInformation(int, vtkInformation* info) override;

  /**
   * Adds the representation to the view.  This is called from
   * vtkView::AddRepresentation().  Subclasses should override this method.
   * Returns true if the addition succeeds.
   */
  bool AddToView(vtkView* view) override;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  bool RemoveFromView(vtkView* view) override;

  /**
   * Fires UpdateDataEvent
   */
  void TriggerUpdateDataEvent();

private:
  vtkCompositeRepresentation(const vtkCompositeRepresentation&) = delete;
  void operator=(const vtkCompositeRepresentation&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
  vtkCommand* Observer;
};

#endif
