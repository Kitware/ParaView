/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSILInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSILInformation.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkClientServerStream.h"
#include "vtkExecutive.h"
#include "vtkGraph.h"
#include "vtkGraphReader.h"
#include "vtkGraphWriter.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVSILInformation);
vtkCxxSetObjectMacro(vtkPVSILInformation, SIL, vtkGraph);
//----------------------------------------------------------------------------
vtkPVSILInformation::vtkPVSILInformation()
{
  this->RootOnly = 1;
  this->SIL = 0;
}

//----------------------------------------------------------------------------
vtkPVSILInformation::~vtkPVSILInformation()
{
  this->SetSIL(0);
}

//----------------------------------------------------------------------------
void vtkPVSILInformation::CopyFromObject(vtkObject* obj)
{
  this->SetSIL(0);

  vtkAlgorithmOutput* algOutput = vtkAlgorithmOutput::SafeDownCast(obj);
  if (!algOutput)
  {
    vtkAlgorithm* alg = vtkAlgorithm::SafeDownCast(obj);
    if (alg)
    {
      algOutput = alg->GetOutputPort(0);
    }
  }
  if (!algOutput)
  {
    vtkErrorMacro("Information can only be gathered from a vtkAlgorithmOutput.");
    return;
  }

  vtkAlgorithm* reader = algOutput->GetProducer();
  // Since vtkPVSILInformation is RootOnly information object, calling
  // vtkAlgorithm::UpdateInformation() is scary since if the algorithm does
  // indeed update the information, it will only happen on the root node and
  // cause deadlocks (e.g. pvcs.GridConnectivity). Since information objects are
  // only expected to gather information currently available, we shouldn't be
  // calling this UpdateInformation() in the first place.
  //
  // reader->UpdateInformation();

  vtkInformation* info = reader->GetExecutive()->GetOutputInformation(algOutput->GetIndex());

  if (info && info->Has(vtkDataObject::SIL()))
  {
    vtkGraph* sil = vtkGraph::SafeDownCast(info->Get(vtkDataObject::SIL()));
    this->SetSIL(sil);
  }
}

//----------------------------------------------------------------------------
void vtkPVSILInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  if (!this->SIL)
  {
    *css << vtkClientServerStream::Reply
         << vtkClientServerStream::InsertArray(static_cast<unsigned char*>(NULL), 0)
         << vtkClientServerStream::End;
    return;
  }

  vtkGraph* clone = this->SIL->NewInstance();
  clone->ShallowCopy(this->SIL);

  vtkGraphWriter* writer = vtkGraphWriter::New();
  writer->SetFileTypeToBinary();
  writer->WriteToOutputStringOn();
  writer->SetInputData(clone);
  writer->Write();

  *css << vtkClientServerStream::Reply
       << vtkClientServerStream::InsertArray(
            writer->GetBinaryOutputString(), writer->GetOutputStringLength())
       << vtkClientServerStream::End;
  writer->RemoveAllInputs();
  writer->Delete();
  clone->Delete();
}

//----------------------------------------------------------------------------
void vtkPVSILInformation::CopyFromStream(const vtkClientServerStream* css)
{
  this->SetSIL(0);
  vtkTypeUInt32 length;
  if (css->GetArgumentLength(0, 0, &length) && length > 0)
  {
    unsigned char* raw_data = new unsigned char[length];
    css->GetArgument(0, 0, raw_data, length);
    vtkGraphReader* reader = vtkGraphReader::New();
    reader->SetBinaryInputString(reinterpret_cast<const char*>(raw_data), length);
    reader->ReadFromInputStringOn();
    delete[] raw_data;
    reader->Update();
    this->SetSIL(reader->GetOutput());
    reader->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkPVSILInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SIL: " << this->SIL << endl;
}
