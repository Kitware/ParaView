/*=========================================================================

  Program:   ParaView
  Module:    vtkPVReaderModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVReaderModule - Module representing a reader
// .SECTION Description
// The class vtkPVReaderModule is used to represent a reader (or a pipeline
// which contains a reader). A prototype for each file type is created at
// startup by parsing the (XML) configuration file. Later, when the user
// tries to open a file, the prototypes are consulted. The first prototype
// which respond positively to CanReadFile() is asked to read the file and
// create a new instance of a vtkPVReaderModule (or any subclass) to be
// added to the list of existing sources.
//
// .SECTION See also
// vtkPVAdvancedReaderModule vtkPVEnSightReaderModule vtkPVPLOT3DReaderModule

#ifndef __vtkPVReaderModule_h
#define __vtkPVReaderModule_h

#include "vtkPVSource.h"

class vtkPVFileEntry;
//BTX
template <class value>
class vtkVector;
template <class value>
class vtkVectorIterator;
//ETX

class VTK_EXPORT vtkPVReaderModule : public vtkPVSource
{
public:
  static vtkPVReaderModule* New();
  vtkTypeRevisionMacro(vtkPVReaderModule, vtkPVSource);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source. The only default element created is a
  // file entry. Of course, more can be added in the configuration file.
  virtual void CreateProperties();

  // Description:
  // Returns true (1) if the current reader can read the specified file,
  // false (0) otherwise. In the default implementation, this is done by
  // comparing the extension of the file to a list of extensions specified
  // by the configuration (XML) file -see AddExtension-.
  virtual int CanReadFile(const char* fname);

  // Description:
  // Used mainly by the scripting interface, these three methods are
  // normally called in order during the file opening process. 
  // InitializeReadCustom() (invoked on the prototype) returns a clone.
  // ReadFileInformation() and FinalizeRead() are then invoked on
  // the clone to finish the reading process. These methods can be
  // changed by custom subclasses which require special handling of
  // ParaView traces.
  virtual int Initialize(const char* fname, vtkPVReaderModule*& newModule);
  virtual int Finalize  (const char* fname);
  virtual int ReadFileInformation(const char* fname);

  // Description:
  // Add extension recognized by the reader. This is displayed in the
  // selection dialog and used in the default implementation of
  // CanReadFile(). 
  void AddExtension(const char*);
  
  // Description:
  // Get the number of registered file extensions.
  vtkIdType GetNumberOfExtensions();
  
  // Description:
  // Get the ith file extension.
  const char* GetExtension(vtkIdType i);

  // Description:
  // Remove the path and return the filename.
  const char* RemovePath(const char* fname);

  // Description:
  // This tells vtkPVWindow whether it should call Accept() on the module
  // returned by ReadFile. In the default implementation, the ReadFile()
  // creates a clone and sets up all the filename (which is the only user
  // modifiable option). Since the user does not have to make any
  // selections before the file is loaded, vtkPVWindow calls Accept instead
  // of the user. This behaviour is changed in vtkPVAdvancedReaderModule
  // which first calls UpdateInformation() on the VTK reader, obtains some
  // preliminary information from the file and then prompts the user for
  // some stuff. In this situation, vtkPVWindow does not call Accept. This
  // is mainly used to avoid reading the whole file before asking the user
  // the initial configuration (for example, the user might want to load
  // only a subset of available attriutes)
  vtkSetMacro(AcceptAfterRead, int);
  vtkGetMacro(AcceptAfterRead, int);

  // Description:
  // Get the file entry.
  vtkGetObjectMacro(FileEntry, vtkPVFileEntry);

  // Description:
  // Saves the pipeline in a ParaView script.  This is similar
  // to saveing a trace, except only the last state is stored.
  virtual void SaveState(ofstream *file);

  // Description:
  // Puts the file entry at the begining of the list for batch files.
  // The file name has to be set before array selection.
  void AddPVFileEntry(vtkPVFileEntry *pvw);
  
  // Description:
  // Get the number of time steps that can be provided by this reader.
  // Timesteps are available either from an animation file or from a
  // time-series of files as detected by the file entry widget.
  // Returns 0 if time steps are not available, and the number of
  // timesteps otherwise.
  virtual int GetNumberOfTimeSteps();
  
  // Description:
  // Set the time step that should be provided by the reader.  This
  // value is ignored unless GetNumberOfTimeSteps returns 1 or more.
  virtual void SetRequestedTimeStep(int);
  
protected:
  vtkPVReaderModule();
  ~vtkPVReaderModule();

  const char* ExtractExtension(const char* fname);

  vtkPVFileEntry* FileEntry;
  int AcceptAfterRead;

  virtual int FinalizeInternal(const char* fname, 
                               int accept);

  void SetReaderFileName(const char* fname);
  
//BTX
  vtkVector<const char*>* Extensions;
  vtkVectorIterator<const char*>* Iterator;
//ETX

  // Description: 
  // Creates and returns (by reference) a copy of this source. It will
  // create a new instance of the same type as the current object using
  // NewInstance() and then call ClonePrototype() on all widgets and add
  // these clones to it's widget list. The return value is VTK_OK is the
  // cloning was successful.
  int CloneAndInitialize(
    int makeCurrent, vtkPVReaderModule*& clone, const char* groupName);

  int PackFileEntry;
  int AddFileEntry;

private:
  vtkPVReaderModule(const vtkPVReaderModule&); // Not implemented
  void operator=(const vtkPVReaderModule&); // Not implemented
};

#endif
