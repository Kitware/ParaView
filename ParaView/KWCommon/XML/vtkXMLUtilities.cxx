/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUtilities.cxx
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
#include "vtkXMLUtilities.h"

#include "vtkObjectFactory.h"
#include "vtkString.h"
#include "vtkVector.txx"
#include "vtkVectorIterator.txx"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"

vtkStandardNewMacro(vtkXMLUtilities);
vtkCxxRevisionMacro(vtkXMLUtilities, "1.4");

#define  VTK_XML_UTILITIES_FACTORED_POOL_NAME "FactoredPool"
#define  VTK_XML_UTILITIES_FACTORED_NAME      "Factored"
#define  VTK_XML_UTILITIES_FACTORED_REF_NAME  "FactoredRef"

//----------------------------------------------------------------------------
void vtkXMLUtilities::ConvertSpecialChars(const char *str, ostream &os)
{
  if (!str)
    {
    return;
    }

  while (*str)
    {
    switch (*str)
      {
      case '&':
        os << "&amp;";
        break;

      case '"':
        os << "&quot;";
        break;

      case '\'':
        os << "&apos;";
        break;

      case '<':
        os << "&lt;";
        break;

      case '>':
        os << "&gt;";
        break;

      case '\n':
        os << "&#xA;";
        break;

      default:
        os << *str;
      }
    str++;
    }
}

//----------------------------------------------------------------------------
void vtkXMLUtilities::CollateAttributes(vtkXMLDataElement *elem, 
                                        ostream &os,
                                        const char *sep)
{
  if (!elem)
    {
    return;
    }

  int i, nb = elem->GetNumberOfAttributes();
  for (i = 0; i < nb; i++)
    {
    const char *name = elem->GetAttributeName(i);
    if (name)
      {
      const char *value = elem->GetAttribute(name);
      if (value)
        {
        if (i)
          {
          os << (sep ? sep : " ");
          }
        os << name << "=\"";
        vtkXMLUtilities::ConvertSpecialChars(value, os);
        os << '\"';
        }
      }
    }

  return;
}

//----------------------------------------------------------------------------
void vtkXMLUtilities::FlattenElement(vtkXMLDataElement *elem, 
                                     ostream &os,
                                     vtkIndent *indent,
                                     int indent_attributes)
{
  if (!elem)
    {
    return;
    }

  unsigned long pos = os.tellp();

  // Name

  if (indent)
    {
    os << *indent;
    }

  os << '<' << elem->GetName();

  // Attributes

  if (elem->GetNumberOfAttributes())
    {
    os << ' ';
    if (indent && indent_attributes)
      {
      unsigned long len = (unsigned long)os.tellp() - pos;
      char *sep = new char [1 + len + 1];
      sep[0] = '\n';
      vtkString::FillString(sep + 1, ' ', len);
      vtkXMLUtilities::CollateAttributes(elem, os, sep);
      delete [] sep;
      }
    else
      {
      vtkXMLUtilities::CollateAttributes(elem, os);
      }
    }

  // Nested elements and close

  int nb_nested = elem->GetNumberOfNestedElements();
  if (!nb_nested)
    {
    os << "/>";
    }
  else
    {
    os << '>';
    if (indent)
      {
      os << '\n';
      }
    for (int i = 0; i < nb_nested; i++)
      {
      if (indent)
        {
        vtkIndent next_indent = indent->GetNextIndent();
        vtkXMLUtilities::FlattenElement(elem->GetNestedElement(i), 
                                        os, &next_indent);
        }
      else
        {
        vtkXMLUtilities::FlattenElement(elem->GetNestedElement(i), os);
        }
      }
    if (indent)
      {
      os << *indent;
      }
    os << "</" << elem->GetName() << '>';
    }
  if (indent)
    {
    os << '\n';
    }
}

//----------------------------------------------------------------------------
void vtkXMLUtilities::WriteElement(vtkXMLDataElement *elem, 
                                   const char *filename)
{
  if (!filename)
    {
    return;
    }

  ofstream os(filename, ios::out);
  vtkIndent indent;

  vtkXMLUtilities::FlattenElement(elem, os, &indent);
}

//----------------------------------------------------------------------------
vtkXMLDataElement*
vtkXMLUtilities::ReadElement(const char *filename)
{
  if (!filename)
    {
    return NULL;
    }

  vtkXMLDataElement *res = NULL;
  vtkXMLDataParser* xml_parser = vtkXMLDataParser::New();

  ifstream is(filename);
  xml_parser->SetStream(&is);
  if (xml_parser->Parse())
    {
    res = xml_parser->GetRootElement();
    res->SetReferenceCount(res->GetReferenceCount() + 1);
    }

  xml_parser->Delete();
  return res;
}

//----------------------------------------------------------------------------
int vtkXMLUtilities::FindSimilarElements(vtkXMLDataElement *elem, 
                                         vtkXMLDataElement *tree, 
                                         vtkXMLDataElement ***results)
{
  if (!elem || ! tree)
    {
    return 0;
    }

  // Create a data element container, and find all similar elements

  vtkXMLUtilities::DataElementContainer *container = 
    vtkXMLUtilities::DataElementContainer::New();
  
  vtkXMLUtilities::FindSimilarElementsInternal(elem, tree, container);

  // If nothing was found, exit now

  int size = (int)container->GetNumberOfItems();
  if (size)
    {
    // Allocate an array of element and copy the contents of the container
    // to this flat structure

    *results = new vtkXMLDataElement* [size];
    
    size = 0;
    vtkXMLDataElement *similar_elem = NULL;
    vtkXMLUtilities::DataElementContainerIterator *it = 
      container->NewIterator();

    it->InitTraversal();
    while (!it->IsDoneWithTraversal())
      {
      if (it->GetData(similar_elem) == VTK_OK)
        {
        (*results)[size++] = similar_elem;
        }
      it->GoToNextItem();
      }
    it->Delete();
    }

  container->Delete();

  return size;
}

//----------------------------------------------------------------------------
void vtkXMLUtilities::FindSimilarElementsInternal(vtkXMLDataElement *elem, 
                                                  vtkXMLDataElement *tree, 
                                                 DataElementContainer *results)
{
  if (!elem || !tree || !results || elem == tree)
    {
    return;
    }

  // If the element is equal to the current tree, append it to the
  // results, otherwise check the sub-trees

  if (elem->IsEqualTo(tree))
    {
    results->AppendItem(tree);
    }
  else
    {
    for (int i = 0; i < tree->GetNumberOfNestedElements(); i++)
      {
      vtkXMLUtilities::FindSimilarElementsInternal(
        elem, tree->GetNestedElement(i), results);
      }
    }
}

//----------------------------------------------------------------------------
void vtkXMLUtilities::FactorElements(vtkXMLDataElement *tree)
{
  if (!tree)
    {
    return;
    }

  // Create the factored pool, and add it to the tree so that it can
  // factor itself too

  vtkXMLDataElement *pool = vtkXMLDataElement::New();
  pool->SetName(VTK_XML_UTILITIES_FACTORED_POOL_NAME);
  tree->AddNestedElement(pool);

  // Factor the tree, as long as some factorization has occured
  // (multiple pass might be needed because larger trees are factored
  // first)

  while (vtkXMLUtilities::FactorElementsInternal(tree, tree, pool)) {};

  // Nothing factored, remove the useless pool

  if (!pool->GetNumberOfNestedElements())
    {
    tree->RemoveNestedElement(pool);
    }

  pool->Delete();
}

//----------------------------------------------------------------------------
int vtkXMLUtilities::FactorElementsInternal(vtkXMLDataElement *tree,
                                            vtkXMLDataElement *root,
                                            vtkXMLDataElement *pool)
{
  if (!tree || !root || !pool)
    {
    return 0;
    }

  // Do not bother factoring something already factored

  if (tree->GetName() && 
      !strcmp(tree->GetName(), VTK_XML_UTILITIES_FACTORED_REF_NAME))
    {
    return 0;
    }

  // Try to find all trees similar to the current tree

  vtkXMLDataElement **similar_trees;
  int nb_of_similar_trees = vtkXMLUtilities::FindSimilarElements(
    tree, root, &similar_trees);

  // None was found, try to factor the sub-trees

  if (!nb_of_similar_trees)
    {
    int res = 0;
    for (int i = 0; i < tree->GetNumberOfNestedElements(); i++)
      {
      res += vtkXMLUtilities::FactorElementsInternal(
        tree->GetNestedElement(i), root, pool);
      }
    return res ? 1 : 0;
    }

  // Otherwise replace those trees with factored refs

  char buffer[5];
  sprintf(buffer, "%02d_", pool->GetNumberOfNestedElements());

  ostrstream id;
  id << buffer << tree->GetName() << ends;
    
  vtkXMLDataElement *factored = vtkXMLDataElement::New();
  factored->SetName(VTK_XML_UTILITIES_FACTORED_NAME);
  factored->SetAttribute("Id", id.str());
  pool->AddNestedElement(factored);
  factored->Delete();

  vtkXMLDataElement *tree_copy = vtkXMLDataElement::New();
  tree_copy->DeepCopy(tree);
  factored->AddNestedElement(tree_copy);
  tree_copy->Delete();
    
  for (int i = 0; i < nb_of_similar_trees; i++)
    {
    similar_trees[i]->RemoveAllAttributes();
    similar_trees[i]->RemoveAllNestedElements();
    similar_trees[i]->SetName(VTK_XML_UTILITIES_FACTORED_REF_NAME);
    similar_trees[i]->SetAttribute("Id", id.str());
    }

  tree->RemoveAllAttributes();
  tree->RemoveAllNestedElements();
  tree->SetName(VTK_XML_UTILITIES_FACTORED_REF_NAME);
  tree->SetAttribute("Id", id.str());
    
  id.rdbuf()->freeze(0);

  delete [] similar_trees;

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLUtilities::UnFactorElements(vtkXMLDataElement *tree)
{
  if (!tree)
    {
    return;
    }

  // Search for the factored pool, if not found, we are done

  vtkXMLDataElement *pool = tree->FindNestedElementWithName(
    VTK_XML_UTILITIES_FACTORED_POOL_NAME);
  if (!pool)
    {
    return;
    }

  // Remove the pool from the tree, because it makes no sense
  // unfactoring it too

  pool->Register(tree);
  tree->RemoveNestedElement(pool);

  // Unfactor the tree

  vtkXMLUtilities::UnFactorElementsInternal(tree, pool);

  // Remove the useless empty pool

  pool->UnRegister(tree);
}

//----------------------------------------------------------------------------
int vtkXMLUtilities::UnFactorElementsInternal(vtkXMLDataElement *tree,
                                              vtkXMLDataElement *pool)
{
  if (!tree || !pool)
    {
    return 0;
    }

  int res = 0;

  // We found a factor, replace it with the corresponding sub-tree

  if (tree->GetName() &&
      !strcmp(tree->GetName(), VTK_XML_UTILITIES_FACTORED_REF_NAME))
    {
    vtkXMLDataElement *original_tree = 
      pool->FindNestedElementWithNameAndAttribute(
        VTK_XML_UTILITIES_FACTORED_NAME, "Id", tree->GetAttribute("Id"));
    if (original_tree && original_tree->GetNumberOfNestedElements())
      {
      tree->DeepCopy(original_tree->GetNestedElement(0));
      res++;
      }
    }

  // Now try to unfactor the sub-trees

  for (int i = 0; i < tree->GetNumberOfNestedElements(); i++)
    {
    res += vtkXMLUtilities::UnFactorElementsInternal(
      tree->GetNestedElement(i), pool);
    }

  return res ? 1 : 0;
}

