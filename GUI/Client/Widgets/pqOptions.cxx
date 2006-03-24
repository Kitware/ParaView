// -*- c++ -*-
/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqOptions.h"

#include <vtkObjectFactory.h>

vtkCxxRevisionMacro(pqOptions, "1.3");
vtkStandardNewMacro(pqOptions);

//-----------------------------------------------------------------------------

pqOptions::pqOptions()
{
  // The client/server mode is actually determined by the ProcessType.
  // However, because that is an enumeration, we have to set it in post
  // processing based on the ClientMode flag.
  this->ClientMode = 0;
}

pqOptions::~pqOptions()
{
}

void pqOptions::PrintSelf(ostream &os, vtkIndent indent)
{
  os << indent << "ClientMode: " << this->ClientMode << endl;

  this->Superclass::PrintSelf(os, indent);
}

void pqOptions::SetClientMode(int Mode)
{
  this->ClientMode = Mode;
}

void pqOptions::SetServerHost(const char* const Host)
{
  strcpy(ServerHostName, Host);
}

void pqOptions::SetServerPort(int Port)
{
  this->ServerPort = Port;
}


//-----------------------------------------------------------------------------

void pqOptions::Initialize()
{
  this->Superclass::Initialize();

  // Use AddArgument methods to add new arguments here.
  this->AddBooleanArgument("--client", "-c", &this->ClientMode,
                           "Run as a client to a ParaView server "
                           "(which must be launched separately.");
}

//-----------------------------------------------------------------------------

int pqOptions::PostProcess(int argc, const char * const *argv)
{
  // Do any post processing of arguments (for example, if one argument
  // implies another argument).

  // ParaView now selects the client/server mode by generating different
  // executables for each one.  While that may be a better approach, this
  // simple example just uses a flag.
  if (this->ClientMode)
    {
    this->SetProcessType(vtkPVOptions::PVCLIENT);
    }
  else
    {
    this->SetProcessType(vtkPVOptions::PARAVIEW);
    }

  return this->Superclass::PostProcess(argc, argv);
}
