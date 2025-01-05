add_rules("mode.debug", "mode.release")

add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")
add_repositories("engsr6982-repo https://github.com/engsr6982/xmake-repo.git")

-- add_requires("levilamina x.x.x") for a specific version
-- add_requires("levilamina develop") to use develop version
-- please note that you should add bdslibrary yourself if using dev version
if is_config("target_type", "server") then
    add_requires("levilamina 1.0.0-rc.1", {configs = {target_type = "server"}})
else
    add_requires("levilamina 1.0.0-rc.1", {configs = {target_type = "client"}})
end
add_requires("levibuildscript")
add_requires("exprtk 0.0.3")
add_requires("legacymoney 0.9.0-rc.1")
-- add_requires("more_events 0.2.0") --deprecated

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

option("target_type")
    set_default("server")
    set_showmenu(true)
    set_values("server", "client")
option_end()

target("PLand") -- Change this to your mod name.
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    add_cxflags(
        "/EHa",
        "/utf-8",
        "/W4",
        "/w44265",
        "/w44289",
        "/w44296",
        "/w45263",
        "/w44738",
        "/w45204"
    )
    add_defines("NOMINMAX", "UNICODE", "LDAPI_EXPORT")
    add_files("src/**.cpp", "src/**.cc")
    add_includedirs("src", "include")
    add_packages(
        "levilamina",
        "exprtk",
        -- "more_events",
        "legacymoney"
    )

    -- MoreEvents test
    -- add_includedirs("D:/Projects/MoreEvents/bin/include")
    -- add_links("D:/Projects/MoreEvents/bin/lib/*.lib")

    -- add_shflags("/DELAYLOAD:bedrock_server.dll") -- To use symbols provided by SymbolProvider.
    set_exceptions("none") -- To avoid conflicts with /EHa.
    set_kind("shared")
    set_languages("c++20")

    add_defines("PLUGIN_NAME=\"[PLand] \"")

    if is_mode("debug") then
        add_defines("DEBUG", "LL_I18N_COLLECT_STRINGS")
        set_symbols("debug", "edit")
    else 
        set_symbols("debug")
    end 

    after_build(function (target)
        local bindir = path.join(os.projectdir(), "bin")
        local outputdir = path.join(bindir, target:name())
        -- Lang
        local langdir = path.join(os.projectdir(), "assets", "lang")
        os.cp(langdir, outputdir)
    end)
