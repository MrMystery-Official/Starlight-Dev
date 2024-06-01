#pragma once

#include "TextureToGo.h"
#include <vector>

namespace TextureFormatDecoder {

void DecodeBC1(unsigned int Width, unsigned int Height, std::vector<unsigned char>& Data, std::vector<unsigned char>& Dest, TextureToGo* TexToGo);
void DecodeBC3SRG(unsigned int Width, unsigned int Height, std::vector<unsigned char>& Data, std::vector<unsigned char>& Dest, TextureToGo* TexToGo);
void DecodeBC4(unsigned int Width, unsigned int Height, std::vector<unsigned char>& Data, std::vector<unsigned char>& Dest, TextureToGo* TexToGo);
void DecodeASTC8x8UNorm(unsigned int Width, unsigned int Height, std::vector<unsigned char>& Data, std::vector<unsigned char>& Dest, TextureToGo* TexToGo);
void DecodeASTC4x4UNorm(unsigned int Width, unsigned int Height, std::vector<unsigned char>& Data, std::vector<unsigned char>& Dest, TextureToGo* TexToGo);

}
