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
#include "vtkClientServerStreamInstantiator.h"
#include "vtkExecutive.h"
#include "vtkGraph.h"
#include "vtkGraphReader.h"
#include "vtkGraphWriter.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSubsetInclusionLattice.h"

vtkStandardNewMacro(vtkPVSILInformation);
vtkCxxSetObjectMacro(vtkPVSILInformation, SIL, vtkGraph);
//----------------------------------------------------------------------------
vtkPVSILInformation::vtkPVSILInformation()
{
  this->RootOnly = 1;
  this->SIL = nullptr;
}

//----------------------------------------------------------------------------
vtkPVSILInformation::~vtkPVSILInformation()
{
  this->SetSIL(nullptr);
}

//----------------------------------------------------------------------------
void vtkPVSILInformation::CopyFromObject(vtkObject* obj)
{
  this->SetSIL(nullptr);
  this->SubsetInclusionLattice = nullptr;

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
    this->SetSIL(vtkGraph::SafeDownCast(info->Get(vtkDataObject::SIL())));
  }
  else if (info && info->Has(vtkSubsetInclusionLattice::SUBSET_INCLUSION_LATTICE()))
  {
    this->SubsetInclusionLattice = vtkSubsetInclusionLattice::SafeDownCast(
      info->Get(vtkSubsetInclusionLattice::SUBSET_INCLUSION_LATTICE()));
  }
}

//----------------------------------------------------------------------------
void vtkPVSILInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;
  if (this->SIL)
  {
    vtkGraph* clone = this->SIL->NewInstance();
    clone->ShallowCopy(this->SIL);

    vtkGraphWriter* writer = vtkGraphWriter::New();
    writer->SetFileTypeToBinary();
    writer->WriteToOutputStringOn();
    writer->SetInputData(clone);
    writer->Write();

    *css << vtkClientServerStream::InsertArray(
      writer->GetBinaryOutputString(), static_cast<int>(writer->GetOutputStringLength()));

    writer->RemoveAllInputs();
    writer->Delete();
    clone->Delete();
  }
  else
  {
    *css << vtkClientServerStream::InsertArray(static_cast<unsigned char*>(NULL), 0);
  }

  if (this->SubsetInclusionLattice)
  {
    *css << this->SubsetInclusionLattice->GetClassName();
    *css << this->SubsetInclusionLattice->Serialize();
  }
  else
  {
    *css << ""
         << "";
  }
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVSILInformation::CopyFromStream(const vtkClientServerStream* css)
{
  this->SetSIL(nullptr);
  this->SubsetInclusionLattice = nullptr;
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
  if (css->GetNumberOfArguments(0) == 3)
  {
    std::string classname, data;
    css->GetArgument(0, 1, &classname);
    css->GetArgument(0, 2, &data);

    vtkSmartPointer<vtkObjectBase> obj;
    obj.TakeReference(vtkClientServerStreamInstantiator::CreateInstance(classname.c_str()));
    if (vtkSubsetInclusionLattice* sil = vtkSubsetInclusionLattice::SafeDownCast(obj))
    {
      this->SubsetInclusionLattice = sil;
    }
    else
    {
      this->SubsetInclusionLattice = vtkSmartPointer<vtkSubsetInclusionLattice>::New();
    }
    this->SubsetInclusionLattice->Deserialize(data);
  }
}

//----------------------------------------------------------------------------
vtkSubsetInclusionLattice* vtkPVSILInformation::GetSubsetInclusionLattice() const
{
  return this->SubsetInclusionLattice.GetPointer();
}

//----------------------------------------------------------------------------
void vtkPVSILInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SIL: " << this->SIL << endl;
}
