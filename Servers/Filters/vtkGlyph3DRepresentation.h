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
// .NAME vtkGlyph3DRepresentation
// .SECTION Description
// vtkGlyph3DRepresentation is a representation that uses the vtkGlyph3DMapper
// for rendering glyphs.

#ifndef __vtkGlyph3DRepresentation_h
#define __vtkGlyph3DRepresentation_h

#include "vtkGeometryRepresentation.h"

class vtkGlyph3DMapper;
class vtkPVArrowSource;

class VTK_EXPORT vtkGlyph3DRepresentation : public vtkGeometryRepresentation
{
public:
  static vtkGlyph3DRepresentation* New();
  vtkTypeMacro(vtkGlyph3DRepresentation, vtkGeometryRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Toggle the visibility of the original mesh.
  // If this->GetVisibility() is false, then this has no effect.
  void SetMeshVisibility(bool visible);

  // Description:
  // Get/Set the visibility for this representation. When the visibility of
  // representation of false, all view passes are ignored.
  virtual void SetVisibility(bool);

  //**************************************************************************
  // Forwarded to vtkGlyph3DMapper
  void SetMaskArray(const char* val);
  void SetScaleArray(const char* val);
  void SetOrientationArray(const char* val);
  void SetScaling(int val);
  void SetScaleMode(int val);
  void SetScaleFactor(double val);
  void SetOrient(int val);
  void SetOrientationMode(int val);
  void SetMasking(int val);
  double* GetBounds();

//BTX
protected:
  vtkGlyph3DRepresentation();
  ~vtkGlyph3DRepresentation();

  // Description:
  // Fill input port information.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  virtual int RequestData(vtkInformation*,
    vtkInformationVector**, vtkInformationVector*);

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
  // Used in ConvertSelection to locate the prop used for actual rendering.
  virtual vtkPVLODActor* GetRenderedProp()
    { return this->GlyphActor; }

  vtkGlyph3DMapper* GlyphMapper;
  vtkGlyph3DMapper* LODGlyphMapper;
  vtkPVLODActor* GlyphActor;
  vtkQuadricClustering* GlyphDecimator;
  vtkUnstructuredDataDeliveryFilter* DataCollector;
  vtkUnstructuredDataDeliveryFilter* LODDataCollector;
  vtkPVArrowSource* DummySource;

  bool MeshVisibility;

private:
  vtkGlyph3DRepresentation(const vtkGlyph3DRepresentation&); // Not implemented
  void operator=(const vtkGlyph3DRepresentation&); // Not implemented
//ETX
};

#endif
