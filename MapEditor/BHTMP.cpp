#include "BHTMP.h"

#include "Logger.h"
#include <fstream>
#include "BinaryVectorReader.h"
#include <stb/stb_image_write.h>
#include <stb/stb_image.h>
#include "Editor.h"
#include "ZStdFile.h"
#include "ActorMgr.h"

/*
#include <vector>
#include <string>
#include <unordered_map>

class BHTMPFile
{
public:

	std::vector<float> Tiles;

	BHTMPFile() {}
	BHTMPFile(std::string Path);
	BHTMPFile(std::vector<unsigned char> Bytes);
};
*/

BHTMPFile::BHTMPFile(std::vector<unsigned char> Bytes)
{
	BinaryVectorReader Reader(Bytes);

	this->Height = Reader.ReadUInt32();
	this->Width = Reader.ReadUInt32();
	Logger::Info("BHTMPDecoder", "Width: " + std::to_string(Width) + ", Height: " + std::to_string(Height) + ", Memory:" + std::to_string(Width * Height * sizeof(float)) + " Bytes");

	this->Tiles.resize(Width * Height);

	Reader.Seek(0x10, BinaryVectorReader::Position::Begin); //0x10 = Data begin

	float BiggestTile = 0;

	for (int i = 0; i < Width * Height; i++) //Go through every tile
	{
		this->Tiles[i] = Reader.ReadFloat();
		Reader.Seek(4, BinaryVectorReader::Position::Current); //Has Data? Unused in editor

		BiggestTile = std::fmax(BiggestTile, this->Tiles[i]);
	}
	
	/*
	std::vector<uint8_t> PixelData(this->Tiles.size() * 4);

	for (int i = 0; i < this->Tiles.size(); i++)
	{
		PixelData[i * 4] = std::max(0, (int)((this->Tiles[i] / BiggestTile) * 255));
		PixelData[i * 4 + 1] = PixelData[i * 4];
		PixelData[i * 4 + 2] = PixelData[i * 4];
		PixelData[i * 4 + 3] = PixelData[i * 4];
	}

	int Length = 0;
	unsigned char* PNG = stbi_write_png_to_mem_forward(PixelData.data(), Width * 4, Width, Height, 4, &Length);

	std::ofstream OutputFile(Editor::GetWorkingDirFile("Test.png"), std::ios::binary);
	OutputFile.write((char*)PNG, Length);
	OutputFile.close();

	free(PNG);
	*/

    BfresModel.IsDefaultModel() = true;

    std::vector<float> Vertices(Width * Height * 3);

    for (int i = 0; i < Width * Height; i++)
    {
        Vertices[i * 3] = i % Width;
        Vertices[i * 3 + 1] = this->Tiles[i];
        Vertices[i * 3 + 2] = i / Height;
    }

    std::vector<unsigned int> Indices;

    for (int row = 0; row < Width - 1; row++) {
        for (int col = 0; col < Height - 1; col++) {
            short TopLeftIndexNum = (short)(row * Width + col);
            short TopRightIndexNum = (short)(row * Width + col + 1);
            short BottomLeftIndexNum = (short)((row + 1) * Width + col);
            short BottomRightIndexNum = (short)((row + 1) * Width + col + 1);

            // write out two triangles
            Indices.push_back(TopLeftIndexNum);
            Indices.push_back(BottomLeftIndexNum);
            Indices.push_back(TopRightIndexNum);

            Indices.push_back(TopRightIndexNum);
            Indices.push_back(BottomLeftIndexNum);
            Indices.push_back(BottomRightIndexNum);
        }
    }

    std::vector<float> TexCoords(130 * 130 * 2);
    for (int i = 0; i < TexCoords.size() / 2; i++)
    {
        if (i > 0)
        {
            TexCoords[i * 2] = TexCoords[i * 2 - 1] + 0.007f;
            TexCoords[i * 2 + 1] = TexCoords[i * 2] + 0.007f;
            continue;
        }
        TexCoords[i * 2] = 0.0f;
        TexCoords[i * 2 + 1] = 0.007f;
    }

    BfresFile::Model Model;
    BfresFile::LOD LODModel;
    LODModel.Faces.push_back(Indices);
    Model.Vertices.push_back(Vertices);

    BfresFile::Material Material;
    Material.Name = "Mt_DefaultModel";

    BfresFile::BfresTexture Texture;
    Texture.TexCoordinates = TexCoords;

    TextureToGo TexToGo;
    TexToGo.GetPixels().resize(5 * 5 * 4);
    for (int i = 0; i < 5 * 5 / 2; i++)
    {
        //Red-black texture indicating that the texture format is unsupported
        TexToGo.GetPixels()[i * 8] = 255;
        TexToGo.GetPixels()[i * 8 + 1] = 0;
        TexToGo.GetPixels()[i * 8 + 2] = 0;
        TexToGo.GetPixels()[i * 8 + 3] = 255;

        TexToGo.GetPixels()[i * 8 + 4] = 0;
        TexToGo.GetPixels()[i * 8 + 5] = 0;
        TexToGo.GetPixels()[i * 8 + 6] = 0;
        TexToGo.GetPixels()[i * 8 + 7] = 255;
    }
    TexToGo.IsFail() = false;
    TexToGo.GetWidth() = 5;
    TexToGo.GetHeight() = 5;
    if (!TextureToGoLibrary::IsTextureLoaded("TerrainTexture"))
        TextureToGoLibrary::Textures.insert({ "TerrainTexture", TexToGo });

    Texture.Texture = TextureToGoLibrary::GetTexture("TerrainTexture");

    Material.Textures.push_back(Texture);
    Model.Materials.push_back(Material);
    Model.LODs.push_back(LODModel);
    BfresModel.GetModels().push_back(Model);

    BfresModel.CreateOpenGLObjects();

    BfresLibrary::Models["MapEditor_Terrain"] = BfresModel;

    Actor& TerrainActor = ActorMgr::AddActor("MapEditor_Terrain", true, false);
    TerrainActor.Model = &BfresLibrary::Models["MapEditor_Terrain"];
    ActorMgr::UpdateModelOrder();
}

BHTMPFile::BHTMPFile(std::string Path)
{
	std::ifstream File(Path, std::ios::binary);

	if (!File.eof() && !File.fail())
	{
		File.seekg(0, std::ios_base::end);
		std::streampos FileSize = File.tellg();

		std::vector<unsigned char> Bytes(FileSize);

		File.seekg(0, std::ios_base::beg);
		File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);

		this->BHTMPFile::BHTMPFile(Bytes);

		File.close();
	}
	else
	{
		Logger::Error("BHTMPDecoder", "Could not open file \"" + Path + "\"");
	}
}

void BHTMPFile::Draw(Shader* Shader)
{
    /*
    glm::mat4 GLMModel = glm::mat4(1.0f);  // Identity matrix
    GLMModel = glm::scale(GLMModel, glm::vec3(8.0f, 1.0f, 8.0f));
    for (uint32_t SubModelIndexOpaque : BfresModel.GetModels()[0].LODs[0].OpaqueObjects)
    {
        BfresModel.GetModels()[0].LODs[0].GL_Meshes[SubModelIndexOpaque].UpdateInstances(1);
        glUniformMatrix4fv(glGetUniformLocation(Shader->ID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(GLMModel));
        BfresModel.GetModels()[0].LODs[0].GL_Meshes[SubModelIndexOpaque].Draw();
    }
    */
}