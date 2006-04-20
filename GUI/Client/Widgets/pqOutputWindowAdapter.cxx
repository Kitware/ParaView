/*=========================================================================

   Program:   ParaQ
   Module:    pqOutputWindowAdapter.cxx

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

#include "pqOutputWindowAdapter.h"

#include <vtkObjectFactory.h>

vtkCxxRevisionMacro(pqOutputWindowAdapter, "1.3");
vtkStandardNewMacro(pqOutputWindowAdapter);

pqOutputWindowAdapter::pqOutputWindowAdapter() :
  TextCount(0),
  ErrorCount(0),
  WarningCount(0),
  GenericWarningCount(0)
{
}

pqOutputWindowAdapter::~pqOutputWindowAdapter()
{
}

const unsigned int pqOutputWindowAdapter::getTextCount()
{
  return this->TextCount;
}

const unsigned int pqOutputWindowAdapter::getErrorCount()
{
  return this->ErrorCount;
}

const unsigned int pqOutputWindowAdapter::getWarningCount()
{
  return this->WarningCount;
}

const unsigned int pqOutputWindowAdapter::getGenericWarningCount()
{
  return this->GenericWarningCount;
}

void pqOutputWindowAdapter::DisplayText(const char* text)
{
  ++this->TextCount;
  emit displayText(text);
}

void pqOutputWindowAdapter::DisplayErrorText(const char* text)
{
  ++this->ErrorCount;
  emit displayErrorText(text);
}

void pqOutputWindowAdapter::DisplayWarningText(const char* text)
{
  ++this->WarningCount;
  emit displayWarningText(text);
}

void pqOutputWindowAdapter::DisplayGenericWarningText(const char* text)
{
  ++this->GenericWarningCount;
  emit displayGenericWarningText(text);
}
