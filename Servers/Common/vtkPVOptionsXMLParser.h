/*=========================================================================
  
  Program:   ParaView
  Module:    vtkPVOptionsXMLParser.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVOptionsXMLParser - ParaView options storage
// .SECTION Description
// An object of this class represents a storage for ParaView options
// 
// These options can be retrieved during run-time, set using configuration file
// or using Command Line Arguments.
// 
// .SECTION See Also
// kwsys::CommandLineArguments

#ifndef __vtkPVOptionsXMLParser_h
#define __vtkPVOptionsXMLParser_h

#include "vtkXMLParser.h"
#include "vtkPVOptions.h" // for enum
class vtkPVOptionsXMLParserInternal;
class vtkPVOptions;

class VTK_EXPORT vtkPVOptionsXMLParser : public vtkXMLParser
{
public:
  static vtkPVOptionsXMLParser* New();
  vtkTypeRevisionMacro(vtkPVOptionsXMLParser,vtkXMLParser);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add arguments to the xml parser.  These should be the 
  // long arguments from the vtkPVOptions class of the form
  // --foo, and pass in the variable that needs to be set with the value.
  void AddBooleanArgument(const char* longarg, int* var, int type=vtkPVOptions::ALLPROCESS);
  void AddArgument(const char* longarg, int* var, int type=vtkPVOptions::ALLPROCESS);
  void AddArgument(const char* longarg, char** var, int type=vtkPVOptions::ALLPROCESS);
  void SetPVOptions(vtkPVOptions* o) 
    {
      this->PVOptions = o;
    }
protected:
  // Description:
  // Default constructor.
  vtkPVOptionsXMLParser();

  // Description:
  // Destructor.
  virtual ~vtkPVOptionsXMLParser();

  // Called when a new element is opened in the XML source.  Should be
  // replaced by subclasses to handle each element.
  //  name = Name of new element.
  //  atts = Null-terminated array of attribute name/value pairs.
  //         Even indices are attribute names, and odd indices are values.
  virtual void StartElement(const char* name, const char** atts);

  // Called at the end of an element in the XML source opened when
  // StartElement was called.
  virtual void EndElement(const char* name);  
  // Call to process the .. of  <Option>...</>
  void HandleOption(const char** atts);
  // Call to process the .. of  <Option>...</>
  void HandleProcessType(const char** atts);
private:
  vtkPVOptionsXMLParser(const vtkPVOptionsXMLParser&); // Not implemented
  void operator=(const vtkPVOptionsXMLParser&); // Not implemented
  int InPVXTag;
  vtkPVOptions* PVOptions;
  vtkPVOptionsXMLParserInternal* Internals;
};

#endif // #ifndef __vtkPVOptionsXMLParser_h

