#include "Serializer.hpp"

void Serializer::reset() {
    buf.clear();
    index = 0;
}

void Serializer::expect(char c) {
    if (index >= buf.size() || buf[index] != c) {
        throw std::runtime_error(std::string("Expecting char '") + c + std::string("' at position ") + std::to_string(index));
    }
    index++;
}

std::pair<const char*, size_t> Serializer::expectBencodeString() {
    if (index >= buf.size() || !isdigit(buf[index])) {
        throw std::runtime_error(std::string("Expecting decimal number at position '") + std::to_string(index));
    }
    char* endptr;
    unsigned long long len = strtoull(buf.c_str() + index, &endptr, 10);
    index += endptr - (buf.c_str() + index);
    expect(':');
    if ((unsigned long long)(uint32_t)index != index || index + len >= buf.size()) {
        throw std::runtime_error("String length too large at position:" + std::to_string(index));
    }

    const char* retPtr = buf.c_str() + index;
    index += len;
    return std::make_pair(retPtr, len);
}

void Serializer::beginSave() {
    reset();
    type = ReadWrite::Write;
    buf += "l";
}

void Serializer::endSave() {
    buf += "e";
}

void Serializer::saveToFile(std::string filename) {
    std::ofstream outf(filename, std::ios_base::out | std::ios_base::binary);
    outf.write(buf.data(), buf.size());
}

void Serializer::loadFromFile(std::string filename) {
    reset();
    std::ifstream inf(filename, std::ios_base::in | std::ios_base::binary);

    inf.seekg(0, std::ios_base::end);
    ssize_t sz = inf.tellg();
    buf.resize(sz);

    inf.seekg(0, std::ios_base::beg);
    inf.read(&buf[0], sz);
}

void Serializer::beginLoad() {
    type = ReadWrite::Read;
    expect('l');
}

void Serializer::endLoad() {
    expect('e');
}

void Serializer::handleByteBuffer(char const* key, void* s, size_t n) {
    size_t keyLen = strlen(key);

    if (type == ReadWrite::Read) {
        expect('d');
        auto readKey = expectBencodeString();
        if (readKey.second != keyLen || memcmp(readKey.first, key, keyLen)) {
            throw std::runtime_error(std::string("Key didn't match: ") + key);
        }
        auto readValue = expectBencodeString();
        if (readValue.second != n) {
            throw std::runtime_error(std::string("Value length didn't match: ") + std::to_string(n));
        }
        memcpy(s, readValue.first, n);
        expect('e');
    } else {
        buf += "d";

        buf += std::to_string(keyLen);
        buf += ":";
        buf += key;

        buf += std::to_string(n);
        buf += ":";
        buf.append((const char*)s, n);

        buf += "e";
    }
}
