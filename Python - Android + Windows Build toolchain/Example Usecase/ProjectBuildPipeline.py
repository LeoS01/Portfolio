import sys
import P_Ex
import subprocess

from pathlib import Path
from P_Build import Log, Config
from P_Build.AndMnfst import AndMnfst
from P_Build.WinRes import WinRes
from P_Build.AndBuild import AndBuild, InstallAndRun
from P_Build.WriteAArchive import WriteAArchive, IsDirArchivable
from P_Build.FetchFiles import FetchFiles
from P_Build.CppBuild import CppBuild
from P_Build.IconBuild import IconBuild
from P_Build.JavComp import JavComp


#ARGS
args = sys.argv[1:]
VERBOSE:bool = "-v" in args
ANDROID:bool = "-and" in args or "-android" in args
RELEASE:bool = "-release" in args
COMPILEONLY:bool = "-co" in args

#Config
DIR_SRC:str = ".\\code"
DIR_ASSETS:str = ".\\assets"
outSuffix = ".release" if RELEASE else ".debug"
dir_out = ".\\bin"
log = Log(VERBOSE)

def WriteAssets():
	nonAssetSuffix = [".cpp", ".h"]
	for dir in Path(DIR_SRC).glob("*"):
		if not dir.is_dir():
			continue
		
		dirStr = str(dir)
		if IsDirArchivable(dirStr, False, nonAssetSuffix):
			WriteAArchive(dirStr, False, VERBOSE, nonAssetSuffix)
		else:
			log.VMessage("Non asset dir: " + dir.name)



#Run
try:
	if ANDROID:
		#Config
		suffix = ".aab" if RELEASE else ".apk"
		key = "C:\\pDebug.jks"

		dir_out += f"\\android{outSuffix}"
		FILE_OUT = dir_out + "\\" + Config.GetOutputName(suffix)
		Config.SetOutputDirectory(dir_out)

		#Assets and Resources
		manifest = AndMnfst(release=RELEASE)
		WriteAssets()
		WriteAArchive(DIR_ASSETS, False, VERBOSE)
		IconBuild( "android", "./app.ico", VERBOSE, False)

		#Source Build
		cppFiles: list[str] = FetchFiles(FILE_OUT, DIR_SRC + "\\*.cpp", ".h", VERBOSE)
		CppBuild("android", cppFiles, VERBOSE, RELEASE)
		JavComp(Config.GetEngineIncl(), VERBOSE, manifest)

		#APK Build
		finApp = AndBuild(key, VERBOSE, RELEASE)
		runCmd = InstallAndRun(finApp, key, VERBOSE)

		#Run
		if not COMPILEONLY:
			P_Ex.RunParrallel(runCmd[0])
			subprocess.run(runCmd[1], shell=True)
			subprocess.run("clear", shell=True)

		exit(0)

	else:
		#Config
		dir_out += f"\\win{outSuffix}"
		FILE_OUT = dir_out + "\\" + Config.GetOutputName(".exe")
		Config.SetOutputDirectory(dir_out)

		#Assets and Resources
		WriteAssets()
		WriteAArchive(DIR_ASSETS, False, VERBOSE)
		IconBuild("win32", "./app.ico", VERBOSE)
		WinRes(VERBOSE)
		
		#Code
		cppFiles: list[str] = FetchFiles(FILE_OUT, DIR_SRC + "\\*.cpp", ".h", VERBOSE)
		CppBuild("win32", cppFiles, VERBOSE, RELEASE)
		
		#Run
		if not COMPILEONLY:
			P_Ex.RunParrallel(FILE_OUT)
			subprocess.run("clear", shell=True)

		exit(0)

#Fail
except Exception as e:
	Log(False).Error(str(e))
	exit(-1)