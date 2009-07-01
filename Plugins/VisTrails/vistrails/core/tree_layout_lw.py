
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
""" This file has the implementation of an algorithm to layout general
rooted trees in a nice way. The "lw" in the file name "tree_layout_lw.py"
stands for (L)inear (W)alker. This code is based on the paper:

    Christoph Buchheim, Michael Junger, and Sebastian Leipert.
    Improving walker's algorithm to run in linear time.
    In Stephen G. Kobourov and Michael T. Goodrich, editors, Graph
    Drawing, volume 2528 of Lecture Notes in Computer Science, pages
    344-353. Springer, 2002.

which is a faster (it is linear!) way to compute the tree layout
proposed by Walker than the algorithm described by him.
The original paper is:

    John Q. Walker II.
    A node-positioning algorithm for general trees.
    Softw., Pract. Exper., 20(7):685-705, 1990.

"""

class TreeLW:
    """
    The input to the algorithm must be a tree
    in this format.

    """
    def __init__(self):
        self.nodes = []
        self.maxLevel = 0

    def root(self):
        return self.nodes[0]

    def addNode(self, parentNode, width, height, object = None):
        newNode = NodeLW(width,height,object)        
        self.nodes.append(newNode)

        # add
        if parentNode != None:
            parentNode.addChild(newNode)

        # update max level
        self.maxLevel = max(self.maxLevel,newNode.level)
        return newNode

    def changeParentOfNodeWithNoParent(self, parentNode, childNode):
        if childNode.parent != None:
            raise Exception()

        #
        parentNode.addChild(childNode)        
        maxLevel = self.__dfsUpdateLevel(childNode)

        # update max level
        self.maxLevel = max(self.maxLevel, maxLevel)

    def __dfsUpdateLevel(self, node):
        if node.parent == None:
            node.level = 0
        else:
            node.level = node.parent.level + 1
        maxLevel = node.level
        for child in node.childs:
            maxLevel = max(maxLevel, self.__dfsUpdateLevel(child))
        return maxLevel

    def boundingBox(self):
        kbb = KeepBoundingBox()
        for w in self.nodes:
            kbb.addPoint(w.x-w.width/2.0, w.y-w.height/2.0)
            kbb.addPoint(w.x+w.width/2.0, w.y+w.height/2.0)
        return kbb.getBoundingBox()            

    def getMaxNodeHeightPerLevel(self):
        result = [0] * (self.maxLevel+1)
        for w in self.nodes:
            level = w.level
            result[level] = max(result[level],w.height)
        return result

    @staticmethod
    def randomTree(n,  k=10000000):
        p = [0] * n
        import random
        for i in xrange(1, n):
            minIndex = max(i-k, 0)
            index = random.randint(minIndex, i-1) # random number in {0,1,2,...,i-1}
            p[i] = index
        t = TreeLW()
        nodes= []
        for i in xrange(n):
            if i==0:
                parent= None
            else:
                parent=nodes[p[i]]
            width = 5 + 10*random.random()
            height = 5 + 10*random.random()
            nodes.append(t.addNode(parent,width,height,i))
        return t            

class KeepBoundingBox:
    def __init__(self):
        self.minx = None
        self.miny = None
        self.maxx = None
        self.maxy = None
        self.size = 0

    def addPoint(self,x,y):
        if self.minx == None or self.minx > x:
            self.minx = x
        if self.miny == None or self.miny > y:
            self.miny = y
        if self.maxx == None or self.maxx < x:
            self.maxx = x
        if self.maxy == None or self.maxy < y:
            self.maxy = y
        self.size = self.size + 1

    def getBoundingBox(self):
        return [self.minx, self.miny, self.maxx-self.minx, self.maxy - self.miny]

class NodeLW:
    """
    Node of the tree with all the auxiliar
    variables needed to the LW algorithm.
    The fields width, height and object
    are given as input. The first two are
    used to layout while the last one might
    be used by the user of this class.

    """    
    def __init__(self, width, height, object = None):
        self.width = width
        self.height = height
        self.object = object

        self.childs = []

        self.parent = None
        self.index = 0

        # level of the node
        self.level = 0
        
        # intermediate variables for 
        # layout algorithm
        self.mod = 0
        self.prelim = 0
        self.ancestor = None
        self.thread = None
        self.change = 0
        self.shift = 0

        # final center position
        self.x = 0
        self.y = 0
        
    def getNumChilds(self):
        return len(self.childs)
        
    def hasChild(self):
        return len(self.childs) > 0

    def addChild(self, node):
        self.childs.append(node)
        node.index = len(self.childs) - 1
        node.parent = self
        node.level = self.level + 1

    def isLeaf(self):
        return len(self.childs) == 0

    def leftChild(self):
        return self.childs[0]

    def rightChild(self):
        return self.childs[len(self.childs)-1]

    def leftSibling(self):
        if self.index > 0:
            return self.parent.childs[self.index-1]
        else:
            return None

    def leftMostSibling(self):
        if self.parent != None:
            return self.parent.childs[0]
        else:
            return self

    def isSiblingOf(self, v):
        return self.parent == v.parent and self.parent != None

class TreeLayoutLW:

    """
    TreeLayoutLW: the LW stands for Linear Walker.

    This code is based on the paper:

    Christoph Buchheim, Michael Junger, and Sebastian Leipert.
    Improving walker's algorithm to run in linear time.
    In Stephen G. Kobourov and Michael T. Goodrich, editors, Graph
    Drawing, volume 2528 of Lecture Notes in Computer Science, pages
    344-353. Springer, 2002.

    which is a faster (linear) way to compute the tree layout
    proposed by Walker than the algorithm described by him.
    The original paper is:

    John Q. Walker II.
    A node-positioning algorithm for general trees.
    Softw., Pract. Exper., 20(7):685-705, 1990.
    """

    # vertical alignment of nodes in the 
    # level band: TOP, MIDDLE, BOTTOM
    TOP = 0
    MIDDLE = 1
    BOTTOM = 2

    def __init__(self, tree, vertical_alignment=1, xdistance=10, ydistance=10):
        self.xdistance = xdistance
        self.ydistance = ydistance
        self.tree = tree
        self.vertical_alignment = vertical_alignment
        self.treeLayout()

    def treeLayout(self):
        for v in self.tree.nodes:
            v.mod = 0
            v.thread = None
            v.ancestor = v
            
        r = self.tree.root()
        self.firstWalk(r)
        self.secondWalk(r, -r.prelim)
        self.setVerticalPositions()

    def setVerticalPositions(self):

        # set y position
        maxNodeHeightPerLevel = self.tree.getMaxNodeHeightPerLevel()
        info_level = []
        position_level = 0
        for level in xrange(len(maxNodeHeightPerLevel)):
            height_level = maxNodeHeightPerLevel[level]
            info_level.append((position_level,height_level))
            position_level += self.ydistance + height_level
            
        #
        for w in self.tree.nodes:
            level = w.level
            position_level, height_level = info_level[level]
            if self.vertical_alignment == TreeLayoutLW.TOP:
                w.y = position_level + w.height/2.0
            elif self.vertical_alignment == TreeLayoutLW.MIDDLE:
                w.y = position_level + height_level/2.0
            else: # bottom
                w.y = position_level + height_level - w.height/2.0
                
        
    def gap(self, v1, v2):

        return self.xdistance + (v1.width + v2.width)/2.0        


    def firstWalk(self, v):
        
        if v.isLeaf():
            v.prelim = 0
            w = v.leftSibling()
            if w != None:
                v.prelim = w.prelim + self.gap(w,v)

        else:
            
            defaultAncestor = v.leftChild()
            for w in v.childs:
                self.firstWalk(w)
                defaultAncestor = self.apportion(w, defaultAncestor)
            self.executeShifts(v)
            
            midpoint = (v.leftChild().prelim + v.rightChild().prelim) / 2.0

            w = v.leftSibling()
            if w != None:
                v.prelim = w.prelim + self.gap(w,v)
                v.mod = v.prelim - midpoint
            else:
                v.prelim = midpoint


    def apportion(self,  v,  defaultAncestor):

        """
        Apportion: to divide and assign proportionally.

        Suppose the the left siblings of "v" are all aligned. 
        Now align the subtree with root "v" (note: the correct
        alignment of the left siblings of "v" are encoded 
        in the auxiliar variables, the x and y 
        are not correct; only in the end the correct 
        values are assigned to x  and y). 
        By property (*) in Section 4 the gratest 
        distinct ancestor of a node "w" in the 
        subtrees at rooted at the left siblings of "v"
        is w.ancestor if this value is a left sibling
        of v otherwise it is "defaultAncestor".

        """
        w = v.leftSibling()
        if w != None:
            # p stands for + or plus (right subtree)
            # m stands for - or minus (left subtree)
            # i stands for inside
            # o stands for outside
            # v stands for vertex
            # s stands for shift
            vip = vop = v
            vim = w
            vom = vip.leftMostSibling()
            sip = vip.mod
            sop = vop.mod
            sim = vim.mod
            som = vom.mod
            while self.nextRight(vim) != None and self.nextLeft(vip) != None:
                
                vim = self.nextRight(vim)
                vip = self.nextLeft(vip)
                vom = self.nextLeft(vom)
                vop = self.nextRight(vop)

                vop.ancestor = v
                
                shift = (vim.prelim + sim) - (vip.prelim + sip) + self.gap(vim,vip)
                
                if shift > 0:
                    self.moveSubtree(self.ancestor(vim,v,defaultAncestor),v,shift)
                    sip += shift
                    sop += shift

                sim += vim.mod
                sip += vip.mod
                som += vom.mod
                sop += vop.mod

            if self.nextRight(vim) != None and self.nextRight(vop) == None:            
                vop.thread = self.nextRight(vim)
                vop.mod += sim - sop

            if self.nextLeft(vip) != None and self.nextLeft(vom) == None:            
                vom.thread = self.nextLeft(vip)
                vom.mod += sip - som
                defaultAncestor = v
            
        return defaultAncestor

    def nextLeft(self, v):
        if v.hasChild():
            return v.leftChild()
        else:
            return v.thread

    def nextRight(self, v):
        if v.hasChild():
            return v.rightChild()
        else:
            return v.thread

    def moveSubtree(self, wm, wp, shift):
        subtrees = float(wp.index - wm.index)
        wp.change += -shift/subtrees
        wp.shift += shift
        wm.change += shift/subtrees
        wp.prelim += shift
        wp.mod += shift

    def executeShifts(self, v):
        shift = 0
        change = 0
        for i in xrange(v.getNumChilds()-1,-1,-1):
            w = v.childs[i]
            w.prelim += shift
            w.mod += shift
            change += w.change
            shift += w.shift + change

    def ancestor(self, vim, v, defaultAncestor):
        if vim.ancestor.isSiblingOf(v):
            return vim.ancestor
        else:
            return defaultAncestor

    def secondWalk(self,  v, m):
        v.x = v.prelim + m
        for w in v.childs:
            self.secondWalk(w, m + v.mod)

# graph
if __name__ == "__main__":

    t = TreeLW()

    a = t.addNode(None,1,1,"a")

    b = t.addNode(a,1,1,"b")
    c = t.addNode(a,1,1,"c")
    d = t.addNode(a,1,1,"d")

    e = t.addNode(b,1,1,"e")
    f = t.addNode(b,1,1,"f")
    g = t.addNode(d,1,1,"g")
    h = t.addNode(d,1,1,"h")

    i = t.addNode(f,1,1,"i")
    j = t.addNode(f,1,1,"j")
    k = t.addNode(h,1,1,"k")
    l = t.addNode(h,1,1,"l")
    m = t.addNode(h,1,1,"m")
    n = t.addNode(h,1,1,"n")
    o = t.addNode(h,1,1,"o")
    
    layout = TreeLayoutLW(t)
    
