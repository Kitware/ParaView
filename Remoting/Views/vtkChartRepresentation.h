// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkChartRepresentation
 *
 * vtkChartRepresentation is the base representation for charting
 * representations. Currently, ParaView's charting views are client-side only
 * views that render only on the client side. That being the case, when running
 * in client-server mode or in parallel, the data-delivery mode is fixed. Hence,
 * unlike representations for 3D views, this representation delivers the data in
 * RequestData() itself. This makes it possible for client code to call
 * UpdatePipeline() on the representation proxy and then access the delivered
 * vtkTable on the client.
 */

#ifndef vtkChartRepresentation_h
#define vtkChartRepresentation_h

#include "vtkPVDataRepresentation.h"
#include "vtkParaViewDeprecation.h" // for deprecation
#include "vtkSmartPointer.h"        // needed for vtkSmartPointer
#include "vtkWeakPointer.h"         // needed for vtkWeakPointer

#include <map>    // needed for std::map
#include <set>    // needed for std::set
#include <vector> // needed for std::vector

class vtkChartSelectionRepresentation;
class vtkAbstractChartExporter;
class vtkCSVExporter;
class vtkDataObjectTree;
class vtkPVContextView;
class vtkSelectionDeliveryFilter;
class vtkTable;

class VTKREMOTINGVIEWS_EXPORT vtkChartRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkChartRepresentation* New();
  vtkTypeMacro(vtkChartRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * These must only be set during initialization before adding the
   * representation to any views or calling Update().
   */
  void SetSelectionRepresentation(vtkChartSelectionRepresentation*);

  /**
   * Set visibility of the representation.
   */
  void SetVisibility(bool visible) override;

  /**
   * This needs to be called on all instances of vtkGeometryRepresentation when
   * the input is modified. This is essential since the geometry filter does not
   * have any real-input on the client side which messes with the Update
   * requests.
   */
  void MarkModified() override;

  // *************************************************************************

  ///@{
  /**
   * Set the field association for arrays to use. When changed, this will call
   * MarkModified().
   */
  void SetFieldAssociation(int);
  vtkGetMacro(FieldAssociation, int);
  ///@}

  ///@{
  /**
   * Get/Set the name of the assembly to use for mapping block visibilities,
   * colors and opacities.
   *
   * The default is Hierarchy.
   */
  vtkSetStringMacro(ActiveAssembly);
  vtkGetStringMacro(ActiveAssembly);
  ///@}

  ///@{
  /**
   * Update list of selectors that determine the selected blocks.
   */
  void AddBlockSelector(const char* selector);
  void RemoveAllBlockSelectors();
  ///@}

  ///@{
  /**
   * Methods to control block selection.
   * When changed, this will call MarkModified().
   */
  PARAVIEW_DEPRECATED_IN_6_1_0("Use AddBlockSelector() with SetActiveAssembly instead. "
                               "CompositeDataSetIndex is no longer used.")
  void SetCompositeDataSetIndex(unsigned int); // only used for single block selection
  PARAVIEW_DEPRECATED_IN_6_1_0("Use AddBlockSelector() with SetActiveAssembly instead. "
                               "CompositeDataSetIndex is no longer used.")
  void AddCompositeDataSetIndex(unsigned int);
  PARAVIEW_DEPRECATED_IN_6_1_0("Use RemoveAllBlockSelectors() with SetActiveAssembly instead. "
                               "CompositeDataSetIndex is no longer used.")
  void ResetCompositeDataSetIndices();
  ///@}

  /**
   * Override because of internal selection representations that need to be
   * initialized as well.
   */
  unsigned int Initialize(unsigned int minIdAvailable, unsigned int maxIdAvailable) override;

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   * Overridden to handle REQUEST_RENDER() to call PrepareForRendering.
   */
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

  /**
   * Method to provide the default name given the name of a table and a column
   * in that table. When overriding this method, ensure that the implementation
   * does not depend on the instance being initialized or have valid data. This
   * method is called on the client-side when filling up the domains for the
   * properties.
   */
  virtual std::string GetDefaultSeriesLabel(
    const std::string& tableName, const std::string& columnName);

  ///@{
  /**
   * Flatten the table, i.e. split any multicomponent columns into separate
   * components.
   */
  vtkSetMacro(FlattenTable, int);
  vtkGetMacro(FlattenTable, int);
  ///@}

  /**
   * This method is called on the client-side by the vtkPVContextView whenever a
   * new selection is made on all the visible representations in that view.
   * The goal of this method is allow the representations to transform the
   * selection created in the view (which is an id-based selection based on the
   * vtkTable that is fed into the vtkChart) to an appropriate selection based
   * on the data going into the representation.
   * Return false if the selection is not applicable to this representation or
   * the conversion cannot be made.
   * Default implementation simply ensures that the FieldType on the
   * selection nodes is set to match up with the FieldAssociation on the
   * representation.
   */
  virtual bool MapSelectionToInput(vtkSelection* sel);

  /**
   * This is the inverse of MapSelectionToInput(). In this case, we are
   * converting the selection defined on the input for the representation to a
   * selection that corresponds to elements being rendered in the view.
   * The default implementation checks removes vtkSelectionNode items in sel
   * that don't have the FieldType that matches this->FieldAssociation.
   * Similar to MapSelectionToInput(), this method is expected to transform the
   * sel in place and return false is the selection is not applicable to this
   * representation or the conversion cannot be made.
   */
  virtual bool MapSelectionToView(vtkSelection* sel);

  ///@{
  /**
   * Called by vtkPVContextView::Export() to export the representation's data to
   * a CSV file. Return false on failure which will call the exporting process
   * to abort and raise an error. Default implementation simply returns false.
   */
  virtual bool Export(vtkAbstractChartExporter* vtkNotUsed(exporter)) { return false; }
  ///@}

  enum ArraySelectionModes
  {
    MERGED_BLOCKS = 0,
    INDIVIDUAL_BLOCKS = 1,
  };

  ///@{
  /**
   * ArraySelectionMode controls how arrays are selected for charting in composite datasets.
   * MERGED_BLOCKS means arrays are selected once and applied to all blocks.
   * INDIVIDUAL_BLOCKS means arrays are selected independently for each leaf block.
   */
  vtkGetMacro(ArraySelectionMode, int);
  void SetArraySelectionMode(int mode);
  ///@}

  ///@{
  /**
   * Set/Get the placeholder data type.
   *
   * This value is set by the proxy and is needed to ensure that the client knows the type of the
   * input that lives on the server.
   *
   * In the past, we were always creating a placeholder of type vtkMultiBlockDataSet when no
   * input was available on the client. This is no longer valid.
   *
   * This can be potentially removed in the future once vtkMultiBlockDataSet is deprecated.
   */
  void SetPlaceHolderDataType(int datatype);
  vtkGetMacro(PlaceHolderDataType, int);
  ///@}
protected:
  vtkChartRepresentation();
  ~vtkChartRepresentation() override;

  /**
   * Fill input port information.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * This method is called before actual render if this->MTime was modified
   * since the last time this method was called. Subclasses should override to
   * update "appearance" related changes that don't affect data.
   * When this method is called, you're assured that this->ContextView is
   * valid.
   * Note that this method will not be called if this->GetVisibility()
   * returns false, this subclasses should also override SetVisibility() to
   * hide "actors" and such.
   */
  virtual void PrepareForRendering() {}

  /**
   * Subclasses should override this to connect inputs to the internal pipeline
   * as necessary. Since most representations are "meta-filters" (i.e. filters
   * containing other filters), you should create shallow copies of your input
   * before connecting to the internal pipeline. The convenience method
   * GetInternalOutputPort will create a cached shallow copy of a specified
   * input for you. The related helper functions GetInternalAnnotationOutputPort,
   * GetInternalSelectionOutputPort should be used to obtain a selection or
   * annotation port whose selections are localized for a particular input data object.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

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
   * Convenience method to get the first vtkTable from LocalOutput, if any.
   * If pre_delivery is true (default is false), then the data from most-recent
   * `REQUEST_UPDATE` pass is used otherwise the delivered data object from most recent
   * `REQUEST_RENDER` pass is used.
   */
  vtkTable* GetLocalOutput(bool pre_delivery = false);

  /**
   * Method to be overridden to transform input data.
   * The default implementation just returns the data object provided in parameter.
   */
  virtual vtkSmartPointer<vtkDataObject> TransformInputData(vtkDataObject* data);

  /**
   * Chart representation/views expect that the data is reduced to root node and
   * then passed on the rendering nodes. This method may be overridden to
   * customize the reduction. The default implementation uses
   * vtkBlockDeliveryPreprocessor to convert to tables and then use
   * vtkPVMergeTablesMultiBlock as the reduction algorithm.
   */
  virtual vtkSmartPointer<vtkDataObject> ReduceDataToRoot(vtkDataObject* data);

  /**
   * Method to be overridden to apply an operation of the table after it is
   * gathered to the first rank for rendering the chart.  This allows subclasses
   * to operate on the final table.  The default implementation just returns the
   * input.
   */
  virtual vtkSmartPointer<vtkDataObject> TransformTable(vtkSmartPointer<vtkDataObject> table);

  using MapOfTables = std::map<std::string, std::pair<vtkSmartPointer<vtkTable>, unsigned int>>;
  /**
   * Convenience method to get all vtkTable instances with their associated names.
   */
  bool GetLocalOutput(MapOfTables& tables);

  int FieldAssociation;
  vtkWeakPointer<vtkPVContextView> ContextView;
  int FlattenTable;
  int ArraySelectionMode = MERGED_BLOCKS;

  // This is used to be able to create the correct placeHolder in RequestData for the client
  int PlaceHolderDataType = VTK_PARTITIONED_DATA_SET_COLLECTION;

  vtkSmartPointer<vtkDataObjectTree> LocalOutput;

  char* ActiveAssembly = nullptr;
  std::vector<std::string> BlockSelectors;
  std::set<unsigned int> CompositeIndices; // the selected blocks

  vtkWeakPointer<vtkChartSelectionRepresentation> SelectionRepresentation;

private:
  vtkChartRepresentation(const vtkChartRepresentation&) = delete;
  void operator=(const vtkChartRepresentation&) = delete;

  vtkTimeStamp PrepareForRenderingTime;
  vtkMTimeType LastLocalOutputMTime;
  vtkSmartPointer<vtkChartSelectionRepresentation> DummyRepresentation;
  vtkSmartPointer<vtkDataObjectTree> LocalOutputRequestData;
};

#endif
