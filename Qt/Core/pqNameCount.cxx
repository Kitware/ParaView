/*=========================================================================

   Program: ParaView
   Module:    pqNameCount.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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

/// \file pqNameCount.cxx
/// \date 12/9/2005

#include "pqNameCount.h"

#include <QHash>
#include <QString>


class pqNameCountInternal : public QHash<QString, unsigned int> {};


pqNameCount::pqNameCount()
{
  this->Internal = new pqNameCountInternal();
}

pqNameCount::~pqNameCount()
{
  if(this->Internal)
    delete this->Internal;
}

unsigned int pqNameCount::GetCount(const QString &name)
{
  unsigned int count = 1;
  if(this->Internal)
    {
    QHash<QString, unsigned int>::Iterator iter = this->Internal->find(name);
    if(iter == this->Internal->end())
      this->Internal->insert(name, count);
    else
      count = *iter;
    }

  return count;
}

unsigned int pqNameCount::GetCountAndIncrement(const QString &name)
{
  unsigned int count = 1;
  if(this->Internal)
    {
    QHash<QString, unsigned int>::Iterator iter = this->Internal->find(name);
    if(iter == this->Internal->end())
      this->Internal->insert(name, count + 1);
    else
      {
      count = *iter;
      (*iter) += 1;
      }
    }

  return count;
}

void pqNameCount::IncrementCount(const QString &name)
{
  if(this->Internal)
    {
    QHash<QString, unsigned int>::Iterator iter = this->Internal->find(name);
    if(iter != this->Internal->end())
      (*iter) += 1;
    }
}

void pqNameCount::SetCount(const QString &name, unsigned int count)
{
  if(this->Internal)
    {
    QHash<QString, unsigned int>::Iterator iter = this->Internal->find(name);
    if(iter != this->Internal->end())
      {
      (*iter) = count;
      }
    else
      {
      // Add the new name into the map.
      this->Internal->insert(name, count);
      }
    }
}

void pqNameCount::Reset()
{
  if(this->Internal)
    {
    this->Internal->clear();
    }
}


