/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPDataWriter.cxx
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
#include "vtkXMLPDataWriter.h"
#include "vtkDataSet.h"

vtkCxxRevisionMacro(vtkXMLPDataWriter, "1.1");

//----------------------------------------------------------------------------
vtkXMLPDataWriter::vtkXMLPDataWriter()
{
  this->Piece = 0;
  this->NumberOfPieces = 1;
  this->GhostLevel = 0;
  this->WriteSummaryFileInitialized = 0;
  this->WriteSummaryFile = 0;
  
  this->PathName = 0;
  this->FileNameBase = 0;
  this->FileNameExtension = 0;
}

//----------------------------------------------------------------------------
vtkXMLPDataWriter::~vtkXMLPDataWriter()
{
  if(this->PathName) { delete [] this->PathName; }
  if(this->FileNameBase) { delete [] this->FileNameBase; }
  if(this->FileNameExtension) { delete [] this->FileNameExtension; }
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << "\n";
}

//----------------------------------------------------------------------------
vtkDataSet* vtkXMLPDataWriter::GetInputAsDataSet()
{
  if(this->NumberOfInputs < 1)
    {
    return 0;
    }
  
  return static_cast<vtkDataSet*>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::SetWriteSummaryFile(int flag)
{
  this->WriteSummaryFileInitialized = 1;
  vtkDebugMacro(<< this->GetClassName() << " ("
                << this << "): setting WriteSummaryFile to " << flag);
  if(this->WriteSummaryFile != flag)
    {
    this->WriteSummaryFile = flag;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkXMLPDataWriter::Write()
{
  // Make sure we have a file to write.
  if(!this->FileName)
    {
    vtkErrorMacro("Write() called with no FileName set.");
    return 0;
    }
  
  // Prepare the file name.
  this->SplitFileName();
  
  // Make sure our input's information is up to date.
  vtkDataSet* input = this->GetInputAsDataSet();  
  input->UpdateInformation();
  
  // Write the piece now so the data are up to date.
  this->WritePiece();
  
  // Decide whether to write the summary file.
  int writeSummary = 0;
  if(this->WriteSummaryFileInitialized)
    {
    writeSummary = this->WriteSummaryFile;
    }
  else if(this->Piece == 0)
    {
    writeSummary = 1;
    }
  
  // Write the summary file if requested.
  if(writeSummary)
    {
    if(!this->Superclass::Write()) { return 0; }
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::WriteFileAttributes()
{
  this->Superclass::WriteFileAttributes();
  ostream& os = *(this->Stream);
  os << " type=\"" << this->GetDataSetName() << "\""; 
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::WritePrimaryElementAttributes()
{
  this->WriteScalarAttribute("GhostLevel", this->GhostLevel);  
}

//----------------------------------------------------------------------------
int vtkXMLPDataWriter::WriteData()
{
  // Write the summary file.
  ostream& os = *(this->Stream);
  vtkIndent indent;
  vtkIndent nextIndent = indent.GetNextIndent();
  
  this->StartFile();
  
  os << indent << "<" << this->GetDataSetName();
  this->WritePrimaryElementAttributes();
  os << ">\n";
  
  // Write the information needed for a reader to produce the output's
  // information during UpdateInformation without reading a piece.
  this->WritePData(indent.GetNextIndent());
  
  // Write the elements referencing each piece and its file.
  int i;
  for(i=0;i < this->NumberOfPieces; ++i)
    {
    os << nextIndent << "<Piece";
    this->WritePPieceAttributes(i);
    os << "/>\n";
    }
  
  os << indent << "</" << this->GetDataSetName() << ">\n";
  
  this->EndFile();
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::WritePData(vtkIndent indent)
{
  vtkDataSet* input = this->GetInputAsDataSet();  
  this->WritePPointData(input->GetPointData(), indent);
  this->WritePCellData(input->GetCellData(), indent);
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::WritePPieceAttributes(int index)
{
  char* fileName = this->CreatePieceFileName(index);
  this->WriteStringAttribute("Source", fileName);
  delete [] fileName;
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::SplitFileName()
{
  // Split the FileName into its PathName, FileNameBase, and
  // FileNameExtension components.
  int length = strlen(this->FileName);
  char* fileName = new char[length+1];
  strcpy(fileName, this->FileName);
  char* begin = fileName;
  char* end = fileName + length;
  char* s;
  
#if defined(_WIN32)
  // Convert to UNIX-style slashes.
  for(s=begin;s != end;++s) { if(*s == '\\') { *s = '/'; } }
#endif
  
  // Extract the path name up to the last '/'.
  if(this->PathName) { delete [] this->PathName; this->PathName = 0; }
  char* rbegin = end-1;
  char* rend = begin-1;
  for(s=rbegin;s != rend;--s) { if(*s == '/') { break; } }
  if(s >= begin)
    {
    length = (s-begin)+1;
    this->PathName = new char[length+1];
    strncpy(this->PathName, this->FileName, length);
    this->PathName[length] = '\0';
    begin = s+1;
    }
  
  // "begin" now points at the beginning of the file name.
  // Look for the first "." to pull off the longest extension.
  if(this->FileNameExtension)
    { delete [] this->FileNameExtension; this->FileNameExtension = 0; }
  for(s=begin; s != end; ++s) { if(*s == '.') { break; } }
  if(s < end)
    {
    length = end-s;
    this->FileNameExtension = new char[length+1];
    strncpy(this->FileNameExtension, s, length);
    this->FileNameExtension[length] = '\0';
    end = s;
    }
  
  // "end" now points to end of the file name.
  if(this->FileNameBase) { delete [] this->FileNameBase; }
  length = end-begin;
  this->FileNameBase = new char[length+1];
  strncpy(this->FileNameBase, begin, length);
  this->FileNameBase[length] = '\0';
  
  // Cleanup temporary name.
  delete [] fileName;
}

//----------------------------------------------------------------------------
char* vtkXMLPDataWriter::CreatePieceFileName(int index, const char* path)
{
  ostrstream fn;
  if(path) { fn << path; }
  fn << this->FileNameBase << index;
  if(this->FileNameExtension) { fn << this->FileNameExtension; }
  fn << ends;
  return fn.str();
}

//----------------------------------------------------------------------------
int vtkXMLPDataWriter::WritePiece()
{
  // Construct the output filename.
  char* fileName = this->CreatePieceFileName(this->Piece, this->PathName);
  
  // Create the writer for the piece.  Its configuration should match
  // our own writer.
  vtkXMLWriter* pWriter = this->CreatePieceWriter();
  pWriter->SetFileName(fileName);
  pWriter->SetCompressor(this->Compressor);
  pWriter->SetDataMode(this->DataMode);
  pWriter->SetByteOrder(this->ByteOrder);
  pWriter->SetEncodeAppendedData(this->EncodeAppendedData);
  
  // Write the piece.
  int result = pWriter->Write();
  
  // Cleanup.
  pWriter->Delete();
  delete [] fileName;
  
  return result;
}
