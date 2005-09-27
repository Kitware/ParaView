/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXMLPackageParser.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
  vtkPVWidget* GetPVWidget(vtkPVXMLElement* element, vtkPVSource* pvm, int store);

  // Get the vtkPVWindow currently being stored.
  vtkPVWindow* GetPVWindow();

  void ProcessConfiguration();
  void CreateReaderModule(vtkPVXMLElement* me);
  void CreateSourceModule(vtkPVXMLElement* me);
  void CreateFilterModule(vtkPVXMLElement* me);
  void CreateManipulator(vtkPVXMLElement* ma);
  void CreateWriter(vtkPVXMLElement* ma);
  int CreateModule(vtkPVXMLElement* me, vtkPVSource* pvm);
  int LoadPackageLibrary(vtkPVXMLElement* le);
  int LoadServerManagerFile(vtkPVXMLElement* le);
  int ParseVTKFilter(vtkPVXMLElement* filterElement,
                     vtkPVSource* pvm);

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
