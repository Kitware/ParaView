/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineSource.cxx

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

/// \file pqPipelineSource.cxx
/// \date 4/17/2006

#include "pqPipelineSource.h"

#include "pqPipelineDisplay.h"
#include "pqPipelineLink.h"
#include "pqPipelineFilter.h"
#include <QList>
#include "vtkSMProxy.h"


class pqPipelineSourceInternal : public QList<pqPipelineObject *> {};


pqPipelineSource::pqPipelineSource(vtkSMProxy *proxy,
    pqPipelineModel::ItemType type)
  : pqPipelineObject(), ProxyName()
{
  this->Internal = new pqPipelineSourceInternal();
  this->Display = new pqPipelineDisplay();
  this->Proxy = 0;

  // Set the model item type.
  this->SetType(type);

  // Set the proxy, which will increment the reference count.
  this->SetProxy(proxy);
}

pqPipelineSource::~pqPipelineSource()
{
  this->ClearConnections();
  this->SetProxy(0);
  if(this->Internal)
    {
    delete this->Internal;
    this->Internal = 0;
    }

  if(this->Display)
    {
    delete this->Display;
    this->Display = 0;
    }
}

void pqPipelineSource::ClearConnections()
{
  if(this->Internal)
    {
    // Note: The source object should delete the memory allocated
    // for the link object since the server object has no reference.
    QList<pqPipelineObject *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      if((*iter)->GetType() == pqPipelineModel::Link)
        {
        delete *iter;
        *iter = 0;
        }
      }

    this->Internal->clear();
    }
}

void pqPipelineSource::SetProxy(vtkSMProxy *proxy)
{
  if(this->Proxy != proxy)
    {
    // Release the reference to the old proxy.
    if(this->Proxy)
      {
      this->Proxy->UnRegister(0);
      }

    // Save the pointer and add a reference to the new proxy.
    this->Proxy = proxy;
    if(this->Proxy)
      {
      this->Proxy->Register(0);
      }
    }
}

int pqPipelineSource::GetOutputCount() const
{
  if(this->Internal)
    {
    return this->Internal->size();
    }

  return 0;
}

pqPipelineObject *pqPipelineSource::GetOutput(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->size())
    {
    return (*this->Internal)[index];
    }

  return 0;
}

int pqPipelineSource::GetOutputIndexFor(pqPipelineObject *output) const
{
  if(this->Internal && output)
    {
    pqPipelineLink *link = 0;
    pqPipelineObject *current = 0;
    QList<pqPipelineObject *>::Iterator iter = this->Internal->begin();
    for(int index = 0; iter != this->Internal->end(); ++iter, ++index)
      {
      // The output may be in a link object.
      current = *iter;
      if(current->GetType() == pqPipelineModel::Link)
        {
        link = dynamic_cast<pqPipelineLink *>(current);
        current = link->GetLink();
        }

      // The requested output object may be a link, so test the
      // list item as well as the link output.
      if(current == output || *iter == output)
        {
        return index;
        }
      }
    }

  return -1;
}

bool pqPipelineSource::HasOutput(pqPipelineObject *output) const
{
  return this->GetOutputIndexFor(output) != -1;
}

void pqPipelineSource::AddOutput(pqPipelineObject *output)
{
  if(this->Internal && output)
    {
    this->Internal->append(output);
    }
}

void pqPipelineSource::InsertOutput(int index, pqPipelineObject *output)
{
  if(this->Internal && output)
    {
    this->Internal->insert(index, output);
    }
}

void pqPipelineSource::RemoveOutput(pqPipelineObject *output)
{
  if(this->Internal && output)
    {
    QList<pqPipelineObject *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      if(*iter == output)
        {
        this->Internal->erase(iter);
        break;
        }
      }
    }
}


