
import QtTesting

exceptionsList = [ 'MainWindow/variableToolbar/QWidgetAction0',
                   'MainWindow/representationToolbar/QWidgetAction0' ]

# test that all actions in the tool bars are in the menus
missingActionsString = QtTesting.invokeMethod('MainWindow', 'findToolBarActionsNotInMenus')
missingActions = missingActionsString.split(',')
numMissing = 0
for str in missingActions:
  str = str.strip()
  if str not in exceptionsList:
    numMissing = numMissing + 1
    print 'missing action: ' + str

if numMissing != 0:
  raise ValueError('Some actions are missing from the menu')

