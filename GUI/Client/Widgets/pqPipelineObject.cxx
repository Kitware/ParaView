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

#include "pqPipelineObject.h"

#include "pqPipelineWindow.h"
#include "vtkSMProxy.h"
#include <QList>


class pqPipelineObjectInternal
{
public:
  pqPipelineObjectInternal();
  ~pqPipelineObjectInternal() {}

  QList<pqPipelineObject *> Inputs;
  QList<pqPipelineObject *> Outputs;
};


pqPipelineObjectInternal::pqPipelineObjectInternal()
  : Inputs(), Outputs()
{
}


pqPipelineObject::pqPipelineObject(vtkSMProxy *proxy, ObjectType type)
  : ProxyName()
{
  this->Internal = new pqPipelineObjectInternal();
  this->Display = 0;
  this->Type = type;
  this->Proxy = proxy;
  this->Window = 0;
}

pqPipelineObject::~pqPipelineObject()
{
  // Clean up the connection lists.
  if(this->Internal)
    {
    delete this->Internal;
    }
}

int pqPipelineObject::GetInputCount() const
{
  if(this->Internal)
    {
    return this->Internal->Inputs.size();
    }
  return 0;
}

pqPipelineObject *pqPipelineObject::GetInput(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->Inputs.size())
    {
    return this->Internal->Inputs[index];
    }
  return 0;
}

bool pqPipelineObject::HasInput(pqPipelineObject *input) const
{
  if(this->Internal && input)
    {
    QList<pqPipelineObject *>::Iterator iter = this->Internal->Inputs.begin();
    for( ; iter != this->Internal->Inputs.end(); ++iter)
      {
      if(*iter == input)
        {
        return true;
        }
      }
    }

  return false;
}

void pqPipelineObject::AddInput(pqPipelineObject *intput)
{
  if(this->Internal && intput)
    {
    this->Internal->Inputs.append(intput);
    }
}

void pqPipelineObject::RemoveInput(pqPipelineObject *input)
{
  if(this->Internal && input)
    {
    QList<pqPipelineObject *>::Iterator iter = this->Internal->Inputs.begin();
    for( ; iter != this->Internal->Inputs.end(); ++iter)
      {
      if(*iter == input)
        {
        this->Internal->Inputs.erase(iter);
        break;
        }
      }
    }
}

int pqPipelineObject::GetOutputCount() const
{
  if(this->Internal)
    {
    return this->Internal->Outputs.size();
    }
  return 0;
}

pqPipelineObject *pqPipelineObject::GetOutput(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->Outputs.size())
    {
    return this->Internal->Outputs[index];
    }
  return 0;
}

bool pqPipelineObject::HasOutput(pqPipelineObject *output) const
{
  if(this->Internal && output)
    {
    QList<pqPipelineObject *>::Iterator iter = this->Internal->Outputs.begin();
    for( ; iter != this->Internal->Outputs.end(); ++iter)
      {
      if(*iter == output)
        {
        return true;
        }
      }
    }

  return false;
}

void pqPipelineObject::AddOutput(pqPipelineObject *output)
{
  if(this->Internal && output)
    {
    this->Internal->Outputs.append(output);
    }
}

void pqPipelineObject::RemoveOutput(pqPipelineObject *output)
{
  if(this->Internal && output)
    {
    QList<pqPipelineObject *>::Iterator iter = this->Internal->Outputs.begin();
    for( ; iter != this->Internal->Outputs.end(); ++iter)
      {
      if(*iter == output)
        {
        this->Internal->Outputs.erase(iter);
        break;
        }
      }
    }
}

void pqPipelineObject::ClearConnections()
{
  if(this->Internal)
    {
    this->Internal->Inputs.clear();
    this->Internal->Outputs.clear();
    }
}


