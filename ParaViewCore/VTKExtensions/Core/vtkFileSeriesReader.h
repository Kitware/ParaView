/*=========================================================================

  Program:   ParaView
  Module:    vtkFileSeriesReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

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
 * @class   vtkFileSeriesReader
 * @brief   meta-reader to read file series
 *
 *
 *
 * vtkFileSeriesReader is a meta-reader that can work with various
 * readers to load file series. To the pipeline, it looks like a reader
 * that supports time. It updates the file name to the internal reader
 * whenever a different time step is requested.
 *
 * If the reader already supports time, then this meta-filter will multiplex the
 * time.  It will union together all the times and forward time requests to the
 * file with the correct time.  Overlaps are handled by requesting data from the
 * file with the upper range the farthest in the future.
 *
 * There are two ways to specify a series of files.  The first way is by adding
 * the filenames one at a time with the AddFileName method.  The second way is
 * by providing a single "meta" file.  This meta file is a simple text file that
 * lists a file per line.  The files can be relative to the meta file.  This
 * method is useful when the actual reader points to a set of files itself.  The
 * UseMetaFile toggles between these two methods of specifying files.
 *
*/

#ifndef vtkFileSeriesReader_h
#define vtkFileSeriesReader_h

#include "vtkMetaReader.h"
#include "vtkPVVTKExtensionsCoreModule.h" //needed for exports

class vtkStringArray;

struct vtkFileSeriesReaderInternals;

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkFileSeriesReader : public vtkMetaReader
{
public:
  static vtkFileSeriesReader* New();
  vtkTypeMacro(vtkFileSeriesReader, vtkMetaReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * All pipeline passes are forwarded to the internal reader. The
   * vtkFileSeriesReader reports time steps in RequestInformation. It
   * updated the file name of the internal in RequestUpdateExtent based
   * on the time step request.
   */
  virtual int ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  /**
   * CanReadFile is forwarded to the internal reader if it supports it.
   */
  virtual int CanReadFile(const char* filename);

  /**
   * Adds names of files to be read. The files are read in the order
   * they are added.
   */
  virtual void AddFileName(const char* fname);

  /**
   * Remove all file names.
   */
  virtual void RemoveAllFileNames();

  /**
   * Returns the number of file names added by AddFileName.
   */
  virtual unsigned int GetNumberOfFileNames();

  /**
   * Returns the name of a file with index idx.
   */
  virtual const char* GetFileName(unsigned int idx);

  const char* GetCurrentFileName();

  //@{
  /**
   * If true, then use the meta file.  False by default.
   */
  vtkGetMacro(UseMetaFile, int);
  vtkSetMacro(UseMetaFile, int);
  vtkBooleanMacro(UseMetaFile, int);
  //@}

  //@{
  /**
   * If true, then treat file series like it does not contain any time step
   * values. False by default.
   */
  vtkGetMacro(IgnoreReaderTime, int);
  vtkSetMacro(IgnoreReaderTime, int);
  vtkBooleanMacro(IgnoreReaderTime, int);
  //@}

protected:
  vtkFileSeriesReader();
  ~vtkFileSeriesReader();

  virtual int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;
  virtual int RequestUpdateExtent(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  virtual int RequestUpdateTime(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  };
  virtual int RequestUpdateTimeDependentInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  };
  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

  virtual int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  /**
   * Make sure the reader's output is set to the given index and, if it changed,
   * run RequestInformation on the reader.
   */
  virtual int RequestInformationForInput(
    int index, vtkInformation* request = NULL, vtkInformationVector* outputVector = NULL);

  /**
   * Reads a metadata file and returns a list of filenames (in filesToRead).  If
   * the file could not be read correctly, 0 is returned.
   */
  virtual int ReadMetaDataFile(
    const char* metafilename, vtkStringArray* filesToRead, int maxFilesToRead = VTK_INT_MAX);

  /**
   * True if use a meta-file, false otherwise
   */
  int UseMetaFile;

  /**
   * Re-reads information from the metadata file, if necessary.
   */
  virtual void UpdateMetaData();

  /**
   * Resets information about TimeRanges. Called in RequestInformation().
   */
  void ResetTimeRanges();

  // Add/Remove filenames without changing the MTime.
  void RemoveAllFileNamesInternal();
  void AddFileNameInternal(const char*);

  int IgnoreReaderTime;

  int ChooseInput(vtkInformation*);

private:
  vtkFileSeriesReader(const vtkFileSeriesReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFileSeriesReader&) VTK_DELETE_FUNCTION;

  vtkFileSeriesReaderInternals* Internal;
};

#endif
