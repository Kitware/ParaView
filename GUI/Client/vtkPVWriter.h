/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVWriter - Wraps a VTK file writer.
// .SECTION Description
// vtkPVWriter provides functionality for writers similar to that
// provided by vtkPVReaderModule for readers.  An instance of this
// class is configured by an XML ModuleInterface specification and
// knows how to create and use a single VTK file writer object.

#ifndef __vtkPVWriter_h
#define __vtkPVWriter_h

#include "vtkKWObject.h"

class vtkDataObject;
class vtkPVApplication;
class vtkPVSource;
//BTX
template <class value>
class vtkVector;
template <class value>
class vtkVectorIterator;
//ETX

class VTK_EXPORT vtkPVWriter : public vtkKWObject
{
public:
  static vtkPVWriter* New();
  vtkTypeRevisionMacro(vtkPVWriter,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);  
  
  // Description:
  // Get/Set the name of the vtk data type that this writer can write.
  vtkSetStringMacro(InputClassName);
  vtkGetStringMacro(InputClassName);
  
  // Description:
  // Get/Set the name of the actual class that implements the writer.
  vtkSetStringMacro(WriterClassName);
  vtkGetStringMacro(WriterClassName);
  
  // Description:
  // Get/Set the description of the file type supported by this
  // writer.
  vtkSetStringMacro(Description);
  vtkGetStringMacro(Description);

  // Description:
  // Add extension recognized by the writer. This is displayed in the
  // selection dialog 
  void AddExtension(const char*);
  
  // Description:
  // Get the number of registered file extensions.
  vtkIdType GetNumberOfExtensions();

  // Description:
  // Get the ith file extension.
  const char* GetExtension(vtkIdType i);
  
  // Description:
  // Get/Set whether the file writer is for parallel file formats.
  vtkSetMacro(Parallel, int);
  vtkGetMacro(Parallel, int);
  vtkBooleanMacro(Parallel, int);
  
  // Description:
  // Get/Set the method called to set the writer's data mode.  Default
  // is no method.
  vtkSetStringMacro(DataModeMethod);
  vtkGetStringMacro(DataModeMethod);
  
  // Description:
  // Check whether this writer supports the given VTK data set's type.
  virtual int CanWriteData(vtkDataObject* data, int parallel, int numParts);
  
  // Description:
  // Returns true (1) if the current writer can write to the specified file,
  // false (0) otherwise. In the default implementation, this is done by
  // comparing the extension of the file to a list of extensions specified
  // by the configuration (XML) file -see AddExtension-.
  virtual int CanWriteFile(const char* fname);

  // Description:
  // This just returns the application typecast correctly.
  vtkPVApplication* GetPVApplication();
  
  // Description:
  // Write the data from the given source to the given file name.
  virtual void Write(const char* fileName, vtkPVSource* pvs,
                     int numProcs, int ghostLevel, int timeSeries);
  
protected:
  vtkPVWriter();
  ~vtkPVWriter();
  
  int WriteOneFile(const char* fileName, vtkPVSource* pvs,
                   int numProcs, int ghostLevel);

  const char* ExtractExtension(const char* fname);
  
  char* InputClassName;
  char* WriterClassName;
  char* Description;
//BTX
  vtkVector<const char*>* Extensions;
  vtkVectorIterator<const char*>* Iterator;
//ETX
  int Parallel;
  char* DataModeMethod;
  
private:
  vtkPVWriter(const vtkPVWriter&); // Not implemented
  void operator=(const vtkPVWriter&); // Not implemented
};

#endif
