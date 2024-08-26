engine_files = {"mint_engine/src/**.h", "mint_engine/src/**.c", "mint_engine/src/**.cpp"}
game_files = {"src/**.h", "src/**.c", "src/**.cpp"}
_files = {}
for k,v in ipairs(engine_files) do
    table.insert(_files, v)
end 
for k,v in ipairs(game_files) do
    table.insert(_files, v)
end 


workspace "Golf"
    configurations {"Debug", "Release"}

project "Golf"
    kind "ConsoleApp"
    language "C++"
    targetdir "bin"
    files(_files)
    removefiles {"mint_engine/src/external/**", }

    vpaths {
       ["Engine/*"] = engine_files,
       ["Game/*"] = game_files,
    }

    includedirs{"mint_engine/src"}

    filter {"system:windows"}
        defines{"_CRT_SECURE_NO_WARNINGS"}

    filter "configurations:Debug"
        defines{"DEBUG"}
        symbols "On"
        architecture "x86_64"

    filter "configurations:Release"
        defines{"NDEBUG"}
        optimize "On"
        architecture "x86_64"