/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerArraySelection.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVServerArraySelection.h"

#include "vtkClientServerInterpreter.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkSource.h"

#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVServerArraySelection);
vtkCxxRevisionMacro(vtkPVServerArraySelection, "1.1.2.1");

//----------------------------------------------------------------------------
vtkPVServerArraySelection::vtkPVServerArraySelection()
{
  this->Arrays = new vtkClientServerStream;
}

//----------------------------------------------------------------------------
vtkPVServerArraySelection::~vtkPVServerArraySelection()
{
  delete this->Arrays;
}

//----------------------------------------------------------------------------
void vtkPVServerArraySelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
const vtkClientServerStream&
vtkPVServerArraySelection::GetArraySettings(vtkSource* source, int point)
{
  // Reset the stream for a new list of array names.
  this->Arrays->Reset();
  *this->Arrays << vtkClientServerStream::Reply;

  // Make sure we have a process module.
  if(this->ProcessModule && source)
    {
    // Get the local process interpreter.
    vtkClientServerInterpreter* interp =
      this->ProcessModule->GetLocalInterpreter();

    // Get the ID for the reader.
    vtkClientServerID readerID = interp->GetIDFromObject(source);
    if(readerID.ID)
      {
      // Get the number of arrays.
      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke
             << readerID
             << (point? "GetNumberOfPointArrays":"GetNumberOfCellArrays")
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
        // Get the array name.
        stream << vtkClientServerStream::Invoke
               << readerID
               << (point? "GetPointArrayName":"GetCellArrayName") << i
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
        vtkstd::string name = pname;

        // Get the array status.
        stream << vtkClientServerStream::Invoke
               << readerID
               << (point? "GetPointArrayStatus":"GetCellArrayStatus")
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
        *this->Arrays << name.c_str() << status;
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
  *this->Arrays << vtkClientServerStream::End;
  return *this->Arrays;
}
