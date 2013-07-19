solution "Bootil"

	language "C++"
	location ( os.get() .. "/" .. _ACTION )
	flags { "Symbols", "NoEditAndContinue", "NoPCH", "StaticRuntime", "EnableSSE" }
	targetdir ( "../lib/" .. os.get() .. "/" .. _ACTION )
	includedirs { "../include/", "../src/3rdParty/" }
	defines { "BOOST_ALL_NO_LIB" }
	
	if os.is( "linux" ) or os.is( "macosx" ) then
		buildoptions { "-fPIC" }
	end
	
	if os.is( "windows" ) then
		links { "WS2_32" }
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


project "Bootil-Static"
	uuid ( "AB8E7B19-A70C-4767-88DE-F02160167C2E" )
	defines { "BOOTIL_COMPILE_STATIC" }
	files { "../src/**.cpp", "../include/**.h", "../src/**.c", "../src/**.cc", "premake4.lua" }
	kind "StaticLib"
	targetname( "bootil_static" )
