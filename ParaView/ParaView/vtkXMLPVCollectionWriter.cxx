/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPVCollectionWriter.cxx
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
#include "vtkXMLPVCollectionWriter.h"

#include "vtkCallbackCommand.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLPDataWriter.h"
#include "vtkXMLPImageDataWriter.h"
#include "vtkXMLPPolyDataWriter.h"
#include "vtkXMLPRectilinearGridWriter.h"
#include "vtkXMLPStructuredGridWriter.h"
#include "vtkXMLPUnstructuredGridWriter.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkXMLRectilinearGridWriter.h"
#include "vtkXMLStructuredGridWriter.h"
#include "vtkXMLUnstructuredGridWriter.h"
#include "vtkXMLWriter.h"

#include <vtkstd/string>
#include <vtkstd/vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkXMLPVCollectionWriter);
vtkCxxRevisionMacro(vtkXMLPVCollectionWriter, "1.4");

class vtkXMLPVCollectionWriterInternals
{
public:
  vtkstd::vector< vtkSmartPointer<vtkXMLWriter> > Writers;
  vtkstd::string FilePath;
  vtkstd::string FilePrefix;
  vtkstd::string CreatePieceFileName(int index, const char* path=0);
};

//----------------------------------------------------------------------------
vtkXMLPVCollectionWriter::vtkXMLPVCollectionWriter()
{
  this->Internal = new vtkXMLPVCollectionWriterInternals;
  this->Piece = 0;
  this->NumberOfPieces = 1;
  this->GhostLevel = 0;
  this->WriteCollectionFileInitialized = 0;
  this->WriteCollectionFile = 0;

  // Setup a callback for the internal writers to report progress.
  this->ProgressObserver = vtkCallbackCommand::New();
  this->ProgressObserver->SetCallback(&vtkXMLPVCollectionWriter::ProgressCallbackFunction);
  this->ProgressObserver->SetClientData(this);
}

//----------------------------------------------------------------------------
vtkXMLPVCollectionWriter::~vtkXMLPVCollectionWriter()
{
  this->ProgressObserver->Delete();
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkXMLPVCollectionWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLPVCollectionWriter::AddInput(vtkDataSet* ds)
{
  this->vtkProcessObject::AddInput(ds);
}

//----------------------------------------------------------------------------
void vtkXMLPVCollectionWriter::SetWriteCollectionFile(int flag)
{
  this->WriteCollectionFileInitialized = 1;
  vtkDebugMacro(<< this->GetClassName() << " ("
                << this << "): setting WriteCollectionFile to " << flag);
  if(this->WriteCollectionFile != flag)
    {
    this->WriteCollectionFile = flag;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkXMLPVCollectionWriter::WriteInternal()
{
  // Prepare file prefix for creation of internal file names.
  this->SplitFileName();
  
  // Create writers for each input.
  this->CreateWriters();
  
  // Decide whether to write the collection file.
  int writeCollection = 0;
  if(this->WriteCollectionFileInitialized)
    {
    writeCollection = this->WriteCollectionFile;
    }
  else if(this->Piece == 0)
    {
    writeCollection = 1;
    }
  
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  
  // Write each input.
  int i;
  for(i=0; i < this->GetNumberOfInputs(); ++i)
    {
    this->SetProgressRange(progressRange, i,
                           this->GetNumberOfInputs()+writeCollection);
    if(vtkXMLWriter* w = this->Internal->Writers[i].GetPointer())
      {
      // Set the file name.
      vtkstd::string fname =
        this->Internal->CreatePieceFileName(i,
                                            this->Internal->FilePath.c_str());
      w->SetFileName(fname.c_str());
      
      w->AddObserver(vtkCommand::ProgressEvent, this->ProgressObserver);
      
      // Write the data.
      w->Write();
      
      w->RemoveObserver(this->ProgressObserver);
      }
    }
  
  // Write the collection file if requested.
  if(writeCollection)
    {
    this->SetProgressRange(progressRange, this->GetNumberOfInputs(),
                           this->GetNumberOfInputs()+writeCollection);
    if(!this->Superclass::WriteInternal()) { return 0; }
    }
  
  return 1;  
}

//----------------------------------------------------------------------------
int vtkXMLPVCollectionWriter::WriteData()
{
  // Write the collection file.
  this->StartFile();
  vtkIndent indent = vtkIndent().GetNextIndent();
  
  // Open the primary element.
  ostream& os = *(this->Stream);
  os << indent << "<" << this->GetDataSetName() << ">\n";
  
  // Write a DataSet entry for each input that was written.
  int i;
  for(i=0; i < this->GetNumberOfInputs(); ++i)
    {
    if(this->Internal->Writers[i].GetPointer())
      {
      vtkstd::string fname = this->Internal->CreatePieceFileName(i);
      os << indent.GetNextIndent()
         << "<DataSet part=\"" << i << "\" file=\""
         << fname.c_str() << "\"/>\n";
      }
    }
  
  // Close the primary element.
  os << indent << "</" << this->GetDataSetName() << ">\n";
  this->EndFile();
  
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkXMLPVCollectionWriter::GetDefaultFileExtension()
{
  return "vtc";
}

//----------------------------------------------------------------------------
const char* vtkXMLPVCollectionWriter::GetDataSetName()
{
  return "Collection";
}

//----------------------------------------------------------------------------
void vtkXMLPVCollectionWriter::CreateWriters()
{
  int i;
  this->Internal->Writers.resize(this->GetNumberOfInputs());
  for(i=0; i < this->GetNumberOfInputs(); ++i)
    {
    // Create a writer based on the type of this input.
    switch (this->Inputs[i]->GetDataObjectType())
      {
      case VTK_POLY_DATA:    
        if(this->NumberOfPieces > 1)
          {
          if(!this->Internal->Writers[i].GetPointer() ||
             (strcmp(this->Internal->Writers[i]->GetClassName(),
                     "vtkXMLPPolyDataWriter") != 0))
            {
            vtkXMLPPolyDataWriter* w = vtkXMLPPolyDataWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
            }
          vtkXMLPPolyDataWriter::SafeDownCast(this->Internal->Writers[i].GetPointer())
            ->SetInput(vtkPolyData::SafeDownCast(this->Inputs[i]));
          }
        else
          {
          if(!this->Internal->Writers[i].GetPointer() ||
             (strcmp(this->Internal->Writers[i]->GetClassName(),
                     "vtkXMLPolyDataWriter") != 0))
            {
            vtkXMLPolyDataWriter* w = vtkXMLPolyDataWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
            }
          vtkXMLPolyDataWriter::SafeDownCast(this->Internal->Writers[i].GetPointer())
            ->SetInput(vtkPolyData::SafeDownCast(this->Inputs[i]));
          }
        break;
      case VTK_STRUCTURED_POINTS:
      case VTK_IMAGE_DATA:
        if(this->NumberOfPieces > 1)
          {
          if(!this->Internal->Writers[i].GetPointer() ||
             (strcmp(this->Internal->Writers[i]->GetClassName(),
                     "vtkXMLPImageDataWriter") != 0))
            {
            vtkXMLPImageDataWriter* w = vtkXMLPImageDataWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
            }
          vtkXMLPImageDataWriter::SafeDownCast(this->Internal->Writers[i].GetPointer())
            ->SetInput(vtkImageData::SafeDownCast(this->Inputs[i]));
          }
        else
          {
          if(!this->Internal->Writers[i].GetPointer() ||
             (strcmp(this->Internal->Writers[i]->GetClassName(),
                     "vtkXMLImageDataWriter") != 0))
            {
            vtkXMLImageDataWriter* w = vtkXMLImageDataWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
            }
          vtkXMLImageDataWriter::SafeDownCast(this->Internal->Writers[i].GetPointer())
            ->SetInput(vtkImageData::SafeDownCast(this->Inputs[i]));
          }
        break;
      case VTK_UNSTRUCTURED_GRID:
        if(this->NumberOfPieces > 1)
          {
          if(!this->Internal->Writers[i].GetPointer() ||
             (strcmp(this->Internal->Writers[i]->GetClassName(),
                     "vtkXMLPUnstructuredGridWriter") != 0))
            {
            vtkXMLPUnstructuredGridWriter* w = vtkXMLPUnstructuredGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
            }
          vtkXMLPUnstructuredGridWriter::SafeDownCast(this->Internal->Writers[i].GetPointer())
            ->SetInput(vtkUnstructuredGrid::SafeDownCast(this->Inputs[i]));
          }
        else
          {
          if(!this->Internal->Writers[i].GetPointer() ||
             (strcmp(this->Internal->Writers[i]->GetClassName(),
                     "vtkXMLUnstructuredGridWriter") != 0))
            {
            vtkXMLUnstructuredGridWriter* w = vtkXMLUnstructuredGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
            }
          vtkXMLUnstructuredGridWriter::SafeDownCast(this->Internal->Writers[i].GetPointer())
            ->SetInput(vtkUnstructuredGrid::SafeDownCast(this->Inputs[i]));
          }
        break;
      case VTK_STRUCTURED_GRID:
        if(this->NumberOfPieces > 1)
          {
          if(!this->Internal->Writers[i].GetPointer() ||
             (strcmp(this->Internal->Writers[i]->GetClassName(),
                     "vtkXMLPStructuredGridWriter") != 0))
            {
            vtkXMLPStructuredGridWriter* w = vtkXMLPStructuredGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
            }
          vtkXMLPStructuredGridWriter::SafeDownCast(this->Internal->Writers[i].GetPointer())
            ->SetInput(vtkStructuredGrid::SafeDownCast(this->Inputs[i]));
          }
        else
          {
          if(!this->Internal->Writers[i].GetPointer() ||
             (strcmp(this->Internal->Writers[i]->GetClassName(),
                     "vtkXMLStructuredGridWriter") != 0))
            {
            vtkXMLStructuredGridWriter* w = vtkXMLStructuredGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
            }
          vtkXMLStructuredGridWriter::SafeDownCast(this->Internal->Writers[i].GetPointer())
            ->SetInput(vtkStructuredGrid::SafeDownCast(this->Inputs[i]));
          }
        break;
      case VTK_RECTILINEAR_GRID:
        if(this->NumberOfPieces > 1)
          {
          if(!this->Internal->Writers[i].GetPointer() ||
             (strcmp(this->Internal->Writers[i]->GetClassName(),
                     "vtkXMLPRectilinearGridWriter") != 0))
            {
            vtkXMLPRectilinearGridWriter* w = vtkXMLPRectilinearGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
            }
          vtkXMLPRectilinearGridWriter::SafeDownCast(this->Internal->Writers[i].GetPointer())
            ->SetInput(vtkRectilinearGrid::SafeDownCast(this->Inputs[i]));
          }
        else
          {
          if(!this->Internal->Writers[i].GetPointer() ||
             (strcmp(this->Internal->Writers[i]->GetClassName(),
                     "vtkXMLRectilinearGridWriter") != 0))
            {
            vtkXMLRectilinearGridWriter* w = vtkXMLRectilinearGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
            }
          vtkXMLRectilinearGridWriter::SafeDownCast(this->Internal->Writers[i].GetPointer())
            ->SetInput(vtkRectilinearGrid::SafeDownCast(this->Inputs[i]));
          }
        break;
      }
    
    // Copy settings to the writer.
    if(vtkXMLWriter* w = this->Internal->Writers[i].GetPointer())
      {
      w->SetDebug(this->GetDebug());
      w->SetByteOrder(this->GetByteOrder());
      w->SetCompressor(this->GetCompressor());
      w->SetBlockSize(this->GetBlockSize());
      w->SetDataMode(this->GetDataMode());
      w->SetEncodeAppendedData(this->GetEncodeAppendedData());
      }
    
    // If this is a parallel writer, set the piece information.
    if(vtkXMLPDataWriter* w = vtkXMLPDataWriter::SafeDownCast(this->Internal->Writers[i].GetPointer()))
      {
      w->SetStartPiece(this->Piece);
      w->SetEndPiece(this->Piece);
      w->SetNumberOfPieces(this->NumberOfPieces);
      w->SetGhostLevel(this->GhostLevel);
      }
    }
}

//----------------------------------------------------------------------------
void vtkXMLPVCollectionWriter::SplitFileName()
{
  vtkstd::string fileName = this->FileName;
  vtkstd::string name;
  
  // Split the file name and extension from the path.
  vtkstd::string::size_type pos = fileName.find_last_of("/\\");
  if(pos != fileName.npos)
    {
    // Keep the slash in the file path.
    this->Internal->FilePath = fileName.substr(0, pos+1);
    name = fileName.substr(pos+1);
    }
  else
    {
    this->Internal->FilePath = "";
    name = fileName;
    }
  
  // Split the extension from the file name.
  pos = name.find_last_of(".");
  if(pos != name.npos)
    {
    this->Internal->FilePrefix = name.substr(0, pos);
    }
  else
    {
    this->Internal->FilePrefix = name;
    }
}

//----------------------------------------------------------------------------
vtkstd::string
vtkXMLPVCollectionWriterInternals::CreatePieceFileName(int index,
                                                       const char* path)
{
  vtkstd::string fname;
  ostrstream fn_with_warning_C4701;
  if(path)
    {
    fn_with_warning_C4701 << path;
    }
  fn_with_warning_C4701
    << this->FilePrefix.c_str() << "_" << index << "."
    << this->Writers[index]->GetDefaultFileExtension() << ends;
  fname = fn_with_warning_C4701.str();
  fn_with_warning_C4701.rdbuf()->freeze(0);
  return fname;
}

//----------------------------------------------------------------------------
void vtkXMLPVCollectionWriter::ProgressCallbackFunction(vtkObject* caller,
                                                        unsigned long,
                                                        void* clientdata,
                                                        void*)
{
  vtkProcessObject* w = vtkProcessObject::SafeDownCast(caller);
  if(w)
    {
    reinterpret_cast<vtkXMLPVCollectionWriter*>(clientdata)->ProgressCallback(w);
    }
}

//----------------------------------------------------------------------------
void vtkXMLPVCollectionWriter::ProgressCallback(vtkProcessObject* w)
{
  float width = this->ProgressRange[1]-this->ProgressRange[0];
  float internalProgress = w->GetProgress();
  float progress = this->ProgressRange[0] + internalProgress*width;
  this->UpdateProgressDiscrete(progress);
  if(this->AbortExecute)
    {
    w->SetAbortExecute(1);
    }
}
