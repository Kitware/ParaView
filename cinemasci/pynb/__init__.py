import csv
import ipywidgets
import os
from IPython.display import display

class CinemaViewer():

    LayoutHorizontal = 0
    LayoutVertical   = 1
    CinemaViewerDefaultHeight = 512

    def __init__(self):
        self.imageWidgets = []
        self.layout = CinemaViewer.LayoutVertical
        self.height = CinemaViewer.CinemaViewerDefaultHeight

        self.parametersAndFilepathsSelectionOutput = None
        self.parameterValuesOutput = None
        self.imagesOutput = None
        self.uiImageSize = None
        self.hideParameterControls = []
        self.hideUIControls = []

        # =====================================================================================
        # instance variables that control ...
        self.parameterWidgets = [] # the selected parameters
        self.filepathWidgets = [] # the selected file paths
        self.parameterValueWidgets = [] # the selected parameter values
        self.uiWidgets = [] # widges for ui interaction
        self.dbPathWidget = []
        self.parameterKey2filepathMap = dict() # the key-value map that maps parameter combinations to filepaths

    def setLayoutToVertical(self):
        self.layout = CinemaViewer.LayoutVertical 

    def setLayoutToHorizontal(self):
        self.layout = CinemaViewer.LayoutHorizontal 

    def setHeight(self, height):
        self.height = height

    def displayControls(self):
        with self.parameterValuesOutput:
            self.parameterValuesOutput.clear_output()
            tempParameterValueWidgets = []
            for w in self.parameterValueWidgets:
                if w.description not in self.hideParameterControls:
                    tempParameterValueWidgets.append(w)
            tempUIWidgets = []
            for w in self.uiWidgets:
                if w.description not in self.hideUIControls:
                    tempUIWidgets.append(w)
            temp = ipywidgets.VBox(tempParameterValueWidgets + tempUIWidgets)
            display( temp )     
        
    def hideUIControl(self, name):
        for w in self.uiWidgets:
            if (name == w.description):
                if (name not in self.hideUIControls):
                    self.hideUIControls.append(name)
                self.displayControls()
                return True
        return False   
    
    def showUIControl(self, name):
        for w in self.uiWidgets:
            if (name == w.description):
                if (name in self.hideUIControls):
                    self.hideUIControls.remove(name)
                self.displayControls()
                return True
        return False

    def getUINames(self):
        ui = []
        for w in self.uiWidgets:
            ui.append(w.description)
        return ui
    
    def getUIValues(self):
        ui = {}
        for w in self.uiWidgets:
            ui[w.description] = w.value
        return ui
    
    def getUIOptions(self, name):
        for w in self.uiWidgets:
            if (w.description == name):
                return w.options
            
    def getUIValue(self, name):
        for w in self.uiWidgets:
            if (w.description == name):
                return w.value
    
    def setUIValues(self, valuesDict):
        for w in self.uiWidgets:
            if w.description in valuesDict.keys():
                if (valuesDict[w.description] in w.options):
                    w.value = valuesDict[w.description]
                else:
                    print(str(valuesDict[w.description]) + " is not an value of " + w.description)
                
    
    def hideParameterControl(self, name):
        for w in self.parameterValueWidgets:
            if (name == w.description):
                if (name not in self.hideParameterControls):
                    self.hideParameterControls.append(name)
                self.displayControls()
                return True
        return False   
    
    def showParameterControl(self, name):
        for w in self.parameterValueWidgets:
            if (name == w.description):
                if (name in self.hideParameterControls):
                    self.hideParameterControls.remove(name)
                self.displayControls()
                return True
        return False

    def getParameterNames(self):
        params = []
        for w in self.parameterValueWidgets:
            params.append(w.description)
        return params

    def getParameterValues(self):
        params = {}
        for w in self.parameterValueWidgets:
            params[w.description] = w.value
        return params
    
    def getParameterOptions(self, name):
        for w in self.parameterValueWidgets:
            if (w.description == name):
                return w.options
    
    def getParameterValue(self, name):
        for w in self.parameterValueWidgets:
            if (w.description == name):
                return w.value
            
    def setParameterValues(self, valuesDict):
        for w in self.parameterValueWidgets:
            if w.description in valuesDict.keys():
                if (valuesDict[w.description] in w.options):
                    w.value = valuesDict[w.description]
                else:
                    print(str(valuesDict[w.description]) + " is not an value of " + w.description)
            
#         for key in valuesDict:
#             for w in self.parameterValueWidgets:
#                 if key == w.description:
#                     if (valuesDict[key] in w.options):
#                         w.value = valuesDict[key]
#                     else:
#                         print(str(valuesDict[key]) + " is not an option of " + key)

    def parseInput(self, paths):
        cdatabases = paths.split(' ')
        cdatabases = list(filter(lambda a: a != '', cdatabases))
        return cdatabases

    def readDataBaseHeader(self, path):
        index2ParameterNameMap = []
        with open(path+"/data.csv") as csv_file:
            csv_reader = csv.reader(csv_file, delimiter=",")
            # parse header
            for row in csv_reader:
                i = 0
                index2ParameterNameMap = [j for j in range(len(row))]
                for value in row:
                    if value=="FILE":
                        fileColumnIndex = i
                    index2ParameterNameMap[i] = value
                    i+=1
                break
                
        return index2ParameterNameMap

    # =====================================================================================
    # determines for each column if it contains numeric values
    def determineNumericColumns(self, path):
        index2isNumeric = []
        with open(path+"/data.csv") as csv_file:
            csv_reader = csv.reader(csv_file, delimiter=",")
            
            # skip header
            for row in csv_reader:
                break
                
            # determine type
            for row in csv_reader:
                for value in row:
                    try:
                        valueAsNumber = float(value)
                        index2isNumeric.append(True)
                    except ValueError:
                        index2isNumeric.append(False)
                break
                
        return index2isNumeric

    # =====================================================================================
    # determines the set of possible parameter values for each column
    def readParameterValues(self, path, selectedParameters):
        index2isNumeric = self.determineNumericColumns(path)
        parameterValues = dict()
        
        for i in selectedParameters:
            parameterValues[i] = set()
        
        with open(path+"/data.csv") as csv_file:
            
            csv_reader = csv.reader(csv_file, delimiter=",")
            
            # skip header
            for row in csv_reader:
                break;
            
            # read content
            for row in csv_reader:
                for i in selectedParameters:
                    value = row[i]
                    if index2isNumeric[i]:
                        value = float(value)
                    parameterValues[i].add(value)
                
        return parameterValues

    # =====================================================================================
    # builds the a map that maps a combination of parameter values to a set of filepaths
    def buildParameterKey2FilepathMap(self, path, selectedParameters, selectedFilepaths):
        self.parameterKey2filepathMap.clear()
        
        index2isNumeric = self.determineNumericColumns(path)
        
        with open(path+"/data.csv") as csv_file:
            
            csv_reader = csv.reader(csv_file, delimiter=",")
            
            # skip header
            for row in csv_reader:
                break;
            
            # read content
            for row in csv_reader:
                filepaths = []
                parameterKey = ""
                for i in selectedParameters:
                    value = row[i]
                    if index2isNumeric[i]:
                        value = str(float(value))
                    
                    parameterKey += value+"_"
                    
                for i in selectedFilepaths:
                    filepaths.append(row[i])
                
                if self.parameterKey2filepathMap.get(parameterKey)==None:
                    self.parameterKey2filepathMap[parameterKey] = []
                
                self.parameterKey2filepathMap[parameterKey] += filepaths
        
        return 

    # =====================================================================================
    # builds a parameter key based on widget values
    def buildParameterKey(self):
        key = ""
        for w in self.parameterValueWidgets:
            key += str(w.value)+"_"
        return key

    # =====================================================================================
    # update widgets to distinguish between parameters and filepaths of the selected database
    def updateParameterAndFilepathWidgets(self, ignore):
        cdatabases = self.dbPathWidget.value.split(' ')
        cdb = cdatabases[0] #all databases are assumed to have same parameter list
        dbHeader = self.readDataBaseHeader(cdb)
        self.parameterWidgets.clear()
        self.filepathWidgets.clear()

        for param in dbHeader:
            pW = ipywidgets.ToggleButton(
                value= param!="FILE",
                description=param,
                disabled=False
            )
            pW.observe(self.updateParameterValueWidgets, names='value')
            self.parameterWidgets.append(pW)

            fW = ipywidgets.ToggleButton(
                value= param=="FILE",
                description=param,
                disabled=False
            )
            fW.observe(self.updateParameterValueWidgets, names='value')
            self.filepathWidgets.append(fW)

        grid = ipywidgets.GridspecLayout(2, len(self.parameterWidgets)+1)
        grid[0,0] = ipywidgets.Label("Parameters")
        for j in range(0,len(self.parameterWidgets)):
            grid[0,j+1] = self.parameterWidgets[j]
        for j in range(0,len(self.filepathWidgets)):
            grid[1,j+1] = self.filepathWidgets[j]

        with self.parametersAndFilepathsSelectionOutput:
            self.parametersAndFilepathsSelectionOutput.clear_output()
            # display(grid)

        self.updateParameterValueWidgets('')
        return

    # =====================================================================================
    # update widgets to select parameter values 
    def updateParameterValueWidgets(self, ignore):
        selectedParameters = dict()
        selectedFilepaths = dict()
        if True:
            i=0
            for w in self.parameterWidgets:
                if w.value==True:
#                 print(w.description)
                    selectedParameters[i] = w.description
                i+=1
            i=0
            for w in self.filepathWidgets:
                if w.value==True:
#                 print(w.description)
                    selectedFilepaths[i] = w.description
                i+=1
        
        cdatabases = self.dbPathWidget.value.split(' ')
        cdb = cdatabases[0]#all databases are assumed to have same parameter list
        selectedParameterValues = self.readParameterValues(cdb, selectedParameters)
        self.buildParameterKey2FilepathMap(cdb, selectedParameters, selectedFilepaths)

        self.parameterValueWidgets.clear()
        for i in selectedParameters:
            w = ipywidgets.SelectionSlider(
                options=sorted(selectedParameterValues[i]),
                description=selectedParameters[i],
                disabled=False,
                continuous_update=True,
                orientation='horizontal',
                readout=True
            )
            w.observe(self.updateImages, names='value')
            self.parameterValueWidgets.append(w)

        self.uiWidgets.clear()
        self.uiImageSize = ipywidgets.SelectionSlider(
            options=range(100,513),
            description='image size',
            disabled=False,
            continuous_update=True,
            orientation='horizontal',
            readout=True
        )
        self.uiImageSize.observe(self.updateImages, names='value')
        self.uiWidgets.append(self.uiImageSize)
            
#       uiImgLayout = ipywidgets.ToggleButtons(
#           options=['Vertical', 'Horizontal'],
#           description='Layout:',
#           disabled=False,
#           continuous_update=True,
#       )
#       uiImgLayout.observe(self.updateImages, names='value')
#       self.uiWidgets.append(uiImgLayout)

        with self.parameterValuesOutput:
            self.parameterValuesOutput.clear_output()
            temp = ipywidgets.VBox(self.parameterValueWidgets + self.uiWidgets)
            display( temp )

#         with self.parameterValuesOutput:
#             self.parameterValuesOutput.clear_output()
#             tempParameterValueWidgets = []
#             for w in self.parameterValueWidgets:
#                 if w.description not in self.hideParameterControls:
#                     tempParameterValueWidgets.append(w)
#             tempUIWidgets = []
#             for w in self.uiWidgets:
#                 if w.description not in self.hideUIControls:
#                     tempUIWidgets.append(w)
#             temp = ipywidgets.VBox(tempParameterValueWidgets + tempUIWidgets)
#             display( temp )

        self.updateImages('')
        return


    # =====================================================================================
    # fetch images that correspond to the currently selected parameter values 
    # =====================================================================================
    def updateImages(self, ignore, changeLayout=False):
        cdatabases = self.parseInput(self.dbPathWidget.value)
        
        imgsize = str(self.uiImageSize.value)
        
        key = self.buildParameterKey()
        files = []
        for cdb in cdatabases:
            for filepath in self.parameterKey2filepathMap[key]:
                file = open(cdb+'/'+filepath, "rb")
                files.append(file.read())

        if len(files)!=len(self.imageWidgets):
            self.imageWidgets.clear()
            self.imagesOutput.clear_output()
            for file in files:
                w = ipywidgets.Image(
                    format='png',
                    layout={'display':'block','max_width': str(imgsize)+"px",'max_height': str(imgsize)+"px"}
                )
                self.imageWidgets.append(w)
                with self.imagesOutput:
                    display(w)

        else:
             for i in range(0,len(files)):
                 self.imageWidgets[i].layout.max_height = str(imgsize)+"px"
                 self.imageWidgets[i].layout.max_width = str(imgsize)+"px"

             if (changeLayout):
                 if self.layout == CinemaViewer.LayoutVertical:
                     with self.imagesOutput:
                         self.imagesOutput.clear_output()
                         temp = ipywidgets.VBox(self.imageWidgets)
                         display(temp)
                 else:
                     with self.imagesOutput:
                         self.imagesOutput.clear_output()
                         temp = ipywidgets.HBox(self.imageWidgets)
                         display(temp)

        for i in range(0,len(files)):
            self.imageWidgets[i].value = files[i]
            
        return

    # =====================================================================================
    # load a set of cinema databases and build UI elements to display them 
    # =====================================================================================
    def load(self, paths):
        # =====================================================================================
        # check input paths of cinema dbs
        # =====================================================================================
        cdatabases = self.parseInput(paths)
        for cdb in cdatabases:
            if (not os.path.isdir(cdb)):
                print(cdb + " does not exist")
                return
        
        # =====================================================================================
        # create display outputs for widgets
        # =====================================================================================
        self.parametersAndFilepathsSelectionOutput = ipywidgets.Output(layout={'border': '0px solid black', 'width':'98%'})
        self.parameterValuesOutput = ipywidgets.Output(layout={'border': '0px solid black', 
                                        'width':'40%', 'height':'{}px'.format(self.height)})
        self.imagesOutput = ipywidgets.Output(layout={'border': '0px solid black', 'width':'59%'})
        self.uiImageSize = None

        # =====================================================================================
        # create database path widget 
        # =====================================================================================
        self.dbPathWidget = ipywidgets.Text(
            value='',
            placeholder='Absolute path to .cdb',
            description='CDB:',
            continuous_update=False,
            disabled=False,
            layout=ipywidgets.Layout(width='90%')
        )
        # set paths
        self.dbPathWidget.value = paths

        # start listening for new database path
        self.updateParameterAndFilepathWidgets('')      
        
        # display UI
        frame = ipywidgets.VBox([
            self.parametersAndFilepathsSelectionOutput,
            ipywidgets.HBox([self.parameterValuesOutput, self.imagesOutput])
        ])
        display(frame)

        # update the images, and change the layout
        self.updateImages('', changeLayout=True)

