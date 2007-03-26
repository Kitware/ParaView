#include "vtkDataSetAttributes.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLParser.h"
#include "vtkPVXMLElement.h"
#include "vtkSMApplication.h"
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
#include "vtkSMNumberOfPartsDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyManager.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringVectorProperty.h"

#include <vtkstd/list>
#include <vtkstd/string>

class vtkStringPairList : public vtkstd::list<vtkstd::pair<vtkstd::string, vtkstd::string>> {};
typedef vtkstd::list<vtkstd::pair<vtkstd::string, vtkstd::string>>::iterator vtkStringPairListIterator;

bool operator < (const vtkstd::pair<vtkstd::string, vtkstd::string> &x,
                 const vtkstd::pair<vtkstd::string, vtkstd::string> &y)
{
  return x.first < y.first;
}

void WriteDocumentation(vtkSMDocumentation *doc, ofstream &docFile)
{
  if (doc->GetLongHelp())
    {
    docFile << "<b>long help:</b> " << doc->GetLongHelp() << endl;
    docFile << "<br>" << endl;
    }
  if (doc->GetShortHelp())
    {
    docFile << "<b>short help:</b> " << doc->GetShortHelp() << endl;
    docFile << "<br>" << endl;
    }
  if (doc->GetDescription())
    {
    docFile << "<b>description:</b> " << doc->GetDescription() << endl;
    docFile << "<br>" << endl;
    }
}

void WriteDefaultValues(vtkSMProperty *prop, ofstream &docFile)
{
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
      if (intVecProp->GetNumberOfElements() > 1)
        {
        docFile << "<b>Default Values:</b>" << endl;
        }
      else
        {
        docFile << "<b>Default Value:</b>" << endl;
        }
      for (i = 0; i < intVecProp->GetNumberOfElements(); i++)
        {
        docFile << elements[i] << " ";
        }
      docFile << "<br>" << endl;
      }
    }
  else if (doubleVecProp)
    {
    double *elements = doubleVecProp->GetElements();
    if (doubleVecProp->GetNumberOfElements() && elements)
      {
      if (doubleVecProp->GetNumberOfElements() > 1)
        {
        docFile << "<b>Default Values:</b>" << endl;
        }
      else
        {
        docFile << "<b>Default Value:</b>" << endl;
        }
      for (i = 0; i < doubleVecProp->GetNumberOfElements(); i++)
        {
        docFile << elements[i] << " ";
        }
      docFile << "<br>" << endl;
      }
    }
  else if (stringVecProp)
    {
    if (stringVecProp->GetNumberOfElements() &&
        strlen(stringVecProp->GetElement(0)) > 0)
      {
      if (stringVecProp->GetNumberOfElements() > 1)
        {
        docFile << "<b>Default Values:</b>" << endl;
        }
      else
        {
        docFile << "<b>Default Value:</b>" << endl;
        }
      for (i = 0; i < stringVecProp->GetNumberOfElements(); i++)
        {
        docFile << stringVecProp->GetElement(i) << " ";
        }
      docFile << "<br>" << endl;
      }
    }
}

void WriteDomainHeader(int &written, ofstream &docFile)
{
  if (!written)
    {
    docFile << "<b>Restrictions:</b> ";
    written = 1;
    }
}

void WriteDomain(vtkSMDomain *dom, ofstream &docFile, int &headerWritten)
{
  const char* className = dom->GetClassName();
  unsigned int i;
  int minExists, maxExists;

  if (!strcmp("vtkSMArrayListDomain", className))
    {
    vtkSMArrayListDomain *ald = vtkSMArrayListDomain::SafeDownCast(dom);
    switch (ald->GetAttributeType())
      {
      case vtkDataSetAttributes::SCALARS:
        WriteDomainHeader(headerWritten, docFile);
        docFile << "An array of scalars is required." << endl;
        break;
      case vtkDataSetAttributes::VECTORS:
        WriteDomainHeader(headerWritten, docFile);
        docFile << "An array of vectors is required." << endl;
        break;
      }
    }
  else if (!strcmp("vtkSMArrayRangeDomain", className))
    {
    WriteDomainHeader(headerWritten, docFile);
    docFile << "The value must lie within the range of the selected data "
            << "array." << endl;
    }
  else if (!strcmp("vtkSMArraySelectionDomain", className))
    {
    WriteDomainHeader(headerWritten, docFile);
    docFile << "The list of array names is provided by the reader." << endl;
    }
  else if (!strcmp("vtkSMBooleanDomain", className))
    {
    WriteDomainHeader(headerWritten, docFile);
    docFile << "Only the values 0 and 1 are accepted." << endl;
    }
  else if (!strcmp("vtkSMBoundsDomain", className))
    {
    vtkSMBoundsDomain *bd = vtkSMBoundsDomain::SafeDownCast(dom);
    switch (bd->GetMode())
      {
      case vtkSMBoundsDomain::NORMAL:
        WriteDomainHeader(headerWritten, docFile);
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
        WriteDomainHeader(headerWritten, docFile);
        docFile << "Determine the length of the dataset's diagonal. The "
                << "value must lie within -diagonal length to +diagonal "
                << "length." << endl;
        break;
      case vtkSMBoundsDomain::SCALED_EXTENT:
        WriteDomainHeader(headerWritten, docFile);
        docFile << "The value must be less than the largest dimension of the "
                << "dataset multiplied by a scale factor of "
                << bd->GetScaleFactor() << "." << endl;
        break;
      }
    }
  else if (!strcmp("vtkSMDataTypeDomain", className))
    {
    WriteDomainHeader(headerWritten, docFile);
    vtkSMDataTypeDomain *dtd = vtkSMDataTypeDomain::SafeDownCast(dom);
    docFile << "The selected dataset must be one of the following types:";
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
      WriteDomainHeader(headerWritten, docFile);
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
      WriteDomainHeader(headerWritten, docFile);
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
      WriteDomainHeader(headerWritten, docFile);
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
    WriteDomainHeader(headerWritten, docFile);
    docFile << "The value must be one of the following:";
    for (i = 0; i < ed->GetNumberOfEntries(); i++)
      {
      docFile << " " << ed->GetEntryText(i);
      if (i != (ed->GetNumberOfEntries() - 1))
        {
        docFile << ",";
        }
      }
    docFile << "." << endl;
    }
  else if (!strcmp("vtkSMExtentDomain", className))
    {
    WriteDomainHeader(headerWritten, docFile);
    docFile << "The values must lie within the extent of the dataset." << endl;
    }
  else if (!strcmp("vtkSMFieldDataDomain", className))
    {
    WriteDomainHeader(headerWritten, docFile);
    docFile << "Valud array names will be chosen from point and cell data."
            << endl;
    }
  else if (!strcmp("vtkSMFileListDomain", className))
    {
    }
  else if (!strcmp("vtkSMFixedTypeDomain", className))
    {
    WriteDomainHeader(headerWritten, docFile);
    docFile << "Once set, the input dataset type cannot be changed." << endl;
    }
  else if (!strcmp("vtkSMInputArrayDomain", className))
    {
    vtkSMInputArrayDomain *iad = vtkSMInputArrayDomain::SafeDownCast(dom);
    WriteDomainHeader(headerWritten, docFile);
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
      WriteDomainHeader(headerWritten, docFile);
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
      WriteDomainHeader(headerWritten, docFile);
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
      WriteDomainHeader(headerWritten, docFile);
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
        WriteDomainHeader(headerWritten, docFile);
        docFile << "The number of groups is specified by the input dataset.";
        break;
      case vtkSMNumberOfGroupsDomain::SINGLE:
        WriteDomainHeader(headerWritten, docFile);
        docFile << "Multi-group datasets must contain only one group.";
        break;
      case vtkSMNumberOfGroupsDomain::MULTIPLE:
        WriteDomainHeader(headerWritten, docFile);
        docFile << "Multi-group datasets must contain more than one group.";
        break;
      }
    docFile << endl;
    }
  else if (!strcmp("vtkSMNumberOfPartsDomain", className))
    {
    vtkSMNumberOfPartsDomain *nopd =
      vtkSMNumberOfPartsDomain::SafeDownCast(dom);
    switch (nopd->GetPartMultiplicity())
      {
      case vtkSMNumberOfPartsDomain::SINGLE:
        WriteDomainHeader(headerWritten, docFile);
        docFile << "Multi-part datasets must have only one part.";
        break;
      case vtkSMNumberOfPartsDomain::MULTIPLE:
        WriteDomainHeader(headerWritten, docFile);
        docFile << "Multi-part datasets must have more than one part.";
        break;
      }
    docFile << endl;
    }
  else if (!strcmp("vtkSMProxyGroupDomain", className))
    {
    vtkSMProxyGroupDomain *pgd = vtkSMProxyGroupDomain::SafeDownCast(dom);
    WriteDomainHeader(headerWritten, docFile);
    docFile << "The dataset must have been the result of the following:";
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
    WriteDomainHeader(headerWritten, docFile);
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
    WriteDomainHeader(headerWritten, docFile);
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

  if (headerWritten)
    {
    docFile << "<br>" << endl;
    }
}

void ExtractProxyNames(vtkPVXMLElement *elem, vtkStringPairList *proxyNameList)
{
  if (elem->GetNumberOfNestedElements() == 0)
    {
    char *elemName = elem->GetName();
    if (!strcmp(elemName, "Category"))
      {
      return;
      }
    elemName[0] = tolower(elemName[0]);
    ostrstream groupName;
    if (!strcmp(elemName, "reader"))
      {
      groupName << "sources" << ends;
      }
    else
      {
      groupName << elemName << "s" << ends;
      }
    const char *xmlProxyName = elem->GetAttribute("name");
    vtkstd::pair<vtkstd::string, vtkstd::string> namePair(xmlProxyName, groupName.str());
    proxyNameList->insert(proxyNameList->end(), namePair);
    groupName.rdbuf()->freeze(0);
    }
  else
    {
    unsigned int i;
    for (i = 0; i < elem->GetNumberOfNestedElements(); i++)
      {
      ExtractProxyNames(elem->GetNestedElement(i), proxyNameList);
      }
    }
}

void WriteProxies(vtkStringPairList *stringList, vtkStringPairList *labelList,
                  vtkSMProxyManager *manager, char *filePath)
{
  vtkSMProxy *proxy;
  vtkSMProperty *prop;
  vtkSMPropertyIterator *pIt;
  vtkSMDomainIterator *dIt;
  vtkSMDocumentation *doc;
  ofstream docFile;

  vtkStringPairListIterator iter;
  for (iter = stringList->begin(); iter != stringList->end(); iter++)
    {
    proxy = manager->GetPrototypeProxy((*iter).second.c_str(), (*iter).first.c_str());
    if (!proxy)
      {
      return;
      }

    ostrstream filename;
    filename << filePath << "/" << (*iter).first.c_str() << ".html" << ends;
    docFile.open(filename.str());
    filename.rdbuf()->freeze(0);
    docFile << "<html>" << endl;
    docFile << "<head>" << endl;
    docFile << "<title>";

    char *label = proxy->GetXMLLabel();
    if (label)
      {
      docFile << label;
      vtkstd::pair<vtkstd::string, vtkstd::string> nameLabelPair(
        label, (*iter).first);
      labelList->insert(labelList->end(), nameLabelPair);
      }
    else
      {
      docFile << (*iter).first.c_str();
      vtkstd::pair<vtkstd::string, vtkstd::string> nameLabelPair(
        (*iter).first, (*iter).first);
      labelList->insert(labelList->end(), nameLabelPair);
      }
    docFile << "</title>" << endl;
    docFile << "</head>" << endl;
    docFile << "<body>" << endl;
    docFile << "<h2>";
    if (label)
      {
      docFile << label;
      }
    else
      {
      docFile << (*iter).first.c_str();
      }
    docFile << "</h2>" << endl;
    doc = proxy->GetDocumentation();
    if (doc)
      {
      WriteDocumentation(doc, docFile);
      }
    else
      {
      docFile << "NO DOCUMENTATION" << endl;
      docFile << "<br>" << endl;
      }
    pIt = proxy->NewPropertyIterator();
    pIt->Begin();
    for (; !pIt->IsAtEnd(); pIt->Next())
      {
      prop = pIt->GetProperty();
      if (!prop->GetInformationOnly() && !prop->GetIsInternal())
        {
        docFile << "<br>" << endl;
        docFile << "<b>" << pIt->GetKey() << "</b><br>" << endl;
        doc = prop->GetDocumentation();
        if (doc)
          {
          WriteDocumentation(doc, docFile);
          }
        else
          {
          docFile << "NO DOCUMENTATION" << endl;
          docFile << "<br>" << endl;
          }

        WriteDefaultValues(prop, docFile);

        dIt = prop->NewDomainIterator();
        dIt->Begin();
        if (prop->GetNumberOfDomains())
          {
          int domainHeaderWritten = 0;
          for (; !dIt->IsAtEnd(); dIt->Next())
            {
            WriteDomain(dIt->GetDomain(), docFile, domainHeaderWritten);
            }
          docFile << "<br>" << endl;
          }
        dIt->Delete();
        }
      }
    docFile << "</body>" << endl;
    docFile << "</html>" << endl;
    pIt->Delete();
    docFile.close();
    }
}

void WriteHTMLList(vtkStringPairList *labelList, ofstream &baseFile)
{
  vtkStringPairListIterator iter;
  for (iter = labelList->begin(); iter != labelList->end(); iter++)
    {
    baseFile << "<a href=\"" << (*iter).second.c_str() << ".html\">"
             << (*iter).first.c_str() << "</a><br>" << endl;
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
  ostrstream baseFileName;
  baseFileName << argv[1] << "/" << baseName << ".html" << ends;
  baseFile.open(baseFileName.str());
  baseFileName.rdbuf()->freeze(0);
  baseFile << "<html>" << endl;
  baseFile << "<head>" << endl;
  baseFile << "<title>" << endl;
  char* proxyTypeName = strchr(baseName, 'w');
  proxyTypeName++;
  baseFile << proxyTypeName << endl;
  baseFile << "</title>" << endl;
  baseFile << "</head>" << endl;
  
  vtkProcessModule *pm = vtkProcessModule::New();
  pm->Initialize();
  vtkProcessModule::SetProcessModule(pm);
  vtkSMApplication *app = vtkSMApplication::New();
  app->Initialize();

  vtkSMProxyManager *manager = vtkSMObject::GetProxyManager();

  vtkStringPairList *proxyNameList = new vtkStringPairList;
  ExtractProxyNames(rootElem, proxyNameList);
  proxyNameList->sort();
  proxyNameList->unique();

  vtkStringPairList *proxyLabelList = new vtkStringPairList;
  
  WriteProxies(proxyNameList, proxyLabelList, manager, argv[1]);

  proxyLabelList->sort();

  WriteHTMLList(proxyLabelList, baseFile);

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
  pm->Finalize();
  pm->Delete();
  app->Finalize();
  app->Delete();
  return 0;
}
