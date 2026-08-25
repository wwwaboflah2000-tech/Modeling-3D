import os

env = SConscript("godot-cpp/SConstruct", {"api_version": "4.7"})

env.Append(CPPPATH=[
    "src/",
    "libs/meshoptimizer/src/",
    "libs/",
    "libs/OpenSubdiv/opensubdiv/"
])

sources = (
    Glob("src/*.cpp") +
    Glob("libs/meshoptimizer/src/*.cpp") +
    Glob("libs/OpenSubdiv/opensubdiv/far/*.cpp") +
    Glob("libs/OpenSubdiv/opensubdiv/vtr/*.cpp") +
    Glob("libs/OpenSubdiv/opensubdiv/sdc/*.cpp")
)

# الصيغة المعيارية الرسمية لجودوت لإنشاء ملف الـ .so
library = env.SharedLibrary(
    "bin/libcarstudio{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source=sources,
)

Default(library)
