import os

env = SConscript("godot-cpp/SConstruct", {"api_version": "4.7"})

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

library = env.SharedLibrary(
    "bin/libcarstudio{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source=sources,
)

Default(library)
