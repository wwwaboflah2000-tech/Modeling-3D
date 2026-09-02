import os

env = SConscript("godot-cpp/SConstruct", {"api_version": "4.7"})

env.Append(CXXFLAGS=["-std=c++20", "-fexceptions", "-O3"])
env.Append(CCFLAGS=["-fexceptions", "-O3"])

env.Append(LINKFLAGS=["-static-libstdc++"])

env.Append(CPPPATH=["src/", "libs/eigen/"])

sources = Glob("src/*.cpp")

library = env.SharedLibrary(
    "bin/carstudio{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source=sources,
)

Default(library)
