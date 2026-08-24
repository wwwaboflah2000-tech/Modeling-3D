import os

env = SConscript("godot-cpp/SConstruct", {"api_version": "4.7"})
env.Append(CPPPATH=["src/", "libs/meshoptimizer/src/", "libs/"])

sources = Glob("src/*.cpp") + Glob("libs/meshoptimizer/src/*.cpp")

library = env.SharedLibrary(
    target="bin/libcarstudio.{}.{}.{}".format(env["platform"], env["target"], env["arch"]),
    source=sources,
)
Default(library)
