
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

"""Keychain keeps a dictionary of key/value pairs.
It is useful for storing passwords during a vistrails session.
The values are hashed given the key and the caller object id.
A value can only be retrieved by the same object that set it.
Use this at your own risk!

"""
import inspect
import os
import sys
import md5
from core import debug
from core.utils import VistrailsInternalError
from itertools import izip

##############################################################################

class KeyChain(object):
    """ 
    """

    def __init__(self):
        """ initializes the internal dictionary"""
        self.__keys = {}

    def count(self):
        """count() -> int
        Returns the number of keys

        """
        return len(self.__keys.keys())

    def clear(self):
        """clear() -> None
        clear the internal dictionary
        """
        self.__keys.clear()
    
    def set_key(self, key, value):
        """ set_key(key: str, value: str) -> None
        Sets a key value pair in the internal dictionary

        """
        #get the arguments of the frame that called us
        args = inspect.getargvalues(inspect.currentframe().f_back)[3]
        try:
            #this will return the instance of the object that called us
            caller = id(args['self'])
            newkey = str(caller)+str(key)
            hashkey = md5.new(newkey).hexdigest()[:16]
            cryptvalue = crypt(hashkey,value)
            self.__keys[hashkey] = cryptvalue
            
        except KeyError:
            dbg = debug.DebugPrint
            dbg.critical("KeyChain: You need to call this method inside "
                         "another a object's method")

    def get_key(self, key):
        """ get_key(key:str) -> str
        Returns the value for the key. Only the object that set the key is
        able to retrieve its value

        """
        result = ""
        #get the arguments of the frame that called us
        args = inspect.getargvalues(inspect.currentframe().f_back)[3]
        try:
            #this will return the instance of the object that called us
            caller = id(args['self'])
            newkey = str(caller)+str(key)
            hashkey = md5.new(newkey).hexdigest()[:16]
            if self.__keys.has_key(hashkey):
                return crypt(hashkey,self.__keys[hashkey])
            else:
                dbg = debug.DebugPrint
                dbg.warning("KeyChain: the key is not present or only the"
                            " object that set the key can get it")
                return  ""
        except KeyError:
            dbg = debug.DebugPrint
            dbg.critical("KeyChain: You need to call this method inside "
            "another a object's method")    
        
##############################################################################
""" 
XTEA Block Encryption Algorithm

Author: Paul Chakravarti (paul_dot_chakravarti_at_mac_dot_com)
License: Public Domain

This module provides a Python implementation of the XTEA block encryption
algorithm (http://www.cix.co.uk/~klockstone/xtea.pdf). 

The module implements the basic XTEA block encryption algortithm
(`xtea_encrypt`/`xtea_decrypt`) and also provides a higher level `crypt`
function which symmetrically encrypts/decrypts a variable length string using
XTEA in OFB mode as a key generator. The `crypt` function does not use
`xtea_decrypt` which is provided for completeness only (but can be used
to support other stream modes - eg CBC/CFB).

This module is intended to provide a simple 'privacy-grade' Python encryption
algorithm with no external dependencies. The implementation is relatively slow
and is best suited to small volumes of data. Note that the XTEA algorithm has
not been subjected to extensive analysis (though is believed to be relatively
secure - see http://en.wikipedia.org/wiki/XTEA). For applications requiring
'real' security please use a known and well tested algorithm/implementation.

The security of the algorithm is entirely based on quality (entropy) and
secrecy of the key. You should generate the key from a known random source and
exchange using a trusted mechanism. In addition, you should always use a random
IV to seed the key generator (the IV is not sensitive and does not need to be
exchanged securely)

    >>> import os
    >>> iv = 'ABCDEFGH'
    >>> z = crypt('0123456789012345','Hello There',iv)
    >>> z.encode('hex')
    'fe196d0a40d6c222b9eff3'
    >>> crypt('0123456789012345',z,iv)
    'Hello There'

""" 

import struct

def crypt(key,data,iv='\00\00\00\00\00\00\00\00',n=32):
    """
        Encrypt/decrypt variable length string using XTEA cypher as
        key generator (OFB mode)
        * key = 128 bit (16 char) 
        * iv = 64 bit (8 char)
        * data = string (any length)

        >>> import os
        >>> key = os.urandom(16)
        >>> iv = os.urandom(8)
        >>> data = os.urandom(10000)
        >>> z = crypt(key,data,iv)
        >>> crypt(key,z,iv) == data
        True

    """
    def keygen(key,iv,n):
        while True:
            iv = xtea_encrypt(key,iv,n)
            for k in iv:
                yield ord(k)
    xor = [ chr(x^y) for (x,y) in izip(map(ord,data),keygen(key,iv,n))]
    return "".join(xor)

def xtea_encrypt(key,block,n=32,endian="!"):
    """
        Encrypt 64 bit data block using XTEA block cypher
        * key = 128 bit (16 char) 
        * block = 64 bit (8 char)
        * n = rounds (default 32)
        * endian = byte order (see 'struct' doc - default big/network) 

        >>> z = xtea_encrypt('0123456789012345','ABCDEFGH')
        >>> z.encode('hex')
        'b67c01662ff6964a'

        Only need to change byte order if sending/receiving from 
        alternative endian implementation 

        >>> z = xtea_encrypt('0123456789012345','ABCDEFGH',endian="<")
        >>> z.encode('hex')
        'ea0c3d7c1c22557f'

    """
    v0,v1 = struct.unpack(endian+"2L",block)
    k = struct.unpack(endian+"4L",key)
    sum,delta,mask = 0L,0x9e3779b9L,0xffffffffL
    for round in xrange(n):
        v0 = (v0 + (((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]))) & mask
        sum = (sum + delta) & mask
        v1 = (v1 + (((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]))) & mask
    return struct.pack(endian+"2L",v0,v1)

def xtea_decrypt(key,block,n=32,endian="!"):
    """
        Decrypt 64 bit data block using XTEA block cypher
        * key = 128 bit (16 char) 
        * block = 64 bit (8 char)
        * n = rounds (default 32)
        * endian = byte order (see 'struct' doc - default big/network) 

        >>> z = 'b67c01662ff6964a'.decode('hex')
        >>> xtea_decrypt('0123456789012345',z)
        'ABCDEFGH'

        Only need to change byte order if sending/receiving from 
        alternative endian implementation 

        >>> z = 'ea0c3d7c1c22557f'.decode('hex')
        >>> xtea_decrypt('0123456789012345',z,endian="<")
        'ABCDEFGH'

    """
    v0,v1 = struct.unpack(endian+"2L",block)
    k = struct.unpack(endian+"4L",key)
    delta,mask = 0x9e3779b9L,0xffffffffL
    sum = (delta * n) & mask
    for round in xrange(n):
        v1 = (v1 - (((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]))) & mask
        sum = (sum - delta) & mask
        v0 = (v0 - (((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]))) & mask
    return struct.pack(endian+"2L",v0,v1)
    
##############################################################################

import unittest
class A(object):
    def set_key(self, chain, key, value):
        chain.set_key(key,value)

    def get_key(self, chain, key):
        return chain.get_key(key)
    
class TestKeyChain(unittest.TestCase):
    def testusage(self):
        keyChain = KeyChain()
        this = A()
        other = A()
        #test key insertion
        this.set_key(keyChain, "mykey.name", "value")
        self.assertEquals(keyChain.count(),1)
        #test key retrieval
        value = this.get_key(keyChain, "mykey.name")
        self.assertEquals(value, "value")
        #test key protection
        value = other.get_key(keyChain, "mykey.name")
        self.assertEquals(value, "")   

if __name__ == '__main__':
    unittest.main()
