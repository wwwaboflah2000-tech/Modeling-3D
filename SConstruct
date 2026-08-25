import os

env = SConscript("godot-cpp/SConstruct", {"api_version": "4.7"})

# إضافة مسارات الهيدرز للمكتبات
env.Append(CPPPATH=[
    "src/",
    "libs/meshoptimizer/src/",
    "libs/",
    "libs/OpenSubdiv/opensubdiv/"
])

# تجميع مصادر C++ الخاصة بنا ومصادر OpenSubdiv و MeshOptimizer
sources = (
    Glob("src/*.cpp") +
    Glob("libs/meshoptimizer/src/*.cpp") +
    Glob("libs/OpenSubdiv/opensubdiv/far/*.cpp") +
    Glob("libs/OpenSubdiv/opensubdiv/vtr/*.cpp") +
    Glob("libs/OpenSubdiv/opensubdiv/sdc/*.cpp")
)

library = env.SharedLibrary(
    target="bin/libcarstudio.{}.{}.{}".format(env["platform"], env["target"], env["arch"]),
    source=sources,
)
Default(library)
