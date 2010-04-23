/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerArrayHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVServerArrayHelper.h"

#include "vtkArrayIteratorIncludes.h"
#include "vtkArrayIteratorTemplate.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkPVServerArrayHelper);
//-----------------------------------------------------------------------------
vtkPVServerArrayHelper::vtkPVServerArrayHelper()
{
  this->Result = new vtkClientServerStream;
}

//-----------------------------------------------------------------------------
vtkPVServerArrayHelper::~vtkPVServerArrayHelper()
{
  delete this->Result;
}

//-----------------------------------------------------------------------------
template<class iterT>
void vtkPVServerArrayHelperSerializer(
  iterT* iter, vtkClientServerStream& stream)
{
  vtkIdType numValues = iter->GetNumberOfValues();
  for (vtkIdType cc=0; cc < numValues; cc++)
    {
    stream << iter->GetValue(cc);
    }
}

//-----------------------------------------------------------------------------
const vtkClientServerStream&
vtkPVServerArrayHelper::GetArray(vtkObject* object,
  const char* command)
{
  this->Result->Reset();

  vtkProcessModule* pm = this->GetProcessModule();
  if (!pm)
    {
    vtkErrorMacro("ProcessModule must be set,");
    return this->EmptyResult();
    }
  vtkClientServerInterpreter* interp = pm->GetInterpreter();

  vtkClientServerID id = interp->GetIDFromObject(object);
  if (!id.ID)
    {
    vtkErrorMacro("Failed to locate ID for server side object.");
    return this->EmptyResult();
    }

  vtkClientServerStream commandStream;
  commandStream << vtkClientServerStream::Invoke
    << id
    << command
    << vtkClientServerStream::End;
  interp->ProcessStream(commandStream);
  vtkDataArray* dataArray = NULL;
  if (!interp->GetLastResult().GetArgument(0, 0, (vtkObjectBase**)&dataArray))
    {
    vtkErrorMacro("Error getting return value of command: " << command);
    return this->EmptyResult();
    }
  if(!dataArray)
    {
    return this->EmptyResult();
    }
  vtkArrayIterator* iter = dataArray->NewIterator();
  *this->Result << vtkClientServerStream::Reply;
  switch (dataArray->GetDataType())
    {
    vtkArrayIteratorTemplateMacro(
      vtkPVServerArrayHelperSerializer(
        static_cast<VTK_TT*>(iter), *this->Result));
    }
  iter->Delete();

  *this->Result << vtkClientServerStream::End;
  return *this->Result;
}


//-----------------------------------------------------------------------------
const vtkClientServerStream& vtkPVServerArrayHelper::EmptyResult()
{
  this->Result->Reset();
  *this->Result << vtkClientServerStream::Reply
    << vtkClientServerStream::End;
  return *this->Result;
}

//-----------------------------------------------------------------------------
void vtkPVServerArrayHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
