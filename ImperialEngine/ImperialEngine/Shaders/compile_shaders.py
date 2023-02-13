#store hash for this folder
#if hash is different then compile shaders
import hashlib, os
COMPILER_PATH = os.environ.get("VK_SDK_PATH") + "\\Bin\\glslangValidator.exe"
FORCE_COMPILE = True

# TODO: I think currently this tries to compile spv files cus they have the '.frag' and etc in their name

def CompileDir(directory):
    #gather all .frag, .vert files
    #compile them all
    for root, dirs, files in os.walk(directory):
        for names in files:
            pos = max(names.rfind(".frag"), names.rfind(".vert"), names.rfind(".mesh"), names.rfind(".comp"))

            if names.find(".spv") != -1 or names.find(".bak") != -1:
              continue
            if pos > 0:
                fn = names[:pos]
                ft = names[pos + 1:]
                renamed = directory + "\\spir-v\\" + fn + "." + ft + ".spv"
                args = COMPILER_PATH + " -V --target-env vulkan1.2" + " -o" + " " + renamed + " " + directory + "\\glsl\\" + names
                #print(args)
                os.system(args)


def GetHashofDirs(directory, verbose=0):
  SHAhash = hashlib.md5()
  if not os.path.exists (directory):
    return -1

  try:
    for root, dirs, files in os.walk(directory):
      for names in files:
        if names == ".dirhash":
            continue
        if verbose == 1:
          print('Hashing', names)
        filepath = os.path.join(root,names)
        try:
          f1 = open(filepath, 'rb')
        except:
          # You can't open the file for some reason
          f1.close()
          continue

        while 1:
          # Read file in as little chunks
          buf = f1.read(4096)
          if not buf : break
          SHAhash.update(hashlib.md5(buf).hexdigest().encode("utf-8"))
        f1.close()

  except:
    import traceback
    # Print the stack traceback
    traceback.print_exc()
    return -2

  return SHAhash.hexdigest()

#changeback = False
curdir = os.getcwd()

if curdir.rfind("Shaders") < 0:
    os.chdir("Shaders")
    #changeback = Trues
hash = GetHashofDirs(os.getcwd())
f = open(".dirhash", "r+")
oldhash = f.read(32)
if hash != oldhash or FORCE_COMPILE:
  print("Compiling shaders..")
  CompileDir(os.getcwd())
  hash = GetHashofDirs(os.getcwd())
  f.seek(0)
  f.write(hash)
  f.truncate()
    #print("Old hash: ", oldhash)
    #print("New hash: ", hash)
else:
    print("No changes in Shaders")
f.close()