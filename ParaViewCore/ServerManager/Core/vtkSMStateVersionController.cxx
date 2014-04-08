/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStateVersionController.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStateVersionController.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"

#include <set>
#include <string>
#include <vector>
#include <vtk_pugixml.h>
#include <vtksys/ios/sstream>

namespace
{
  class vtkSMVersion
    {
    public:
      int Major;
      int Minor;
      int Patch;

      vtkSMVersion(int major, int minor, int patch):
        Major(major), Minor(minor), Patch(patch)
        {
        }

      bool operator < (const vtkSMVersion& other) const
        {
        if (this->Major == other.Major)
          {
          if (this->Minor == other.Minor)
            {
            return this->Patch < other.Patch;
            }
          else
            {
            return this->Minor < other.Minor;
            }
          }
        else
          {
          return this->Major < other.Major;
          }
        }
    };

  //===========================================================================
  // Helper functions
  //===========================================================================
  void PurgeElements(const pugi::xpath_node_set &elements_of_interest)
    {
    for (pugi::xpath_node_set::const_iterator iter = elements_of_interest.begin();
      iter != elements_of_interest.end(); ++iter)
      {
      pugi::xml_node node = iter->node();
      node.parent().remove_child(node);
      }
    }
  void PurgeElement(const pugi::xml_node& node)
    {
    node.parent().remove_child(node);
    }

  //===========================================================================
  bool Process_4_0_to_4_1(pugi::xml_document &document)
    {
    //-------------------------------------------------------------------------
    // For CTHPart filter, we need to convert AddDoubleVolumeArrayName,
    // AddUnsignedCharVolumeArrayName and AddFloatVolumeArrayName to a single
    // VolumeArrays property. The type specific separation is no longer
    // needed.
    //-------------------------------------------------------------------------
    pugi::xpath_node_set cthparts =
      document.select_nodes(
        "//ServerManagerState/Proxy[@group='filters' and @type='CTHPart']");
    for (pugi::xpath_node_set::const_iterator iter = cthparts.begin();
      iter != cthparts.end(); ++iter)
      {
      std::set<std::string> selected_arrays;

      pugi::xml_node cthpart_node = iter->node();
      pugi::xpath_node_set elements_of_interest = cthpart_node.select_nodes(
        "//Property[@name='AddDoubleVolumeArrayName' "
                "or @name='AddFloatVolumeArrayName' "
                "or @name='AddUnsignedCharVolumeArrayName']"
        "/Element[@value]");

      for (pugi::xpath_node_set::const_iterator iter2 = elements_of_interest.begin();
        iter2 != elements_of_interest.end(); ++iter2)
        {
        selected_arrays.insert(
          iter2->node().attribute("value").value());
        }

      // Remove all of those old property elements.
      PurgeElements(cthpart_node.select_nodes(
          "//Property[@name='AddDoubleVolumeArrayName' "
                "or @name='AddFloatVolumeArrayName' "
                "or @name='AddUnsignedCharVolumeArrayName']"));

      // Add new XML state for "VolumeArrays" property with the value as
      // "selected_arrays".
      vtksys_ios::ostringstream stream;
      stream << "<Property name=\"VolumeArrays\" >\n";
      int index=0;
      for (std::set<std::string>::const_iterator it = selected_arrays.begin();
        it != selected_arrays.end(); ++it, ++index)
        {
        stream << "  <Element index=\"" << index << "\" value=\"" << (*it).c_str() << "\"/>\n";
        }
      stream << "</Property>\n";
      std::string buffer = stream.str();
      if (!cthpart_node.append_buffer(buffer.c_str(), buffer.size()))
        {
        abort();
        }
      }
    //-------------------------------------------------------------------------
    return true;
    }

  //===========================================================================
  bool Process_4_1_to_4_2(pugi::xml_document &document)
    {
    //-------------------------------------------------------------------------
    // Convert ColorArrayName and ColorAttributeType properties to new style.
    // Since of separate properties, we now have just 1 ColorArrayName property
    // with 5 elements.
    //-------------------------------------------------------------------------
    // Find all representations.
    pugi::xpath_node_set representation_elements =
      document.select_nodes("//ServerManagerState/Proxy[@group='representations']");
    for (pugi::xpath_node_set::const_iterator iter = representation_elements.begin();
      iter != representation_elements.end(); ++iter)
      {
      // select ColorAttributeType and ColorArrayName properties for each
      // representation.
      std::string colorArrayName;
      if (pugi::xpath_node nameElement = iter->node().select_single_node(
          "//Property[@name='ColorArrayName' and @number_of_elements='1']"
          "/Element[@index='0' and @value]"))
        {
        colorArrayName = nameElement.node().attribute("value").value();

        // remove the "Property" xml-element
        PurgeElement(nameElement.node().parent());
        }

      std::string attributeType("");
      if (pugi::xpath_node typeElement = iter->node().select_single_node(
          "//Property[@name='ColorAttributeType' and @number_of_elements='1']"
          "/Element[@index='0' and @value]"))
        {
        attributeType = typeElement.node().attribute("value").value();

        // remove the "Property" xml-element
        PurgeElement(typeElement.node().parent());
        }
      if (!colorArrayName.empty() || !attributeType.empty())
        {
        vtksys_ios::ostringstream stream;
        stream << "<Property name=\"ColorArrayName\" number_of_elements=\"5\">\n"
          << "   <Element index=\"0\" value=\"\" />\n"
          << "   <Element index=\"1\" value=\"\" />\n"
          << "   <Element index=\"2\" value=\"\" />\n"
          << "   <Element index=\"3\" value=\"" << attributeType.c_str() << "\" />\n"
          << "   <Element index=\"4\" value=\"" << colorArrayName.c_str()<< "\" />\n"
          << "</Property>\n";
        std::string buffer = stream.str();
        if (!iter->node().append_buffer(buffer.c_str(), buffer.size()))
          {
          abort();
          }
        }
      }
    //-------------------------------------------------------------------------
    return true;
    }
};

vtkStandardNewMacro(vtkSMStateVersionController);
//----------------------------------------------------------------------------
vtkSMStateVersionController::vtkSMStateVersionController()
{
}

//----------------------------------------------------------------------------
vtkSMStateVersionController::~vtkSMStateVersionController()
{
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::Process(vtkPVXMLElement* parent)
{
  vtkPVXMLElement* root = parent;
  if (parent && strcmp(parent->GetName(), "ServerManagerState") != 0)
    {
    root = root->FindNestedElementByName("ServerManagerState");
    }

  if (!root || strcmp(root->GetName(), "ServerManagerState") != 0)
    {
    vtkErrorMacro("Invalid root element. Expected \"ServerManagerState\"");
    return false;
    }

  vtkSMVersion version(0, 0, 0);
  if (const char* str_version = root->GetAttribute("version"))
    {
    int v[3];
    sscanf(str_version, "%d.%d.%d", &v[0], &v[1], &v[2]);
    version = vtkSMVersion(v[0], v[1], v[2]);
    }

  bool status = true;
  if (version < vtkSMVersion(4, 0, 1))
    {
    vtkWarningMacro("State file version is less than 4.0."
      "We will try to load the state file. It's recommended, however, "
      "that you load the state in ParaView 4.0.1 (or 4.1.0) and save a newer version "
      "so that it can be loaded more faithfully. "
      "Loading state files generated from ParaView versions older than 4.0.1 "
      "is no longer supported.");

    version = vtkSMVersion(4, 0, 1);
    }

  // A little hackish for now, convert vtkPVXMLElement to string.
  vtksys_ios::ostringstream stream;
  root->PrintXML(stream, vtkIndent());

  // parse using pugi.
  pugi::xml_document document;

  if (!document.load(stream.str().c_str()))
    {
    vtkErrorMacro("Failed to convert from vtkPVXMLElement to pugi::xml_document");
    return false;
    }

  if (status && version < vtkSMVersion(4, 1, 0))
    {
    status = Process_4_0_to_4_1(document);
    version = vtkSMVersion(4, 1, 0);
    }

  if (status && version < vtkSMVersion(4, 2, 0))
    {
    status = Process_4_1_to_4_2(document);
    version = vtkSMVersion(4, 2, 0);
    }

  if (status)
    {
    vtksys_ios::ostringstream stream2;
    document.save(stream2, "  ");

    vtkNew<vtkPVXMLParser> parser;
    if (parser->Parse(stream2.str().c_str()))
      {
      root->RemoveAllNestedElements();
      vtkPVXMLElement* newRoot = parser->GetRootElement();

      newRoot->CopyAttributesTo(root);
      for (unsigned int cc=0, max=newRoot->GetNumberOfNestedElements(); cc <max;cc++)
        {
        root->AddNestedElement(newRoot->GetNestedElement(cc));
        }
      }
    else
      {
      vtkErrorMacro("Internal error: Error parsing converted XML state file.");
      }
    }
  return status;
}

//----------------------------------------------------------------------------
void vtkSMStateVersionController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
