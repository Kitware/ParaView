/*=========================================================================

  Program:   ParaView
  Module:    vtkClientServerInterpreter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClientServerInterpreter.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkDynamicLoader.h"
#include "vtkObjectFactory.h"

#include <vtksys/SystemTools.hxx>

#include <map>
#include <sstream>
#include <string>
#include <vector>

vtkStandardNewMacro(vtkClientServerInterpreter);

//----------------------------------------------------------------------------
class vtkClientServerInterpreterInternals
{
public:
  struct ContextInformation
  {
    ContextInformation(void* ctx, vtkContextFreeFunction freeFunction)
    {
      this->Context = ctx;
      this->FreeFunction = freeFunction;
    }
    ~ContextInformation()
    {
      if (this->FreeFunction)
      {
        this->FreeFunction(this->Context);
      }
    }

    void* Context;
    vtkContextFreeFunction FreeFunction;
  };
  template <typename FunctionType>
  struct FunctionWithContext
  {
    FunctionWithContext(FunctionType function, ContextInformation* ctx)
    {
      this->Function = function;
      this->Context = ctx;
    }
    ~FunctionWithContext() { delete this->Context; }

    FunctionType Function;
    ContextInformation* Context;

  private:
    FunctionWithContext(const FunctionWithContext&) = delete;
    FunctionWithContext& operator=(const FunctionWithContext&) = delete;
  };
  typedef FunctionWithContext<vtkClientServerNewInstanceFunction> NewInstanceFunction;
  typedef FunctionWithContext<vtkClientServerCommandFunction> CommandFunction;
  typedef std::map<std::string, const NewInstanceFunction*> NewInstanceFunctionsType;
  typedef std::map<std::string, const CommandFunction*> ClassToFunctionMapType;
  typedef std::map<vtkTypeUInt32, vtkClientServerStream*> IDToMessageMapType;
  NewInstanceFunctionsType NewInstanceFunctions;
  ClassToFunctionMapType ClassToFunctionMap;
  IDToMessageMapType IDToMessageMap;
};

//----------------------------------------------------------------------------
vtkClientServerInterpreter::vtkClientServerInterpreter()
{
  this->NextAvailableId = 0;
  this->Internal = new vtkClientServerInterpreterInternals;
  this->LastResultMessage = new vtkClientServerStream(this);
  this->LogStream = 0;
  this->LogFileStream = 0;
}

//----------------------------------------------------------------------------
vtkClientServerInterpreter::~vtkClientServerInterpreter()
{
  // Delete any remaining instance functions.
  vtkClientServerInterpreterInternals::NewInstanceFunctionsType::iterator ni;
  for (ni = this->Internal->NewInstanceFunctions.begin();
       ni != this->Internal->NewInstanceFunctions.end(); ++ni)
  {
    delete ni->second;
  }

  // Delete any remaining command functions.
  vtkClientServerInterpreterInternals::ClassToFunctionMapType::iterator ci;
  for (ci = this->Internal->ClassToFunctionMap.begin();
       ci != this->Internal->ClassToFunctionMap.end(); ++ci)
  {
    delete ci->second;
  }

  // Delete any remaining messages.
  vtkClientServerInterpreterInternals::IDToMessageMapType::iterator hi;
  for (hi = this->Internal->IDToMessageMap.begin(); hi != this->Internal->IDToMessageMap.end();
       ++hi)
  {
    delete hi->second;
  }

  // End logging.
  this->SetLogStream(0);

  delete this->LastResultMessage;
  this->LastResultMessage = 0;
  delete this->Internal;
  this->Internal = 0;
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkClientServerInterpreter::GetObjectFromID(vtkClientServerID id, int noerror)
{
  // Get the message corresponding to this ID.
  if (const vtkClientServerStream* tmp = this->GetMessageFromID(id))
  {
    // Retrieve the object from the message.
    vtkObjectBase* obj = 0;
    if (tmp->GetNumberOfArguments(0) == 1 && tmp->GetArgument(0, 0, &obj))
    {
      return obj;
    }
    else
    {
      if (!noerror)
      {
        vtkErrorMacro("Attempt to get an object for ID "
          << id.ID << " whose message does not contain exactly one object.");
      }
      return 0;
    }
  }
  else
  {
    if (!noerror)
    {
      vtkErrorMacro(
        "Attempt to get object for ID " << id.ID << " that is not present in the hash table.");
    }
    return 0;
  }
}

//----------------------------------------------------------------------------
vtkClientServerID vtkClientServerInterpreter::GetIDFromObject(vtkObjectBase* key)
{
  // Search the hash table for the given object.
  vtkClientServerID result;
  vtkClientServerInterpreterInternals::IDToMessageMapType::iterator hi;
  for (hi = this->Internal->IDToMessageMap.begin(); hi != this->Internal->IDToMessageMap.end();
       ++hi)
  {
    vtkObjectBase* obj;
    if (hi->second->GetArgument(0, 0, &obj) && obj == key)
    {
      result.ID = hi->first;
      break;
    }
  }
  return result;
}

//----------------------------------------------------------------------------
void vtkClientServerInterpreter::SetLogFile(const char* name)
{
  // Close any existing log.
  this->SetLogStream(0);

  // If a non-empty name was given, open a new log file.
  if (name && name[0])
  {
    this->LogFileStream = new ofstream(name);
    if (this->LogFileStream && *this->LogFileStream)
    {
      this->LogStream = this->LogFileStream;
    }
    else
    {
      vtkErrorMacro("Error opening log file \"" << name << "\" for writing.");
      if (this->LogFileStream)
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
  if (ostr != this->LogStream)
  {
    // Close the current log file, if any.
    if (this->LogStream && this->LogStream == this->LogFileStream)
    {
      delete this->LogFileStream;
      this->LogFileStream = 0;
    }

    // Set the log to use the given stream.
    this->LogStream = ostr;
  }
}

//----------------------------------------------------------------------------
int vtkClientServerInterpreter::ProcessStream(const unsigned char* msg, size_t msgLength)
{
  vtkClientServerStream css;
  css.SetData(msg, msgLength);
  return this->ProcessStream(css);
}

//----------------------------------------------------------------------------
int vtkClientServerInterpreter::ProcessStream(const vtkClientServerStream& css)
{
  for (int i = 0; i < css.GetNumberOfMessages(); ++i)
  {
    if (!this->ProcessOneMessage(css, i))
    {
      return 0;
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkClientServerInterpreter::ProcessOneMessage(const vtkClientServerStream& css, int message)
{
  // Log the message.
  if (this->LogStream)
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
  switch (cmd)
  {
    case vtkClientServerStream::New:
      result = this->ProcessCommandNew(css, message);
      break;
    case vtkClientServerStream::Invoke:
      result = this->ProcessCommandInvoke(css, message);
      break;
    case vtkClientServerStream::Delete:
      result = this->ProcessCommandDelete(css, message);
      break;
    case vtkClientServerStream::Assign:
      result = this->ProcessCommandAssign(css, message);
      break;
    default:
    {
      // Command is not known.
      std::ostringstream error;
      error << "Message with type " << vtkClientServerStream::GetStringFromCommand(cmd)
            << " cannot be executed." << ends;
      this->LastResultMessage->Reset();
      *this->LastResultMessage << vtkClientServerStream::Error << error.str().c_str()
                               << vtkClientServerStream::End;
    }
    break;
  }

  // Log the result of the message.
  if (this->LogStream)
  {
    if (this->LastResultMessage->GetNumberOfMessages() > 0)
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

  // If the command failed with an error message, invoke the error
  // event so observers can handle the error.
  if (!result)
  {
    vtkClientServerInterpreterErrorCallbackInfo info = { &css, message };
    this->InvokeEvent(vtkCommand::UserEvent, &info);
  }

  return result;
}

//----------------------------------------------------------------------------
int vtkClientServerInterpreter::ProcessCommandNew(const vtkClientServerStream& css, int midx)
{
  // This command ignores any previous result.
  this->LastResultMessage->Reset();

  // Make sure we have some instance creation functions registered.
  if (this->Internal->NewInstanceFunctions.empty())
  {
    *this->LastResultMessage << vtkClientServerStream::Error
                             << "Attempt to create object with no registered class wrappers."
                             << vtkClientServerStream::End;
    return 0;
  }

  // Get the class name and desired ID for the instance.
  const char* cname = 0;
  vtkClientServerID id;
  if (css.GetNumberOfArguments(midx) == 2 && css.GetArgument(midx, 0, &cname) &&
    css.GetArgument(midx, 1, &id))
  {
    // Make sure the given ID is valid.
    if (id.ID == 0)
    {
      *this->LastResultMessage << vtkClientServerStream::Error << "Cannot create object with ID 0."
                               << vtkClientServerStream::End;
      return 0;
    }

    // Make sure the ID doesn't exist.
    if (this->Internal->IDToMessageMap.find(id.ID) != this->Internal->IDToMessageMap.end())
    {
      std::ostringstream error;
      error << "Attempt to create object with existing ID " << id.ID << "." << ends;
      *this->LastResultMessage << vtkClientServerStream::Error << error.str().c_str()
                               << vtkClientServerStream::End;
      return 0;
    }

    // Find a NewInstance function that knows about the class.
    int created = 0;
    if (this->Internal->NewInstanceFunctions.count(cname))
    {
      const vtkClientServerInterpreterInternals::NewInstanceFunction* n =
        this->Internal->NewInstanceFunctions[cname];
      vtkClientServerNewInstanceFunction function = n->Function;
      void* ctx = n->Context ? n->Context->Context : 0;
      this->NewInstance(function(ctx), id);
      created = 1;
    }
    if (created)
    {
      // Object was created.  Notify observers.
      vtkClientServerInterpreter::NewCallbackInfo info;
      info.Type = cname;
      info.ID = id.ID;
      this->InvokeEvent(vtkCommand::UserEvent + 1, &info);
      return 1;
    }
    else
    {
      // Object was not created.
      std::ostringstream error;
      error << "Cannot create object of type \"" << cname << "\"." << ends;
      *this->LastResultMessage << vtkClientServerStream::Error << error.str().c_str()
                               << vtkClientServerStream::End;
    }
  }
  else
  {
    *this->LastResultMessage
      << vtkClientServerStream::Error
      << "Invalid arguments to vtkClientServerStream::New.  "
         "There must be exactly two arguments.  The first must be a string and "
         "the second an id."
      << vtkClientServerStream::End;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkClientServerInterpreter::ProcessCommandInvoke(const vtkClientServerStream& css, int midx)
{
  // Create a message with all known id_value arguments expanded.
  vtkClientServerStream msg;
  if (!this->ExpandMessage(css, midx, 0, msg))
  {
    // ExpandMessage left an error in the LastResultMessage for us.
    return 0;
  }

  // Now that id_values have been expanded, we do not need the last
  // result.  Reset the result to empty before processing the message.
  this->LastResultMessage->Reset();

  // Get the object and method to be invoked.
  vtkObjectBase* obj;
  const char* method;
  if (msg.GetNumberOfArguments(0) >= 2 && msg.GetArgument(0, 0, &obj) &&
    msg.GetArgument(0, 1, &method))
  {
    // Log the expanded form of the message.
    if (this->LogStream)
    {
      *this->LogStream << "Invoking ";
      msg.Print(*this->LogStream);
      this->LogStream->flush();
    }

    // Find the command function for this object's type.
    if (obj && this->HasCommandFunction(obj->GetClassName()))
    {
      if (this->CallCommandFunction(
            obj->GetClassName(), obj, method, msg, *this->LastResultMessage))
      {
        return 1;
      }
    }
    else
    {
      // Command function was not found for the class.
      std::ostringstream error;
      const char* cname = obj ? obj->GetClassName() : "(vtk object is NULL)";
      error << "Wrapper function not found for class \"" << cname << "\"." << ends;
      *this->LastResultMessage << vtkClientServerStream::Error << error.str().c_str()
                               << vtkClientServerStream::End;
    }
  }
  else
  {
    *this->LastResultMessage
      << vtkClientServerStream::Error
      << "Invalid arguments to vtkClientServerStream::Invoke.  "
         "There must be at least two arguments.  The first must be an object "
         "and the second a string."
      << vtkClientServerStream::End;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkClientServerInterpreter::ProcessCommandDelete(const vtkClientServerStream& msg, int midx)
{
  // This command ignores any previous result.
  if (this->LastResultMessage == NULL)
  {
    return 0;
  }
  this->LastResultMessage->Reset();

  // Get the ID to delete.
  vtkClientServerID id;
  if (msg.GetNumberOfArguments(midx) == 1 && msg.GetArgument(midx, 0, &id))
  {
    // Make sure the given ID is valid.
    if (id.ID == 0)
    {
      *this->LastResultMessage << vtkClientServerStream::Error << "Cannot delete object with ID 0."
                               << vtkClientServerStream::End;
      return 0;
    }

    // Find the ID in the map.
    vtkClientServerStream* item = 0;
    vtkClientServerInterpreterInternals::IDToMessageMapType::iterator itemi;
    itemi = this->Internal->IDToMessageMap.find(id.ID);
    if (itemi != this->Internal->IDToMessageMap.end())
    {
      item = itemi->second;
    }
    else
    {
      *this->LastResultMessage << vtkClientServerStream::Error
                               << "Attempt to delete ID that does not exist."
                               << vtkClientServerStream::End;
      return 0;
    }

    // If the value is an object, notify observers of deletion.
    vtkObjectBase* obj;
    if (item->GetArgument(0, 0, &obj) && obj)
    {
      vtkClientServerInterpreter::NewCallbackInfo info;
      info.Type = obj->GetClassName();
      info.ID = id.ID;
      this->InvokeEvent(vtkCommand::UserEvent + 2, &info);
    }

    // Remove the ID from the map.
    this->Internal->IDToMessageMap.erase(id.ID);

    // Delete the entry's value.
    delete item;

    return 1;
  }
  else
  {
    *this->LastResultMessage << vtkClientServerStream::Error
                             << "Invalid arguments to vtkClientServerStream::Delete.  "
                                "There must be exactly one argument and it must be an id."
                             << vtkClientServerStream::End;
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkClientServerInterpreter::ProcessCommandAssign(const vtkClientServerStream& css, int midx)
{
  // Create a message with all known id_value arguments expanded
  // except for the first argument.
  vtkClientServerStream msg;
  if (!this->ExpandMessage(css, midx, 1, msg))
  {
    // ExpandMessage left an error in the LastResultMessage for us.
    return 0;
  }

  // Now that id_values have been expanded, we do not need the last
  // result.  Reset the result to empty before processing the message.
  this->LastResultMessage->Reset();

  // Make sure the first argument is an id.
  vtkClientServerID id;
  if (msg.GetNumberOfArguments(0) >= 1 && msg.GetArgument(0, 0, &id))
  {
    // Make sure the given ID is valid.
    if (id.ID == 0)
    {
      *this->LastResultMessage << vtkClientServerStream::Error << "Cannot assign to ID 0."
                               << vtkClientServerStream::End;
      return 0;
    }

    // Make sure the ID doesn't exist.
    if (this->Internal->IDToMessageMap.find(id.ID) != this->Internal->IDToMessageMap.end())
    {
      std::ostringstream error;
      error << "Attempt to assign existing ID " << id.ID << "." << ends;
      *this->LastResultMessage << vtkClientServerStream::Error << error.str().c_str()
                               << vtkClientServerStream::End;
      return 0;
    }

    // Copy the expanded message to the result message except for the
    // first argument.
    *this->LastResultMessage << vtkClientServerStream::Reply;
    for (int a = 1; a < msg.GetNumberOfArguments(0); ++a)
    {
      *this->LastResultMessage << msg.GetArgument(0, a);
    }
    *this->LastResultMessage << vtkClientServerStream::End;

    // Copy the result to store it in the map.  The result itself
    // remains unchanged.
    vtkClientServerStream* tmp;
    tmp = new vtkClientServerStream(*this->LastResultMessage, this);
    this->Internal->IDToMessageMap[id.ID] = tmp;
    return 1;
  }
  else
  {
    this->LastResultMessage->Reset();
    *this->LastResultMessage << vtkClientServerStream::Error
                             << "Invalid arguments to vtkClientServerStream::Assign.  "
                                "There must be at least one argument and it must be an id."
                             << vtkClientServerStream::End;
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkClientServerInterpreter::ExpandMessage(
  const vtkClientServerStream& in, int inIndex, int startArgument, vtkClientServerStream& out)
{
  // Reset the output and make sure we have input.
  out.Reset();
  if (inIndex < 0 || inIndex >= in.GetNumberOfMessages())
  {
    std::ostringstream error;
    error << "ExpandMessage called to expand message index " << inIndex << " in a stream with "
          << in.GetNumberOfMessages() << " messages." << ends;
    this->LastResultMessage->Reset();
    *this->LastResultMessage << vtkClientServerStream::Error << error.str().c_str()
                             << vtkClientServerStream::End;
    return 0;
  }

  // Copy the command.
  out << in.GetCommand(inIndex);

  // Just copy the first arguments.
  int a;
  for (a = 0; a < startArgument && a < in.GetNumberOfArguments(inIndex); ++a)
  {
    out << in.GetArgument(inIndex, a);
  }

  // Expand id_value for remaining arguments.
  for (a = startArgument; a < in.GetNumberOfArguments(inIndex); ++a)
  {
    if (in.GetArgumentType(inIndex, a) == vtkClientServerStream::id_value)
    {
      vtkClientServerID id;
      in.GetArgument(inIndex, a, &id);

      // If the ID is in the map, expand it.  Otherwise, leave it.
      if (const vtkClientServerStream* tmp = this->GetMessageFromID(id))
      {
        for (int b = 0; b < tmp->GetNumberOfArguments(0); ++b)
        {
          out << tmp->GetArgument(0, b);
        }
      }
      else
      {
        out << in.GetArgument(inIndex, a);
      }
    }
    else if (in.GetArgumentType(inIndex, a) == vtkClientServerStream::LastResult)
    {
      // Insert the last result value.
      for (int b = 0; b < this->LastResultMessage->GetNumberOfArguments(0); ++b)
      {
        out << this->LastResultMessage->GetArgument(0, b);
      }
    }
    else if (in.GetArgumentType(inIndex, a) == vtkClientServerStream::stream_value)
    {
      // Evaluate the expression and insert the result.
      vtkClientServerStream* lastResult = this->LastResultMessage;
      this->LastResultMessage = new vtkClientServerStream();
      vtkClientServerStream substream;
      in.GetArgument(inIndex, a, &substream);
      if (this->ProcessStream(substream))
      {
        // Insert the last result value.
        for (int b = 0; b < this->LastResultMessage->GetNumberOfArguments(0); ++b)
        {
          out << this->LastResultMessage->GetArgument(0, b);
        }
      }
      // restore last-result
      delete this->LastResultMessage;
      this->LastResultMessage = lastResult;
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
const vtkClientServerStream* vtkClientServerInterpreter::GetMessageFromID(vtkClientServerID id)
{
  // Find the message in the map.
  vtkClientServerInterpreterInternals::IDToMessageMapType::iterator tmp;
  tmp = this->Internal->IDToMessageMap.find(id.ID);
  if (id.ID > 0 && tmp != this->Internal->IDToMessageMap.end())
  {
    return tmp->second;
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
int vtkClientServerInterpreter::NewInstance(vtkObjectBase* obj, vtkClientServerID id)
{
  // Store the object in the last result.
  this->LastResultMessage->Reset();
  *this->LastResultMessage << vtkClientServerStream::Reply << obj << vtkClientServerStream::End;

  // Last result holds a reference.  Remove reference from ::New()
  // call in generated code.
  obj->Delete();

  // Copy the result to store it in the map.  The result itself
  // remains unchanged.  The ProcessCommandNew method already checked
  // that the id does not exist in the map, so the insertion does not
  // have to be checked.
  vtkClientServerStream* entry = new vtkClientServerStream(*this->LastResultMessage, this);
  this->Internal->IDToMessageMap[id.ID] = entry;
  return 1;
}

//----------------------------------------------------------------------------
namespace
{
class vtkClientServerInterpreterCommand : public vtkCommand
{
public:
  static vtkClientServerInterpreterCommand* New() { return new vtkClientServerInterpreterCommand; }
  void Execute(vtkObject*, unsigned long, void*) override
  {
    this->Interpreter->ProcessStream(this->Stream);
  }

  vtkClientServerStream Stream;
  vtkClientServerInterpreter* Interpreter;

protected:
  vtkClientServerInterpreterCommand() {}
  ~vtkClientServerInterpreterCommand() override {}
private:
  vtkClientServerInterpreterCommand(const vtkClientServerInterpreterCommand&);
  void operator=(const vtkClientServerInterpreterCommand&);
};
}

int vtkClientServerInterpreter::NewObserver(
  vtkObject* obj, const char* event, const vtkClientServerStream& css)
{
  // Create the command object and add the observer.
  vtkClientServerInterpreterCommand* cmd = vtkClientServerInterpreterCommand::New();
  cmd->Stream = css;
  cmd->Interpreter = this;
  unsigned long id = obj->AddObserver(event, cmd);
  cmd->Delete();

  // Store the observer id as the last result.
  this->LastResultMessage->Reset();
  *this->LastResultMessage << vtkClientServerStream::Reply << id << vtkClientServerStream::End;
  return 1;
}

//----------------------------------------------------------------------------
void vtkClientServerInterpreter::AddCommandFunction(const char* cname,
  vtkClientServerCommandFunction func, void* ctx, vtkContextFreeFunction freeFunction)
{
  vtkClientServerInterpreterInternals::ClassToFunctionMapType::const_iterator it =
    this->Internal->ClassToFunctionMap.find(cname);

  if (it != this->Internal->ClassToFunctionMap.end())
  {
    if (it->second->Context == ctx && it->second->Function == func)
    { // we already have this EXACT definition available so don't add
      return;
    }
    vtkErrorMacro("Ignoring duplicate command function for " << cname);
    return;
  }

  vtkClientServerInterpreterInternals::ContextInformation* context = NULL;
  if (ctx || freeFunction)
  {
    context = new vtkClientServerInterpreterInternals::ContextInformation(ctx, freeFunction);
  }

  this->Internal->ClassToFunctionMap[cname] =
    new vtkClientServerInterpreterInternals::CommandFunction(func, context);
}

//----------------------------------------------------------------------------
bool vtkClientServerInterpreter::HasCommandFunction(const char* cname)
{
  if (!cname)
  {
    return false;
  }
  return (this->Internal->ClassToFunctionMap.count(cname) > 0);
}

//----------------------------------------------------------------------------
int vtkClientServerInterpreter::CallCommandFunction(const char* cname, vtkObjectBase* ptr,
  const char* method, const vtkClientServerStream& msg, vtkClientServerStream& result)
{
  vtkClientServerInterpreterInternals::ClassToFunctionMapType::const_iterator f =
    this->Internal->ClassToFunctionMap.find(cname);

  if (f == this->Internal->ClassToFunctionMap.end())
  {
    vtkErrorMacro("Cannot find command function for \"" << cname << "\".");
    return 1;
  }

  const vtkClientServerInterpreterInternals::CommandFunction* n = f->second;

  vtkClientServerCommandFunction function = n->Function;
  void* ctx = n->Context ? n->Context->Context : 0;
  return function(this, ptr, method, msg, result, ctx);
}

void vtkClientServerInterpreter::AddNewInstanceFunction(const char* name,
  vtkClientServerNewInstanceFunction f, void* ctx, vtkContextFreeFunction freeFunction)
{
  vtkClientServerInterpreterInternals::NewInstanceFunctionsType::const_iterator it =
    this->Internal->NewInstanceFunctions.find(name);

  if (it != this->Internal->NewInstanceFunctions.end())
  {
    if (it->second->Context == ctx && it->second->Function == f)
    { // we already have this EXACT definition available so don't add
      return;
    }
    vtkErrorMacro("Ignoring duplicate instance function for " << name);
    return;
  }

  vtkClientServerInterpreterInternals::ContextInformation* context = NULL;
  if (ctx || freeFunction)
  {
    context = new vtkClientServerInterpreterInternals::ContextInformation(ctx, freeFunction);
  }

  this->Internal->NewInstanceFunctions[name] =
    new vtkClientServerInterpreterInternals::NewInstanceFunction(f, context);
}

//----------------------------------------------------------------------------
int vtkClientServerInterpreter::Load(const char* moduleName)
{
  return this->Load(moduleName, 0);
}

//----------------------------------------------------------------------------
static void vtkClientServerInterpreterSplit(
  const char* path, char split, char slash, std::vector<std::string>& paths)
{
  std::string str = path ? path : "";
  std::string::size_type lpos = 0;
  std::string::size_type rpos = str.npos;
  while ((rpos = str.find(split, lpos)) != str.npos)
  {
    if (lpos < rpos)
    {
      std::string dir = str.substr(lpos, rpos - lpos);
      if (*(dir.end() - 1) != slash)
      {
        dir += slash;
      }
      paths.push_back(dir);
    }
    lpos = rpos + 1;
  }
  if (lpos < str.length())
  {
    std::string dir = str.substr(lpos);
    if (*(dir.end() - 1) != slash)
    {
      dir += slash;
    }
    paths.push_back(dir);
  }
}

//----------------------------------------------------------------------------
int vtkClientServerInterpreter::Load(const char* moduleName, const char* const* optionalPaths)
{
  // The library search path.
  typedef std::vector<std::string> PathsType;
  PathsType paths;

  // Try user-specified paths if any.
  if (optionalPaths)
  {
    for (const char* const* p = optionalPaths; *p; ++p)
    {
      std::string path = *p;
      if (path.length() > 0)
      {
        char end = *(path.end() - 1);
        if (end != '/' && end != '\\')
        {
          path += "/";
        }
        paths.push_back(path);
      }
    }
  }

// Try system paths.
#if defined(_WIN32) && !defined(__CYGWIN__)
  vtkClientServerInterpreterSplit(getenv("PATH"), ';', '\\', paths);
#else
  vtkClientServerInterpreterSplit(getenv("LD_LIBRARY_PATH"), ':', '/', paths);
  vtkClientServerInterpreterSplit(getenv("PATH"), ':', '/', paths);
  paths.push_back("/usr/lib/");
  paths.push_back("/usr/lib/vtk/");
  paths.push_back("/usr/local/lib/");
  paths.push_back("/usr/local/lib/vtk/");
#endif

  // Search for the module.
  std::string searched;
  std::string libName = vtkDynamicLoader::LibPrefix();
  libName += moduleName;
  libName += vtkDynamicLoader::LibExtension();
  for (PathsType::iterator p = paths.begin(); p != paths.end(); ++p)
  {
    vtksys::SystemTools::Stat_t data;
    std::string fullPath = *p;

#if defined(CMAKE_INTDIR)
    // Look in the subdirectory for the configuration in which this
    // interpreter was built.
    std::string fullPathWithIntDir = fullPath;
    fullPathWithIntDir += CMAKE_INTDIR "/";
    fullPathWithIntDir += libName;
    if (vtksys::SystemTools::Stat(fullPathWithIntDir.c_str(), &data) == 0)
    {
      return this->LoadInternal(moduleName, fullPathWithIntDir.c_str());
    }
    searched += p->substr(0, p->length() - 1);
    searched += "/" CMAKE_INTDIR;
    searched += "\n";
#endif

    // Look in the directory specified.
    fullPath += libName;
    if (vtksys::SystemTools::Stat(fullPath.c_str(), &data) == 0)
    {
      return this->LoadInternal(moduleName, fullPath.c_str());
    }
    searched += p->substr(0, p->length() - 1);
    searched += "\n";
  }

  vtkErrorMacro("Cannot find module \"" << libName.c_str() << "\".  "
                                        << "The following paths were searched:\n"
                                        << searched.c_str());
  return 0;
}

//----------------------------------------------------------------------------
int vtkClientServerInterpreter::LoadInternal(const char* moduleName, const char* fullPath)
{
  // The type of a module initializer function.
  typedef void (*InitFunction)(vtkClientServerInterpreter*);

  // Load the library dynamically.
  vtkLibHandle lib = vtkDynamicLoader::OpenLibrary(fullPath);
  if (!lib)
  {
    vtkErrorMacro("Cannot load module \"" << moduleName << "\" from \"" << fullPath << "\".");
    const char* error = vtkDynamicLoader::LastError();
    if (error)
    {
      vtkErrorMacro(<< error);
    }
    return 0;
  }

  // Get the init function.
  std::string initFuncName = moduleName;
  initFuncName += "_Initialize";
  auto func =
    reinterpret_cast<InitFunction>(vtkDynamicLoader::GetSymbolAddress(lib, initFuncName.c_str()));
  if (!func)
  {
    vtkErrorMacro(
      "Cannot find function \"" << initFuncName.c_str() << "\" in \"" << fullPath << "\".");
    return 0;
  }

  // Call the init function.
  func(this);
  return 1;
}

//----------------------------------------------------------------------------
void vtkClientServerInterpreter::ClearLastResult()
{
  this->LastResultMessage->Reset();
}
//----------------------------------------------------------------------------
vtkClientServerID vtkClientServerInterpreter::GetNextAvailableId()
{
  return vtkClientServerID(++this->NextAvailableId);
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkClientServerInterpreter::NewInstance(const char* classname)
{
  if (!this->Internal->NewInstanceFunctions.count(classname))
  {
    return NULL;
  }

  const vtkClientServerInterpreterInternals::NewInstanceFunction* n =
    this->Internal->NewInstanceFunctions[classname];

  vtkClientServerNewInstanceFunction function = n->Function;
  void* ctx = n->Context ? n->Context->Context : 0;

  return function(ctx);
}

//----------------------------------------------------------------------------
void vtkClientServerInterpreter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
