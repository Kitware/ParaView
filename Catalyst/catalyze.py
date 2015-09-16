#!/usr/bin/env python
#
# Scripted used to transform a ParaView source tree into a Catalyst source tree.
#
# Usage: python catalyze.py -r <paraview_repo> -i <input_dir> -o <output_dir>
#
#  paraview_repo - The ParaView repository to use as the source.
#  input_dir - The directory containing the manifest.json describing the
#              transformation and any replacement files.
#  output_dir - The output directory where the Catalyst source will be written.
#
import sys
import os
import os.path
import subprocess
import shutil
import fnmatch
from xml.etree import ElementTree as ET
import argparse
import json
import stat

# submodules to skip over ...
submodules_exclude = ['VTK']

def _get_argument_parser():
  parser = argparse.ArgumentParser()

  parser.add_argument('-r', dest='repo', action='store', default='',
                        help='the source repo, defaults to repo in which the script is contained')

  parser.add_argument('-i', dest='input_dirs', action='append',
                        help='(repeatable) the directory contain manifest.json'\
                        'and other resources; muliple input decks can be'\
                        'specified by repeating -i in the order of processing'\
                        'The directory contain manifest.json and other resources')
  parser.add_argument('-o', dest='output_dir', action='store',
                        help='the directory where the modified sources will be written')
  parser.add_argument('-t', dest='copy_tests', action='store_true',
                        help='also copy over the test folders of the editions')

  usage = "Usage: %prog [options]"

  return parser

def edition_name(dir):
  base = os.path.basename(dir)
  if not base:
    base = os.path.basename(os.path.dirname(dir))
  return base

def filter_proxies(fin, fout, proxies, all_proxies):
  root = ET.fromstring(fin.read())
  if not root.tag == 'ServerManagerConfiguration':
    raise RuntimeError('Invalid ParaView XML file input')
  new_tree = ET.Element('ServerManagerConfiguration')
  proxy_tags = (
    'CameraProxy',
    'ChartRepresentationProxy',
    'ComparativeViewProxy',
    'ContextViewProxy',
    'MultiSliceViewProxy',
    'NullProxy',
    'ParallelCoordinatesRepresentationProxy',
    'PlotMatrixViewProxy',
    'Proxy',
    'PVRepresentationProxy',
    'PSWriterProxy',
    'PWriterProxy',
    'PythonViewProxy',
    'RenderViewProxy',
    'RepresentationProxy',
    'SourceProxy',
    'SpreadSheetRepresentationProxy',
    'TimeKeeperProxy',
    'TransferFunctionProxy',
    'ViewLayoutProxy',
    'WriterProxy')
  def is_wanted(proxy):
    return proxy.tag in proxy_tags and \
           'name' in proxy.attrib and \
           proxy.attrib['name'] in proxies
  for group in root.iter('ProxyGroup'):
    new_proxies = filter(is_wanted, list(group))
    for proxy in new_proxies:
      removed_subproxies = []
      for subproxy in proxy.iter('SubProxy'):
        for p in subproxy.iter('Proxy'):
          # p.attrib doesn't have proxyname it
          # means the proxy definition is inline.
          if p.attrib.has_key('proxyname') and (p.attrib['proxyname'] not in all_proxies):
            removed_subproxies.append(p.attrib['name'])
            proxy.remove(subproxy)
            break
      for reptype in proxy.iter('RepresentationType'):
        if reptype.attrib['subproxy'] in removed_subproxies:
          proxy.remove(reptype)
    if new_proxies:
      new_group = ET.Element(group.tag, group.attrib)
      map(new_group.append, new_proxies)
      new_tree.append(new_group)

  fout.write(ET.tostring(new_tree))

def error(err):
  print >> sys.stderr, "Error: %s" % str(err)
  sys.exit(-1)

def copy_path(src, dest, exclude):
  try:
    if os.path.isdir(src):
      if not os.path.exists(dest):
        shutil.copytree(src, dest, ignore=shutil.ignore_patterns(*exclude))
    else:
      dest_parent_dir = os.path.dirname(dest)
      if not os.path.exists(dest_parent_dir):
        os.makedirs(dest_parent_dir)
      shutil.copyfile(src, dest)
  except (IOError, shutil.Error, os.error) as err:
    error(err)

def replace_paths(config, paths):
  for replace in paths:
    replace_with = os.path.join(config.current_input_dir, replace['path'])
    if os.path.isdir(replace_with):
      error('%s is a directory, only support replacing a file' % replace_with)
    output = os.path.join(config.output_dir, replace['path'])
    dest_parent_dir = os.path.dirname(output)
    if not os.path.exists(dest_parent_dir):
      os.makedirs(dest_parent_dir)
    if not os.path.exists(replace_with):
      error('%s doesn\'t exist' % replace_with)
    try:
      shutil.copyfile(replace_with, output)
    except shutil.Error as err:
      error(err)

def patch_path(config, path_entry):
  work_dir = config.output_dir

  if path_entry['path'].startswith('VTK/'):
    work_dir = os.path.join(work_dir, 'VTK')

  try:
    p = subprocess.Popen(['patch', '-p1'], cwd=work_dir, stdin=subprocess.PIPE)
    patch = '\n'.join(path_entry['patch'])
    p.stdin.write(patch+'\n')
    p.stdin.close()
    p.wait()
    if p.returncode != 0:
      error('Failed to apply patch for: %s' % path_entry['path'])
  except Exception as err:
    error(err)

def run_patches(config, path_entry):
  work_dir = config.output_dir
  editions = map(edition_name, config.input_dirs)

  try:
    for patch in path_entry['patches']:
      if 'if-edition' in patch and patch['if-edition'] not in editions:
        continue
      p = subprocess.Popen(['patch', '-p1'], cwd=work_dir, stdin=subprocess.PIPE)
      patch_path = os.path.join(config.current_input_dir, patch['path'])
      with open(patch_path) as patch_file:
        p.stdin.write(patch_file.read())
      p.stdin.close()
      p.wait()
      if p.returncode != 0:
        error('Failed to apply patch for: %s' % path_entry['path'])
  except Exception as err:
    error(err)

def include_paths(config, src_base, dest_base, paths):
  classes = []
  for inc in paths:
    if 'class' in inc:
      inc_src = os.path.join(src_base, '%s.h' % inc['class'])
      inc_des = os.path.join(dest_base, '%s.h' % inc['class'])
      copy_path(inc_src, inc_des, [])
      inc_src = os.path.join(src_base, '%s.cxx' % inc['class'])
      inc_des = os.path.join(dest_base, '%s.cxx' % inc['class'])
      copy_path(inc_src, inc_des, [])
      classes.append(inc['class'])
    else:
      inc_src = os.path.join(src_base, inc['path'])
      inc_des = os.path.join(dest_base, inc['path'])
      copy_path(inc_src, inc_des, [])
  return classes

def copy_paths(config, paths):
  try:
    for path_entry in paths:
      classes = []
      src = os.path.join(config.repo, path_entry['path'])
      dest = os.path.join(config.output_dir, path_entry['path'])
      dest_parent_dir = os.path.dirname(dest)
      if not os.path.exists(dest_parent_dir):
        os.makedirs(dest_parent_dir)

      exclude = []

      # exclude an paths listed.
      if 'exclude' in path_entry:
        exclude = map(lambda d: d['path'], path_entry['exclude'])

      # if we are replacing the file then don't bother copying
      if 'replace' in path_entry:
        exclude.append(path_entry['path'])

      if 'include' in path_entry:
        exclude.append('*')
        classes += include_paths(config, src, dest, path_entry['include'])

      # do the actual copy
      copy_path(src, dest, exclude)

      if 'replace' in path_entry:
        replace_paths(config, path_entry['replace'])

      if 'patch' in path_entry:
        patch_path(config, path_entry)

      if 'patches' in path_entry:
        run_patches(config, path_entry)

      if classes:
        ename = edition_name(config.current_input_dir)
        with open(os.path.join(dest, '%s.catalyst.cmake' % ename), 'w+') as fout:
          fout.write('list(APPEND Module_SRCS\n')
          for cls in classes:
            fout.write('  %s.cxx\n' % cls)
          fout.write('  )')

  except (IOError, os.error) as err:
    error(err)

def create_cmake_script(config, manifest_list):
  cs_modules = set()
  python_modules = set()

  # what modules to client/server wrap?

  for manifest in manifest_list:
    if manifest.has_key("modules"):
      modules = manifest["modules"]
      for module in modules:
        if 'cswrap' in module and module['cswrap']:
          cs_modules.add(module['name'])

        if 'pythonwrap' in module and module['pythonwrap']:
          python_modules.add(module['name'])

  # ClientServer wrap
  cmake_script='''#!/bin/bash
cmake="$( which cmake )"
case "$1" in
  --cmake=*)
    cmake="${1#--cmake=}"
    shift
    ;;
  *)
    ;;
esac

$cmake \\
  --no-warn-unused-cli \\
  -DPARAVIEW_CS_MODULES:STRING="%(cs_modules)s" \\
  -DVTK_WRAP_PYTHON_MODULES:STRING="%(python_modules)s" \\
''' % {
      'cs_modules': ';'.join(cs_modules),
      'python_modules': ';'.join(python_modules),
    }

  for key, value in cmake_cache(config, manifest_list).items():
    cmake_script += '  -D%s=%s \\\n' % (key, value)

  # add ParaView git describe so the build has the correct version
  try:
    version = subprocess.check_output(['git', 'describe'], cwd=config.repo)
  except subprocess.CalledProcessError as err:
    error(err)

  cmake_script+='  -DPARAVIEW_GIT_DESCRIBE="%s" \\\n' % version.strip()

  cmake_script += ' "$@"\n'

  file = os.path.join(config.output_dir, 'cmake.sh')

  try:
    with open(file, 'w') as fd:
      fd.write(cmake_script)

    os.chmod(file, stat.S_IEXEC | stat.S_IREAD | stat.S_IWRITE)
  except (IOError, os.error) as err:
    error(err)

def cmake_cache(config, manifest_list):
  cache_entries = { }
  for manifest in manifest_list:
    try:
      for entry in manifest['cmake']['cache']:
        # note manifest_list will be treated as an ordered list with newer
        # manifests replacing older values set for the same key.
        cache_entries['%s:%s' % (entry['name'], entry['type'])] = entry['value']
    except KeyError:
      pass
  return cache_entries

def process(config):

  editions = set()
  all_editions = set(map(edition_name, config.input_dirs))
  all_manifests = []
  all_proxies = []
  for input_dir in config.input_dirs:
    print "Processing ", input_dir
    with open(os.path.join(input_dir, 'manifest.json'), 'r') as fp:
      manifest = json.load(fp)
      config.current_input_dir  = input_dir
      editions.add(manifest['edition'])
      if manifest.has_key('requires'):
        required = set(manifest['requires'])
        diff = required.difference(editions)
        if len(diff):
          missing = ', '.join(list(diff))
          raise RuntimeError('Missing required editions: %s' % missing)
      if manifest.has_key('after'):
        after = set(manifest['after'])
        adiff = after.difference(editions)
        diff = adiff.intersection(all_editions)
        if len(diff):
          missing = ', '.join(list(diff))
          raise RuntimeError('Editions must come before %s: %s' % (manifest['edition'], missing))
      if manifest.has_key('paths'):
        copy_paths(config, manifest['paths'])
      if manifest.has_key('modules'):
        copy_paths(config, manifest['modules'])
      if manifest.has_key('proxies'):
        all_proxies.append(manifest['proxies'])

      all_manifests.append(manifest)

  proxy_map = {}
  _all_proxies = []
  for proxies in all_proxies:
    for proxy in proxies:
      path = proxy['path']
      if path not in proxy_map:
        proxy_map[path] = []
      proxy_map[path] += proxy['proxies']
      _all_proxies += proxy['proxies']
  all_proxies = set(_all_proxies)

  for proxy_file, proxies in proxy_map.items():
    input_path = os.path.join(config.repo, proxy_file)
    output_path = os.path.join(config.output_dir, proxy_file)
    output_dir = os.path.dirname(output_path)
    if not os.path.exists(output_dir):
      os.makedirs(output_dir)

    with open(input_path, 'r') as fin:
      with open(output_path, 'w+') as fout:
        filter_proxies(fin, fout, set(proxies), all_proxies)

  create_cmake_script(config, all_manifests)

def copyTestTrees(config):
  all_dirs = config.input_dirs
  repo = config.repo
  testingDst = os.path.join(config.output_dir, 'Testing')
  os.makedirs(testingDst)
  for input_dir in all_dirs:
    testingSrc = os.path.join(input_dir, 'Testing')
    if os.path.isdir(testingSrc):
        for f in os.listdir(testingSrc):
          print f
          src = os.path.join(testingSrc,f)
          dst = os.path.join(testingDst,f)
          copy_path(src,dst,[])

def main():
  parser = _get_argument_parser()
  config = parser.parse_args()

  if len(config.repo.strip()) == 0:
    try:
      path = subprocess.check_output(['git', 'rev-parse', '--show-toplevel'],
                                     cwd=os.path.dirname(os.path.abspath(__file__)))
    except subprocess.CalledProcessError as err:
      error(err)
  else:
    path = config.repo

  config.repo = path.strip()

  # cleanup output directory.
  if os.path.exists(os.path.join(config.output_dir, '.git')):
    error('Refusing to output into a git repository')
  shutil.rmtree(config.output_dir, ignore_errors=True)

  process(config)
  if (config.copy_tests):
    copyTestTrees(config)

if __name__ == '__main__':
  main()

