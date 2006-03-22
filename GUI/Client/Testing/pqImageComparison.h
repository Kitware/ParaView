/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqImageComparison_h
#define _pqImageComparison_h

#include "QtTestingExport.h"
#include <vtkIOStream.h>

class QString;
class vtkRenderWindow;

/// Provides functionality for generating and comparing reference images for regression testing
class QTTESTING_EXPORT pqImageComparison
{
public:
  /// Saves the contents of a render window to a file for later use as a reference image
  static bool SaveScreenshot(vtkRenderWindow* RenderWindow, const QString& File);
  /// Compares the contents of a render window to a reference image, returning true iff the two match within a given threshold
  static bool CompareImage(vtkRenderWindow* RenderWindow, const QString& ReferenceImage, double Threshold, ostream& Output, const QString& TempDirectory);
};

#endif // !_pqImageComparison_h

