import os

env = SConscript("godot-cpp/SConstruct", {"api_version": "4.7"})

# 1. تفعيل C++20 ودعم الاستثناءات
env.Append(CXXFLAGS=["-std=c++20", "-fexceptions"])
env.Append(CCFLAGS=["-fexceptions"])

# 2. حزم مكتبة C++ بشكل ثابت (Static) ليعمل على كل هواتف الأندرويد بدون أي خطأ
env.Append(LINKFLAGS=["-static-libstdc++"])

# 3. مسارات الهيدرز
env.Append(CPPPATH=[
    "src/",
    "libs/pmp-library/src/",
    "libs/eigen/"
])

sources = (
    Glob("src/*.cpp") +
    Glob("libs/pmp-library/src/pmp/*.cpp") +
    Glob("libs/pmp-library/src/pmp/algorithms/*.cpp")
)

# ضبط التسمية الصحيحة لإنتاج libcarstudio.android.template_debug.arm64.so
library = env.SharedLibrary(
    "bin/carstudio{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source=sources,
)

Default(library)
