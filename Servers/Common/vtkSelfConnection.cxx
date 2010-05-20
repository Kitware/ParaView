/*=========================================================================

  Program:   ParaView
  Module:    vtkSelfConnection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelfConnection.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkDummyController.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkUndoSet.h"
#include "vtkUndoStack.h"
//-----------------------------------------------------------------------------
class vtkSelfConnectionUndoSet : public vtkUndoSet
{
public:
  static vtkSelfConnectionUndoSet* New();
  vtkTypeMacro(vtkSelfConnectionUndoSet, vtkUndoSet);
 
  virtual int Undo() 
    {
    return 1;
    }
  
  virtual int Redo() 
    {
    return 1;
    }

  void SetXMLElement(vtkPVXMLElement*);
  vtkGetObjectMacro(XMLElement, vtkPVXMLElement);

protected:
  vtkSelfConnectionUndoSet() 
    {
    this->XMLElement = 0;
    };
  ~vtkSelfConnectionUndoSet()
    { 
    this->SetXMLElement(0);
    };

  vtkPVXMLElement* XMLElement;
private:
  vtkSelfConnectionUndoSet(const vtkSelfConnectionUndoSet&);
  void operator=(const vtkSelfConnectionUndoSet&);
};

vtkStandardNewMacro(vtkSelfConnectionUndoSet);
vtkCxxSetObjectMacro(vtkSelfConnectionUndoSet, XMLElement, vtkPVXMLElement);
//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSelfConnection);
//-----------------------------------------------------------------------------
vtkSelfConnection::vtkSelfConnection()
{
  this->Controller = vtkDummyController::New();
  vtkMultiProcessController::SetGlobalController(this->Controller);
  this->UndoRedoStack = NULL; // created if needed.
}

//-----------------------------------------------------------------------------
vtkSelfConnection::~vtkSelfConnection()
{
  vtkMultiProcessController::SetGlobalController(0);
  if (this->UndoRedoStack)
    {
    this->UndoRedoStack->Delete();
    this->UndoRedoStack = NULL;
    }
}

//-----------------------------------------------------------------------------
int vtkSelfConnection::Initialize(int argc, char** argv, int *partitionId)
{
  this->Controller->Initialize(&argc, &argv, 1);
  *partitionId = this->GetPartitionId();
  // Nothing to do here, really.
  // Just return success.
  return this->Superclass::Initialize(argc, argv, partitionId);
}

//-----------------------------------------------------------------------------
vtkTypeUInt32 vtkSelfConnection::CreateSendFlag(vtkTypeUInt32 servers)
{
  // Everything is just processed on this single process.
  if (servers != 0)
    {
    return vtkProcessModule::CLIENT;
    }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkSelfConnection::SendStreamToClient(vtkClientServerStream& stream)
{
  return this->ProcessStreamLocally(stream);
}

//-----------------------------------------------------------------------------
int vtkSelfConnection::ProcessStreamLocally(vtkClientServerStream& stream)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->GetInterpreter()->ProcessStream(stream);
  return 0;
}

//-----------------------------------------------------------------------------
int vtkSelfConnection::ProcessStreamLocally(unsigned char* data, int length)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->GetInterpreter()->ProcessStream(data, length);
  return 0;
}

//-----------------------------------------------------------------------------
const vtkClientServerStream& vtkSelfConnection::GetLastResult(vtkTypeUInt32 )
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  return pm->GetInterpreter()->GetLastResult();
}
  
//-----------------------------------------------------------------------------
void vtkSelfConnection::GatherInformation(vtkTypeUInt32 vtkNotUsed(serverFlags), 
  vtkPVInformation* info, vtkClientServerID id)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkObject* object = vtkObject::SafeDownCast(pm->GetObjectFromID(id));
  if (!object)
    {
    vtkErrorMacro("Failed to locate object with ID: " << id);
    return;
    }

  info->CopyFromObject(object);
}

//-----------------------------------------------------------------------------
int vtkSelfConnection::LoadModule(const char* name, const char* directory)
{
  const char* paths[] = { directory, 0};
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int localResult = pm->GetInterpreter()->Load(name, paths);
  return localResult;
}

//-----------------------------------------------------------------------------
void vtkSelfConnection::PushUndo(const char* label, vtkPVXMLElement* root)
{
  if (!this->UndoRedoStack)
    {
    this->UndoRedoStack = vtkUndoStack::New();
    }
  vtkSelfConnectionUndoSet* elem = vtkSelfConnectionUndoSet::New();
  elem->SetXMLElement(root);
  this->UndoRedoStack->Push(label, elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* vtkSelfConnection::NewNextUndo()
{
  if (!this->UndoRedoStack || !this->UndoRedoStack->CanUndo())
    {
    vtkErrorMacro("Nothing to undo.");
    return 0;
    }
  vtkSelfConnectionUndoSet* set = vtkSelfConnectionUndoSet::SafeDownCast(
    this->UndoRedoStack->GetNextUndoSet());
  this->UndoRedoStack->PopUndoStack();

  vtkPVXMLElement* elem = set->GetXMLElement();
  elem->Register(this); // so that caller can call Delete();
  return elem;
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* vtkSelfConnection::NewNextRedo()
{
  if (!this->UndoRedoStack || !this->UndoRedoStack->CanRedo())
    {
    vtkErrorMacro("Nothing to redo.");
    return 0;
    }
  vtkSelfConnectionUndoSet* set = vtkSelfConnectionUndoSet::SafeDownCast(
    this->UndoRedoStack->GetNextRedoSet());
  this->UndoRedoStack->PopRedoStack();

   vtkPVXMLElement* elem = set->GetXMLElement();
  elem->Register(this); // so that caller can call Delete();
  return elem;
}

//-----------------------------------------------------------------------------
void vtkSelfConnection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
