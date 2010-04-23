/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerArraySelection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVServerArraySelection.h"

#include "vtkClientServerInterpreter.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"

#include <vtkstd/string>
#include <vtksys/ios/sstream>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVServerArraySelection);

//----------------------------------------------------------------------------
class vtkPVServerArraySelectionInternals
{
public:
  vtkClientServerStream Result;
};

//----------------------------------------------------------------------------
vtkPVServerArraySelection::vtkPVServerArraySelection()
{
  this->Internal = new vtkPVServerArraySelectionInternals;
}

//----------------------------------------------------------------------------
vtkPVServerArraySelection::~vtkPVServerArraySelection()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVServerArraySelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
const vtkClientServerStream&
vtkPVServerArraySelection
::GetArraySettings(vtkAlgorithm* source, const char* arrayname)
{
  // Reset the stream for a new list of array names.
  this->Internal->Result.Reset();
  this->Internal->Result << vtkClientServerStream::Reply;

  // Make sure we have a process module.
  if(this->ProcessModule && source)
    {
    // Get the local process interpreter.
    vtkClientServerInterpreter* interp =
      this->ProcessModule->GetInterpreter();

    // Get the ID for the reader.
    vtkClientServerID readerID = interp->GetIDFromObject(source);
    if(readerID.ID)
      {
      vtksys_ios::ostringstream aname;
      aname << "GetNumberOf" << arrayname << "Arrays" << ends;

      // Get the number of arrays.
      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke
             << readerID
             << aname.str().c_str()
             << vtkClientServerStream::End;
      interp->ProcessStream(stream);
      stream.Reset();
      int numArrays = 0;
      if(!interp->GetLastResult().GetArgument(0, 0, &numArrays))
        {
        vtkErrorMacro("Error getting number of arrays from reader.");
        }

      // For each array, get its name and status.
      for(int i=0; i < numArrays; ++i)
        {
        vtksys_ios::ostringstream naname;
        naname << "Get" << arrayname << "ArrayName" << ends;

        // Get the array name.
        stream << vtkClientServerStream::Invoke
               << readerID
               << naname.str().c_str()
               << i
               << vtkClientServerStream::End;
        if(!interp->ProcessStream(stream))
          {
          break;
          }
        stream.Reset();
        const char* pname;
        if(!interp->GetLastResult().GetArgument(0, 0, &pname))
          {
          vtkErrorMacro("Error getting array name from reader.");
          break;
          }
        if (!pname)
          {
          // Initializing a vtkstd::string to NULL does not have a defined
          // behavior.
          break;
          }
        vtkstd::string name = pname;

        vtksys_ios::ostringstream saname;
        saname << "Get" << arrayname << "ArrayStatus" << ends;
        // Get the array status.
        stream << vtkClientServerStream::Invoke
          << readerID
          << saname.str().c_str()
          << name.c_str()
          << vtkClientServerStream::End;
        if(!interp->ProcessStream(stream))
          {
          break;
          }
        stream.Reset();
        int status = 0;
        if(!interp->GetLastResult().GetArgument(0, 0, &status))
          {
          vtkErrorMacro("Error getting array status from reader.");
          break;
          }

        // Store the name/status pair in the result message.
        this->Internal->Result << name.c_str() << status;
        }
      }
    else
      {
      vtkErrorMacro("GetArraySettings cannot get an ID from "
                    << source->GetClassName() << "(" << source << ").");
      }
    }
  else
    {
    if(!this->ProcessModule)
      {
      vtkErrorMacro("GetArraySettings requires a ProcessModule.");
      }
    if(!source)
      {
      vtkErrorMacro("GetArraySettings cannot work with a NULL reader.");
      }
    }

  // End the message and return the stream.
  this->Internal->Result << vtkClientServerStream::End;
  return this->Internal->Result;
}
