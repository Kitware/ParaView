/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineFilter.cxx

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

/// \file pqPipelineFilter.cxx
/// \date 4/17/2006

#include "pqPipelineFilter.h"
#include <QList>


class pqPipelineFilterInternal : public QList<pqPipelineSource *> {};


pqPipelineFilter::pqPipelineFilter(vtkSMProxy *proxy,
    pqPipelineModel::ItemType type)
  : pqPipelineSource(proxy, type)
{
  this->Internal = new pqPipelineFilterInternal();
}

pqPipelineFilter::~pqPipelineFilter()
{
  if(this->Internal)
    {
    delete this->Internal;
    this->Internal = 0;
    }
}

void pqPipelineFilter::ClearConnections()
{
  pqPipelineSource::ClearConnections();
  if(this->Internal)
    {
    this->Internal->clear();
    }
}

int pqPipelineFilter::GetInputCount() const
{
  if(this->Internal)
    {
    return this->Internal->size();
    }
  return 0;
}

pqPipelineSource *pqPipelineFilter::GetInput(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->size())
    {
    return (*this->Internal)[index];
    }
  return 0;
}

int pqPipelineFilter::GetInputIndexFor(pqPipelineSource *input) const
{
  if(this->Internal && input)
    {
    return this->Internal->indexOf(input);
    }

  return -1;
}

bool pqPipelineFilter::HasInput(pqPipelineSource *input) const
{
  return this->GetInputIndexFor(input) != -1;
}

void pqPipelineFilter::AddInput(pqPipelineSource *intput)
{
  if(this->Internal && intput)
    {
    this->Internal->append(intput);
    }
}

void pqPipelineFilter::RemoveInput(pqPipelineSource *input)
{
  if(this->Internal && input)
    {
    QList<pqPipelineSource *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      if(*iter == input)
        {
        this->Internal->erase(iter);
        break;
        }
      }
    }
}


