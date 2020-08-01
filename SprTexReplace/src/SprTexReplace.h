#pragma once
#include "Types.h"
#include "SourceImage.h"
#include "SprSet.h"
#include <vector>
#include <future>

namespace SprTexReplace
{
	using GLGenTextures_t = void __stdcall (i32, u32*);
	using GLCompressedTexImage2D_t = void __stdcall (u32 target, i32 level, u32 internalFormat, i32 width, i32 height, i32 border, i32 imageSize, const void* data);

	using GetFileContent_t = u8 * __fastcall (const void* handle);
	using SprSetParseTexSet_t = int __cdecl (SprSet* thisSprSet);

	namespace Hooks
	{
		void __stdcall GLGenTextures(i32 count, u32* outGLTexIDs);
		int __cdecl SprSetParseTexSet(SprSet* thisSprSet);
	}

	struct SprTexInfo
	{
		std::string TextureName;
		std::string ImageSourcePath;
	};

	struct SprSetInfo
	{
		std::string SetName;
		std::vector<SprTexInfo> Textures;
	};

	struct PluginState : NonCopyable
	{
		PluginState();
		~PluginState() = default;

		std::string SprTexReplaceSubDirectory = "plugins/spr_tex_replace";
		std::string WorkingDirectory;

		GLGenTextures_t* OriginalGLGenTextures = nullptr;
		SprSetParseTexSet_t* OriginalParseSprSetTexSet = nullptr;

		bool GenTexturesHookEnabled = false;
		std::vector<u32> InterceptedGLTextureIDs;

		std::future<std::vector<SprSetInfo>> RegisteredReplaceInfoFuture;
		std::vector<SprSetInfo> RegisteredReplaceInfo;

		std::vector<std::unique_ptr<SourceImage>> TempSourceImageBuffer;

		void WaitForAsyncDirectoryUpdate();
		void IterateRegisterReplaceDirectoryFilePathsAsync();
	};

	extern PluginState EvilGlobalState;
}
