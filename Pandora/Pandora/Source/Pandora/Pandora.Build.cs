// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using System.IO;
using System.Reflection;
using UnrealBuildTool;

public class Pandora : ModuleRules
{
    public Pandora(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        var externalSource = Path.Combine(ModuleDirectory, "./Private/Externals");
        var luaSource = Path.Combine(externalSource, "./lua/inc");
        var sluaSource = Path.Combine(externalSource, "./slua/inc");
        var luaExtenders = Path.Combine(ModuleDirectory, "./Private/LuaExtenders");

        var luaLib = Path.Combine(ModuleDirectory, "../ThirdParty");

        PublicDefinitions.Add("PDR_ENABLE_WEBSOCKETS=0");
        PublicDefinitions.Add("PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD=0");
        bEnableExceptions = true;

        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
                externalSource,
                luaSource,
                Path.Combine(luaSource, "lua"),
                Path.Combine(luaSource, "luasocket"),
                sluaSource,
                Path.Combine(sluaSource, "BufferExtension"),
                Path.Combine(sluaSource, "LuaSocketExtension"),
                Path.Combine(ModuleDirectory, "./Private"),
                luaExtenders,
            }
            );
            
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "Paper2D",
                "UMG",
                //"slua_unreal",
                // ... add other public dependencies that you statically link with here ...
            }
            );
            
        
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "PakFile",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "Json",
                "JsonUtilities",
                "Http",
                "UMG",
                "Paper2D",
                "Sockets",
#if UE_4_21_OR_LATER
                "WebSockets",
#else
#endif
                // ... add private dependencies that you statically link with here ...  
            }
            );

        PublicIncludePathModuleNames.AddRange(
            new string[]
            {
            });



        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
            );
        


#if UE_4_21_OR_LATER
#else
        Definitions.Add("IGNORE_LUA_WEBSOCKET_WRAP");
#endif

        if (Target.Type == TargetRules.TargetType.Editor)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
        }

        if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32)
        {
            PrivateDependencyModuleNames.Add("Projects");
            PublicDefinitions.Add("__BYTE_ORDER__=0");
            PublicDefinitions.Add("__ORDER_LITTLE_ENDIAN__=0");
            PublicDefinitions.Add("WIN32_LEAN_AND_MEAN");
            if (Target.Platform == UnrealTargetPlatform.Win64)
            {
                //PublicAdditionalLibraries.Add(Path.Combine(luaLib, "Win64/pdrlua.lib"));
            }
            if (Target.Platform == UnrealTargetPlatform.Win32)
            {
                //PublicAdditionalLibraries.Add(Path.Combine(luaLib, "Win32/pdrlua.lib"));
            }
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
#if UE_4_24_OR_LATER
            //PublicAdditionalLibraries.Add(Path.Combine(luaLib, "Android/arm64-v8a/libpdrlua.a"));
            //PublicAdditionalLibraries.Add(Path.Combine(luaLib, "Android/armeabi-v7a/libpdrlua.a"));
            //PublicAdditionalLibraries.Add(Path.Combine(luaLib, "Android/x86/libpdrlua.a"));
#else
            //PublicLibraryPaths.Add(Path.Combine(luaLib, "Android/arm64-v8a"));
            //PublicLibraryPaths.Add(Path.Combine(luaLib, "Android/armeabi-v7a"));
            //PublicLibraryPaths.Add(Path.Combine(luaLib, "Android/x86"));
            //PublicAdditionalLibraries.Add("pdrlua");
#endif
            PublicDefinitions.Add("___PDR_LUA_USE_LONGJMP");
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            //PublicAdditionalLibraries.Add(Path.Combine(luaLib, "Linux/libpdrlua.a"));
            PublicDefinitions.Add("___PDR_LUA_USE_LONGJMP");
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            //PublicAdditionalLibraries.Add(Path.Combine(luaLib, "iOS/libpdrlua.a"));
            PublicDefinitions.Add("___PDR_LUA_USE_LONGJMP");
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            //PublicAdditionalLibraries.Add(Path.Combine(luaLib, "Mac/libpdrlua.a"));
            PrivateDependencyModuleNames.Add("Projects");
            PublicDefinitions.Add("___PDR_LUA_USE_LONGJMP");
        }
    }
}
