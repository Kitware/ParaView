/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLPVDWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPVDWriter
 * @brief   Data writer for ParaView
 *
 * vtkXMLPVDWriter is used to save all parts of a current
 * source to a file with pieces spread across other server processes.
*/

#ifndef vtkXMLPVDWriter_h
#define vtkXMLPVDWriter_h

#include "vtkPVVTKExtensionsIOCoreModule.h" //needed for exports
#include "vtkXMLWriter.h"

class vtkCallbackCommand;
class vtkXMLPVDWriterInternals;

class VTKPVVTKEXTENSIONSIOCORE_EXPORT vtkXMLPVDWriter : public vtkXMLWriter
{
public:
  static vtkXMLPVDWriter* New();
  vtkTypeMacro(vtkXMLPVDWriter, vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

  //@{
  /**
   * Get/Set the piece number to write.  The same piece number is used
   * for all inputs.
   */
  vtkGetMacro(Piece, int);
  vtkSetMacro(Piece, int);
  //@}

  //@{
  /**
   * Get/Set the number of pieces into which the inputs are split.
   */
  vtkGetMacro(NumberOfPieces, int);
  vtkSetMacro(NumberOfPieces, int);
  //@}

  //@{
  /**
   * Get/Set the number of ghost levels to be written for unstructured
   * data.
   */
  vtkGetMacro(GhostLevel, int);
  vtkSetMacro(GhostLevel, int);
  //@}

  //@{
  /**
   * When WriteAllTimeSteps is turned ON, the writer is executed once for
   * each timestep available from its input. The default is OFF.
   */
  vtkSetMacro(WriteAllTimeSteps, int);
  vtkGetMacro(WriteAllTimeSteps, int);
  vtkBooleanMacro(WriteAllTimeSteps, int);
  //@}

  /**
   * Add an input of this algorithm.
   */
  void AddInputData(vtkDataObject*);

  //@{
  /**
   * Get/Set whether this instance will write the main collection
   * file.
   */
  vtkGetMacro(WriteCollectionFile, int);
  virtual void SetWriteCollectionFile(int flag);
  //@}

  // See the vtkAlgorithm for a description of what these do
  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkXMLPVDWriter();
  ~vtkXMLPVDWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // add in request update extent to set time step information
  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  // Replace vtkXMLWriter's writing driver method.
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int WriteData() override;
  const char* GetDataSetName() override;

  // Methods to create the set of writers matching the set of inputs.
  void CreateWriters();
  vtkXMLWriter* GetWriter(int index);

  // Methods to help construct internal file names.
  void SplitFileName();
  const char* GetFilePrefix();
  const char* GetFilePath();

  // Methods to construct the list of entries for the collection file.
  void AppendEntry(const char* entry);
  void DeleteAllEntries();

  // Write the collection file if it is requested.
  int WriteCollectionFileIfRequested();

  // Make a directory.
  void MakeDirectory(const char* name);

  // Remove a directory.
  void RemoveADirectory(const char* name);

  // Internal implementation details.
  vtkXMLPVDWriterInternals* Internal;

  // The piece number to write.
  int Piece;

  // The number of pieces into which the inputs are split.
  int NumberOfPieces;

  // The number of ghost levels to write for unstructured data.
  int GhostLevel;

  // Whether to write the collection file on this node.
  int WriteCollectionFile;
  int WriteCollectionFileInitialized;

  // Callback registered with the ProgressObserver.
  static void ProgressCallbackFunction(vtkObject*, unsigned long, void*, void*);
  // Progress callback from internal writer.
  virtual void ProgressCallback(vtkAlgorithm* w);

  // The observer to report progress from the internal writer.
  vtkCallbackCommand* ProgressObserver;

  // Garbage collection support.
  void ReportReferences(vtkGarbageCollector*) override;

  // The current time step for time series inputs.
  int CurrentTimeIndex;

  // Option to write all time steps (ON) or just the current one (OFF)
  int WriteAllTimeSteps;

private:
  vtkXMLPVDWriter(const vtkXMLPVDWriter&) = delete;
  void operator=(const vtkXMLPVDWriter&) = delete;
};

#endif
