import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import csv

# Like the application, scripts should be configured to run from ImperialEngine/ImperialEngine/

#  -- Static Settings --
use_premade_results = False
engine_path = "../bin/x64/Release/ImperialEngine.exe"
#TODO: make this local path
msbuild_path = "\"C:/Program Files/Microsoft Visual Studio/2022/Community/Msbuild/Current/Bin/MSBuild.exe\""
vulkan_info_path = os.environ.get("VK_SDK_PATH") + "\\Bin\\vulkaninfoSDK.exe"
smooth_data = True

save_figures = True
show_figures = False

supports_mesh_shading = False

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
## -- Functions -- 

def read_data(data_file_name):
    df = pd.read_csv(data_file_name, encoding="iso-8859-1", sep=";")
    return df

def check_mesh_shader_support():
    cmd = vulkan_info_path + " > " + os.devnull
    result = os.system(cmd)

    if result == 0:
        with os.popen(cmd) as f:
            output = f.read()
            if "VK_NV_mesh_shader" in output:
                print("VK_NV_mesh_shader is supported on this device.")
                return True
            else:
                print("VK_NV_mesh_shader is not supported on this device.")
                return False
    else:
        print(f"Error running {cmd}: exit code {result}")
        return False

def ensure_test_dir_exits():
    directory_path = os.path.join(os.getcwd(), "Testing", "TestData")
    if not os.path.exists(directory_path):
        os.makedirs(directory_path)
        print(f"Created directory {directory_path}")
    else:
        print(f"Directory {directory_path} already exists")

def read_all(filepath):
    df_cpu = read_data(filepath + "TestData-Traditional.csv")
    df_gpu = read_data(filepath + "TestData-GPU-Driven.csv")
    df_mesh = []
    if supports_mesh_shading:
        df_mesh = read_data(filepath + "TestData-GPU-Driven-Mesh.csv")
    else:
        df_mesh = read_data(filepath + "TestData-GPU-Driven.csv") # this might be easier to work around when mesh shading is not enabled
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
    # will truncate title to 64 chars
    path = cwd + "Testing/TestData/" + test_id + "/" + title[:64] + "-" + subtitle
    plt.savefig(path)

def show_figure():
    if show_figure == True:
        plt.show()

def plot_bar(cpu, gpu, mesh, title, test_id):
    plt.subplots()
    plt.title(title)
    plt.ylabel("darbo laikas, ms")
    if supports_mesh_shading:
        plt.bar(["Tradicinis", "GPU valdomas", "Klasterizuotas"], [cpu, gpu, mesh])
    else:
        plt.bar(["Tradicinis", "GPU valdomas"], [cpu, gpu])

    if save_figures == True:
        save_figure(test_id, title, "barchart")
    if show_figures == True:
        show_figure()
    plt.close()

def plot_lines(cpu, gpu, mesh, title, draw_rt_line, test_id):
        plt.subplots()
        plt.title(title)
        plt.xlabel("kadras")
        plt.ylabel("darbo laikas, ms")
        if draw_rt_line == True and title != "Apdoroti trikampiai":
            plt.plot(np.full(len(cpu), 16), label="60 FPS riba", color="red")
        plt.plot(cpu, label="Tradicinis")
        plt.plot(gpu, label="GPU valdomas")
        if supports_mesh_shading:
            plt.plot(mesh, label="Klasterizuotas")
        plt.legend()
        if save_figures == True:
            save_figure(test_id, title, "linechart")
        if show_figures == True:
            show_figure()
        plt.close()

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

def plot_lines3(data_lists, data_labels, x_data_lists, title, test_id, xlabel = "kadras", x_tri = False, draw_rt = False):
    fig, ax = plt.subplots(figsize=(8, 6))
    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel("darbo laikas, ms")


    if draw_rt == True:
        plt.plot(np.full(500, 16), label="60 FPS riba", color="red")
    for data, l, x in zip(data_lists, data_labels, x_data_lists):
        plt.plot(x, data, label=l)

    if x_tri:
        text_offset = 15
        for data, l, x in zip(data_lists, data_labels, x_data_lists):
            for time, tri in zip(data, x):
                if time >= 16:
                    plt.scatter(tri, 16, c="black", zorder=3)
                    ax.annotate(f"{int(tri)} mln. tri.", (tri, 16), textcoords="offset points", xytext=(0, text_offset), ha="center", fontweight="bold", zorder=4)
                    text_offset *= -1
                    break
    
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
                ha='center', va='bottom', color='black', weight='bold')

    plt.legend(loc="upper center", bbox_to_anchor=(0.5, -0.2), ncol=1)
    plt.tight_layout()

    ax.set_xticks(x)
    ax.set_xticklabels(data_labels, rotation=45, ha='right')
    plt.tight_layout()
    if save_figures == True:
        save_figure(test_id, title, "barchart")
    if show_figures == True:
        show_figure()

def plot_bar3(data_lists_cpu, data_lists_gpu, data_lists_mesh, data_labels, title, test_id):
    fig, ax = plt.subplots(figsize=(8, 6))
    plt.title(title)
    plt.ylabel("vidutinė greitaveika, ms")

    x = np.arange(len(data_lists_cpu))
    rects = []
    rects += ax.bar(x - 0.2, data_lists_cpu, 0.2, label = "Tradicinis")
    rects += ax.bar(x, data_lists_gpu, 0.2, label="GPU valdomas")
    if supports_mesh_shading:
        rects += ax.bar(x + 0.2, data_lists_mesh, 0.2, label="Klasterizuotas")
    for rect in rects:
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width() / 2., 0.5 * height,
                '{:.2f}'.format(height),
                ha='center', va='bottom', color='black', weight='bold', rotation=90)

    plt.legend()
    plt.tight_layout()

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

    if supports_mesh_shading:
        labels = ["Tradicinis", "GPU valdomas", "Klasterizuotas"]
        x_datas = [df_cpu["Triangles"] / 1e6, df_gpu["Triangles"] / 1e6, df_mesh["Triangles"] / 1e6]
        plot_lines3([df_cpu["Frame Time"], df_gpu["Frame Time"], df_mesh["Frame Time"]], labels, x_datas, "Darbo laiko prie vieno kadro ir trikampių santykis", test_id_str, "apdorotų trikampių skaičius (milijonais)", True, True)
    else:
        labels = ["Tradicinis", "GPU valdomas", "Klasterizuotas"]
        x_datas = [df_cpu["Triangles"] / 1e6, df_gpu["Triangles"] / 1e6]
        plot_lines3([df_cpu["Frame Time"], df_gpu["Frame Time"]], labels, x_datas, "Darbo laiko prie vieno kadro ir trikampių santykis", test_id_str, "apdorotų trikampių skaičius (milijonais)", True, True)

def process_mesh_results(results, title):
    if supports_mesh_shading == False:
        print("Skipping processing mesh results because this device does not support mesh shading");
    dfs_mesh = []
    last_test_id_str = ""
    for result in results:
        test_id_str = test_id_to_filename(result.test_id)
        last_test_id_str = test_id_str
        file_path = cwd + "Testing/TestData/" + test_id_str + "/"
        dfs_mesh.append(read_data(file_path + "TestData-GPU-Driven-Mesh.csv"))

    plot_lines2([df["Frame Time"] for df in dfs_mesh], [result.desc for result in results], title, last_test_id_str)

    a = [df["Frame Time"].mean() for df in dfs_mesh]
    b = [result.desc for result in results]
    data, descs = zip(*sorted(zip(a, b)))
    plot_bar2(data, descs, title, last_test_id_str)    


def process_results(result):
    if result.test_id == 0:
        print("Test run was not successful")

    test_id_str = test_id_to_filename(result.test_id)
    file_path = cwd + "Testing/TestData/" + test_id_str + "/"
    df_cpu, df_gpu, df_mesh = read_all(file_path)

    #  -- draw graphs -- 
    # line graphs
    for col in df_cpu.columns:
        plot_lines(df_cpu[col], df_gpu[col], df_mesh[col], translation[col], False, test_id_str)

    # bar chart
    for col in df_cpu.columns:
        avg_cpu = df_cpu[col].mean()
        avg_gpu = df_gpu[col].mean()
        avg_mesh = df_mesh[col].mean()

        plot_bar(avg_cpu, avg_gpu, avg_mesh, translation[col], test_id_str)

def process_results_into_table(results):
    dfs_cpu = []
    dfs_gpu = []
    dfs_mesh = []
    last_test_id_str = ""
    for result in results:
        test_id_str = test_id_to_filename(result.test_id)
        last_test_id_str = test_id_str
        file_path = cwd + "Testing/TestData/" + test_id_str + "/"
        dfs_cpu.append(smoothing(read_data(file_path + "TestData-Traditional.csv")))
        dfs_gpu.append(smoothing(read_data(file_path + "TestData-GPU-Driven.csv")))
        if supports_mesh_shading:
            dfs_mesh.append(smoothing(read_data(file_path + "TestData-GPU-Driven-Mesh.csv")))

    path = cwd + "Testing/TestData/" + last_test_id_str + "/frametime_results.csv"
    with open(path, mode='w', newline='', encoding="utf-8-sig") as file:
        writer = csv.writer(file)

        if supports_mesh_shading == True:
            writer.writerow(["Scenos pavadinimas", "Tradicinis", "GPU valdomas", "Tinklų apdorojimo"])
            for result, df_cpu, df_gpu, df_mesh in zip(results, dfs_cpu, dfs_gpu, dfs_mesh):
                writer.writerow([result.desc, df_cpu["Frame Time"].mean(), df_gpu["Frame Time"].mean(), df_mesh["Frame Time"].mean()])
        else:
            writer.writerow(["Scenos pavadinimas", "Tradicinis", "GPU valdomas"])
            for result, df_cpu, df_gpu in zip(results, dfs_cpu, dfs_gpu):
                writer.writerow([result.desc, df_cpu["Frame Time"].mean(), df_gpu["Frame Time"].mean()])




def process_combined_results(results, title):
    dfs_cpu = []
    dfs_gpu = []
    dfs_mesh = []
    last_test_id_str = ""
    for result in results:
        test_id_str = test_id_to_filename(result.test_id)
        last_test_id_str = test_id_str
        file_path = cwd + "Testing/TestData/" + test_id_str + "/"
        dfs_cpu.append(smoothing(read_data(file_path + "TestData-Traditional.csv")))
        dfs_gpu.append(smoothing(read_data(file_path + "TestData-GPU-Driven.csv")))
        if supports_mesh_shading:
            dfs_mesh.append(smoothing(read_data(file_path + "TestData-GPU-Driven-Mesh.csv")))

    
    # skip results with cluster culling only changes for non mesh shading pipelines
    plot_lines2([df["Frame Time"] for df in dfs_cpu], [result.desc for result in results], "Tradicinis", last_test_id_str)
    plot_lines2([df["Frame Time"] for df in dfs_gpu], [result.desc for result in results], "GPU", last_test_id_str)
    if supports_mesh_shading:
        plot_lines2([df["Frame Time"] for df in dfs_mesh], [result.desc for result in results], "Mesh", last_test_id_str)

    cpu_data = [df["Frame Time"].mean() for df in dfs_cpu]
    gpu_data = [df["Frame Time"].mean() for df in dfs_gpu]
    if supports_mesh_shading:
        mesh_data = [df["Frame Time"].mean() for df in dfs_mesh]
    plot_bar3(cpu_data, gpu_data, mesh_data, [result.desc for result in results], title, last_test_id_str)


# All optimizations
#use this for "testing"
def test_suite1():
    run_count = " --run-for=100"

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
    #result = run_test("--file-count=1 --load-files Scene/sponza_wc_v_cameranear.glb" + run_count)
    #test_results.append(TestResult(result, "'Sponza' scena, visos optimizacijos"))


    # -- donut and monkey --
    result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=10000" + run_count)
    test_results.append(TestResult(result, "Spurga ir Suzanne su atsitiktiniu išmėtymu"))

    result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=10000 --camera-movement=away" + run_count)
    test_results.append(TestResult(result, "Spurga ir Suzanne su atsitiktiniu išmėtymu"))

    for result in test_results:
        process_results(result)

def test_suite_performance():
    results = []
    if use_premade_results == False:
        run_count = " --run-for=250"

        defines = "BENCHMARK_MODE#1;"
        # get the test environment ready
        compile_shaders("DEBUG_MESH=0")
        res = compile_engine(defines)
        if res > 0:
            print("Failed to successfully compile engine")
            return

        result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=max" + run_count)
        results.append(TestResult(result, "Atsitiktinai sugeneruotos scenos greitaveika su 1M obj."))
    
        result = run_test("--file-count=2 --camera-movement=rotate --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=max" + run_count)
        results.append(TestResult(result, "Atsitiktinai sugeneruotos scenos greitaveika su 1M obj., kai kamera sukasi aplink Y ašį"))
    
        result = run_test("--file-count=1 --load-files Scene/sponza_wc_v_flat.glb" + run_count)
        results.append(TestResult(result, "Sponza scenos greitaveika"))
    
        result = run_test("--file-count=1 --camera-movement=rotate --load-files Scene/sponza_wc_v_flat.glb" + run_count)
        results.append(TestResult(result, "Sponza scenos greitaveika, kai kamera sukasi aplink Y ašį"))
    
        result = run_test("--file-count=1 --load-files Scene/sponza_wc_v_cameranear.glb" + run_count)
        results.append(TestResult(result, "Sponza scenos, kai kamera scenos pakraštyje greitaveika"))
    
        result = run_test("--file-count=1 --camera-movement=rotate --load-files Scene/sponza_wc_v_cameranear.glb" + run_count)
        results.append(TestResult(result, "Sponza scenos, kai kamera scenos pakraštyje greitaveika ir sukasi aplink Y ašį"))
    
        result = run_test("--file-count=1 --load-files Scene/Sponza_Custom.glb" + run_count)
        results.append(TestResult(result, "Koreguotos Sponza scenos greitaveika"))
    
        result = run_test("--file-count=1 --camera-movement=rotate --load-files Scene/Sponza_Custom.glb" + run_count)
        results.append(TestResult(result, "Koreguotos Sponza scenos greitaveika, kai kamera sukasi aplink Y ašį"))

        result = run_test("--file-count=1 --load-files Scene/BigDragons.glb" + run_count)
        results.append(TestResult(result, "Koreguotos Sponza scenos greitaveika"))

        result = run_test("--file-count=1 --camera-movement=rotate --load-files Scene/BigDragons.glb" + run_count)
        results.append(TestResult(result, "Koreguotos Sponza scenos greitaveika, kai kamera sukasi aplink Y ašį"))

    else:
        fake_test_results = [ TestResult(426110245, 'Atsitiktinai sugeneruotos scenos greitaveika su 1M obj.')
                            , TestResult(426110313, 'Atsitiktinai sugeneruotos scenos greitaveika su 1M obj., kai kamera sukasi aplink Y ašį')
                            , TestResult(426110347, 'Sponza scenos greitaveika')
                            , TestResult(426110421, 'Sponza scenos greitaveika, kai kamera sukasi aplink Y ašį')
                            , TestResult(426110454, 'Sponza scenos, kai kamera scenos pakraštyje greitaveika')
                            , TestResult(426110528, 'Sponza scenos, kai kamera scenos pakraštyje greitaveika ir sukasi aplink Y ašį')
                            , TestResult(426110607, 'Koreguotos Sponza scenos greitaveika')
                            , TestResult(426110646, 'Koreguotos Sponza scenos greitaveika, kai kamera sukasi aplink Y ašį')
                            , TestResult(426111420, 'Koreguotos Sponza scenos greitaveika')
                            , TestResult(426111440, 'Koreguotos Sponza scenos greitaveika, kai kamera sukasi aplink Y ašį')]

        results = fake_test_results

    for result in results:
        print(result.test_id)
        process_results(result)
    
    process_results_into_table(results)

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
        results.append(TestResult(result, "Visos optimizacijos"))

        if supports_mesh_shading:
            universal_defines = "CONE_CULLING_ENABLED#0"
            defines = "BENCHMARK_MODE#1;" + universal_defines
            compile_shaders("DEBUG_MESH=0;" + universal_defines)
            result = compile_engine(defines)
            if result > 0:
                print("Failed to successfully compile engine")
                return
            
            result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=100000" + run_count)
            results.append(TestResult(result, "Be klast. atm."))

        # prepare environment without LOD and without cone culling
        universal_defines = "LOD_ENABLED#0;CONE_CULLING_ENABLED#0"
        defines = "BENCHMARK_MODE#1;" + universal_defines
        compile_shaders("DEBUG_MESH=0;" + universal_defines)
        result = compile_engine(defines)
        if result > 0:
            print("Failed to successfully compile engine")
            return
        
        result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=100000" + run_count)
        if supports_mesh_shading:
            results.append(TestResult(result, "Be klast. atm. ir be LOD"))
        else:
            results.append(TestResult(result, "Be LOD"))

        # prepare environment without LOD and without cone culling and without VF culling
        universal_defines = "LOD_ENABLED#0;CONE_CULLING_ENABLED#0;CULLING_ENABLED#0"
        defines = "BENCHMARK_MODE#1;" + universal_defines
        compile_shaders("DEBUG_MESH=0;" + universal_defines)
        result = compile_engine(defines)
        if result > 0:
            print("Failed to successfully compile engine")
            return
        
        result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=100000" + run_count)
        if supports_mesh_shading:
            results.append(TestResult(result, "Be klast. atm. ir be LOD ir be nem. obj. atm."))
        else:
            results.append(TestResult(result, "Be LOD ir be nem. obj. atm."))

    else:
        fake_test_results = [ TestResult(425093655, 'Visos optimizacijos')
                            , TestResult(425093736, 'Be klast. atm.')
                            , TestResult(425093759, 'Be klast. atm. ir be LOD')
                            , TestResult(425093856, 'Be klast. atm. ir be LOD ir be nem. obj. atm.')]

        results = fake_test_results

    process_combined_results(results, "Optimizacijų įtaka greitaveikai vaizduojant atsitiktinai sugeneruotą sceną su 100 t. obj.")

def test_suite_optimization_bigmesh():
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
        
        result = run_test("--file-count=1 --load-files Scene/BigDragons.glb" + run_count)
        results.append(TestResult(result, "Visos optimizacijos"))

        if supports_mesh_shading:
            # prepare environment without cone culling
            universal_defines = "CONE_CULLING_ENABLED#0"
            defines = "BENCHMARK_MODE#1;" + universal_defines
            compile_shaders("DEBUG_MESH=0;" + universal_defines)
            result = compile_engine(defines)
            if result > 0:
                print("Failed to successfully compile engine")
                return
        
            result = run_test("--file-count=1 --load-files Scene/BigDragons.glb" + run_count)
            results.append(TestResult(result, "Be klast. atm."))

        # prepare environment without LOD and without cone culling
        universal_defines = "LOD_ENABLED#0;CONE_CULLING_ENABLED#0"
        defines = "BENCHMARK_MODE#1;" + universal_defines
        compile_shaders("DEBUG_MESH=0;" + universal_defines)
        result = compile_engine(defines)
        if result > 0:
            print("Failed to successfully compile engine")
            return
        
        result = run_test("--file-count=1 --load-files Scene/BigDragons.glb" + run_count)
        if supports_mesh_shading:
            results.append(TestResult(result, "Be klast. atm. ir be LOD"))
        else:
            results.append(TestResult(result, "Be LOD"))

        # prepare environment without LOD and without cone culling and without VF culling
        universal_defines = "LOD_ENABLED#0;CONE_CULLING_ENABLED#0;CULLING_ENABLED#0"
        defines = "BENCHMARK_MODE#1;" + universal_defines
        compile_shaders("DEBUG_MESH=0;" + universal_defines)
        result = compile_engine(defines)
        if result > 0:
            print("Failed to successfully compile engine")
            return
        
        result = run_test("--file-count=1 --load-files Scene/BigDragons.glb" + run_count)
        if supports_mesh_shading:
            results.append(TestResult(result, "Be klast. atm. ir be LOD ir be nem. obj. atm."))
        else:
            results.append(TestResult(result, "Be LOD ir be nem. obj. atm."))
    else:
        fake_test_results = [ TestResult(425104239, 'Visos optimizacijos')
                            , TestResult(425104315, 'Be klast. atm.')
                            , TestResult(425104354, 'Be klast. atm. ir be LOD')
                            , TestResult(425104435, 'Be klast. atm. ir be LOD ir be nem. obj. atm.')]

        results = fake_test_results

    process_combined_results(results, "Optimizacijų įtaka greitaveikai vaizduojant Drakonų sceną")

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
        
        result = run_test("--file-count=1 --load-files Scene/big_dragon.obj --distribute=random --growth-step=1" + run_count)
        results = TestResult(result, "Augantis objektų skaičius")

        process_results_obj_count(results)
    else:
        process_results_obj_count(TestResult(427064906, "Augantis objektų skaičius"))

def test_suite_mesh():
    if supports_mesh_shading == False:
        return
    if use_premade_results == False:
        run_count = " --run-for=100"

        results_random = []
        results_dragons= []
        results_sponza = []
        combinations = [(64, 64), (64, 84), (64, 124), (48, 124), (48, 84), (48, 64), (32, 32), (32, 64), (32, 84)]
        for combo in combinations:
            universal_defines = "MESHLET_MAX_VERTS#{};MESHLET_MAX_PRIMS#{}".format(combo[0], combo[1])
            defines = "BENCHMARK_MODE#1;" + universal_defines
            compile_shaders("DEBUG_MESH=0;" + universal_defines)
            result = compile_engine(defines)
            if result > 0:
                print("Failed to successfully compile engine")
                return
            
            #result = run_test("--file-count=2 --load-files Scene/Donut.obj Scene/Suzanne.obj --distribute=random --entity-count=max" + run_count)
            #results_random.append(TestResult(result, "Random su max vir: {} ir max tri: {}". format(combo[0], combo[1])))
            #
            #result = run_test("--file-count=1 --load-files Scene/BigDragons.glb" + run_count)
            #results_dragons.append(TestResult(result, "Random su max vir: {} ir max tri: {}". format(combo[0], combo[1])))

            result = run_test("--file-count=1 --load-files Scene/sponza_wc_v_flat.glb" + run_count)
            results_sponza.append(TestResult(result, "max vir: {} ir max tri: {}". format(combo[0], combo[1])))


    else:
        results_random = [TestResult(423164443, "max vir: 64 ir max tri: 64"),
                   TestResult(423164528, "max vir: 64 ir max tri: 84"),
                   TestResult(423164614, "max vir: 64 ir max tri: 124"),
                   TestResult(423164659, "max vir: 48 ir max tri: 124"),
                   TestResult(423164744, "max vir: 48 ir max tri: 84"),
                   TestResult(423164828, "max vir: 48 ir max tri: 64"),
                   TestResult(423164912, "max vir: 32 ir max tri: 32"),
                   TestResult(423164956, "max vir: 32 ir max tri: 64"),
                   TestResult(423165040, "max vir: 32 ir max tri: 84")]

        results_dragons = [TestResult(423164501, "max vir: 64 ir max tri: 64"),
                   TestResult(423164546, "max vir: 64 ir max tri: 84"),
                   TestResult(423164631, "max vir: 64 ir max tri: 124"),
                   TestResult(423164716, "max vir: 48 ir max tri: 124"),
                   TestResult(423164801, "max vir: 48 ir max tri: 84"),
                   TestResult(423164846, "max vir: 48 ir max tri: 64"),
                   TestResult(423164929, "max vir: 32 ir max tri: 32"),
                   TestResult(423165013, "max vir: 32 ir max tri: 64"),
                   TestResult(423165058, "max vir: 32 ir max tri: 84")]
        results_sponza = [TestResult(425120641, "max vir: 64 ir max tri: 64"),
                   TestResult(425120729, "max vir: 64 ir max tri: 84"),
                   TestResult(425120818, "max vir: 64 ir max tri: 124"),
                   TestResult(425120906, "max vir: 48 ir max tri: 124"),
                   TestResult(425120954, "max vir: 48 ir max tri: 84"),
                   TestResult(425121042, "max vir: 48 ir max tri: 64"),
                   TestResult(425121127, "max vir: 32 ir max tri: 32"),
                   TestResult(425121213, "max vir: 32 ir max tri: 64"),
                   TestResult(425121258, "max vir: 32 ir max tri: 84")]
        
    process_mesh_results(results_random, "Atsitiktinai sugeneruotos scenos su 1M objektų vidutinė greitaveika")
    process_mesh_results(results_dragons, 'Drakonų scenos vidutinė greitaveika')
    process_mesh_results(results_sponza, 'Sponza scenos vidutinė greitaveika')



def main():
    ensure_test_dir_exits()
    supports_mesh_shading = check_mesh_shader_support()
    #test_suite1()
    test_suite_performance()
    #test_suite_optimization()
    #test_suite_optimization_bigmesh()
    #test_suite_object_count()
    #test_suite_mesh()

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
    print ("Compiling engine with args: " + args)
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