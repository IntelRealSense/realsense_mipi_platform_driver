import sys
import os
from Presets import Preset


def main():

    print("Json to Preset H-file ...")

    preset = Preset()

    paramsNum = len(sys.argv) 
    if (paramsNum > 1):
        directory = sys.argv[1]
        #print directory
        preset.PresetHeaderFileGenerate(directory)
    else:
        directory = './Input/Jsons/'
        preset.PresetHeaderFileGenerate(directory)

    print("Done")

if __name__ == "__main__":
    main()
                        