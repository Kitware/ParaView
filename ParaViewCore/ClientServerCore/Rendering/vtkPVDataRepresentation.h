/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVDataRepresentation
 *
 * vtkPVDataRepresentation adds some ParaView specific API to data
 * representations.
 * @sa
 * vtkPVDataRepresentationPipeline
*/

#ifndef vtkPVDataRepresentation_h
#define vtkPVDataRepresentation_h

#include "vtkCommand.h" // needed for vtkCommand
#include "vtkDataRepresentation.h"
#include "vtkPVClientServerCoreRenderingModule.h" // needed for exports
#include "vtkWeakPointer.h"                       // needed for vtkWeakPointer
#include <string>                                 // needed for string

class vtkInformationRequestKey;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVDataRepresentation : public vtkDataRepresentation
{
public:
  vtkTypeMacro(vtkPVDataRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   *
   * @note Starting with ParaView 5.8, this method is no longer called for
   * hidden representation i.e. representations for which
   * `vtkDataRepresentation::GetVisibility()` returns false.
   */
  virtual int ProcessViewRequest(
    vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo);

  /**
   * This is one of the most important functions. In VTK pipelines, it's very
   * easy for the pipeline to decide when it needs to re-execute.
   * vtkAlgorithm::Update() can go up the entire pipeline to see if any filters
   * MTime changed (among other things) and if so, it can re-execute the pipeline.
   * However in case of representations, the real input connection may only be
   * present on the data-server nodes. In that case the
   * vtkPVDataRepresentation::RequestData() will only get called on the
   * data-server nodes. That means that representations won't be able to any
   * data-delivery in RequestData(). We'd need some other mechanisms to
   * synchronize data-delivery among processes. To avoid that conundrum, the
   * vtkSMRepresentationProxy calls MarkModified() on all processes whenever any
   * filter in the pipeline is modified. In this method, the
   * vtkPVDataRepresentation subclasses should ensure that they mark all
   * delivery related filters dirty in their RequestData to ensure they execute
   * then next time they are updated. The vtkPVDataRepresentation also uses a
   * special executive which avoids updating the representation unless
   * MarkModified() was called since the last Update(), thus acting as a
   * update-suppressor.
   */
  virtual void MarkModified();

  /**
   * Initialize the representation with an identifier range so each internal
   * representation can own a unique ID.
   * If a representation requires more IDs than the set of ids provided,
   * the representation MUST complains by an error or abort explaining how many
   * ids where expected so the number of reserved ids could be easily adjust.
   * Unless noted otherwise, this method must be called before calling any
   * other methods on this class.
   * \note CallOnAllProcesses
   * Internally you can pick an id that follow that condition
   * minIdAvailable <= id <= maxIdAvailable
   * Return the minIdAvailable after initialization so that new range could be used
   */
  virtual unsigned int Initialize(unsigned int minIdAvailable, unsigned int maxIdAvailable);

  /**
   * Return 0 if the Initialize() method was not called otherwise a unique ID
   * that will be shared across the processes for that same object.
   */
  unsigned int GetUniqueIdentifier() { return this->UniqueIdentifier; }

  /**
   * Get/Set the visibility for this representation. When the visibility of
   * representation of false, all view passes are ignored.
   */
  virtual void SetVisibility(bool val) { this->Visibility = val; }
  vtkGetMacro(Visibility, bool);

  /**
   * Returns the data object that is rendered from the given input port.
   */
  virtual vtkDataObject* GetRenderedDataObject(int vtkNotUsed(port))
  {
    return this->GetInputDataObject(0, 0);
  }

  //@{
  /**
   * Set the update time.
   */
  virtual void SetUpdateTime(double time);
  vtkGetMacro(UpdateTime, double);
  //@}

  //@{
  /**
   * Get whether the UpdateTime is valid. `ResetUpdateTime` can be used to clear
   * the UpdateTimeValid flag.
   */
  vtkGetMacro(UpdateTimeValid, bool);
  void ResetUpdateTime();
  //@}

  //@{
  /**
   * Generally, caching is within the purview of the vtkPVView (and subclasses).
   * However, a representation may choose to override that caching by forcing
   * specific caching parameters. This is primarily intended for views like
   * comparative view that rely on caching to show multiple frames from the same
   * pipeline at a time.
   */
  virtual void SetForcedCacheKey(double val) { this->ForcedCacheKey = val; }
  virtual void SetForceUseCache(bool val) { this->ForceUseCache = val; }
  vtkGetMacro(ForcedCacheKey, double);
  vtkGetMacro(ForceUseCache, bool);
  //@}

  /**
   * Returns the cache-key the representation is currently using. This takes
   * into consideration whether ForcedCacheKey is to be used.
   */
  double GetCacheKey() const;

  //@{
  /**
   * Making these methods public. When constructing composite representations,
   * we need to call these methods directly on internal representations.
   */
  bool AddToView(vtkView* view) override;
  bool RemoveFromView(vtkView* view) override;
  //@}

  /**
   * Retrieves an output port for the input data object at the specified port
   * and connection index. This may be connected to the representation's
   * internal pipeline.
   * Overridden to use vtkPVTrivialProducer instead of vtkTrivialProducer
   */
  vtkAlgorithmOutput* GetInternalOutputPort() override { return this->GetInternalOutputPort(0); }
  vtkAlgorithmOutput* GetInternalOutputPort(int port) override
  {
    return this->GetInternalOutputPort(port, 0);
  }
  vtkAlgorithmOutput* GetInternalOutputPort(int port, int conn) override;

  /**
   * Provides access to the view.
   */
  vtkView* GetView() const;

  /**
   * Returns the timestamp when `RequestData` was executed on the
   * representation.
   */
  vtkMTimeType GetPipelineDataTime();

  //@{
  /**
   * This is solely intended to simplify debugging and use for any other purpose
   * is vehemently discouraged.
   */
  virtual void SetLogName(const std::string& name) { this->LogName = name; }
  const std::string& GetLogName() const { return this->LogName; }
  //@}

  //@{
  /**
   * This flag indicates if the representation is for a temporal pipeline. If
   * true, then calling `SetUpdateTime` will result in calling
   * `this->MarkModified()` i.e. will cause the representation to update on
   * subsequent update request. Otherwise, the subsequent update request will be
   * a no-op (unless the representation ended up calling MarkModified due to
   * other changes i.e. change in input pipeline). `HasTemporalPipeline` gets
   * set in the first update on the representation, however, only on the
   * data-server side. vtkPVView ensures that the flag is synced up among all
   * ranks at the end of each `vtkPVView::Update` call.
   */
  vtkGetMacro(HasTemporalPipeline, bool);
  vtkSetMacro(HasTemporalPipeline, bool);
  //@}

  /**
   * Returns true if the representation needs an update.
   */
  bool GetNeedsUpdate();

  enum
  {
    /**
     * This event is fired in `ProcessViewRequest` when `REQUEST_UPDATE` was
     * requested and the representation needed an update (i.e.
     * this->GetNeedsUpdate() == true), however was skipped since the data
     * is already cached by the view.
     */
    SkippedUpdateDataEvent = vtkCommand::UserEvent + 91,
    UpdateTimeChangedEvent,
  };

  //@{
  /**
   * Overridden to ensure that `MarkModified` is called.
   */
  void SetInputConnection(int port, vtkAlgorithmOutput* input) override;
  using Superclass::SetInputConnection;
  void AddInputConnection(int port, vtkAlgorithmOutput* input) override;
  using Superclass::AddInputConnection;
  //@}

protected:
  vtkPVDataRepresentation();
  ~vtkPVDataRepresentation() override;

  /**
   * Create a default executive.
   */
  vtkExecutive* CreateDefaultExecutive() override;

  /**
   * Overridden to invoke vtkCommand::UpdateDataEvent.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestUpdateTime(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double UpdateTime;
  bool UpdateTimeValid;
  unsigned int UniqueIdentifier;

private:
  vtkPVDataRepresentation(const vtkPVDataRepresentation&) = delete;
  void operator=(const vtkPVDataRepresentation&) = delete;

  bool Visibility;
  bool ForceUseCache;
  double ForcedCacheKey;
  double CacheKey;

  bool HasTemporalPipeline;

  class Internals;
  Internals* Implementation;
  vtkWeakPointer<vtkView> View;
  std::string LogName;
};

#endif
