/*=========================================================================

  Program:   ParaView
  Module:    vtkChartRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkChartRepresentation
// .SECTION Description
// vtkChartRepresentation is the base representation for charting
// representations. Currently, ParaView's charting views are client-side only
// views that render only on the client side. That being the case, when running
// in client-server mode or in parallel, the data-delivery mode is fixed. Hence,
// unlike representations for 3D views, this representation delivers the data in
// RequestData() itself. This makes it possible for client code to call
// UpdatePipeline() on the representation proxy and then access the delivered
// vtkTable on the client.

#ifndef __vtkChartRepresentation_h
#define __vtkChartRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVDataRepresentation.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer
#include "vtkSmartPointer.h" // needed for vtkSmartPointer
#include <vector> //needed for ivars
#include <set> //needed for ivars

class vtkBlockDeliveryPreprocessor;
class vtkChartNamedOptions;
class vtkChartSelectionRepresentation;
class vtkClientServerMoveData;
class vtkDataObjectTree;
class vtkMultiBlockDataSet;
class vtkPVCacheKeeper;
class vtkPVContextView;
class vtkReductionFilter;
class vtkSelectionDeliveryFilter;
class vtkTable;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkChartRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkChartRepresentation* New();
  vtkTypeMacro(vtkChartRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These must only be set during initialization before adding the
  // representation to any views or calling Update().
  void SetSelectionRepresentation(vtkChartSelectionRepresentation*);

  // Description:
  // Set the options object. This must be done before any other state is
  // updated.
  virtual void SetOptions(vtkChartNamedOptions*);

  // Description:
  // Set visibility of the representation.
  virtual void SetVisibility(bool visible);

  // Description:
  // Get the number of series in this representation
  virtual int GetNumberOfSeries();

  // Description:
  // Get the name of the series with the given index.  Returns 0 if the index
  // is out of range.  The returned pointer is only valid until the next call
  // to GetSeriesName.
  virtual const char* GetSeriesName(int series);

  //Description:
  //Get the names of the series
  void GetSeriesNames(std::vector<const char*>& names);

  // Description:
  // Get the names of the arrays
  void GetArraysNames(std::vector<const char*>& names);

  // Description:
  // Force the chaty to rescale its axes.
  virtual void RescaleChart();

  // Description:
  // This needs to be called on all instances of vtkGeometryRepresentation when
  // the input is modified. This is essential since the geometry filter does not
  // have any real-input on the client side which messes with the Update
  // requests.
  virtual void MarkModified();

  // *************************************************************************
  // methods to control block selection
  void SetFieldAssociation(int);
  void SetCompositeDataSetIndex(unsigned int); //only used for single block selection
  void AddCompositeDataSetIndex(unsigned int);
  void ResetCompositeDataSetIndices();

  // Description:
  // Override because of internal selection representations that need to be
  // initilized as well.
  virtual unsigned int Initialize(unsigned int minIdAvailable, unsigned int maxIdAvailable);


//BTX
protected:
  vtkChartRepresentation();
  ~vtkChartRepresentation();

  // Description:
  // Fill input port information.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Subclasses should override this to connect inputs to the internal pipeline
  // as necessary. Since most representations are "meta-filters" (i.e. filters
  // containing other filters), you should create shallow copies of your input
  // before connecting to the internal pipeline. The convenience method
  // GetInternalOutputPort will create a cached shallow copy of a specified
  // input for you. The related helper functions GetInternalAnnotationOutputPort,
  // GetInternalSelectionOutputPort should be used to obtain a selection or
  // annotation port whose selections are localized for a particular input data object.
  virtual int RequestData(vtkInformation*,
    vtkInformationVector**, vtkInformationVector*);

  virtual int RequestUpdateExtent(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  // Description:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().  Subclasses should override this method.
  // Returns true if the addition succeeds.
  virtual bool AddToView(vtkView* view);

  // Description:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().  Subclasses should override this method.
  // Returns true if the removal succeeds.
  virtual bool RemoveFromView(vtkView* view);

  // Description:
  // Overridden to check with the vtkPVCacheKeeper to see if the key is cached.
  virtual bool IsCached(double cache_key);

  // Description:
  // Returns vtkTable at the local processes.
  bool GetLocalOutput(std::vector<vtkTable*>& tables, std::vector<std::string> *blockNames = NULL);
  void UpdateSeriesNames();

  // Description:
  // Method used recursively to traverse the tree and build the full path name of the blocks.
  void FillTableList(vtkMultiBlockDataSet* tree, std::vector<vtkTable*>& tables, std::vector<std::string> *blockNames = NULL, const char* currentPath = "");

  vtkBlockDeliveryPreprocessor* Preprocessor;
  vtkPVCacheKeeper* CacheKeeper;
  vtkReductionFilter* ReductionFilter;
  vtkClientServerMoveData* DeliveryFilter;
  vtkWeakPointer<vtkPVContextView> ContextView;
  vtkChartNamedOptions* Options;

  bool EnableServerSideRendering;
  vtkSmartPointer<vtkMultiBlockDataSet> LocalOutput;

  std::vector<std::string> SeriesNames; //all series names consistent with local output
  std::set<std::string> ArrayNames; // Similar to array name without block name prefix
  std::set<int> CompositeIndices; //the selected blocks

  vtkChartSelectionRepresentation* SelectionRepresentation;

  // Vars used to Cache GetLocalOutput call when possible
  std::vector<vtkTable*>   LocalOutputTableCache;
  std::vector<std::string> LocalOutputBlockNameCache;
  unsigned long            LocalOutputCacheTimeStamp;

private:
  vtkChartRepresentation(const vtkChartRepresentation&); // Not implemented
  void operator=(const vtkChartRepresentation&); // Not implemented
//ETX
};

#endif
