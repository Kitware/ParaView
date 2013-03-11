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
import subprocess
import shutil
import fnmatch
import argparse
import json
import stat

# submodules to skip over ...
submodules_exclude = ['VTK']

def _get_argument_parser():
  parser = argparse.ArgumentParser()

  parser.add_argument('-r', dest='repo', action='store', default='',
                        help='The source repo, defaults to repo in which the script is contained')

  parser.add_argument('-i', dest='input_dir', action='store',
                        help='The directory contain manifest.json and other resources')

  parser.add_argument('-o', dest='output_dir', action='store',
                        help='The directory where the modified sources will be written')

  usage = "usage: %prog [options]"

  return parser

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
        os.makedirs(dest_parent_dir);
      shutil.copyfile(src, dest)
  except (IOError, shutil.Error, os.error) as err:
    error(err)

def replace_paths(config, paths):
  for replace in paths:
    replace_with = config.input_dir + '/' + replace['path']
    if os.path.isdir(replace_with):
      error('%s is a directory, only support replacing a file' % replace_with)
    if not os.path.exists(replace_with):
      error('%s doesn\'t exist' % replace_with)
    try:
      shutil.copyfile(replace_with, config.output_dir + '/' + replace['path'])
    except shutil.Error as err:
      error(err)

def patch_path(config, path_entry):
  work_dir = config.output_dir

  if path_entry['path'].startswith('VTK/'):
    work_dir = work_dir + '/VTK'

  try:
    p = subprocess.Popen(['/usr/bin/patch', '-p1'], cwd=work_dir, stdin=subprocess.PIPE)
    patch = '\n'.join(path_entry['patch'])
    p.stdin.write(patch+'\n')
    p.stdin.close();
    p.wait()
    if p.returncode != 0:
      error('Failed to apply patch for: %s' % path_entry['path'])
  except Exception as err:
    error(err)

def include_paths(config, src_base, dest_base, paths):
  for inc in paths:
    inc_src = src_base + '/' + inc['path']
    inc_des = dest_base + '/' + inc['path']
    copy_path(inc_src, inc_des, [])

def copy_paths(config, paths):
  try:
    for path_entry in paths:
      src = config.repo + '/' + path_entry['path']
      dest = config.output_dir + '/' + path_entry['path']
      dest_parent_dir = os.path.dirname(dest)
      if not os.path.exists(dest_parent_dir):
        os.makedirs(dest_parent_dir);

      exclude = []

      # exclude an paths listed.
      if 'exclude' in path_entry:
        exclude = map(lambda d: d['path'], path_entry['exclude']);

      # if we are replacing the file then don't bother copying
      if 'replace' in path_entry:
        exclude.append(path_entry['path'])

      if 'include' in path_entry:
        exclude.append('*')
        include_paths(config, src, dest, path_entry['include'])

      # do the actual copy
      copy_path(src, dest, exclude)

      if 'replace' in path_entry:
        replace_paths(config, path_entry['replace'])

      if 'patch' in path_entry:
        patch_path(config, path_entry)

  except (IOError, os.error) as err:
    error(err)

def create_cmake_script(config, manifest):
  cmake_script='#!/bin/bash\n';
  modules = manifest['modules']
  cs_modules = []
  python_modules = []

  last = modules[len(modules)-1]
  # what modules to client/server wrap?

  for module in modules:
    if 'cswrap' in module and module['cswrap']:
      cs_modules.append(module['name'])

    if 'pythonwrap' in module and module['pythonwrap']:
      python_modules.append(module['name'])

  # ClientServer wrap
  cmake_script += 'cmake \\\n'
  cmake_script += '  -DPARAVIEW_CS_MODULES:STRING="%s" \\\n' % (';'.join(cs_modules))
  # Python modules
  cmake_script+='  -DVTK_WRAP_PYTHON_MODULES:STRING="%s" \\\n' % (';'.join(python_modules))

  for key, value in cmake_cache(config, manifest).items():
    cmake_script += '  -D%s=%s \\\n' % (key, value)

  # add ParaView git describe so the build has the correct version
  try:
    version = subprocess.check_output(['git', 'describe'], cwd=config.repo)
  except subprocess.CalledProcessError as err:
    error(err)

  cmake_script+='  -DPARAVIEW_GIT_DESCRIBE="%s" \\\n' % version.strip()

  cmake_script += ' $@\n'

  file = config.output_dir + '/cmake.sh'

  try:
    with open(file, 'w') as fd:
      fd.write(cmake_script)

    os.chmod(file, stat.S_IEXEC | stat.S_IREAD | stat.S_IWRITE)
  except (IOError, os.error) as err:
    error(err)

def cmake_cache(config, manifest):
  cache_entries = { }
  for entry in manifest['cmake']['cache']:
    cache_entries['%s:%s' % (entry['name'], entry['type'])] = entry['value']

  return cache_entries

def process(config):

  with open(config.input_dir + '/manifest.json' , 'r') as fp:
    manifest = json.load(fp)

  copy_paths(config, manifest['paths'])
  copy_paths(config, manifest['modules'])

  create_cmake_script(config, manifest)

def main():
  parser = _get_argument_parser()
  config = parser.parse_args()

  if len(config.repo.strip()) == 0:
    try:
      path = subprocess.check_output(['git', 'rev-parse', '--show-toplevel'], cwd=config.input_dir)
    except subprocess.CalledProcessError as err:
      error(err)
  else:
    path = config.repo

  config.repo = path.strip()

  process(config)

if __name__ == '__main__':
  main()

