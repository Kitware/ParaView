/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVAnimationWriter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAnimationWriter - Data writer for ParaView
// .SECTION Description
// vtkPVAnimationWriter is used to save all parts of a current
// source to a file with pieces spread across ther server processes.

#ifndef __vtkPVAnimationWriter_h
#define __vtkPVAnimationWriter_h

#include "vtkXMLWriter.h"

class vtkPVAnimationWriterInternals;
class vtkPolyData;

class VTK_EXPORT vtkPVAnimationWriter : public vtkXMLWriter
{
public:
  static vtkPVAnimationWriter* New();
  vtkTypeRevisionMacro(vtkPVAnimationWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent);  
  
  // Description:
  // Add an input corresponding to a named group.  Multiple inputs
  // with the same group name will be considered multiple parts of the
  // same group.
  void AddInput(vtkPolyData*, const char* group);
  
  // Description:
  // Start a new animation with the current set of inputs.
  void Start();
  
  // Description:
  // Write the current time step.
  void WriteTime(double time);
  
  // Description:
  // Finish an animation by writing the collection file.
  void Finish();
  
  // Description:
  // Get the default file extension for files written by this writer.
  virtual const char* GetDefaultFileExtension();
  
  // Description:
  // Get/Set the piece number to write.  The same piece number is used
  // for all inputs.
  vtkGetMacro(Piece, int);
  vtkSetMacro(Piece, int);
  
  // Description:
  // Get/Set the number of pieces into which the inputs are split.
  vtkGetMacro(NumberOfPieces, int);
  vtkSetMacro(NumberOfPieces, int);
  
  // Description:
  // Get/Set whether this instance will write the main animation
  // file.
  vtkGetMacro(WriteAnimationFile, int);
  virtual void SetWriteAnimationFile(int flag);
protected:
  vtkPVAnimationWriter();
  ~vtkPVAnimationWriter();  
  
  // Override vtkProcessObject's AddInput method to prevent compiler
  // warnings.
  virtual void AddInput(vtkDataObject*);

  // Replace vtkXMLWriter's writing driver method.
  virtual int WriteInternal();
  virtual int WriteData();
  virtual const char* GetDataSetName();
  
  // Utility methods.
  void SplitFileName();
  void CreateWriters();
  
  // The piece assigned to this process.
  int Piece;
  
  // The total number of pieces across all processes.
  int NumberOfPieces;
  
  // Status safety check for method call ordering.
  int StartCalled;
  int FinishCalled;
  
  // Whether to write the animation file on this node.
  int WriteAnimationFile;
  int WriteAnimationFileInitialized;
  
  // Internal implementation details.
  vtkPVAnimationWriterInternals* Internal;
  
private:
  vtkPVAnimationWriter(const vtkPVAnimationWriter&);  // Not implemented.
  void operator=(const vtkPVAnimationWriter&);  // Not implemented.
};

#endif
