/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWLabel.cxx
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
#include "vtkKWApplication.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLabel );

vtkKWLabel::vtkKWLabel()
{
  this->Label    = new char[1];
  this->Label[0] = 0;
  this->LineType = vtkKWLabel::SingleLine;
  this->Width    = 0;
}

vtkKWLabel::~vtkKWLabel()
{
  delete [] this->Label;
}

void vtkKWLabel::SetLabel(const char* l)
{
  if(!l)
    {
    l = "";
    }
  
  delete [] this->Label;
  this->Label = strcpy(new char[strlen(l)+1], l);
  if(this->Application)
    {
    // if this has been created then change the text
    this->Script("%s configure -text {%s}", this->GetWidgetName(), 
		 this->Label);
    }
}


void vtkKWLabel::Create(vtkKWApplication *app, const char *args)
{
  const char *wname;

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Label already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  if ( this->LineType == vtkKWLabel::MultiLine )
    {
    this->Script("message %s -text {%s} %s -width %d", 
		 wname, this->Label, args, this->Width);
    }
  else
    {
    this->Script("label %s -text {%s} %s", wname, this->Label, args);
    }
}

void vtkKWLabel::SetLineType( int type )
{
  if ( this->Application )
    {
    if ( this->LineType != type )
      {
      this->Script("lindex [ %s configure -text ] 4", 
		   this->GetWidgetName());
      char *str = this->Application->GetMainInterp()->result;
      this->Script("destroy %s", this->GetWidgetName());
      if ( this->LineType == vtkKWLabel::MultiLine )
	{
	this->Script("message %s -text {%s} -width %d", 
		     this->GetWidgetName(), str, this->Width);
	}
      else
	{
	this->Script("label %s -text {%s}", 
		     this->GetWidgetName(), str);
	}		   
      }
    }
  this->LineType = type;
}




