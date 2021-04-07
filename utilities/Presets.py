'''
Created on 21/07/2013

@author: Mosaab M
'''
    
import string
import os
import re
import struct
from operator import index
from os.path import basename

class Preset():
    def __init__(self):
        self.JsonFileName = "./Input/JsonEx.json"
        self.RegsFileName = "./Input/Registers_B0.h"
        self.outCurrBinFile = 0
        self.jsonFile = 0
        self.RegsFile = 0
        self.offset = 0
        self.PRESET_NAME_MAX_LEN = 20
        self.StatesDict = { '"off"' : 0, '"laserOn"': 1, '"laserAuto"': 2, '"ledOn"': 3, '"False"' : 0, '"True"' : 1}
        self.dict = {

            'et_id' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'id', 'factor' : 1, 'default' : 0}],
            'et_lambdaCensus' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'lambdacensus', 'factor' : 1, 'default' : 0}],
            'et_lambdaAD' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'lambdaad', 'factor' : 1, 'default' : 0}],
            'et_ignoreSAD' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'ignoresad', 'factor' : 1, 'default' : 0}],

            'et_colorControl': [{'regFieldName' : 'Disablesadcolor', 'jsonFieldName' : 'disablesadcolor', 'factor' : 1, 'default' : 0}, 
                               {'regFieldName' : 'Disableraucolor', 'jsonFieldName' : 'disableraucolor', 'factor' : 1, 'default' : 0}, 
                               {'regFieldName' : 'Disableslorightcolor', 'jsonFieldName' : 'disableslorightcolor', 'factor' : 1, 'default' : 0}, 
                               {'regFieldName' : 'Disablesloleftcolor', 'jsonFieldName' : 'disablesloleftcolor', 'factor' : 1, 'default' : 0},
                               {'regFieldName' : 'Disablesadnormalize', 'jsonFieldName' : 'disablesadnormalize', 'factor' : 1, 'default' : 0}],

            'et_leftRightThrld': [{'regFieldName' : 'LrAgreeThreshold', 'jsonFieldName' : 'leftrightthreshold', 'factor' : 1, 'default' : 0}],

            'et_scoreThrld': [{'regFieldName' : 'ScoreThreshA', 'jsonFieldName' : 'minscorethresha', 'factor' : 1, 'default' : 0},
                              {'regFieldName' : 'ScoreThreshB', 'jsonFieldName' : 'maxscorethreshb', 'factor' : 1, 'default' : 0}],

            'et_medianThrld': [{'regFieldName' : 'DeepseaMedianThreshold', 'jsonFieldName' : 'medianthreshold', 'factor' : 1, 'default' : 0}],
            
            'et_neighborThrld': [{'regFieldName' : 'DeepseaNeighborThreshold', 'jsonFieldName' : 'neighborthresh', 'factor' : 1, 'default' : 0}],

            'et_robbinsMunroe': [{'regFieldName' : 'PlusIncrement', 'jsonFieldName' : 'robbinsmonroincrement', 'factor' : 1, 'default' : 0},
                                 {'regFieldName' : 'MinusDecrement', 'jsonFieldName' : 'robbinsmonrodecrement', 'factor' : 1, 'default' : 0}],

            'et_secondPeakThrld': [{'regFieldName' : 'DeepseaSecondPeakThreshold', 'jsonFieldName' : 'secondpeakdelta', 'factor' : 1, 'default' : 0}],

            'et_textureThrld': [{'regFieldName' : 'TextureDifferenceThreshold', 'jsonFieldName' : 'texturedifferencethresh', 'factor' : 1, 'default' : 0},
                                 {'regFieldName' : 'TextureCountThreshold', 'jsonFieldName' : 'texturecountthresh', 'factor' : 1, 'default' : 0}],

            'et_rauVectorMinima': [{'regFieldName' : 'Minwest', 'jsonFieldName' : 'rauminw', 'factor' : 1, 'default' : 0},
                                 {'regFieldName' : 'Mineast', 'jsonFieldName' : 'raumine', 'factor' : 1, 'default' : 0},
                                 {'regFieldName' : 'Minwesum', 'jsonFieldName' : 'rauminwesum', 'factor' : 1, 'default' : 0},
                                 {'regFieldName' : 'Minnorth', 'jsonFieldName' : 'rauminn', 'factor' : 1, 'default' : 0},
                                 {'regFieldName' : 'Minsouth', 'jsonFieldName' : 'raumins', 'factor' : 1, 'default' : 0},
                                 {'regFieldName' : 'Minnssum', 'jsonFieldName' : 'rauminnssum', 'factor' : 1, 'default' : 0},
                                 {'regFieldName' : 'Ushrink', 'jsonFieldName' : 'regionshrinku', 'factor' : 1, 'default' : 0},
                                 {'regFieldName' : 'Vshrink', 'jsonFieldName' : 'regionshrinkv', 'factor' : 1, 'default' : 0}],
            
            'et_rauThrld': [{'regFieldName' : 'Raudiffthresholdred', 'jsonFieldName' : 'regioncolorthresholdr', 'factor' : 1022, 'default' : 0},
                                 {'regFieldName' : 'Raudiffthresholdgreen', 'jsonFieldName' : 'regioncolorthresholdg', 'factor' : 1022, 'default' : 0},
                                 {'regFieldName' : 'Raudiffthresholdblue', 'jsonFieldName' : 'regioncolorthresholdb', 'factor' : 1022, 'default' : 0}],
            
            'et_rsmControl': [{'regFieldName' : 'RemoveThresh', 'jsonFieldName' : 'rsmremovethreshold', 'factor' : 168, 'default' : 0},
                                 {'regFieldName' : 'SloRauDiffThresh', 'jsonFieldName' : 'rsmrauslodiffthreshold', 'factor' : 32, 'default' : 0},
                                 {'regFieldName' : 'DiffThresh', 'jsonFieldName' : 'rsmdiffthreshold', 'factor' : 32, 'default' : 0},
                                 {'regFieldName' : 'RsmBypass', 'jsonFieldName' : 'usersm', 'factor' : -1, 'default' : 0}],

            'et_sloK1P': [{'regFieldName' : 'K1penalty', 'jsonFieldName' : 'scanlinep1', 'factor' : 1, 'default' : 0}],
            'et_sloK1PMod1': [{'regFieldName' : 'K1penaltymod1', 'jsonFieldName' : 'scanlinep1onediscon', 'factor' : 1, 'default' : 0}],
            'et_sloK1PMod2': [{'regFieldName' : 'K1penaltymod2', 'jsonFieldName' : 'scanlinep1twodiscon', 'factor' : 1, 'default' : 0}],
            'et_sloK2P': [{'regFieldName' : 'K2penalty', 'jsonFieldName' : 'scanlinep2', 'factor' : 1, 'default' : 0}],
            'et_sloK2PMod1': [{'regFieldName' : 'K2penaltymod1', 'jsonFieldName' : 'scanlinep2onediscon', 'factor' : 1, 'default' : 0}],
            'et_sloK2PMod2': [{'regFieldName' : 'K2penaltymod2', 'jsonFieldName' : 'scanlinep2twodiscon', 'factor' : 1, 'default' : 0}],
            
            'et_sloThrld': [{'regFieldName' : 'Diffthresholdred', 'jsonFieldName' : 'scanlineedgetaur', 'factor' : 1, 'default' : 0},
                            {'regFieldName' : 'Diffthresholdgreen', 'jsonFieldName' : 'scanlineedgetaug', 'factor' : 1, 'default' : 0},
                            {'regFieldName' : 'Diffthresholdblue', 'jsonFieldName' : 'scanlineedgetaub', 'factor' : 1, 'default' : 0}],
            
            'et_disparityMode' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'disparitymode', 'factor' : 1, 'default' : 0}],
            'et_disparityMultiplier' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'disparitymultiplier', 'factor' : 1, 'default' : 32}],
            'et_disparityShift' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'disparityshift', 'factor' : 1, 'default' : 0}],

            'et_censusUSize' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'censususize', 'factor' : 1, 'default' : 0}],
            'et_censusVSize' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'censusvsize', 'factor' : 1, 'default' : 0}],
            'et_minZ' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'depthclampmin', 'factor' : 1, 'default' : 0}],
            'et_maxZ' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'depthclampmax', 'factor' : 1, 'default' : 65535}],
            'et_zUnits' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'zunits', 'factor' : 1, 'default' : 1000}],
            
            'et_autoExposureSetPoint' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'autoexposure-setpoint', 'factor' : 1, 'default' : 800}],

            'et_colorCorrection1' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'colorcorrection1', 'factor' : 1, 'default' : 0.0}],
            'et_colorCorrection2' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'colorcorrection2', 'factor' : 1, 'default' : 0.0}],
            'et_colorCorrection3' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'colorcorrection3', 'factor' : 1, 'default' : 0.0}],
            'et_colorCorrection4' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'colorcorrection4', 'factor' : 1, 'default' : 0.0}],
            'et_colorCorrection5' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'colorcorrection5', 'factor' : 1, 'default' : 0.0}],
            'et_colorCorrection6' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'colorcorrection6', 'factor' : 1, 'default' : 0.0}],
            'et_colorCorrection7' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'colorcorrection7', 'factor' : 1, 'default' : 0.0}],
            'et_colorCorrection8' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'colorcorrection8', 'factor' : 1, 'default' : 0.0}],
            'et_colorCorrection9' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'colorcorrection9', 'factor' : 1, 'default' : 0.0}],
            'et_colorCorrection10' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'colorcorrection10', 'factor' : 1, 'default' : 0.0}],
            'et_colorCorrection11' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'colorcorrection11', 'factor' : 1, 'default' : 0.0}],
            'et_colorCorrection12' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'colorcorrection12', 'factor' : 1, 'default' : 0.0}],

            'et_laserState' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'laserstate', 'factor' : 1, 'default' : 'on'}],
            'et_laserPower' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'laserpower', 'factor' : 1, 'default' : 330}],
            'et_roiTop' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'roitop', 'factor' : 1, 'default' : 0}],
            'et_roiBottom' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'roibottom', 'factor' : 1, 'default' : 1079}],
            'et_roiLeft' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'roileft', 'factor' : 1, 'default' : 0}],
            'et_roiRight' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'roiright', 'factor' : 1, 'default' : 1919}],

            'et_autoExposure' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'autoexposure-auto', 'factor' : 1, 'default' : 'True'}],
            'et_manualExposure' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'autoexposure-manual', 'factor' : 1, 'default' : 33000}],

            'et_gain' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'gain', 'factor' : 1, 'default' : 16}],
            'et_autoExposureFaceAuthLoops' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'faceauthloops', 'factor' : 1, 'default' : 0}],

            'et_exposureListSize' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposurelistsize', 'factor' : 1, 'default' : 0}],

            'et_exposure0' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure0', 'factor' : 1, 'default' : 0}],
            'et_exposure1' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure1', 'factor' : 1, 'default' : 0}],
            'et_exposure2' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure2', 'factor' : 1, 'default' : 0}],
            'et_exposure3' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure3', 'factor' : 1, 'default' : 0}],
            'et_exposure4' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure4', 'factor' : 1, 'default' : 0}],
            'et_exposure5' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure5', 'factor' : 1, 'default' : 0}],
            'et_exposure6' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure6', 'factor' : 1, 'default' : 0}],
            'et_exposure7' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure7', 'factor' : 1, 'default' : 0}],
            'et_exposure8' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure8', 'factor' : 1, 'default' : 0}],
            'et_exposure9' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure9', 'factor' : 1, 'default' : 0}],
            'et_exposure10' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure10', 'factor' : 1, 'default' : 0}],
            'et_exposure11' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure11', 'factor' : 1, 'default' : 0}],
            'et_exposure12' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure12', 'factor' : 1, 'default' : 0}],
            'et_exposure13' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure13', 'factor' : 1, 'default' : 0}],
            'et_exposure14' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure14', 'factor' : 1, 'default' : 0}],
            'et_exposure15' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure15', 'factor' : 1, 'default' : 0}],
            'et_exposure16' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure16', 'factor' : 1, 'default' : 0}],
            'et_exposure17' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure17', 'factor' : 1, 'default' : 0}],
            'et_exposure18' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure18', 'factor' : 1, 'default' : 0}],
            'et_exposure19' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure19', 'factor' : 1, 'default' : 0}],
            'et_exposure20' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure20', 'factor' : 1, 'default' : 0}],
            'et_exposure21' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure21', 'factor' : 1, 'default' : 0}],
            'et_exposure22' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure22', 'factor' : 1, 'default' : 0}],
            'et_exposure23' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure23', 'factor' : 1, 'default' : 0}],
            'et_exposure24' : [{'regFieldName' : 'not_valid', 'jsonFieldName' : 'exposure24', 'factor' : 1, 'default' : 0}],
            
        }

    def presetWriteVar(self, var, varType, name):
        
        if ('float' == varType):
            varFormat = "%lf"
            varHex = self.float_to_hex(var);
            varInt = int(varHex, 0)
        else:
            varFormat = "%d"
            varInt = int(var) 

        if ('int16' in varType):
            listOffsets = (0,8)
        else:
            listOffsets = (0,8,16,24)

        tempList = [self.outCurrBinFile.write(chr(((varInt >> i) & 0xff))) for i in listOffsets]

        return

    def presetVarHandle(self, varType, varName):
        #print varName

        Val = self.presetVarValueGet('et_' + varName)
        self.presetWriteVar(Val, varType, varName)

        return

    def presetScpWriteVar(self, varType, varName):

        Val32 = 0xDEADBEEF

        dicEntryIndex = 'et_' + varName;
        if (self.dict[dicEntryIndex][0]['regFieldName'] == 'not_valid'):
            Val32 = self.presetVarValueGet('et_' + varName)
        else:
            Val32 = self.presetRegValueGet('et_' + varName)

        if (Val32 != 0xDEADBEEF):
            self.presetWriteVar(Val32, varType, varName)

        return

    def regsFileSearch(self, fieldName):

        for tempLine in self.RegsFileLines:
            if (fieldName in tempLine):
                #print tempLine
                break
        
        return tempLine

    def regsFieldValueGet(self, entry):

        regsFieldName = entry['regFieldName']
        jsonFieldName = entry['jsonFieldName']
        factor = entry['factor']

        fieldLine = self.regsFileSearch('uint32_t ' + regsFieldName)
        bitsString = fieldLine.split("Bits :[")[1].split("]")[0]
        shiftBit = int(bitsString.split(":")[0])
        lastBit = int(bitsString.split(":")[1])
        mask = ((1 << (lastBit - shiftBit + 1)) - 1)

        fieldFloat = float(self.presetJsonSearch(jsonFieldName, entry['default']))
        if (factor == -1):
            fieldFloatFactor = (~(int(fieldFloat)))
        else:
            fieldFloatFactor = fieldFloat * factor
        fieldValInt = ((int(fieldFloatFactor) & mask) << shiftBit)

        return fieldValInt


    def presetVarValueGet(self, varName):
        jsonFieldName = self.dict[varName][0]['jsonFieldName']  # single field
        defaultVal = self.dict[varName][0]['default']

        res = self.presetJsonSearch(jsonFieldName, defaultVal)
        if ((jsonFieldName ==  "laserstate") or (jsonFieldName ==  "autoexposure-auto")):
            regVal = self.StatesDict[res]
        else:
            regVal = float(res)
        return regVal

    def presetRegValueGet(self, regName):
        regVal = 0
        for entry in self.dict[regName]:
            regFieldVal = self.regsFieldValueGet(entry)
            regVal = regVal | regFieldVal
        return regVal
    
    def presetScpHandle(self):
        with open("./Input/Scp.h", "r") as scpFile:
            lines = scpFile.readlines()

        for line in lines:
            #print line            
            line = line.strip()
            varType = line.split(" ")[0].strip()

            if ((varType == "float") or (varType == "uint32_t") or (varType == "int32_t") 
                or ("TRegScp" in varType) or ("ET" in varType)):
                size = len(line.split(" "))
                varName = line.split(" ")[size - 1].split(";")[0]
                self.presetScpWriteVar(varType, varName)
        return

    def float_to_hex(self, f):
        return hex(struct.unpack('<I', struct.pack('<f', f))[0])

    def presetJsonSearch(self, fieldName, defaultVal):
        fieldVal = defaultVal

        for tempLine in self.jsonFileLines:
            if (fieldName in tempLine):
                fieldVal = tempLine.split(":")[1].strip().split(",")[0]
                break

        return fieldVal

    def presetColorCorrectionHandle(self):
        for idx in range(1, 13):
            self.presetVarHandle('float', 'colorCorrection' + str(idx))
        
        return

    def presetExposureListHandle(self, varType, name):
        if (name == 'exposureListSize'):
            self.presetVarHandle(varType, name)
        else:
            for idx in range(0, 25):
                self.presetVarHandle(varType, 'exposure' + str(idx))
        return

    def PresetNamePrint(self, presetName):

        presetNameLen = len(presetName)
        for i in range(0, presetNameLen):
            self.outCurrBinFile.write(presetName[i])

        for i in range(presetNameLen, self.PRESET_NAME_MAX_LEN):
            self.outCurrBinFile.write('\0')

    def PresetJsonFileParse(self):

        self.jsonFile = open(self.JsonFileName,"r")
        self.jsonFileLines = self.jsonFile.readlines()
        self.RegsFile = open(self.RegsFileName,"r")
        self.RegsFileLines = self.RegsFile.readlines()

        #print os.getcwd()
        presetName = os.path.splitext(basename(self.JsonFileName))[0]
        print presetName
        
        currBinFilePath = "./Output/" + presetName + ".bin"
        self.outCurrBinFile = open(currBinFilePath,"wb+")

        with open("./Input/Preset.h", "r") as f:
            lines = f.readlines()

        for line in lines:
            #print line
            
            line = line.strip()
            varType = line.split(" ")[0].strip()
            
            if (varType == "TPresetName"):
                self.PresetNamePrint(presetName)

            elif (varType == "STScpParams"):
                self.presetScpHandle()

            elif ("int" in varType):    #int32_t, uint16_t, uint32_t 
                varName = line.split(" ")[1].split(";")[0].strip()
                if ("exposureList" in varName):
                    self.presetExposureListHandle(varType, varName)  # not supported yet
                else:
                    self.presetVarHandle(varType, varName)

            elif ("T_COLOR_CORRECTION_MATRIX_FLOAT" in varType):
                self.presetColorCorrectionHandle()


        self.outCurrBinFile.close()
        return 0

    def PresetHeaderFileGenerate(self, directory):

        print "input jsons folder: " + directory


        for filename in os.listdir(directory):
            #print filename
            if (filename.endswith(".json")): 
                self.JsonFileName = directory + '/' + filename
                self.PresetJsonFileParse()

        return