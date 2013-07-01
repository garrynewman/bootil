solution "Bootil"

	language "C++"
	location ( os.get() .. "/" .. _ACTION )
	flags { "Symbols", "NoEditAndContinue", "NoPCH", "StaticRuntime", "EnableSSE" }
	targetdir ( "../lib/" .. os.get() .. "/" .. _ACTION )
	includedirs { "../include/", "../src/3rdParty/" }
	
	if os.is( "linux" ) or os.is( "macosx" ) then
		buildoptions { "-fPIC" }
		-- linkoptions  { "-fPIC" }  -- OSX does not need this for linking
	end

	configurations
	{
		"Release",
		"Debug"
	}

configuration "Release"
	defines { "NDEBUG" }
	flags{ "OptimizeSpeed", "FloatFast" }

configuration "Debug"
	defines { "_DEBUG" }
	targetsuffix "_debug"

project "Bootil-Shared"
	uuid ( "22348F3E-2CE7-42fd-B7DC-2861A17D9FEE" )
	defines { "BOOTIL_COMPILE_DLL" }
	files { "../src/**.cpp", "../src/**.c", "../src/**.cc" }
	kind "SharedLib"
	targetname( "bootil" )

project "Bootil-Static"
	uuid ( "AB8E7B19-A70C-4767-88DE-F02160167C2E" )
	defines { "BOOTIL_COMPILE_STATIC" }
	files { "../src/**.cpp", "../src/**.c", "../src/**.cc" }
	kind "StaticLib"
	targetname( "bootil_static" )
