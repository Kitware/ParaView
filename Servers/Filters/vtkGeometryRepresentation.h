/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGeometryRepresentation - representation for showing any datasets as
// external shell of polygons.
// .SECTION Description
// vtkGeometryRepresentation is a representation for showing polygon geometry.
// It handles non-polygonal datasets by extracting external surfaces. One can
// use this representation to show surface/wireframe/points/surface-with-edges.

#ifndef __vtkGeometryRepresentation_h
#define __vtkGeometryRepresentation_h

#include "vtkPVDataRepresentation.h"
#include "vtkProperty.h" // needed for VTK_POINTS etc.

class vtkCompositePolyDataMapper2;
class vtkOrderedCompositeDistributor;
class vtkPVCacheKeeper;
class vtkPVGeometryFilter;
class vtkPVLODActor;
class vtkQuadricClustering;
class vtkScalarsToColors;
class vtkTexture;
class vtkUnstructuredDataDeliveryFilter;

class VTK_EXPORT vtkGeometryRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkGeometryRepresentation* New();
  vtkTypeMacro(vtkGeometryRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
  // typically called by the vtkView to request meta-data from the
  // representations or ask them to perform certain tasks e.g.
  // PrepareForRendering.
  virtual int ProcessViewRequest(vtkInformationRequestKey* request_type,
    vtkInformation* inInfo, vtkInformation* outInfo);

  // Description:
  // This needs to be called on all instances of vtkGeometryRepresentation when
  // the input is modified. This is essential since the geometry filter does not
  // have any real-input on the client side which messes with the Update
  // requests.
  virtual void MarkModified();

  // This is same a vtkDataObject::FieldAssociation types so you can use those
  // as well.
  enum AttributeTypes
    {
    POINT_DATA=0,
    CELL_DATA=1
    };

  // Description:
  // Get/Set the visibility for this representation. When the visibility of
  // representation of false, all view passes are ignored.
  virtual void SetVisibility(bool val);

  // Description:
  // Enable/Disable LOD;
  void SetSuppressLOD(bool suppress)
    { this->SuppressLOD = suppress; }

  // Description:
  // Methods to control scalar coloring. ColorAttributeType defines the
  // attribute type.
  vtkSetMacro(ColorAttributeType, int);
  vtkGetMacro(ColorAttributeType, int);

  // Description:
  // Pick the array to color with.
  vtkSetStringMacro(ColorArrayName);
  vtkGetStringMacro(ColorArrayName);

  // Description:
  // Set the lighting properties of the object. vtkGeometryRepresentation
  // overrides these based of the following conditions:
  // \li When Representation is wireframe or points, it disables diffuse or
  // specular.
  // \li When scalar coloring is employed, it disabled specular.
  vtkSetMacro(Ambient, double);
  vtkSetMacro(Diffuse, double);
  vtkSetMacro(Specular, double);
  vtkGetMacro(Ambient, double);
  vtkGetMacro(Diffuse, double);
  vtkGetMacro(Specular, double);

  enum RepresentationTypes
    {
    POINTS = VTK_POINTS,
    WIREFRAME = VTK_WIREFRAME,
    SURFACE = VTK_SURFACE,
    SURFACE_WITH_EDGES = 3
    };

  // Description:
  // Set the representation type. This adds VTK_SURFACE_WITH_EDGES to those
  // defined in vtkProperty.
  vtkSetClampMacro(Representation, int, POINTS, SURFACE_WITH_EDGES);
  vtkGetMacro(Representation, int);

  // Description:
  // Returns the data object that is rendered from the given input port.
  virtual vtkDataObject* GetRenderedDataObject(int port);

  //***************************************************************************
  // Forwarded to vtkPVGeometryFilter
  void SetUseOutline(int);

  //***************************************************************************
  // Forwarded to vtkProperty.
  virtual void SetAmbientColor(double r, double g, double b);
  virtual void SetColor(double r, double g, double b);
  virtual void SetDiffuseColor(double r, double g, double b);
  virtual void SetEdgeColor(double r, double g, double b);
  virtual void SetInterpolation(int val);
  virtual void SetLineWidth(double val);
  virtual void SetOpacity(double val);
  virtual void SetPointSize(double val);
  virtual void SetSpecularColor(double r, double g, double b);
  virtual void SetSpecularPower(double val);

  //***************************************************************************
  // Forwarded to Actor.
  virtual void SetOrientation(double, double, double);
  virtual void SetOrigin(double, double, double);
  virtual void SetPickable(int val);
  virtual void SetPosition(double, double, double);
  virtual void SetScale(double, double, double);
  virtual void SetTexture(vtkTexture*);

  //***************************************************************************
  // Forwarded to Mapper and LODMapper.
  virtual void SetInterpolateScalarsBeforeMapping(int val);
  virtual void SetLookupTable(vtkScalarsToColors* val);
  virtual void SetMapScalars(int val);
  virtual void SetStatic(int val);

  // Description:
  // Convert the selection to a type appropriate for sharing with other
  // representations through vtkAnnotationLink, possibly using the view.
  // For the superclass, we just return the same selection.
  // Subclasses may do something more fancy, like convert the selection
  // from a frustrum to a list of pedigree ids.  If the selection cannot
  // be applied to this representation, return NULL.
  virtual vtkSelection* ConvertSelection(vtkView* view, vtkSelection* selection);

//BTX
protected:
  vtkGeometryRepresentation();
  ~vtkGeometryRepresentation();

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

  // Description:
  // Produce meta-data about this representation that the view may find useful.
  virtual bool GenerateMetaData(vtkInformation*, vtkInformation*);

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
  // Passes on parameters to vtkProperty and vtkMapper
  virtual void UpdateColoringParameters();

  // Description:
  // Used in ConvertSelection to locate the prop used for actual rendering.
  virtual vtkPVLODActor* GetRenderedProp()
    { return this->Actor; }

  // Description:
  // Overridden to check with the vtkPVCacheKeeper to see if the key is cached.
  virtual bool IsCached(double cache_key);

  vtkAlgorithm* GeometryFilter;
  vtkAlgorithm* MultiBlockMaker;
  vtkPVCacheKeeper* CacheKeeper;
  vtkQuadricClustering* Decimator;
  vtkCompositePolyDataMapper2* Mapper;
  vtkCompositePolyDataMapper2* LODMapper;
  vtkPVLODActor* Actor;
  vtkProperty* Property;
  vtkUnstructuredDataDeliveryFilter* DeliveryFilter;
  vtkUnstructuredDataDeliveryFilter* LODDeliveryFilter;
  vtkOrderedCompositeDistributor* Distributor;

  int ColorAttributeType;
  char* ColorArrayName;
  double Ambient;
  double Specular;
  double Diffuse;
  int Representation;
  bool SuppressLOD;

private:
  vtkGeometryRepresentation(const vtkGeometryRepresentation&); // Not implemented
  void operator=(const vtkGeometryRepresentation&); // Not implemented

  friend class vtkSelectionRepresentation;
  char* DebugString;
  vtkSetStringMacro(DebugString);
//ETX
};

#endif

