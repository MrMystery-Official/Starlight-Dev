#pragma once

#include <cstdint>
#include <vector>

class EffectFile {
public:
    struct HeaderVFXB {
        char Magic[4];
        uint32_t Padding = 0;
        uint8_t Unknown1 = 0;
        uint16_t GraphicsAPIVersion = 0;
        uint16_t VFXVersion = 0;
        uint16_t ByteOrderMask = 0;
        uint16_t Unknown2 = 0;
        uint32_t FileNameOffset = 0;
        uint16_t Unknown3 = 0;
        uint16_t BaseSectionOffset = 0;
        uint32_t Unknown4 = 0;
        uint32_t FileSize = 0;
    };

    HeaderVFXB Header;

    EffectFile(std::vector<unsigned char> Bytes);
};