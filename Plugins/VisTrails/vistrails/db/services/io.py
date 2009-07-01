
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
##
############################################################################

from datetime import datetime
from core.system import get_elementtree_library, temporary_directory,\
     execute_cmdline
import core.requirements
ElementTree = get_elementtree_library()

import sys
import os
import os.path
import tempfile

from db import VistrailsDBException
from db.domain import DBVistrail, DBWorkflow, DBLog, DBAbstraction, DBGroup
import db.services.abstraction
import db.services.log
import db.services.workflow
import db.services.vistrail
from db.versions import getVersionDAO, currentVersion, translateVistrail, \
    getVersionSchemaDir

def open_db_connection(config):
    import MySQLdb

    if config is None:
        msg = "You need to provide valid config dictionary"
        raise Exception(msg)
    try:
        db_connection = MySQLdb.connect(**config)
        return db_connection
    except MySQLdb.Error, e:
        # should have a DB exception type
        msg = "cannot open connection (%d: %s)" % (e.args[0], e.args[1])
        raise VistrailsDBException(msg)

def close_db_connection(db_connection):
    if db_connection is not None:
        db_connection.close()

def test_db_connection(config):
    """testDBConnection(config: dict) -> None
    Tests a connection raising an exception in case of error.
    
    """
    import MySQLdb

    try:
        db_connection = MySQLdb.connect(**config)
        close_db_connection(db_connection)
    except MySQLdb.Error, e:
        msg = "connection test failed (%d: %s)" % (e.args[0], e.args[1])
        raise VistrailsDBException(msg)

def translate_to_tbl_name(obj_type):
    map = {DBVistrail.vtType: 'vistrail',
           DBWorkflow.vtType: 'workflow',
           DBLog.vtType: 'log_tbl',
           DBAbstraction.vtType: 'abstraction',
           }
    return map[obj_type]

def date_to_str(date):
    return date.strftime('%Y-%m-%d %H:%M:%S')

def get_db_object_list(config, obj_type):
    
    import MySQLdb

    result = []    
    db = open_db_connection(config)

    #FIXME Create a DBGetVistrailListSQLDAOBase for this
    # and maybe there's another way to build this query
    command = """SELECT o.id, o.name, o.last_modified
    FROM %s o
    ORDER BY o.name
    """
#     command = """SELECT o.id, v.name, a.date, a.user
#     FROM %s o, action a,
#     (SELECT a.entity_id, MAX(a.date) as recent, a.user
#     FROM action a
#     GROUP BY entity_id) latest
#     WHERE o.id = latest.entity_id 
#     AND a.entity_id = o.id
#     AND a.date = latest.recent 
#     """ % obj_type

    try:
        c = db.cursor()
        c.execute(command % translate_to_tbl_name(obj_type))
        rows = c.fetchall()
        result = rows
        c.close()
        
    except MySQLdb.Error, e:
        msg = "Couldn't get list of vistrails objects from db (%d : %s)" % \
            (e.args[0], e.args[1])
        raise VistrailsDBException(msg)
    return result

def get_db_object_modification_time(db_connection, obj_id, obj_type):
    import MySQLdb

    command = """
    SELECT o.last_modified
    FROM %s o
    WHERE o.id = %s
    """

    try:
        c = db_connection.cursor()
        c.execute(command % (translate_to_tbl_name(obj_type), obj_id))
        time = c.fetchall()[0][0]
        c.close()
    except MySQLdb.Error, e:
        msg = "Couldn't get object modification time from db (%d : %s)" % \
            (e.args[0], e.args[1])
        raise VistrailsDBException(msg)
    return time

def get_matching_abstraction_id(db_connection, abstraction):
    import MySQLdb

    last_action_id = -1
    last_action = None
    for action in abstraction.db_actions:
        if action.db_id > last_action_id:
            last_action_id = action.db_id
            last_action = action

    command = """
    SELECT g.id 
    FROM abstraction g, action a
    WHERE g.name = '%s'
    AND a.entity_type = 'abstraction'
    AND a.entity_id = g.id
    AND a.user = '%s'
    AND a.date = '%s'
    AND a.id = %s
    """
    
    id = None
    try:
        c = db_connection.cursor()
        c.execute(command % (abstraction.db_name,
                             last_action.db_user,
                             date_to_str(last_action.db_date),
                             last_action.db_id))
        result = c.fetchall()
        c.close()
        if len(result) > 0:
            print 'got result:', result
            id = result[0][0]
    except MySQLdb.Error, e:
        msg = "Couldn't get object modification time from db (%d : %s)" % \
            (e.args[0], e.args[1])
        raise VistrailsDBException(msg)
    return id

def setup_db_tables(db_connection, version=None):
    import MySQLdb

    schemaDir = getVersionSchemaDir(version)
    try:
        # delete tables
        c = db_connection.cursor()
        f = open(os.path.join(schemaDir, 'vistrails_drop.sql'))
        db_script = f.read()
        c.execute(db_script)
        c.close()
        f.close()

        # create tables        
        c = db_connection.cursor()
        f = open(os.path.join(schemaDir, 'vistrails.sql'))
        db_script = f.read()
        c.execute(db_script)
        f.close()
        c.close()
    except MySQLdb.Error, e:
        raise VistrailsDBException("unable to create tables: " + str(e))

##############################################################################
# General I/O

def open_from_xml(filename, type):
    if type == DBVistrail.vtType:
        return open_vistrail_from_xml(filename)
    elif type == DBWorkflow.vtType:
        return open_workflow_from_xml(filename)
    elif type == DBLog.vtType:
        return open_log_from_xml(filename)

def save_to_xml(obj, filename):
    if obj.vtType == DBVistrail.vtType:
        return save_vistrail_to_xml(obj, filename)
    elif obj.vtType == DBWorkflow.vtType:
        return save_workflow_to_xml(obj, filename)
    elif obj.vtType == DBLog.vtType:
        return save_log_to_xml(obj, filename)

def open_from_zip_xml(filename, type):
    if type == DBVistrail.vtType:
        return open_vistrail_from_zip_xml(filename)
    elif type == DBWorkflow.vtType:
        return open_workflow_from_zip_xml(filename)
    elif type == DBLog.vtType:
        return open_log_from_zip_xml(filename)

def save_to_zip_xml(obj, filename):
    if obj.vtType == DBVistrail.vtType:
        return save_vistrail_to_zip_xml(obj, filename)
    elif obj.vtType == DBWorkflow.vtType:
        return save_workflow_to_zip_xml(obj, filename)
    elif obj.vtType == DBLog.vtType:
        return save_log_to_zip_xml(obj, filename)

def open_from_db(db_connection, type, obj_id):
    if type == DBVistrail.vtType:
        return open_vistrail_from_db(db_connection, obj_id)
    elif type == DBWorkflow.vtType:
        return open_workflow_from_db(db_connection, obj_id)
    elif type == DBLog.vtType:
        return open_log_from_db(db_connection, obj_id)

def save_to_db(obj, db_connection, do_copy=False):
    if obj.vtType == DBVistrail.vtType:
        return save_vistrail_to_db(obj, db_connection, do_copy)
    elif obj.vtType == DBWorkflow.vtType:
        return save_workflow_to_db(obj, db_connection, do_copy)
    elif obj.vtType == DBLog.vtType:
        return save_log_to_db(obj, db_connection, do_copy)

def serialize(object):
    daoList = getVersionDAO(currentVersion)
    return daoList.serialize(object)

def unserialize(str, obj_type):
    daoList = getVersionDAO(currentVersion)
    return daoList.unserialize(str, obj_type)
 
##############################################################################
# Vistrail I/O

def open_vistrail_from_xml(filename):
    """open_vistrail_from_xml(filename) -> Vistrail"""
    tree = ElementTree.parse(filename)
    version = get_version_for_xml(tree.getroot())
    try:
        daoList = getVersionDAO(version)
        vistrail = daoList.open_from_xml(filename, DBVistrail.vtType)
        vistrail = translateVistrail(vistrail, version)
        db.services.vistrail.update_id_scope(vistrail)
    except VistrailsDBException, e:
        msg = "This vistrail was created by a newer version of VisTrails "
        msg += "and cannot be opened."
        raise VistrailsDBException(msg)
    return vistrail

def unzip_file(filename, name_in_archive):
    # name_in_archive = 'vistrail'
    (file_, xmlfname) = tempfile.mkstemp(suffix='.xml')
    core.requirements.require_executable('unzip')
    os.close(file_)
    output = []
    cmdline = ['unzip', '-p', filename, name_in_archive, '>', xmlfname]
    result = execute_cmdline(cmdline,output)

    # print result, output    
    if result == 0 or len(output) == 0:
        return xmlfname
    return None

def open_vistrail_from_zip_xml(filename):
    """open_vistrail_from_zip_xml(filename) -> Vistrail
    Open a vistrail from a zip compressed format.
    It expects that the file inside archive has name vistrail

    """

    vt_xml_fname = unzip_file(filename, 'vistrail')
    if vt_xml_fname:            
        vistrail = open_vistrail_from_xml(vt_xml_fname)
        os.unlink(vt_xml_fname)
    else:
        # raise Exception(" ".join(output))
        raise Exception("Cannot find vistrail in file '%s'" % filename)
    log_xml_fname = unzip_file(filename, 'log')
    if log_xml_fname:
        vistrail.log_filename = log_xml_fname

    return vistrail
            
def open_vistrail_from_db(db_connection, id, lock=False):
    """open_vistrail_from_db(db_connection, id : long, lock: bool) 
         -> DBVistrail 

    """
    if db_connection is None:
        msg = "Need to call open_db_connection() before reading"
        raise Exception(msg)

    # method because we use recursion for nested abstractions
    def load_abstractions(vistrail, entity):
        abstractions = {}
        for action in entity.db_actions:
            for operation in action.db_operations:
                if operation.vtType == 'add' or operation.vtType == 'change':
                    if operation.db_data.vtType == 'abstractionRef':
                        abstractions[operation.db_data.db_abstraction_id] = 1
        for a_id in abstractions.iterkeys():
            abstraction = open_abstraction_from_db(db_connection, a_id)
            db.services.abstraction.update_id_scope(abstraction)
            vistrail.db_add_abstraction(abstraction)
            load_abstractions(vistrail, abstraction)


    vt = read_sql_objects(db_connection, DBVistrail.vtType, id, lock)[0]
    # not sure where this really should be done...
    # problem is that db reads the add ops, then change ops, then delete ops
    # need them ordered by their id
    for db_action in vt.db_get_actions():
        db_action.db_operations.sort(key=lambda x: x.db_id)
    db.services.vistrail.update_id_scope(vt)
    load_abstractions(vt, vt)
    return vt

def save_vistrail_to_xml(vistrail, filename):
    daoList = getVersionDAO(currentVersion)
    tags = {'xmlns:xsi': 'http://www.w3.org/2001/XMLSchema-instance',
            'xsi:schemaLocation': 'http://www.vistrails.org/vistrail.xsd'
            }
    daoList.save_to_xml(vistrail, filename, tags, currentVersion)
    return vistrail

def save_vistrail_to_zip_xml(vistrail, filename):
    """save_vistrail_to_zip_xml(vistrail: Vistrail, filename:str)-> None
    Generate a zip compressed version of vistrail.
    It raise an Exception if there was an error
    
    """

    (file_, xmlfname) = tempfile.mkstemp(suffix='.xml')
    os.close(file_)
    save_vistrail_to_xml(vistrail,xmlfname)
    vt_fname = os.path.join(os.path.dirname(xmlfname), 'vistrail')
    os.rename(xmlfname, vt_fname)
    zip_fnames = [vt_fname, ]

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
        os.rename(log_filename, log_fname)
        zip_fnames.append(log_fname)

#     name_in_archive = os.path.join(os.path.dirname(xmlfname),'vistrail')
#     os.rename(xmlfname,name_in_archive)

    core.requirements.require_executable('zip')
    output = []
    cmdline = ['zip', '-r', '-j', '-q', filename] + zip_fnames
    result = execute_cmdline(cmdline,output)
    
    #print result, output
    for fname in zip_fnames:
        os.unlink(fname)
    if result != 0 and len(output) != 0:
        for line in output:
            if line.find('deflated') == -1:
                raise Exception(" ".join(output))
    return vistrail
            
def save_vistrail_to_db(vistrail, db_connection, do_copy=False):
    vistrail.db_version = currentVersion
    
    # method because we use recursion for nested abstractions
    def save_abstractions(vistrail, entity):
        for action in entity.db_actions:
            for operation in action.db_operations:
                if operation.vtType == 'add' or operation.vtType == 'change':
                    if operation.db_data.vtType == 'abstractionRef':
                        old_id = operation.db_data.db_abstraction_id
                        # new_id = abstraction_map[old_id]
                        abstraction = vistrail.db_abstractions_id_index[old_id]
                        save_abstractions(vistrail, abstraction)
                        abstraction = \
                            save_abstraction_to_db(abstraction, db_connection)
                        vistrail.db_new_abstractions.append(abstraction)
                        new_id = abstraction.db_id
                        if new_id != old_id:
                            operation.db_data.db_abstraction_id = new_id

    # remove abstractions for db write
    vistrail.db_new_abstractions = []
    save_abstractions(vistrail, vistrail)
    new_abstractions = vistrail.db_new_abstractions
    vistrail.db_abstractions = []
    vistrail.db_abstractions_id_index = {}

    db_connection.begin()
    if not do_copy and vistrail.db_last_modified is not None:
        new_time = get_db_object_modification_time(db_connection, 
                                                   vistrail.db_id,
                                                   DBVistrail.vtType)
        if new_time > vistrail.db_last_modified:
            # need synchronization
            old_vistrail = open_vistrail_from_db(db_connection, vistrail.db_id,
                                                 True)
            # the "old" one is modified and changes integrated
            db.services.vistrail.synchronize(old_vistrail, vistrail)
            vistrail = old_vistrail
    vistrail.db_last_modified = get_current_time(db_connection)
    if do_copy and vistrail.db_id is not None:
        vistrail.db_id = None
    write_sql_objects(db_connection, [vistrail], do_copy)
    db_connection.commit()
    
    # add abstractions back
    vistrail.db_abstractions = new_abstractions
    for abstraction in vistrail.db_abstractions:
        vistrail.db_abstractions_id_index[abstraction.db_id] = abstraction
    return vistrail

##############################################################################
# Workflow I/O

def open_workflow_from_xml(filename):
    """open_workflow_from_xml(filename) -> DBWorkflow"""
    tree = ElementTree.parse(filename)
    version = get_version_for_xml(tree.getroot())
    daoList = getVersionDAO(version)
    workflow = daoList.open_from_xml(filename, DBWorkflow.vtType)
    workflow = translateWorkflow(workflow, version)
    db.services.workflow.update_id_scope(workflow)
    return workflow

def open_workflow_from_db(db_connection, id, lock=False):
    """open_workflow_from_db(db_connection, id : long: lock: bool) 
         -> DBWorkflow 
    
    """
    if db_connection is None:
        msg = "Need to call open_db_connection() before reading"
        raise Exception(msg)
    wf = read_sql_objects(db_connection, DBWorkflow.vtType, id, lock)[0]
    return wf
    
def save_workflow_to_xml(workflow, filename):
    daoList = getVersionDAO(currentVersion)
    tags = {'xmlns:xsi': 'http://www.w3.org/2001/XMLSchema-instance',
            'xsi:schemaLocation': 'http://www.vistrails.org/workflow.xsd'
            }
    daoList.save_to_xml(workflow, filename, tags, currentVersion)
    return workflow

def save_workflow_to_db(workflow, db_connection, do_copy=False):
    db_connection.begin()
    workflow.db_version = currentVersion
    workflow.db_last_modified = get_current_time(db_connection)
    if do_copy and workflow.db_id is not None:
        workflow.db_id = None
    write_sql_objects(db_connection, [workflow], do_copy)
    db_connection.commit()
    return workflow

##############################################################################
# Logging I/O

def open_log_from_xml(filename):
    """open_log_from_xml(filename) -> DBLog"""
    tree = ElementTree.parse(filename)
    version = get_version_for_xml(tree.getroot())
    daoList = getVersionDAO(version)
    log = daoList.open_from_xml(filename, DBLog.vtType)
    log = translateLog(log, version)
    db.services.log.update_id_scope(log)
    return log

def open_log_from_db(db_connection, id, lock=False):
    """open_log_from_db(db_connection, id : long: lock: bool) 
         -> DBLog 
    
    """
    if db_connection is None:
        msg = "Need to call open_db_connection() before reading"
        raise Exception(msg)
    log = read_sql_objects(db_connection, DBLog.vtType, id, lock)[0]
    return log

def save_log_to_xml(log, filename):
    daoList = getVersionDAO(currentVersion)
    tags = {'xmlns:xsi': 'http://www.w3.org/2001/XMLSchema-instance',
            'xsi:schemaLocation': 'http://www.vistrails.org/log.xsd'
            }
    daoList.save_to_xml(log, filename, tags, currentVersion)
    return log

def save_log_to_db(log, db_connection, do_copy=False):
    db_connection.begin()
    log.db_version = currentVersion
    log.db_last_modified = get_current_time(db_connection)
    if do_copy and log.db_id is not None:
        log.db_id = None
    write_sql_objects(db_connection, [log], do_copy)
    db_connection.commit()
    return log

##############################################################################
# Abstraction I/O

def open_abstraction_from_db(db_connection, id, lock=False):
    """open_abstraction_from_db(db_connection, id : long: lock: bool) 
         -> DBAbstraction 
    
    """
    if db_connection is None:
        msg = "Need to call open_db_connection() before reading"
        raise Exception(msg)
    abstraction = read_sql_objects(db_connection, DBAbstraction.vtType, 
                                   id, lock)[0]

    # not sure where this really should be done...
    # problem is that db reads the add ops, then change ops, then delete ops
    # need them ordered by their id
    for db_action in abstraction.db_get_actions():
        db_action.db_operations.sort(key=lambda x: x.db_id)
    db.services.abstraction.update_id_scope(abstraction)
    return abstraction

def save_abstraction_to_db(abstraction, db_connection, do_copy=False):
    db_connection.begin()
    if abstraction.db_last_modified is None:
        do_copy = True
    if not do_copy:
        match_id = get_matching_abstraction_id(db_connection, abstraction)
        # FIXME remove print
        print 'match_id:', match_id
        if match_id is not None:
            abstraction.db_id = match_id
            abstraction.is_new = False
        else:
            do_copy = True
        new_time = get_db_object_modification_time(db_connection, 
                                                   abstraction.db_id,
                                                   DBAbstraction.vtType)
        if new_time > abstraction.db_last_modified:
            # need synchronization
            # FIXME remove print
            print '*** doing synchronization ***'
            old_abstraction = open_abstraction_from_db(db_connection, 
                                                       abstraction.db_id,
                                                       True)
            # the "old" one is modified and changes integrated
            db.services.vistrail.synchronize(old_abstraction, abstraction)
            abstraction = old_abstraction
    if do_copy:
        abstraction.db_id = None
    abstraction.db_last_modified = get_current_time(db_connection)
    write_sql_objects(db_connection, [abstraction], do_copy)
    db_connection.commit()
    return abstraction

##############################################################################
# I/O Utilities

def read_sql_objects(db_connection, vtType, id, lock=False):
    dao_list = getVersionDAO(currentVersion)

    all_objects = {}
    res = []
    global_props = {'id': id}
    # print global_props
    res_objects = dao_list['sql'][vtType].get_sql_columns(db_connection, 
                                                          global_props,
                                                          lock)
    all_objects.update(res_objects)
    res = res_objects.values()
    del global_props['id']

    for dao in dao_list['sql'].itervalues():
        if (dao == dao_list['sql'][DBVistrail.vtType] or
            # dao == dao_list['sql'][DBWorkflow.vtType] or
            dao == dao_list['sql'][DBLog.vtType] or
            dao == dao_list['sql'][DBAbstraction.vtType]):
            continue
        current_objs = dao.get_sql_columns(db_connection, global_props, lock)
        if dao == dao_list['sql'][DBWorkflow.vtType]:
            for key, obj in current_objs.iteritems():
                if key[0] == vtType and key[1] == id:
                    continue
                elif key[0] == DBWorkflow.vtType:
                    res_objs = \
                        read_sql_objects(db_connection, key[0], key[1], lock)
                    res_dict = {}
                    for res_obj in res_objs:
                        res_dict[(res_obj.db_id, res_obj.vtType)] = res_obj
                    all_objects.update(res_dict)
        else:
            all_objects.update(current_objs)

    for key, obj in all_objects.iteritems():
        if key[0] == vtType and key[1] == id:
            continue
        dao_list['sql'][obj.vtType].from_sql_fast(obj, all_objects)
    for obj in all_objects.itervalues():
        obj.is_dirty = False
        obj.is_new = False
    return res

def write_sql_objects(db_connection, objectList, do_copy=False, 
                      global_props=None):
    dao_list = getVersionDAO(currentVersion)

    for object in objectList:
        children = object.db_children() # forSQL=True)
        children.reverse()
        if global_props is None:
            global_props = {'entity_type': "'" + object.vtType + "'"}
        # print 'global_props:', global_props

        # assumes not deleting entire thing
        (child, _, _) = children[0]
        dao_list['sql'][child.vtType].set_sql_columns(db_connection, child, 
                                                      global_props, do_copy)
        dao_list['sql'][child.vtType].to_sql_fast(child, do_copy)
        if not do_copy:
            for (child, _, _) in children:
                for obj in child.db_deleted_children(True):
                    dao_list['sql'][obj.vtType].delete_sql_column(db_connection,
                                                                  obj,
                                                                  global_props)

        (child, _, _) = children.pop(0)
        child.is_dirty = False
        child.is_new = False
        for (child, _, _) in children:
            # print "child:", child.vtType, child.db_id
            dao_list['sql'][child.vtType].set_sql_columns(db_connection, child, 
                                                          global_props, do_copy)
            dao_list['sql'][child.vtType].to_sql_fast(child, do_copy)
            if child.vtType == DBGroup.vtType:
                if child.db_workflow:
                    # print '*** entity_type:', global_props['entity_type']
                    write_sql_objects(db_connection, [child.db_workflow],
                                      do_copy,
                                      {'entity_id': global_props['entity_id'],
                                       'entity_type': \
                                           global_props['entity_type']}
                                      )
                                            
            child.is_dirty = False
            child.is_new = False

def get_version_for_xml(root):
    version = root.get('version', None)
    if version is not None:
        return version
    msg = "Cannot find version information"
    raise VistrailsDBException(msg)

def get_current_time(db_connection=None):
    timestamp = datetime.now()
    if db_connection is not None:
        try:
            c = db_connection.cursor()
            c.execute("SELECT NOW()")
            row = c.fetchone()
            if row:
                timestamp = row[0]
            c.close()
        except MySQLdb.Error, e:
            print "Logger Error %d: %s" % (e.args[0], e.args[1])

    return timestamp


##############################################################################
# Testing

import unittest
import core.system
import os

class TestDBIO(unittest.TestCase):
    def test1(self):
        """test importing an xml file"""

        vistrail = open_vistrail_from_xml( \
            os.path.join(core.system.vistrails_root_directory(),
                         'tests/resources/dummy.xml'))
        assert vistrail is not None
        
    def test2(self):
        """test importing an xml file"""

        vistrail = open_vistrail_from_xml( \
            os.path.join(core.system.vistrails_root_directory(),
                         'tests/resources/dummy_new.xml'))
        assert vistrail is not None

    def test3(self):
        """test importing a vt file"""

        vistrail = open_vistrail_from_zip_xml( \
            os.path.join(core.system.vistrails_root_directory(),
                         'tests/resources/dummy_new.vt'))
        assert vistrail is not None

    def test4(self):
        """ test saving a vt file """

        filename = os.path.join(core.system.vistrails_root_directory(),
                                'tests/resources/dummy_new_temp.vt')
    
        vistrail = open_vistrail_from_zip_xml( \
            os.path.join(core.system.vistrails_root_directory(),
                         'tests/resources/dummy_new.vt'))
        try:
            save_vistrail_to_zip_xml(vistrail, filename)
            if os.path.isfile(filename):
                os.unlink(filename)
        except Exception, e:
            self.fail(str(e))
        

