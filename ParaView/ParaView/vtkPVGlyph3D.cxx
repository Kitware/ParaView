/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGlyph3D.cxx
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
#include "vtkPVGlyph3D.h"
#include "vtkPVApplication.h"
#include "vtkStringList.h"
#include "vtkKWCompositeCollection.h"
#include "vtkPVWindow.h"
#include "vtkObjectFactory.h"
#include "vtkPVSourceInterface.h"
#include "vtkPVData.h"
int vtkPVGlyph3DCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVGlyph3D::vtkPVGlyph3D()
{
  this->CommandFunction = vtkPVGlyph3DCommand;
  
  this->GlyphSource = NULL;

  // Glyph adds too its input, so sould not replace it.
  this->ReplaceInput = 0;
}

//----------------------------------------------------------------------------
vtkPVGlyph3D::~vtkPVGlyph3D()
{
  if (this->GlyphSource)
    {
    this->GlyphSource->UnRegister(this);
    this->GlyphSource = NULL;
    }
}

//----------------------------------------------------------------------------
vtkPVGlyph3D* vtkPVGlyph3D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVGlyph3D");
  if(ret)
    {
    return (vtkPVGlyph3D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVGlyph3D;
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::SetGlyphSource(vtkPVData *source)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (source)
    {
    source->Register(this);
    pvApp->BroadcastScript("%s SetSource %s", this->GetVTKSourceTclName(),
                          source->GetVTKDataTclName());
    }
  else
    {
    pvApp->BroadcastScript("%s SetSource {}", this->GetVTKSourceTclName());
    }
  if (this->GlyphSource)
    {
    this->GlyphSource->UnRegister(this);
    }
  this->GlyphSource = source;
}

//----------------------------------------------------------------------------
vtkPVData* vtkPVGlyph3D::GetGlyphSource()
{
  return this->GlyphSource;
}


//----------------------------------------------------------------------------
void vtkPVGlyph3D::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  
  this->vtkPVSource::CreateProperties();
    
  this->AddInputMenu("Input", "PVInput", "vtkPolyData",
                     "Select the input for the filter.", 
                     this->GetPVWindow()->GetSources());                            

  this->AddInputMenu("Glyph", "GlyphSource", "vtkPolyData",
                     "Select the data set to use as the glyph geometry.", 
                     this->GetPVWindow()->GetGlyphSources());
  
  this->AddModeList("Scale Mode", "ScaleMode", "Select whether/how to scale the glyphs");
  this->AddModeListItem("Scalar", 0);
  this->AddModeListItem("Vector", 1);
  this->AddModeListItem("Vector Components", 2);
  this->AddModeListItem("Data Scalaing Off", 3);
    
  this->AddModeList("Vector Mode", "VectorMode", "Select what to use as vectors for scaling/rotation");
  this->AddModeListItem("Vector", 0);
  this->AddModeListItem("Normal", 1);
  this->AddModeListItem("Vector RotationOff", 2);
    
  this->AddLabeledToggle("Orient", "Orient", "Select whether to orient the glyphs");
  this->AddLabeledToggle("Scaling", "Scaling", "Select whether to scale the glyphs");

  this->AddLabeledEntry("Scale Factor", "ScaleFactor", "Select the amount to scale the glyphs by");

  // Make sure the Input menus reflect the actual values.
  // This call is called too many times.  
  // It does not hurt anything, but should be cleaned up.
  this->UpdateParameterWidgets();  
}


