/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientServerInterpreter.cxx
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
#include "vtkClientServerInterpreter.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkHashMap.txx"
#include "vtkObjectFactory.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkClientServerInterpreter);
vtkCxxRevisionMacro(vtkClientServerInterpreter, "1.1.2.16");

//----------------------------------------------------------------------------
// Internal container instantiations.
static inline vtkClientServerCommandFunction
vtkContainerCreateMethod(vtkClientServerCommandFunction d1)
{
  return vtkContainerDefaultCreate(d1);
}
static inline void vtkContainerDeleteMethod(vtkClientServerCommandFunction) {}

template class vtkHashMap<vtkTypeUInt32, vtkClientServerStream*>;
template class vtkHashMap<const char*, vtkClientServerCommandFunction>;

//----------------------------------------------------------------------------
class vtkClientServerInterpreterInternals
{
public:
  typedef vtkstd::vector<vtkClientServerNewInstanceFunction> NewInstanceFunctionsType;
  NewInstanceFunctionsType NewInstanceFunctions;
};

//----------------------------------------------------------------------------
vtkClientServerInterpreter::vtkClientServerInterpreter()
{
  this->Internal = new vtkClientServerInterpreterInternals;
  this->IDToMessageMap = IDToMessageMapType::New();
  this->ClassToFunctionMap = ClassToFunctionMapType::New();
  this->LastResultMessage = new vtkClientServerStream;
  this->LogStream = 0;
  this->LogFileStream = 0;
}

//----------------------------------------------------------------------------
vtkClientServerInterpreter::~vtkClientServerInterpreter()
{
  // Delete any remaining messages.
  vtkHashMapIterator<vtkTypeUInt32, vtkClientServerStream*>* hi =
    this->IDToMessageMap->NewIterator();
  vtkClientServerStream* tmp;
  while(!hi->IsDoneWithTraversal())
    {
    hi->GetData(tmp);
    delete tmp;
    hi->GoToNextItem();
    }
  hi->Delete();

  // End logging.
  this->SetLogStream(0);

  this->IDToMessageMap->Delete();
  this->ClassToFunctionMap->Delete();

  delete this->LastResultMessage;

  delete this->Internal;
}

//----------------------------------------------------------------------------
vtkObjectBase*
vtkClientServerInterpreter::GetObjectFromID(vtkClientServerID id)
{
  // Get the message corresponding to this ID.
  if(const vtkClientServerStream* tmp = this->GetMessageFromID(id))
    {
    // Retrieve the object from the message.
    vtkObjectBase* obj = 0;
    if(tmp->GetNumberOfArguments(0) == 1 && tmp->GetArgument(0, 0, &obj))
      {
      return obj;
      }
    else
      {
      vtkErrorMacro("Attempt to get an object for ID " << id.ID
                    << " whose message does not contain exactly one object.");
      return 0;
      }
    }
  else
    {
    vtkErrorMacro("Attempt to get object for ID " << id.ID
                  << " that is not present in the hash table.");
    return 0;
    }
}

//----------------------------------------------------------------------------
vtkClientServerID
vtkClientServerInterpreter::GetIDFromObject(vtkObjectBase* key)
{
  // Search the hash table for the given object.
  vtkHashMapIterator<vtkTypeUInt32, vtkClientServerStream*>* hi =
    this->IDToMessageMap->NewIterator();
  vtkClientServerStream* msg;
  vtkTypeUInt32 id = 0;
  while (!hi->IsDoneWithTraversal())
    {
    hi->GetData(msg);
    vtkObjectBase* obj;
    if(msg->GetArgument(0, 0, &obj) && obj == key)
      {
      hi->GetKey(id);
      break;
      }
    hi->GoToNextItem();
    }
  hi->Delete();

  // Convert the result to an ID object.
  vtkClientServerID result = {id};
  return result;
}

//----------------------------------------------------------------------------
void vtkClientServerInterpreter::SetLogFile(const char* name)
{
  // Close any existing log.
  this->SetLogStream(0);

  // If a non-empty name was given, open a new log file.
  if(name && name[0])
    {
    this->LogFileStream = new ofstream(name);
    if(this->LogFileStream && *this->LogFileStream)
      {
      this->LogStream = this->LogFileStream;
      }
    else
      {
      vtkErrorMacro("Error opening log file \"" << name << "\" for writing.");
      if(this->LogFileStream)
        {
        delete this->LogFileStream;
        this->LogFileStream = 0;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkClientServerInterpreter::SetLogStream(ostream* ostr)
{
  if(ostr != this->LogStream)
    {
    // Close the current log file, if any.
    if(this->LogStream && this->LogStream == this->LogFileStream)
      {
      delete this->LogFileStream;
      this->LogFileStream = 0;
      }

    // Set the log to use the given stream.
    this->LogStream = ostr;
    }
}

//----------------------------------------------------------------------------
int vtkClientServerInterpreter::ProcessStream(const unsigned char* msg,
                                              size_t msgLength)
{
  vtkClientServerStream css;
  css.SetData(msg, msgLength);
  return this->ProcessStream(css);
}

//----------------------------------------------------------------------------
int vtkClientServerInterpreter::ProcessStream(const vtkClientServerStream& css)
{
  for(int i=0; i < css.GetNumberOfMessages(); ++i)
    {
    if(!this->ProcessOneMessage(css, i))
      {
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int
vtkClientServerInterpreter::ProcessOneMessage(const vtkClientServerStream& css,
                                              int message)
{
  // Log the message.
  if(this->LogStream)
    {
    *this->LogStream << "---------------------------------------"
                     << "---------------------------------------\n";
    *this->LogStream << "Processing ";
    css.PrintMessage(*this->LogStream, message);
    this->LogStream->flush();
    }

  // Look for known commands in the message.
  int result = 0;
  vtkClientServerStream::Commands cmd = css.GetCommand(message);
  switch(cmd)
    {
    case vtkClientServerStream::New:
      result = this->ProcessCommandNew(css, message); break;
    case vtkClientServerStream::Invoke:
      result = this->ProcessCommandInvoke(css, message); break;
    case vtkClientServerStream::Delete:
      result = this->ProcessCommandDelete(css, message); break;
    case vtkClientServerStream::Assign:
      result = this->ProcessCommandAssign(css, message); break;
    default:
      {
      // Command is not known.
      ostrstream error;
      error << "Message with type "
            << vtkClientServerStream::GetStringFromCommand(cmd)
            << " cannot be executed." << ends;
      this->LastResultMessage->Reset();
      *this->LastResultMessage
        << vtkClientServerStream::Error << error.str()
        << vtkClientServerStream::End;
      error.rdbuf()->freeze(0);
      } break;
    }

  // Log the result of the message.
  if(this->LogStream)
    {
    if(this->LastResultMessage->GetNumberOfMessages() > 0)
      {
      *this->LogStream << "Result ";
      this->LastResultMessage->Print(*this->LogStream);
     }
    else
      {
      *this->LogStream << "Empty Result\n";
      }
    this->LogStream->flush();
    }

  // Report an error if the command failed with an error message.
  if(!result)
    {
    const char* errorMessage;
    if(this->LastResultMessage->GetNumberOfMessages() > 0 &&
       this->LastResultMessage->GetCommand(0) ==
       vtkClientServerStream::Error &&
       this->LastResultMessage->GetArgument(0, 0, &errorMessage))
      {
      ostrstream error;
      error << "\nwhile processing\n";
      css.PrintMessage(error, message);
      error << ends;
      vtkErrorMacro(<< errorMessage << error.str());
      error.rdbuf()->freeze(0);
      abort();
      }
    }

  return result;
}

//----------------------------------------------------------------------------
int
vtkClientServerInterpreter
::ProcessCommandNew(const vtkClientServerStream& css, int midx)
{
  // This command ignores any previous result.
  this->LastResultMessage->Reset();

  // Make sure we have some instance creation functions registered.
  if(this->Internal->NewInstanceFunctions.size() == 0)
    {
    *this->LastResultMessage
      << vtkClientServerStream::Error
      << "Attempt to create object with no registered class wrappers."
      << vtkClientServerStream::End;
    return 0;
    }

  // Get the class name and desired ID for the instance.
  const char* cname = 0;
  vtkClientServerID id;
  if(css.GetNumberOfArguments(midx) == 2 &&
     css.GetArgument(midx, 0, &cname) && css.GetArgument(midx, 1, &id))
    {
    // Make sure the given ID is valid.
    if(id.ID == 0)
      {
      *this->LastResultMessage
        << vtkClientServerStream::Error << "Cannot create object with ID 0."
        << vtkClientServerStream::End;
      return 0;
      }

    // Make sure the ID doesn't exist.
    vtkClientServerStream* tmp;
    if(this->IDToMessageMap->GetItem(id.ID, tmp) == VTK_OK)
      {
      ostrstream error;
      error << "Attempt to create object with existing ID " << id.ID << "."
            << ends;
      *this->LastResultMessage
        << vtkClientServerStream::Error << error.str()
        << vtkClientServerStream::End;
      error.rdbuf()->freeze(0);
      return 0;
      }

    // Find a NewInstance function that knows about the class.
    int created = 0;
    for(vtkClientServerInterpreterInternals::NewInstanceFunctionsType::iterator
          it = this->Internal->NewInstanceFunctions.begin();
        !created && it != this->Internal->NewInstanceFunctions.end(); ++it)
      {
      // Try this new-instance function.
      if((*(*it))(this, cname, id))
        {
        created = 1;
        }
      }
    if(created)
      {
      // Object was created.  Notify observers.
      vtkClientServerInterpreter::NewCallbackInfo info;
      info.Type = cname;
      info.ID = id.ID;
      this->InvokeEvent(vtkCommand::UserEvent+1, &info);
      return 1;
      }
    else
      {
      // Object was not created.
      ostrstream error;
      error << "Cannot create object of type \"" << cname << "\"." << ends;
      *this->LastResultMessage
        << vtkClientServerStream::Error << error.str()
        << vtkClientServerStream::End;
      error.rdbuf()->freeze(0);
      }
    }
  else
    {
    *this->LastResultMessage
      << vtkClientServerStream::Error
      << "Invalid arguments to vtkClientServerStream::New."
      << vtkClientServerStream::End;
    }
  return 0;
}

//----------------------------------------------------------------------------
int
vtkClientServerInterpreter
::ProcessCommandInvoke(const vtkClientServerStream& css, int midx)
{
  // Create a message with all known id_value arguments expanded.
  vtkClientServerStream msg;
  this->ExpandMessage(css, midx, 0, msg);

  // Now that id_values have been expanded, we do not need the last
  // result.  Reset the result to empty before processing the message.
  this->LastResultMessage->Reset();

  // Get the object and method to be invoked.
  vtkObjectBase* obj;
  const char* method;
  if(msg.GetNumberOfArguments(0) >= 2 &&
     msg.GetArgument(0, 0, &obj) && msg.GetArgument(0, 1, &method))
    {
    // Log the expanded form of the message.
    if(this->LogStream)
      {
      *this->LogStream << "Invoking ";
      msg.Print(*this->LogStream);
      this->LogStream->flush();
      }

    // Find the command function for this object's type.
    if(vtkClientServerCommandFunction func = this->GetCommandFunction(obj))
      {
      // Try to invoke the method.  If it fails, LastResultMessage
      // will have the error message.
      if(func(this, obj, method, msg, *this->LastResultMessage))
        {
        return 1;
        }
      }
    else
      {
      // Command function was not found for the class.
      ostrstream error;
      const char* cname = obj? obj->GetClassName():"(vtk object is NULL)";
      error << "Wrapper function not found for class \"" << cname << "\"."
            << ends;
      *this->LastResultMessage
        << vtkClientServerStream::Error << error.str()
        << vtkClientServerStream::End;
      error.rdbuf()->freeze(0);
      }
    }
  else
    {
    *this->LastResultMessage
      << vtkClientServerStream::Error
      << "Invalid arguments to vtkClientServerStream::Invoke."
      << vtkClientServerStream::End;
    }
  return 0;
}

//----------------------------------------------------------------------------
int
vtkClientServerInterpreter
::ProcessCommandDelete(const vtkClientServerStream& msg, int midx)
{
  // This command ignores any previous result.
  this->LastResultMessage->Reset();

  // Get the ID to delete.
  vtkClientServerID id;
  if(msg.GetNumberOfArguments(midx) == 1 && msg.GetArgument(midx, 0, &id))
    {
    // Make sure the given ID is valid.
    if(id.ID == 0)
      {
      *this->LastResultMessage
        << vtkClientServerStream::Error << "Cannot delete object with ID 0."
        << vtkClientServerStream::End;
      return 0;
      }

    // Find the ID in the map.
    vtkClientServerStream* item = 0;
    if(this->IDToMessageMap->GetItem(id.ID, item) != VTK_OK)
      {
      *this->LastResultMessage
        << vtkClientServerStream::Error
        << "Attempt to delete ID that does not exist."
        << vtkClientServerStream::End;
      return 0;
      }

    // If the value is an object, notify observers of deletion.
    vtkObjectBase* obj;
    if(item->GetArgument(0, 0, &obj))
      {
      vtkClientServerInterpreter::NewCallbackInfo info;
      info.Type = obj->GetClassName();
      info.ID = id.ID;
      this->InvokeEvent(vtkCommand::UserEvent+2, &info);
      }

    // Remove the ID from the map.
    this->IDToMessageMap->RemoveItem(id.ID);

    // Delete the entry's value.
    delete item;

    return 1;
    }
  else
    {
    *this->LastResultMessage
      << vtkClientServerStream::Error
      << "Invalid arguments to vtkClientServerStream::Delete."
      << vtkClientServerStream::End;
    }

  return 0;
}

//----------------------------------------------------------------------------
int
vtkClientServerInterpreter
::ProcessCommandAssign(const vtkClientServerStream& css, int midx)
{
  // Create a message with all known id_value arguments expanded
  // except for the first argument.
  vtkClientServerStream msg;
  this->ExpandMessage(css, midx, 1, msg);

  // Now that id_values have been expanded, we do not need the last
  // result.  Reset the result to empty before processing the message.
  this->LastResultMessage->Reset();

  // Make sure the first argument is an id.
  vtkClientServerID id;
  if(msg.GetNumberOfArguments(0) >= 1 && msg.GetArgument(0, 0, &id))
    {
    // Make sure the given ID is valid.
    if(id.ID == 0)
      {
      *this->LastResultMessage
        << vtkClientServerStream::Error << "Cannot assign to ID 0."
        << vtkClientServerStream::End;
      return 0;
      }

    // Make sure the ID doesn't exist.
    vtkClientServerStream* tmp;
    if(this->IDToMessageMap->GetItem(id.ID, tmp) == VTK_OK)
      {
      ostrstream error;
      error << "Attempt to assign existing ID " << id.ID << "." << ends;
      *this->LastResultMessage
        << vtkClientServerStream::Error << error.str()
        << vtkClientServerStream::End;
      error.rdbuf()->freeze(0);
      return 0;
      }

    // Copy the expanded message to the result message except for the
    // first argument.
    *this->LastResultMessage << vtkClientServerStream::Reply;
    for(int a=1; a < msg.GetNumberOfArguments(0); ++a)
      {
      *this->LastResultMessage << msg.GetArgument(0, a);
      }
    *this->LastResultMessage << vtkClientServerStream::End;

    // Copy the result to store it in the map.  The result itself
    // remains unchanged.
    tmp = new vtkClientServerStream(*this->LastResultMessage);
    this->IDToMessageMap->SetItem(id.ID, tmp);
    return 1;
    }
  else
    {
    this->LastResultMessage->Reset();
    *this->LastResultMessage
      << vtkClientServerStream::Error
      << "Invalid arguments to vtkClientServerStream::Assign."
      << vtkClientServerStream::End;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkClientServerInterpreter::ExpandMessage(const vtkClientServerStream& in,
                                              int inIndex, int startArgument,
                                              vtkClientServerStream& out)
{
  // Reset the output and make sure we have input.
  out.Reset();
  if(inIndex < 0 || inIndex >= in.GetNumberOfMessages())
    {
    return 0;
    }

  // Copy the command.
  out << in.GetCommand(inIndex);

  // Just copy the first arguments.
  int a;
  for(a=0; a < startArgument && a < in.GetNumberOfArguments(inIndex); ++a)
    {
    out << in.GetArgument(inIndex, a);
    }

  // Expand id_value for remaining arguments.
  for(a=startArgument; a < in.GetNumberOfArguments(inIndex); ++a)
    {
    if(in.GetArgumentType(inIndex, a) == vtkClientServerStream::id_value)
      {
      vtkClientServerID id;
      in.GetArgument(inIndex, a, &id);

      // If the ID is in the map, expand it.  Otherwise, leave it.
      if(const vtkClientServerStream* tmp = this->GetMessageFromID(id))
        {
        for(int b=0; b < tmp->GetNumberOfArguments(0); ++b)
          {
          out << tmp->GetArgument(0, b);
          }
        }
      else
        {
        out << in.GetArgument(inIndex, a);
        }
      }
    else if(in.GetArgumentType(inIndex, a) ==
            vtkClientServerStream::LastResult)
      {
      // Insert the last result value.
      for(int b=0; b < this->LastResultMessage->GetNumberOfArguments(0); ++b)
        {
        out << this->LastResultMessage->GetArgument(0, b);
        }
      }
    else
      {
      // Just copy the argument.
      out << in.GetArgument(inIndex, a);
      }
    }

  // End the message.
  out << vtkClientServerStream::End;

  return 1;
}

//----------------------------------------------------------------------------
const vtkClientServerStream*
vtkClientServerInterpreter::GetMessageFromID(vtkClientServerID id)
{
  // Find the message in the map.
  vtkClientServerStream* tmp;
  if(id.ID > 0 && this->IDToMessageMap->GetItem(id.ID, tmp) == VTK_OK)
    {
    return tmp;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkClientServerInterpreter::GetLastResult() const
{
  return *this->LastResultMessage;
}

//----------------------------------------------------------------------------
int vtkClientServerInterpreter::NewInstance(vtkObjectBase* obj,
                                            vtkClientServerID id)
{
  // Store the object in the last result.
  this->LastResultMessage->Reset();
  *this->LastResultMessage
    << vtkClientServerStream::Reply << obj << vtkClientServerStream::End;

  // Last result holds a reference.  Remove reference from ::New()
  // call in generated code.
  obj->Delete();

  // Copy the result to store it in the map.  The result itself
  // remains unchanged.  The ProcessCommandNew method already checked
  // that the id does not exist in the map, so the insertion does not
  // have to be checked.
  vtkClientServerStream* entry =
    new vtkClientServerStream(*this->LastResultMessage);
  this->IDToMessageMap->SetItem(id.ID, entry);
  return 1;
}

//----------------------------------------------------------------------------
class vtkClientServerInterpreterCommand: public vtkCommand
{
public:
  static vtkClientServerInterpreterCommand* New()
    { return new vtkClientServerInterpreterCommand; }
  void Execute(vtkObject*, unsigned long, void *)
    { this->Interpreter->ProcessStream(this->Stream); }

  vtkClientServerStream Stream;
  vtkClientServerInterpreter* Interpreter;
protected:
  vtkClientServerInterpreterCommand() {}
  ~vtkClientServerInterpreterCommand() {}
private:
  vtkClientServerInterpreterCommand(const vtkClientServerInterpreterCommand&);
  void operator=(const vtkClientServerInterpreterCommand&);
};

int vtkClientServerInterpreter::NewObserver(vtkObject* obj, const char* event,
                                            const vtkClientServerStream& css)
{
  // Create the command object and add the observer.
  vtkClientServerInterpreterCommand* cmd =
    vtkClientServerInterpreterCommand::New();
  cmd->Stream = css;
  cmd->Interpreter = this;
  unsigned long id = obj->AddObserver(event, cmd);
  cmd->Delete();

  // Store the observer id as the last result.
  this->LastResultMessage->Reset();
  *this->LastResultMessage
    << vtkClientServerStream::Reply << id << vtkClientServerStream::End;
  return 1;
}

//----------------------------------------------------------------------------
void
vtkClientServerInterpreter
::AddCommandFunction(const char* cname, vtkClientServerCommandFunction func)
{
  this->ClassToFunctionMap->SetItem(cname, func);
}

//----------------------------------------------------------------------------
vtkClientServerCommandFunction
vtkClientServerInterpreter::GetCommandFunction(vtkObjectBase* obj)
{
  if(obj)
    {
    // Lookup the function for this object's class.
    vtkClientServerCommandFunction res = 0;
    const char* cname = obj->GetClassName();
    this->ClassToFunctionMap->GetItem(cname, res);
    if(!res)
      {
      vtkErrorMacro("Cannot find command function for \"" << cname << "\".");
      }
    return res;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void
vtkClientServerInterpreter
::AddNewInstanceFunction(vtkClientServerNewInstanceFunction f)
{
  this->Internal->NewInstanceFunctions.push_back(f);
}
