/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVScalarBarRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

/**
 * @class   vtkPVScalarBarRepresentation
 * @brief   Represent scalar bar for vtkScalarBarWidget.
 *
 * Subclass of vtkScalarBarRepresentation that considers viewport size
 * for placement of the representation.
 */

#ifndef vtkPVScalarBarRepresentation_h
#define vtkPVScalarBarRepresentation_h

#include "vtkRemotingViewsModule.h" // needed for export macro

#include "vtkScalarBarRepresentation.h"

class VTKREMOTINGVIEWS_EXPORT vtkPVScalarBarRepresentation : public vtkScalarBarRepresentation
{
public:
  vtkTypeMacro(vtkPVScalarBarRepresentation, vtkScalarBarRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPVScalarBarRepresentation* New();

  /**
   * Override to obtain viewport size and potentially adjust placement
   * of the representation.
   */
  int RenderOverlay(vtkViewport*) override;

protected:
  vtkPVScalarBarRepresentation() = default;
  ~vtkPVScalarBarRepresentation() override = default;

private:
  vtkPVScalarBarRepresentation(const vtkPVScalarBarRepresentation&) = delete;
  void operator=(const vtkPVScalarBarRepresentation&) = delete;
};

#endif // vtkPVScalarBarRepresentation_h
