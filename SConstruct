import os

env = SConscript("godot-cpp/SConstruct", {"api_version": "4.7"})

# 1. تفعيل معيار C++20 ودعم الـ Exceptions لمكتبة PMP
env.Append(CXXFLAGS=["-std=c++20", "-fexceptions"])
env.Append(CCFLAGS=["-fexceptions"])

# 2. إضافة مسارات الهيدرز
env.Append(CPPPATH=[
    "src/",
    "libs/pmp-library/src/",
    "libs/eigen/"
])

# 3. تجميع ملفات C++
sources = (
    Glob("src/*.cpp") +
    Glob("libs/pmp-library/src/pmp/*.cpp") +
    Glob("libs/pmp-library/src/pmp/algorithms/*.cpp")
)

library = env.SharedLibrary(
    "bin/libcarstudio{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source=sources,
)

Default(library)
