// -*- c++ -*-

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqOptions.h"

#include <vtkObjectFactory.h>

vtkCxxRevisionMacro(pqOptions, "1.2");
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
