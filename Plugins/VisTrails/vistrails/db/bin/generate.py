
############################################################################
##
## This file is part of the Vistrails ParaView Plugin.
##
## This file may be used under the terms of the GNU General Public
## License version 2.0 as published by the Free Software Foundation
## and appearing in the file LICENSE.GPL included in the packaging of
## this file.  Please review the following to ensure GNU General Public
## Licensing requirements will be met:
## http://www.opensource.org/licenses/gpl-2.0.php
##
## This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
## WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
############################################################################

############################################################################
##
## Copyright (C) 2009 VisTrails, Inc. All rights reserved.
##
############################################################################

############################################################################
##
## Copyright (C) 2008 VisTrails, Inc. All rights reserved.
##
############################################################################
"""auto-generates code given specs"""

import os
import sys
import shutil
import getopt
from parser import AutoGenParser
from auto_gen import AutoGen
from et_auto_gen import XMLAutoGen
from sql_auto_gen import SQLAutoGen

BASE_DIR = os.path.dirname(os.getcwd())

DOMAIN_INIT = """from auto_gen import *"""
PERSISTENCE_INIT = \
"""from xml.auto_gen import XMLDAOListBase
from sql.auto_gen import SQLDAOListBase

class DAOList(dict):
    def __init__(self):
        self['xml'] = XMLDAOListBase()
        self['sql'] = SQLDAOListBase()

"""
COPYRIGHT_NOTICE = \
"""############################################################################
##
## Copyright (C) 2006-2007 University of Utah. All rights reserved.
##
## This file is part of VisTrails.
##
## This file may be used under the terms of the GNU General Public
## License version 2.0 as published by the Free Software Foundation
## and appearing in the file LICENSE.GPL included in the packaging of
## this file.  Please review the following to ensure GNU General Public
## Licensing requirements will be met:
## http://www.opensource.org/licenses/gpl-license.php
##
## If you are unsure which license is appropriate for your use (for
## instance, you are interested in developing a commercial derivative
## of VisTrails), please contact us at vistrails@sci.utah.edu.
##
## This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
## WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
############################################################################

"""

def usage(usageDict):
    usageStr = ''
    unrequired = ''
    required = ''
    for (opt, info) in usageDict.iteritems():
        if info[1]:
            required += '-%s <%s> ' % (opt[0], info[2])
            usageStr += '    -%s <%s>  ' % (opt[0], info[2])
        else:
            if len(opt) > 1:
                unrequired += '[-%s <%s>] ' % (opt[0], info[2])
                usageStr += '    -%s <%s>  ' % (opt[0], info[2])
            else:
                unrequired += '[-%s] ' % opt[0]
                usageStr += '    -%s            ' % opt[0]
        usageStr += '%s\n' % info[0]
    print 'Usage: python generate.py %s%s\n%s' % (required, 
                                                             unrequired, 
                                                             usageStr)

def dirStructure(baseDir):
    dirs = {}
    dirs['base'] = baseDir
    dirs['specs'] = os.path.join(dirs['base'], 'specs')
    dirs['persistence'] = os.path.join(dirs['base'], 'persistence')
    dirs['domain'] = os.path.join(dirs['base'], 'domain')
    dirs['schemas'] = os.path.join(dirs['base'], 'schemas')
    dirs['xmlPersistence'] = os.path.join(dirs['persistence'], 'xml')
    dirs['sqlPersistence'] = os.path.join(dirs['persistence'], 'sql')
    dirs['xmlSchema'] = os.path.join(dirs['schemas'], 'xml')
    dirs['sqlSchema'] = os.path.join(dirs['schemas'], 'sql')
    return dirs

def makeAllDirs(dirs):
    for (name, dir) in dirs.iteritems():
        if not os.path.exists(dir):
            print "creating directory '%s'" % dir
            os.makedirs(dir)
        if not name in ['specs', 'schemas', 'xmlSchema', 'sqlSchema']:
            init_file = os.path.join(dir, '__init__.py')
            if not os.path.exists(init_file):
                print "creating file '%s'" % init_file
                f = open(init_file, 'w')
                f.write(COPYRIGHT_NOTICE)
                if name == 'domain':
                    f.write(DOMAIN_INIT)
                elif name == 'persistence':
                    f.write(PERSISTENCE_INIT)
                else:
                    f.write('pass')
                f.close()
                    
def main(argv=None):
    options = {}
    objects = None

    optionsUsage = {'a': ('generate all database information (-p -s -x)', 
                          False),
                    'b:': ('base directory', False, 'dir'),
                    'd:': ('versions directory', False, 'dir'),
                    'p': ('generate python domain classes', False),
                    's': ('generate sql schema and persistence classes', False),
                    'x': ('generate xml schema and persistence classes', False),
                    'v:': ('vistrail version tag', True, 'version'),
                    'm': ('make all directories', False),
                    'n': ('do not change current version', False)}

    optStr = ''.join(optionsUsage.keys())
    optKeys = optStr.replace(':','')
    for idx in xrange(len(optKeys)):
        options[optKeys[idx]] = False

    try:
  (optlist, args) = getopt.getopt(sys.argv[1:], optStr)
  for opt in optlist:
            if opt[1] is not None and opt[1] != '':
                options[opt[0][1:]] = opt[1]
            else:
                options[opt[0][1:]] = True
    except getopt.GetoptError:
        usage(optionsUsage)
        return

    if options['b']:
        baseDir = options['b']
    else:
  baseDir = BASE_DIR
    baseDirs = dirStructure(baseDir)

    if not options['v']:
        usage(optionsUsage)
        return

    version = options['v']
    versionName = 'v' + version.replace('.', '_')
    if options['d']:
        versionsDir = options['d']
    else:
        versionsDir = os.path.join(baseDir, 'versions')
    versionDir = os.path.join(versionsDir, versionName)
    versionDirs = dirStructure(versionDir)

    print baseDirs
    print versionDirs

    if options['m']:
        makeAllDirs(baseDirs)
        makeAllDirs(versionDirs)

    # copy specs to version
    for file in os.listdir(baseDirs['specs']):
        if file.endswith('.xml'):
            print 'copying %s' % file
            filename = os.path.join(baseDirs['specs'], file)
            toFile = os.path.join(versionDirs['specs'], file)
            shutil.copyfile(filename, toFile)

    if options['p'] or options['a']:
  # generate python domain objects
  print "generating python domain objects..."
  if objects is None:
      parser = AutoGenParser()
      objects = parser.parse(baseDirs['specs'])
  autoGen = AutoGen(objects)
  domainFile = os.path.join(versionDirs['domain'], 'auto_gen.py')
  f = open(domainFile, 'w')
        f.write(COPYRIGHT_NOTICE)
  f.write(autoGen.generatePythonCode())
  f.close()

        if not options['n']:
            domainFile = os.path.join(baseDirs['domain'], '__init__.py')
            f = open(domainFile, 'w')
            f.write(COPYRIGHT_NOTICE)
            f.write('from db.versions.%s.domain import *\n' % \
                        versionName)
            f.close()

    if options['x'] or options['a']:
  # generate xml schema and dao objects
  print "generating xml schema and dao objects..."
  if objects is None:
      parser = AutoGenParser()
      objects = parser.parse(baseDirs['specs'])
  xmlAutoGen = XMLAutoGen(objects)
  
#   schemaFile = os.path.join(versionDirs['xmlSchema'], 'workflow.xsd')
#   f = open(schemaFile, 'w')
#       f.write(COPYRIGHT_NOTICE)
#   f.write(xmlAutoGen.generateSchema('workflow'))
#   f.close()

  schemaFile = os.path.join(versionDirs['xmlSchema'], 'vistrail.xsd')
  f = open(schemaFile, 'w')
        f.write(COPYRIGHT_NOTICE)
  f.write(xmlAutoGen.generateSchema('vistrail'))
  f.close()

#   schemaFile = os.path.join(versionDirs['xmlSchema'], 'log.xsd')
#   f = open(schemaFile, 'w')
#       f.write(COPYRIGHT_NOTICE)
#   f.write(xmlAutoGen.generateSchema('log'))
#   f.close()

  daoFile = os.path.join(versionDirs['xmlPersistence'], 'auto_gen.py')
  f = open(daoFile, 'w')
        f.write(COPYRIGHT_NOTICE)
  f.write(xmlAutoGen.generateDAO(versionName))
  f.write(xmlAutoGen.generateDAOList())
  f.close()

#         if not options['n']:
#             domainFile = os.path.join(baseDirs['xmlPersistence'], 'auto_gen.py')
#             f = open(domainFile, 'w')
#             f.write(COPYRIGHT_NOTICE)
#             f.write('from db.versions.%s.persistence.xml.auto_gen import *\n' % \
#                         versionName)
#             f.close()


    if options['s'] or options['a']:
  # generate sql schema and dao objects
  print "generating sql schema and dao objects..."
  if objects is None:
      parser = AutoGenParser()
      objects = parser.parse(baseDirs['specs'])
  sqlAutoGen = SQLAutoGen(objects)
  
  schemaFile = os.path.join(versionDirs['sqlSchema'], 'vistrails.sql')
  f = open(schemaFile, 'w')
        f.write(COPYRIGHT_NOTICE)
  f.write(sqlAutoGen.generateSchema())
  f.close()
  
  schemaFile = os.path.join(versionDirs['sqlSchema'],'vistrails_drop.sql')
  f = open(schemaFile, 'w')
        f.write(COPYRIGHT_NOTICE)
  f.write(sqlAutoGen.generateDeleteSchema())
  f.close()

   daoFile = os.path.join(versionDirs['sqlPersistence'], 'auto_gen.py')
   f = open(daoFile, 'w')
        f.write(COPYRIGHT_NOTICE)
   f.write(sqlAutoGen.generateDAO(versionName))
  f.write(sqlAutoGen.generateDAOList())
  f.close()

#         if not options['n']:
#             domainFile = os.path.join(baseDirs['sqlPersistence'], 'auto_gen.py')
#             f = open(domainFile, 'w')
#             f.write(COPYRIGHT_NOTICE)
#             f.write('from db.versions.%s.persistence.sql.auto_gen import *\n' % \
#                         versionName)
#             f.close()

    if not options['n']:
        domainFile = os.path.join(baseDirs['persistence'], '__init__.py')
        f = open(domainFile, 'w')
        f.write(COPYRIGHT_NOTICE)
        f.write('from db.versions.%s.persistence import *\n' % \
                    versionName)
        f.close()
            
if __name__ == '__main__':
    main()
