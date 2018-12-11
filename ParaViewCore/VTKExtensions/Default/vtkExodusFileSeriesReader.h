// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExodusFileSeriesReader.h

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
 * @class   vtkExodusFileSeriesReader
 * @brief   meta-reader to read Exodus file series from simulation restarts
 *
 *
 *
 * Add some special sauce to the superclass that allows it to work with the
 * parallel Exodus reader.  Specifically, changing the file name causes
 * the selected output arrays to be cleared out.  This class saves and
 * restores the information.
 *
*/

#ifndef vtkExodusFileSeriesReader_h
#define vtkExodusFileSeriesReader_h

#include "vtkFileSeriesReader.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkExodusFileSeriesReader : public vtkFileSeriesReader
{
public:
  vtkTypeMacro(vtkExodusFileSeriesReader, vtkFileSeriesReader);
  static vtkExodusFileSeriesReader* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkExodusFileSeriesReader();
  ~vtkExodusFileSeriesReader() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestInformationForInput(
    int index, vtkInformation* request, vtkInformationVector* outputVector) override;

  // Replaces the filenames, which probably represents partitions of the data,
  // with a set of files where each represents a set of solution files for one
  // of the simulation restarts.
  virtual void FindRestartedResults();

private:
  vtkExodusFileSeriesReader(const vtkExodusFileSeriesReader&) = delete;
  void operator=(const vtkExodusFileSeriesReader&) = delete;
};

#endif // vtkExodusFileSeriesReader_h
