#! /usr/bin/env python
#------------------------------------------------------------------------------
# THIS FILE HAS BEEN MODIFIED FROM THE ORIGINAL (freeze.py).
# THE MODIFICATIONS ENABLE FREEZING OF MODULES/PACKAGES WITHOUT IMPORTING THEM.
# THAT MAKES IT POSSIBLE TO FREEZE PARAVIEW MODULES WITHOUT IMPORTING THE
# CORRESPONDING C/C++ LIBRARIES.
#------------------------------------------------------------------------------

"""Freeze a Python script into a binary.

usage: freeze [options...] script [module]...

Options:
-p prefix:    This is the prefix used when you ran ``make install''
              in the Python build directory.
              (If you never ran this, freeze won't work.)
              The default is whatever sys.prefix evaluates to.
              It can also be the top directory of the Python source
              tree; then -P must point to the build tree.

-P exec_prefix: Like -p but this is the 'exec_prefix', used to
                install objects etc.  The default is whatever sys.exec_prefix
                evaluates to, or the -p argument if given.
                If -p points to the Python source tree, -P must point
                to the build tree, if different.

-e extension: A directory containing additional .o files that
              may be used to resolve modules.  This directory
              should also have a Setup file describing the .o files.
              On Windows, the name of a .INI file describing one
              or more extensions is passed.
              More than one -e option may be given.

-o dir:       Directory where the output files are created; default '.'.

-m:           Additional arguments are module names instead of filenames.

-a package=dir: Additional directories to be added to the package's
                __path__.  Used to simulate directories added by the
                package at runtime (eg, by OpenGL and win32com).
                More than one -a option may be given for each package.

-l file:      Pass the file to the linker (windows only)

-d:           Debugging mode for the module finder.

-q:           Make the module finder totally quiet.

-h:           Print this help message.

-x module     Exclude the specified module. It will still be imported
              by the frozen binary if it exists on the host system.

-X module     Like -x, except the module can never be imported by
              the frozen binary.

-E:           Freeze will fail if any modules can't be found (that
              were not excluded using -x or -X).

-i filename:  Include a file with additional command line options.  Used
              to prevent command lines growing beyond the capabilities of
              the shell/OS.  All arguments specified in filename
              are read and the -i option replaced with the parsed
              params (note - quoting args in this file is NOT supported)

-s subsystem: Specify the subsystem (For Windows only.);
              'console' (default), 'windows', 'service' or 'com_dll'

-w:           Toggle Windows (NT or 95) behavior.
              (For debugging only -- on a win32 platform, win32 behavior
              is automatic.)

-r prefix=f:  Replace path prefix.
              Replace prefix with f in the source path references
              contained in the resulting binary.

Arguments:

script:       The Python script to be executed by the resulting binary.

module ...:   Additional Python modules (referenced by pathname)
              that will be included in the resulting binary.  These
              may be .py or .pyc files.  If -m is specified, these are
              module names that are search in the path instead.
              If -p is specified, all packages and modules under that path will
              be frozen (without import dependency tracking).

NOTES:

In order to use freeze successfully, you must have built Python and
installed it ("make install").

The script should not use modules provided only as shared libraries;
if it does, the resulting binary is not self-contained.
"""


# Import standard modules

import modulefinder
import getopt
import os
import sys


# Import the freeze-private modules

import checkextensions
import makeconfig
import makefreeze
import makemakefile
import parsesetup
import bkfile


# Main program

def main():
    # overridable context
    prefix = None                       # settable with -p option
    exec_prefix = None                  # settable with -P option
    extensions = []
    exclude = []                        # settable with -x option
    addn_link = []      # settable with -l, but only honored under Windows.
    path = sys.path[:]
    modargs = 0
    debug = 1
    odir = ''
    win = sys.platform[:3] == 'win'
    replace_paths = []                  # settable with -r option
    error_if_any_missing = 0

    # default the exclude list for each platform
    if win: exclude = exclude + [
        'dos', 'dospath', 'mac', 'macpath', 'macfs', 'MACFS', 'posix',
        'os2', 'ce', 'riscos', 'riscosenviron', 'riscospath',
        ]

    fail_import = exclude[:]

    # output files
    frozen_c = 'frozen.c'
    config_c = 'config.c'
    target = 'a.out'                    # normally derived from script name
    makefile = 'Makefile'
    subsystem = 'console'

    # parse command line by first replacing any "-i" options with the
    # file contents.
    pos = 1
    while pos < len(sys.argv)-1:
        # last option can not be "-i", so this ensures "pos+1" is in range!
        if sys.argv[pos] == '-i':
            try:
                options = open(sys.argv[pos+1]).read().split()
            except IOError, why:
                usage("File name '%s' specified with the -i option "
                      "can not be read - %s" % (sys.argv[pos+1], why) )
            # Replace the '-i' and the filename with the read params.
            sys.argv[pos:pos+2] = options
            pos = pos + len(options) - 1 # Skip the name and the included args.
        pos = pos + 1

    # Now parse the command line with the extras inserted.
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'r:a:dEe:hmo:p:P:qs:wX:x:l:')
    except getopt.error, msg:
        usage('getopt error: ' + str(msg))

    # proces option arguments
    for o, a in opts:
        if o == '-h':
            print __doc__
            return
        if o == '-d':
            debug = debug + 1
        if o == '-e':
            extensions.append(a)
        if o == '-m':
            modargs = 1
        if o == '-o':
            odir = a
        if o == '-p':
            prefix = a
        if o == '-P':
            exec_prefix = a
        if o == '-q':
            debug = 0
        if o == '-w':
            win = not win
        if o == '-s':
            if not win:
                usage("-s subsystem option only on Windows")
            subsystem = a
        if o == '-x':
            exclude.append(a)
        if o == '-X':
            exclude.append(a)
            fail_import.append(a)
        if o == '-E':
            error_if_any_missing = 1
        if o == '-l':
            addn_link.append(a)
        if o == '-a':
            apply(modulefinder.AddPackagePath, tuple(a.split("=", 2)))
        if o == '-r':
            f,r = a.split("=", 2)
            replace_paths.append( (f,r) )

    # modules that are imported by the Python runtime
    implicits = []
    for module in ('site', 'warnings',):
        if module not in exclude:
            implicits.append(module)

    # default prefix and exec_prefix
    if not exec_prefix:
        if prefix:
            exec_prefix = prefix
        else:
            exec_prefix = sys.exec_prefix
    if not prefix:
        prefix = sys.prefix

    # determine whether -p points to the Python source tree
    ishome = os.path.exists(os.path.join(prefix, 'Python', 'ceval.c'))

    # locations derived from options
    version = sys.version[:3]
    if win:
        extensions_c = 'frozen_extensions.c'
    if ishome:
        print "(Using Python source directory)"
        binlib = exec_prefix
        incldir = os.path.join(prefix, 'Include')
        config_h_dir = exec_prefix
        config_c_in = os.path.join(prefix, 'Modules', 'config.c.in')
        frozenmain_c = os.path.join(prefix, 'Python', 'frozenmain.c')
        makefile_in = os.path.join(exec_prefix, 'Makefile')
        if win:
            frozendllmain_c = os.path.join(exec_prefix, 'Pc\\frozen_dllmain.c')
    else:
        # the directory we're looking for is different for different systems
        # so we look for a file that exists in that directory
        import fnmatch
        match = None
        searchlib = os.path.join(exec_prefix, 'lib', 'python%s' % version)
        for root, dirnames, filenames in os.walk(searchlib):
            for filename in fnmatch.filter(filenames, 'config.c.in'):
                match = root

        if not match:
            usage('could not find python lib directory')

        binlib = os.path.join(exec_prefix,
                              'lib', 'python%s' % version, match)
        incldir = os.path.join(prefix, 'include', 'python%s' % version)
        config_h_dir = os.path.join(exec_prefix, 'include',
                                    'python%s' % version)
        config_c_in = os.path.join(binlib, 'config.c.in')
        frozenmain_c = os.path.join(binlib, 'frozenmain.c')
        makefile_in = os.path.join(binlib, 'Makefile')
        frozendllmain_c = os.path.join(binlib, 'frozen_dllmain.c')
    supp_sources = []
    defines = []
    includes = ['-I' + incldir, '-I' + config_h_dir]

    # sanity check of directories and files
    check_dirs = [prefix, exec_prefix, binlib, incldir]
    if not win:
        # These are not directories on Windows.
        check_dirs = check_dirs + extensions
    for dir in check_dirs:
        if not os.path.exists(dir):
            usage('needed directory %s not found' % dir)
        if not os.path.isdir(dir):
            usage('%s: not a directory' % dir)
    if win:
        files = supp_sources + extensions # extensions are files on Windows.
    else:
        files = [config_c_in, makefile_in] + supp_sources
    for file in supp_sources:
        if not os.path.exists(file):
            usage('needed file %s not found' % file)
        if not os.path.isfile(file):
            usage('%s: not a plain file' % file)
    if not win:
        for dir in extensions:
            setup = os.path.join(dir, 'Setup')
            if not os.path.exists(setup):
                usage('needed file %s not found' % setup)
            if not os.path.isfile(setup):
                usage('%s: not a plain file' % setup)

    # check that enough arguments are passed
    if not args:
        usage('at least one filename argument required')

    # check that file arguments exist
    for arg in args:
        if arg == '-m' or arg == '-p':
            break
        # if user specified -m on the command line before _any_
        # file names, then nothing should be checked (as the
        # very first file should be a module name)
        if modargs:
            break
        if not os.path.exists(arg):
            usage('argument %s not found' % arg)
        if not os.path.isfile(arg):
            usage('%s: not a plain file' % arg)

    # process non-option arguments
    scriptfile = args[0]
    modules = args[1:]

    # derive target name from script name
    base = os.path.basename(scriptfile)
    base, ext = os.path.splitext(base)
    if base:
        if base != scriptfile:
            target = base
        else:
            target = base + '.bin'

    # handle -o option
    base_frozen_c = frozen_c
    base_config_c = config_c
    base_target = target
    if odir and not os.path.isdir(odir):
        try:
            os.mkdir(odir)
            print "Created output directory", odir
        except os.error, msg:
            usage('%s: mkdir failed (%s)' % (odir, str(msg)))
    base = ''
    if odir:
        base = os.path.join(odir, '')
        frozen_c = os.path.join(odir, frozen_c)
        config_c = os.path.join(odir, config_c)
        target = os.path.join(odir, target)
        makefile = os.path.join(odir, makefile)
        if win: extensions_c = os.path.join(odir, extensions_c)

    # Handle special entry point requirements
    # (on Windows, some frozen programs do not use __main__, but
    # import the module directly.  Eg, DLLs, Services, etc
    custom_entry_point = None  # Currently only used on Windows
    python_entry_is_main = 1   # Is the entry point called __main__?
    # handle -s option on Windows
    if win:
        import winmakemakefile
        try:
            custom_entry_point, python_entry_is_main = \
                winmakemakefile.get_custom_entry_point(subsystem)
        except ValueError, why:
            usage(why)


    # Actual work starts here...

    # collect all modules of the program
    dir = os.path.dirname(scriptfile)
    path[0] = dir
    mf = modulefinder.ModuleFinder(path, debug, exclude, replace_paths)

    if win and subsystem=='service':
        # If a Windows service, then add the "built-in" module.
        mod = mf.add_module("servicemanager")
        mod.__file__="dummy.pyd" # really built-in to the resulting EXE

    for mod in implicits:
        mf.import_hook(mod)

    _current = None
    directories = []
    for mod in modules:
        if mod == '-m':
            _current = mod
        elif mod == '-p':
            _current = mod
        elif _current == '-p':
            _current = None
            if not os.path.exists(mod):
                usage('needed directory %s not found' % mod)
            if not os.path.isdir(mod):
                usage('%s: not a directory' % mod)
            directories.append(mod)
        elif _current == '-m':
            _current = None
            if mod[-2:] == '.*':
                mf.import_hook(mod[:-2], None, ["*"])
            else:
                mf.import_hook(mod)
        else:
            _current = None
            mf.load_file(mod)

    # process directories and add modules/packages found under those directories
    # to the paths
    add_packages(mf, directories, exclude)

    # Add the main script as either __main__, or the actual module name.
    if python_entry_is_main:
        mf.run_script(scriptfile)
    else:
        mf.load_file(scriptfile)

    if debug > 0:
        mf.report()
        print
    dict = mf.modules

    if error_if_any_missing:
        missing = mf.any_missing()
        if missing:
            sys.exit("There are some missing modules: %r" % missing)



    # generate output for frozen modules
    files = makefreeze.makefreeze(base, dict, debug, custom_entry_point,
                                  fail_import)

    # look for unfrozen modules (builtin and of unknown origin)
    builtins = []
    unknown = []
    mods = dict.keys()
    mods.sort()
    for mod in mods:
        if dict[mod].__code__:
            continue
        if not dict[mod].__file__:
            builtins.append(mod)
        else:
            unknown.append(mod)

    # search for unknown modules in extensions directories (not on Windows)
    addfiles = []
    frozen_extensions = [] # Windows list of modules.
    if unknown or (not win and builtins):
        if not win:
            addfiles, addmods = \
                      checkextensions.checkextensions(unknown+builtins,
                                                      extensions)
            for mod in addmods:
                if mod in unknown:
                    unknown.remove(mod)
                    builtins.append(mod)
        else:
            # Do the windows thang...
            import checkextensions_win32
            # Get a list of CExtension instances, each describing a module
            # (including its source files)
            frozen_extensions = checkextensions_win32.checkextensions(
                unknown, extensions, prefix)
            for mod in frozen_extensions:
                unknown.remove(mod.name)

    # report unknown modules
    if unknown:
        sys.stderr.write('Warning: unknown modules remain: %s\n' %
                         ' '.join(unknown))

    # windows gets different treatment
    if win:
        # Taking a shortcut here...
        import winmakemakefile, checkextensions_win32
        checkextensions_win32.write_extension_table(extensions_c,
                                                    frozen_extensions)
        # Create a module definition for the bootstrap C code.
        xtras = [frozenmain_c, os.path.basename(frozen_c),
                 frozendllmain_c, os.path.basename(extensions_c)] + files
        maindefn = checkextensions_win32.CExtension( '__main__', xtras )
        frozen_extensions.append( maindefn )
        outfp = open(makefile, 'w')
        try:
            winmakemakefile.makemakefile(outfp,
                                         locals(),
                                         frozen_extensions,
                                         os.path.basename(target))
        finally:
            outfp.close()
        return

    # --- We skip this part since CMake-ified Python doesn't have config_c_in
    # file present and besides, we don't need the make files.
    ## generate config.c and Makefile
    #builtins.sort()
    #infp = open(config_c_in)
    #outfp = bkfile.open(config_c, 'w')
    #try:
    #    makeconfig.makeconfig(infp, outfp, builtins)
    #finally:
    #    outfp.close()
    #infp.close()

    #cflags = ['$(OPT)']
    #cppflags = defines + includes
    #libs = [os.path.join(binlib, 'libpython$(VERSION).a')]

    #somevars = {}
    #if os.path.exists(makefile_in):
    #    makevars = parsesetup.getmakevars(makefile_in)
    #    for key in makevars.keys():
    #        somevars[key] = makevars[key]

    #somevars['CFLAGS'] = ' '.join(cflags) # override
    #somevars['CPPFLAGS'] = ' '.join(cppflags) # override
    #files = [base_config_c, base_frozen_c] + \
    #        files + supp_sources +  addfiles + libs + \
    #        ['$(MODLIBS)', '$(LIBS)', '$(SYSLIBS)']

    #outfp = bkfile.open(makefile, 'w')
    #try:
    #    makemakefile.makemakefile(outfp, somevars, files, base_target)
    #finally:
    ##    outfp.close()

    # Done!

    #if odir:
    #    print 'Now run "make" in', odir,
    #    print 'to build the target:', base_target
    #else:
    #    print 'Now run "make" to build the target:', base_target


# Print usage message and exit

def usage(msg):
    sys.stdout = sys.stderr
    print "Error:", msg
    print "Use ``%s -h'' for help" % sys.argv[0]
    sys.exit(2)

def add_packages(mf, directories, exclude):
    # manually add all packages under the given directories.
    for dir in directories:
        for dirpath, dirnames, filenames in os.walk(dir):
            relpath = os.path.relpath(dirpath, dir)
            if relpath == ".":
                continue
            packagename = relpath.replace('/', '.')
            if packagename in exclude:
                dirnames[:] = []
                continue
            if not "__init__.py" in filenames:
                # skip pacakges without init.py for now.
                continue
            # process all files in this package.
            for filename in filenames:
                if os.path.splitext(filename)[-1] != ".py":
                    continue

                modulepath = None # this must be non-null for a package.
                pathname = os.path.join(dirpath, filename) 

                if filename == "__init__.py":
                    modulename = packagename 
                    modulepath = [pathname]
                else:
                    modulename = \
                        packagename + "." + os.path.splitext(filename)[0]
                mf.import_hook(modulename)
    return dict
main()
