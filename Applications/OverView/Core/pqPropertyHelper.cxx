/*=========================================================================

   Program: ParaView
   Module:    pqPropertyHelper.cxx

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

#include "pqPropertyHelper.h"
#include "vtkStdString.h"

////////////////////////////////////////////////////////////////////
// pqPropertyHelper

pqPropertyHelper::pqPropertyHelper(vtkSMProxy* proxy, const char* name) :
  vtkSMPropertyHelper(proxy, name)
{
}

void pqPropertyHelper::Set(const vtkStdString& value)
{
  vtkSMPropertyHelper::Set(0, value.c_str());
}

void pqPropertyHelper::Set(unsigned int index, const vtkStdString& value)
{
  vtkSMPropertyHelper::Set(index, value.c_str());
}

void pqPropertyHelper::Set(const QString& value)
{
  vtkSMPropertyHelper::Set(0, value.toUtf8().data());
}

void pqPropertyHelper::Set(unsigned int index, const QString& value)
{
  vtkSMPropertyHelper::Set(index, value.toUtf8().data());
}

void pqPropertyHelper::Set(const QStringList& value)
{
  this->SetNumberOfElements(value.size());
  for(int i = 0; i != value.size(); ++i)
    vtkSMPropertyHelper::Set(i, value.at(i).toUtf8().data());
}

const QString pqPropertyHelper::GetAsString(unsigned int index)
{
  return vtkSMPropertyHelper::GetAsString(index);
}

const QStringList pqPropertyHelper::GetAsStringList()
{
  QStringList result;
  for(int i = 0; i != this->GetNumberOfElements(); ++i)
    result.push_back(vtkSMPropertyHelper::GetAsString(i));
  return result;
}

