import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Like the application, scripts should be configured to run from ImperialEngine/ImperialEngine/

#  -- Static Settings --
fake_test_results = 405104819
engine_path = "../bin/x64/Release/ImperialEngine.exe"
msbuild_path = "\"C:/Program Files/Microsoft Visual Studio/2022/Community/Msbuild/Current/Bin/MSBuild.exe\""

save_figures = True
show_figures = True

cwd = os.getcwd()
# make sure we start from ImperialEngine/ImperialEngine
if cwd.find("Testing\\Tests"):
    os.chdir("../../")
    cwd = os.getcwd()
cwd = cwd + "/"

translation = {
    "Culling" : "Į kameros nupjautinę piramidę nepatenkančių objektų atmetimas",
    "Drawing" : "Piešimas",
    "CPU Main Thread" : "CPU Pagrindinė Gija",
    "CPU Render Thread" : "CPU Vaizdavimo Gija",
    "GPU Frame" : "GPU Pilnas Darbas",
    "Unnamed: 5" : "aa"
    }

## -- data structures --

class TestResult:
    def __init__(self, test_id, desc):
        self.test_id = test_id
        self.desc = desc

test_results = []

## -- Functions -- 

def read_data(data_file_name):
    df = pd.read_csv(data_file_name, encoding="iso-8859-1", sep=";")
    return df

# test ids have 10 digits, have to make sure we add trailing zero
def test_id_to_filename(test_id):
    test_dir = f"{test_id:0>10}"
    return test_dir

def save_figure(test_id, title, subtitle):
    if save_figures == False:
        return
    
    path = cwd + "Testing/TestData/" + test_id + "/" + title + "-" + subtitle
    plt.savefig(path)

def show_figure():
    if show_figure == True:
        plt.show()

def plot_bar(cpu, gpu, mesh, title, test_id):
    plt.figure()
    plt.title(title)
    plt.ylabel("darbo laikas, ms")
    plt.bar(["Tradicinis", "GPU valdomas", "GPU valdomas su tinklų šešėliuokliais"], [cpu, gpu, mesh])
    save_figure(test_id, title, "barchart")
    show_figure()

def plot_lines(cpu, gpu, mesh, title, test_id):
        plt.figure()
        plt.title(title)
        plt.xlabel("kadras")
        plt.ylabel("darbo laikas, ms")
        plt.plot(cpu, label="Tradicinis")
        plt.plot(gpu, label="GPU valdomas")
        plt.plot(mesh, label="GPU valdomas su tinklų šešėliuokliais")
        plt.legend()
        save_figure(test_id, title, "linechart")
        show_figure()

def process_results(result):
    if result.test_id == 0:
        print("Test run was not successful")

    test_id_str = test_id_to_filename(result.test_id)
    file_path = cwd + "Testing/TestData/" + test_id_str + "/"
    df_cpu = read_data(file_path + "TestData-Traditional.csv")
    df_gpu = read_data(file_path + "TestData-GPU-Driven.csv")
    df_mesh = read_data(file_path + "TestData-GPU-Driven-Mesh.csv")

    #  -- draw graphs -- 
    # line graphs
    for col in df_cpu.columns:
        plot_lines(df_cpu[col], df_gpu[col], df_mesh[col], translation[col], test_id_str)

    # bar chart
    for col in df_cpu.columns:
        avg_cpu = df_cpu[col].mean()
        avg_gpu = df_gpu[col].mean()
        avg_mesh = df_mesh[col].mean()

        plot_bar(avg_cpu, avg_gpu, avg_mesh, translation[col], test_id_str)

    print("done")




# All optimizations
def test_suite1():
    run_count = " --run-for=1000"

    # get the test environment ready
    compile_engine()

    #  -- regular sponza --
    result = run_test("--file-count=1 --load-files Scene/sponza.glb" + run_count)
    test_results.append(TestResult(result, "'Sponza' scena, visos optimizacijos, kamera statine"))

    result = run_test("--file-count=1  --camera-movement=rotate --load-files Scene/sponza.glb" + run_count)
    test_results.append(TestResult(result, "'Sponza' scena, visos optimizacijos, kamera sukasi"))


    # -- sponza with curtains --
    result = run_test("--file-count=1 --load-files Scene/sponza_wc.glb" + run_count)
    test_results.append(TestResult(result, "'Sponza' scena, visos optimizacijos"))


    # -- sponza with curtains and vines --
    result = run_test("--file-count=1 --load-files Scene/sponza_wc_v_flat.glb" + run_count)
    test_results.append(TestResult(result, "'Sponza' scena, visos optimizacijos"))


    # -- donut and monkey --
    result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=max" + run_count)
    test_results.append(TestResult(result, "Spurga ir Suzanne su atsitiktiniu išmėtymu"))

    result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=max --camera-movement=rotate" + run_count)
    test_results.append(TestResult(result, "Spurga ir Suzanne su atsitiktiniu išmėtymu"))


    # very high poly


    # very low poly



def main():
    test_suite1()

    for result in test_results:
        process_results(result)

def compile_engine():
    project = "ImperialEngine.vcxproj"
    config = "/p:configuration=Release /p:platform=x64 /p:OutDir=../bin/x64/Release/ /p:IntDir=../bin/intermediates/x64/Release/"
    config_debug = "/p:configuration=Development /p:platform=x64 /p:OutDir=../bin/x64/Development/ /p:IntDir=../bin/intermediates/x64/Development/ -v:m"
    args = " " + project + " " + config
    exit_code = os.system(msbuild_path + args)
    print(exit_code)

def run_test(args):
    command = cwd + engine_path + " " + args
    exit_code = os.system(command)
    return exit_code


if __name__ == "__main__":
    main()