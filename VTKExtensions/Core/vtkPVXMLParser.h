/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXMLParser.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVXMLParser
 *
 * This is a subclass of vtkXMLParser that constructs a representation
 * of parsed XML using vtkPVXMLElement.
*/

#ifndef vtkPVXMLParser_h
#define vtkPVXMLParser_h

#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro
#include "vtkSmartPointer.h"              // needed for vtkSmartPointer.
#include "vtkXMLParser.h"

class vtkPVXMLElement;

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkPVXMLParser : public vtkXMLParser
{
public:
  vtkTypeMacro(vtkPVXMLParser, vtkXMLParser);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPVXMLParser* New();

  /**
   * Write the parsed XML into the output stream.
   */
  void PrintXML(ostream& os);

  /**
   * Get the root element from the XML document.
   */
  vtkPVXMLElement* GetRootElement();

  //@{
  /**
   * If on, then the Parse method will NOT report an error using vtkErrorMacro.
   * Rather, it will just return false.  This feature is useful when simply
   * checking to see if a file is a valid XML file or there is otherwise a way
   * to recover from the failed parse.  This flag is off by default.
   */
  vtkGetMacro(SuppressErrorMessages, int);
  vtkSetMacro(SuppressErrorMessages, int);
  vtkBooleanMacro(SuppressErrorMessages, int);
  //@}

  /**
   * Convenience method to parse XML contents. Will return NULL is the
   * xmlcontents cannot be parsed.
   */
  static vtkSmartPointer<vtkPVXMLElement> ParseXML(
    const char* xmlcontents, bool suppress_errors = false);

protected:
  vtkPVXMLParser();
  ~vtkPVXMLParser() override;

  int SuppressErrorMessages;

  void StartElement(const char* name, const char** atts) override;
  void EndElement(const char* name) override;
  void CharacterDataHandler(const char* data, int length) override;

  void AddElement(vtkPVXMLElement* element);
  void PushOpenElement(vtkPVXMLElement* element);
  vtkPVXMLElement* PopOpenElement();

  // The root XML element.
  vtkPVXMLElement* RootElement;

  // The stack of elements currently being parsed.
  vtkPVXMLElement** OpenElements;
  unsigned int NumberOfOpenElements;
  unsigned int OpenElementsSize;

  // Counter to assign unique element ids to those that don't have any.
  unsigned int ElementIdIndex;

  // Called by Parse() to read the stream and call ParseBuffer.  Can
  // be replaced by subclasses to change how input is read.
  int ParseXML() override;

  // Overridden to implement the SuppressErrorMessages feature.
  void ReportXmlParseError() override;

private:
  vtkPVXMLParser(const vtkPVXMLParser&) = delete;
  void operator=(const vtkPVXMLParser&) = delete;
};

#endif
