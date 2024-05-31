#pragma once

#include <string>
#include <vector>

class HGHTFile {
public:
    struct Tile {
    };

    HGHTFile() { }
    HGHTFile(std::string Path);
    HGHTFile(std::vector<unsigned char> Bytes);
};
