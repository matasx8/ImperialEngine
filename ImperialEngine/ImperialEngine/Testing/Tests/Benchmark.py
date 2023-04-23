import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Like the application, scripts should be configured to run from ImperialEngine/ImperialEngine/

#  -- Static Settings --
use_premade_results = True
engine_path = "../bin/x64/Release/ImperialEngine.exe"
msbuild_path = "\"C:/Program Files/Microsoft Visual Studio/2022/Community/Msbuild/Current/Bin/MSBuild.exe\""
smooth_data = True

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
    "Frame Time" : "Bendras darbas prie kadro",
    "CPU Main Thread" : "CPU Pagrindinė Gija",
    "CPU Render Thread" : "CPU Vaizdavimo Gija",
    "GPU Frame" : "GPU Pilnas Darbas",
    "Triangles" : "Apdoroti trikampiai"
    }

## -- data structures --

class TestResult:
    def __init__(self, test_id, desc):
        self.test_id = test_id
        self.desc = desc

test_results = []
fake_test_results = [TestResult(423065040, 'Visos optim.')
                     , TestResult(423065104, 'Be LOD')
                     , TestResult(423065122, 'Be Klast. Atm.')
                     , TestResult(423065145, 'Be LOD ir Be Klast. Atm.')]

## -- Functions -- 

def read_data(data_file_name):
    df = pd.read_csv(data_file_name, encoding="iso-8859-1", sep=";")
    return df

def read_all(filepath):
    df_cpu = read_data(filepath + "TestData-Traditional.csv")
    df_gpu = read_data(filepath + "TestData-GPU-Driven.csv")
    df_mesh = read_data(filepath + "TestData-GPU-Driven-Mesh.csv")
    return (df_cpu, df_gpu, df_mesh)

def smoothing(df):
    if smooth_data == True:
        exclude = ["Triangles"]
        df_dropped = df.drop(columns=exclude)
        smoothed = df_dropped.rolling(5, center=True).mean()
        smoothed[exclude] = df[exclude]
        return smoothed
    else:
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
    if save_figures == True:
        save_figure(test_id, title, "barchart")
    if show_figures == True:
        show_figure()

def plot_lines(cpu, gpu, mesh, title, draw_rt_line, test_id):
        plt.figure()
        plt.title(title)
        plt.xlabel("kadras")
        plt.ylabel("darbo laikas, ms")
        if draw_rt_line == True and title != "Apdoroti trikampiai":
            plt.plot(np.full(len(cpu), 16), label="60 FPS riba", color="red")
        plt.plot(cpu, label="Tradicinis")
        plt.plot(gpu, label="GPU valdomas")
        plt.plot(mesh, label="GPU valdomas su tinklų šešėliuokliais")
        plt.legend()
        if save_figures == True:
            save_figure(test_id, title, "linechart")
        if show_figures == True:
            show_figure()

def plot_lines2(data_lists, data_labels, title, test_id):
    plt.subplots(figsize=(8, 12))
    plt.title(title)
    plt.xlabel("kadras")
    plt.ylabel("darbo laikas, ms")
    for data, l in zip(data_lists, data_labels):
        plt.plot(data, label=l)
    plt.legend(loc="upper center", bbox_to_anchor=(0.5, -0.2), ncol=1)
    plt.tight_layout()
    if save_figures == True:
        save_figure(test_id, title, "linechart")
    if show_figures == True:
        show_figure()

def plot_lines3(data_lists, data_labels, x_data_lists, title, test_id):
    plt.subplots(figsize=(8, 6))
    plt.title(title)
    plt.xlabel("kadras")
    plt.ylabel("darbo laikas, ms")
    for data, l, x in zip(data_lists, data_labels, x_data_lists):
        plt.plot(x, data, label=l)
    plt.legend(loc="upper center", bbox_to_anchor=(0.5, -0.2), ncol=1)
    plt.tight_layout()
    if save_figures == True:
        save_figure(test_id, title, "linechart")
    if show_figures == True:
        show_figure()

def plot_bar2(data_lists, data_labels, title, test_id):
    fig, ax = plt.subplots(figsize=(8, 6))
    plt.title(title)
    plt.ylabel("darbo laikas, ms")

    x = np.arange(len(data_lists))
    rects = ax.bar(x, data_lists)
    for rect in rects:
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width() / 2., 0.5 * height,
                '{:.2f}'.format(height),
                ha='center', va='bottom', color='white', weight='bold')


    ax.set_xticks(x)
    ax.set_xticklabels(data_labels, rotation=45, ha='right')
    plt.tight_layout()
    if save_figures == True:
        save_figure(test_id, title, "barchart")
    if show_figures == True:
        show_figure()

def process_results_obj_count(result):
    if result.test_id == 0:
        print("Test run was not successful")

    test_id_str = test_id_to_filename(result.test_id)
    file_path = cwd + "Testing/TestData/" + test_id_str + "/"
    df_cpu, df_gpu, df_mesh = read_all(file_path)
    df_cpu = smoothing(df_cpu)
    df_gpu = smoothing(df_gpu)
    df_mesh = smoothing(df_mesh)
    for col in df_cpu.columns:
        plot_lines(df_cpu[col], df_gpu[col], df_mesh[col], translation[col], True, test_id_str)

    labels = ["Tradicinis", "GPU valdomas", "GPU valdomas su tinklų skaičiavimu"]
    x_datas = [df_cpu["Triangles"], df_gpu["Triangles"], df_mesh["Triangles"]]
    plot_lines3([df_cpu["Frame Time"], df_gpu["Frame Time"], df_mesh["Frame Time"]], labels, x_datas, "Darbo laiko ir trikampių santykis", test_id_str)

def process_mesh_results(results):
    dfs_mesh = []
    last_test_id_str = ""
    for result in results:
        test_id_str = test_id_to_filename(result.test_id)
        last_test_id_str = test_id_str
        file_path = cwd + "Testing/TestData/" + test_id_str + "/"
        dfs_mesh.append(read_data(file_path + "TestData-GPU-Driven-Mesh.csv"))

    plot_lines2([df["Frame Time"] for df in dfs_mesh], [result.desc for result in results], "Mesh", last_test_id_str)

    plot_bar2([df["Frame Time"].mean() for df in dfs_mesh], [result.desc for result in results], "Mesh", last_test_id_str)    


def process_results(result):
    if result.test_id == 0:
        print("Test run was not successful")

    test_id_str = test_id_to_filename(result.test_id)
    file_path = cwd + "Testing/TestData/" + test_id_str + "/"
    df_cpu, df_gpu, df_mesh = read_all(file_path)

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

def process_combined_results(results):
    dfs_cpu = []
    dfs_gpu = []
    dfs_mesh = []
    last_test_id_str = ""
    for result in results:
        test_id_str = test_id_to_filename(result.test_id)
        last_test_id_str = test_id_str
        file_path = cwd + "Testing/TestData/" + test_id_str + "/"
        dfs_cpu.append(read_data(file_path + "TestData-Traditional.csv"))
        dfs_gpu.append(read_data(file_path + "TestData-GPU-Driven.csv"))
        dfs_mesh.append(read_data(file_path + "TestData-GPU-Driven-Mesh.csv"))

    plot_lines2([df["Frame Time"] for df in dfs_cpu], [result.desc for result in results], "Tradicinis", last_test_id_str)
    plot_lines2([df["Frame Time"] for df in dfs_gpu], [result.desc for result in results], "GPU", last_test_id_str)
    plot_lines2([df["Frame Time"] for df in dfs_mesh], [result.desc for result in results], "Mesh", last_test_id_str)

    plot_bar2([df["Frame Time"].mean() for df in dfs_cpu], [result.desc for result in results], "Tradicinis", last_test_id_str)
    plot_bar2([df["Frame Time"].mean() for df in dfs_gpu], [result.desc for result in results], "GPU", last_test_id_str)
    plot_bar2([df["Frame Time"].mean() for df in dfs_mesh], [result.desc for result in results], "Mesh", last_test_id_str)


# All optimizations
def test_suite1():
    run_count = " --run-for=250"

    # get the test environment ready
    defines = "BENCHMARK_MODE#1"
    compile_shaders("DEBUG_MESH=0")
    result = compile_engine(defines)
    if result > 0:
        print("Failed to successfully compile engine")
        return

    #  -- regular sponza --
    #result = run_test("--file-count=1 --load-files Scene/sponza.glb" + run_count)
    #test_results.append(TestResult(result, "'Sponza' scena, visos optimizacijos, kamera statine"))

    #result = run_test("--file-count=1  --camera-movement=rotate --load-files Scene/sponza.glb" + run_count)
    #test_results.append(TestResult(result, "'Sponza' scena, visos optimizacijos, kamera sukasi"))


    # -- sponza with curtains --
    #result = run_test("--file-count=1 --load-files Scene/sponza_wc.glb" + run_count)
    #test_results.append(TestResult(result, "'Sponza' scena, visos optimizacijos"))


    # -- sponza with curtains and vines --
    result = run_test("--file-count=1 --load-files Scene/sponza_wc_v_cameranear.glb" + run_count)
    test_results.append(TestResult(result, "'Sponza' scena, visos optimizacijos"))


    # -- donut and monkey --
    #result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=max" + run_count)
    #test_results.append(TestResult(result, "Spurga ir Suzanne su atsitiktiniu išmėtymu"))

    #result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=max --camera-movement=away" + run_count)
    #test_results.append(TestResult(result, "Spurga ir Suzanne su atsitiktiniu išmėtymu"))

    #result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=max --camera-movement=away" + run_count)
    #test_results.append(TestResult(result, "Spurga ir Suzanne su atsitiktiniu išmėtymu"))

    #result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=max --camera-movement=static_offset" + run_count)
    #test_results.append(TestResult(result, "Spurga ir Suzanne su atsitiktiniu išmėtymu"))


    # very high poly


    # very low poly

# no LOD
def test_suite2():
    run_count = " --run-for=250"

    universal_defines = "LOD_ENABLED#0"
    defines = "BENCHMARK_MODE#1;" + universal_defines
    # get the test environment ready
    compile_shaders(universal_defines)
    res = compile_engine(defines)
    if res > 0:
        print("Failed to successfully compile engine")
        return

    result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=max --camera-movement=away" + run_count)
    test_results.append(TestResult(result, "Spurga ir Suzanne su atsitiktiniu išmėtymu"))

# no LOD and no Cone Culling
def test_suite3():
    run_count = " --run-for=100"

    universal_defines = "LOD_ENABLED#0;CONE_CULLING_ENABLED#0"
    defines = "BENCHMARK_MODE#1;" + universal_defines
    # get the test environment ready
    compile_engine(defines)
    result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=max --camera-movement=rotate" + run_count)
    test_results.append(TestResult(result, "Spurga ir Suzanne su atsitiktiniu išmėtymu"))

# No optimizations
def test_suite3():
    run_count = " --run-for=100"

    universal_defines = "LOD_ENABLED#0;CONE_CULLING_ENABLED#0;CULLING_ENABLED#0"
    defines = "BENCHMARK_MODE#1;" + universal_defines
    # get the test environment ready
    compile_engine(defines)

def test_suite_optimization():
    if use_premade_results == False:
        run_count = " --run-for=500"

        results = []

        # prepare environment with all optimizations
        defines = "BENCHMARK_MODE#1"
        compile_shaders("DEBUG_MESH=0")
        result = compile_engine(defines)
        if result > 0:
            print("Failed to successfully compile engine")
            return
        
        result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=100000" + run_count)
        results.append(TestResult(result, "Spurga ir Suzanne su atsitiktiniu išmėtymu su visomis optimizacijomis"))

        # prepare environment without LOD
        universal_defines = "LOD_ENABLED#0"
        defines = "BENCHMARK_MODE#1;" + universal_defines
        compile_shaders("DEBUG_MESH=0;" + universal_defines)
        result = compile_engine(defines)
        if result > 0:
            print("Failed to successfully compile engine")
            return
        
        result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=100000" + run_count)
        results.append(TestResult(result, "Spurga ir Suzanne su atsitiktiniu išmėtymu be LOD"))

        # prepare environment without cone culling
        universal_defines = "CONE_CULLING_ENABLED#0"
        defines = "BENCHMARK_MODE#1;" + universal_defines
        compile_shaders("DEBUG_MESH=0;" + universal_defines)
        result = compile_engine(defines)
        if result > 0:
            print("Failed to successfully compile engine")
            return
        
        result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=100000" + run_count)
        results.append(TestResult(result, "Spurga ir Suzanne su atsitiktiniu išmėtymu be klasterių atmetimo"))

        # prepare environment without LOD and without cone culling
        universal_defines = "LOD_ENABLED#0;CONE_CULLING_ENABLED#0"
        defines = "BENCHMARK_MODE#1;" + universal_defines
        compile_shaders("DEBUG_MESH=0;" + universal_defines)
        result = compile_engine(defines)
        if result > 0:
            print("Failed to successfully compile engine")
            return
        
        result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=100000" + run_count)
        results.append(TestResult(result, "Spurga ir Suzanne su atsitiktiniu išmėtymu be LOD ir be klasterių atmetimo"))
    else:
        results = fake_test_results

    process_combined_results(results)

def test_suite_object_count():
    if use_premade_results == False:
        run_count = " --run-for=1000"

        # prepare environment with all optimizations
        defines = "BENCHMARK_MODE#1"
        compile_shaders("DEBUG_MESH=0")
        result = compile_engine(defines)
        if result > 0:
            print("Failed to successfully compile engine")
            return
        
        result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --growth-step=1000" + run_count)
        results = TestResult(result, "Augantis objektų skaičius")

        process_results_obj_count(results)
    else:
        process_results_obj_count(TestResult(423103839, "Augantis objektų skaičius"))

def test_suite_mesh():
    if use_premade_results == False:
        run_count = " --run-for=100"

        results_random = []
        results_dragons= []
        combinations = [(64, 64), (64, 84), (64, 124), (48, 124), (48, 84), (48, 64), (32, 32), (32, 64), (32, 84)]
        for combo in combinations:
            universal_defines = "MESHLET_MAX_VERTS#{};MESHLET_MAX_PRIMS#{}".format(combo[0], combo[1])
            defines = "BENCHMARK_MODE#1;" + universal_defines
            compile_shaders("DEBUG_MESH=0;" + universal_defines)
            result = compile_engine(defines)
            if result > 0:
                print("Failed to successfully compile engine")
                return
            
            result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=max" + run_count)
            results.append(TestResult(result, "Random su max vir: {} ir max tri: {}". format(combo[0], combo[1])))

            result = run_test("--file-count=1 --load-files Scene/BigDragons.glb" + run_count)
            results.append(TestResult(result, "Random su max vir: {} ir max tri: {}". format(combo[0], combo[1])))

        process_mesh_results(results)

    else:
        results_random = [TestResult(423164443, "Random su max vir: 64 ir max tri: 64"),
                   TestResult(423164528, "Random su max vir: 64 ir max tri: 84"),
                   TestResult(423164614, "Random su max vir: 64 ir max tri: 124"),
                   TestResult(423164659, "Random su max vir: 48 ir max tri: 124"),
                   TestResult(423164744, "Random su max vir: 48 ir max tri: 84"),
                   TestResult(423164828, "Random su max vir: 48 ir max tri: 64"),
                   TestResult(423164912, "Random su max vir: 32 ir max tri: 32"),
                   TestResult(423164956, "Random su max vir: 32 ir max tri: 64"),
                   TestResult(423165040, "Random su max vir: 32 ir max tri: 84")]

        results_dragons = [TestResult(423164501, "Random su max vir: 64 ir max tri: 64"),
                   TestResult(423164546, "Random su max vir: 64 ir max tri: 84"),
                   TestResult(423164631, "Random su max vir: 64 ir max tri: 124"),
                   TestResult(423164716, "Random su max vir: 48 ir max tri: 124"),
                   TestResult(423164801, "Random su max vir: 48 ir max tri: 84"),
                   TestResult(423164846, "Random su max vir: 48 ir max tri: 64"),
                   TestResult(423164929, "Random su max vir: 32 ir max tri: 32"),
                   TestResult(423165013, "Random su max vir: 32 ir max tri: 64"),
                   TestResult(423165058, "Random su max vir: 32 ir max tri: 84")]
        process_mesh_results(results_random)
        process_mesh_results(results_dragons)



def main():
    #test_suite1()
    #test_suite2()
    #test_suite_optimization()
    #test_suite_object_count()
    test_suite_mesh()

    for result in test_results:
        process_results(result)

def compile_engine(defines):
    define_args = ""
    if len(defines) > 0:
        split_defines = defines.split(';')
        post_processed_split = [" /p:Define" + str(ind) + "=" + defi for ind, defi in enumerate(split_defines)]
        define_args = "".join(post_processed_split)
    project = "ImperialEngine.vcxproj"
    config = "/p:configuration=Release /p:platform=x64 /p:OutDir=../bin/x64/Release/ /p:IntDir=../bin/intermediates/x64/Release/" + define_args
    #config_debug = "/p:configuration=Development /p:platform=x64 /p:OutDir=../bin/x64/Development/ /p:IntDir=../bin/intermediates/x64/Development/ -v:m"
    args = " " + project + " " + config
    exit_code = os.system(msbuild_path + args)
    return exit_code

def compile_shaders(defines):
    defines = defines.replace("#","=")
    seperated = defines.split(';')
    post_processed_defines = ""
    if len(defines) > 0:
        post_processed_defines = "-D" + " -D".join(seperated)

    compile_script = "py Shaders/compile_shaders.py"
    os.system(compile_script + " " + post_processed_defines)

def run_test(args):
    command = cwd + engine_path + " " + args
    exit_code = os.system(command)
    return exit_code


if __name__ == "__main__":
    main()