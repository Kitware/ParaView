/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXMLPackageParser.h
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
// .NAME vtkPVXMLPackageParser parses ParaView Package configuration files.
// .SECTION Description
// This is a subclass of vtkPVXMLParser intended to parse the package
// configuration files specifying the widgets used to control each
// module in the package.
#ifndef __vtkPVXMLPackageParser_h
#define __vtkPVXMLPackageParser_h

#include "vtkPVXMLParser.h"

//BTX
template <class key, class data>
class vtkArrayMap;
//ETX

class vtkPVSource;
class vtkPVWindow;
class vtkPVWidget;

class VTK_EXPORT vtkPVXMLPackageParser : public vtkPVXMLParser
{
public:
  vtkTypeRevisionMacro(vtkPVXMLPackageParser,vtkPVXMLParser);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPVXMLPackageParser* New();
  
  // Description:
  // Create widget prototypes from parsed configuration and store them
  // in the given window.  Should be called after a Parse() method.
  void StoreConfiguration(vtkPVWindow* window);
  
protected:
  vtkPVXMLPackageParser();
  ~vtkPVXMLPackageParser();
  
  // Get the vtkPVWidget corresponding to the given vtkPVXMLElement.
  vtkPVWidget* GetPVWidget(vtkPVXMLElement* element);
  
  // Get the vtkPVWindow currently being stored.
  vtkPVWindow* GetPVWindow();
  
  void ProcessConfiguration();
  void CreateReaderModule(vtkPVXMLElement* me);
  void CreateSourceModule(vtkPVXMLElement* me);
  void CreateFilterModule(vtkPVXMLElement* me);
  int CreateModule(vtkPVXMLElement* me, vtkPVSource* pvm);
  int LoadLibrary(vtkPVXMLElement* le);
  
  //BTX
  typedef vtkArrayMap<vtkPVXMLElement*, vtkPVWidget*> InternalWidgetMap;
  //ETX
  
  // Map of XML element representation to widget.
  InternalWidgetMap* WidgetMap;
  
  // The window into which the modules will be stored.
  vtkPVWindow* Window;
  
  //BTX
  friend class vtkPVXMLElement;
  friend class vtkPVWidget;
  //ETX
  
private:
  
  // Used by GetPVWidget.  Do not call directly.
  vtkPVWidget* CreatePVWidget(vtkPVXMLElement* element);

private:
  vtkPVXMLPackageParser(const vtkPVXMLPackageParser&);  // Not implemented.
  void operator=(const vtkPVXMLPackageParser&);  // Not implemented.
};

#endif
