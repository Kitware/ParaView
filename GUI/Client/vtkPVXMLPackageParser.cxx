/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXMLPackageParser.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVXMLPackageParser.h"

#include "vtkArrayMap.txx"
#include "vtkKWDirectoryUtilities.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVCameraManipulator.h"
#include "vtkPVInputProperty.h"
#include "vtkPVProcessModule.h"
#include "vtkPVReaderModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVWidget.h"
#include "vtkPVWindow.h"
#include "vtkPVWriter.h"
#include "vtkPVXMLElement.h"
#include "vtkParaViewInstantiator.h"
#include "vtkSMApplication.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkStringList.h"

#include <ctype.h>

vtkCxxRevisionMacro(vtkPVXMLPackageParser, "1.46.2.1");
vtkStandardNewMacro(vtkPVXMLPackageParser);

#ifndef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
template class VTK_EXPORT vtkAbstractMap<vtkPVXMLElement*, vtkPVWidget*>;
template class VTK_EXPORT vtkArrayMap<vtkPVXMLElement*, vtkPVWidget*>;
#endif

//----------------------------------------------------------------------------
vtkPVXMLPackageParser::vtkPVXMLPackageParser()
{
  this->WidgetMap = InternalWidgetMap::New();
  this->Window = 0;
}

//----------------------------------------------------------------------------
vtkPVXMLPackageParser::~vtkPVXMLPackageParser()
{
  this->WidgetMap->Delete();
}

//----------------------------------------------------------------------------
void vtkPVXMLPackageParser::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVXMLPackageParser::CreatePVWidget(vtkPVXMLElement* element)
{
  // Create the widget with the instantiator.
  vtkObject* object = 0;
  ostrstream name;
  name << "vtkPV" << element->GetName() << ends;
  object = vtkInstantiator::CreateInstance(name.str());
  name.rdbuf()->freeze(0);

  // Make sure we got a widget.
  vtkPVWidget* pvWidget = vtkPVWidget::SafeDownCast(object);
  if(!pvWidget)
    {
    if(object) { object->Delete(); }
    vtkErrorMacro("Error creating " << element->GetName());
    return 0;
    }

  // Set the widget's trace name.  This is the reverse of the scoped
  // id.
  ostrstream tname;
  tname << "WidgetTrace";
  vtkPVXMLElement* e = element;
  while(e)
    {
    tname << "." << e->GetId();
    e = e->GetParent();
    }
  tname << ends;
  pvWidget->SetTraceName(tname.str());
  pvWidget->SetTraceNameState(vtkPVWidget::Default);
  tname.rdbuf()->freeze(0);
  return pvWidget;
}

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVXMLPackageParser::GetPVWidget(vtkPVXMLElement* element,
                                                vtkPVSource* pvm,
                                                int store)
{
  vtkPVWidget* pvWidget = 0;
  // Check if the widget has alread been created.
  if(this->WidgetMap->GetItem(element, pvWidget) != VTK_OK)
    {
    // If not, create one.
    pvWidget = this->CreatePVWidget(element);
    if(!pvWidget) { return 0; }

    // Needed for debugging
    pvWidget->SetPVSource(pvm);

    // Add it to the map.
    if ( store )
      {
      this->WidgetMap->SetItem(element, pvWidget);
      }

    // Now initialize it.  Must be done after adding to map to avoid
    // loops on circular references.
    if(!pvWidget->ReadXMLAttributes(element, this))
      {
      pvWidget->Delete();
      pvWidget = 0;
      if ( store )
        {
        this->WidgetMap->SetItem(element, pvWidget);
        }
      }
    }
  else
    {
    // Increment the reference count. This is necessary to make the
    // behavior same whether a widget is created or returned from the
    // map. Always call Delete() after getting the widget.
    pvWidget->Register(0);
    }
  return pvWidget;
}

//----------------------------------------------------------------------------
void vtkPVXMLPackageParser::StoreConfiguration(vtkPVWindow* window)
{
  this->Window = window;
  this->ProcessConfiguration();
  this->Window = 0;
}

//----------------------------------------------------------------------------
vtkPVWindow* vtkPVXMLPackageParser::GetPVWindow()
{
  return this->Window;
}

//----------------------------------------------------------------------------
void vtkPVXMLPackageParser::ProcessConfiguration()
{
  // Get the root element.
  vtkPVXMLElement* root = this->GetRootElement();
  if(!root)
    {
    vtkErrorMacro("Must parse a configuration before storing it.");
    return;
    }

  // Loop over the top-level elements.
  unsigned int i;
  // First server manager files
  for(i=0; i < root->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* element = root->GetNestedElement(i);
    const char* name = element->GetName();
    if(strcmp(name, "ServerManagerFile") == 0)
      {
      this->LoadServerManagerFile(element);
      }
    }
  // Instantiate new server manager prototypes
  // These are used by the modules
  vtkSMProxyManager* proxm = vtkSMObject::GetProxyManager();
  proxm->InstantiateGroupPrototypes("filters");

  for(i=0; i < root->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* element = root->GetNestedElement(i);
    const char* name = element->GetName();
    if(strcmp(name, "Module") == 0)
      {
      const char* module_type = element->GetAttribute("module_type");
      if(!module_type)
        {
        const char* module_name = element->GetAttribute("name");
        if(module_name)
          {
          vtkErrorMacro("Module " << module_name <<
                        " missing module_type attribute.");
          }
        else
          {
          vtkErrorMacro("Module missing module_type attribute.");
          }
        continue;
        }
      if(strcmp(module_type, "Reader") == 0)
        {
        this->CreateReaderModule(element);
        }
      else if(strcmp(module_type, "Source") == 0)
        {
        this->CreateSourceModule(element);
        }
      else if(strcmp(module_type, "Filter") == 0)
        {
        this->CreateFilterModule(element);
        }
      else 
        {
        vtkErrorMacro("Module with unknown module_type=\"" << module_type
                      << "\"");
        }
      }
    else if(strcmp(name, "Manipulator") == 0)
      {
      this->CreateManipulator(element);
      }
    else if(strcmp(name, "Writer") == 0)
      {
      this->CreateWriter(element);
      }
    else if(strcmp(name, "Library") == 0)
      {
      if(!this->LoadPackageLibrary(element))
        {
        return;
        }
      }
    else if(strcmp(name, "ServerManagerFile") != 0)
      {
      vtkWarningMacro("Ignoring unknown top-level element " << name);
      }
    }
}

//----------------------------------------------------------------------------
static int vtkPVXMLPackageParserIsSpace(char c)
{
  return isspace(c);
}

//----------------------------------------------------------------------------
void vtkPVXMLPackageParser::CreateReaderModule(vtkPVXMLElement* me)
{
  vtkPVReaderModule* pvm = 0;
  const char* className = me->GetAttribute("class");
  if(className)
    {
    vtkObject* object = vtkInstantiator::CreateInstance(className);
    pvm = vtkPVReaderModule::SafeDownCast(object);
    if(!pvm)
      {
      vtkErrorMacro("Cannot create Module class \"" << className << "\"");
      if(object)
        {
        object->Delete();
        }
      return;
      }
    }
  else
    {
    pvm = vtkPVReaderModule::New();
    }

  const char* extensions = me->GetAttribute("extensions");
  if(extensions)
    {
    const char* start = extensions;
    const char* end = 0;

    // Parse the space-separated list.
    while(*start)
      {
      while(*start && vtkPVXMLPackageParserIsSpace(*start)) { ++start; }
      end = start;
      while(*end && !vtkPVXMLPackageParserIsSpace(*end)) { ++end; }
      int length = end-start;
      if(length)
        {
        char* entry = new char[length+1];
        strncpy(entry, start, length);
        entry[length] = '\0';
        pvm->AddExtension(entry);
        delete [] entry;
        }
      start = end;
      }
    }
  else
    {
    vtkErrorMacro("Reader Module has no extensions attribute.");
    pvm->Delete();
    return;
    }

  const char* file_description = me->GetAttribute("file_description");
  if(!file_description)
    {
    vtkErrorMacro("Reader Module has no file_description attribute.");
    pvm->Delete();
    return;
    }

  // Setup the standard module parts.
  if(!this->CreateModule(me, pvm))
    {
    pvm->Delete();
    return;
    }

  // Add this reader for its extensions instead of as a prototype.
  int i;
  pvm->SetLabelNoTrace(file_description);
  for(i=0;i < pvm->GetNumberOfExtensions(); ++i)
    {
    this->Window->AddFileType(file_description, pvm->GetExtension(i), pvm);
    }

  pvm->Delete();
}

//----------------------------------------------------------------------------
void vtkPVXMLPackageParser::CreateSourceModule(vtkPVXMLElement* me)
{
  vtkPVSource* pvm = 0;
  const char* className = me->GetAttribute("class");
  if(className)
    {
    vtkObject* object = vtkInstantiator::CreateInstance(className);
    pvm = vtkPVSource::SafeDownCast(object);
    if(!pvm)
      {
      vtkErrorMacro("Cannot create Module class \"" << className << "\"");
      if(object)
        {
        object->Delete();
        }
      return;
      }
    }
  else
    {
    pvm = vtkPVSource::New();
    }

  // Get the name of the module.
  const char* name = me->GetAttribute("name");
  if(!name)
    {
    vtkErrorMacro("Module missing name attribute.");
    pvm->Delete();
    return;
    }

  // Setup the standard module parts.
  if(!this->CreateModule(me, pvm))
    {
    pvm->Delete();
    return;
    }

  // Add the source prototype.
  pvm->InitializePrototype();
  this->Window->AddPrototype(name, pvm);
  pvm->Delete();
}

//----------------------------------------------------------------------------
void vtkPVXMLPackageParser::CreateFilterModule(vtkPVXMLElement* me)
{
  vtkPVSource* pvm = 0;
  const char* className = me->GetAttribute("class");
  if(className)
    {
    vtkObject* object = vtkInstantiator::CreateInstance(className);
    pvm = vtkPVSource::SafeDownCast(object);
    if(!pvm)
      {
      vtkErrorMacro("Cannot create Module class \"" << className << "\"");
      if(object)
        {
        object->Delete();
        }
      return;
      }
    }
  else
    {
    pvm = vtkPVSource::New();
    }

  // Determines whether the input of this filter will remain
  // visible
  int replace_input;
  if(me->GetScalarAttribute("replace_input", &replace_input))
    {
    pvm->SetReplaceInput(replace_input);
    }

  // Get the name of the module.
  const char* name = me->GetAttribute("name");
  if(!name)
    {
    vtkErrorMacro("Module missing name attribute.");
    pvm->Delete();
    return;
    }

  // Setup the standard module parts.
  if(!this->CreateModule(me, pvm))
    {
    pvm->Delete();
    return;
    }

  // Add the source prototype.
  pvm->InitializePrototype();
  this->Window->AddPrototype(name, pvm);

  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  if (pm)
    {
    vtkSMSourceProxy* proxy = vtkSMSourceProxy::SafeDownCast(
      pm->GetProxy("filters_prototypes", name));
    if (proxy)
      {
      pvm->SetProxy(proxy);
      }
    }
  
  pvm->Delete();
}

//----------------------------------------------------------------------------
int vtkPVXMLPackageParser::CreateModule(vtkPVXMLElement* me, vtkPVSource* pvm)
{
  vtkPVApplication *pvApp = this->Window->GetPVApplication();
  pvm->SetApplication(pvApp);
  //law int fixme; // Move Source notbook into window.
  pvm->SetNotebook(this->Window->GetMainView()->GetSourceNotebook());
  const char* classAttr;

  const char* menu_name = me->GetAttribute("menu_name");
  if(menu_name) { pvm->SetMenuName(menu_name); }

  const char* root_name = me->GetAttribute("root_name");
  if(root_name) { pvm->SetName(root_name); }
  else
    {
    vtkErrorMacro("Module missing root_name attribute.");
    return 0;
    }

  const char* short_help = me->GetAttribute("short_help");
  if(short_help) { pvm->SetShortHelp(short_help); }

  const char* long_help = me->GetAttribute("long_help");
  if(long_help) { pvm->SetLongHelp(long_help); }

  const char* multiprocess_support = me->GetAttribute("multiprocess_support");
  if(multiprocess_support)
    {
    if (strcmp(multiprocess_support, "single_process") == 0)
      {
      pvm->SetVTKMultipleProcessFlag(0);
      }
    else if (strcmp(multiprocess_support, "multiple_processes") == 0)
      {
      pvm->SetVTKMultipleProcessFlag(1);
      }
    else if (strcmp(multiprocess_support, "both") != 0)
      {
      vtkErrorMacro("Unrecognized multiprocess_support attribute value: "
                    << multiprocess_support << ".");
      }
    }

  const char* name = me->GetAttribute("name");
  if (name)
    {
    pvm->SetModuleName(name);
    }

  const char* overide_autoaccept = me->GetAttribute("overide_autoaccept");
  int overide = 0;
  if (overide_autoaccept && atoi(overide_autoaccept))
    {
    overide = 1;
    }
  pvm->SetOverideAutoAccept(overide);


  const char* button_image = me->GetAttribute("button_image");
  if(name && button_image)
    {
    const char* button_image_file = me->GetAttribute("button_image_file");
    const char* button_help = me->GetAttribute("button_help");
    const char* button_visibility = me->GetAttribute("button_visibility");
    int vis = 1;
    if (button_visibility && ! atoi(button_visibility))
      {
      vis = 0;
      }
    ostrstream command;
    command << "CreatePVSource " << name << ends;
    this->Window->AddToolbarButton(name, button_image, button_image_file,
                                   command.str(), button_help, vis);
    command.rdbuf()->freeze(0);
    pvm->SetToolbarModule(1);
    }

  // Loop over the elements describing the module.
  unsigned int i;
  for(i=0; i < me->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* element = me->GetNestedElement(i);
    name = element->GetName();
    if(strcmp(name, "Source") == 0)
      {  // Item describing a VTK source.
      classAttr = element->GetAttribute("class");
      if (classAttr)
        {
        pvm->SetSourceClassName(classAttr);
        }
      else
        {
        vtkErrorMacro(<< "Source missing class ("
                      << (menu_name ? menu_name : "null") << ")");
        return 0;
        }
      }
    else if (strcmp(name, "Filter") == 0)
      { // Item describing a VTK filter and inputs.
      if (this->ParseVTKFilter(element, pvm) == 0)
        {
        return 0;
        }
      }
    else if (strcmp(name, "Documentation") == 0)
      {
      // Ignore this documentation element.
      }
    else
      {
      // Assume it is a widget.
      vtkPVWidget* widget = this->GetPVWidget(element, pvm, 1);
      if(widget)
        {
        pvm->AddPVWidget(widget);
        widget->Delete();
        }
      else
        {
        vtkErrorMacro("Error creating widget " << name);
        return 0;
        }
      }
    }

  return 1;
}



//----------------------------------------------------------------------------
// Parses information about a VTK source and inputs.
int vtkPVXMLPackageParser::ParseVTKFilter(vtkPVXMLElement* filterElement,
                                          vtkPVSource* pvm)
{
  vtkPVXMLElement* inputElement;
  const char* classAttr;
  const char* quantityAttr;

  classAttr = filterElement->GetAttribute("class");
  if (classAttr)
    {
    pvm->SetSourceClassName(classAttr);
    }
  else
    {
    vtkErrorMacro("Filter missing class.");
    return 0;
    }

  // Loop over inputs of filter.
  unsigned int filterIdx;
  for(filterIdx=0; filterIdx < filterElement->GetNumberOfNestedElements(); ++filterIdx)
    {
    inputElement = filterElement->GetNestedElement(filterIdx);
    const char* inputElementName = inputElement->GetName();
    if (strcmp(inputElementName, "Input") == 0)
      { // Item describing a VTK filter input.
      // Get name (used for set/add method) of input from attribute.
      const char* inputName = inputElement->GetAttribute("name");
      if (inputName == NULL)
        {
        vtkErrorMacro("Input missing name. " << classAttr);
        return 0;
        }

      // Get the class name for this input.
      const char* inputClass = inputElement->GetAttribute("class");
      if(!inputClass)
        {
        vtkErrorMacro("Input element missing input attribute. " << classAttr);
        return 0;
        }
      vtkPVInputProperty *prop = pvm->GetInputProperty(inputName);
      prop->SetType(inputClass);

      // Attribute that tells whether the filter uses AddInput.
      quantityAttr = inputElement->GetAttribute("quantity");
      if (quantityAttr)
        {
        if (strcmp(quantityAttr, "Multiple") == 0 ||
            strcmp(quantityAttr, "multiple") == 0)
          {
          pvm->SetVTKMultipleInputsFlag(1);
          }
        }

      // We allow only one input with quantity "Multiple".
      // Let the user know of a violation.
      if (pvm->GetVTKMultipleInputsFlag() &&
          pvm->GetNumberOfInputProperties() > 1)
        {
        vtkWarningMacro("Only one 'multiple' input is allowed. " << classAttr);
        return 0;
        }

      }
    else
      { // Only input elements inside filter element.
      vtkWarningMacro("UnKnown XML element (" << inputElementName
                      << ") in filter: " << classAttr);
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVXMLPackageParser::LoadPackageLibrary(vtkPVXMLElement* le)
{
  // Get the library name.
  const char* name = le->GetAttribute("name");
  if(!name)
    {
    vtkErrorMacro("Library missing name attribute.");
    return 0;
    }

  // Check if a directory is specified.
  const char* directory = le->GetAttribute("directory");

  // Load the module on the server nodes.
  vtkPVProcessModule* pm =
    this->Window->GetPVApplication()->GetProcessModule();
  if(!pm->LoadModule(name, directory))
    {
    vtkErrorMacro("Error loading Library component " << name);
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPVXMLPackageParser::LoadServerManagerFile(vtkPVXMLElement* le)
{
  // Get the library name.
  const char* name = le->GetAttribute("name");
  if(!name)
    {
    vtkErrorMacro("Library missing name attribute.");
    return 0;
    }

  // Check if a directory is specified.
  char* tmpDir = 0;
  const char* directory = le->GetAttribute("directory");
  if (!directory)
    {
    if (this->GetFileName())
      {
      tmpDir = new char[strlen(this->GetFileName())+1];
      }
    else
      {
      tmpDir = new char[2];
      }
    vtkKWDirectoryUtilities::GetFilenamePath(this->GetFileName(), tmpDir);
    directory = tmpDir;
    }

  // Load the module on the server nodes.
  vtkSMApplication* sma =
    this->Window->GetPVApplication()->GetSMApplication();
  if(!sma->ParseConfigurationFile(name, directory))
    {
    vtkErrorMacro("Error loading server manager configuraiton file: " << name);
    delete[] tmpDir;
    return 0;
    }

  sma->AddConfigurationFile(name, directory);

  delete[] tmpDir;
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVXMLPackageParser::CreateManipulator(vtkPVXMLElement* ma)
{
  // Get the name of the manipulator.
  const char* name = ma->GetAttribute("name");
  if(!name)
    {
    vtkErrorMacro("Manipulator missing name attribute.");
    return;
    }

  // Get the type of the manipulator.
  const char* types = ma->GetAttribute("types");
  if(!types)
    {
    vtkErrorMacro("Manipulator \"" << name << "\" missing types attribute.");
    return;
    }

  vtkPVCameraManipulator* pcm = 0;
  const char* className = ma->GetAttribute("class");
  if(className)
    {
    vtkObject* object = vtkInstantiator::CreateInstance(className);
    pcm = vtkPVCameraManipulator::SafeDownCast(object);
    if(!pcm)
      {
      vtkErrorMacro("Cannot create Manipulator class \"" << className << "\"");
      if(object)
        {
        object->Delete();
        }
      return;
      }
    }
  else
    {
    vtkErrorMacro("Manipulator \"" <<name<< "\" does not define a class name.");
    }

  // Add the source prototype.
  this->Window->AddManipulator(types, name, pcm);

  unsigned int i;
  for( i=0; i < ma->GetNumberOfNestedElements(); i++ )
    {
    vtkPVXMLElement* element = ma->GetNestedElement(i);
    const char* variable = element->GetAttribute("variable");
    vtkPVWidget* widget = this->GetPVWidget(element, 0, 0);
    if(widget && variable)
      {
      this->Window->AddManipulatorArgument(types, name, variable, widget);
      }
    else
      {
      vtkErrorMacro("Error creating widget " << name);
      }
    if ( widget )
      {
      widget->Delete();
      }
    }

  pcm->Delete();
}

//----------------------------------------------------------------------------
void vtkPVXMLPackageParser::CreateWriter(vtkPVXMLElement* we)
{
  vtkPVWriter* pwm = 0;
  const char* className = we->GetAttribute("class");
  if(className)
    {
    vtkObject* object = vtkInstantiator::CreateInstance(className);
    pwm = vtkPVWriter::SafeDownCast(object);
    if(!pwm)
      {
      vtkErrorMacro("Cannot create Writer class \"" << className << "\"");
      if(object)
        {
        object->Delete();
        }
      return;
      }
    }
  else
    {
    pwm = vtkPVWriter::New();
    }

  // Setup the writer's input type.
  const char* input = we->GetAttribute("input");
  if(!input)
    {
    vtkErrorMacro("Writer missing input attribute.");
    return;
    }
  pwm->SetInputClassName(input);

  // Setup the writer's type.
  const char* writer = we->GetAttribute("writer");
  if(!writer)
    {
    vtkErrorMacro("Writer missing writer attribute.");
    return;
    }
  pwm->SetWriterClassName(writer);

  // Setup the writer's file extension.
  const char* extension = we->GetAttribute("extension");
  if(!extension)
    {
    vtkErrorMacro("Writer missing extension attribute.");
    return;
    }
  pwm->SetExtension(extension);

  // Setup the writer's file description.
  const char* file_description = we->GetAttribute("file_description");
  if(!file_description)
    {
    vtkErrorMacro("Writer missing file_description attribute.");
    return;
    }
  pwm->SetDescription(file_description);

  // Check whether the writer is for parallel formats.
  const char* parallel = we->GetAttribute("parallel");
  if(parallel && (strcmp(parallel, "1") == 0))
    {
    pwm->SetParallel(1);
    }

  // Check for the data mode method attribute.
  const char* data_mode_method = we->GetAttribute("data_mode_method");
  if(data_mode_method)
    {
    pwm->SetDataModeMethod(data_mode_method);
    }

  // Add the writer.
  this->Window->AddFileWriter(pwm);

  pwm->Delete();
}

