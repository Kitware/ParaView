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
 * source to a file with pieces spread across ther server processes.
*/

#ifndef vtkXMLPVDWriter_h
#define vtkXMLPVDWriter_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkXMLWriter.h"

class vtkCallbackCommand;
class vtkXMLPVDWriterInternals;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkXMLPVDWriter : public vtkXMLWriter
{
public:
  static vtkXMLPVDWriter* New();
  vtkTypeMacro(vtkXMLPVDWriter, vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Get the default file extension for files written by this writer.
   */
  virtual const char* GetDefaultFileExtension() VTK_OVERRIDE;

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

  // See the vtkAlgorithm for a desciption of what these do
  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

protected:
  vtkXMLPVDWriter();
  ~vtkXMLPVDWriter();

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  // Replace vtkXMLWriter's writing driver method.
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;
  virtual int WriteData() VTK_OVERRIDE;
  virtual const char* GetDataSetName() VTK_OVERRIDE;

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
  virtual void ReportReferences(vtkGarbageCollector*) VTK_OVERRIDE;

private:
  vtkXMLPVDWriter(const vtkXMLPVDWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLPVDWriter&) VTK_DELETE_FUNCTION;
};

#endif
