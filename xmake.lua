add_requires("fmt")
add_rules("mode.debug", "mode.release")

-- A list of warnings:
local warnings = {
    "all",      -- turn on all warnings
    "extra",    -- turn on extra warnings
    "error",    -- treat warnings as errors
    "pedantic", -- be pedantic
}

local other_cxflags = {
    "-Wswitch-default",     -- warn if no default case in a switch statement
}

target("reimu")
    set_kind("binary")
    set_warnings(warnings)
    add_cxflags(other_cxflags)
    add_includedirs("include/")
    add_files("src/*/*.cpp")
    add_files("src/main.cpp")
    set_toolchains("gcc")
    set_languages("c++23")
    add_packages("fmt")
