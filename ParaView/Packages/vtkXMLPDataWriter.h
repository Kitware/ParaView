/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPDataWriter.h
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
// .NAME vtkXMLPDataWriter - Write image data in a parallel XML format.
// .SECTION Description
// vtkXMLPDataWriter

#ifndef __vtkXMLPDataWriter_h
#define __vtkXMLPDataWriter_h

#include "vtkXMLWriter.h"

class vtkDataSet;

class VTK_EXPORT vtkXMLPDataWriter : public vtkXMLWriter
{
public:
  vtkTypeRevisionMacro(vtkXMLPDataWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the number of pieces that are being written in parallel.
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  
  // Description:
  // Get/Set the piece number assigned to this writer.
  vtkSetMacro(Piece, int);
  vtkGetMacro(Piece, int);
  
  // Description:
  // Get/Set the ghost level used for this writer's piece.
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);
  
  // Description:
  // Get/Set whether this instance of the writer should write the
  // summary file that refers to all of the pieces' individual files.
  // Default is yes only for piece 0 writer.
  virtual void SetWriteSummaryFile(int flag);
  vtkGetMacro(WriteSummaryFile, int);
  vtkBooleanMacro(WriteSummaryFile, int);
protected:
  vtkXMLPDataWriter();
  ~vtkXMLPDataWriter();
  
  vtkDataSet* GetInputAsDataSet();
  
  virtual vtkXMLWriter* CreatePieceWriter()=0;
  
  int Write();
  void WriteFileAttributes();
  virtual void WritePrimaryElementAttributes();
  int WriteData();
  virtual void WritePData(vtkIndent indent);
  virtual void WritePPieceAttributes(int index);
  
  char* CreatePieceFileName(int index, const char* path=0);
  void SplitFileName();
  int WritePiece();
  
  int Piece;
  int NumberOfPieces;
  int GhostLevel;
  int WriteSummaryFile;
  int WriteSummaryFileInitialized;
  
  char* PathName;
  char* FileNameBase;
  char* FileNameExtension;
  
private:
  vtkXMLPDataWriter(const vtkXMLPDataWriter&);  // Not implemented.
  void operator=(const vtkXMLPDataWriter&);  // Not implemented.
};

#endif
