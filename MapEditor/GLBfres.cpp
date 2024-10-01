#include "GLBfres.h"

#include "Logger.h"
#include "GLFormatHelper.h"
#include "Util.h"
#include "Editor.h"

std::unordered_map<BfresFile::BfresAttribFormat, GLBfres::FormatInfo> GLBfres::mFormatList =
{
	{ BfresFile::BfresAttribFormat::Format_32_32_32_32_Single, {4, false, GL_FLOAT} },
	{ BfresFile::BfresAttribFormat::Format_32_32_32_Single, {3, false, GL_FLOAT} },
	{ BfresFile::BfresAttribFormat::Format_32_32_Single, {2, false, GL_FLOAT} },
	{ BfresFile::BfresAttribFormat::Format_32_Single, {1, false, GL_FLOAT} },

	{ BfresFile::BfresAttribFormat::Format_16_16_16_16_Single, {4, false, GL_HALF_FLOAT} },
	{ BfresFile::BfresAttribFormat::Format_16_16_Single, {2, false, GL_HALF_FLOAT} },
	{ BfresFile::BfresAttribFormat::Format_16_Single, {2, false, GL_HALF_FLOAT} },

	{ BfresFile::BfresAttribFormat::Format_16_16_SNorm, {2, true, GL_SHORT} },
	{ BfresFile::BfresAttribFormat::Format_16_16_UNorm, {2, true, GL_UNSIGNED_SHORT} },

	{ BfresFile::BfresAttribFormat::Format_10_10_10_2_SNorm, {4, true, GL_INT_2_10_10_10_REV} },
	{ BfresFile::BfresAttribFormat::Format_10_10_10_2_UNorm, {4, true, GL_UNSIGNED_INT_2_10_10_10_REV} },

	{ BfresFile::BfresAttribFormat::Format_8_8_8_8_SNorm, {4, true, GL_BYTE} },
	{ BfresFile::BfresAttribFormat::Format_8_8_8_8_UNorm, {4, true, GL_UNSIGNED_BYTE} },

	{ BfresFile::BfresAttribFormat::Format_8_8_UNorm, {2, true, GL_UNSIGNED_BYTE} },
	{ BfresFile::BfresAttribFormat::Format_8_8_SNorm, {2, true, GL_BYTE} },
	{ BfresFile::BfresAttribFormat::Format_8_UNorm, {1, true, GL_UNSIGNED_BYTE} },

	//Ints
	{ BfresFile::BfresAttribFormat::Format_10_10_10_2_UInt, {4, true, GL_UNSIGNED_INT} },

	{ BfresFile::BfresAttribFormat::Format_8_8_8_8_UInt, {4, false, GL_UNSIGNED_BYTE} },
	{ BfresFile::BfresAttribFormat::Format_8_8_UInt, {2, false, GL_UNSIGNED_BYTE} },
	{ BfresFile::BfresAttribFormat::Format_8_UInt, {1, false, GL_UNSIGNED_BYTE} },

	{ BfresFile::BfresAttribFormat::Format_16_16_16_16_UInt, {4, false, GL_UNSIGNED_SHORT} },
	{ BfresFile::BfresAttribFormat::Format_16_16_UInt, {2, false, GL_UNSIGNED_SHORT} },
	{ BfresFile::BfresAttribFormat::Format_16_UInt, {1, false, GL_UNSIGNED_SHORT} },
};

std::unordered_map<std::string, uint32_t> GLBfres::mAttributeLocations =
{
	{"_p0", 0},
	{"_n0", 1},
	{"_t0", 2},
	{"_u0", 3},
	{"_instanceMatrix", 4},
};

void GLBfres::LoadFallbackTexture(GLMaterial& Material)
{
	Material.mAlbedoTexture = GLTextureLibrary::GetTexture(&TextureToGoLibrary::GetTexture("TexturedCube.txtg")->GetSurface(0));
}

GLBfres::GLBfres(BfresFile* Bfres, GLenum TextureFilter) : mBfres(Bfres)
{
	mInstanceMatrix.GenBuffer(GL_ARRAY_BUFFER);

	for (auto& [ModelKey, ModelVal] : Bfres->Models.Nodes)
	{
		BfresFile::Model& Model = ModelVal.Value;
		mIndexBuffers.resize(Model.Shapes.Nodes.size());
		mShapeVAOs.resize(Model.Shapes.Nodes.size());
		mShapeBuffers.resize(Model.Shapes.Nodes.size());
		mMaterials.resize(Model.Shapes.Nodes.size());
		uint16_t ShapeIndex = 0;
		for (auto& [ShapeKey, ShapeVal] : Model.Shapes.Nodes)
		{
			BfresFile::Shape& Shape = ShapeVal.Value;

			if (Model.Materials.GetByIndex(Shape.MaterialIndex).Value.Textures.empty())
			{
				mIndexBuffers.resize(mIndexBuffers.size() - 1);
				mShapeVAOs.resize(mShapeVAOs.size() - 1);
				mShapeBuffers.resize(mShapeBuffers.size() - 1);
				mMaterials.resize(mMaterials.size() - 1);
				continue;
			}

			mIndexBuffers[ShapeIndex].first.GenBuffer(GL_ELEMENT_ARRAY_BUFFER);
			mIndexBuffers[ShapeIndex].first.SetData<unsigned char>(Shape.Meshes[0].IndexBuffer);
			mIndexBuffers[ShapeIndex].second = Shape.Meshes[0].IndexCount;

			mShapeBuffers[ShapeIndex].resize(Shape.Buffer.Buffers.size() + 1);

			for (auto& [RenderInfoKey, RenderInfoVal] : Model.Materials.Nodes[Model.Materials.GetKey(Shape.MaterialIndex)].Value.RenderInfos.Nodes)
			{
				if (RenderInfoVal.Value.Name == "gsys_alpha_test_enable")
				{
					mMaterials[ShapeIndex].mEnableAlphaTest = RenderInfoVal.Value.GetValueStrings()[0] == "true";
				}
				else if (RenderInfoVal.Value.Name == "gsys_render_state_mode")
				{

					if (RenderInfoVal.Value.GetValueStrings()[0] == "opaque")
					{
						mMaterials[ShapeIndex].mRenderStateMode = RenderStateMode::OPAQUE;
					}
					else if (RenderInfoVal.Value.GetValueStrings()[0] == "mask")
					{
						mMaterials[ShapeIndex].mRenderStateMode = RenderStateMode::MASK;
					}
					else
					{
						mMaterials[ShapeIndex].mRenderStateMode = RenderStateMode::TRANSLUCENT;
					}
				}
				else if (RenderInfoVal.Value.Name == "gsys_render_state_display_face")
				{

					if (RenderInfoVal.Value.GetValueStrings()[0] == "front")
					{
						mMaterials[ShapeIndex].mRenderStateDisplayFace = GL_BACK;
					}
					else if (RenderInfoVal.Value.GetValueStrings()[0] == "back")
					{
						mMaterials[ShapeIndex].mRenderStateDisplayFace = GL_FRONT;
					}
					//else if (RenderInfoVal.Value.GetValueStrings()[0] == "both")
					//{
						//mMaterials[ShapeIndex].mRenderStateDisplayFace = GL_NONE;
					//}
					else if (RenderInfoVal.Value.GetValueStrings()[0] == "none" || RenderInfoVal.Value.GetValueStrings()[0] == "both")
					{
						mMaterials[ShapeIndex].mRenderStateDisplayFace = GL_NONE;
					}
					else
					{
						Logger::Warning("GLBfres", "Unknown gsys_render_state_display_face value: " + RenderInfoVal.Value.GetValueStrings()[0]);
					}
				}
			}
			for (auto& [ParametersKey, ParametersVal] : Model.Materials.GetByIndex(Shape.MaterialIndex).Value.ShaderParams.Nodes)
			{
				if (ParametersVal.Value.Name == "p_texture_array_index0")
				{
					mMaterials[ShapeIndex].mTextureArrayIndex = (uint32_t)std::get<float>(ParametersVal.Value.DataValue);
				}
			}

			mIsDiscard = mIsDiscard || mMaterials[ShapeIndex].mEnableAlphaTest;

			uint32_t AlbedoCount = 0;
			std::vector<TextureToGo*> Textures;
			for (auto& [Key, Val] : Model.Materials.GetByIndex(Shape.MaterialIndex).Value.Samplers.Nodes)
			{
				if (Key.starts_with("_a") && Key.length() == 3)
				{
					if (Util::FileExists((Bfres->mTexDir == "" ? Editor::GetRomFSFile("TexToGo/" + Model.Materials.GetByIndex(Shape.MaterialIndex).Value.Textures[AlbedoCount] + ".txtg") : (Bfres->mTexDir + "/" + Model.Materials.GetByIndex(Shape.MaterialIndex).Value.Textures[AlbedoCount] + ".txtg"))))
					{
						Textures.push_back(TextureToGoLibrary::GetTexture(Model.Materials.GetByIndex(Shape.MaterialIndex).Value.Textures[AlbedoCount] + ".txtg", Bfres->mTexDir));
						AlbedoCount++;
					}
				}
			}

			if (AlbedoCount == 0)
			{
				if (!Util::FileExists(Bfres->mTexDir == "" ? Editor::GetRomFSFile("TexToGo/" + Model.Materials.GetByIndex(Shape.MaterialIndex).Value.Textures.front() + ".txtg") : (Bfres->mTexDir + "/" + Model.Materials.GetByIndex(Shape.MaterialIndex).Value.Textures.front() + ".txtg")))
				{
					mIndexBuffers.resize(mIndexBuffers.size() - 1);
					mShapeVAOs.resize(mShapeVAOs.size() - 1);
					mShapeBuffers.resize(mShapeBuffers.size() - 1);
					mMaterials.resize(mMaterials.size() - 1);
					continue;
				}
				AlbedoCount = 1;
				Textures.push_back(TextureToGoLibrary::GetTexture(Model.Materials.GetByIndex(Shape.MaterialIndex).Value.Textures.front() + ".txtg", Bfres->mTexDir));
			}

			//Tries to find first not transparent texture, if it even exists
			if (AlbedoCount > 1)
			{
				for (uint32_t i = 0; i < AlbedoCount; i++)
				{
					if (Textures[i]->GetPolishedFormat() == TextureFormat::Format::BC1_UNORM)
					{
						AlbedoCount = i + 1;
						break;
					}
				}
			}

			if (AlbedoCount > 0)
			{
				mMaterials[ShapeIndex].mAlbedoTexture = GLTextureLibrary::GetTexture(&(Textures[AlbedoCount - 1]->GetSurface(mMaterials[ShapeIndex].mTextureArrayIndex)), GL_TEXTURE0, false, TextureFilter);
			}
			else
			{
				LoadFallbackTexture(mMaterials[ShapeIndex]);
			}

			mMaterials[ShapeIndex].mIndexFormat = Shape.Meshes[0].IndexFormat == BfresFile::BfresIndexFormat::UInt32 ? GL_UNSIGNED_INT : (Shape.Meshes[0].IndexFormat == BfresFile::BfresIndexFormat::UInt16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE);

			mShapeBuffers[ShapeIndex][0].GenBuffer(GL_ARRAY_BUFFER);
			mShapeBuffers[ShapeIndex][0].SetData<glm::vec4>(Shape.Vertices);

			uint8_t ShapeBufferIndex = 1;
			for (size_t i = 1; i < Shape.Buffer.Buffers.size(); i++)
			{
				mShapeBuffers[ShapeIndex][ShapeBufferIndex].GenBuffer(GL_ARRAY_BUFFER);
				mShapeBuffers[ShapeIndex][ShapeBufferIndex].SetData<unsigned char>(Shape.Buffer.Buffers[i].Data);
				ShapeBufferIndex++;
			}

			mShapeVAOs[ShapeIndex] = GLBfresVAO(mShapeBuffers[ShapeIndex], mIndexBuffers[ShapeIndex].first);

			mShapeVAOs[ShapeIndex].Buffers[ShapeBufferIndex] = mInstanceMatrix;

			for (auto& [AttributeKey, AttributeVal] : Shape.Buffer.Attributes.Nodes)
			{
				if (!mAttributeLocations.contains(AttributeVal.Value.Name))
					continue;

				if (AttributeVal.Value.Name != "_p0")
				{
					GLenum Format = mFormatList[AttributeVal.Value.Format].m_Type;
					int32_t Count = mFormatList[AttributeVal.Value.Format].m_Count;
					int32_t Stride = Shape.Buffer.Buffers[AttributeVal.Value.BufferIndex].Stride;
					bool Normalized = mFormatList[AttributeVal.Value.Format].m_Normalized;

					mShapeVAOs[ShapeIndex].AddAttribute(mAttributeLocations[AttributeVal.Value.Name], Count, Format, Normalized, Stride, AttributeVal.Value.Offset, AttributeVal.Value.BufferIndex);
				}
				else
				{
					mShapeVAOs[ShapeIndex].AddAttribute(mAttributeLocations[AttributeVal.Value.Name], 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 0, 0);
				}
			}
			
			mShapeVAOs[ShapeIndex].AddAttribute(mAttributeLocations["_instanceMatrix"], 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), 0, Shape.Buffer.Buffers.size(), 1);
			mShapeVAOs[ShapeIndex].AddAttribute(mAttributeLocations["_instanceMatrix"] + 1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), 1 * sizeof(glm::vec4), Shape.Buffer.Buffers.size(), 1);
			mShapeVAOs[ShapeIndex].AddAttribute(mAttributeLocations["_instanceMatrix"] + 2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), 2 * sizeof(glm::vec4), Shape.Buffer.Buffers.size(), 1);
			mShapeVAOs[ShapeIndex].AddAttribute(mAttributeLocations["_instanceMatrix"] + 3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), 3 * sizeof(glm::vec4), Shape.Buffer.Buffers.size(), 1);

			/*
			mShapeVAOs[ShapeIndex].AddAttribute(mAttributeLocations["_boneTransformationMatrix"], 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), 0, Shape.Buffer.Buffers.size() + 1, 0);
			mShapeVAOs[ShapeIndex].AddAttribute(mAttributeLocations["_boneTransformationMatrix"] + 1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), 1 * sizeof(glm::vec4), Shape.Buffer.Buffers.size() + 1, 0);
			mShapeVAOs[ShapeIndex].AddAttribute(mAttributeLocations["_boneTransformationMatrix"] + 2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), 2 * sizeof(glm::vec4), Shape.Buffer.Buffers.size() + 1, 0);
			mShapeVAOs[ShapeIndex].AddAttribute(mAttributeLocations["_boneTransformationMatrix"] + 3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), 3 * sizeof(glm::vec4), Shape.Buffer.Buffers.size() + 1, 0);
			*/

			(mMaterials[ShapeIndex].mRenderStateMode == RenderStateMode::OPAQUE ? mOpaqueObjects : mTransparentObjects).push_back(ShapeIndex);

			ShapeIndex++;
		}
	}
}

void GLBfres::Draw(std::vector<glm::mat4>& ModelMatrices, Shader* Shader)
{
	mInstanceMatrix.SetData<glm::mat4>(ModelMatrices);

	for (uint16_t i : mOpaqueObjects)
	{
		mShapeVAOs[i].Enable(Shader);
		mShapeVAOs[i].Use();

		mMaterials[i].mAlbedoTexture->Bind();

		if (mMaterials[i].mRenderStateDisplayFace != GL_NONE)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(mMaterials[i].mRenderStateDisplayFace);
		}
		else
		{
			glDisable(GL_CULL_FACE);
		}

		glDrawElementsInstanced(GL_TRIANGLES, mIndexBuffers[i].second, mMaterials[i].mIndexFormat, 0, ModelMatrices.size());
	}

	for (uint16_t i : mTransparentObjects)
	{
		mShapeVAOs[i].Enable(Shader);
		mShapeVAOs[i].Use();

		mMaterials[i].mAlbedoTexture->Bind();

		if (mMaterials[i].mRenderStateDisplayFace != GL_NONE)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(mMaterials[i].mRenderStateDisplayFace);
		}
		else
		{
			glDisable(GL_CULL_FACE);
		}

		glDrawElementsInstanced(GL_TRIANGLES, mIndexBuffers[i].second, mMaterials[i].mIndexFormat, 0, ModelMatrices.size());
	}
}

void GLBfres::Delete()
{
	mInstanceMatrix.Dispose();
	for (GLBfresVAO& VAO : mShapeVAOs)
	{
		VAO.Dispose();
	}
	for (std::vector<BufferObject>& BufferVec : mShapeBuffers)
	{
		for (BufferObject& Buffer : BufferVec)
		{
			Buffer.Dispose();
		}
	}
	for (auto& [Buffer, Value] : mIndexBuffers)
	{
		Buffer.Dispose();
	}
}


std::unordered_map<TextureToGo::Surface*, Texture> GLTextureLibrary::mTextures;

bool GLTextureLibrary::IsTextureLoaded(TextureToGo::Surface* TexToGo)
{
	return mTextures.contains(TexToGo);
}

Texture* GLTextureLibrary::GetTexture(TextureToGo::Surface* TexToGo, GLenum Slot, bool GenMipMaps, GLenum TextureFilter)
{
	if (!IsTextureLoaded(TexToGo))
		mTextures.insert({ TexToGo, Texture(TexToGo, Slot, GenMipMaps, TextureFilter) });
	return &mTextures[TexToGo];
}

std::unordered_map<TextureToGo::Surface*, Texture>& GLTextureLibrary::GetTextures()
{
	return mTextures;
}

void GLTextureLibrary::Cleanup()
{
	for (auto& [Key, Tex] : mTextures)
	{
		Tex.Delete();
	}
}


std::unordered_map<BfresFile*, GLBfres> GLBfresLibrary::mModels;

bool GLBfresLibrary::IsModelLoaded(BfresFile* Model)
{
	return mModels.contains(Model);
}

GLBfres* GLBfresLibrary::GetModel(BfresFile* Model, GLenum TextureFilter)
{
	if (!IsModelLoaded(Model))
		mModels.insert({ Model, GLBfres(Model, TextureFilter) });
	return &mModels[Model];
}

void GLBfresLibrary::Cleanup()
{
	for (auto& [Bfres, GLBfres] : mModels)
	{
		GLBfres.Delete();
	}
}