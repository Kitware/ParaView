/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProcessModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProcessModule.h"

#include "vtkCallbackCommand.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMProcessModule);
vtkCxxRevisionMacro(vtkSMProcessModule, "1.1");

//----------------------------------------------------------------------------
vtkSMProcessModule::vtkSMProcessModule()
{
  this->Interpreter = 0;
  this->InterpreterObserver = 0;
  this->ReportInterpreterErrors = 1;
}

//----------------------------------------------------------------------------
vtkSMProcessModule::~vtkSMProcessModule()
{
  // Free Interpreter and ClientServerStream.
  this->FinalizeInterpreter();
}

//----------------------------------------------------------------------------
void vtkSMProcessModule::ProcessStream(vtkClientServerStream* stream)
{
  this->Interpreter->ProcessStream(*stream);
}

//----------------------------------------------------------------------------
// ClientServer wrapper initialization functions.

//----------------------------------------------------------------------------
void vtkSMProcessModule::InitializeInterpreter()
{
  if(this->Interpreter)
    {
    return;
    }

  // Create the interpreter and supporting stream.
  this->Interpreter = vtkClientServerInterpreter::New();

}

//----------------------------------------------------------------------------
void vtkSMProcessModule::FinalizeInterpreter()
{
  if(!this->Interpreter)
    {
    return;
    }

  // Free the interpreter and supporting stream.
  this->Interpreter->RemoveObserver(this->InterpreterObserver);
  this->InterpreterObserver->Delete();

  this->Interpreter->Delete();
  this->Interpreter = 0;
}

//----------------------------------------------------------------------------
void vtkSMProcessModule::InterpreterCallbackFunction(vtkObject*,
                                                     unsigned long eid,
                                                     void* cd, void* d)
{
  reinterpret_cast<vtkSMProcessModule*>(cd)->InterpreterCallback(eid, d);
}

//----------------------------------------------------------------------------
void vtkSMProcessModule::InterpreterCallback(unsigned long, void* pinfo)
{
  if(!this->ReportInterpreterErrors)
    {
    return;
    }

  const char* errorMessage;
  vtkClientServerInterpreterErrorCallbackInfo* info
    = static_cast<vtkClientServerInterpreterErrorCallbackInfo*>(pinfo);
  const vtkClientServerStream& last = this->Interpreter->GetLastResult();
  if(last.GetNumberOfMessages() > 0 &&
     (last.GetCommand(0) == vtkClientServerStream::Error) &&
     last.GetArgument(0, 0, &errorMessage))
    {
    ostrstream error;
    error << "\nwhile processing\n";
    info->css->PrintMessage(error, info->message);
    error << ends;
    vtkErrorMacro(<< errorMessage << error.str());
    error.rdbuf()->freeze(0);
    vtkErrorMacro("Aborting execution for debugging purposes.");
    //abort();
    }
}

//----------------------------------------------------------------------------
void vtkSMProcessModule::ClearLastResult()
{
  this->Interpreter->ClearLastResult();
}

//----------------------------------------------------------------------------
void vtkSMProcessModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

