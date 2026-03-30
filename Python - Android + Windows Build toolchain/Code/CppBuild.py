HELPMSG="""
Compiles a set of .cpp files to .obj and then links all .obj (windows: and .rc) files into a (windows: .exe, android: .so)
Files can be delivered via -i or via stdin/pipes
Depends on external build-tools, documented in the engine root folder.
""".strip()

import os
import argparse
import sys
from pathlib import Path
from P_Build import Log, Files, RunCMD, Config


#Vars
#https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-170
VCVARCONTEXT = "vcvars32"


#Core interface
def CppLink(platform:str, objFolder:str, verbose:bool) -> str:
    #https://learn.microsoft.com/en-us/cpp/build/reference/linking?view=msvc-170

    #Init
    stepName = ".obj to binary"
    Log(False).Head(stepName)
    outDir = Config.outputDirectory

    #Alloc
    log = Log(verbose)
    expectedOutPath: str = outDir + "\\" + Config.GetOutputName()

    #Get command and vars
    log.VMessage("Using platform: " + platform)
    linkCommand: str = ""
    match platform:
        case "win32":
            #Get
            oPath = Path(outDir)
            log.VMessage(f"Searching recursively in {oPath.name} for {Files.OBJENDING} and {Files.RESENDING}")
            allFiles = list(oPath.rglob("*" + Files.OBJENDING)) + list(oPath.rglob("*" + Files.RESENDING))

            #Construct
            sourceFiles:str = ""
            for file in allFiles:
                if file.is_dir():
                    continue
                
                sourceFiles += f'"{file.resolve()}" '

            #Apply
            expectedOutPath += ".exe"
            linkCommand = VCVARCONTEXT + " && " + f'link {sourceFiles} -out:"{expectedOutPath}"'

        case "android":
            #Get
            ndk = Files.AndroidSDKUtil()
            links = "-static-libstdc++ -llog -lEGL -landroid -lOPenSLES -lGlesv3 -laaudio"

            #Loop API
            for abi in ndk.androidABIs:
                #Validate operation
                objDir:str = objFolder + "\\" + ndk.abiFolderMap[abi]
                if not Path(objDir).exists():
                    log.VMessage(f"{objDir} does not exist, skipping...")   #<- expected to happen on debug builds
                    continue

                #Validate Directory
                soDir = outDir + "\\" + stepName
                Files.EnsurePathExists(soDir)

                #Assemble
                if linkCommand != "":
                    linkCommand += " &&"
                expectedOutPath = soDir + f"\\{ndk.abiFolderMap[abi]}.so"   #<- overwrite ok, checking against one should suffice
                linkCommand += ndk.compilerPath + f' --target={abi + "-linux-android" + ndk.androidVersion} -shared {links} -o "{expectedOutPath}" "{objFolder + "\\" + ndk.abiFolderMap[abi] + "\\*" + Files.OBJENDING}"'

    if linkCommand == "":
        log.Error("Unimplemented platform: " + platform)
        raise Exception("Invalid platform")

    #Validate
    try:
        #Exists?
        if not Path(expectedOutPath).exists():
            log.VMessage(f"{Path(expectedOutPath).name} does not exist, linking!")
            raise Exception()
        
        #Newer file?
        outMTime: float = os.path.getmtime(expectedOutPath)
        targetFilest: list[Path] = list( Path(objFolder).rglob("*" + Files.OBJENDING) ) + list( Path(objFolder).rglob("*" + Files.RESENDING) )
        
        for file in targetFilest:
            if os.path.getmtime( str(file.resolve()) ) > outMTime:
                log.VMessage(f"{file.name} is newer than {Path(expectedOutPath).name}, relinking!")
                raise Exception()
            log.VMessage(file.name + " is older than " + Path(expectedOutPath).name)
            
        log.Info("Aborting, everything up to date!")
        return expectedOutPath
    except:
        pass
            
    #Run
    Files.EnsurePathExists(Path(expectedOutPath).parent)
    RunCMD(linkCommand)

    #Finalize
    log.Success(f'Linked to {Path(expectedOutPath).suffix}!')
    return expectedOutPath

def CppComp(platform:str, inputFiles:list[str], verbose:bool, release:bool = False) -> str:
    #Init
    stepName:str = ".cpp to .obj"
    Log(False).Head(stepName)
    log = Log(verbose)

    #Validate
    if inputFiles is None or len(inputFiles) <= 0:
        log.Info("No input files found, aborting")
        return ""
    
    outDir =  Config.outputDirectory + "\\" + stepName
    Files.EnsurePathExists(outDir)

    #Loop
    log.VMessage(f"Exporting for platform: {platform}")
    for cppFile in inputFiles:
        #Win:
        #https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-1-c4530?view=msvc-170
        #https://learn.microsoft.com/en-us/cpp/build/reference/compiler-command-line-syntax?view=msvc-170

        #Android:
        #https://stackoverflow.com/questions/5311515/gcc-fpic-option
        #https://developer.android.com/ndk/guides/cpp-support
        #https://developer.android.com/ndk/guides/stable_apis
        #https://developer.android.com/ndk/guides/other_build_systems

        #Flag
        #https://clang.llvm.org/docs/ClangCommandLineReference.html

        #Alloc
        rawCPP: str = cppFile.replace('"', '')
        fileName: str = Path(rawCPP).name
        objName: str = fileName + Files.OBJENDING
        keywords: str = f"{"-DNDEBUG" if release else "-DDEBUG"}"
        cppVersion:str = "20"

        #Don't date-validate .cpp files as their includes may be newer than the associated .obj

        #Get command
        log.VMessage("\nprocessing .cpp")
        command:str = ""
        match platform:
            case "win32":
                objOutDir: str = str(Path.cwd().resolve()) + "\\" + outDir
                Files.EnsurePathExists(objOutDir)

                #Do
                keywords += " -DWIN32 -D_WIN32"
                command = VCVARCONTEXT + " && " + f'cl -std:c++{cppVersion} -c -EHsc' + f' "{rawCPP}" {keywords} -I "{Config.GetEngineIncl()}" -Fo:"{objOutDir + "\\" + objName}"'     

            case "android":
                ndk = Files.AndroidSDKUtil()
                
                #Assemble full command
                for abi in ndk.androidABIs:
                    log.VMessage(f"Compiling for {abi}")
                    #Validate
                    objOutDir: str = outDir + "\\" + ndk.abiFolderMap[abi]
                    Files.EnsurePathExists(objOutDir)

                    #Config
                    flags:str = f'-std=c++{cppVersion} -fpic -c -I "{Config.GetEngineIncl()}" -I {ndk.ndkDir}\\sources\\android\\native_app_glue -fsigned-char'  #Char unsigned by default~ lets change that!
                    if abi == "aarch64":
                        flags += " -ffixed-x18"

                    #Do
                    keywords += " -D__ANDROID__"
                    if command != "":
                        command += " && "
                    command += ndk.compilerPath + f' --target={abi + "-linux-android" + ndk.androidVersion} {flags} {keywords} -o "{objOutDir + "\\" + objName}" "{rawCPP}"'
                    
                    #Finalize
                    if not release:
                        log.VMessage("Compiling only for " + abi)   #This kinda assumes that the debug-device is arm64 based
                        break
                
        if command == "":
            log.Error("Unknown platform: " + platform)
            raise Exception("Invalid Platform")

        #Execute cmd
        RunCMD(command)

        #Finalize
        log.Success(f"Compiled {fileName} into {objName}")

    return outDir


def CppBuild(platform:str, inputFiles:list[str], verbose:bool, release:bool = False) -> str:
    objFolder:str = CppComp(platform, inputFiles, verbose, release)
    if objFolder == "":
        return ""
    
    return CppLink(platform, objFolder, verbose)

#Main
if __name__ == "__main__":

    #Config args
    argprs = argparse.ArgumentParser(
        prog="CppBuild",
        description=HELPMSG
    )

    argprs.add_argument("-v", "--verbose", action="store_true", help="Log detailed output")
    argprs.add_argument("-o", "--output", action="store", help="Set the output DIRECTORY")
    argprs.add_argument("-r", "--recursive", action="store_true", help="Search cwd recursively?")
    argprs.add_argument("-plt", "--platform", action="store", help="Set target PLATFORM (e.G. 'win32' or 'android')")

    #Get args
    args = argprs.parse_args(sys.argv[1:])
    verbose: bool = args.verbose
    recursive: bool = args.recursive
    release: bool = False #'Release' software is ought to have a more elaborate build-pipeline
    outDir: str = args.output if (args.output != None) else "."
    plt: str = args.platform if (args.platform != None) else sys.platform
    log = Log(verbose)

    #Get input
    inputFiles: list[str] = Files.GetAutoInputs(".cpp", recursive)

    #Log
    log.VHead("Arguments:")
    log.VMessage(f"verbose = {verbose}")
    log.VMessage(f"recursive = {recursive}")
    log.VMessage(f"platform = {plt}")
    log.VMessage(f"inputFiles ({len(inputFiles)}):")
    for file in inputFiles:
        log.VMessage("  " + file)
    log.VMessage(f"outDir = {outDir}")

    #Do
    Config.SetOutputDirectory(outDir)
    print( CppBuild(plt, inputFiles, verbose, release) )  
    exit(0)
