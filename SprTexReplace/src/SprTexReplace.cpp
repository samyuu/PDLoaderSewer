#include "SprTexReplace.h"
#include "GLTextureUpload.h"
#include "Path.h"
#include "UTF8.h"
#include "Log.h"
#include "StringUtil.h"
#include <filesystem>

namespace SprTexReplace
{
	PluginState EvilGlobalState = {};

	PluginState::PluginState()
	{
		constexpr auto reasonablePerSprSetMaxTextureCount = 128;

		InterceptedGLTextureIDs.reserve(reasonablePerSprSetMaxTextureCount);
		TempImageFileBuffer.reserve(reasonablePerSprSetMaxTextureCount);
	}

	namespace
	{
		void TryRegisterSprTexReplacePath(const std::filesystem::path& texturePath, std::vector<SprTexInfo>& outResults)
		{
			const auto textureFileName = texturePath.filename().u8string();
			if (!ImageFile::IsValidFileName(textureFileName))
				return;

			auto& sprTexInfo = outResults.emplace_back();
			sprTexInfo.TextureName = std::string(Path::TrimExtension(textureFileName));
			sprTexInfo.ImageFilePath = texturePath.u8string();
		}

		void TryRegisterSprSetReplaceDirectory(const std::filesystem::path& sprSetDirectory, std::vector<SprSetInfo>& outResults)
		{
			constexpr auto sprPrefixThatAllFArcsShouldHave = std::string_view { "spr_" };

			auto directoryName = sprSetDirectory.filename().u8string();
			if (!Util::StartsWithInsensitive(directoryName, sprPrefixThatAllFArcsShouldHave))
				return;

			auto& sprSetInfo = outResults.emplace_back(SprSetInfo { std::move(directoryName) });
			for (const auto sprTexPath : std::filesystem::directory_iterator(sprSetDirectory))
				TryRegisterSprTexReplacePath(sprTexPath.path(), sprSetInfo.Textures);
		}
	}

	void PluginState::WaitForAsyncDirectoryUpdate()
	{
		if (RegisteredReplaceInfoFuture.valid())
			RegisteredReplaceInfo = std::move(RegisteredReplaceInfoFuture.get());
	}

	void PluginState::IterateRegisterReplaceDirectoryFilePathsAsync()
	{
		RegisteredReplaceInfoFuture = std::async(std::launch::async, [this]
		{
			std::vector<SprSetInfo> results;

			const auto directoryToSerach = (ThisDLLDirectory + "/" + SprTexReplaceSubDirectory);
			const auto directoryToSerachPath = std::filesystem::path(UTF8::WideArg(directoryToSerach).c_str());

			try
			{
				for (const auto sprSetDirectory : std::filesystem::directory_iterator(directoryToSerachPath))
					TryRegisterSprSetReplaceDirectory(sprSetDirectory.path(), results);
			}
			catch (const std::exception& ex)
			{
				LOG_WRITELINE("Exception thrown: %s", ex.what());
			}

			return results;
		});
	}
}

namespace SprTexReplace
{
	namespace
	{
		const SprSetInfo* FindMatchingSprSetReplaceInfo(const SprSet& sprSet)
		{
			EvilGlobalState.WaitForAsyncDirectoryUpdate();

			const auto setNameToFind = sprSet.GetSprName();
			const auto foundReplacement = std::find_if(
				EvilGlobalState.RegisteredReplaceInfo.begin(),
				EvilGlobalState.RegisteredReplaceInfo.end(),
				[&](const auto& info) { return (info.SetName == setNameToFind); });

			return (foundReplacement != EvilGlobalState.RegisteredReplaceInfo.end() && !foundReplacement->Textures.empty()) ? &(*foundReplacement) : nullptr;
		}

		const SprTexInfo* FindMatchingSprTexReplaceInfo(const SprSet& sprSet, const size_t texIndex, const SprSetInfo& setInfo)
		{
			const auto textureNameToFind = sprSet.GetTextureName(texIndex);
			const auto foundReplacement = std::find_if(
				setInfo.Textures.begin(),
				setInfo.Textures.end(),
				[&](const auto& info) { return (info.TextureName == textureNameToFind); });

			return (foundReplacement != setInfo.Textures.end()) ? &(*foundReplacement) : nullptr;
		}

		bool LoadImageFilesIntoTempBuffer(const SprSet& sprSet, const SprSetInfo& setInfo)
		{
			const auto textureCount = sprSet.GetTextureCount();

			EvilGlobalState.TempImageFileBuffer.clear();
			EvilGlobalState.TempImageFileBuffer.reserve(textureCount);

			for (size_t textureIndex = 0; textureIndex < textureCount; textureIndex++)
			{
				// TODO: Instead of loading textures just in time it'd be a lot more efficient to async load them at the same time the original farc is read
				const auto replaceInfo = FindMatchingSprTexReplaceInfo(sprSet, textureIndex, setInfo);

				if (replaceInfo != nullptr)
					EvilGlobalState.TempImageFileBuffer.emplace_back(replaceInfo->ImageFilePath);
				else
					EvilGlobalState.TempImageFileBuffer.push_back(std::nullopt);
			}

			const auto anyReplacementFound = std::any_of(
				EvilGlobalState.TempImageFileBuffer.begin(),
				EvilGlobalState.TempImageFileBuffer.end(),
				[&](const auto& image) { return image.has_value(); });;

			return anyReplacementFound;
		}

		void PreventYCbCrTextureDetection(SprSet& sprSet, const size_t textureIndex)
		{
			sprSet.ForEachMipMap(textureIndex, [&](SprSet::TextureData& texture, SprSet::MipMapData& mipMap)
			{
				// NOTE: Prevent YCbCr detection and it's multiple mipmaps messing up the plugin replaced textures
				if (texture.MipLevels == 2 && mipMap.Format == TextureFormat::RGTC2)
				{
					// NOTE: Change to DXT5 because it uses the same block size
					mipMap.Format = TextureFormat::DXT5;

					if (mipMap.MipIndex == 1)
						texture.MipMapCount = texture.MipLevels = 1;
				}
			});
		}

		void DoSprSetTexReplacementsPreLoad(SprSet& sprSet)
		{
			for (size_t textureIndex = 0; textureIndex < sprSet.GetTextureCount(); textureIndex++)
			{
				if (EvilGlobalState.TempImageFileBuffer[textureIndex].has_value())
					PreventYCbCrTextureDetection(sprSet, textureIndex);
			}
		}

		void UploadImageFileToGLTexture(const ImageFile& imageFile, const u32 glTextureID)
		{
			// BUG: Potential bug if the source image file failes to load but the YCbCr textures have already been pre-load edited
			const auto imageView = imageFile.GetImageView();

			if (imageView.Data == nullptr || imageView.Width < 1 || imageView.Height < 1)
			{
				const auto imageFilePath = imageFile.GetFilePath();
				LOG_WRITELINE("Failed to load '%.*s'! Corrupted YCbCr textures expected.", static_cast<int>(imageFilePath.size()), imageFilePath.data());

				return;
			}

			GLBindUploadImageViewTextureData(imageFile, imageView, glTextureID);
		}

		void DoSprSetTexReplacementsPostLoad(const SprSet& sprSet)
		{
			const auto textureCount = sprSet.GetTextureCount();
			for (size_t textureIndex = 0; textureIndex < textureCount; textureIndex++)
			{
				if (!EvilGlobalState.TempImageFileBuffer[textureIndex].has_value())
					continue;

				const auto& imageFile = EvilGlobalState.TempImageFileBuffer[textureIndex].value();

				const auto interceptedIndex = EvilGlobalState.InterceptedGLTextureIDs.size() - textureCount + textureIndex;
				if (interceptedIndex >= EvilGlobalState.InterceptedGLTextureIDs.size())
					continue;

				const auto glTextureID = EvilGlobalState.InterceptedGLTextureIDs[interceptedIndex];
				if (glTextureID == InvalidGLTextureID)
					continue;

				UploadImageFileToGLTexture(imageFile, glTextureID);
			}
		}
	}
}

namespace SprTexReplace::Hooks
{
	void __stdcall GLGenTextures(i32 count, u32* outGLTexIDs)
	{
		assert(EvilGlobalState.OriginalGLGenTextures != nullptr);
		EvilGlobalState.OriginalGLGenTextures(count, outGLTexIDs);

		if (EvilGlobalState.GenTexturesHookEnabled)
		{
			for (size_t i = 0; i < count; i++)
				EvilGlobalState.InterceptedGLTextureIDs.push_back((outGLTexIDs != nullptr) ? outGLTexIDs[i] : InvalidGLTextureID);
		}
	}

	int __cdecl SprSetParseTexSet(SprSet* thisSprSet)
	{
		if (thisSprSet == nullptr)
			return EvilGlobalState.OriginalParseSprSetTexSet(thisSprSet);

		const auto matchingSetReplaceInfo = FindMatchingSprSetReplaceInfo(*thisSprSet);
		if (matchingSetReplaceInfo == nullptr)
			return EvilGlobalState.OriginalParseSprSetTexSet(thisSprSet);

		const auto anyReplacementFound = LoadImageFilesIntoTempBuffer(*thisSprSet, *matchingSetReplaceInfo);
		if (!anyReplacementFound)
			return EvilGlobalState.OriginalParseSprSetTexSet(thisSprSet);

		DoSprSetTexReplacementsPreLoad(*thisSprSet);

		// TODO: Before calling the original function it'd me a lot more efficient to prevent it from uploading its own data for the textures that are known to be replaced. 
		//		 Doing that should be as simple as also hooking the uploader sub function(s) and checking against the currently uploaded texture
		EvilGlobalState.InterceptedGLTextureIDs.clear();
		EvilGlobalState.GenTexturesHookEnabled = true;
		const auto originalReturnValue = EvilGlobalState.OriginalParseSprSetTexSet(thisSprSet);
		EvilGlobalState.GenTexturesHookEnabled = false;

		DoSprSetTexReplacementsPostLoad(*thisSprSet);
		EvilGlobalState.TempImageFileBuffer.clear();

		return originalReturnValue;
	}
}
