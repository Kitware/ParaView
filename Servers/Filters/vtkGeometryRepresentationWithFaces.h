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
// .NAME vtkGeometryRepresentationWithFaces
// .SECTION Description
// vtkGeometryRepresentationWithFaces extends vtkGeometryRepresentation to add
// support for rendering back and front faces separately (with different
// visibility and properties).

#ifndef __vtkGeometryRepresentationWithFaces_h
#define __vtkGeometryRepresentationWithFaces_h

#include "vtkGeometryRepresentation.h"

class VTK_EXPORT vtkGeometryRepresentationWithFaces : public vtkGeometryRepresentation
{
public:
  static vtkGeometryRepresentationWithFaces* New();
  vtkTypeMacro(vtkGeometryRepresentationWithFaces, vtkGeometryRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  enum RepresentationTypesEx
    {
    FOLLOW_FRONTFACE= vtkGeometryRepresentation::SURFACE_WITH_EDGES+1,
    CULL_BACKFACE,
    CULL_FRONTFACE
    };

  // Description:
  // Get/Set the visibility for this representation. When the visibility of
  // representation of false, all view passes are ignored.
  virtual void SetVisibility(bool val);

  // Description:
  // Set the backface representation.
  vtkSetClampMacro(BackfaceRepresentation, int, POINTS, CULL_FRONTFACE);
  vtkGetMacro(BackfaceRepresentation, int);

  //***************************************************************************
  // Forwaded to vtkProperty(BackfaceProperty)
  void SetBackfaceAmbientColor(double r, double g, double b);
  void SetBackfaceDiffuseColor(double r, double g, double b);
  void SetBackfaceOpacity(double val);

//BTX
protected:
  vtkGeometryRepresentationWithFaces();
  ~vtkGeometryRepresentationWithFaces();

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
  // Produce meta-data about this representation that the view may find useful.
  virtual bool GenerateMetaData(vtkInformation*, vtkInformation*);

  // Description:
  // Passes on parameters to vtkProperty and vtkMapper
  virtual void UpdateColoringParameters();

  vtkCompositePolyDataMapper2* BackfaceMapper;
  vtkCompositePolyDataMapper2* LODBackfaceMapper;
  vtkPVLODActor* BackfaceActor;
  vtkProperty* BackfaceProperty;
  int BackfaceRepresentation;

private:
  vtkGeometryRepresentationWithFaces(const vtkGeometryRepresentationWithFaces&); // Not implemented
  void operator=(const vtkGeometryRepresentationWithFaces&); // Not implemented
//ETX
};

#endif
