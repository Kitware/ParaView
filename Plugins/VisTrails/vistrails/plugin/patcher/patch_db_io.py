
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
## Copyright (C) 2006, 2007, 2008 University of Utah. All rights reserved.
## Copyright (C) 2008, 2009 VisTrails, Inc. All rights reserved.
##
############################################################################
import array
import core.requirements
import db.services.io
import os
import os.path
import tempfile
import zipfile
from core.system import execute_cmdline
from db.versions import getVersionDAO, currentVersion
from db.services.io import save_vistrail_to_xml, open_vistrail_from_xml,\
    unzip_file


def new_save_vistrail_to_zip_xml(vistrail, filename):
    (file_, xmlfname) = tempfile.mkstemp(suffix='.xml')
    os.close(file_)
    save_vistrail_to_xml(vistrail,xmlfname)
    vt_fname = os.path.join(os.path.dirname(xmlfname), 'vistrail')
    if os.path.exists(vt_fname):
        os.remove(vt_fname)
    os.rename(xmlfname, vt_fname)
    zip_fnames = [vt_fname, ]

    # Save binary data
    (bin_file, bin_filename) = tempfile.mkstemp(suffix='.bin')
    os.close(bin_file)
    bfile = open(bin_filename, "wb")
    vistrail.binary_data.tofile(bfile)
    bfile.close()
    bin_fname = os.path.join(os.path.dirname(bin_filename), 'data')
    if os.path.exists(bin_fname):
        os.remove(bin_fname)
    os.rename(bin_filename, bin_fname)
    zip_fnames.append(bin_fname)
    
    if vistrail.log is not None and len(vistrail.log.workflow_execs) > 0:
        if vistrail.log_filename is None:
            (log_file, log_filename) = tempfile.mkstemp(suffix='.xml')
            os.close(log_file)
            log_file = open(log_filename, "wb")
        else:
            log_filename = vistrail.log_filename
            log_file = open(log_filename, 'ab')

        print "+++ ", log_filename
        print "*** ", log_file
        if not hasattr(log_file, "write"):
            print "no!!!"
        
        # append log to log_file
        for workflow_exec in vistrail.log.workflow_execs:
            daoList = getVersionDAO(currentVersion)
            daoList.save_to_xml(workflow_exec, log_file, {}, currentVersion)
        log_file.close()

        log_fname = os.path.join(os.path.dirname(log_filename), 'log')
        if os.path.exists(log_fname):
            os.remove(log_fname)
        os.rename(log_filename, log_fname)
        zip_fnames.append(log_fname)

    try:
        zf = zipfile.ZipFile(file=filename,mode='w',
                             allowZip64=True)
        # Add standard vistrails files
        for f in zip_fnames:
            zf.write(f,os.path.basename(f),zipfile.ZIP_DEFLATED)
        # Add saved files. Append indicator of file type because it needed
        # when extracting the zip on Windows
        for (f,b) in vistrail.saved_files:
            basename = os.path.join("vt_saves",os.path.basename(f))
            if b:
                basename += ".b"
            else:
                basename += ".a"
            zf.write(f, basename, zipfile.ZIP_DEFLATED)
        zf.close()
    except Exception, e:
        raise Exception('Error writing file!\nThe file may be invalid or you\nmay have insufficient permissions.')
        
    # Remove temporary files
    for f in zip_fnames:
        os.unlink(f)

    return vistrail


def new_open_vistrail_from_zip_xml(filename):
    """open_vistrail_from_zip_xml(filename) -> Vistrail
    Open a vistrail from a zip compressed format.
    It expects that the file inside archive has name vistrail

    """
    try:
        zf = zipfile.ZipFile(filename, 'r')
    except:
        raise Exception('Error opening file!\nThe file may be invalid or you\nmay have insufficient permissions.')

    # Unzip vistrail
    (file_, xmlfname) = tempfile.mkstemp(suffix='.xml')
    os.close(file_)
    file(xmlfname, 'w').write(zf.read('vistrail'))
    vistrail = open_vistrail_from_xml(xmlfname)
    os.unlink(xmlfname)
    
    # Unzip data file
    (bfile_, binfname) = tempfile.mkstemp()
    os.close(bfile_)
    file(binfname, 'wb').write(zf.read('data'))
    vistrail.binary_data = array.array('c')
    bsize = os.path.getsize(binfname)
    vt_bin_file = open(binfname, 'rb')
    vistrail.binary_data.fromfile(vt_bin_file, bsize)
    vt_bin_file.close()
    os.unlink(binfname)

    # Unzip save data
    namelist = zf.namelist()
    vistrail.saved_files = []
    for name in namelist:
        if name != "vistrail" and name != "data":
            # Filename has a '.b' or '.a' appended to it so we know
            # how to write it.  In Python 2.6 or later, we should just use
            # zipFile.extractAll instead of zipFile.read
            (basename,ext) = os.path.splitext(name)
            sfilename = core.system.temporary_save_file(os.path.basename(basename))
            if ext == ".b":
                file(sfilename, 'wb').write(zf.read(name))
            else:
                file(sfilename, 'w').write(zf.read(name))
            vistrail.saved_files.append((sfilename, ext))

    return vistrail


db.services.io.save_vistrail_to_zip_xml = new_save_vistrail_to_zip_xml
db.services.io.open_vistrail_from_zip_xml = new_open_vistrail_from_zip_xml
