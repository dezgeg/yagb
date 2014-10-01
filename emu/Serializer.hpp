#pragma once

#include <cctype>
#include <cstring>
#include <string>
#include <stdexcept>
#include <utility>
#include <fstream>

enum class ReadWrite {
    Read,
    Write,
};

class Serializer {
    std::string buf;
    size_t index;
    ReadWrite type;

    void reset();
    void expect(char c);
    std::pair<const char*, size_t> expectBencodeString();

public:
    void handleByteBuffer(const char* key, void* s, size_t n);

    template<typename T>
    inline void handleObject(const char* key, T& obj) {
        handleByteBuffer(key, (char*)&obj, sizeof(T));
    }

    void beginSave();
    void endSave();
    void saveToFile(std::string filename);

    void loadFromFile(std::string filename);
    void beginLoad();
    void endLoad();
};
