/*=========================================================================

  Program:   ParaView
  Module:    vtkSpyPlotFileSeriesReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2014 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef vtkSpyPlotFileSeriesReader_h
#define vtkSpyPlotFileSeriesReader_h

#include "vtkFileSeriesReader.h"
#include "vtkPVVTKExtensionsIOSPCTHModule.h" //needed for exports

/// vtkSpyPlotFileSeriesReader extends vtkFileSeriesReader to change the number
/// of output ports on this reader. Based on whether markers support was
/// enabled, this reader will have 2 or 3 output ports.
class VTKPVVTKEXTENSIONSIOSPCTH_EXPORT vtkSpyPlotFileSeriesReader : public vtkFileSeriesReader
{
public:
  static vtkSpyPlotFileSeriesReader* New();
  vtkTypeMacro(vtkSpyPlotFileSeriesReader, vtkFileSeriesReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSpyPlotFileSeriesReader();
  ~vtkSpyPlotFileSeriesReader() override;

private:
  vtkSpyPlotFileSeriesReader(const vtkSpyPlotFileSeriesReader&) = delete;
  void operator=(const vtkSpyPlotFileSeriesReader&) = delete;
};

#endif /* vtkSpyPlotFileSeriesReader_h */
