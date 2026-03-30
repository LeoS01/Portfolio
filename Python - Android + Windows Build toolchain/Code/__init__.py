"""
This toolchain is designed to 'merge' a source folder into the output.
The output name is dependent on the folder it is called from~ please ensure you execute your build-script from the same place!
"""

from pathlib import Path
import xml.etree.ElementTree  as ET
import os
import sys
import stat
import re
import subprocess

#Interact with the main build process over this class
class Config:
    #Directory of this abs-path/../.. = Engine directory local
    #(local path required to work for engine-copy)
    def GetEngineDir() -> str:
        return f"{os.path.dirname(os.path.abspath(__file__))}\\..\\.."
    
    def GetEngineIncl() -> str:
        return Config.GetEngineDir() + "\\Include"

    outputDirectory = "."
    def SetOutputDirectory(newOutputDir:str) -> str:
        Log(False).Info("New build directory: " + newOutputDir)
        Config.outputDirectory = newOutputDir
        return Config.outputDirectory
    
    def GetOutputName(suffix = "") -> str:
        return Path.cwd().name.replace(" ", "") + suffix

#Misc
def AutoDependencyInstall(libs: list[str]) -> int:
    #Log
    print("Seems like youre missing")
    for lib in libs:
        print(lib)

    #Choose
    print("Do you wish to install them? (Y/N)")
    rslt = input()
    if "n" in rslt or "N" in rslt:
        print("Aborted")
        exit(0)
    elif "y" in rslt or "Y" in rslt:
        for lib in libs:
            print(f"Installing {lib}")
            subprocess.run(f"pip install {lib}", shell=True)
        print("Installed libraries!")
        exit(0)

    #Invalid
    print("unknown answer: " + rslt)
    exit(-1)

def RunCMD(cmd:str):
    rslt = subprocess.run(cmd, text=True, capture_output=True, shell=True)
    if rslt.returncode != 0:
        Log(False).Error("Full command: " + cmd.replace(" && ", "\n&&\n") + "\n")
        Log(False).Error(rslt.stdout + "\n" + rslt.stderr)
        raise RuntimeError("Shell Error")

#CONST
class ANSI:
    #https://gist.github.com/JBlond/2fea43a3049b38287e5e9cefc87b2124
    RED    =   "\x1b[0;31m"
    GREEN  =   "\x1b[0;32m"
    YELLOW =   "\x1b[0;33m"
    PURPLE =   "\x1b[0;35m"
    CYAN   =   "\x1b[0;36m"
    RESET   =  "\x1b[0;37m"

class Log:
    def __init__(self, verbose: bool):
        self.verbose = verbose

    def VHead(self, msg: str):
        if self.verbose:
            HEADER = ANSI.PURPLE + "\n---"
            print(HEADER + msg + ANSI.RESET, file=sys.stderr)

    def VMessage(self, msg: str):
        if self.verbose:
            print(ANSI.CYAN + msg + ANSI.RESET, file=sys.stderr)

    def Head(self, msg: str):
        BAR:str = "---------"
        print(ANSI.PURPLE + f"\n\n{BAR}" + msg + BAR + ANSI.RESET, file=sys.stderr)

    def Error(self, msg: str):
        print(ANSI.RED + msg + ANSI.RESET, file=sys.stderr)

    def Success(self, msg: str):
        print(ANSI.GREEN + msg + ANSI.RESET, file=sys.stderr)

    def Info(self, msg: str):
        print(ANSI.CYAN + msg + ANSI.RESET, file=sys.stderr)

    def Warning(self, msg:str):
        print(ANSI.YELLOW + msg + ANSI.RESET, file=sys.stderr)

class Files:
    SUFFIX_ARCHIVE:str = ".p_assets"
    OBJENDING: str = ".obj"
    RESENDING: str = ".res"
    INTERMEDIATEFOLDERNAME:str = "intermediate"
    ANDROIDMANIFESTNAME: str = "AndroidManifest.xml"
    ANDROID_PLATFORM_VERSIONLESS: str = os.path.expandvars("%ANDROID_HOME%\\platforms\\android-")

    def IsPathHidden(path: str):
        #https://pytutorial.com/python-sysplatform-detecting-the-operating-system/
        #https://docs.python.org/3/library/stat.html
        #https://learn.microsoft.com/en-us/windows/win32/fileio/file-attribute-constants?redirectedfrom=MSDN

        #Alloc
        startsHidden:bool = Path(path).name.startswith('.') 

        #Non Win
        if sys.platform != "win32":
            return startsHidden

        #Win
        return startsHidden or os.stat(path).st_file_attributes & stat.FILE_ATTRIBUTE_HIDDEN
        
    def EnsurePathExists(directoryAbs: str):
        dir: Path = Path(directoryAbs)
        if not dir.exists():
            os.makedirs(dir)

    #Returns reason for recreation:str or None
    def IsOutputUpToDate(outputFile:str, inputFiles:list[str]) -> bool:
        #Absent?
        oPath = Path(outputFile)
        if not Path.exists(oPath):
            return False
        
        #Up to date?
        oPathFull = oPath.resolve()
        for file in inputFiles:
            if os.path.getmtime(file) > os.path.getmtime(oPathFull):
                return False
        
        #No need
        return True
        
    #Either stdin or cwd glob
    def GetAutoInputs(suffix:str, recursive:bool) -> list[str]:
        #https://stackoverflow.com/questions/1450393/how-do-i-read-from-stdin
        #https://askubuntu.com/questions/66195/what-is-a-tty-and-how-do-i-access-a-tty
        #Alloc
        inputFiles: list[str] = []

        #Stdin
        if not sys.stdin.isatty():
            for line in sys.stdin.read().splitlines(): 
                inputFiles.append(line)
        #Success?
        if len(inputFiles) > 0:
            return inputFiles

        #Glob
        globFunc = Path(".").rglob if recursive else Path(".").glob
        for file in globFunc("*" + suffix):
            inputFiles.append( str(file.resolve()) )
        #Success?
        if len(inputFiles) > 0:
            return inputFiles
        
        #Failed
        return []
    
    #Splits "directory/*.file" into ["directory", "*.file"]
    def SplitDirPattern(dirPattern:str) -> tuple[str, str]:
        searchDir:str = "."
        searchFile:str = "*"

        sPath = Path(dirPattern)
        if sPath.is_dir():
            searchDir = dirPattern
        else:
            searchDir = sPath.parent.resolve()
            searchFile = sPath.name

        return [searchDir, searchFile]
    
    #Returns AndroidManifest.xml in cwd or engine
    def GetAnAndroidManifestPath(dir:str, recursive:bool = True) -> str:

        #Pointing to a file already?
        if not Path(dir).is_dir():
            dir = str(Path(dir).parent)

        #Local?
        lglob = Path(dir).rglob if recursive else Path(dir).glob
        for file in lglob(Files.ANDROIDMANIFESTNAME):
            if not file.is_dir():
                path = file.resolve()
                return str(path)
        
        raise Exception(f"Cannot find any 'AndroidManifest.xml' in '{dir}'")

    #Get Platform-Directory from a manifest
    def GetAndroidPlatformVersion(androidXMLDirectory: str) -> str:
        #Validate
        if not Path(androidXMLDirectory).exists():
            return "XML Not present"
        
        #Alloc
        fullNSPrefix:str = "{http://schemas.android.com/apk/res/android}"

        #Get
        tree = ET.parse(androidXMLDirectory)

        rslt = tree.find("uses-sdk",)
        if rslt is None:
            return "sdk-node not found"
        
        rslt = rslt.get(fullNSPrefix + "minSdkVersion")
        if rslt is None:
            return "version-attribute not found"
        
        #Finalize
        return rslt
        
    #Non recursive, assumes versioning syntax with '.' seperator e.G. 0.2.4
    def GetFolderWithHighestMajorVersion(directory:str) -> str:
        #https://docs.python.org/3/library/re.html#re.search
        current: tuple[str, int] = ["", -1]

        for folder in Path(directory).glob("*"):
            if not folder.is_dir():
                continue

            version:int = int(re.search(r"\d+", folder.name).group())
            if version > current[1]:
                current[0] = str(folder.resolve())
                current[1] = version

        return current[0]

    class AndroidSDKUtil:
        def __init__(self):
            #https://developer.android.com/ndk/guides/other_build_systems
            #https://developer.android.com/ndk/guides/abis

            #Core
            self.androidABIs:list[str] = ["aarch64", "armv7a", "i686", "x86_64"]
            self.abiFolderMap:dict[str, str] = {
                "aarch64": "arm64-v8a",
                "armv7a": "armeabi-v7a",
                "i686": "x86",
                "x86_64": "x86_64"
            }
            self.androidHome:str = os.path.expandvars("$ANDROID_HOME") 
            self.androidXMLPath:str = Files.GetAnAndroidManifestPath(Config.outputDirectory)
            self.androidVersion:str = Files.GetAndroidPlatformVersion(self.androidXMLPath)
            self.ndkDir:str = Files.GetFolderWithHighestMajorVersion(self.androidHome + "\\ndk")
            self.androidPlatform:str = Files.ANDROID_PLATFORM_VERSIONLESS + self.androidVersion
            self.platformTools:str = self.androidHome + "\\platform-tools"
            self.buildTools:str = Files.GetFolderWithHighestMajorVersion(self.androidHome + "\\build-tools")

            #Clang path
            androidLLVMFolder:str = ""
            match sys.platform:
                case "win32":
                    androidLLVMFolder = "windows-x86_64"
                case "linux":
                    androidLLVMFolder = "linux-x86_64"
                case "darwin":
                    androidLLVMFolder = "darwin-x86_64"
            if androidLLVMFolder == "":
                raise Exception("NDK not found on " + sys.platform)
            
            self.compilerPath:str = self.ndkDir + "\\toolchains\\llvm\\prebuilt\\" + androidLLVMFolder + "\\bin\\clang++"
            if not Path(self.compilerPath).parent.exists():  #<- checking only parent as clang suffix may change per OS
                raise Exception("Android compiler not found in " + self.compilerPath)