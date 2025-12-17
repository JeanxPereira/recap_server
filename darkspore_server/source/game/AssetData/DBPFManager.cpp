#include "DBPFManager.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <locale>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <sstream>
#include <cstring>
#include <cstdio>


ResourceKey::ResourceKey() : typeID(0), groupID(0), instanceID(0) {}

ResourceKey::ResourceKey(std::int32_t typeId, std::int32_t groupId, std::int32_t instanceId)
    : typeID(typeId), groupID(groupId), instanceID(instanceId) {}

std::int32_t ResourceKey::GetTypeID() const {
    return typeID;
}

void ResourceKey::SetTypeID(std::int32_t v) {
    typeID = v;
}

std::int32_t ResourceKey::GetGroupID() const {
    return groupID;
}

void ResourceKey::SetGroupID(std::int32_t v) {
    groupID = v;
}

std::int32_t ResourceKey::GetInstanceID() const {
    return instanceID;
}

void ResourceKey::SetInstanceID(std::int32_t v) {
    instanceID = v;
}

bool ResourceKey::IsEquivalent(const ResourceKey& other) const {
    return typeID == other.typeID && instanceID == other.instanceID;
}

bool ResourceKey::operator==(const ResourceKey& other) const {
    return typeID == other.typeID && groupID == other.groupID && instanceID == other.instanceID;
}

std::size_t ResourceKeyHasher::operator()(const ResourceKey& k) const noexcept {
    std::size_t h = static_cast<std::size_t>(static_cast<std::uint32_t>(k.GetTypeID()));
    h = h * 1315423911u + static_cast<std::size_t>(static_cast<std::uint32_t>(k.GetGroupID()));
    h = h * 1315423911u + static_cast<std::size_t>(static_cast<std::uint32_t>(k.GetInstanceID()));
    return h;
}

DBPFManager::InitConfig::InitConfig()
    : mode(SourceMode::XmlOnly),
      asset_package(),
      xml_root(),
      registry_dir(),
      dom_cache_capacity(0) {}

namespace dbpfmanager_detail {

static inline void trim(std::string& s) {
    auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    auto it1 = std::find_if_not(s.begin(), s.end(), [&](char c) { return is_space(static_cast<unsigned char>(c)); });
    auto it2 = std::find_if_not(s.rbegin(), s.rend(), [&](char c) { return is_space(static_cast<unsigned char>(c)); }).base();
    if (it1 >= it2) {
        s.clear();
        return;
    }
    s.assign(it1, it2);
}

static int fnv_hash_local(const std::string& s) {
    if (s.empty()) return 0;
    std::string lower;
    lower.reserve(s.size());
    for (char c : s) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    std::uint32_t rez = 0x811C9DC5u;
    for (char c : lower) {
        rez *= 0x1000193u;
        rez ^= static_cast<std::uint8_t>(c);
    }
    return static_cast<std::int32_t>(rez);
}

static int int32_local(const std::string& str) {
    std::string s = str;
    trim(s);
    if (s.empty()) return 0;

    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        std::uint32_t v = 0;
        std::stringstream ss(s.substr(2));
        ss >> std::hex >> v;
        if (!ss.fail()) return static_cast<std::int32_t>(v);
    }

    if (!s.empty() && s[0] == '#') {
        std::uint32_t v = 0;
        std::stringstream ss(s.substr(1));
        ss >> std::hex >> v;
        if (!ss.fail()) return static_cast<std::int32_t>(v);
    }

    if (!s.empty() && s[0] == '$') {
        std::string name = s.substr(1);
        return fnv_hash_local(name);
    }

    if (!s.empty() && s.back() == 'b') {
        std::string bits = s.substr(0, s.size() - 1);
        std::int32_t v = 0;
        for (char c : bits) {
            v <<= 1;
            if (c == '1') v |= 1;
        }
        return v;
    }

    std::int32_t v = 0;
    std::stringstream ss(s);
    ss >> v;
    return v;
}

class NameRegistry;

class HashManager;


class NameRegistry {
public:
    NameRegistry(const std::string& description, const std::string& fileName);

    void Clear();
    const std::string* GetName(std::int32_t hash) const;
    bool GetHash(const std::string& name, std::int32_t& out) const;
    void Add(const std::string& name, std::int32_t hash);
    void ReadFile(const std::filesystem::path& file);
    void Read(std::istream& in);
    bool IsEmpty() const;
    const std::string& GetFileName() const;
    const std::string& GetDescription() const;

private:
    std::unordered_map<std::string, std::int32_t> hashes;
    std::unordered_map<std::int32_t, std::string> names;
    std::string fileName;
    std::string description;

    static std::string StripComment(const std::string& s);
    void ParseEntry(const std::string& str);
};

NameRegistry::NameRegistry(const std::string& d, const std::string& f)
    : hashes(), names(), fileName(f), description(d) {}

void NameRegistry::Clear() {
    hashes.clear();
    names.clear();
}

const std::string* NameRegistry::GetName(std::int32_t hash) const {
    auto it = names.find(hash);
    if (it == names.end()) return nullptr;
    return &it->second;
}

bool NameRegistry::GetHash(const std::string& name, std::int32_t& out) const {
    auto it = hashes.find(name);
    if (it != hashes.end()) {
        out = it->second;
        return true;
    }
    std::string lower;
    lower.reserve(name.size());
    for (char c : name) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    it = hashes.find(lower);
    if (it != hashes.end()) {
        out = it->second;
        return true;
    }
    return false;
}

void NameRegistry::Add(const std::string& name, std::int32_t hash) {
    if (name.empty()) return;
    std::string key = name;
    if (!key.empty() && key.back() == '~') {
        std::string lower;
        lower.reserve(key.size());
        for (char c : key) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        key.swap(lower);
    }
    hashes[key] = hash;
    names[hash] = name;
}

void NameRegistry::ReadFile(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open registry file: " + file.string());
    Read(in);
}

std::string NameRegistry::StripComment(const std::string& s) {
    std::size_t pos = s.find("//");
    if (pos == std::string::npos) return s;
    return s.substr(0, pos);
}

void NameRegistry::ParseEntry(const std::string& str) {
    std::size_t tab = str.find('\t');
    if (tab == std::string::npos) {
        std::int32_t hash = fnv_hash_local(str);
        names[hash] = str;
        return;
    }
    std::string name = str.substr(0, tab);
    std::string hashStr = str.substr(tab + 1);
    trim(name);
    trim(hashStr);
    std::int32_t hashVal = int32_local(hashStr);
    if (!name.empty() && name.back() == '~') {
        std::string lower;
        lower.reserve(name.size());
        for (char c : name) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        hashes[lower] = hashVal;
    } else {
        hashes[name] = hashVal;
    }
    names[hashVal] = name;
}

void NameRegistry::Read(std::istream& in) {
    std::string line;
    while (std::getline(in, line)) {
        std::string noComment = StripComment(line);
        trim(noComment);
        if (noComment.empty()) continue;
        if (!noComment.empty() && noComment[0] == '#') continue;
        ParseEntry(noComment);
    }
}

bool NameRegistry::IsEmpty() const {
    return names.empty() && hashes.empty();
}

const std::string& NameRegistry::GetFileName() const {
    return fileName;
}

const std::string& NameRegistry::GetDescription() const {
    return description;
}

class HashManager {
public:
    static HashManager& Get() {
        static HashManager inst;
        return inst;
    }

    void Initialize(const std::filesystem::path& registryDir) {
        if (initialized) return;
        initialized = true;

        std::filesystem::path filePath = registryDir / fileRegistry.GetFileName();
        std::filesystem::path typePath = registryDir / typeRegistry.GetFileName();
        std::filesystem::path propPath = registryDir / propRegistry.GetFileName();
        std::filesystem::path simPath = registryDir / simulatorRegistry.GetFileName();
        std::filesystem::path simStubPath = registryDir / "reg_simulator_stub.txt";

        {
            std::ifstream in(filePath, std::ios::binary);
            if (!in) throw std::runtime_error("file name registry not found: " + filePath.string());
            fileRegistry.Read(in);
        }
        {
            std::ifstream in(typePath, std::ios::binary);
            if (!in) throw std::runtime_error("type registry not found: " + typePath.string());
            typeRegistry.Read(in);
        }
        {
            std::ifstream in(propPath, std::ios::binary);
            if (!in) throw std::runtime_error("property registry not found: " + propPath.string());
            propRegistry.Read(in);
        }
        {
            std::ifstream in(simPath, std::ios::binary);
            if (!in) throw std::runtime_error("simulator registry not found: " + simPath.string());
            simulatorRegistry.Read(in);
        }
        {
            std::ifstream in(simStubPath, std::ios::binary);
            if (!in) throw std::runtime_error("simulator stub registry not found: " + simStubPath.string());
            simulatorRegistry.Read(in);
        }
    }

    NameRegistry& GetProjectRegistry() {
        return projectRegistry;
    }

    std::int32_t FnvHash(const std::string& s) const {
        return fnv_hash_local(s);
    }

    std::string HexToString(int num) const {
        std::uint32_t v = static_cast<std::uint32_t>(num);
        char buf[11];
        std::snprintf(buf, sizeof(buf), "0x%08x", v);
        return std::string(buf);
    }

    std::string HexToStringUC(int num) const {
        std::uint32_t v = static_cast<std::uint32_t>(num);
        char buf[11];
        std::snprintf(buf, sizeof(buf), "0x%08X", v);
        return std::string(buf);
    }

    std::int32_t Int32(const std::string& str) {
        std::string s = str;
        trim(s);
        if (s.empty()) return 0;

        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            std::uint32_t v = 0;
            std::stringstream ss(s.substr(2));
            ss >> std::hex >> v;
            if (!ss.fail()) return static_cast<std::int32_t>(v);
        }

        if (!s.empty() && s[0] == '#') {
            std::uint32_t v = 0;
            std::stringstream ss(s.substr(1));
            ss >> std::hex >> v;
            if (!ss.fail()) return static_cast<std::int32_t>(v);
        }

        if (!s.empty() && s[0] == '$') {
            return GetFileHash(s.substr(1));
        }

        if (!s.empty() && s.back() == 'b') {
            std::string bits = s.substr(0, s.size() - 1);
            std::int32_t v = 0;
            for (char c : bits) {
                v <<= 1;
                if (c == '1') v |= 1;
            }
            return v;
        }

        std::int32_t v = 0;
        std::stringstream ss(s);
        ss >> v;
        return v;
    }

    std::string GetFileName(int hash) {
        const std::string* s = fileRegistry.GetName(hash);
        if (!s) s = projectRegistry.GetName(hash);
        if (s) return *s;
        return HexToStringUC(hash);
    }

    std::string GetTypeName(int hash) {
        const std::string* s = typeRegistry.GetName(hash);
        if (!s && !extraRegistry.IsEmpty()) s = extraRegistry.GetName(hash);
        if (s) return *s;
        return HexToStringUC(hash);
    }

    std::int32_t GetFileHash(const std::string& name) {
        if (name.empty()) return -1;

        if (!name.empty() && name[0] == '#') {
            std::uint32_t v = 0;
            std::stringstream ss(name.substr(1));
            ss >> std::hex >> v;
            return static_cast<std::int32_t>(v);
        }

        if (name.size() > 2 && name[0] == '0' && (name[1] == 'x' || name[1] == 'X')) {
            std::uint32_t v = 0;
            std::stringstream ss(name.substr(2));
            ss >> std::hex >> v;
            return static_cast<std::int32_t>(v);
        }

        std::int32_t h;
        if (fileRegistry.GetHash(name, h)) return h;
        if (projectRegistry.GetHash(name, h)) return h;

        std::int32_t hash = FnvHash(name);
        if (updateProjectRegistry) projectRegistry.Add(name, hash);
        return hash;
    }

    std::int32_t GetTypeHash(const std::string& name) {
        if (name.empty()) return -1;

        if (!name.empty() && name[0] == '#') {
            std::uint32_t v = 0;
            std::stringstream ss(name.substr(1));
            ss >> std::hex >> v;
            return static_cast<std::int32_t>(v);
        }

        if (name.size() > 2 && name[0] == '0' && (name[1] == 'x' || name[1] == 'X')) {
            std::uint32_t v = 0;
            std::stringstream ss(name.substr(2));
            ss >> std::hex >> v;
            return static_cast<std::int32_t>(v);
        }

        std::int32_t h;
        if (typeRegistry.GetHash(name, h)) return h;

        if (!extraRegistry.GetHash(name, h)) {
            h = FnvHash(name);
            extraRegistry.Add(name, h);
        }
        return h;
    }

private:
    HashManager()
        : fileRegistry("File Names", "reg_file.txt"),
          typeRegistry("Types", "reg_type.txt"),
          propRegistry("Properties", "reg_property.txt"),
          simulatorRegistry("Simulator Attributes", "reg_simulator.txt"),
          projectRegistry("Names used by the project", "names.txt"),
          extraRegistry("Extra Types", "extra_types.txt"),
          updateProjectRegistry(false),
          initialized(false) {}

    NameRegistry fileRegistry;
    NameRegistry typeRegistry;
    NameRegistry propRegistry;
    NameRegistry simulatorRegistry;
    NameRegistry projectRegistry;
    NameRegistry extraRegistry;
    bool updateProjectRegistry;
    bool initialized;
};

class FileReadStream {
public:
    explicit FileReadStream(const std::filesystem::path& path)
        : file(path, std::ios::binary), baseOffset(0) {
        if (!file) throw std::runtime_error("cannot open package: " + path.string());
    }

    void Seek(std::int64_t off) {
        file.seekg(off + baseOffset, std::ios::beg);
    }

    void SeekAbs(std::int64_t off) {
        file.seekg(off, std::ios::beg);
    }

    void Skip(std::int64_t n) {
        file.seekg(n, std::ios::cur);
    }

    std::int64_t GetFilePointer() {
        return static_cast<std::int64_t>(file.tellg()) - baseOffset;
    }

    std::int64_t GetFilePointerAbs() {
        return static_cast<std::int64_t>(file.tellg());
    }

    void SetBaseOffset(std::int64_t v) {
        baseOffset = v;
    }

    std::int64_t GetBaseOffset() const {
        return baseOffset;
    }

    void ReadBytes(std::uint8_t* dst, std::size_t len) {
        file.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(len));
        if (!file) throw std::runtime_error("unexpected end of file");
    }

    std::uint8_t ReadByte() {
        char c;
        file.read(&c, 1);
        if (!file) throw std::runtime_error("unexpected end of file");
        return static_cast<std::uint8_t>(c);
    }

    bool ReadBoolean() {
        std::uint8_t b = ReadByte();
        if (b == 0) return false;
        if (b == 1) return true;
        throw std::runtime_error("invalid boolean value");
    }

    std::int16_t ReadLEShort() {
        std::uint8_t b[2];
        ReadBytes(b, 2);
        return static_cast<std::int16_t>(static_cast<std::uint16_t>(b[0] | (b[1] << 8)));
    }

    std::int32_t ReadLEInt() {
        std::uint8_t b[4];
        ReadBytes(b, 4);
        std::uint32_t v = static_cast<std::uint32_t>(b[0])
                         | (static_cast<std::uint32_t>(b[1]) << 8)
                         | (static_cast<std::uint32_t>(b[2]) << 16)
                         | (static_cast<std::uint32_t>(b[3]) << 24);
        return static_cast<std::int32_t>(v);
    }

    std::uint32_t ReadLEUInt() {
        return static_cast<std::uint32_t>(ReadLEInt());
    }

    std::int64_t ReadLELong() {
        std::uint8_t b[8];
        ReadBytes(b, 8);
        std::uint64_t v = 0;
        v |= static_cast<std::uint64_t>(b[0]);
        v |= static_cast<std::uint64_t>(b[1]) << 8;
        v |= static_cast<std::uint64_t>(b[2]) << 16;
        v |= static_cast<std::uint64_t>(b[3]) << 24;
        v |= static_cast<std::uint64_t>(b[4]) << 32;
        v |= static_cast<std::uint64_t>(b[5]) << 40;
        v |= static_cast<std::uint64_t>(b[6]) << 48;
        v |= static_cast<std::uint64_t>(b[7]) << 56;
        return static_cast<std::int64_t>(v);
    }

private:
    std::ifstream file;
    std::int64_t baseOffset;
};

class MemoryBuffer {
public:
    MemoryBuffer() = default;
    explicit MemoryBuffer(std::size_t size) : data(size) {}
    explicit MemoryBuffer(std::vector<std::uint8_t>&& v) : data(std::move(v)) {}

    const std::vector<std::uint8_t>& GetRawData() const { return data; }
    std::vector<std::uint8_t>& GetRawData() { return data; }
    std::size_t Size() const { return data.size(); }

private:
    std::vector<std::uint8_t> data;
};

class RefPackCompression {
public:
    static void DecompressFast(const std::uint8_t* input, std::size_t inputSize, std::uint8_t* output, std::size_t outputSize) {
        if (inputSize < 5) throw std::runtime_error("input too small");
        std::size_t pin = 0;
        std::uint8_t cType = input[pin++];
        pin++;
        if (cType != 0x10 && cType != 0x50) throw std::runtime_error("unknown compression type");
        int decompSize = ((input[pin++] & 0xFF) << 16) | ((input[pin++] & 0xFF) << 8) | (input[pin++] & 0xFF);
        if (decompSize < 0) throw std::runtime_error("invalid decompressed size");
        if (outputSize < static_cast<std::size_t>(decompSize)) throw std::runtime_error("output buffer too small");
        int size = 0;
        while (size < decompSize) {
            if (pin >= inputSize) throw std::runtime_error("compressed data truncated");
            int numPlain = 0;
            int numCopy = 0;
            int copyOff = 0;
            int ctrl = input[pin++] & 0xFF;
            if (ctrl >= 252) {
                numPlain = ctrl & 0x03;
            } else if (ctrl >= 224) {
                numPlain = ((ctrl & 0x1F) << 2) + 4;
            } else if (ctrl >= 192) {
                if (pin + 3 > inputSize) throw std::runtime_error("compressed data truncated");
                std::uint8_t b1 = input[pin++];
                std::uint8_t b2 = input[pin++];
                std::uint8_t b3 = input[pin++];
                numPlain = ctrl & 0x03;
                numCopy = ((ctrl & 0x0C) << 6) + (b3 & 0xFF) + 5;
                copyOff = ((ctrl & 0x10) << 12) + ((b1 & 0xFF) << 8) + (b2 & 0xFF) + 1;
            } else if (ctrl >= 128) {
                if (pin + 2 > inputSize) throw std::runtime_error("compressed data truncated");
                std::uint8_t b2 = input[pin++];
                std::uint8_t b3 = input[pin++];
                numPlain = ((b2 & 0xC0) >> 6) & 0x03;
                numCopy = (ctrl & 0x3F) + 4;
                copyOff = ((b2 & 0x3F) << 8) + (b3 & 0xFF) + 1;
            } else {
                if (pin + 1 > inputSize) throw std::runtime_error("compressed data truncated");
                std::uint8_t b1 = input[pin++];
                numPlain = ctrl & 0x03;
                numCopy = ((ctrl & 0x1C) >> 2) + 3;
                copyOff = ((ctrl & 0x60) << 3) + (b1 & 0xFF) + 1;
            }
            if (numPlain > 0) {
                if (pin + static_cast<std::size_t>(numPlain) > inputSize) throw std::runtime_error("compressed data truncated");
                if (size + numPlain > decompSize) throw std::runtime_error("plain run exceeds output");
                std::memcpy(output + size, input + pin, static_cast<std::size_t>(numPlain));
                pin += static_cast<std::size_t>(numPlain);
                size += numPlain;
            }
            if (numCopy > 0) {
                if (copyOff <= 0 || copyOff > size) throw std::runtime_error("invalid copy offset");
                if (size + numCopy > decompSize) throw std::runtime_error("copy run exceeds output");
                if (numCopy > copyOff) {
                    for (int c = 0; c < numCopy; ++c) {
                        output[size] = output[size - copyOff];
                        size++;
                    }
                } else {
                    std::memcpy(output + size, output + size - copyOff, static_cast<std::size_t>(numCopy));
                    size += numCopy;
                }
            }
        }
    }
};

class DBPFItem {
public:
    DBPFItem()
        : isCompressed(false),
          chunkOffset(0),
          memSize(0),
          compressedSize(0),
          name(),
          isSaved(true) {}

    void Read(FileReadStream& s, bool isDBBF, bool readType, bool readGroup) {
        if (readType) name.SetTypeID(s.ReadLEInt());
        if (readGroup) name.SetGroupID(s.ReadLEInt());
        name.SetInstanceID(s.ReadLEInt());
        if (isDBBF) chunkOffset = static_cast<std::uint64_t>(s.ReadLELong());
        else chunkOffset = static_cast<std::uint64_t>(s.ReadLEUInt());
        compressedSize = s.ReadLEInt() & 0x7fffffff;
        memSize = s.ReadLEInt();
        std::int16_t comp = s.ReadLEShort();
        if (comp == 0) isCompressed = false;
        else if (comp == -1) isCompressed = true;
        else throw std::runtime_error("unknown compression label");
        isSaved = s.ReadBoolean();
        s.Skip(1);
    }

    MemoryBuffer ProcessFile(FileReadStream& s) const {
        s.Seek(static_cast<std::int64_t>(chunkOffset));
        if (memSize < 0) throw std::runtime_error("invalid memSize");
        if (compressedSize < 0 && isCompressed) throw std::runtime_error("invalid compressedSize");
        if (isCompressed) {
            std::vector<std::uint8_t> comp(static_cast<std::size_t>(compressedSize));
            if (!comp.empty()) s.ReadBytes(comp.data(), comp.size());
            MemoryBuffer buf(static_cast<std::size_t>(memSize));
            if (!buf.GetRawData().empty()) {
                RefPackCompression::DecompressFast(comp.data(), comp.size(), buf.GetRawData().data(), buf.GetRawData().size());
            }
            return buf;
        } else {
            MemoryBuffer buf(static_cast<std::size_t>(memSize));
            if (!buf.GetRawData().empty()) s.ReadBytes(buf.GetRawData().data(), buf.GetRawData().size());
            return buf;
        }
    }

    const ResourceKey& GetName() const {
        return name;
    }

private:
    bool isCompressed;
    std::uint64_t chunkOffset;
    int memSize;
    int compressedSize;
    ResourceKey name;
    bool isSaved;

    friend class DBPFIndex;
};

class DBPFIndex {
public:
    DBPFIndex() : groupID(-1), typeID(-1), items(), itemsOffset(0) {}

    void Read(FileReadStream& s) {
        int typeFlags = s.ReadLEInt();
        if (typeFlags & (1 << 0)) typeID = s.ReadLEInt();
        if (typeFlags & (1 << 1)) groupID = s.ReadLEInt();
        if (typeFlags & (1 << 2)) s.ReadLEInt();
        itemsOffset = s.GetFilePointer();
    }

    void ReadItems(FileReadStream& s, int numItems, bool isDBBF) {
        if (numItems < 0) throw std::runtime_error("negative index count");
        s.Seek(itemsOffset);
        bool readGroup = groupID == -1;
        bool readType = typeID == -1;
        items.clear();
        items.reserve(static_cast<std::size_t>(numItems));
        for (int i = 0; i < numItems; ++i) {
            DBPFItem item;
            if (!readGroup) item.name.SetGroupID(groupID);
            if (!readType) item.name.SetTypeID(typeID);
            item.Read(s, isDBBF, readType, readGroup);
            items.push_back(std::move(item));
        }
    }

    int GetGroupID() const {
        return groupID;
    }

    int GetTypeID() const {
        return typeID;
    }

    const std::vector<DBPFItem>& GetItems() const {
        return items;
    }

private:
    int groupID;
    int typeID;
    std::vector<DBPFItem> items;
    std::int64_t itemsOffset;

    friend class DatabasePackedFile;
};

class DatabasePackedFile {
public:
    DatabasePackedFile()
        : majorVersion(3),
          minVersion(0),
          indexMajorVersion(0),
          indexMinorVersion(3),
          isDBBF(false),
          index(),
          indexCount(0),
          indexOffset(0),
          indexSize(0) {}

    void ReadHeader(FileReadStream& s) {
        int magic = s.ReadLEInt();
        if (magic == TYPE_DBPF) {
            isDBBF = false;
            ReadDBPF(s);
        } else if (magic == TYPE_DBBF) {
            isDBBF = true;
            ReadDBBF(s);
        } else {
            throw std::runtime_error("unrecognised DBPF type magic");
        }
    }

    void ReadIndex(FileReadStream& s) {
        s.Seek(indexOffset);
        index.Read(s);
    }

    void ReadAll(FileReadStream& s) {
        ReadHeader(s);
        ReadIndex(s);
        index.ReadItems(s, indexCount, isDBBF);
    }

    const DBPFIndex& GetIndex() const { return index; }
    DBPFIndex& GetIndex() { return index; }
    int GetIndexCount() const { return indexCount; }
    bool IsDBBF() const { return isDBBF; }

    const DBPFItem* GetItem(const ResourceKey& key) const {
        for (const auto& it : index.items) {
            if (it.GetName() == key) return &it;
        }
        return nullptr;
    }

private:
    static const int TYPE_DBPF = 0x46504244;
    static const int TYPE_DBBF = 0x46424244;

    int majorVersion;
    int minVersion;
    int indexMajorVersion;
    int indexMinorVersion;
    bool isDBBF;
    DBPFIndex index;
    int indexCount;
    std::int64_t indexOffset;
    std::int64_t indexSize;

    void ReadDBPF(FileReadStream& s) {
        majorVersion = s.ReadLEInt();
        minVersion = s.ReadLEInt();
        s.Skip(20);
        indexMajorVersion = s.ReadLEInt();
        indexCount = s.ReadLEInt();
        s.Skip(4);
        indexSize = static_cast<std::int64_t>(s.ReadLEUInt());
        s.Skip(12);
        indexMinorVersion = s.ReadLEInt();
        indexOffset = static_cast<std::int64_t>(s.ReadLEUInt());
    }

    void ReadDBBF(FileReadStream& s) {
        majorVersion = s.ReadLEInt();
        minVersion = s.ReadLEInt();
        s.Skip(20);
        indexMajorVersion = s.ReadLEInt();
        indexCount = s.ReadLEInt();
        indexSize = static_cast<std::int64_t>(s.ReadLEInt());
        s.Skip(8);
        indexMinorVersion = s.ReadLEInt();
        indexOffset = static_cast<std::int64_t>(s.ReadLEInt());
    }
};

static void FindNamesFile(const std::vector<DBPFItem>& items, FileReadStream& s, HashManager& hasher) {
    int group = hasher.GetFileHash("sporemaster");
    int name = hasher.GetFileHash("names");

    for (const auto& it : items) {
        const ResourceKey& k = it.GetName();
        if (k.GetGroupID() == group && k.GetInstanceID() == name) {
            MemoryBuffer buf = it.ProcessFile(s);
            const auto& raw = buf.GetRawData();
            if (raw.empty()) {
                std::cout << "[DBPFManager] names file found but empty" << std::endl;
                return;
            }

            std::string text(reinterpret_cast<const char*>(raw.data()), raw.size());
            std::istringstream in(text);
            hasher.GetProjectRegistry().Read(in);

            std::cout << "[DBPFManager] loaded names registry from DBPF (sporemaster/names)" << std::endl;
            return;
        }
    }

    std::cout << "[DBPFManager] names registry not found inside DBPF; hex names may be used" << std::endl;
}

class DBPFManagerImpl {
public:
    explicit DBPFManagerImpl(const DBPFManager::InitConfig& cfg)
        : config(cfg),
          stream(),
          header(),
          itemsByKey(),
          mutex() {
        HashManager::Get().Initialize(cfg.registry_dir);

        if (config.mode != DBPFManager::SourceMode::PackageBridge) {
            std::cout << "[DBPFManager] initialized in XmlOnly mode (DBPF disabled)" << std::endl;
            return;
        }

        stream = std::make_unique<FileReadStream>(config.asset_package);
        header.ReadAll(*stream);

        const auto& index = header.GetIndex();
        const auto& items = index.GetItems();

        std::cout << "[DBPFManager] DBPF parsed "
                  << items.size()
                  << " items (indexCount=" << header.GetIndexCount()
                  << ", isDBBF=" << (header.IsDBBF() ? "true" : "false")
                  << ")" << std::endl;

        itemsByKey.reserve(items.size());
        for (const auto& item : items) {
            const ResourceKey& n = item.GetName();
            ResourceKey key(n.GetTypeID(), n.GetGroupID(), n.GetInstanceID());
            itemsByKey.emplace(std::move(key), &item);
        }

        FindNamesFile(items, *stream, HashManager::Get());
    }

    bool has_binary(const ResourceKey& key) const {
        if (config.mode != DBPFManager::SourceMode::PackageBridge) return false;
        return itemsByKey.find(key) != itemsByKey.end();
    }

    bool get_binary(const ResourceKey& key, std::vector<std::uint8_t>& out) const {
        if (config.mode != DBPFManager::SourceMode::PackageBridge) return false;
        auto it = itemsByKey.find(key);
        if (it == itemsByKey.end()) return false;
        const DBPFItem* item = it->second;
        std::lock_guard<std::mutex> lock(mutex);
        MemoryBuffer buf = item->ProcessFile(*stream);
        out = buf.GetRawData();
        return true;
    }

    bool debug_get_first_of_type(const std::string& typeName,
                                 std::vector<std::uint8_t>& out,
                                 std::string& virtualName) {
        if (config.mode != DBPFManager::SourceMode::PackageBridge)
            return false;

        auto& hasher = HashManager::Get();
        const int typeHash = hasher.GetTypeHash(typeName);

        const auto& items = header.GetIndex().GetItems();

        for (const auto& item : items) {
            const ResourceKey& k = item.GetName();
            if (k.GetTypeID() != typeHash)
                continue;

            {
                std::lock_guard<std::mutex> lock(mutex);
                MemoryBuffer buf = item.ProcessFile(*stream);
                out = buf.GetRawData();
            }

            // Build a readable virtual file name, e.g. "SomePhase.Phase"
            const std::string typeStr = hasher.GetTypeName(k.GetTypeID());
            const std::string fileStr = hasher.GetFileName(k.GetInstanceID());
            virtualName = fileStr + "." + typeStr;
            return true;
        }

        return false;
    }

private:
    DBPFManager::InitConfig config;
    std::unique_ptr<FileReadStream> stream;
    DatabasePackedFile header;
    std::unordered_map<ResourceKey, const DBPFItem*, ResourceKeyHasher> itemsByKey;
    mutable std::mutex mutex;
};

} // namespace dbpfmanager_detail

namespace {
std::unique_ptr<dbpfmanager_detail::DBPFManagerImpl> g_dbpfManager;
}

bool DBPFManager::init(const InitConfig& cfg) {
    try {
        g_dbpfManager = std::make_unique<dbpfmanager_detail::DBPFManagerImpl>(cfg);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[DBPFManager] init failed: " << e.what() << std::endl;
        g_dbpfManager.reset();
        return false;
    }
}

void DBPFManager::shutdown() {
    g_dbpfManager.reset();
}

bool DBPFManager::has_binary(const ResourceKey& key) {
    if (!g_dbpfManager) return false;
    return g_dbpfManager->has_binary(key);
}

bool DBPFManager::get_binary(const ResourceKey& key, std::vector<std::uint8_t>& out) {
    if (!g_dbpfManager) return false;
    return g_dbpfManager->get_binary(key, out);
}

bool DBPFManager::debug_get_first_of_type(const std::string& typeName,
                                          std::vector<std::uint8_t>& out,
                                          std::string& virtualName) {
    if (!g_dbpfManager) return false;
    return g_dbpfManager->debug_get_first_of_type(typeName, out, virtualName);
}