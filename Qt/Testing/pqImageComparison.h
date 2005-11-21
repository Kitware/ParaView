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

#include <vtkIOStream.h>

class QString;
class vtkRenderWindow;

bool pqSaveScreenshot(vtkRenderWindow* RenderWindow, const QString& File);
bool pqCompareImage(vtkRenderWindow* RenderWindow, const QString& ReferenceImage, double Threshold, ostream& Output, const QString& TempDirectory);

#endif // !_pqImageComparison_h

