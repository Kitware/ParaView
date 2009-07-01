
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
"""Hasher class for vistrail items."""

from core.cache.utils import hash_list
import sha

##############################################################################

class Hasher(object):

    @staticmethod
    def parameter_signature(p):
        hasher = sha.new()
        hasher.update(p.type)
        hasher.update(p.strValue)
        hasher.update(p.name)
        hasher.update(p.evaluatedStrValue)
        return hasher.digest()

    @staticmethod
    def function_signature(function):
        hasher = sha.new()
        hasher.update(function.name)
        hasher.update(function.returnType)
        hasher.update(hash_list(function.params, Hasher.parameter_signature))
        return hasher.digest()

    @staticmethod
    def connection_signature(c):
        hasher = sha.new()
        hasher.update(c.source.name)
        hasher.update(c.destination.name)
        return hasher.digest()

    @staticmethod
    def connection_subpipeline_signature(c, source_sig, dest_sig):
        """Returns the signature for the connection, including source
and dest subpipelines"""
        hasher = sha.new()
        hasher.update(Hasher.connection_signature(c))
        hasher.update(source_sig)
        hasher.update(dest_sig)
        return hasher.digest()

    @staticmethod
    def module_signature(obj):
        hasher = sha.new()
        hasher.update(obj.name)
        hasher.update(obj.package)
        hasher.update(obj.namespace or '')
        hasher.update(hash_list(obj.functions, Hasher.function_signature))
        return hasher.digest()

    @staticmethod
    def subpipeline_signature(module_sig, upstream_sigs):
        """Returns the signature for a subpipeline, given the
signatures for the upstream pipelines and connections.

        WARNING: For efficiency, upstream_sigs is mutated!
        """
        hasher = sha.new()
        hasher.update(module_sig)
        upstream_sigs.sort()
        for pipeline_connection_sig in upstream_sigs:
            hasher.update(pipeline_connection_sig)
        return hasher.digest()

    @staticmethod
    def compound_signature(sig_list):
        """compound_signature(list of signatures) -> sha digest
        returns the signature of the compound object formed by the list
        of signatures, assuming the list order is irrelevant"""
        hasher = sha.new()
        for h in sorted(sig_list):
            hasher.update(h)
        return hasher.digest()
