// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDistributedTrivialProducer.h"

#include "vtkDataObject.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <map>
#include <string>
#include <vector>

//----------------------------------------------------------------------------
// Internal Static data structure
//----------------------------------------------------------------------------
struct vtkPVTrivialProducerStaticInternal
{
  std::map<std::string, vtkSmartPointer<vtkDataObject>> RegisteredDataObjectMap;

  bool HasKey(const char* key)
  {
    if (key == nullptr)
    {
      return false;
    }
    return this->RegisteredDataObjectMap.find(key) != this->RegisteredDataObjectMap.end();
  }

  vtkDataObject* GetDataObject(const char* key)
  {
    if (this->HasKey(key))
    {
      return this->RegisteredDataObjectMap[key].GetPointer();
    }
    return nullptr;
  }

  void Print(ostream& os, vtkIndent indent)
  {
    std::map<std::string, vtkSmartPointer<vtkDataObject>>::iterator iter =
      this->RegisteredDataObjectMap.begin();
    while (iter != this->RegisteredDataObjectMap.end())
    {
      os << indent << iter->first.c_str() << "\n";
      iter->second.GetPointer()->PrintSelf(os, indent.GetNextIndent());
    }
  }
};
static vtkPVTrivialProducerStaticInternal Value;
vtkPVTrivialProducerStaticInternal* vtkDistributedTrivialProducer::InternalStatic = &Value;
//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkDistributedTrivialProducer);
//----------------------------------------------------------------------------
vtkDistributedTrivialProducer::vtkDistributedTrivialProducer() = default;

//----------------------------------------------------------------------------
vtkDistributedTrivialProducer::~vtkDistributedTrivialProducer() = default;
//----------------------------------------------------------------------------
void vtkDistributedTrivialProducer::SetGlobalOutput(const char* key, vtkDataObject* output)
{
  if (key)
  {
    vtkDistributedTrivialProducer::InternalStatic->RegisteredDataObjectMap[key] = output;
    vtkLog(TRACE, << "Set Global Dataset for " << key);
  }
}

//----------------------------------------------------------------------------
void vtkDistributedTrivialProducer::ReleaseGlobalOutput(const char* key)
{
  if (key)
  {
    vtkDistributedTrivialProducer::InternalStatic->RegisteredDataObjectMap.erase(key);
  }
  else
  {
    vtkDistributedTrivialProducer::InternalStatic->RegisteredDataObjectMap.clear();
  }
}

//----------------------------------------------------------------------------
void vtkDistributedTrivialProducer::UpdateFromGlobal(const char* key)
{
  vtkDebugMacro(<< "Update DS with key " << key);
  if (vtkDistributedTrivialProducer::InternalStatic->GetDataObject(key))
  {
    // vtkDistributedTrivialProducer::InternalStatic->GetDataObject(key)->PrintSelf(cout,
    // vtkIndent(5));
  }
  else
  {
    vtkDebugMacro(<< "No dataset for key=" << key);
  }
  this->SetOutput(vtkDistributedTrivialProducer::InternalStatic->GetDataObject(key));
}

//----------------------------------------------------------------------------
void vtkDistributedTrivialProducer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  vtkDistributedTrivialProducer::InternalStatic->Print(os, indent);
}
