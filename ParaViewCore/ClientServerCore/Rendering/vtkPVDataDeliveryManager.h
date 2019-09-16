/*=========================================================================

  Program: ParaView
  Module:  vtkPVDataDeliveryManager

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVDataDeliveryManager
 * @brief manager for data-delivery.
 *
 * ParaView's multi-configuration / multi-process modes pose a challenge for
 * views. At runtime, the current configuration will determine which processes
 * have what parts of data and which processes are expected to "render" that data.
 * While views and their representations may add certain qualifiers to this
 * statement, generally speaking, all views have to support taking the data from
 * the data-processing nodes and delivering it to the rendering nodes. This is
 * where vtkPVDataDeliveryManager comes in play. It helps views (viz. vtkPVView
 * subclasses) move the data.
 */

#ifndef vtkPVDataDeliveryManager_h
#define vtkPVDataDeliveryManager_h

#include "vtkObject.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkSmartPointer.h"                      // needed for iVar.
#include "vtkTuple.h"                             // needed for vtkTuple.
#include "vtkWeakPointer.h"                       // needed for iVar.

class vtkAlgorithmOutput;
class vtkDataObject;
class vtkExtentTranslator;
class vtkInformation;
class vtkPKdTree;
class vtkPVDataRepresentation;
class vtkPVView;

#include <vector>

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVDataDeliveryManager : public vtkObject
{
public:
  vtkTypeMacro(vtkPVDataDeliveryManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the render-view. The view is not reference counted.
   */
  void SetView(vtkPVView*);
  vtkPVView* GetView() const;
  //@}

  /**
   * Returned a hash number that can be used to verify that both client and
   * server side are in synch representation wise for delivery.
   */
  int GetSynchronizationMagicNumber();

  //@{
  /**
   * View uses these methods to register a representation with the storage. This
   * makes it possible for representations to communicate with the storage
   * directly using a self pointer, while enables views on different processes
   * to communicate information about representations using their unique ids.
   */
  void RegisterRepresentation(vtkPVDataRepresentation* repr);
  void UnRegisterRepresentation(vtkPVDataRepresentation*);
  vtkPVDataRepresentation* GetRepresentation(unsigned int);
  //@}

  //@{
  /**
   * Representations (indirectly via vtkPVRenderView::SetPiece()) call this
   * method to register the geometry type they are rendering. Every
   * representation that requires delivering of any geometry must register with
   * the vtkPVDataDeliveryManager and never manage the delivery on its own.
   */
  void SetPiece(vtkPVDataRepresentation* repr, vtkDataObject* data, bool low_res,
    unsigned long trueSize = 0, int port = 0);
  //@}

  //@{
  bool HasPiece(vtkPVDataRepresentation* repr, bool low_res = false, int port = 0);
  //@}

  /**
   * Returns the local data object set by calling `SetPiece` (or from the
   * cache). This is the data object pre-delivery.
   */
  vtkDataObject* GetPiece(vtkPVDataRepresentation* repr, bool low_res, int port = 0);

  /**
   * Returns the data object post-delivery.
   */
  vtkDataObject* GetDeliveredPiece(vtkPVDataRepresentation* repr, bool low_res, int port = 0);

  /**
   * Clear all cached data objects for the given representation.
   */
  void ClearCache(vtkPVDataRepresentation* repr);

  //@{
  /**
   * Provides access to the producer port for the geometry of a registered
   * representation. Representations use these methods (indirectly via
   * vtkPVRenderView::GetPieceProducer() and GetPieceProducerLOD()) to obtain
   * the geometry producer for the geometry to be rendered.
   */
  vtkAlgorithmOutput* GetProducer(vtkPVDataRepresentation*, bool low_res, int port = 0);
  //@}

  //@{
  /**
   * Set/Get meta-data container for the specific piece. Views can use it to
   * store arbitrary metadata for each piece.
   */
  vtkInformation* GetPieceInformation(vtkPVDataRepresentation* repr, bool low_res, int port = 0);
  //@}

  //@{
  /**
   * Returns number of known port for the representation.
   */
  int GetNumberOfPorts(vtkPVDataRepresentation* repr);
  //@}

  /**
   * Returns the size for all visible geometry. If low_res is true, and low-res
   * data is not available for a particular representation, then it's high-res
   * data size will be used assuming that the representation is going to render
   * the high-res geometry for low-res rendering as well.
   */
  unsigned long GetVisibleDataSize(bool low_res);

  /**
   * Internal method used to determine the list of representations that need
   * their geometry delivered. This is done on the "client" side, with the
   * client decide what geometries it needs and then requests those from the
   * server-sides using Deliver().
   */
  bool NeedsDelivery(
    vtkMTimeType timestamp, std::vector<unsigned int>& keys_to_deliver, bool use_lod);

  /**
   * Triggers delivery for the geometries of indicated representations.
   */
  void Deliver(int use_low_res, unsigned int size, unsigned int* keys);

  /**
   * Views that support changing of which ranks do the rendering at runtime
   * based on things like data sizes, etc. may override this method to provide a
   * unique key for each different mode. This makes it possible to keep
   * delivered data object for each mode separate and thus avoid transfers if
   * the mode is changed on the fly.
   *
   * Default implementation simply returns 0.
   */
  virtual int GetDeliveredDataKey(bool low_res) const
  {
    (void)low_res;
    return 0;
  }

protected:
  vtkPVDataDeliveryManager();
  ~vtkPVDataDeliveryManager() override;

  double GetCacheKey(vtkPVDataRepresentation* repr) const;

  /**
   * This method is called to request that the subclass do appropriate transfer
   * for the indicated representation.
   */
  virtual void MoveData(vtkPVDataRepresentation* repr, bool low_res, int port) = 0;

  class vtkInternals;
  vtkInternals* Internals;

private:
  vtkPVDataDeliveryManager(const vtkPVDataDeliveryManager&) = delete;
  void operator=(const vtkPVDataDeliveryManager&) = delete;

  vtkWeakPointer<vtkPVView> View;
};

#endif

// VTK-HeaderTest-Exclude: vtkPVDataDeliveryManager.h
