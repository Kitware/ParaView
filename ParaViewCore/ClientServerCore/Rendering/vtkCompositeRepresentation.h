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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Methods overridden to propagate to the active representation.
   */
  virtual void SetVisibility(bool val) VTK_OVERRIDE;

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
  virtual void SetInputConnection(int port, vtkAlgorithmOutput* input) VTK_OVERRIDE;
  virtual void SetInputConnection(vtkAlgorithmOutput* input) VTK_OVERRIDE;
  virtual void AddInputConnection(int port, vtkAlgorithmOutput* input) VTK_OVERRIDE;
  virtual void AddInputConnection(vtkAlgorithmOutput* input) VTK_OVERRIDE;
  virtual void RemoveInputConnection(int port, vtkAlgorithmOutput* input) VTK_OVERRIDE;
  virtual void RemoveInputConnection(int port, int idx) VTK_OVERRIDE;
  //@}

  /**
   * Propagate the modification to all internal representations.
   */
  virtual void MarkModified() VTK_OVERRIDE;

  /**
   * Returns the data object that is rendered from the given input port.
   */
  virtual vtkDataObject* GetRenderedDataObject(int port) VTK_OVERRIDE;

  /**
   * Returns the list of available representation types as a string array.
   */
  vtkStringArray* GetRepresentationTypes();

  //@{
  /**
   * Passed on to internal representations as well.
   */
  virtual void SetUpdateTime(double time) VTK_OVERRIDE;
  virtual void SetForceUseCache(bool val) VTK_OVERRIDE;
  virtual void SetForcedCacheKey(double val) VTK_OVERRIDE;
  //@}

protected:
  vtkCompositeRepresentation();
  ~vtkCompositeRepresentation();

  virtual int FillInputPortInformation(int, vtkInformation* info) VTK_OVERRIDE;

  /**
   * Adds the representation to the view.  This is called from
   * vtkView::AddRepresentation().  Subclasses should override this method.
   * Returns true if the addition succeeds.
   */
  virtual bool AddToView(vtkView* view) VTK_OVERRIDE;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  virtual bool RemoveFromView(vtkView* view) VTK_OVERRIDE;

  /**
   * Fires UpdateDataEvent
   */
  void TriggerUpdateDataEvent();

private:
  vtkCompositeRepresentation(const vtkCompositeRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositeRepresentation&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;
  vtkCommand* Observer;
};

#endif
