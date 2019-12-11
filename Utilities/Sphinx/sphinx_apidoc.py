# -*- coding: utf-8 -*-
"""
    sphinx.apidoc **modified**
    ~~~~~~~~~~~~~~~~~~~~~~~~~~

    This is a modified version of apidoc.py provided by Sphinx. We have modified
    it to generate separate rst files for each of modules in ParaView e.g.
    paraview.simple, paraview.servermanager etc. The default behavior is that
    all these modules are documented on the same page corresponding to the
    package which is very hard to read, especially in case of ParaView.

    Following is the original header form the file.
    ~~~~~~~~~~~~~

    sphinx.apidoc

    Parses a directory tree looking for Python modules and packages and creates
    ReST files appropriately to create code documentation with Sphinx.  It also
    creates a modules index (named modules.<suffix>).

    This is derived from the "sphinx-autopackage" script, which is:
    Copyright 2008 Société des arts technologiques (SAT),
    http://www.sat.qc.ca/

    :copyright: Copyright 2007-2011 by the Sphinx team, see AUTHORS.
    :license: BSD, see LICENSE for details.
"""
import os
import sys
import optparse
from os import path

# Custom module filter for ParaView
def is_exclude_module(module):
    excludeList = ['paraview/vtk/vtk','paraview/vtk/tk','paraview/vtk/gtk','paraview/vtk/test',
            'paraview/vtk/qt', 'paraview/vtk/wx', 'paraview/vtk/util',
            'paraview/_arg', 'paraview/compile_all', 'paraview/vtkConstants',
            'paraview/tpl',
            'paraview/modules']
    m = str(module)
    for exclude in excludeList:
        if m.__contains__(exclude):
            print("Exclude: %s" % m)
            return True
    return False

# automodule options
if 'SPHINX_APIDOC_OPTIONS' in os.environ:
    OPTIONS = os.environ['SPHINX_APIDOC_OPTIONS'].split(',')
else:
    OPTIONS = [
        'members',
        'undoc-members',
        # 'inherited-members', # disabled because there's a bug in sphinx
        'show-inheritance',
    ]

INITPY = '__init__.py'


def makename(package, module):
    """Join package and module with a dot."""
    # Both package and module can be None/empty.
    if package:
        name = package
        if module:
            name += '.' + module
    else:
        name = module
    return name


def write_file(name, text, opts):
    """Write the output file for module/package <name>."""
    fname = path.join(opts.destdir, '%s.%s' % (name, opts.suffix))
    if opts.dryrun:
        print('Would create file %s.' % fname)
        return
    if not opts.force and path.isfile(fname):
        print('File %s already exists, skipping.' % fname)
    else:
        print('Creating file %s.' % fname)
        with open(fname, 'wb') as f:
            if sys.version_info >= (3,):
                f.write(text.encode('UTF-8'))
            else:
                f.write(text)

def format_heading(level, text):
    """Create a heading of <level> [1, 2 or 3 supported]."""
    underlining = ['=', '-', '~', ][level-1] * len(text)
    return '%s\n%s\n\n' % (text, underlining)


def format_directive(module, package=None):
    """Create the automodule directive and add the options."""
    directive = '.. automodule:: %s\n' % makename(package, module)
    for option in OPTIONS:
        directive += '    :%s:\n' % option
    return directive

def format_simple_directive(module, proxy_list, package=None, main_module=True):
    """Create the automodule directive for the simple module."""
    if main_module:
        directive = '.. automodule:: %s\n' % makename(package, module)
        directive += '    :members:\n'
        directive += '    :exclude-members: %s\n' % ','.join(proxy_list)
        for option in OPTIONS:
            if option != 'members':
                directive += '    :%s:\n' % option
    else:
        directive = '.. currentmodule:: %s\n\n' %makename(package, module)
        # Adding a hidden toctree to prevent warnings about document not
        # included in any toc.
        # This is also required instead of :toctree: argument of autosummary
        # directive to prevent a clobbered top level index file.
        directive += '.. toctree::\n'
        directive += '    :hidden:\n\n'
        for prx in proxy_list:
            directive += '    %s.%s.%s\n' % (package, module, prx)
        directive += '\n.. autosummary::\n'
        directive += '    :nosignatures:\n\n'
        for prx in proxy_list:
            directive += '    %s\n' % prx
    return directive

def format_simple_proxy_directive(prx, package='paraview', module='simple'):
    """Create the autoclass directive for proxy"""
    directive = '.. currentmodule:: %s.%s\n\n' % (package, module)
    directive += '.. autofunction:: %s\n\n' % prx

    # Parse pydoc output for the proxy
    import parse_pydoc_output
    parser = parse_pydoc_output.ParsePyDocOutput('%s.%s' %(package, module),
        ['servermanager'], prx)
    directive += format_heading(2, 'Data Descriptors')
    directive += '%s\n\n' % parser.data_mems.replace('*', '\\*')
    inh_data_mems = parser.inh_data_mems
    for inh_data_mem in inh_data_mems:
        directive += format_heading(3, 'Data Descriptors inherited from %s'\
            % inh_data_mem)
        directive += '%s\n\n' % inh_data_mems[inh_data_mem].replace('*', '\\*')
    directive += format_heading(2, 'Methods')
    directive += '%s\n\n' % parser.method_mems.replace('*', '\\*')
    inh_method_mems = parser.inh_method_mems
    for inh_method_mem in inh_method_mems:
        directive += format_heading(3, 'Methods inherited from %s'\
            % inh_method_mem)
        directive += '%s\n\n' % inh_method_mems[inh_method_mem].replace('*', '\\*')
    return directive

def create_module_file(package, module, opts):
    """Build the text of the file and write the file."""
    text = format_heading(1, '%s Module' % module)
    #text += format_heading(2, ':mod:`%s` Module' % module)
    text += format_directive(module, package)
    write_file(makename(package, module), text, opts)

def create_simple_module_files(package, module, proxy, opts):
    """Build the text of the simple and proxy files and write them."""
    import paraview.simple
    gen_prx = sorted(paraview.simple._get_generated_proxies())

    # Create file for simple module
    text = format_heading(1, '%s Module' % module)
    text += "For generated server-side proxies, please refer to "\
            ":doc:`paraview.servermanager_proxies`\n\n"
    text += format_simple_directive(module, gen_prx, package)
    write_file(makename(package, module), text, opts)

    # Create a common summary page for all generated proxies
    text = format_heading(1, 'Available readers, sources, writers, filters and animation cues')
    text += "Proxies generated for server side objects "\
            "under :doc:`paraview.simple`\n\n"
    text += format_simple_directive(module, gen_prx, package, False)
    write_file(makename(package, proxy), text, opts)

    # Create a separate file for each proxy
    for prx in gen_prx:
        text = format_heading(1, '%s.%s.%s' % (package, module, prx))
        text += format_simple_proxy_directive(prx)
        text += '\n\nFor the full list of servermanager proxies, please refer to '\
                ':doc:`Available readers, sources, writers, filters and animation cues <paraview.servermanager_proxies>`'
        write_file(makename('%s.%s' % (package, module), prx), text, opts)

def create_package_file(root, master_package, subroot, py_files, opts, subs):
    """Build the text of the file and write the file."""
    # **modified**
    # This function has been modified from the original. We now create a
    # separate file for each module in the package, rather than adding it to the
    # same file.
    package = path.split(root)[-1]
    text = format_heading(1, '%s Package' % package)

    modules = []
    # add each module in the package
    for py_file in py_files:
        if shall_skip(path.join(root, py_file)):
            continue
        is_package = py_file == INITPY
        py_file = path.splitext(py_file)[0]
        py_path = makename(subroot, py_file)
        if is_package:
            heading = ':mod:`%s` Package' % package
            text += format_heading(2, heading)
            text += format_directive(is_package and subroot or py_path,
                                     master_package)
            text += '\n'
        else:
            if master_package == 'paraview' and py_path == 'simple':
                proxy_path = 'servermanager_proxies'
                create_simple_module_files(master_package, py_path, proxy_path,
                                           opts)
                modules.append(makename(master_package, py_path))
                modules.append(makename(master_package, proxy_path))
            else:
                # create a new file for the module.
                create_module_file(master_package, py_path, opts)
                modules.append(makename(master_package, py_path))

    if modules:
        text += format_heading(2, 'Modules')
        text += '.. toctree::\n\n'
        for module in modules:
            text += '    %s\n' % module
        text += '\n'

    # build a list of directories that are packages (contain an INITPY file)
    subs = [sub for sub in subs if path.isfile(path.join(root, sub, INITPY))]
    # if there are some package directories, add a TOC for theses subpackages
    if subs:
        text += format_heading(2, 'Subpackages')
        text += '.. toctree::\n\n'
        for sub in subs:
            text += '    %s.%s\n' % (makename(master_package, subroot), sub)
        text += '\n'

    write_file(makename(master_package, subroot), text, opts)


def create_modules_toc_file(modules, opts, name='modules'):
    """Create the module's index."""
    text = format_heading(1, '%s' % opts.header)
    text += '.. toctree::\n'
    text += '   :maxdepth: %s\n\n' % opts.maxdepth

    modules.sort()
    prev_module = ''
    for module in modules:
        # look if the module is a subpackage and, if yes, ignore it
        if module.startswith(prev_module + '.'):
            continue
        prev_module = module
        text += '   %s\n' % module

    write_file(name, text, opts)


def shall_skip(module):
    """Check if we want to skip this module."""
    # skip it if there is nothing (or just \n or \r\n) in the file
    if is_exclude_module(module):
        return True

    return path.getsize(module) <= 2


def recurse_tree(rootpath, excludes, opts):
    """
    Look for every file in the directory tree and create the corresponding
    ReST files.
    """
    # use absolute path for root, as relative paths like '../../foo' cause
    # 'if "/." in root ...' to filter out *all* modules otherwise
    rootpath = path.normpath(path.abspath(rootpath))
    # check if the base directory is a package and get its name
    if INITPY in os.listdir(rootpath):
        root_package = rootpath.split(path.sep)[-1]
    else:
        # otherwise, the base is a directory with packages
        root_package = None

    toplevels = []
    for root, subs, files in os.walk(rootpath):
        if is_excluded(root, excludes):
            del subs[:]
            continue
        # document only Python module files
        py_files = sorted([f for f in files if path.splitext(f)[1] == '.py'])
        is_pkg = INITPY in py_files
        if is_pkg:
            py_files.remove(INITPY)
            py_files.insert(0, INITPY)
        elif root != rootpath:
            # only accept non-package at toplevel
            del subs[:]
            continue
        # remove hidden ('.') and private ('_') directories
        subs[:] = sorted(sub for sub in subs if sub[0] not in ['.', '_'])

        if is_pkg:
            # we are in a package with something to document
            if subs or len(py_files) > 1 or not \
                shall_skip(path.join(root, INITPY)):
                subpackage = root[len(rootpath):].lstrip(path.sep).\
                    replace(path.sep, '.')
                create_package_file(root, root_package, subpackage,
                                    py_files, opts, subs)
                toplevels.append(makename(root_package, subpackage))
        else:
            # if we are at the root level, we don't require it to be a package
            assert root == rootpath and root_package is None
            for py_file in py_files:
                if not shall_skip(path.join(rootpath, py_file)):
                    module = path.splitext(py_file)[0]
                    create_module_file(root_package, module, opts)
                    toplevels.append(module)

    return toplevels


def normalize_excludes(rootpath, excludes):
    """
    Normalize the excluded directory list:
    * must be either an absolute path or start with rootpath,
    * otherwise it is joined with rootpath
    * with trailing slash
    """
    f_excludes = []
    for exclude in excludes:
        if not path.isabs(exclude) and not exclude.startswith(rootpath):
            exclude = path.join(rootpath, exclude)
        f_excludes.append(path.normpath(exclude) + path.sep)
    return f_excludes


def is_excluded(root, excludes):
    """
    Check if the directory is in the exclude list.

    Note: by having trailing slashes, we avoid common prefix issues, like
          e.g. an exlude "foo" also accidentally excluding "foobar".
    """
    sep = path.sep
    if not root.endswith(sep):
        root += sep
    for exclude in excludes:
        if root.startswith(exclude):
            return True
    return False


def main(argv=sys.argv):
    """
    Parse and check the command line arguments.
    """
    parser = optparse.OptionParser(
        usage="""\
usage: %prog [options] -o <output_path> <module_path> [exclude_paths, ...]

Look recursively in <module_path> for Python modules and packages and create
one reST file with automodule directives per package in the <output_path>.

Note: By default this script will not overwrite already created files.""")

    parser.add_option('-o', '--output-dir', action='store', dest='destdir',
                      help='Directory to place all output', default='')
    parser.add_option('-d', '--maxdepth', action='store', dest='maxdepth',
                      help='Maximum depth of submodules to show in the TOC '
                      '(default: 2)', type='int', default=2)
    parser.add_option('-f', '--force', action='store_true', dest='force',
                      help='Overwrite all files')
    parser.add_option('-n', '--dry-run', action='store_true', dest='dryrun',
                      help='Run the script without creating files')
    parser.add_option('-T', '--no-toc', action='store_true', dest='notoc',
                      help='Don\'t create a table of contents file')
    parser.add_option('-s', '--suffix', action='store', dest='suffix',
                      help='file suffix (default: rst)', default='rst')
    parser.add_option('-F', '--full', action='store_true', dest='full',
                      help='Generate a full project with sphinx-quickstart')
    parser.add_option('-H', '--doc-project', action='store', dest='header',
                      help='Project name (default: root module name)')
    parser.add_option('-A', '--doc-author', action='store', dest='author',
                      type='str',
                      help='Project author(s), used when --full is given')
    parser.add_option('-V', '--doc-version', action='store', dest='version',
                      help='Project version, used when --full is given')
    parser.add_option('-R', '--doc-release', action='store', dest='release',
                      help='Project release, used when --full is given, '
                      'defaults to --doc-version')

    (opts, args) = parser.parse_args(argv[1:])

    if not args:
        parser.error('A package path is required.')

    rootpath, excludes = args[0], args[1:]
    if not opts.destdir:
        parser.error('An output directory is required.')
    if opts.header is None:
        opts.header = path.normpath(rootpath).split(path.sep)[-1]
    if opts.suffix.startswith('.'):
        opts.suffix = opts.suffix[1:]
    if not path.isdir(rootpath):
        print(sys.stderr, '%s is not a directory.' % rootpath)
        sys.exit(1)
    if not path.isdir(opts.destdir):
        if not opts.dryrun:
            os.makedirs(opts.destdir)
    excludes = normalize_excludes(rootpath, excludes)
    modules = recurse_tree(rootpath, excludes, opts)
    print("Detected Packages:\n\t", "\n\t".join(modules))
    if opts.full:
        from sphinx import quickstart as qs
        modules.sort()
        prev_module = ''
        text = ''
        for module in modules:
            if module.startswith(prev_module + '.'):
                continue
            prev_module = module
            text += '   %s\n' % module
        d = dict(
            path = opts.destdir,
            sep  = False,
            dot  = '_',
            project = opts.header,
            author = opts.author or 'Author',
            version = opts.version or '',
            release = opts.release or opts.version or '',
            suffix = '.' + opts.suffix,
            master = 'index',
            epub = True,
            ext_autodoc = True,
            ext_viewcode = True,
            makefile = True,
            batchfile = True,
            mastertocmaxdepth = opts.maxdepth,
            mastertoctree = text,
        )
        if not opts.dryrun:
            qs.generate(d, silent=True, overwrite=opts.force)
    elif not opts.notoc:
        create_modules_toc_file(modules, opts)

if __name__ == "__main__":
    main()
