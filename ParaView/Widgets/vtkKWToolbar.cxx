/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWToolbar.cxx
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
#include "vtkKWToolbar.h"
#include "vtkObjectFactory.h"
#include "vtkVector.txx"
#include "vtkVectorIterator.txx"

#ifndef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION

template class VTK_EXPORT vtkAbstractList<vtkKWWidget*>;
template class VTK_EXPORT vtkVector<vtkKWWidget*>;
template class VTK_EXPORT vtkAbstractIterator<vtkIdType,vtkKWWidget*>;
template class VTK_EXPORT vtkVectorIterator<vtkKWWidget*>;

#endif

//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWToolbar );


int vtkKWToolbarCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWToolbar::vtkKWToolbar()
{
  this->CommandFunction = vtkKWToolbarCommand;
  this->Expanding = 0;

  this->Frame = vtkKWWidget::New();
  this->Frame->SetParent(this);

  this->Widgets = vtkVector<vtkKWWidget*>::New();
}

//----------------------------------------------------------------------------
vtkKWToolbar::~vtkKWToolbar()
{
  this->Frame->Delete();
  this->Frame = 0;
  this->Widgets->Delete();
}


void vtkKWToolbar::AddWidget(vtkKWWidget* widget)
{
  this->Widgets->AppendItem(widget);
}

//----------------------------------------------------------------------------
void vtkKWToolbar::Create(vtkKWApplication *app)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("widget already created");
    return;
    }
  this->SetApplication(app);

  // create the main frame for this widget
  this->Script("frame %s -relief raised -bd 1", this->GetWidgetName());

  this->Script("bind %s <Configure> {%s ScheduleResize}",
               this->GetWidgetName(), this->GetTclName());

  this->Frame->Create(app, "frame", "");
  this->Script("pack %s -side left -expand yes   -anchor nw ",
	       this->Frame->GetWidgetName());


}

//----------------------------------------------------------------------------
void vtkKWToolbar::ScheduleResize()
{  
  if (this->Expanding)
    {
    return;
    }
  this->Expanding = 1;
  this->Script("after idle {%s Resize}", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkKWToolbar::UpdateWidgets()
{
  if ( this->Widgets->GetNumberOfItems() > 0 )
    {
    vtkVectorIterator<vtkKWWidget*>* it = this->Widgets->NewIterator();
    
    int totReqWidth=0;
    while ( !it->IsDoneWithTraversal() )
      {
      vtkKWWidget* widget = 0;
      if (it->GetData(widget) == VTK_OK)
	{
	this->Script("winfo reqwidth %s", widget->GetWidgetName());
	totReqWidth += this->GetIntegerResult(this->Application);
	}
      it->GoToNextItem();
      }

    this->Script("winfo width %s", this->GetWidgetName());
    int width = this->GetIntegerResult(this->Application);

    int widthWidget = totReqWidth / this->Widgets->GetNumberOfItems();
    int numPerRow = width / widthWidget;

    if ( numPerRow > 0 )
      {
      int row = 0;
      ostrstream s;
      it->InitTraversal();
      int num = 0;
      while ( !it->IsDoneWithTraversal() )
	{
	vtkKWWidget* widget = 0;
	if (it->GetData(widget) == VTK_OK)
	  {
	  s << "grid " << widget->GetWidgetName() << " -row " 
	    << row << " -column " << num << " -sticky nsew " << endl;
	  num++;
	  if ( num == numPerRow ) { row++; num=0;}
	  }
	it->GoToNextItem();
	}
      s << ends;
      this->Script(s.str());
      s.rdbuf()->freeze(0);
      }
    it->Delete();

    }
}

//----------------------------------------------------------------------------
void vtkKWToolbar::Resize()
{
  this->UpdateWidgets();
  this->Expanding = 0;
}


//----------------------------------------------------------------------------
void vtkKWToolbar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Frame: " << this->Frame << endl;
}
