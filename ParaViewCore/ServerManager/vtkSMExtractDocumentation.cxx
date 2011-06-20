#include "vtkDataSetAttributes.h"
#include "vtkInitializationHelper.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMBoundsDomain.h"
#include "vtkSMDataTypeDomain.h"
#include "vtkSMDocumentation.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMNumberOfGroupsDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringVectorProperty.h"
#include <vtkstd/list>
#include <vtkstd/string>
#include <vtkstd/map>
#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>
#include <vtksys/RegularExpression.hxx>


// For now let the template be here itself. We may want to read these from
// external files in future.

// Template for the <head/> with CSS styles for the Proxy documentation.
static const char* ProxyDocumentHeadTemplate =
  "<head>\n"\
  "  <title>%TITLE%</title>\n"\
  "  <link rel=\"stylesheet\" type=\"text/css\" href=\"ParaViewDoc.css\"/>"\
  "</head>\n";

// Template for writing the documentation associated with a proxy.
static const char* ProxyDocumentationTemplate = 
  "<div class=\"ProxyDocumentation\">\n"\
    "    <div class=\"ProxyHeading\"\n"\
    "      title=\"%SHORTHELP%\" >\n"\
    "      %LABEL% <span class=\"ProxyHeadingSmallText\">(%NAME%)</span>\n"\
    "    </div>\n"\
    "    <div class=\"ProxyLongHelp\">\n"\
    "      %LONGHELP% \n"\
    "    </div>\n"\
    "    <div class=\"ProxyDescription\">\n"\
    "    %DESCRIPTION%\n"\
    "    </div>\n"\
    "  <!-- End of Proxy Documentation -->\n"\
    "</div>\n";

// Template for header before listing properties for the proxy.
static const char* PropertiesTableHeaderTemplate =
  "<table class=\"PropertiesTable\" border=\"1\" cellpadding=\"5\" >\n"\
    "  <tr class=\"PropertiesTableHeading\">\n"\
    "    <td><b>Property</b></td><td><b>Description</b></td><td><b>Default Value(s)</b></td><td><b>Restrictions</b></td>\n"\
    "  </tr>\n";

// Template for the footer after having finished with listing all properties of the proxy.
static const char* PropertiesTableFooterTemplate = "</table>\n";

// Template for every property.
static const char* PropertyTemplate =
  "<tr>\n"\
    "  <td><b>%LABEL%</b><br/><i>(%NAME%)</i></td>\n"\
    "  <td>%DESCRIPTION%</td>\n"\
    "  <td>%DEFAULTVALUES%</td>\n"\
    "  <td>%DOMAINS%</td>\n"\
    "</tr>\n";

// Template for header before lising proxies.
static const char* ProxyListTitleTemplate =
  "<div class=\"ProxyDocumentation\">\n"\
    "  <div class=\"ProxyHeading\">\n"\
    "    %TITLE%\n"\
    "  </div>\n"\
    "</div>\n";

static const char* ProxyListTableHeaderTemplate = 
  "<table class=\"PropertiesTable\" border=\"1\" cellpadding=\"5\">\n"\
    "  <tr><td><b>Name</b></td><td><b>Description</b></td></tr>\n";
static const char* ProxyListTableFooterTemplate = "</table>\n";

static const char* ProxyListItemTemplate = 
  "<tr><td><a href=\"%LINK%\">%LABEL%</a></td><td>%DESCRIPTION%</td></tr>\n";
  

class vtkStringPairList : public vtkstd::list<vtkstd::pair<vtkstd::string, vtkstd::string> > {};
typedef vtkstd::list<vtkstd::pair<vtkstd::string, vtkstd::string> >::iterator vtkStringPairListIterator;

bool operator < (const vtkstd::pair<vtkstd::string, vtkstd::string> &x,
                 const vtkstd::pair<vtkstd::string, vtkstd::string> &y)
{
  return x.first < y.first;
}

typedef vtkstd::map<vtkstd::string, vtkstd::string> TemplateMap;

void WriteFromTemplate(ostream& docFile, const char* cdoc_template, TemplateMap& data)
{
  vtkstd::string doc_template = cdoc_template;
  TemplateMap::iterator iter = data.begin();
  for (;iter != data.end(); iter++)
    {
    vtkstd::string key = iter->first;
    vtkstd::string value = iter->second;
    key = "%" + key + "%";

    // Replace every occurance of key in the doc_template with the data.
    vtksys::SystemTools::ReplaceString(doc_template, key.c_str(), value.c_str());
    }

  // Dump doc_template (which now has all keys replaces with nice data values) 
  // into the file.
  docFile << doc_template.c_str();
}

bool WriteDefaultValues(vtkSMProperty *prop, ostream& docFile)
{
  bool written = false;
  vtkSMIntVectorProperty *intVecProp =
    vtkSMIntVectorProperty::SafeDownCast(prop);
  vtkSMDoubleVectorProperty *doubleVecProp =
    vtkSMDoubleVectorProperty::SafeDownCast(prop);
  vtkSMStringVectorProperty *stringVecProp =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  unsigned int i;
  if (intVecProp)
    {
    int *elements = intVecProp->GetElements();
    if (intVecProp->GetNumberOfElements() && elements)
      {
      for (i = 0; i < intVecProp->GetNumberOfElements(); i++)
        {
        docFile << elements[i] << " ";
        written = true;
        }
      }
    }
  else if (doubleVecProp)
    {
    double *elements = doubleVecProp->GetElements();
    if (doubleVecProp->GetNumberOfElements() && elements)
      {
      for (i = 0; i < doubleVecProp->GetNumberOfElements(); i++)
        {
        docFile << elements[i] << " ";
        written = true;
        }
      }
    }
  else if (stringVecProp)
    {
    if (stringVecProp->GetNumberOfElements() &&
        strlen(stringVecProp->GetElement(0)) > 0)
      {
      for (i = 0; i < stringVecProp->GetNumberOfElements(); i++)
        {
        docFile << stringVecProp->GetElement(i);
        written = true;
        }
      }
    }
  return written;
}

vtkstd::string FilterDescription(vtkstd::string description)
{
  // Remove leading/trailing spaces.
  vtksys::RegularExpression regExp = "^[ \n\r\t]*(.+)[ \n\r\t]*$";
  if (regExp.find(description))
    {
    description = regExp.match(1);
    }
  vtksys::SystemTools::ReplaceString(description,"\n\n","<br/><br/>\n");
  vtksys::SystemTools::ReplaceString(description,"\n","<br/>\n");
  return description;
}

bool WriteDomain(vtkSMDomain *dom, ostream &docFile)
{
  bool domainWritten = false;
  const char* className = dom->GetClassName();
  unsigned int i;
  int minExists, maxExists;

  if (!strcmp("vtkSMArrayListDomain", className))
    {
    vtkSMArrayListDomain *ald = vtkSMArrayListDomain::SafeDownCast(dom);
    switch (ald->GetAttributeType())
      {
      case vtkDataSetAttributes::SCALARS:
        domainWritten = true;
        docFile << "An array of scalars is required." << endl;
        break;
      case vtkDataSetAttributes::VECTORS:
        domainWritten = true;
        docFile << "An array of vectors is required." << endl;
        break;
      }
    }
  else if (!strcmp("vtkSMArrayRangeDomain", className))
    {
    domainWritten = true;
    docFile << "The value must lie within the range of the selected data "
            << "array." << endl;
    }
  else if (!strcmp("vtkSMArraySelectionDomain", className))
    {
    domainWritten = true;
    docFile << "The list of array names is provided by the reader." << endl;
    }
  else if (!strcmp("vtkSMBooleanDomain", className))
    {
    domainWritten = true;
    docFile << "Only the values 0 and 1 are accepted." << endl;
    }
  else if (!strcmp("vtkSMBoundsDomain", className))
    {
    vtkSMBoundsDomain *bd = vtkSMBoundsDomain::SafeDownCast(dom);
    switch (bd->GetMode())
      {
      case vtkSMBoundsDomain::NORMAL:
        domainWritten = true;
        docFile << "The coordinate must lie within the bounding box of the "
                << "dataset. It will default to the ";
        switch (bd->GetDefaultMode())
          {
          case vtkSMBoundsDomain::MIN:
            docFile << "minimum ";
            break;
          case vtkSMBoundsDomain::MAX:
            docFile << "maximum ";
            break;
          case vtkSMBoundsDomain::MID:
            docFile << "midpoint ";
            break;
          }
        docFile << "in each dimension." << endl;
        break;
      case vtkSMBoundsDomain::MAGNITUDE:
        domainWritten = true;
        docFile << "Determine the length of the dataset's diagonal. The "
                << "value must lie within -diagonal length to +diagonal "
                << "length." << endl;
        break;
      case vtkSMBoundsDomain::SCALED_EXTENT:
        domainWritten = true;
        docFile << "The value must be less than the largest dimension of the "
                << "dataset multiplied by a scale factor of "
                << bd->GetScaleFactor() << "." << endl;
        break;
      }
    }
  else if (!strcmp("vtkSMDataTypeDomain", className))
    {
    domainWritten = true;
    vtkSMDataTypeDomain *dtd = vtkSMDataTypeDomain::SafeDownCast(dom);
    docFile << "The selected dataset must be one of the following types (or a subclass of one of them):";
    for (i = 0; i < dtd->GetNumberOfDataTypes(); i++)
      {
      docFile << " " << dtd->GetDataType(i);
      if (i != (dtd->GetNumberOfDataTypes() - 1))
        {
        docFile << ",";
        }
      }
    docFile << "." << endl;
    }
  else if (!strcmp("vtkSMDoubleRangeDomain", className))
    {
    vtkSMDoubleRangeDomain *drd = vtkSMDoubleRangeDomain::SafeDownCast(dom);
    unsigned int numEntries = drd->GetNumberOfEntries();
    double min = drd->GetMinimum(0, minExists);
    double max = drd->GetMaximum(0, maxExists);

    if (minExists && !maxExists)
      {
      domainWritten = true;
      docFile << "The value must be greater than or equal to ";
      if (numEntries > 1)
        {
        docFile << "(";
        }
      docFile << min;
      for (i = 1; i < numEntries; i++)
        {
        min = drd->GetMinimum(i, minExists);
        docFile << ", ";
        if (minExists)
          {
          docFile << min;
          }
        }
      if (numEntries > 1)
        {
        docFile << ")";
        }
      docFile << "." << endl;
      }
    else if (!minExists && maxExists)
      {
      domainWritten = true;
      docFile << "The value must be less than or equal to ";
      if (numEntries > 1)
        {
        docFile << "(";
        }
      docFile << max;
      for (i = 1; i < numEntries; i++)
        {
        min = drd->GetMaximum(i, maxExists);
        docFile << ", ";
        if (maxExists)
          {
          docFile << max;
          }
        }
      if (numEntries > 1)
        {
        docFile << ")";
        }
      docFile << "." << endl;
      }
    else if (minExists && maxExists)
      {
      domainWritten = true;
      docFile << "The value must be greater than or equal to ";
      if (numEntries > 1)
        {
        docFile << "(";
        }
      docFile << min;
      for (i = 1; i < numEntries; i++)
        {
        min = drd->GetMinimum(i, minExists);
        docFile << ", ";
        if (minExists)
          {
          docFile << min;
          }
        }
      if (numEntries > 1)
        {
        docFile << ")";
        }
      docFile << " and less than or equal to ";
      if (numEntries > 1)
        {
        docFile << "(";
        }
      docFile << max;
      for (i = 1; i < numEntries; i++)
        {
        min = drd->GetMaximum(i, maxExists);
        docFile << ", ";
        if (maxExists)
          {
          docFile << max;
          }
        }
      if (numEntries > 1)
        {
        docFile << ")";
        }
      docFile << "." << endl;
      }
    }
  else if (!strcmp("vtkSMEnumerationDomain", className))
    {
    vtkSMEnumerationDomain *ed = vtkSMEnumerationDomain::SafeDownCast(dom);
    domainWritten = true;
    docFile << "The value must be one of the following:";
    for (i = 0; i < ed->GetNumberOfEntries(); i++)
      {
      docFile << " " << ed->GetEntryText(i) << " (" << ed->GetEntryValue(i)
              << ")";
      if (i != (ed->GetNumberOfEntries() - 1))
        {
        docFile << ",";
        }
      }
    docFile << "." << endl;
    }
  else if (!strcmp("vtkSMExtentDomain", className))
    {
    domainWritten = true;
    docFile << "The values must lie within the extent of the input dataset."
            << endl;
    }
  else if (!strcmp("vtkSMFieldDataDomain", className))
    {
    domainWritten = true;
    docFile << "Valud array names will be chosen from point and cell data."
            << endl;
    }
  else if (!strcmp("vtkSMFileListDomain", className))
    {
    }
  else if (!strcmp("vtkSMFixedTypeDomain", className))
    {
    domainWritten = true;
    docFile << "Once set, the input dataset type cannot be changed." << endl;
    }
  else if (!strcmp("vtkSMInputArrayDomain", className))
    {
    vtkSMInputArrayDomain *iad = vtkSMInputArrayDomain::SafeDownCast(dom);
    domainWritten = true;
    docFile << "The dataset must contain a ";
    switch (iad->GetAttributeType())
      {
      case vtkSMInputArrayDomain::POINT:
        docFile << "point";
        break;
      case vtkSMInputArrayDomain::CELL:
        docFile << "cell";
        break;
      case vtkSMInputArrayDomain::ANY:
        docFile << "point or cell";
        break;
      }
    docFile << " array";
    if (iad->GetNumberOfComponents() != 0)
      {
      docFile << " with " << iad->GetNumberOfComponents() << " components";
      }
    docFile << "." << endl;
    }
  else if (!strcmp("vtkSMIntRangeDomain", className))
    {
    vtkSMIntRangeDomain *ird = vtkSMIntRangeDomain::SafeDownCast(dom);
    unsigned int numEntries = ird->GetNumberOfEntries();
    int min = ird->GetMinimum(0, minExists);
    int max = ird->GetMaximum(0, maxExists);

    if (minExists && !maxExists)
      {
      domainWritten = true;
      docFile << "The value must be greater than or equal to ";
      if (numEntries > 1)
        {
        docFile << "(";
        }
      docFile << min;
      for (i = 1; i < numEntries; i++)
        {
        min = ird->GetMinimum(i, minExists);
        docFile << ", ";
        if (minExists)
          {
          docFile << min;
          }
        }
      if (numEntries > 1)
        {
        docFile << ")";
        }
      docFile << "." << endl;
      }
    else if (!minExists && maxExists)
      {
      domainWritten = true;
      docFile << "The value must be less than or equal to ";
      if (numEntries > 1)
        {
        docFile << "(";
        }
      docFile << max;
      for (i = 1; i < numEntries; i++)
        {
        min = ird->GetMaximum(i, maxExists);
        docFile << ", ";
        if (maxExists)
          {
          docFile << max;
          }
        }
      if (numEntries > 1)
        {
        docFile << ")";
        }
      docFile << "." << endl;
      }
    else if (minExists && maxExists)
      {
      domainWritten = true;
      docFile << "The value must be greater than or equal to ";
      if (numEntries > 1)
        {
        docFile << "(";
        }
      docFile << min;
      for (i = 1; i < numEntries; i++)
        {
        min = ird->GetMinimum(i, minExists);
        docFile << ", ";
        if (minExists)
          {
          docFile << min;
          }
        }
      if (numEntries > 1)
        {
        docFile << ")";
        }
      docFile << " and less than or equal to ";
      if (numEntries > 1)
        {
        docFile << "(";
        }
      docFile << max;
      for (i = 1; i < numEntries; i++)
        {
        min = ird->GetMaximum(i, maxExists);
        docFile << ", ";
        if (maxExists)
          {
          docFile << max;
          }
        }
      if (numEntries > 1)
        {
        docFile << ")";
        }
      docFile << "." << endl;
      }
    }
  else if (!strcmp("vtkSMNumberOfGroupsDomain", className))
    {
    vtkSMNumberOfGroupsDomain *nogd =
      vtkSMNumberOfGroupsDomain::SafeDownCast(dom);
    switch (nogd->GetGroupMultiplicity())
      {
      case vtkSMNumberOfGroupsDomain::NOT_SET:
        domainWritten = true;
        docFile << "The number of groups is specified by the input dataset.";
        break;
      case vtkSMNumberOfGroupsDomain::SINGLE:
        domainWritten = true;
        docFile << "Multi-group datasets must contain only one group.";
        break;
      case vtkSMNumberOfGroupsDomain::MULTIPLE:
        domainWritten = true;
        docFile << "Multi-group datasets must contain more than one group.";
        break;
      }
    docFile << endl;
    }
  else if (!strcmp("vtkSMProxyGroupDomain", className))
    {
    vtkSMProxyGroupDomain *pgd = vtkSMProxyGroupDomain::SafeDownCast(dom);
    if (pgd->GetNumberOfGroups() == 1 &&
        (!strcmp(pgd->GetGroup(0), "implicit_functions")))
      {
      return false;
      }
    domainWritten = true;
    docFile << "The selected object must be the result of the following:";
    for (i = 0; i < pgd->GetNumberOfGroups(); i++)
      {
      if (!strcmp(pgd->GetGroup(i), "sources"))
        {
        docFile << " sources (includes readers)";
        }
      else
        {
        docFile << " " << pgd->GetGroup(i);
        }
      if (i != (pgd->GetNumberOfGroups() - 1))
        {
        docFile << ",";
        }
      }
    docFile << "." << endl;
    }
  else if (!strcmp("vtkSMProxyListDomain", className))
    {
    vtkSMProxyListDomain *pld = vtkSMProxyListDomain::SafeDownCast(dom);
    domainWritten = true;
    docFile << "The value must be set to one of the following:";
    for (i = 0; i < pld->GetNumberOfProxyTypes(); i++)
      {
      docFile << " " << pld->GetProxyName(i);
      if (i != (pld->GetNumberOfProxyTypes() - 1))
        {
        docFile << ",";
        }
      }
    docFile << "." << endl;
    }
  else if (!strcmp("vtkSMStringListDomain", className))
    {
    vtkSMStringListDomain *sld = vtkSMStringListDomain::SafeDownCast(dom);
    domainWritten = true;
    const char* domName = sld->GetXMLName();
    if (!strcmp("AvailableDomains", domName))
      {
      docFile << "The domain must be chosen from those contained in the Xdmf "
              << "dataset." << endl;
      }
    else if (!strcmp("AvailableGrids", domName))
      {
      docFile << "The grid must be chosen from those contained in the Xdmf "
              << "dataset." << endl;
      }
    else
      {
      for (i = 0; i < sld->GetNumberOfStrings(); i++)
        {
        docFile << " " << sld->GetString(i);
        if (i != (sld->GetNumberOfStrings()-1))
          {
          docFile << ",";
          }
        }
      docFile << "." << endl;
      }
    }
  else if (!strcmp("vtkSMStringListRangeDomain", className))
    {
    }
  else if (!strcmp("vtkXdmfPropertyDomain", className))
    {
    }
  else
    {
    docFile << "Unknown domain type: " << dom->GetClassName() << endl;
    }
  return domainWritten;
}

void ExtractProxyNames(vtkPVXMLElement *elem, vtkStringPairList *proxyNameList, vtkSMProxyManager* manager)
{
  if (elem->GetNumberOfNestedElements() == 0)
    {
    char *elemName = elem->GetName();
    if (!strcmp(elemName, "Category"))
      {
      return;
      }
    if (!strcmp(elemName, "ProxyManager"))
      {
      // Get list from ProxyManager
      if (!strcmp(elem->GetAttribute("type"), "reader"))
        {
        vtkPVProxyDefinitionIterator* iter =
            manager->GetProxyDefinitionManager()->NewSingleGroupIterator("sources");

        for(iter->InitTraversal();!iter->IsDoneWithTraversal();iter->GoToNextItem())
          {
          const char* proxyName = iter->GetProxyName();
          vtkPVXMLElement* proxyDef = iter->GetProxyDefinition();
          if(proxyDef)
            {
            vtkPVXMLElement* hints = proxyDef->FindNestedElementByName("Hints");
            if(hints)
              {
              if(hints->FindNestedElementByName("ReaderFactory"))
                {
                vtkstd::pair<vtkstd::string, vtkstd::string> namePair(proxyName, "sources");
                proxyNameList->insert(proxyNameList->end(), namePair);
                }
              }
            }
          }
        iter->Delete();
        }
      else if (!strcmp(elem->GetAttribute("type"), "writer"))
        {
        vtkPVProxyDefinitionIterator* iter =
            manager->GetProxyDefinitionManager()->NewSingleGroupIterator("writers");
       for(iter->InitTraversal();!iter->IsDoneWithTraversal();iter->GoToNextItem())
          {
          const char* proxyName = iter->GetProxyName();
          vtkstd::pair<vtkstd::string, vtkstd::string> namePair(proxyName, "writers");
          proxyNameList->insert(proxyNameList->end(), namePair);
          }
       iter->Delete();
        }
      return;
      }
    elemName[0] = tolower(elemName[0]);
    vtksys_ios::ostringstream groupName;
    if (!strcmp(elemName, "reader"))
      {
      groupName << "sources" << ends;
      }
    else if (strcmp(elemName, "proxy") == 0)
      {
      groupName << elem->GetAttribute("group");
      }
    else
      {
      groupName << elemName << "s" << ends;
      }
    const char *xmlProxyName = elem->GetAttribute("name");
    vtkstd::pair<vtkstd::string, vtkstd::string> namePair(xmlProxyName, groupName.str());
    proxyNameList->insert(proxyNameList->end(), namePair);
    }
  else
    {
    unsigned int i;
    for (i = 0; i < elem->GetNumberOfNestedElements(); i++)
      {
      ExtractProxyNames(elem->GetNestedElement(i), proxyNameList, manager);
      }
    }
}

void WriteProperty(const char* pname, vtkSMProperty* prop, ostream& docFile)
{
  TemplateMap dataMap;
  dataMap["NAME"] = pname;
  dataMap["LABEL"] = prop->GetXMLLabel();
  vtkSMDocumentation* documentation = prop->GetDocumentation();
  if (documentation && documentation->GetDescription())
    {
    dataMap["DESCRIPTION"] = documentation->GetDescription();
    }
  else
    {
    dataMap["DESCRIPTION"] = "&nbsp;";
    }

  vtksys_ios::ostringstream default_value_stream;
  if (!WriteDefaultValues(prop, default_value_stream))
    {
    dataMap["DEFAULTVALUES"] = "&nbsp;";
    }
  else
    {
    dataMap["DEFAULTVALUES"] = default_value_stream.str();
    }

  vtksys_ios::ostringstream all_domains_stream;

  vtkSMDomainIterator* dIt = prop->NewDomainIterator();
  for (dIt->Begin(); !dIt->IsAtEnd(); dIt->Next())
    {
    if (!dIt->GetDomain()->GetIsOptional())
      {
      vtksys_ios::ostringstream domain_stream;
      if (WriteDomain(dIt->GetDomain(), domain_stream))
        {
        all_domains_stream << "<p>"
          << domain_stream.str().c_str()
          << "</p>";
        }
      }
    }
  dIt->Delete();
  all_domains_stream  << "&nbsp;";
  dataMap["DOMAINS"] = all_domains_stream.str();
  WriteFromTemplate(docFile, ::PropertyTemplate, dataMap);
}

void WriteProperties(vtkSMProxy* proxy, ostream& docFile)
{
  if (!proxy)
    {
    return;
    }

  docFile << ::PropertiesTableHeaderTemplate << endl;

  vtkSMPropertyIterator* pIt = proxy->NewPropertyIterator();
  pIt->Begin();
  for (; !pIt->IsAtEnd(); pIt->Next())
    {
    vtkSMProperty* prop = pIt->GetProperty();
    if (!prop->GetInformationOnly() && !prop->GetIsInternal())
      {
      WriteProperty(pIt->GetKey(), prop, docFile);
      }
    }

  docFile << ::PropertiesTableFooterTemplate<< endl;
  pIt->Delete();
}


void WriteProxyHeader(vtkSMProxy* proxy, ostream& docFile)
{
  if (!proxy)
    {
    return;
    }
  TemplateMap dataMap;
  dataMap["TITLE"] = proxy->GetXMLLabel();
  WriteFromTemplate(docFile, ProxyDocumentHeadTemplate, dataMap);
}

void WriteProxyDocumentation(vtkSMProxy* proxy, ostream& docFile)
{
  if (!proxy)
    {
    return;
    }

  TemplateMap dataMap;
  dataMap["NAME"] = proxy->GetXMLName();
  dataMap["LABEL"] = proxy->GetXMLLabel();
  vtkSMDocumentation* documentation = proxy->GetDocumentation();
  if (documentation)
    {
    bool some_help_added = false;
    if (documentation->GetLongHelp())
      {
      dataMap["LONGHELP"] = documentation->GetLongHelp();
      some_help_added = true;
      }
    else
      {
      dataMap["LONGHELP"] = "&nbsp;";
      }
    if (documentation->GetShortHelp())
      {
      dataMap["SHORTHELP"] = documentation->GetShortHelp();
      }
    else
      {
      dataMap["SHORTHELP"] = proxy->GetXMLLabel();
      }
    if (documentation->GetDescription())
      {
      vtkstd::string description = documentation->GetDescription();
      description = FilterDescription(description);
      //vtksys::SystemTools::ReplaceString(description, "\n", "<br /><br />\n");
      dataMap["DESCRIPTION"] = description;
      some_help_added = true;
      }
    else
      {
      dataMap["DESCRIPTION"] = "&nbsp;";
      }

    // Some message only if neither short help nor description is available.
    if (!some_help_added)
      {
      dataMap["LONGHELP"] = "Sorry, no help is currently available.";
      }
    }
  else
    {
    // Fill values for all expected keys.
    dataMap["SHORTHELP"] = proxy->GetXMLLabel();
    dataMap["LONGHELP"] = "Sorry, no help is currently available.";
    dataMap["DESCRIPTION"] = "&nbsp;";
    }

  WriteFromTemplate(docFile, ::ProxyDocumentationTemplate, dataMap);
}

void WriteProxies(vtkStringPairList *stringList, vtkStringPairList *labelList,
                  vtkSMProxyManager *manager, char *filePath)
{
  vtkSMProxy *proxy;
  ofstream docFile;

  vtkStringPairListIterator iter;
  for (iter = stringList->begin(); iter != stringList->end(); iter++)
    {
    proxy = manager->GetPrototypeProxy((*iter).second.c_str(), (*iter).first.c_str());
    if (!proxy)
      {
      continue;
      }

    vtksys_ios::ostringstream filename;
    filename << filePath << "/" << (*iter).first.c_str() << ".html" << ends;
    docFile.open(filename.str().c_str());


    char *label = proxy->GetXMLLabel();
    vtkstd::pair<vtkstd::string, vtkstd::string> nameLabelPair(
      label, (*iter).first);
    labelList->insert(labelList->end(), nameLabelPair);

    docFile << "<html>" << endl;
    WriteProxyHeader(proxy, docFile);

    docFile << "<body>" << endl;
    WriteProxyDocumentation(proxy, docFile);
    WriteProperties(proxy, docFile);
    docFile << "</body>" << endl;
    docFile << "</html>" << endl;
    docFile.close();
    }
}

void WriteHTMLList(const char* groupname, vtkStringPairList *nameList, ostream &baseFile)
{
  // Write header.
  TemplateMap dataMap;
  dataMap["TITLE"] = groupname;
  baseFile << "<html>" << endl;
  WriteFromTemplate(baseFile, ProxyDocumentHeadTemplate, dataMap);
  baseFile << "<body>" << endl;
  WriteFromTemplate(baseFile, ProxyListTitleTemplate, dataMap);
  WriteFromTemplate(baseFile, ProxyListTableHeaderTemplate, dataMap);

  vtkSMProxyManager* manager = vtkSMObject::GetProxyManager();
  vtkStringPairListIterator iter;

  vtkStringPairList proxyListItems;

  for (iter = nameList->begin(); iter != nameList->end(); iter++)
    {
    vtkSMProxy* proxy = manager->GetPrototypeProxy((*iter).second.c_str(), 
      (*iter).first.c_str());
    if (!proxy)
      {
      continue;
      }

    dataMap.clear();
    dataMap["LINK"] = vtkstd::string(proxy->GetXMLName()) + ".html";
    dataMap["LABEL"] = proxy->GetXMLLabel();
    dataMap["DESCRIPTION"] = "&nbsp;";

    vtkSMDocumentation* documentation = proxy->GetDocumentation();
    if (documentation)
      {
      if (documentation->GetShortHelp())
        {
        dataMap["DESCRIPTION"] = documentation->GetShortHelp();
        }
      else if (documentation->GetLongHelp())
        {
        dataMap["DESCRIPTION"] = documentation->GetLongHelp();
        }
      }
    vtksys_ios::ostringstream stream;
    WriteFromTemplate(stream, ProxyListItemTemplate, dataMap);
    vtkstd::pair<vtkstd::string, vtkstd::string> labelHtmlPair(
      proxy->GetXMLLabel(), stream.str());
    proxyListItems.push_back(labelHtmlPair);
    }

  proxyListItems.sort();
  for (iter = proxyListItems.begin(); iter!= proxyListItems.end(); ++iter)
    {
    baseFile << iter->second.c_str() << endl;
    }
  

  WriteFromTemplate(baseFile, ProxyListTableFooterTemplate, dataMap);
  baseFile << "</body>" << endl;
  baseFile << "</html>" << endl;
}

void WriteXMLKeywords(const char* baseName, vtkStringPairList *nameList,
                      istream &inFile, ostream& outFile)
{
  char line[256];
  char extractedName[100];

  vtksys_ios::ostringstream baseFileName;
  baseFileName << baseName << ".html\"" << ends;

  inFile.getline(line, 255);
  while (!inFile.eof() && !inFile.fail())
    {
    outFile << line << endl;
    if (sscanf(line, " <section ref=\"Documentation/%s", extractedName) == 1 &&
        !strcmp(extractedName, baseFileName.str().c_str()))
      {
      break;
      }
    inFile.getline(line, 255);
    }

  vtkSMProxyManager* manager = vtkSMObject::GetProxyManager();
  vtkStringPairListIterator iter;
  for (iter = nameList->begin(); iter != nameList->end(); iter++)
    {
    vtkSMProxy *proxy = manager->GetPrototypeProxy((*iter).second.c_str(),
                                                   (*iter).first.c_str());
    if (!proxy)
      {
      continue;
      }

    outFile << "    <keyword ref=\"Documentation/" << proxy->GetXMLName()
            << ".html\">" << proxy->GetXMLLabel() << "</keyword>" << endl;
    }

  inFile.getline(line, 255);

  // Skip over any keywords that were already in pqClient.adp.
  while (!inFile.eof() && !inFile.fail())
    {
    if (!strcmp(line, "  </section>"))
      {
      outFile << line << endl;
      break;
      }
    inFile.getline(line, 255);
    }

  inFile.getline(line, 255);
  while (!inFile.eof() && !inFile.fail())
    {
    outFile << line << endl;
    inFile.getline(line, 255);
    }
}

int main(int argc, char *argv[])
{
  if (argc < 3)
    {
    cout << "Usage: " << argv[0] << " <output path> <input file name>" << endl;
    return 1;
    }

  vtkPVXMLParser *parser = vtkPVXMLParser::New();
  parser->SetFileName(argv[2]);
  parser->Parse();
  vtkPVXMLElement *rootElem = parser->GetRootElement();

  char *baseName = strrchr(argv[2], '/');
  baseName++;
  char *ext = strchr(baseName, '.');
  int pos = ext - baseName;
  baseName[pos] = '\0';
  ofstream baseFile;
  vtksys_ios::ostringstream baseFileName;
  baseFileName << argv[1] << "/" << baseName << ".html" << ends;
  baseFile.open(baseFileName.str().c_str());

  char* proxyTypeName = strchr(baseName, 'w');
  proxyTypeName++;

  vtkInitializationHelper::Initialize(argv[0],
    vtkProcessModule::PROCESS_CLIENT);
  vtkSMSession* session = vtkSMSession::New();
  vtkSMProxyManager *manager = vtkSMObject::GetProxyManager();
  vtkStringPairList *proxyNameList = new vtkStringPairList;
  ExtractProxyNames(rootElem, proxyNameList, manager);
  proxyNameList->sort();
  proxyNameList->unique();

  vtkStringPairList *proxyLabelList = new vtkStringPairList;
  
  WriteProxies(proxyNameList, proxyLabelList, manager, argv[1]);

  proxyLabelList->sort();

  WriteHTMLList(proxyTypeName, proxyNameList, baseFile);

  ifstream adpFile;
  vtksys_ios::ostringstream adpFileName;
  adpFileName << argv[1] << "/../pqClient.adp" << ends;
  adpFile.open(adpFileName.str().c_str());

  ofstream tmpAdpFile;
  vtksys_ios::ostringstream tmpAdpFileName;
  tmpAdpFileName << argv[1] << "/../temp.adp" << ends;
  tmpAdpFile.open(tmpAdpFileName.str().c_str());

  WriteXMLKeywords(baseName, proxyNameList, adpFile, tmpAdpFile);

  adpFile.close();
  tmpAdpFile.close();
  vtksys::SystemTools::CopyAFile(tmpAdpFileName.str().c_str(),
                                 adpFileName.str().c_str());
  vtksys::SystemTools::RemoveFile(tmpAdpFileName.str().c_str());

  vtkStringPairListIterator iter;
  for (iter = proxyNameList->begin(); iter != proxyNameList->end();)
    {
    proxyNameList->erase(iter++);
    }
  delete proxyNameList;

  vtkStringPairListIterator iter2;
  for (iter2 = proxyLabelList->begin(); iter2 != proxyLabelList->end();)
    {
    proxyLabelList->erase(iter2++);
    }
  delete proxyLabelList;

  baseFile.close();
  parser->Delete();
  session->Delete();
  vtkInitializationHelper::Finalize();
  return 0;
}
