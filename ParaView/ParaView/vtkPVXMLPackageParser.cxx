/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXMLPackageParser.cxx
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
#include "vtkPVXMLPackageParser.h"

#include "vtkArrayMap.txx"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVCameraManipulator.h"
#include "vtkPVReaderModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVWidget.h"
#include "vtkPVWindow.h"
#include "vtkPVWriter.h"
#include "vtkPVXMLElement.h"
#include "vtkParaViewInstantiator.h"

#include <ctype.h>

vtkCxxRevisionMacro(vtkPVXMLPackageParser, "1.8");
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
  tname.rdbuf()->freeze(0);
  return pvWidget;
}

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVXMLPackageParser::GetPVWidget(vtkPVXMLElement* element)
{
  vtkPVWidget* pvWidget = 0;
  // Check if the widget has alread been created.
  if(this->WidgetMap->GetItem(element, pvWidget) != VTK_OK)
    {
    // If not, create one.
    pvWidget = this->CreatePVWidget(element);
    if(!pvWidget) { return 0; }
    
    // Add it to the map.
    this->WidgetMap->SetItem(element, pvWidget);
    
    // Now initialize it.  Must be done after adding to map to avoid
    // loops on circular references.
    if(!pvWidget->ReadXMLAttributes(element, this))
      {
      pvWidget->Delete();
      pvWidget = 0;
      this->WidgetMap->SetItem(element, pvWidget);
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
      if(!this->LoadLibrary(element))
        {
        return;
        }
      }
    else
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
  
  // Setup the filter's input type.
  const char* input = me->GetAttribute("input");
  if(!input)
    {
    vtkErrorMacro("Filter module missing input attribute.");
    return;
    }
  pvm->SetInputClassName(input);

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
  pvm->Delete();
}

//----------------------------------------------------------------------------
int vtkPVXMLPackageParser::CreateModule(vtkPVXMLElement* me, vtkPVSource* pvm)
{
  pvm->SetApplication(this->Window->GetPVApplication());
  pvm->SetParametersParent(this->Window->GetMainView()->GetPropertiesParent());
  
  const char* root_name = me->GetAttribute("root_name");
  if(root_name) { pvm->SetName(root_name); }
  else
    {
    vtkErrorMacro("Module missing root_name attribute.");
    return 0;
    }

  const char* name = me->GetAttribute("name");
  const char* button_image = me->GetAttribute("button_image");
  if(name && button_image) 
    {
    const char* button_image_file = me->GetAttribute("button_image_file");

    const char* button_help = me->GetAttribute("button_help");
    ostrstream command;
    command << "CreatePVSource " << name << ends;
    this->Window->AddToolbarButton(name, button_image, button_image_file,
                                   command.str(), button_help);
    command.rdbuf()->freeze(0);
    }

  const char* multiprocess_support = me->GetAttribute("multiprocess_support");
  if(multiprocess_support) 
    { 
    if (strcmp(multiprocess_support, "single_process") == 0)
      {
#ifdef VTK_USE_MPI
      vtkMultiProcessController* controller =
        this->Window->GetPVApplication()->GetController();
      if (controller && controller->GetNumberOfProcesses() > 1)
        {
        return 0;
        }
#endif
      }
    else if (strcmp(multiprocess_support, "multiple_processes") == 0)
      {
#ifdef VTK_USE_MPI
      vtkMultiProcessController* controller =
        this->Window->GetPVApplication()->GetController();
      if (!controller)
        {
        return 0;
        }
      if (controller->GetNumberOfProcesses() == 1)
        {
        return 0;
        }
#else
      return 0;
#endif
      }
    else if (strcmp(multiprocess_support, "both") != 0)
      {
      vtkErrorMacro("Unrecognized multiprocess_support attribute value: "
                    << multiprocess_support << ".");
      }
    }
  
  const char* output = me->GetAttribute("output");
  if(output) { pvm->SetOutputClassName(output); }
  else
    {
    vtkErrorMacro("Module missing output attribute.");
    return 0;
    }
  
  // Loop over the elements describing the module.
  unsigned int i;
  for(i=0; i < me->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* element = me->GetNestedElement(i);
    const char* name = element->GetName();
    if((strcmp(name, "Source") == 0) || (strcmp(name, "Filter") == 0))
      {
      const char* type = element->GetAttribute("type");
      if(type) { pvm->SetSourceClassName(type); }
      else
        {
        vtkErrorMacro("Filter missing type.");
        return 0;
        }
      }
    else
      {
      // Assume it is a widget.
      vtkPVWidget* widget = this->GetPVWidget(element);
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
int vtkPVXMLPackageParser::LoadLibrary(vtkPVXMLElement* le)
{
  const char* name = le->GetAttribute("name");
  if(!name)
    {
    vtkErrorMacro("Library missing name attribute.");
    return 0;
    }
  
  // Let Tcl find the library.
  this->Window->GetPVApplication()->BroadcastScript(
    "::paraview::load_component %s", name
    );
  
  // Returns empty string if successful.
  const char* result =
    this->Window->GetPVApplication()->GetMainInterp()->result;
  if(strcmp(result, "") != 0)
    {
    vtkErrorMacro("Error loading Library component " << name);
    return 0;
    }
  this->Window->AddPackageName(name);
  
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
    vtkPVWidget* widget = this->GetPVWidget(element);
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

