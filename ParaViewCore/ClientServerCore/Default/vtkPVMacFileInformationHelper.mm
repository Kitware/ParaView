/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMacFileInformationHelper.mm

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMacFileInformationHelper.h"

#include "vtkObjectFactory.h"

#import <Foundation/Foundation.h>

#include <string>
#include <vector>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMacFileInformationHelper);

//-----------------------------------------------------------------------------
vtkPVMacFileInformationHelper::vtkPVMacFileInformationHelper()
{
}

//-----------------------------------------------------------------------------
vtkPVMacFileInformationHelper::~vtkPVMacFileInformationHelper()
{
}

//-----------------------------------------------------------------------------
std::string vtkPVMacFileInformationHelper::GetHomeDirectory()
{
  NSString* homeDirectory = NSHomeDirectory();
  return std::string([homeDirectory UTF8String]);
}

//-----------------------------------------------------------------------------
std::vector< vtkPVMacFileInformationHelper::NamePath > vtkPVMacFileInformationHelper::GetMountedVolumes()
{
  std::vector< vtkPVMacFileInformationHelper::NamePath > volumes;
  NSArray *urls = [[NSFileManager defaultManager]
                    mountedVolumeURLsIncludingResourceValuesForKeys:@[NSURLVolumeNameKey]
                    options:0];
  for (NSURL *url in urls)
    {
    NSString* path = [url path];
    std::string pathStr([path UTF8String]);
    NSString *name = nil;
    [url getResourceValue:&name forKey:NSURLLocalizedNameKey error:NULL];
    std::string nameStr([name UTF8String]);
    volumes.push_back(std::make_pair(nameStr, pathStr));
    }

  return volumes;
}

//-----------------------------------------------------------------------------
std::string vtkPVMacFileInformationHelper::GetBundleDirectory()
{
  NSBundle* bundle = [NSBundle mainBundle];
  NSString* path = [bundle bundlePath];
  std::string pathStr([path UTF8String]);

  return pathStr;
}

//-----------------------------------------------------------------------------
std::string vtkPVMacFileInformationHelper::GetDownloadsDirectory()
{
  std::string downloadsDirectory;
  NSArray* urls = [[NSFileManager defaultManager]
                    URLsForDirectory:NSDownloadsDirectory
                    inDomains:NSUserDomainMask];
  for (NSURL *url in urls)
    {
    NSString* path = [url path];
    downloadsDirectory = [path UTF8String];
    }

  return downloadsDirectory;
}

//-----------------------------------------------------------------------------
void vtkPVMacFileInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
