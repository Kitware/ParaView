/*=========================================================================

   Program: ParaView
   Module:    pqStandardTreeLayoutStrategies.cxx

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

#include "pqStandardTreeLayoutStrategies.h"

#include "vtkBoxLayoutStrategy.h"
#include "vtkSliceAndDiceLayoutStrategy.h"
#include "vtkSquarifyLayoutStrategy.h"
#include "vtkStackedTreeLayoutStrategy.h"

pqStandardTreeLayoutStrategies::pqStandardTreeLayoutStrategies(QObject* o)
  : QObject(o)
{
  this->BoxStrategy             = vtkSmartPointer<vtkBoxLayoutStrategy>::New();
  this->SliceAndDiceStrategy    = vtkSmartPointer<vtkSliceAndDiceLayoutStrategy>::New();
  this->SquarifyStrategy        = vtkSmartPointer<vtkSquarifyLayoutStrategy>::New();
  this->StackedTreeStrategy     = vtkSmartPointer<vtkStackedTreeLayoutStrategy>::New();
}

pqStandardTreeLayoutStrategies::~pqStandardTreeLayoutStrategies()
{
}

QStringList pqStandardTreeLayoutStrategies::treeLayoutStrategies() const
{
  return QStringList() <<
  "Box" <<
  "SliceAndDice" <<
  "Squarify" <<
  "StackedTree"
  ;
}

vtkAreaLayoutStrategy* pqStandardTreeLayoutStrategies::getTreeLayoutStrategy(const QString& layoutStrategy)
{
  if(layoutStrategy == "Box")
    {
    return this->BoxStrategy;
    }
  else if(layoutStrategy == "SliceAndDice")
    {
    return this->SliceAndDiceStrategy;
    }
  else if(layoutStrategy == "Squarify")
    {
    return this->SquarifyStrategy;
    }
  else if(layoutStrategy == "StackedTree")
    {
    return this->StackedTreeStrategy;
    }

  return NULL;
}
