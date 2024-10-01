#define ZSTD_STATIC_LINKING_ONLY

#include "Bfres.h"

#include "Editor.h"
#include "Logger.h"
#include "Util.h"
#include <zstd.h>
#include <glm/gtx/quaternion.hpp>

/* BfresModelLibrary - Start */
std::map<std::string, BfresFile> BfresLibrary::Models;

void BfresLibrary::Initialize()
{
    //BfresLibrary::Models.insert({ "MapEditor_Collision_", BfresFile::CreateDefaultModel("MapEditor_Collision_", 255, 255, 0, 50) });
}

bool BfresLibrary::IsModelLoaded(std::string ModelName)
{
    return BfresLibrary::Models.count(ModelName);
}
BfresFile* BfresLibrary::GetModel(std::string ModelName)
{
    if (!BfresLibrary::IsModelLoaded(ModelName))
        BfresLibrary::Models.insert({ ModelName, BfresFile(Editor::GetBfresFile(ModelName)) });
    return &BfresLibrary::Models[ModelName];
}
void BfresLibrary::Cleanup()
{
    Logger::Info("BfresLibrary", "Deleted models");
}
/* BfresModelLibrary - End */


std::unordered_map<uint64_t, std::string> BfresFile::ExternalBinaryStrings;

void BfresFile::Initialize()
{
    std::ifstream File(Editor::GetRomFSFile("Shader/ExternalBinaryString.bfres.mc"), std::ios::binary);

    if (!File.eof() && !File.fail())
    {
        File.seekg(0, std::ios_base::end);
        std::streampos FileSize = File.tellg();

        std::vector<unsigned char> Bytes(FileSize);

        File.seekg(0, std::ios_base::beg);
        File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);

        File.close();

        BfresBinaryVectorReader Reader(Bytes);
        Reader.ReadUInt32(); //Magic
        Reader.ReadUInt32(); //Version
        int32_t Flags = Reader.ReadInt32();
        uint32_t DecompressedSize = (Flags >> 5) << (Flags & 0xF);
        Reader.Seek(0xC, BfresBinaryVectorReader::Position::Begin);
        std::vector<unsigned char> Source(Reader.GetSize() - 0xC);
        Reader.ReadStruct(Source.data(), Reader.GetSize() - 0xC);

        std::vector<unsigned char> Decompressed(DecompressedSize);

        ZSTD_DCtx* const DCtx = ZSTD_createDCtx();
        ZSTD_DCtx_setFormat(DCtx, ZSTD_format_e::ZSTD_f_zstd1_magicless);
        const size_t DecompSize = ZSTD_decompressDCtx(DCtx, (void*)Decompressed.data(), Decompressed.size(), Source.data(), Source.size());
        ZSTD_freeDCtx(DCtx);

        BfresBinaryVectorReader DecompressedReader(Decompressed);
        DecompressedReader.Seek(0xC0, BfresBinaryVectorReader::Position::Begin);
        uint64_t StringTableOffset = DecompressedReader.ReadUInt64();
        BfresBinaryVectorReader::ResDict<BfresBinaryVectorReader::ResString> StringTable = DecompressedReader.ReadDictionary<BfresBinaryVectorReader::ResString>(StringTableOffset);
        DecompressedReader.Seek(0xF0, BfresBinaryVectorReader::Position::Begin); //Hash table?

        ExternalBinaryStrings.clear();

        for (int i = 0; i < StringTable.Nodes.size(); i++)
        {
            std::string Key = StringTable.GetKey(i);
            uint64_t Offset = DecompressedReader.ReadUInt64();
            ExternalBinaryStrings.insert({ Offset, Key });
        }

    }
    else
    {
        Logger::Error("Bfres", "Could not open external strings file");
    }
}

void BfresFile::VertexBuffer::VertexAttribute::Read(BfresBinaryVectorReader& Reader)
{
    Name = Reader.ReadStringOffset(Reader.ReadUInt64());
    Format = (BfresAttribFormat)Reader.ReadUInt16(true);
    Reader.ReadUInt16(); //Padding?
    Offset = Reader.ReadUInt16();
    BufferIndex = Reader.ReadUInt16();
}

std::vector<glm::vec4> BfresFile::VertexBuffer::VertexAttribute::GetData(BfresFile::VertexBuffer Buffer)
{
    std::vector<glm::vec4> Data(Buffer.VertexCount);

    BfresFile::VertexBuffer::BufferData BData = Buffer.Buffers[BufferIndex];
    BfresBinaryVectorReader Reader(BData.Data);
    for (int i = 0; i < Buffer.VertexCount; i++)
    {
        Reader.Seek(i * BData.Stride + Offset, BfresBinaryVectorReader::Position::Begin);
        Data[i] = Reader.ReadAttribute(Format);
    }

    return Data;
}

void BfresFile::VertexBuffer::VertexBufferInfo::Read(BfresBinaryVectorReader& Reader)
{
    Size = Reader.ReadUInt32();
    Reader.Seek(12, BfresBinaryVectorReader::Position::Current);
}

void BfresFile::VertexBuffer::VertexStrideInfo::Read(BfresBinaryVectorReader& Reader)
{
    Stride = Reader.ReadUInt32();
    Reader.Seek(12, BfresBinaryVectorReader::Position::Current);
}

void BfresFile::VertexBuffer::Read(BfresBinaryVectorReader& Reader)
{
    Reader.ReadStruct(&Header, sizeof(BfresFile::VertexBufferHeader));
    VertexCount = Header.VertexCount;

    uint32_t Pos = Reader.GetPosition();
    Attributes = Reader.ReadDictionary<VertexAttribute>(Header.AttributeArrayDictionary, Header.AttributeArrayOffset);
    BufferInfo = Reader.ReadArray<VertexBufferInfo>(Header.VertexBufferInfoArray, Header.VertexBufferCount);
    BufferStrides = Reader.ReadArray<VertexStrideInfo>(Header.VertexBufferStrideArray, Header.VertexBufferCount);

    Reader.Seek(Pos, BfresBinaryVectorReader::Position::Begin);
}

void BfresFile::VertexBuffer::InitBuffers(BfresBinaryVectorReader& Reader, BufferMemoryPool PoolInfo)
{
    Reader.Seek(PoolInfo.Offset + Header.BufferOffset, BfresBinaryVectorReader::Position::Begin);
    Buffers.resize(Header.VertexBufferCount);
    for (int Buff = 0; Buff < Header.VertexBufferCount; Buff++)
    {
        Reader.Align(8);

        BufferData Buffer;
        Buffer.Data.resize(BufferInfo[Buff].Size);
        Reader.ReadStruct(Buffer.Data.data(), BufferInfo[Buff].Size);
        Buffer.Stride = BufferStrides[Buff].Stride;
        Buffers[Buff] = Buffer;
    }
}

std::vector<glm::vec4> BfresFile::VertexBuffer::TryGetAttributeData(std::string Name)
{
    if (Attributes.Nodes.count(Name))
        return Attributes.Nodes[Name].Value.GetData(*this);

    return std::vector<glm::vec4>();
}

std::vector<glm::vec4> BfresFile::VertexBuffer::GetPositions()
{
    return TryGetAttributeData("_p0");
}
std::vector<glm::vec4> BfresFile::VertexBuffer::GetNormals()
{
    return TryGetAttributeData("_n0");
}
std::vector<glm::vec4> BfresFile::VertexBuffer::GetTexCoords(int Channel)
{
    return TryGetAttributeData("_u" + std::to_string(Channel));
}
std::vector<glm::vec4> BfresFile::VertexBuffer::GetColors(int Channel)
{
    return TryGetAttributeData("_c" + std::to_string(Channel));
}
std::vector<glm::vec4> BfresFile::VertexBuffer::GetBoneWeights(int Channel)
{
    return TryGetAttributeData("_w" + std::to_string(Channel));
}
std::vector<glm::vec4> BfresFile::VertexBuffer::GetBoneIndices(int Channel)
{
    return TryGetAttributeData("_i" + std::to_string(Channel));
}
std::vector<glm::vec4> BfresFile::VertexBuffer::GetTangents()
{
    return TryGetAttributeData("_t0");
}
std::vector<glm::vec4> BfresFile::VertexBuffer::GetBitangent()
{
    return TryGetAttributeData("_b0");
}

void BfresFile::Material::Sampler::Read(BfresBinaryVectorReader& Reader)
{
    WrapModeU = (BfresFile::TexWrap)Reader.ReadUInt8();
    WrapModeV = (BfresFile::TexWrap)Reader.ReadUInt8();
    WrapModeW = (BfresFile::TexWrap)Reader.ReadUInt8();
    CompareFunc = (BfresFile::CompareFunction)Reader.ReadUInt8();
    BorderColorType = (BfresFile::TexBorderType)Reader.ReadUInt8();
    Anisotropic = (BfresFile::MaxAnisotropic)Reader.ReadUInt8();
    FilterFlags = Reader.ReadUInt16();
    MinLOD = Reader.ReadFloat();
    MaxLOD = Reader.ReadFloat();
    LODBias = Reader.ReadFloat();
    Reader.Seek(12, BfresBinaryVectorReader::Position::Current);

    Mipmap = (MipFilterModes)(FilterFlags & FlagsMipmapMask);
    MagFilter = (ExpandFilterModes)(FilterFlags & FlagsExpandMask);
    MinFilter = (ShrinkFilterModes)(FilterFlags & FlagsShrinkMask);
}

void BfresFile::Material::ShaderParam::Read(BfresBinaryVectorReader& Reader)
{
    Reader.ReadUInt64(); //Padding
    Name = Reader.ReadStringOffset(Reader.ReadUInt64());
    DataOffset = Reader.ReadUInt16();
    Type = (BfresFile::ShaderParamType)Reader.ReadUInt16();
    Reader.ReadUInt32(); //Padding
}

void BfresFile::Material::ShaderParam::ReadShaderParamData(BfresBinaryVectorReader& Reader)
{
    Reader.Seek(DataOffset, BfresBinaryVectorReader::Position::Begin);
    this->DataValue = ReadParamData(this->Type, Reader);
}

//TODO: Find a better way
std::variant<bool, float, int32_t, uint32_t, uint8_t, std::vector<bool>, std::vector<float>, std::vector<int32_t>, std::vector<uint32_t>, std::vector<uint8_t>, BfresFile::Material::ShaderParam::Srt2D, BfresFile::Material::ShaderParam::Srt3D, BfresFile::Material::ShaderParam::TexSrt> BfresFile::Material::ShaderParam::ReadParamData(BfresFile::ShaderParamType Type, BfresBinaryVectorReader& Reader)
{
    switch (Type)
    {
    case BfresFile::Bool:
        return (bool)Reader.ReadUInt8();
    case BfresFile::Bool2:
        return Reader.ReadRawBoolArray(2);
    case BfresFile::Bool3:
        return Reader.ReadRawBoolArray(3);
    case BfresFile::Bool4:
        return Reader.ReadRawBoolArray(4);
    case BfresFile::Int:
        return (int32_t)Reader.ReadInt32();
    case BfresFile::Int2:
        return Reader.ReadRawArray<int32_t>(2);
    case BfresFile::Int3:
        return Reader.ReadRawArray<int32_t>(3);
    case BfresFile::Int4:
        return Reader.ReadRawArray<int32_t>(4);
    case BfresFile::UInt:
        return (uint32_t)Reader.ReadUInt32();
    case BfresFile::UInt2:
        return Reader.ReadRawArray<uint32_t>(2);
    case BfresFile::UInt3:
        return Reader.ReadRawArray<uint32_t>(2);
    case BfresFile::UInt4:
        return Reader.ReadRawArray<uint32_t>(2);
    case BfresFile::Float:
        return (float)Reader.ReadFloat();
    case BfresFile::Float2:
        return Reader.ReadRawArray<float>(2);
    case BfresFile::Float3:
        return Reader.ReadRawArray<float>(3);
    case BfresFile::Float4:
        return Reader.ReadRawArray<float>(4);
    case BfresFile::Reserved2:
        return Reader.ReadRawArray<uint8_t>(2);
    case BfresFile::Float2x2:
        return Reader.ReadRawArray<float>(2 * 2);
    case BfresFile::Float2x3:
        return Reader.ReadRawArray<float>(2 * 3);
    case BfresFile::Float2x4:
        return Reader.ReadRawArray<float>(2 * 4);
    case BfresFile::Reserved3:
        return Reader.ReadRawArray<uint8_t>(3);
    case BfresFile::Float3x2:
        return Reader.ReadRawArray<float>(3 * 2);
    case BfresFile::Float3x3:
        return Reader.ReadRawArray<float>(3 * 3);
    case BfresFile::Float3x4:
        return Reader.ReadRawArray<float>(3 * 4);
    case BfresFile::Reserved4:
        return Reader.ReadRawArray<uint8_t>(4);
    case BfresFile::Float4x2:
        return Reader.ReadRawArray<float>(4 * 2);
    case BfresFile::Float4x3:
        return Reader.ReadRawArray<float>(4 * 3);
    case BfresFile::Float4x4:
        return std::vector<float>(4 * 4);
        break;
    case BfresFile::Srt2D:
    {
        Srt2D Value2d;
        Value2d.Scaling = glm::vec2(Reader.ReadFloat(), Reader.ReadFloat());
        Value2d.Rotation = Reader.ReadFloat();
        Value2d.Translation = glm::vec2(Reader.ReadFloat(), Reader.ReadFloat());
        return Value2d;
    }
    case BfresFile::Srt3D:
    {
        Srt3D Value3d;
        Value3d.Scaling = glm::vec3(Reader.ReadFloat(), Reader.ReadFloat(), Reader.ReadFloat());
        Value3d.Rotation = glm::vec3(Reader.ReadFloat(), Reader.ReadFloat(), Reader.ReadFloat());
        Value3d.Translation = glm::vec3(Reader.ReadFloat(), Reader.ReadFloat(), Reader.ReadFloat());
        return Value3d;
    }
    case BfresFile::TexSrt:
    case BfresFile::TexSrtEx:
    {
        TexSrt ValueTex;
        ValueTex.Mode = (TexSrt::TexSrtMode)Reader.ReadInt32();
        ValueTex.Scaling = glm::vec2(Reader.ReadFloat(), Reader.ReadFloat());
        ValueTex.Rotation = Reader.ReadFloat();
        ValueTex.Translation = glm::vec2(Reader.ReadFloat(), Reader.ReadFloat());
        return ValueTex;
    }
    }
}

std::vector<float> BfresFile::Material::RenderInfo::GetValueSingles()
{
    return *(std::vector<float>*) & _Value;
}

std::vector<int32_t> BfresFile::Material::RenderInfo::GetValueInt32s()
{
    return *(std::vector<int32_t>*) & _Value;
}

std::vector<std::string> BfresFile::Material::RenderInfo::GetValueStrings()
{
    return *(std::vector<std::string>*) & _Value;
}

void BfresFile::Material::RenderInfo::ReadData(BfresBinaryVectorReader& Reader, uint32_t Count)
{
    switch (Type)
    {
    case BfresFile::Material::RenderInfo::Int32:
        _Value = Reader.ReadRawArray<int32_t>(Count);
        break;
    case BfresFile::Material::RenderInfo::Single:
        _Value = Reader.ReadRawArray<float>(Count);
        break;
    case BfresFile::Material::RenderInfo::String:
        _Value = Reader.ReadStringOffsets(Count);
        break;
    default:
        break;
    }
}

void BfresFile::Material::RenderInfo::Read(BfresBinaryVectorReader& Reader)
{
}

void BfresFile::Material::ReadShaderOptions(BfresBinaryVectorReader& Reader)
{
    uint32_t NumBoolChoices = HeaderShaderInfo.NumOptionBooleans;
    std::vector<bool> _OptionBooleans = Reader.ReadCustom<std::vector<bool>>([&Reader, &NumBoolChoices]() {
        return Reader.ReadBooleanBits(NumBoolChoices);
        }, HeaderShaderInfo.OptionBoolChoiceOffset);

    uint32_t NumChoiceValues = HeaderShaderInfo.NumOptions - HeaderShaderInfo.NumOptionBooleans;
    std::vector<std::string> _OptionStrings = Reader.ReadCustom<std::vector<std::string>>([&Reader, &NumChoiceValues]()
        {
            return Reader.ReadStringOffsets(NumChoiceValues);
        }, HeaderShaderInfo.OptionStringChoiceOffset);

    BfresBinaryVectorReader::ResDict<BfresBinaryVectorReader::ResString> _OptionKeys = Reader.ReadDictionary<BfresBinaryVectorReader::ResString>(HeaderShaderAssign.OptionsDictOffset);

    uint32_t NumOptionIndices = HeaderShaderInfo.NumOptions;
    std::vector<int16_t> _OptionIndices = Reader.ReadCustom<std::vector<int16_t>>([&Reader, &NumOptionIndices]()
        {
            return Reader.ReadRawArray<int16_t>(NumOptionIndices);
        }, HeaderShaderInfo.OptionIndicesOffset);

    std::vector<std::string> Choices;
    for (int i = 0; i < _OptionBooleans.size(); i++)
    {
        Choices.push_back(_OptionBooleans[i] ? "True" : "False");
    }
    if (!_OptionStrings.empty())
    {
        Choices.insert(Choices.end(), _OptionStrings.begin(), _OptionStrings.end());
    }

    int ChoiceIndex = 0;
    for (int i = 0; i < _OptionKeys.Nodes.size(); i++)
    {
        if (std::find(_OptionIndices.begin(), _OptionIndices.end(), i) == _OptionIndices.end())
            continue;

        BfresBinaryVectorReader::ResString Value;
        Value.String = Choices[ChoiceIndex];
        std::string Key = _OptionKeys.GetKey(i);
        ChoiceIndex++;
        ShaderAssign.ShaderOptions.Add(Key, Value);

        //std::cout << "Shader Assign Option " << Key << ": " << Value.String << std::endl;
    }
}

void BfresFile::Material::ReadRenderInfo(BfresBinaryVectorReader& Reader)
{
    BfresBinaryVectorReader::ResDict<BfresBinaryVectorReader::ResString> Dict = Reader.ReadDictionary<BfresBinaryVectorReader::ResString>(HeaderShaderAssign.RenderInfoDictOffset);
    if (Dict.Nodes.empty()) return;

    for (int i = 0; i < Dict.Nodes.size(); i++)
    {
        RenderInfo Info;

        Reader.Seek(HeaderShaderAssign.RenderInfoOffset + i * 16, BfresBinaryVectorReader::Position::Begin);
        Info.Name = Reader.ReadStringOffset(Reader.ReadUInt64());
        Info.Type = (RenderInfo::RenderInfoType)Reader.ReadUInt8();

        Reader.Seek(Header.RenderInfoNumOffset + i * 2, BfresBinaryVectorReader::Position::Begin);
        uint16_t Count = Reader.ReadUInt16();

        Reader.Seek(Header.RenderInfoDataOffsetTable + i * 2, BfresBinaryVectorReader::Position::Begin);
        uint16_t DataOffset = Reader.ReadUInt16();

        Reader.Seek(Header.RenderInfoBufferOffset + DataOffset, BfresBinaryVectorReader::Position::Begin);
        Info.ReadData(Reader, Count);

        //std::cout << "Render info " << Info.Name << std::endl;

        RenderInfos.Add(Info.Name, Info);
    }
}

void BfresFile::Material::ReadShaderParameters(BfresBinaryVectorReader& Reader)
{
    BfresBinaryVectorReader::ResDict<BfresBinaryVectorReader::ResString> Dict = Reader.ReadDictionary<BfresBinaryVectorReader::ResString>(HeaderShaderAssign.ShaderParamDictOffset);
    if (Dict.Nodes.empty()) return;

    Reader.Seek(HeaderShaderAssign.ShaderParamOffset, BfresBinaryVectorReader::Position::Begin);
    for (int i = 0; i < Dict.Nodes.size(); i++)
    {
        ShaderParam Param;
        Param.Read(Reader);
        ShaderParams.Add(Param.Name, Param);
    }

    uint32_t ShaderParamSize = HeaderShaderAssign.ShaderParamSize;
    std::vector<uint8_t> ParamData = Reader.ReadCustom<std::vector<uint8_t>>([&Reader, &ShaderParamSize]()
        {
            return Reader.ReadRawArray<uint8_t>(ShaderParamSize);
        }, Header.ParamDataOffset);

    BfresBinaryVectorReader DataReader(ParamData);
    for (auto& [Key, Val] : ShaderParams.Nodes)
    {
        Val.Value.ReadShaderParamData(DataReader);
    }
}

BfresBinaryVectorReader::ResDict<BfresBinaryVectorReader::ResString> BfresFile::Material::ReadAssign(BfresBinaryVectorReader& Reader, uint64_t StringListOffset, uint64_t StringDictOffset, uint64_t IndicesOffset, int32_t NumValues)
{
    BfresBinaryVectorReader::ResDict<BfresBinaryVectorReader::ResString> Dict;
    if (NumValues == 0) return Dict;

    std::vector<std::string> _ValueStrings = Reader.ReadCustom<std::vector<std::string>>([&Reader, &NumValues]()
        {
            return Reader.ReadStringOffsets(NumValues);
        }, StringListOffset);

    std::vector<int16_t> _ValueIndices = Reader.ReadCustom<std::vector<int16_t>>([&Reader, &NumValues]()
        {
            return Reader.ReadRawArray<int16_t>(NumValues);
        }, IndicesOffset);

    BfresBinaryVectorReader::ResDict<BfresBinaryVectorReader::ResString> _OptionKeys = Reader.ReadDictionary<BfresBinaryVectorReader::ResString>(StringDictOffset);

    int ChoiceIndex = 0;
    for (int i = 0; i < _ValueStrings.size(); i++)
    {
        if (std::find(_ValueIndices.begin(), _ValueIndices.end(), i) == _ValueIndices.end())
            continue;

        BfresBinaryVectorReader::ResString Value;
        Value.String = _ValueStrings[ChoiceIndex];
        std::string Key = _OptionKeys.GetKey(i);
        ChoiceIndex++;
        Dict.Add(Key, Value);
    }
    return Dict;
}

void BfresFile::Material::Read(BfresBinaryVectorReader& Reader)
{
    Reader.ReadStruct(&Header, sizeof(MaterialHeader));
    Name = Reader.ReadStringOffset(Header.NameOffset);
    uint32_t Pos = Reader.GetPosition();
    Reader.Seek(Header.TextureNamesOffset, BfresBinaryVectorReader::Position::Begin);
    Textures = Reader.ReadStringOffsets(Header.TextureRefCount);

    Samplers = Reader.ReadDictionary<Sampler>(Header.SamplerDictionaryOffset, Header.SamplerOffset);

    Reader.Seek(Header.ShaderInfoOffset, BfresBinaryVectorReader::Position::Begin);
    Reader.ReadStruct(&HeaderShaderInfo, sizeof(ShaderInfoHeader));

    Reader.Seek(HeaderShaderInfo.ShaderAssignOffset, BfresBinaryVectorReader::Position::Begin);
    Reader.ReadStruct(&HeaderShaderAssign, sizeof(ShaderAssignHeader));

    ReadShaderOptions(Reader);
    ReadRenderInfo(Reader);
    ReadShaderParameters(Reader);

    ShaderAssign.ShaderArchiveName = Reader.ReadStringOffset(HeaderShaderAssign.ShaderArchiveNameOffset);
    ShaderAssign.ShadingModelName = Reader.ReadStringOffset(HeaderShaderAssign.ShaderModelNameOffset);

    ShaderAssign.SamplerAssign = ReadAssign(Reader,
        HeaderShaderInfo.SamplerAssignOffset,
        HeaderShaderAssign.SamplerAssignDictOffset,
        HeaderShaderInfo.SamplerAssignIndicesOffset,
        HeaderShaderInfo.NumSamplerAssign);

    ShaderAssign.AttributeAssign = ReadAssign(Reader,
        HeaderShaderInfo.AttributeAssignOffset,
        HeaderShaderAssign.AttributeAssignDictOffset,
        HeaderShaderInfo.AttributeAssignIndicesOffset,
        HeaderShaderInfo.NumAttributeAssign);

    Reader.Seek(Pos, BfresBinaryVectorReader::Position::Begin);
}

/*
void BfresFile::Material::LoadTextures(BfresFile& File, std::string TexDir)
{
    for (std::string Tex : Textures)
    {
        if (!File.Textures.count(Tex))
        {
            if (!Util::FileExists(TexDir == "" ? Editor::GetRomFSFile("TexToGo/" + Tex + ".txtg") : (TexDir + "/" + Tex + ".txtg")))
                continue;

            TextureToGo* TexToGo = TextureToGoLibrary::GetTexture(Tex + ".txtg", TexDir);

            if (!TexToGo->IsLoaded())
                continue;

            File.Textures.insert({ Tex, TexToGo });
        }
    }
}
*/

std::string BfresFile::Material::GetRenderInfoString(std::string Key)
{
    if (RenderInfos.Nodes.count(Key))
    {
        if (!RenderInfos.Nodes[Key].Value.GetValueStrings().empty())
            return RenderInfos.Nodes[Key].Value.GetValueStrings()[0];
    }
    return "";
}

float BfresFile::Material::GetRenderInfoFloat(std::string Key)
{
    if (RenderInfos.Nodes.count(Key))
    {
        if (!RenderInfos.Nodes[Key].Value.GetValueSingles().empty())
            return RenderInfos.Nodes[Key].Value.GetValueSingles()[0];
    }
    return 1.0f;
}

int32_t BfresFile::Material::GetRenderInfoInt(std::string Key)
{
    if (RenderInfos.Nodes.count(Key))
    {
        if (!RenderInfos.Nodes[Key].Value.GetValueInt32s().empty())
            return RenderInfos.Nodes[Key].Value.GetValueInt32s()[0];
    }
    return 1;
}

void BfresFile::Shape::Mesh::IndexBufferInfo::Read(BfresBinaryVectorReader& Reader)
{
    Size = Reader.ReadUInt32();
    Flag = Reader.ReadUInt32();
    Reader.Seek(40, BfresBinaryVectorReader::Position::Current);
}

void BfresFile::Shape::Mesh::SubMesh::Read(BfresBinaryVectorReader& Reader)
{
    Offset = Reader.ReadUInt32();
    Count = Reader.ReadUInt32();
}

void BfresFile::Shape::Mesh::Read(BfresBinaryVectorReader& Reader)
{
    Reader.ReadStruct(&Header, sizeof(BfresFile::MeshHeader));

    uint32_t Pos = Reader.GetPosition();

    Reader.Seek(Header.BufferInfoOffset, BfresBinaryVectorReader::Position::Begin);
    BufferInfo = Reader.ReadResObject<IndexBufferInfo>();
    Reader.Seek(Header.SubMeshArrayOffset, BfresBinaryVectorReader::Position::Begin);
    SubMeshes = Reader.ReadArray<SubMesh>(Header.SubMeshArrayOffset, Header.SubMeshCount);

    IndexCount = Header.IndexCount;
    IndexFormat = Header.IndexFormat;
    PrimitiveType = Header.PrimType;

    Reader.Seek(Pos, BfresBinaryVectorReader::Position::Begin);
}

void BfresFile::Shape::Mesh::InitBuffers(BfresBinaryVectorReader& Reader, BufferMemoryPool Pool)
{
    uint32_t BaseIndexSize = 0;

    switch (Header.IndexFormat)
    {
    case BfresFile::BfresIndexFormat::UnsignedByte:
    {
        BaseIndexSize++;
        break;
    }
    case BfresFile::BfresIndexFormat::UInt16:
    {
        BaseIndexSize += 2;
        break;
    }
    case BfresFile::BfresIndexFormat::UInt32:
    {
        BaseIndexSize += 4;
        break;
    }
    }

    Reader.Seek(Pool.Offset + Header.BufferOffset + Header.BaseIndex * BaseIndexSize, BfresBinaryVectorReader::Position::Begin);
    IndexBuffer.resize(BufferInfo.Size);
    Reader.ReadStruct(IndexBuffer.data(), BufferInfo.Size);
}

std::vector<uint32_t> BfresFile::Shape::Mesh::GetIndices()
{
    BfresBinaryVectorReader Reader(IndexBuffer);
    std::vector<uint32_t> Indices(Header.IndexCount);

    for (int i = 0; i < Indices.size(); i++)
    {
        switch (Header.IndexFormat)
        {
        case BfresFile::BfresIndexFormat::UnsignedByte:
        {
            Indices[i] = Reader.ReadUInt8();
            break;
        }
        case BfresFile::BfresIndexFormat::UInt16:
        {
            Indices[i] = Reader.ReadUInt16();
            break;
        }
        case BfresFile::BfresIndexFormat::UInt32:
        {
            Indices[i] = Reader.ReadUInt32();
            break;
        }
        }
    }
    return Indices;
}

void BfresFile::Shape::ShapeBounding::Read(BfresBinaryVectorReader& Reader)
{
    Center = glm::vec3(Reader.ReadFloat(), Reader.ReadFloat(), Reader.ReadFloat());
    Extent = glm::vec3(Reader.ReadFloat(), Reader.ReadFloat(), Reader.ReadFloat());
}

void BfresFile::Shape::Read(BfresBinaryVectorReader& Reader)
{
    Reader.ReadStruct(&Header, sizeof(BfresFile::ShapeHeader));

    uint32_t Pos = Reader.GetPosition();
    Name = Reader.ReadStringOffset(Header.NameOffset);
    VertexBufferIndex = Header.VertexBufferIndex;
    MaterialIndex = Header.MaterialIndex;
    BoneIndex = Header.BoneIndex;
    SkinCount = Header.MaxSkinInfluence;
    Meshes = Reader.ReadArray<Mesh>(Header.MeshArrayOffset, Header.MeshCount);

    uint32_t NumBounding = 0;
    for (Mesh M : Meshes)
    {
        NumBounding += M.SubMeshes.size() + 1;
    }
    uint32_t NumRadius = Meshes.size();
    BoundingSpheres = Reader.ReadCustom<std::vector<glm::vec4>>([&NumRadius, &Reader]() {

        std::vector<glm::vec4> Values(NumRadius);
        for (int i = 0; i < Values.size(); i++)
        {
            Values[i] = Reader.ReadVector4F();
        }

        return Values;
        }, 0);

    BoundingBoxes = Reader.ReadArray<ShapeBounding>(Header.BoundingBoxOffset, NumBounding);

    Reader.Seek(Pos, BfresBinaryVectorReader::Position::Begin);
}

void BfresFile::Shape::Init(BfresBinaryVectorReader& Reader, BfresFile::VertexBuffer Buffer, BufferMemoryPool Pool, BfresFile::Skeleton& Skeleton)
{
    this->Buffer = Buffer;
    this->Buffer.InitBuffers(Reader, Pool);

    for (Mesh& M : Meshes)
        M.InitBuffers(Reader, Pool);
}

void BfresFile::Skeleton::Bone::Read(BfresBinaryVectorReader& Reader)
{
    Reader.ReadStruct(&Header, sizeof(BoneHeader));
    Name = Reader.ReadStringOffset(Header.NameOffset);

    _Flags = Header.Flags;
    FlagsRotation = (BoneFlagsRotation)(_Flags & _FlagsMaskRotate);
    Position = glm::vec3(Header.PositionX, Header.PositionY, Header.PositionZ);
    Rotate = glm::vec4(Header.RotationX, Header.RotationY, Header.RotationZ, Header.RotationW);
    Scale = glm::vec3(Header.ScaleX, Header.ScaleY, Header.ScaleZ);

    Index = Header.Index;

    SmoothMatrixIndex = Header.SmoothMatrixIndex;
    RigidMatrixIndex = Header.RigidMatrixIndex;
    ParentIndex = Header.ParentIndex;
}

void BfresFile::Skeleton::Bone::CalculateLocalMatrix(BfresBinaryVectorReader::ResDict<Bone>& Bones)
{
    if (CalculatedMatrices)
        return;

    WorldMatrix = glm::mat4(1.0f);

    WorldMatrix = glm::translate(WorldMatrix, Position);
    WorldMatrix = glm::scale(WorldMatrix, Scale);

    if (FlagsRotation == BoneFlagsRotation::EulerXYZ)
    {
        WorldMatrix = glm::rotate(WorldMatrix, Rotate.z, glm::vec3(0.0f, 0.0f, 1.0f));
        WorldMatrix = glm::rotate(WorldMatrix, Rotate.y, glm::vec3(0.0f, 1.0f, 0.0f));
        WorldMatrix = glm::rotate(WorldMatrix, Rotate.x, glm::vec3(1.0f, 0.0f, 0.0f));
    }
    else
    {
        WorldMatrix = glm::toMat4(glm::qua(Rotate[0], Rotate[1], Rotate[2], Rotate[3]));
    }

    if (ParentIndex != -1)
    {
        Bone& Parent = Bones.GetByIndex(ParentIndex).Value;
        Parent.CalculateLocalMatrix(Bones);
        WorldMatrix = Parent.WorldMatrix * WorldMatrix;
    }

    //TODO: INVERSE
    CalculatedMatrices = true;
}

void BfresFile::Skeleton::CalculateMatrices(bool CalculateInverse)
{
    for (auto& [Key, Val] : Bones.Nodes)
    {
        Val.Value.CalculateLocalMatrix(Bones);
    }
}

void BfresFile::Skeleton::Read(BfresBinaryVectorReader& Reader)
{
    Reader.ReadStruct(&Header, sizeof(SkeletonHeader));
    uint32_t Pos = Reader.GetPosition();

    uint32_t NumBoneIndices = Header.NumSmoothMatrices + Header.NumRigidMatrices;

    Bones = Reader.ReadDictionary<Bone>(Header.BoneDictionaryOffset, Header.BoneArrayOffset);
    MatrixToBoneList = Reader.ReadCustom<std::vector<uint16_t>>([&Reader, &NumBoneIndices]()
        {
            return Reader.ReadRawArray<uint16_t>(NumBoneIndices);
        }, Header.MatrixToBoneListOffset);

    CalculateMatrices(true);

    Reader.Seek(Pos, BfresBinaryVectorReader::Position::Begin);
}

void BfresFile::Model::Read(BfresBinaryVectorReader& Reader)
{
    Reader.ReadStruct(&Header, sizeof(BfresFile::ModelHeader));

    uint32_t Pos = Reader.GetPosition();
    Name = Reader.ReadStringOffset(Header.NameOffset);

    VertexBuffers = Reader.ReadArray<VertexBuffer>(Header.VertexArrayOffset, Header.VertexBufferCount);
    Shapes = Reader.ReadDictionary<Shape>(Header.ShapeDictionaryOffset, Header.ShapeArrayOffset);
    Materials = Reader.ReadDictionary<Material>(Header.MaterialDictionaryOffset, Header.MaterialArrayOffset);

    Reader.Seek(Header.SkeletonOffset, BfresBinaryVectorReader::Position::Begin);
    ModelSkeleton = Reader.ReadResObject<Skeleton>();
}

void BfresFile::Model::Init(BfresFile& Bfres, BfresBinaryVectorReader& Reader, BufferMemoryPool PoolInfo, std::string TexDir)
{
    for (auto& [Key, BfresShape] : Shapes.Nodes)
    {
        BfresShape.Value.Init(Reader, VertexBuffers[BfresShape.Value.VertexBufferIndex], PoolInfo, ModelSkeleton);
    }

    /*
    for (auto& [Key, BfresMaterial] : Materials.Nodes)
    {
        BfresMaterial.Value.LoadTextures(Bfres, TexDir);
    }
    */

    for (BfresFile::VertexBuffer& Buffer : VertexBuffers)
    {
        Buffer.InitBuffers(Reader, PoolInfo);
    }

    for (auto& [Key, BfresShape] : Shapes.Nodes)
    {
        BfresShape.Value.Vertices = VertexBuffers[BfresShape.Value.VertexBufferIndex].GetPositions();

        std::vector<glm::vec4> BoneIndices = BfresShape.Value.Buffer.GetBoneIndices();
        if (!BoneIndices.empty() && !BfresShape.Value.Buffer.Attributes.Nodes.contains("_w0"))
        {
            for (size_t i = 0; i < BoneIndices.size(); i++)
            {
                BfresShape.Value.Vertices[i].w = 1.0f;
                BfresShape.Value.Vertices[i] = ModelSkeleton.Bones.GetByIndex(ModelSkeleton.MatrixToBoneList[BoneIndices[i].x]).Value.WorldMatrix * BfresShape.Value.Vertices[i];
                BoundingBoxSphereRadius = std::fmax(BoundingBoxSphereRadius, std::sqrt(std::pow(BfresShape.Value.Vertices[i].x, 2) + std::pow(BfresShape.Value.Vertices[i].y, 2) + std::pow(BfresShape.Value.Vertices[i].z, 2))); //sqrt(PointA^2 + PointB^2 + PointC^2) = distance to middle
            }
        }
        else
        {
            glm::mat4& Matrix = ModelSkeleton.Bones.GetByIndex(0).Value.WorldMatrix;
            for (size_t i = 0; i < BfresShape.Value.Vertices.size(); i++)
            {
                BfresShape.Value.Vertices[i].w = 1.0f;
                BfresShape.Value.Vertices[i] = Matrix * BfresShape.Value.Vertices[i];
                BoundingBoxSphereRadius = std::fmax(BoundingBoxSphereRadius, std::sqrt(std::pow(BfresShape.Value.Vertices[i].x, 2) + std::pow(BfresShape.Value.Vertices[i].y, 2) + std::pow(BfresShape.Value.Vertices[i].z, 2))); //sqrt(PointA^2 + PointB^2 + PointC^2) = distance to middle
            }
        }
    }
}

void BfresFile::EmbeddedFile::Read(BfresBinaryVectorReader& Reader)
{
    uint64_t Offset = Reader.ReadUInt64();
    uint32_t Size = Reader.ReadUInt32();
    Reader.ReadUInt32(); //padding

    Data = Reader.ReadCustom<std::vector<unsigned char>>([&Reader, &Size]()
        {
            return Reader.ReadRawArray<unsigned char>(Size);
        }, Offset);
}

BfresFile::BfresFile(std::string Path, std::vector<unsigned char> Bytes, std::string TexDir) : mTexDir(TexDir)
{
    BfresBinaryVectorReader Reader(Bytes);
    Reader.StringCache = ExternalBinaryStrings;

    Reader.ReadStruct(&BinHeader, sizeof(BinaryHeader));
    Reader.ReadStruct(&Header, sizeof(ResHeader));

    std::string Name = Reader.ReadStringOffset(Header.NameOffset);

    Reader.Seek(0xEE, BfresBinaryVectorReader::Position::Begin);
    uint8_t Flag = Reader.ReadUInt8();

    if (Header.MemoryPoolInfoOffset > 0)
    {
        Reader.Seek(Header.MemoryPoolInfoOffset, BfresBinaryVectorReader::Position::Begin);
        Reader.ReadStruct(&BufferMemoryPoolInfo, sizeof(BufferMemoryPool));
    }
    else
    {
        if (BinHeader.FileSize == Reader.GetSize())
        {
            Logger::Error("BfresDecoder", "Could not find pointer to memory buffer");
            return;
        }

        BufferMemoryPoolInfo.Offset = BinHeader.FileSize + 288; //FileSize is in this case the GlobalBufferOffset
    }

    Models = Reader.ReadDictionary<Model>(Header.ModelDictionaryOffset, Header.ModelOffset);
    EmbeddedFiles = Reader.ReadDictionary<EmbeddedFile>(Header.EmbeddedFilesDictionaryOffset, Header.EmbeddedFilesOffset);

    for (auto& [Key, BfresModel] : Models.Nodes)
    {
        BfresModel.Value.Init(*this, Reader, BufferMemoryPoolInfo, TexDir);
    }

    //m_DefaultModel = false; 
}

void BfresFile::DecompressMeshCodec(std::string Path, std::vector<unsigned char> Bytes)
{
    BfresBinaryVectorReader Reader(Bytes);
    Reader.ReadUInt64(); //Magic(4) + Version(4)
    int32_t Flags = Reader.ReadInt32();
    uint32_t DecompressedSize = (Flags >> 5) << (Flags & 0xF);
    Reader.Seek(0xC, BfresBinaryVectorReader::Position::Begin);
    std::vector<unsigned char> Source(Reader.GetSize() - 0xC);
    Reader.ReadStruct(Source.data(), Reader.GetSize() - 0xC);

    std::vector<unsigned char> Decompressed(DecompressedSize);

    ZSTD_DCtx* const DCtx = ZSTD_createDCtx();
    ZSTD_DCtx_setFormat(DCtx, ZSTD_format_e::ZSTD_f_zstd1_magicless);
    const size_t DecompSize = ZSTD_decompressDCtx(DCtx, (void*)Decompressed.data(), Decompressed.size(), Source.data(), Source.size());
    ZSTD_freeDCtx(DCtx);

    this->BfresFile::BfresFile(Path, Decompressed);
}

BfresFile::BfresFile(std::vector<unsigned char> Bytes)
{
    this->BfresFile::BfresFile("", Bytes);
}

BfresFile::BfresFile(std::string Path)
{
    std::ifstream File(Path, std::ios::binary);

    if (!File.eof() && !File.fail())
    {
        File.seekg(0, std::ios_base::end);
        std::streampos FileSize = File.tellg();

        std::vector<unsigned char> Bytes(FileSize);

        File.seekg(0, std::ios_base::beg);
        File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);

        File.close();

        if (Path.ends_with(".bfres"))
            this->BfresFile::BfresFile(Path, Bytes);
        else
            DecompressMeshCodec(Path, Bytes);
    }
    else
    {
        Logger::Error("Bfres", "Could not open file " + Path);
    }
}