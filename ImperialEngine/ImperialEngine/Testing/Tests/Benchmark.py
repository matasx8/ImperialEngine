import os

# Like the application, scripts should be configured to run from ImperialEngine/ImperialEngine/

engine_path = "../bin/x64/Release/ImperialEngine.exe"
cwd = os.getcwd() + "/"
print(cwd)

def main():
    run_test("")

def run_test(args):
    command = cwd + engine_path + args
    exit_code = os.system(command)
    print(exit_code)


if __name__ == "__main__":
    main()