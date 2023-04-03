import os

# Like the application, scripts should be configured to run from ImperialEngine/ImperialEngine/

engine_path = "../bin/x64/Release/ImperialEngine.exe"
msbuild_path = "\"C:/Program Files/Microsoft Visual Studio/2022/Community/Msbuild/Current/Bin/MSBuild.exe\""
cwd = os.getcwd() + "/"
print(cwd)

def main():
    compile_engine()
    run_test("")

def compile_engine():
    project = "ImperialEngine.vcxproj"
    config = "/p:configuration=Release /p:platform=x64 /p:OutDir=../bin/x64/Release/ /p:IntDir=../bin/intermediates/x64/Release/"
    config_debug = "/p:configuration=Development /p:platform=x64 /p:OutDir=../bin/x64/Development/ /p:IntDir=../bin/intermediates/x64/Development/ -v:m"
    args = " " + project + " " + config
    exit_code = os.system(msbuild_path + args)
    print(exit_code)

def run_test(args):
    command = cwd + engine_path + args
    exit_code = os.system(command)
    print(exit_code)


if __name__ == "__main__":
    main()