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

#include "vtkPVDataRepresentation.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer
#include "vtkSmartPointer.h" // needed for vtkSmartPointer

class vtkAnnotationLink;
class vtkBlockDeliveryPreprocessor;
class vtkClientServerMoveData;
class vtkChartNamedOptions;
class vtkPVCacheKeeper;
class vtkPVContextView;
class vtkReductionFilter;
class vtkSelectionDeliveryFilter;
class vtkTable;

class VTK_EXPORT vtkChartRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkChartRepresentation* New();
  vtkTypeMacro(vtkChartRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  // Forwarded to vtkBlockDeliveryPreprocessor.
  void SetFieldAssociation(int);
  void SetCompositeDataSetIndex(unsigned int);

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
  vtkTable* GetLocalOutput();

  vtkBlockDeliveryPreprocessor* Preprocessor;
  vtkPVCacheKeeper* CacheKeeper;
  vtkReductionFilter* ReductionFilter;
  vtkClientServerMoveData* DeliveryFilter;
  vtkWeakPointer<vtkPVContextView> ContextView;
  vtkChartNamedOptions* Options;

  vtkSelectionDeliveryFilter* SelectionDeliveryFilter;

  vtkAnnotationLink* AnnLink;

  bool EnableServerSideRendering;
  vtkSmartPointer<vtkTable> LocalOutput;

private:
  vtkChartRepresentation(const vtkChartRepresentation&); // Not implemented
  void operator=(const vtkChartRepresentation&); // Not implemented
//ETX
};

#endif
