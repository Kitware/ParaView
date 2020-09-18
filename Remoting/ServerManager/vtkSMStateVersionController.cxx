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
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkWeakPointer.h"

#include <algorithm>
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
// A helper class to keep track of "id"s used in the state file and help
// generate new unique ones.
class UniqueIdGenerator
{
  vtkTypeUInt32 LastUniqueId = 1000; // let's just start at some high number.
public:
  UniqueIdGenerator(const xml_document& document)
  {
    auto xnodes = document.select_nodes("//Proxy[@id]");
    for (const auto& xnode : xnodes)
    {
      this->LastUniqueId = std::max(
        this->LastUniqueId, static_cast<vtkTypeUInt32>(xnode.node().attribute("id").as_uint()));
    }
  }

  vtkTypeUInt32 GetNextUniqueId() { return (++this->LastUniqueId); }
};

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
void PurgeElement(const pugi::xml_node& node)
{
  node.parent().remove_child(node);
}

void PurgeElements(const pugi::xpath_node_set& elements_of_interest)
{
  for (pugi::xpath_node_set::const_iterator iter = elements_of_interest.begin();
       iter != elements_of_interest.end(); ++iter)
  {
    pugi::xml_node node = iter->node();
    PurgeElement(node);
  }
}

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

//===========================================================================
struct Process_5_4_to_5_5
{
  bool operator()(xml_document& document)
  {
    return LockScalarRange(document) && CalculatorAttributeMode(document) &&
      CGNSReaderUpdates(document) && HeadlightToAdditionalLight(document) &&
      DataBoundsInflateScaleFactor(document) && AnnotateAttributesInput(document) &&
      ClipInvert(document);
  }

  bool LockScalarRange(xml_document& document)
  {
    pugi::xpath_node_set proxy_nodes =
      document.select_nodes("//ServerManagerState/Proxy[@group='lookup_tables' and "
                            "@type='PVLookupTable']");

    for (pugi::xpath_node_set::const_iterator iter = proxy_nodes.begin(); iter != proxy_nodes.end();
         ++iter)
    {
      pugi::xml_node proxy_node = iter->node();
      std::string id_string(proxy_node.attribute("id").value());

      pugi::xml_node lock_scalar_range_node =
        proxy_node.find_child_by_attribute("Property", "name", "LockScalarRange");

      pugi::xml_node element = lock_scalar_range_node.child("Element");
      int lock_scalar_range = element.attribute("value").as_int();

      lock_scalar_range_node.attribute("name").set_value("AutomaticRescaleRangeMode");
      lock_scalar_range_node.attribute("id").set_value(
        (id_string + ".AutomaticRescaleRangeMode").c_str());
      if (lock_scalar_range)
      {
        element.attribute("value").set_value("-1");
      }
      else
      {
        if (this->Session)
        {
          vtkSMSessionProxyManager* pxm = this->Session->GetSessionProxyManager();
          vtkSMProxy* settingsProxy = pxm->GetProxy("settings", "GeneralSettings");
          int globalResetMode =
            vtkSMPropertyHelper(settingsProxy, "TransferFunctionResetMode").GetAsInt();
          element.attribute("value").set_value(toString(globalResetMode).c_str());
        }
        else
        {
          vtkGenericWarningMacro("Could not get TransferFunctionResetMode from settings.");
        }
      }
    }

    return true;
  }

  bool AnnotateAttributesInput(xml_document& document)
  {
    pugi::xpath_node_set proxy_nodes =
      document.select_nodes("//ServerManagerState/Proxy[@group='filters' and "
                            "@type='AnnotateAttributeData']");

    for (auto iter = proxy_nodes.begin(); iter != proxy_nodes.end(); ++iter)
    {
      pugi::xml_node proxy_node = iter->node();
      pugi::xml_node association_node =
        proxy_node.find_child_by_attribute("Property", "name", "ArrayAssociation");
      pugi::xml_node arrayname_node =
        proxy_node.find_child_by_attribute("Property", "name", "ArrayName");
      if (!association_node || !arrayname_node)
      {
        continue;
      }

      pugi::xml_node newInputNode = proxy_node.append_child("Property");
      newInputNode.append_attribute("name").set_value("SelectInputArray");
      newInputNode.append_attribute("number_of_elements").set_value(5);

      pugi::xml_node childNode = newInputNode.append_child("Element");
      childNode.append_attribute("index").set_value(0);
      childNode.append_attribute("value").set_value("");

      childNode = newInputNode.append_child("Element");
      childNode.append_attribute("index").set_value(1);
      childNode.append_attribute("value").set_value("");

      childNode = newInputNode.append_child("Element");
      childNode.append_attribute("index").set_value(2);
      childNode.append_attribute("value").set_value("");

      childNode = newInputNode.append_child("Element");
      childNode.append_attribute("index").set_value(3);
      childNode.append_attribute("value").set_value(
        association_node.child("Element").attribute("value").as_int());

      childNode = newInputNode.append_child("Element");
      childNode.append_attribute("index").set_value(4);
      childNode.append_attribute("value").set_value(
        arrayname_node.child("Element").attribute("value").as_string());
    }
    return true;
  }

  bool CalculatorAttributeMode(xml_document& document)
  {
    pugi::xpath_node_set proxy_nodes =
      document.select_nodes("//ServerManagerState/Proxy[@group='filters' and "
                            "@type='Calculator']/Property[@name='AttributeMode']");

    for (auto iter = proxy_nodes.begin(); iter != proxy_nodes.end(); ++iter)
    {
      pugi::xml_node attribute_mode = iter->node();

      pugi::xml_node element = attribute_mode.child("Element");
      int attribute_mode_value = element.attribute("value").as_int();

      attribute_mode.attribute("name").set_value("AttributeType");
      element.attribute("value").set_value(attribute_mode_value - 1);

      attribute_mode.remove_child("Domain");
    }
    return true;
  }

  /**
   * Handle changes to properties on `CGNSSeriesReader`.
   * 1. `BaseStatus`, `FamilyStatus`, `LoadMesh`, and `LoadBndPatch` properties have been removed.
   * 2. a new `Blocks` property takes in block selection instead
   */
  bool CGNSReaderUpdates(xml_document& document)
  {
    pugi::xpath_node_set proxy_nodes = document.select_nodes(
      "//ServerManagerState/Proxy[@group='sources' and @type='CGNSSeriesReader']");
    for (auto xnode : proxy_nodes)
    {
      auto proxyNode = xnode.node();
      if (!proxyNode.select_nodes("//Property[@name='Blocks']").empty())
      {
        // state is already newer.
        continue;
      }

      bool loadMesh = true;
      if (!proxyNode.select_nodes("//Property[@name='LoadMesh']/Element[@index='0' and value='0']")
             .empty())
      {
        loadMesh = false;
      }

      bool loadBndPatch = false;
      if (!proxyNode
             .select_nodes("//Property[@name='LoadBndPatch']/Element[@index='0' and value='1']")
             .empty())
      {
        loadBndPatch = true;
      }

      // now collect names for selected bases and families.
      std::set<std::string> selectedPaths;
      auto xprops_set =
        proxyNode.select_nodes("//Property[@name='BaseStatus']/Element[@value='1']");
      for (auto xpropnode : xprops_set)
      {
        std::string baseName = xpropnode.node().previous_sibling().attribute("value").value();
        if (loadMesh)
        {
          std::ostringstream stream;
          stream << "/Grids/" << baseName.c_str();
          selectedPaths.insert(stream.str());
        }
        if (loadBndPatch)
        {
          std::ostringstream stream;
          stream << "/Patches/" << baseName.c_str();
          selectedPaths.insert(stream.str());
        }
      }

      xprops_set = proxyNode.select_nodes("//Property[@name='FamilyStatus']/Element[@value='1']");
      for (auto xpropnode : xprops_set)
      {
        std::string familyName = xpropnode.node().previous_sibling().attribute("value").value();
        std::ostringstream stream;
        stream << "/Families/" << familyName.c_str();
        selectedPaths.insert(stream.str());
      }

      auto blocksNode = proxyNode.append_child("Property");
      blocksNode.append_attribute("name").set_value("Blocks");
      blocksNode.append_attribute("number_of_elements")
        .set_value(static_cast<int>(selectedPaths.size() * 2));
      int index = 0;
      for (const auto spath : selectedPaths)
      {
        auto eNode = blocksNode.append_child("Element");
        eNode.append_attribute("index").set_value(index++);
        eNode.append_attribute("value").set_value(spath.c_str());

        eNode = blocksNode.append_child("Element");
        eNode.append_attribute("index").set_value(index++);
        eNode.append_attribute("value").set_value(1);
      }
    }
    return true;
  }

  /** Translate the fixed single headlight into a configurable light with the same properties
  */
  bool HeadlightToAdditionalLight(xml_document& document)
  {
    UniqueIdGenerator generator(document);

    pugi::xpath_node_set proxy_nodes =
      document.select_nodes("//ServerManagerState/Proxy[@group='views' and @type='RenderView']");

    pugi::xml_node smstate = document.root().child("ServerManagerState");

    for (auto xnode : proxy_nodes)
    {
      auto proxyNode = xnode.node();
      std::string id_string(proxyNode.attribute("id").value());
      // Don't check for LightSwitch - it seems to find the new light's LightSwitch
      // as well as the old one in RenderView.
      if (proxyNode.select_nodes("//Property[@name='LightDiffuseColor']").empty())
      {
        // state is already newer.
        continue;
      }
      // If the property LightSwitch is on, we add a light
      auto switchNode = proxyNode.select_node("//Property[@name='LightSwitch']");
      if (switchNode.node().child("Element").attribute("value").as_int() == 1)
      {
        // add a proxy group to the SM, with one headlight, with intensity and color set
        // get the old color and intensity. Diffuse color was set by the UI.
        double diffuseR = 1, diffuseG = 1, diffuseB = 1;
        double intensity = 1;
        auto colorNode = proxyNode.select_node("//Property[@name='LightDiffuseColor']").node();
        if (colorNode)
        {
          auto colorElt = colorNode.child("Element");
          diffuseR = colorElt.attribute("value").as_double();
          colorElt = colorElt.next_sibling("Element");
          diffuseG = colorElt.attribute("value").as_double();
          colorElt = colorElt.next_sibling("Element");
          diffuseB = colorElt.attribute("value").as_double();
        }
        auto intensityNode = proxyNode.select_node("//Property[@name='LightIntensity']").node();
        if (intensityNode)
        {
          auto intensityElt = intensityNode.child("Element");
          intensity = intensityElt.attribute("value").as_double();
        }
        // Here's the full structure, we will ignore properties with default values, and domains.
        // <Proxy group="additional_lights" type="Light" id="8232" servers="21">
        //   <Property name="DiffuseColor" id="8232.DiffuseColor" number_of_elements="3">
        //     <Element index="0" value="1"/>
        //     <Element index="1" value="1"/>
        //     <Element index="2" value="1"/>
        //     <Domain name="range" id="8232.DiffuseColor.range"/>
        //   </Property>
        //   <Property name="LightIntensity" id="8232.LightIntensity" number_of_elements="1">
        //     <Element index="0" value="1"/>
        //     <Domain name="range" id="8232.LightIntensity.range"/>
        //   </Property>
        //   <Property name="LightSwitch" id="8232.LightSwitch" number_of_elements="1">
        //     <Element index="0" value="1"/>
        //     <Domain name="bool" id="8232.LightSwitch.bool"/>
        //   </Property>
        //   <Property name="LightType" id="8232.LightType" number_of_elements="1">
        //     <Element index="0" value="1"/>
        //     <Domain name="enum" id="8232.LightType.enum">
        //       <Entry value="1" text="Headlight"/>
        //       <Entry value="2" text="Camera"/>
        //       <Entry value="3" text="Scene"/>
        //       <Entry value="4" text="Ambient"/>
        //     </Domain>
        //   </Property>
        // </Proxy>
        vtkTypeUInt32 proxyId = generator.GetNextUniqueId();
        std::ostringstream stream;
        stream << "<Proxy group=\"additional_lights\" type=\"Light\" id=\"" << proxyId
               << "\" servers=\"21\" >\n";
        stream << "  <Property name=\"DiffuseColor\" id=\"" << proxyId
               << ".DiffuseColor\" number_of_elements=\"3\" >\n";
        stream << "    <Element index=\"" << 0 << "\" value=\"" << diffuseR << "\"/>\n";
        stream << "    <Element index=\"" << 1 << "\" value=\"" << diffuseG << "\"/>\n";
        stream << "    <Element index=\"" << 2 << "\" value=\"" << diffuseB << "\"/>\n";
        stream << "  </Property>\n";
        stream << "  <Property name=\"LightIntensity\" id=\"" << proxyId
               << ".LightIntensity\" number_of_elements=\"1\" >\n";
        stream << "    <Element index=\"" << 0 << "\" value=\"" << intensity << "\"/>\n";
        stream << "  </Property>\n";
        stream << "  <Property name=\"LightType\" id=\"" << proxyId
               << ".LightType\" number_of_elements=\"1\" >\n";
        stream << "    <Element index=\"" << 0 << "\" value=\"1\"/>\n";
        stream << "  </Property>\n";
        stream << "</Proxy>\n";
        // and a proxy collection to the SMS
        // <ProxyCollection name="additional_lights">
        //   <Item id="8232" name="Light1"/>
        // </ProxyCollection>
        stream << "<ProxyCollection name=\"additional_lights\" >\n";
        stream << "  <Item id=\"" << proxyId << "\" name=\"Light1\"/>\n";
        stream << "</ProxyCollection>\n";

        std::string buffer = stream.str();
        if (!smstate.append_buffer(buffer.c_str(), buffer.size()))
        {
          // vtkWarningMacro("Unable to add new light to match deprecated headlight. "
          //   "Add a light manually in the Lights Inspector.");
        }

        // add a proxy property to this view for the light
        // <Property name="AdditionalLights" id="8135.AdditionalLights" number_of_elements="1">
        //   <Proxy value="8232"/>
        // </Property>
        auto additionalLightsNode =
          proxyNode.select_node("//Property[@name='AdditionalLights']").node();
        if (!additionalLightsNode)
        {
          additionalLightsNode = proxyNode.append_child("Property");
          additionalLightsNode.append_attribute("name").set_value("AdditionalLights");
          std::ostringstream stream2;
          stream2 << id_string << ".AdditionalLights";
          additionalLightsNode.append_attribute("id").set_value(stream2.str().c_str());
        }
        additionalLightsNode.append_attribute("number_of_elements").set_value(1);
        additionalLightsNode.append_child("Proxy").append_attribute("value").set_value(proxyId);
      }
      // Remove the old fixed-light properties:
      // LightDiffuseColor, LightAmbientColor, LightSpecularColor,
      // LightIntensity, LightSwitch, LightType
      PurgeElements(proxyNode.select_nodes("//Property[@name='LightDiffuseColor' "
                                           "or @name='LightAmbientColor' "
                                           "or @name='LightSpecularColor' "
                                           "or @name='LightIntensity' "
                                           "or @name='LightSwitch' "
                                           "or @name='LightType']"));
    }
    return true;
  }

  bool DataBoundsInflateScaleFactor(xml_document& document)
  {
    pugi::xpath_node_set proxy_nodes =
      document.select_nodes("//ServerManagerState/Proxy[@group='annotations' and "
                            "@type='GridAxes3DActor']");

    for (auto xnode : proxy_nodes)
    {
      auto proxyNode = xnode.node();
      if (proxyNode.select_nodes("//Property[@name='DataBoundsInflateFactor']").empty())
      {
        // state is already newer.
        continue;
      }

      auto prop = proxyNode.select_node("//Property[@name='DataBoundsInflateFactor']").node();
      auto valueElt = prop.child("Element");
      double inflateFactor = valueElt.attribute("value").as_double();

      prop.attribute("name").set_value("DataBoundsScaleFactor");
      valueElt.attribute("value").set_value(inflateFactor + 1);
      prop.remove_child("Domain");
    }
    return true;
  }

  bool ClipInvert(xml_document& document)
  {
    pugi::xpath_node_set proxy_nodes =
      document.select_nodes("//ServerManagerState/Proxy[@group='filters' and "
                            "@type='Clip']");
    for (auto xnode : proxy_nodes)
    {
      auto proxyNode = xnode.node();
      if (proxyNode.select_nodes("//Property[@name='InsideOut']").empty())
      {
        // state is already newer.
        continue;
      }

      auto prop = proxyNode.select_node("//Property[@name='InsideOut']").node();
      auto valueElt = prop.child("Element");
      int invert = valueElt.attribute("value").as_int();

      prop.attribute("name").set_value("Invert");
      valueElt.attribute("value").set_value(invert);
      prop.remove_child("Domain");
    }
    return true;
  }

  vtkWeakPointer<vtkSMSession> Session;
};

//===========================================================================
struct Process_5_5_to_5_6
{
  bool operator()(xml_document& document) { return ConvertGlyphFilter(document); }

  static bool ConvertGlyphFilter(xml_document& document)
  {
    bool warn = false;
    //-------------------------------------------------------------------------
    // Convert "Glyph" to "GlyphLegacy" and "GlyphWithCustomSource" to
    // "GlyphWithCustomSourceLegacy". We don't explicitly convert those filters to the
    // new ones, we just create the old proxies for now.
    //-------------------------------------------------------------------------
    pugi::xpath_node_set glyph_elements =
      document.select_nodes("//ServerManagerState/Proxy[@group='filters' and @type='Glyph']");
    for (pugi::xpath_node_set::const_iterator iter = glyph_elements.begin();
         iter != glyph_elements.end(); ++iter)
    {
      iter->node().attribute("type").set_value("GlyphLegacy");
      warn = true;
    }
    glyph_elements = document.select_nodes(
      "//ServerManagerState/Proxy[@group='filters' and @type='GlyphWithCustomSource']");
    for (pugi::xpath_node_set::const_iterator iter = glyph_elements.begin();
         iter != glyph_elements.end(); ++iter)
    {
      iter->node().attribute("type").set_value("GlyphWithCustomSourceLegacy");
      warn = true;
    }
    if (warn)
    {
      vtkGenericWarningMacro(
        "The state file uses the old 'Glyph' filter implementation. "
        "The implementation has changed in ParaView 5.6. "
        "Consider replacing the Glyph filter with a new Glyph filter. The old implementation "
        "is still available as 'Glyph Legacy' and will be used for loading this state file.");
    }
    return true;
  }
};

//===========================================================================
struct Process_5_6_to_5_7
{
  bool operator()(xml_document& document)
  {
    return ConvertResampleWithDataset(document) && ConvertIdsFilter(document) &&
      ConvertOSPRayNames(document) && ConvertBox(document) &&
      ConvertExodusLegacyBlockNamesWithElementTypes(document) && RemoveColorPropertyLinks(document);
  }

  static bool ConvertResampleWithDataset(xml_document& document)
  {
    // Change InputProperty names for the inputs to be a bit clearer.
    pugi::xpath_node_set elements = document.select_nodes(
      "//ServerManagerState/Proxy[@group='filters' and @type='ResampleWithDataset']");
    for (auto iter = elements.begin(); iter != elements.end(); ++iter)
    {
      pugi::xml_node proxy_node = iter->node();
      // Rename the Input property and change the name attribute.
      pugi::xml_node input_node = proxy_node.find_child_by_attribute("Property", "name", "Input");
      input_node.attribute("name").set_value("SourceDataArrays");

      // Rename the Source property and change the name attribute.
      pugi::xml_node source_node = proxy_node.find_child_by_attribute("Property", "name", "Source");
      source_node.attribute("name").set_value("DestinationMesh");
    }

    return true;
  }

  static bool ConvertIdsFilter(xml_document& document)
  {
    // `ArrayName` property was removed, instead replaced by `CellIdsArrayName`
    // and `PointIdsArrayName`.
    pugi::xpath_node_set elements =
      document.select_nodes("//ServerManagerState/Proxy[@group='filters' and "
                            "@type='GenerateIdScalars']/Property[@name='ArrayName']");
    for (auto iter = elements.begin(); iter != elements.end(); ++iter)
    {
      auto arrayname_node = iter->node();
      auto proxy_node = arrayname_node.parent();
      std::string id = proxy_node.attribute("id").value();

      arrayname_node.attribute("name").set_value("CellIdsArrayName");
      arrayname_node.attribute("id").set_value((id + ".CellIdsArrayName").c_str());
      proxy_node.append_copy(arrayname_node);

      arrayname_node.attribute("name").set_value("PointIdsArrayName");
      arrayname_node.attribute("id").set_value((id + ".PointIdsArrayName").c_str());
    }

    return true;
  };

  static bool ConvertOSPRayNames(xml_document& document)
  {
    // The `EnableOSPray` property for the first time got a label, which is `Enable Ray Tracing`.
    // Other OSPRay label changes appear to have no effect on the state file.
    pugi::xpath_node_set elements =
      document.select_nodes("//ServerManagerState/Proxy[@group='views' "
                            "and @type='RenderView']/Property[@name='EnableOSPRay']");
    for (auto iter = elements.begin(); iter != elements.end(); ++iter)
    {
      auto property_node = iter->node();
      auto proxy_node = property_node.parent();

      property_node.attribute("name").set_value("EnableRayTracing");
      proxy_node.append_copy(property_node);
    }

    return true;
  };

  static bool ConvertBox(xml_document& document)
  {
    pugi::xpath_node_set elements = document.select_nodes(
      "//ServerManagerState/Proxy[@group='implicit_functions' and @type='Box']");
    for (auto iter = elements.begin(); iter != elements.end(); ++iter)
    {
      pugi::xml_node proxy_node = iter->node();
      std::string id_string(proxy_node.attribute("id").value());

      // add a `UseReferenceBounds=1` property to each one.
      auto node = proxy_node.append_child("Property");
      node.append_attribute("name").set_value("UseReferenceBounds");
      node.append_attribute("id").set_value((id_string + ".UseReferenceBounds").c_str());
      node.append_attribute("number_of_elements").set_value("1");

      auto elem = node.append_child("Element");
      elem.append_attribute("index").set_value("0");
      elem.append_attribute("value").set_value("1");

      // find 'Scale' property and rename it to 'Length'
      for (auto pchild = proxy_node.child("Property"); pchild;
           pchild = pchild.next_sibling("Property"))
      {
        if (strcmp(pchild.attribute("name").as_string(), "Scale") == 0)
        {
          pchild.attribute("name").set_value("Length");
        }
      }
    }

    return true;
  }

  static bool ConvertExodusLegacyBlockNamesWithElementTypes(xml_document& document)
  {
    pugi::xpath_node_set elements =
      document.select_nodes("//ServerManagerState/Proxy[@group='sources' and "
                            "(@type='ExodusIIReader' or @type='ExodusRestartReader')]");
    for (auto iter = elements.begin(); iter != elements.end(); ++iter)
    {
      pugi::xml_node proxy_node = iter->node();
      std::string id_string(proxy_node.attribute("id").value());

      // add a `UseLegacyBlockNamesWithElementTypes=1` property to each one.
      auto node = proxy_node.append_child("Property");
      node.append_attribute("name").set_value("UseLegacyBlockNamesWithElementTypes");
      node.append_attribute("id").set_value(
        (id_string + ".UseLegacyBlockNamesWithElementTypes").c_str());
      node.append_attribute("number_of_elements").set_value("1");

      auto elem = node.append_child("Element");
      elem.append_attribute("index").set_value("0");
      elem.append_attribute("value").set_value("1");
    }

    return true;
  }

  static bool RemoveColorPropertyLinks(xml_document& document)
  {
    pugi::xpath_node_set elements =
      document.select_nodes("//ServerManagerState/Links/GlobalPropertyLink"
                            "[@property='Color']");
    PurgeElements(elements);

    return true;
  }
};

//===========================================================================
struct Process_5_7_to_5_8
{
  bool operator()(xml_document& document) { return ConvertGlobalPropertyLinks(document); }

  /*
   * With 5.8, we no longer save / load "GlobalPropertyLink" which were used to
   * save links to the global ColorPalette proxy. Update that to the new way
   * this is handled.
   */
  static bool ConvertGlobalPropertyLinks(xml_document& document)
  {
    pugi::xpath_node_set elements =
      document.select_nodes("//ServerManagerState/Links/GlobalPropertyLink");
    if (elements.size() == 0)
    {
      return true;
    }

    auto node = document.select_node("//ServerManagerState").node();
    auto settingsNode = node.append_child("Settings");
    auto colorPaletteNode = settingsNode.append_child("SettingsProxy");
    colorPaletteNode.append_attribute("group").set_value("settings");
    colorPaletteNode.append_attribute("type").set_value("ColorPalette");
    auto linksNode = colorPaletteNode.append_child("Links");
    for (auto xnode : elements)
    {
      auto gpnode = xnode.node();
      auto pnode = linksNode.append_child("Property");
      pnode.append_attribute("source_property")
        .set_value(gpnode.attribute("global_name").as_string());
      pnode.append_attribute("target_id").set_value(gpnode.attribute("proxy").as_string());
      pnode.append_attribute("target_property").set_value(gpnode.attribute("property").as_string());
      pnode.append_attribute("unlink_if_modified").set_value(1);
    }

    ::PurgeElements(elements);
    return true;
  }
};

//===========================================================================
struct Process_5_8_to_5_9
{
  bool operator()(xml_document& document) { return WarnCGNSReader(document); }

  static bool WarnCGNSReader(xml_document& document)
  {
    pugi::xpath_node_set elements =
      document.select_nodes("//ServerManagerState/Proxy[@group='sources' and "
                            "(@type='CGNSSeriesReader')]");
    if (elements.size() == 0)
    {
      return true;
    }

    // Unfortunately, there's no easy way to convert obsolete SIL-based
    // selection to base/family selection, so we simply warn.
    vtkGenericWarningMacro(
      "Your state file uses CGNS reader. The selection mechanism "
      "in the reader was changed in ParaView 5.9 and is not compatible with older "
      "states. As a result, the visualization may be different than with earlier versions. "
      "You may have to manually adjust properties on the the CGNS reader after loading state.");
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
bool vtkSMStateVersionController::Process(vtkPVXMLElement* parent, vtkSMSession* session)
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
  if (version < vtkSMVersion(4, 2, 0))
  {
    vtkWarningMacro(
      "State file version is less than 4.2.0. "
      "We will try to load the state file. It's recommended, however, "
      "that you load the state in ParaView 4.2.0 (up to 5.5.2) and save a newer version "
      "so that it can be loaded more faithfully. "
      "Loading state files generated from ParaView versions older than 4.2.0 "
      "is no longer supported.");

    version = vtkSMVersion(4, 2, 0);
  }

  // A little hackish for now, convert vtkPVXMLElement to string.
  std::ostringstream stream;
  root->PrintXML(stream, vtkIndent());

  // parse using pugi.
  pugi::xml_document document;

  if (!document.load_string(stream.str().c_str()))
  {
    vtkErrorMacro("Failed to convert from vtkPVXMLElement to pugi::xml_document");
    return false;
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

  if (status && (version < vtkSMVersion(5, 5, 0)))
  {
    Process_5_4_to_5_5 converter;
    converter.Session = session;
    status = converter(document);
    version = vtkSMVersion(5, 5, 0);
  }

  if (status && (version < vtkSMVersion(5, 6, 0)))
  {
    Process_5_5_to_5_6 converter;
    status = converter(document);
    version = vtkSMVersion(5, 6, 0);
  }

  if (status && (version < vtkSMVersion(5, 7, 0)))
  {
    Process_5_6_to_5_7 converter;
    status = converter(document);
    version = vtkSMVersion(5, 7, 0);
  }

  if (status && (version < vtkSMVersion(5, 8, 0)))
  {
    Process_5_7_to_5_8 converter;
    status = converter(document);
    version = vtkSMVersion(5, 8, 0);
  }

  if (status && (version < vtkSMVersion(5, 9, 0)))
  {
    Process_5_8_to_5_9 converter;
    status = converter(document);
    version = vtkSMVersion(5, 9, 0);
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
