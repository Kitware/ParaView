/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLActor2DReader.cxx
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
#include "vtkXMLActor2DReader.h"

#include "vtkActor2D.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLProperty2DReader.h"
#include "vtkXMLActor2DWriter.h"

vtkStandardNewMacro(vtkXMLActor2DReader);
vtkCxxRevisionMacro(vtkXMLActor2DReader, "1.2");

//----------------------------------------------------------------------------
char* vtkXMLActor2DReader::GetRootElementName()
{
  return "Actor2D";
}

//----------------------------------------------------------------------------
int vtkXMLActor2DReader::Parse(vtkXMLDataElement *elem)
{
  if (!this->Superclass::Parse(elem))
    {
    return 0;
    }

  vtkActor2D *obj = vtkActor2D::SafeDownCast(this->Object);
  if (!obj)
    {
    vtkWarningMacro(<< "The Actor2D is not set!");
    return 0;
    }

  // Get attributes

  float fbuffer2[2];
  int ival;

  if (elem->GetScalarAttribute("LayerNumber", ival))
    {
    obj->SetLayerNumber(ival);
    }

  vtkCoordinate *coord = obj->GetPositionCoordinate();
  if (coord && elem->GetVectorAttribute("Position", 2, fbuffer2) == 2)
    {
    int sys = coord->GetCoordinateSystem();
    coord->SetCoordinateSystemToNormalizedViewport();
    coord->SetValue(fbuffer2[0], fbuffer2[1]);
    coord->SetCoordinateSystem(sys);
    }
  
  coord = obj->GetPosition2Coordinate();
  if (coord && elem->GetVectorAttribute("Position2", 2, fbuffer2) == 2)
    {
    int sys = coord->GetCoordinateSystem();
    coord->SetCoordinateSystemToNormalizedViewport();
    coord->SetValue(fbuffer2[0], fbuffer2[1]);
    coord->SetCoordinateSystem(sys);
    }
  
  // Get nested elements
  
  // Property 2D

  vtkXMLProperty2DReader *xmlr = vtkXMLProperty2DReader::New();
  if (xmlr->IsInNestedElement(
        elem, vtkXMLActor2DWriter::GetPropertyElementName()))
    {
    vtkProperty2D *prop2d = obj->GetProperty();
    if (!prop2d)
      {
      prop2d = vtkProperty2D::New();
      obj->SetProperty(prop2d);
      prop2d->Delete();
      }
    xmlr->SetObject(prop2d);
    xmlr->ParseInNestedElement(
      elem, vtkXMLActor2DWriter::GetPropertyElementName());
    }
  xmlr->Delete();
  
  return 1;
}
