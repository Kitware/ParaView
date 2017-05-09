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

// Don't include vtkAxis. Cannot add dependency on vtkChartsCore in
// vtkPVServerManagerCore.
// #include "vtkAxis.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"

#include <set>
#include <sstream>
#include <sstream>
#include <string>
#include <vector>
#include <vtk_pugixml.h>

using namespace pugi;
using namespace std;

namespace
{
string toString(int i)
{
  ostringstream ostr;
  ostr << i;
  return ostr.str();
}

class vtkSMVersion
{
public:
  int Major;
  int Minor;
  int Patch;

  vtkSMVersion(int major, int minor, int patch)
    : Major(major)
    , Minor(minor)
    , Patch(patch)
  {
  }

  bool operator<(const vtkSMVersion& other) const
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
void PurgeElements(const pugi::xpath_node_set& elements_of_interest)
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
bool Process_4_0_to_4_1(pugi::xml_document& document)
{
  //-------------------------------------------------------------------------
  // For CTHPart filter, we need to convert AddDoubleVolumeArrayName,
  // AddUnsignedCharVolumeArrayName and AddFloatVolumeArrayName to a single
  // VolumeArrays property. The type specific separation is no longer
  // needed.
  //-------------------------------------------------------------------------
  pugi::xpath_node_set cthparts =
    document.select_nodes("//ServerManagerState/Proxy[@group='filters' and @type='CTHPart']");
  for (pugi::xpath_node_set::const_iterator iter = cthparts.begin(); iter != cthparts.end(); ++iter)
  {
    std::set<std::string> selected_arrays;

    pugi::xml_node cthpart_node = iter->node();
    pugi::xpath_node_set elements_of_interest =
      cthpart_node.select_nodes("//Property[@name='AddDoubleVolumeArrayName' "
                                "or @name='AddFloatVolumeArrayName' "
                                "or @name='AddUnsignedCharVolumeArrayName']"
                                "/Element[@value]");

    for (pugi::xpath_node_set::const_iterator iter2 = elements_of_interest.begin();
         iter2 != elements_of_interest.end(); ++iter2)
    {
      selected_arrays.insert(iter2->node().attribute("value").value());
    }

    // Remove all of those old property elements.
    PurgeElements(cthpart_node.select_nodes("//Property[@name='AddDoubleVolumeArrayName' "
                                            "or @name='AddFloatVolumeArrayName' "
                                            "or @name='AddUnsignedCharVolumeArrayName']"));

    // Add new XML state for "VolumeArrays" property with the value as
    // "selected_arrays".
    std::ostringstream stream;
    stream << "<Property name=\"VolumeArrays\" >\n";
    int index = 0;
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
struct Process_4_1_to_4_2
{
  static const char* const NAME;
  static const char* const VALUE;

  bool operator()(xml_document& document)
  {
    bool ret = ConvertRepresentationColorArrayName(document) && ConvertChartNodes(document) &&
      ConvertGlyphFilter(document);
    return ret;
  }

  static bool ConvertRepresentationColorArrayName(xml_document& document)
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
        std::ostringstream stream;
        stream << "<Property name=\"ColorArrayName\" number_of_elements=\"5\">\n"
               << "   <Element index=\"0\" value=\"\" />\n"
               << "   <Element index=\"1\" value=\"\" />\n"
               << "   <Element index=\"2\" value=\"\" />\n"
               << "   <Element index=\"3\" value=\"" << attributeType.c_str() << "\" />\n"
               << "   <Element index=\"4\" value=\"" << colorArrayName.c_str() << "\" />\n"
               << "</Property>\n";
        std::string buffer = stream.str();
        if (!iter->node().append_buffer(buffer.c_str(), buffer.size()))
        {
          abort();
        }
      }
    }

    //-------------------------------------------------------------------------
    // remove camera manipulator proxies and interactorstyles proxies.
    //-------------------------------------------------------------------------
    PurgeElements(document.select_nodes(
      "//ServerManagerState/Proxy[@group='interactorstyles' or @group='cameramanipulators']"));

    //-------------------------------------------------------------------------
    // convert global property links
    //-------------------------------------------------------------------------
    pugi::xml_node smstate = document.root().child("ServerManagerState");
    pugi::xml_node links = smstate.child("Links");

    if (!links)
    {
      links = smstate.append_child("Links");
    }

    pugi::xpath_node_set global_property_links = document.select_nodes(
      "//ServerManagerState/GlobalPropertiesManagers/GlobalPropertiesManager/Link");
    for (pugi::xpath_node_set::const_iterator iter = global_property_links.begin();
         iter != global_property_links.end(); ++iter)
    {
      pugi::xml_node linkNode = iter->node();
      linkNode.set_name("GlobalPropertyLink");

      links.append_copy(linkNode);
    }

    //-------------------------------------------------------------------------
    return true;
  }

  static bool ConvertChartNodes(xml_document& document)
  {
    const char* axisProperties3[] = { "AxisColor", "AxisGridColor", "AxisLabelColor",
      "AxisTitleColor" };
    const char* rangeProperties1[] = { "LeftAxisRange", "BottomAxisRange", "RightAxisRange",
      "TopAxisRange" };
    const char* axisProperties4[] = { "AxisLabelFont", "AxisTitleFont" };
    const char* fontProperties1[] = { "ChartTitleFont",
      // next properties are generated from the
      // previous array axisProperties4[]
      "LeftAxisLabelFont", "BottomAxisLabelFont", "RightAxisLabelFont", "TopAxisLabelFont",
      "LeftAxisTitleFont", "BottomAxisTitleFont", "RightAxisTitleFont", "TopAxisTitleFont" };
    const char* axisProperties1[] = { "AxisLabelNotation", "AxisLabelPrecision", "AxisLogScale",
      "AxisTitle", "AxisUseCustomLabels", "AxisUseCustomRange", "ShowAxisGrid", "ShowAxisLabels" };
    return ConvertChartNode(document, axisProperties3,
             sizeof(axisProperties3) / sizeof(axisProperties3[0]), 3, 4, validOutputForXYChart,
             axisNodeName) &&
      ConvertChartNode(document, rangeProperties1,
             sizeof(rangeProperties1) / sizeof(rangeProperties1[0]), 1, 2, validOutputTrue,
             rangeNodeName) &&
      // the order of execution of the next two functions is important
      ConvertChartNode(document, axisProperties4,
             sizeof(axisProperties4) / sizeof(axisProperties4[0]), 4, 4, validOutputForXYChart,
             axisNodeName) &&
      ConvertChartNode(document, fontProperties1,
             sizeof(fontProperties1) / sizeof(fontProperties1[0]), 1, 4, validOutputTrue,
             fontNodeName) &&
      ConvertChartNode(document, axisProperties1,
             sizeof(axisProperties1) / sizeof(axisProperties1[0]), 1, 4, validOutputForXYChart,
             axisNodeName);
  }

  //---------------------------------------------------------------------------
  static bool ConvertChartNode(xml_document& document, const char* property[],
    int numberOfProperties, int numberOfComponents, int numberOfOutputs,
    bool (*validOutput)(xml_node viewNode, int outputIndex),
    string (*nodeName)(xml_node oldNode, int outputIndex))
  {
    if (numberOfProperties <= 0)
    {
      return true;
    }
    string query("//ServerManagerState/Proxy[@group='views']/"
                 "Property[@name='");
    query = query + property[0] + "'";
    ;
    for (int i = 1; i < numberOfProperties; ++i)
    {
      query = query + "or @name='" + property[i] + "'";
    }
    query += "]";
    xpath_node_set nodes = document.select_nodes(query.c_str());
    for (xpath_node_set::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
      xml_node oldNode = it->node();
      xml_node viewNode = oldNode.parent();
      for (int i = 0; i < numberOfOutputs; ++i)
      {
        if (validOutput(viewNode, i))
        {
          CreateChartPropertyNode(viewNode, oldNode, numberOfComponents, i, nodeName);
        }
      }
      viewNode.remove_child(oldNode);
    }
    return true;
  }

  static void CreateChartPropertyNode(xml_node& viewNode, const xml_node& oldNode,
    int numberOfComponents, int outputIndex, string (*nodeName)(xml_node oldNode, int outputIndex))
  {
    string newNameAttr = nodeName(oldNode, outputIndex);
    xml_node newNode = CreatePropertyNode(viewNode, oldNode, newNameAttr, numberOfComponents);
    for (int i = 0; i < numberOfComponents; ++i)
    {
      CreateChartElementNode(newNode, oldNode, numberOfComponents, outputIndex, i);
    }
  }

  static void CreateChartElementNode(xml_node& newNode, const xml_node& oldNode,
    int numberOfComponents, int outputIndex, int componentIndex)
  {
    xml_node oldElement =
      oldNode
        .select_single_node((string("Element[@index='") +
                              toString(outputIndex * numberOfComponents + componentIndex) + "']")
                              .c_str())
        .node();
    const char* value = oldElement.attribute(VALUE).value();
    CreateElementNode(newNode, componentIndex, value);
  }

  //---------------------------------------------------------------------------

  static string rangeNodeName(xml_node oldNode, int i)
  {
    return string(oldNode.attribute(NAME).value()) + rangeString(i);
  }

  static string axisNodeName(xml_node oldNode, int i)
  {
    return string(axisString(i)) + oldNode.attribute(NAME).value();
  }

  static string fontNodeName(xml_node oldNode, int i)
  {
    string oldNodeName = oldNode.attribute(NAME).value();
    switch (i)
    {
      case 0:
        return oldNodeName + "Family";
      case 1:
        return oldNodeName + "Size";
      case 2:
        return replaceFont(&oldNodeName, "Bold");
      case 3:
        return replaceFont(&oldNodeName, "Italic");
      default:
        return oldNodeName;
    }
  }

  static string replaceFont(string* oldNodeName, const char* modifier)
  {
    const char* FONT = "Font";
    const size_t FONT_LENGTH = 4;
    size_t pos = oldNodeName->find(FONT);
    if (pos != string::npos)
    {
      return oldNodeName->replace(pos, FONT_LENGTH, modifier);
    }
    else
      return *oldNodeName;
  }

  static const char* rangeString(int index)
  {
    switch (index)
    {
      case 0:
        return "Minimum";
      case 1:
        return "Maximum";
    }
    return "";
  }

  static const char* axisString(int axis)
  {
    switch (axis)
    {
      case 0: // case vtkAxis::LEFT:
        return "Left";
      case 1: // case vtkAxis::BOTTOM:
        return "Bottom";
      case 2: // case vtkAxis::RIGHT:
        return "Right";
      case 3: // case vtkAxis::TOP:
        return "Top";
    }
    return "";
  }

  static bool validOutputTrue(xml_node viewNode, int i)
  {
    (void)viewNode;
    (void)i;
    return true;
  }

  static bool validOutputForXYChart(xml_node viewNode, int i)
  {
    const char* const TYPE = "type";
    const char* const XY_CHART_VIEW = "XYChartView";
    if (i < 0 || i >= 4)
    {
      return false;
    }
    if (i < 2)
    {
      // left, bottom
      return true;
    }
    else if (string(viewNode.attribute(TYPE).value()) == string(XY_CHART_VIEW))
    {
      //
      return true;
    }
    else
    {
      return false;
    }
  }

  static void CreateElementNode(xml_node& node, int index, const string& value)
  {
    const char* const ELEMENT = "Element";
    const char* const INDEX = "index";
    xml_node newElement = node.append_child();
    newElement.set_name(ELEMENT);
    xml_attribute a = newElement.append_attribute(INDEX);
    a.set_value(index);
    a = newElement.append_attribute(VALUE);
    a.set_value(value.c_str());
  }

  static xml_node CreatePropertyNode(
    xml_node& viewNode, const xml_node& oldNode, const string& name, int numberOfElements)
  {
    const char* const PROPERTY = "Property";
    const char* const NUMBER_OF_ELEMENTS = "number_of_elements";
    const char* const ID = "id";
    xml_node newNode = viewNode.insert_child_before(node_element, oldNode);
    newNode.set_name(PROPERTY);
    xml_attribute a = newNode.append_attribute(NAME);
    a.set_value(name.c_str());
    string newNodeId = string(viewNode.attribute(ID).value()) + "." + name;
    a = newNode.append_attribute(ID);
    a.set_value(newNodeId.c_str());
    a = newNode.append_attribute(NUMBER_OF_ELEMENTS);
    a.set_value(numberOfElements);
    return newNode;
  }

  static bool ConvertGlyphFilter(xml_document& document)
  {
    bool warn = false;
    //-------------------------------------------------------------------------
    // Convert "Glyph" and "ArbitrarySourceGlyph" to "LegacyGlyph" and
    // "LegacyArbitrarySourceGlyph". We don't explicitly convert those filters to the
    // new ones, we just create the old proxies for now.
    //-------------------------------------------------------------------------
    pugi::xpath_node_set glyph_elements =
      document.select_nodes("//ServerManagerState/Proxy[@group='filters' and @type='Glyph']");
    for (pugi::xpath_node_set::const_iterator iter = glyph_elements.begin();
         iter != glyph_elements.end(); ++iter)
    {
      // It's possible that we are using a development version's state file in
      // which case we don't want to change it.
      if (iter->node().select_single_node("//Property[@name='GlyphMode']"))
      {
        continue;
      }
      iter->node().attribute("type").set_value("LegacyGlyph");
      warn = true;
    }
    glyph_elements = document.select_nodes(
      "//ServerManagerState/Proxy[@group='filters' and @type='ArbitrarySourceGlyph']");
    for (pugi::xpath_node_set::const_iterator iter = glyph_elements.begin();
         iter != glyph_elements.end(); ++iter)
    {
      iter->node().attribute("type").set_value("LegacyArbitrarySourceGlyph");
      warn = true;
    }
    if (warn)
    {
      vtkGenericWarningMacro(
        "The state file uses the old 'Glyph' filter implementation."
        "The implementation has changed considerably in ParaView 4.2. "
        "Consider replacing the Glyph filter with a new Glyph filter. The old implementation "
        "is still available as 'Legacy Glyph' and will be used for loading this state file.");
    }
    return true;
  }
};
const char* const Process_4_1_to_4_2::NAME = "name";
const char* const Process_4_1_to_4_2::VALUE = "value";

//===========================================================================
struct Process_4_2_to_5_1
{
  bool operator()(xml_document& document) { return RemoveCubeAxesColorLinks(document); }

  // Remove global property link state for "CubeAxesColor"
  bool RemoveCubeAxesColorLinks(xml_document& document)
  {
    pugi::xpath_node_set links =
      document.select_nodes("//GlobalPropertyLink[@property=\"CubeAxesColor\"]");
    PurgeElements(links);
    return true;
  }
};

//===========================================================================
struct Process_5_1_to_5_4
{
  bool operator()(xml_document& document) { return ScalarBarLengthToPosition2(document); }

  // Read scalar bar Position2 property and set it as the ScalarBarLength
  bool ScalarBarLengthToPosition2(xml_document& document)
  {
    pugi::xpath_node_set proxy_nodes =
      document.select_nodes("//ServerManagerState/Proxy[@group='representations' and "
                            "@type='ScalarBarWidgetRepresentation']");

    double position2[2];

    // Note: Each property is from a different ScalarBarWidgetRepresentation.
    for (pugi::xpath_node_set::const_iterator iter = proxy_nodes.begin(); iter != proxy_nodes.end();
         ++iter)
    {
      pugi::xml_node proxy_node = iter->node();
      std::string id_string(proxy_node.attribute("id").value());

      //--------------------------
      // Handle Position property
      // We don't change the Position property here as its purpose remains the
      // same (determining the lower left position of the scalar bar). However,
      // we do change the "Window Location" property from the default
      // "Lower Right" to "Any Location" so that the scalar bar will appear
      // approximately where it was. It is a new property, so we need to
      // add an XML node.
      pugi::xml_node location_node = proxy_node.append_child();
      location_node.set_name("Property");
      location_node.append_attribute("name").set_value("WindowLocation");
      location_node.append_attribute("id").set_value((id_string + ".WindowLocation").c_str());
      location_node.append_attribute("number_of_elements").set_value("1");
      pugi::xml_node element_node = location_node.append_child();
      element_node.set_name("Element");
      element_node.append_attribute("index").set_value("0");
      element_node.append_attribute("value").set_value("0");

      //--------------------------
      // Handle Position2 property
      pugi::xml_node pos2_node =
        proxy_node.find_child_by_attribute("Property", "name", "Position2");

      // Change the 'name' attribute
      pos2_node.attribute("name").set_value("ScalarBarLength");

      // Change the 'id' attribute to be safe.
      pos2_node.attribute("id").set_value((id_string + ".ScalarBarLength").c_str());

      // Should be just two Element nodes
      pugi::xml_node first_element = pos2_node.child("Element");
      position2[0] = first_element.attribute("value").as_double();
      position2[1] = first_element.next_sibling("Element").attribute("value").as_double();

      // Assume the length is the largest element and ensure its value is the
      // first Element
      double length = vtkMath::Max(position2[0], position2[1]);
      first_element.attribute("value").set_value(length);

      // Position2 had two elements, ScalarBarLength has 1, so delete the
      // second one.
      pos2_node.remove_child(first_element.next_sibling());

      // Fix up the 'id' attribute in the Domain node
      first_element.next_sibling().attribute("id").set_value(
        (id_string + ".ScalarBarLength").c_str());
    }

    return true;
  }
};

} // end of namespace

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
  std::ostringstream stream;
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
    status = Process_4_1_to_4_2()(document);
    version = vtkSMVersion(4, 2, 0);
  }

  if (status && (version < vtkSMVersion(5, 1, 0)))
  {
    status = Process_4_2_to_5_1()(document);
    version = vtkSMVersion(5, 1, 0);
  }

  if (status && (version < vtkSMVersion(5, 4, 0)))
  {
    status = Process_5_1_to_5_4()(document);
    version = vtkSMVersion(5, 4, 0);
  }

  if (status)
  {
    std::ostringstream stream2;
    document.save(stream2, "  ");

    vtkNew<vtkPVXMLParser> parser;
    if (parser->Parse(stream2.str().c_str()))
    {
      root->RemoveAllNestedElements();
      vtkPVXMLElement* newRoot = parser->GetRootElement();

      newRoot->CopyAttributesTo(root);
      for (unsigned int cc = 0, max = newRoot->GetNumberOfNestedElements(); cc < max; cc++)
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
