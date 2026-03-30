HELPMSG = """
Builds either an .apk or an .aab and runs it on an attached device.
Requires compiled .so and .class files in the input directory (if using AndBuild).
Note: debug keystore.jks files use 'password' as a password and 'pDebug' as an alias.
""".strip()

from pathlib import Path
import subprocess
import argparse
import sys
import os
import datetime
import zipfile
from P_Build import Files, Log, RunCMD, Config

def CompileClassToDex(verbose:bool, recursive:bool = True, release:bool = False) -> str:
    #https://developer.android.com/tools/d8

    #Init
    stepName = ".class to .dex"
    Log(False).Head(stepName)

    #Alloc
    log = Log(verbose)
    sdk = Files.AndroidSDKUtil()
    allFiles: list[str] = []
    fullClasses = ""
    inputDir = Config.outputDirectory
    outputDir = Config.outputDirectory + "\\" + stepName
    expectedOutput = outputDir + "\\classes.dex"

    #Get all files
    globFunc = Path(inputDir).rglob if recursive else Path(inputDir).glob
    for file in globFunc("*.class"):
        if file.is_dir():
            continue

        fdir = str(file.resolve())
        fullClasses += f'"{fdir}" '
        allFiles.append(str(file.resolve()))

    #Validate
    if fullClasses == "":
        log.Error("No .class files found!")
        raise Exception("No .class files!")
    
    if Files.IsOutputUpToDate(expectedOutput, allFiles):
        log.Info(".dex up to date, returning!")
        return expectedOutput

    #Do
    Files.EnsurePathExists(outputDir)
    flags = "--release" if release else "--debug"
    fullCmd = f'"{sdk.buildTools + '\\d8'}"' + f' {fullClasses} {flags} --output "{outputDir}" --lib "{sdk.androidPlatform}\\android.jar"'
    RunCMD(fullCmd)
    
    #Finalize
    log.Success(f'Compiled classes.dex from {Path(inputDir).name}!')
    return expectedOutput

def LinkToAPK(verbose:bool, recursive:bool = True) -> str:
    #https://developer.android.com/studio/projects/add-native-code
    #https://developer.android.com/tools/aapt2
    #https://docs.python.org/3/library/zipfile.html
    #https://developer.android.com/guide/topics/resources/providing-resources
    #https://developer.android.com/tools/zipalign
    #https://systemweakness.com/inside-the-android-app-bdf0af7e457f

    #Init
    stepName = "linking .apk"
    Log(False).Head(stepName)

    #Alloc
    log = Log(verbose)
    sdk = Files.AndroidSDKUtil()

    inputDir = Config.outputDirectory
    outputDir = Config.outputDirectory + "\\" + f"{stepName}"
    uOut:str = f"{outputDir}\\unaligned.apk"
    finOut:str = f"{outputDir}\\aligned.apk"

    sourceFiles:list[str] = []

    #Get all sources
    def AddSrc(pattern:str):
        glob = Path(inputDir).rglob if recursive else Path(inputDir).glob
        for file in glob(pattern):
            if file.is_dir():
                continue
            sourceFiles.append( str(file.resolve()) )

    AddSrc("*.dex")
    AddSrc("*.flat")
    AddSrc("*.so")
    AddSrc("*" + Files.SUFFIX_ARCHIVE)

    #Validate
    if Files.IsOutputUpToDate(finOut, sourceFiles):
        log.Info(".apk up to date, aborting!")
        return finOut

    #Construct command
    newPackageName = "pengine.pinstance." + Config.GetOutputName()
    rename = f"--rename-manifest-package {newPackageName} --rename-resources-package {newPackageName}"
    resourceCommand:str = ""
    for file in sourceFiles:
        fpath = Path(file)
        if not fpath.suffix == ".flat":
            continue

        log.VMessage(f"Appending {fpath.name}")
        resourceCommand += f'-R "{str(fpath.resolve())}" '

    #Run Command (src -> .apk)
    Files.EnsurePathExists(outputDir)
    log.VMessage("\nBuilding base .apk")
    fullCommand = f'{sdk.buildTools}\\aapt2 link --auto-add-overlay -o "{uOut}" --manifest "{sdk.androidXMLPath}" -I "{sdk.androidPlatform}\\android.jar" {rename} {resourceCommand}'
    RunCMD(fullCommand)
    log.VMessage("base .apk was built")

    #Loop
    log.VMessage("\nAdding assets and libs")
    apkZIP = zipfile.ZipFile(uOut, "a")
    for file in sourceFiles:
        pth = Path(file)
        match pth.suffix:
            case ".so":
                apkZIP.write(file, f"lib\\{pth.stem}\\libmain.so")

            case Files.SUFFIX_ARCHIVE:
                apkZIP.write(file, f"assets\\{pth.name}")

            case ".dex":
                apkZIP.write(file, f"{pth.name}")

            case "_":
                continue

        log.VMessage(f"Appended {pth.name}")
    apkZIP.close()

    #Align .apk
    log.VMessage("\nAligning")
    RunCMD(f'{sdk.buildTools}\\zipalign -f -P 16 4 "{uOut}" "{finOut}"')

    #Finalize
    log.Success("Built base .apk!")
    return finOut

def SignFile(filePath:str, keyDir:str, verbose:bool, release:bool = False) -> str:
    #https://developer.android.com/tools/apksigner
    #https://docs.oracle.com/javase/8/docs/technotes/tools/windows/jarsigner.html
    
    #Init
    suffix = Path(filePath).suffix
    Log(False).Head(f"{suffix} -> signed {suffix}")

    #Alloc
    log = Log(verbose)
    sdk = Files.AndroidSDKUtil()
    outFile: str = Config.outputDirectory + "\\" + Config.GetOutputName(suffix)
    fPath = Path(filePath)

    #Validate in
    if Files.IsOutputUpToDate(outFile, [filePath]):
        log.Info(f"{Path(outFile).name} is up to date, aborting!")
        return outFile

    #Do
    log.VMessage(f"Signing {fPath.name} with {Path(keyDir).name}")
    Files.EnsurePathExists(Config.outputDirectory)

    match suffix:
        case ".apk":
            log.VMessage("Signing .apk")
            if release:
                #TODO: Test a proper .apk release build!
                log.Info("Youre trying to release a .apk~ as of now, this is untedsted!")

            debug = "" if release else "--ks-pass pass:password --key-pass pass:password"
            RunCMD(f'{sdk.buildTools}\\apksigner sign --ks "{keyDir}" --out "{outFile}" {debug} "{filePath}"')
            
            #Validate out
            if verbose:
                log.VMessage("\nVerifying signature:")
                rslt = subprocess.run(f"{sdk.buildTools}\\apksigner verify -v {outFile}", shell=True, capture_output=True, text=True)
                log.VMessage(f"{rslt.stdout}\n{rslt.stderr}")

        case ".aab":
            log.VMessage("Signing .aab")
            debugFlags = f"{input("What is your key alias?\n")}" if release else "-storepass password -keypass password pDebug"
            print("And your password?", file=sys.stderr)

            RunCMD(f'jarsigner -keystore "{keyDir}" -signedjar "{outFile}" "{filePath}" {debugFlags}')

            if verbose:
                log.VMessage("\nVerifying signature:")
                rslt = subprocess.run(f"jarsigner -verify -verbose {outFile}", shell=True, capture_output=True, text=True)
                log.VMessage(f"{rslt.stdout}\n{rslt.stderr}")

        case _:
            log.Error(f"Unimplemented type: {suffix}")
            return ""


    #End
    log.Success(f"Signed {suffix}!")
    return outFile

def ConvertAPKtoAAB(inputFile:str, verbose:bool) -> str:
    #https://developer.android.com/tools/aapt2
    #https://developer.android.com/build/building-cmdline#bundletool-build

    #Init
    stepName = ".apk to .aab"
    Log(False).Head(stepName)

    #Alloc
    log = Log(verbose)
    sdk = Files.AndroidSDKUtil()
    iPath = Path(inputFile)

    outputDir = Config.outputDirectory + "\\" + stepName
    intermediate:str = outputDir + "\\" + Files.INTERMEDIATEFOLDERNAME + datetime.datetime.now().strftime("%Y%m%d%H%M%S")
    aabOut = outputDir + "\\" + iPath.stem + ".aab"
    Files.EnsurePathExists(intermediate)

    #Validate
    if Files.IsOutputUpToDate(aabOut, [inputFile]):
        log.Info(".aab is up to date, returning!")
        return aabOut

    #Proto
    log.VMessage("\n.apk to protobuf")
    protOut = intermediate + "\\" + iPath.name + ".proto"
    RunCMD(f'{sdk.buildTools}\\aapt2 convert -o "{protOut}" --output-format proto "{inputFile}" ')

    #Basemodule
    log.VMessage("\naabase.zip assembly")
    with zipfile.ZipFile(protOut) as proto:
        proto.extractall(intermediate)

    #add .proto suffix (doesn't mark them as 'source' files for other steps)
    for file in Path(intermediate).rglob("*"):
        if file.suffix == ".proto" or file.is_dir():
            continue
        
        file.rename(f"{file.resolve()}.proto")


    aaBase:str = intermediate + "\\aabase.zip"
    if Path(aaBase).exists():
        os.remove(aaBase)
        print("Removed old .aab!")

    with zipfile.ZipFile(aaBase, "w") as zip:
        #Core
        log.VMessage("Adding base")
        zip.write(intermediate + "\\AndroidManifest.xml.proto", "manifest/AndroidManifest.xml")
        zip.write(f"{intermediate}\\resources.pb.proto", "resources.pb")
        zip.write(f"{intermediate}\\classes.dex.proto", "dex\\classes.dex")


        for path in Path(intermediate).glob("*"):
            if not path.is_dir():
                continue

            log.VMessage(f"Adding from {path.relative_to(intermediate)}")
            for file in path.rglob("*.proto"):
                if file.is_dir():
                    continue

                zip.write(file.resolve(), f"{file.relative_to(intermediate).parent}\\{file.stem}")

    
    log.VMessage("\nbase.zip to aab!")

    if Path(aabOut).exists():
        log.VMessage("Removing old .aab")
        os.remove(aabOut)
    
    RunCMD(f'java -jar {sdk.androidHome}\\bundletool.jar build-bundle --modules="{aaBase}" --output="{aabOut}"')

    log.Success("Built .aab!")
    return aabOut

#Installs and returns run and close commands
def InstallAndRun(appDir:str, key:str, verbose:bool = True, activityName:str = "P_App._Android.PMainActivity") -> tuple[str, str]:
    #TODO: A shortcoming here is the necessity to always reinstall~
    #However, the engine is designed around mainly developing/testing on a desktop-OS~ thus this won't impose a large issue for now.
    
    #https://developer.android.com/tools/adb#move
    #https://developer.android.com/tools/adb#am
    #https://stackoverflow.com/questions/4567904/how-to-start-an-application-using-android-adb-tools
    #https://developer.android.com/tools/bundletool

    apPath = Path(appDir)
    suffix = Path(appDir).suffix
    stepName = f"run {suffix}"
    Log(False).Head(stepName)

    #Alloc
    log = Log(verbose)
    sdk = Files.AndroidSDKUtil()
    adb = f"{sdk.platformTools}\\adb"

    #Check for devices
    cmd = f"{adb} devices"
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    amountOfDevices = len(result.stdout.strip().split('\n')) - 1
    log.VMessage(f"Found {amountOfDevices} devices!")
    if amountOfDevices <= 0:
        log.Error(f"No devices attached, not running {apPath.name}")
        return

    #Install
    match suffix:
        case ".apk":
            log.VMessage(f"Installing apk {apPath.name}")
            RunCMD(f"{adb} install -r {appDir}")

        case ".aab":
            log.VMessage("Installing .aab!")
            apksOut:str = f"{apPath.parent}\\{apPath.stem}.apks"
            log.VMessage(f"Key in question: '{key}'")
            alias = input("What is your key-alias?\n")
            pwd = input("\nAnd your key-password?\n")
            RunCMD(f'java -jar {sdk.androidHome}\\bundletool.jar build-apks --connected-device --bundle="{appDir}" --output="{apksOut}" --overwrite --ks="{key}" --ks-key-alias="{alias}" --ks-pass=pass:"{pwd}"')
            RunCMD(f'java -jar {sdk.androidHome}\\bundletool.jar install-apks --apks="{apksOut}"')

        case _:
            log.Error(f"Unknown app type: {suffix}")
            return

    #Run
    log.VMessage("\nStarting app")
    startCmd = f"{adb} shell am start -n pengine.pinstance.{apPath.stem}/{activityName}"
    logCatCmd = f"{adb} logcat -c && {adb} logcat -s pengine.log"
    return [f"{startCmd} && {logCatCmd}", f"{adb} kill-server"]


def AndBuild(keyDir:str, verbose:bool = True, release: bool = False) -> str:
    CompileClassToDex(True, True, release)
    outPut = LinkToAPK(verbose)

    if release:
        outPut = ConvertAPKtoAAB(outPut, verbose)

    return SignFile(outPut, keyDir, verbose, release)

if __name__ == "__main__":

    #Config args
    argprs = argparse.ArgumentParser(
        prog="CppBuild",
        description=HELPMSG
    )

    argprs.add_argument("-v", "--verbose", action="store_true", help="Log detailed output")
    argprs.add_argument("-io", "--inoutdir", action="store", help="Set the input and output DIRECTORY")
    argprs.add_argument("-ks", "--keydir", action="store", help="Set target keystore.jks file!")

    #Get args
    args = argprs.parse_args(sys.argv[1:])
    verbose: bool = args.verbose
    release: bool = False
    iodir: str = args.inoutdir if (args.inoutdir != None) else "."
    key: str = args.keydir

    #Log
    log = Log(verbose)
    log.VHead("Arguments:")
    log.VMessage(f"verbose = {verbose}")
    log.VMessage(f"release = {release} ('Release' software is ought to have a more elaborate build-pipeline)")
    log.VMessage(f"iodir = {iodir}")
    log.VMessage(f"key = {key}")

    if not key or not Path(key).exists():
        log.Error(f"Key: '{key}' is invalid!")
        exit(-1)

    Config.SetOutputDirectory(iodir)
    out = AndBuild(key, verbose, release)
    if out != "":
        commands = InstallAndRun(out, key, verbose)

        #Run
        try:
            RunCMD(commands[0])
        except KeyboardInterrupt:
            log.Info("Ended!")
        except Exception as e:
            log.Error(str(e))

        RunCMD(commands[1])

    print(out)
    exit(0)