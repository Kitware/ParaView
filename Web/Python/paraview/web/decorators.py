import sys
if sys.version_info >= (3,):
    xrange = range

#--------------------------------------------------------------------------
# Handle BooleanDomain
#--------------------------------------------------------------------------
def booleanDomainDecorator(props, xmlProps, uiProps, domain):
    uiProps['widget'] = 'checkbox'
    uiProps['type'] = 'int'
    return True

#--------------------------------------------------------------------------
# Handle EnumerationDomain
#--------------------------------------------------------------------------
def enumerationDomainDecorator(props, xmlProps, uiProps, domain):
    uiProps['widget'] = 'list-1'
    uiProps['type'] = 'int'

    valMap = {}
    for entryNum in range(domain.GetNumberOfEntries()):
        valMap[domain.GetEntryText(entryNum)] = domain.GetEntryValue(entryNum)

    uiProps['values'] = valMap

    return True

#--------------------------------------------------------------------------
# Handle StringListDomain
#--------------------------------------------------------------------------
def stringListDomainDecorator(props, xmlProps, uiProps, domain):
    uiProps['widget'] = 'list-1'
    uiProps['type'] = 'str'

    valuesList = []
    for idx in range(domain.GetNumberOfStrings()):
        valuesList.append(domain.GetString(idx))

    uiProps['values'] = valuesList

    return True

#--------------------------------------------------------------------------
# Handle TreeDomain
#--------------------------------------------------------------------------
def numpad(numelts):
    return len(str(int(numelts)))

def treeDomainDecorator(props, xmlProps, uiProps, domain):
    uiProps['type'] = 'int'
    uiProps['widget'] = 'list-n'

    values = []

    index = 0
    stack = []
    stack.append([ domain.GetInformation(), ['Multi-block Dataset'], -1 ])

    while len(stack) > 0:
        me = stack.pop()

        dataInformation = me[0]
        accumulatedName = me[1]
        parentIndex = me[2]

        values.append([accumulatedName, index, parentIndex])

        infoName = None

        if dataInformation:
            infoName = dataInformation.GetCompositeDataClassName()

        if infoName:
            # May have children
            info = dataInformation.GetCompositeDataInformation()
            numChildren = info.GetNumberOfChildren()
            if numChildren > 0:
                for i in range(numChildren - 1, -1, -1):
                    child = info.GetDataInformation(i)
                    childName = info.GetName(i)
                    stack.append([ child, accumulatedName + [childName], index ])

        index += 1

    #valMap = { 'Block 1': 2, 'Block 2': 3 }

    valMap = {}
    blockList = [None] * len(values)
    pads = numpad(len(values))
    for value in values:
        names = value[0]
        blockIdx = value[1]
        parentIdx = value[2]
        padding = '-' * (len(names) - 1)
        valuesEntryName = "%s %s %s" % (str(blockIdx).zfill(pads), padding, names[-1])
        valMap[valuesEntryName] = blockIdx
        blockList[blockIdx] = { 'label': names[-1], 'parent': parentIdx, 'id': blockIdx }

    uiProps['values'] = valMap
    uiProps['blocks'] = blockList
    uiProps['sort'] = True

    return True


#--------------------------------------------------------------------------
# Handle ArrayListDomain
#--------------------------------------------------------------------------
def arrayListDomainDecorator(props, xmlProps, uiProps, domain):
    uiProps['type'] = 'str'
    uiProps['widget'] = 'list-1'

    fieldMap = { "0": "POINTS", "1": "CELLS" }
    valuesMap = {}
    valuesList = []

    for arnum in range(domain.GetNumberOfStrings()):
        stringName = domain.GetString(arnum)
        try:
            assocVar = domain.GetFieldAssociation(arnum)
            valuesMap[stringName] = [ fieldMap[str(assocVar)], stringName ]
        except:
            valuesList.append(stringName)

    if len(valuesList) > 0:
        uiProps['values'] = valuesList
    else:
        uiProps['values'] = valuesMap

    return True

#--------------------------------------------------------------------------
# Handle ArraySelectionDomain
#--------------------------------------------------------------------------
def arraySelectionDomainDecorator(props, xmlProps, uiProps, domain):
    uiProps['type'] = 'str'
    uiProps['widget'] = 'list-n'

    valuesList = []

    for arnum in range(domain.GetNumberOfStrings()):
        valuesList.append(domain.GetString(arnum))

    uiProps['values']  = valuesList

    return True

#--------------------------------------------------------------------------
# Handle IntRangeDomain, DoubleRangeDomain, and ArrayRangeDomain
#--------------------------------------------------------------------------
def numberRangeDomainDecorator(props, xmlProps, uiProps, domain):
    uiProps['widget'] = 'textfield'

    domainClass = domain.GetClassName()

    if domainClass.rfind('Int') >= 0:
        uiProps['type'] = 'int'
    else:
        uiProps['type'] = 'float'

    ranges = []

    idx = 0
    while domain.GetMinimumExists(idx) == 1 and domain.GetMaximumExists(idx) == 1:
        ranges.append({ 'min': domain.GetMinimum(idx), 'max': domain.GetMaximum(idx) })
        idx += 1

    uiProps['range'] = ranges

    if idx == 0:
        return False
    return True

#--------------------------------------------------------------------------
# Handle ProxyListDomain
#--------------------------------------------------------------------------
def proxyListDomainDecorator(props, xmlProps, uiProps, domain):
    uiProps['widget'] = 'list-1'
    uiProps['size'] = 1
    uiProps['type'] = 'proxy'
    values = {}

    for i in xrange(domain.GetNumberOfProxies()):
        nextProxy = domain.GetProxy(i)
        values[nextProxy.GetXMLLabel()] = nextProxy.GetGlobalIDAsString()

    uiProps['values'] = values
    return True

#--------------------------------------------------------------------------
# Decorate for PropertyWidgetDecorator hint with type="ClipScalarDecorator"
#--------------------------------------------------------------------------
# FIXME: Remove this when GenericDecorator goes into master
def clipScalarDecorator(prop, uiElt, hint):
    proxy = None
    try:
        proxy = prop.SMProperty.GetParent()
    except:
        try:
            proxy = prop.GetParent()
        except:
            print ('ERROR: unable to get proxy for property ' + prop.Name)
            return
    excludeAttr = hint.GetAttribute('exclude')
    if not excludeAttr or excludeAttr != '1':
        depends = proxy.GetGlobalIDAsString() + ':ClipFunction:Scalar:1'
    else:
        depends = proxy.GetGlobalIDAsString() + ':ClipFunction:Scalar:0'
    uiElt['depends'] = depends

#--------------------------------------------------------------------------
# Decorate for PropertyWidgetDecorator hint with type="GenericDecorator"
#--------------------------------------------------------------------------
def genericDecorator(prop, uiElt, hint):
    proxy = None
    try:
        proxy = prop.SMProperty.GetParent()
    except:
        try:
            proxy = prop.GetParent()
        except:
            print ('ERROR: unable to get proxy for property ' + prop.Name)
            return

    mode = hint.GetAttribute("mode")

    # For now we just handle visbility mode
    if mode == 'visibility':
        propName = hint.GetAttribute("property")
        value = hint.GetAttribute("value")
        inverse = hint.GetAttribute("inverse")
        visibility = '1'

        if not inverse or inverse != '1':
            visibility = '1'
        else:
            visibility = '0'

        uiElt['depends'] = "%s:%s:%s:%s" % (proxy.GetGlobalIDAsString(), propName, value, visibility)


#--------------------------------------------------------------------------
# Decorate for Widget hint with type="multi_line"
#--------------------------------------------------------------------------
def multiLineDecorator(prop, uiElt, hint):
    uiElt['widget'] = 'textarea'

#--------------------------------------------------------------------------
# Decorate for proxyEditorPropertyWidget hint
#--------------------------------------------------------------------------
def proxyEditorPropertyWidgetDecorator(prop, uiElt, hint):
    propName = hint.GetAttribute("property")
    if propName:
        uiElt['visibilityProperty'] = propName
