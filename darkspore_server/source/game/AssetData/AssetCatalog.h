#pragma once

#include <cstdint>
#include <cstring>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
    #include <filesystem>
    namespace fs = std::filesystem;
#else
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#endif

namespace Game {

enum class DataType {
    BOOL,
    INT,
    INT16,
    INT64,
    UINT8,
    UINT16,
    UINT32,
    UINT64,
    FLOAT,
    GUID,
    VECTOR2,
    VECTOR3,
    QUATERNION,
    LOCALIZEDASSETSTRING,
    CHAR,
    CHAR_PTR,
    KEY,
    ASSET,
    CKEYASSET,
    NULLABLE,
    ARRAY,
    ENUM,
    STRUCT
};

struct TypeDefinition {
    std::string name;
    DataType type;
    std::size_t size;
    std::string targetType;

    TypeDefinition() = default;

    TypeDefinition(const std::string& n, DataType t, std::size_t s)
        : name(n), type(t), size(s), targetType() {
    }

    TypeDefinition(const std::string& n, DataType t, std::size_t s, const std::string& target)
        : name(n), type(t), size(s), targetType(target) {
    }
};

class OffsetManager {
public:
    OffsetManager(const std::uint8_t* data, std::size_t size)
        : m_data(data), m_size(size) {
    }

    void setPrimaryOffset(std::size_t offset) {
        m_primaryOffset = offset;
    }

    void setSecondaryOffset(std::size_t offset) {
        m_secondaryOffset = offset;
    }

    void setDisplaySecondaryOffset(std::size_t offset) {
        m_displaySecondaryOffset = offset;
    }

    std::size_t getPrimaryOffset() const {
        return m_primaryOffset;
    }

    std::size_t getSecondaryOffset() const {
        return m_displaySecondaryOffset > 0 ? m_displaySecondaryOffset : m_secondaryOffset;
    }

    std::size_t getRealSecondaryOffset() const {
        return m_secondaryOffset;
    }

    void advancePrimary(std::size_t bytes) {
        m_primaryOffset += bytes;
    }

    void advanceSecondary(std::size_t bytes) {
        m_secondaryOffset += bytes;
    }

    bool isValidOffset(std::size_t offset, std::size_t size) const {
        return offset + size <= m_size;
    }

    template<typename T>
    T readPrimary() {
        if (!isValidOffset(m_primaryOffset, sizeof(T))) {
            throw std::runtime_error("Attempted to read beyond end of buffer at offset " +
                                     std::to_string(m_primaryOffset));
        }

        T value{};
        std::memcpy(&value, m_data + m_primaryOffset, sizeof(T));
        m_primaryOffset += sizeof(T);
        return value;
    }

    template<typename T>
    T readSecondary() {
        if (!isValidOffset(m_secondaryOffset, sizeof(T))) {
            throw std::runtime_error("Attempted to read beyond end of buffer at secondary offset " +
                                     std::to_string(m_secondaryOffset));
        }

        T value{};
        std::memcpy(&value, m_data + m_secondaryOffset, sizeof(T));
        m_secondaryOffset += sizeof(T);
        return value;
    }

    template<typename T>
    T readAt(std::size_t offset) const {
        if (!isValidOffset(offset, sizeof(T))) {
            throw std::runtime_error("Attempted to read beyond end of buffer at offset " +
                                     std::to_string(offset));
        }

        T value{};
        std::memcpy(&value, m_data + offset, sizeof(T));
        return value;
    }

    std::string readString(bool useSecondary = false) {
        std::size_t currentOffset = useSecondary ? m_secondaryOffset : m_primaryOffset;

        if (currentOffset >= m_size) {
            throw std::runtime_error("Attempted to read string beyond end of buffer");
        }

        std::string result;
        while (currentOffset < m_size) {
            char c = static_cast<char>(m_data[currentOffset]);
            currentOffset++;
            if (c == '\0') break;
            result.push_back(c);
        }

        if (useSecondary) {
            m_secondaryOffset = currentOffset;
        } else {
            m_primaryOffset = currentOffset;
        }

        return result;
    }

private:
    const std::uint8_t* m_data = nullptr;
    std::size_t m_size = 0;

    std::size_t m_primaryOffset = 0;
    std::size_t m_secondaryOffset = 0;
    std::size_t m_displaySecondaryOffset = 0;
};

struct StructMember {
    std::string name;
    std::string typeName;
    std::size_t offset;
    bool useSecondaryOffset;
    std::string elementType;
    bool hasCustomName;
    std::size_t countOffset;

    StructMember(const std::string& n,
                 const std::string& t,
                 std::size_t offs,
                 bool useSec = false,
                 bool customName = false,
                 std::size_t cntOffs = 0)
        : name(n),
          typeName(t),
          offset(offs),
          useSecondaryOffset(useSec),
          elementType(),
          hasCustomName(customName),
          countOffset(cntOffs) {
    }

    StructMember(const std::string& n,
                 const std::string& t,
                 const std::string& elemType,
                 std::size_t offs,
                 bool useSec = false,
                 bool customName = false,
                 std::size_t cntOffs = 0)
        : name(n),
          typeName(t),
          offset(offs),
          useSecondaryOffset(useSec),
          elementType(elemType),
          hasCustomName(customName),
          countOffset(cntOffs) {
    }
};

class StructDefinition {
public:
    StructDefinition(const std::string& n, std::size_t fixedSize = 0)
        : m_name(n), m_fixedSize(fixedSize) {
    }

    const std::string& getName() const {
        return m_name;
    }

    std::size_t getFixedSize() const {
        return m_fixedSize;
    }

    StructDefinition* add(const std::string& name,
                          const std::string& typeName,
                          std::size_t offset,
                          bool useSecondaryOffset = false) {
        m_members.emplace_back(name, typeName, offset, useSecondaryOffset);
        return this;
    }

    StructDefinition* addArray(const std::string& name,
                               const std::string& elementType,
                               std::size_t offset,
                               bool useSecondaryOffset = false,
                               std::size_t countOffset = 0) {
        m_members.emplace_back(name, "array", elementType, offset, useSecondaryOffset, false, countOffset);
        return this;
    }

    StructDefinition* addStructArray(const std::string& name,
                                     std::shared_ptr<StructDefinition> elementStruct,
                                     std::size_t offset,
                                     std::size_t countOffset = 0,
                                     bool useSecondaryOffset = false) {
        m_members.emplace_back(name,
                               "array",
                               elementStruct->getName(),
                               offset,
                               useSecondaryOffset,
                               false,
                               countOffset);
        return this;
    }

    StructDefinition* add(std::shared_ptr<StructDefinition> structDef,
                          std::size_t offset) {
        m_members.emplace_back(structDef->getName(),
                               "struct:" + structDef->getName(),
                               offset,
                               false);
        return this;
    }

    StructDefinition* add(const std::string& name,
                          std::shared_ptr<StructDefinition> structDef,
                          std::size_t offset) {
        m_members.emplace_back(name,
                               "struct:" + structDef->getName(),
                               offset,
                               false,
                               true);
        return this;
    }

    StructDefinition* add(const std::string& name,
                          const std::string& typeName,
                          std::shared_ptr<StructDefinition> targetStruct,
                          std::size_t offset,
                          bool useSecondaryOffset = false) {
        if (typeName == "nullable") {
            std::string specificType = "nullable:" + targetStruct->getName();
            bool customName = (name != targetStruct->getName());
            m_members.emplace_back(name, specificType, offset, useSecondaryOffset, customName);
        } else {
            m_members.emplace_back(name, typeName, offset, useSecondaryOffset);
        }
        return this;
    }

    const std::vector<StructMember>& getMembers() const {
        return m_members;
    }

private:
    std::string m_name;
    std::size_t m_fixedSize;
    std::vector<StructMember> m_members;
};

struct VersionedFileTypeInfo {
    std::string version;
    std::vector<std::string> structTypes;
    std::size_t secondaryOffsetStart;

    VersionedFileTypeInfo(const std::string& v,
                          const std::vector<std::string>& types,
                          std::size_t offsetStart = 0)
        : version(v),
          structTypes(types),
          secondaryOffsetStart(offsetStart) {
    }
};

struct FileTypeInfo {
    std::vector<std::string> structTypes;
    std::size_t secondaryOffsetStart = 0;
    std::vector<VersionedFileTypeInfo> versionedInfo;

    FileTypeInfo() = default;

    FileTypeInfo(const std::vector<std::string>& types,
                 std::size_t offsetStart = 0)
        : structTypes(types),
          secondaryOffsetStart(offsetStart) {
    }

    FileTypeInfo(const std::vector<VersionedFileTypeInfo>& versions)
        : versionedInfo(versions) {
    }
};

class AssetCatalog {
public:
    AssetCatalog();

    void setGameVersion(const std::string& version) {
        m_currentGameVersion = version;
    }

    const std::string& getGameVersion() const {
        return m_currentGameVersion;
    }

    TypeDefinition* addType(const std::string& name,
                            DataType type,
                            std::size_t size) {
        m_types.emplace(name, TypeDefinition(name, type, size));
        return &m_types.at(name);
    }

    TypeDefinition* addType(const std::string& name,
                            DataType type,
                            std::size_t size,
                            const std::string& targetType) {
        m_types.emplace(name, TypeDefinition(name, type, size, targetType));
        return &m_types.at(name);
    }

    TypeDefinition* addArrayType(const std::string& name,
                                 const std::string& elementType,
                                 std::size_t size) {
        std::string arrayTypeName = "array:" + elementType;
        return addType(arrayTypeName, DataType::ARRAY, size, elementType);
    }

    std::shared_ptr<StructDefinition> add_struct(const std::string& name,
                                                 std::size_t fixedSize = 0) {
        auto structDef = std::make_shared<StructDefinition>(name, fixedSize);
        m_structs[name] = structDef;
        addType("struct:" + name, DataType::STRUCT, fixedSize, name);
        registerNullableType(name);
        return structDef;
    }

    void registerFileType(const std::string& extension,
                          const std::vector<std::string>& structTypes,
                          std::size_t secondaryOffsetStart = 0) {
        std::string ext = extension;
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        m_fileTypes.emplace(ext, FileTypeInfo(structTypes, secondaryOffsetStart));
    }

    void registerFileType(const std::string& extension,
                          const std::vector<VersionedFileTypeInfo>& versions) {
        std::string ext = extension;
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        m_fileTypes.emplace(ext, FileTypeInfo(versions));
    }

    void registerFileName(const std::string& fileName,
                          const std::vector<std::string>& structTypes,
                          std::size_t secondaryOffsetStart = 0) {
        std::string name = fileName;
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        m_exactFileNames.emplace(name, FileTypeInfo(structTypes, secondaryOffsetStart));
    }

    void registerFileName(const std::string& fileName,
                          const std::vector<VersionedFileTypeInfo>& versions) {
        std::string name = fileName;
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        m_exactFileNames.emplace(name, FileTypeInfo(versions));
    }

    void registerNullableType(const std::string& targetStructName) {
        std::string nullableTypeName = "nullable:" + targetStructName;
        if (m_types.find(nullableTypeName) == m_types.end()) {
            addType(nullableTypeName, DataType::NULLABLE, 4, targetStructName);
        }
    }

    const TypeDefinition* getType(const std::string& name) const {
        auto it = m_types.find(name);
        if (it != m_types.end()) {
            return &it->second;
        }
        return nullptr;
    }

    std::shared_ptr<StructDefinition> getStruct(const std::string& name) const {
        auto it = m_structs.find(name);
        if (it != m_structs.end()) {
            return it->second;
        }
        return nullptr;
    }

    const FileTypeInfo* getFileType(const std::string& extension) const {
        std::string ext = extension;
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        auto it = m_fileTypes.find(ext);
        if (it != m_fileTypes.end()) {
            return &it->second;
        }

        if (!ext.empty() && ext[0] == '.') {
            std::string extNoDot = ext.substr(1);
            it = m_fileTypes.find("." + extNoDot);
            if (it != m_fileTypes.end()) {
                return &it->second;
            }
        }

        return nullptr;
    }

    const FileTypeInfo* getFileTypeByName(const std::string& filename) const {
        std::string name = fs::path(filename).filename().string();
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        auto it = m_exactFileNames.find(name);
        if (it != m_exactFileNames.end()) {
            return &it->second;
        }

        return nullptr;
    }

    const VersionedFileTypeInfo* getVersionedFileTypeInfo(const FileTypeInfo* fileTypeInfo) const {
        if (!fileTypeInfo || fileTypeInfo->versionedInfo.empty()) {
            return nullptr;
        }

        for (const auto& versionInfo : fileTypeInfo->versionedInfo) {
            if (versionInfo.version == m_currentGameVersion) {
                return &versionInfo;
            }
        }

        return &fileTypeInfo->versionedInfo[0];
    }

    std::vector<std::string> getRegisteredFileTypes() const {
        std::vector<std::string> result;

        for (const auto& pair : m_fileTypes) {
            std::string ext = pair.first;
            if (!ext.empty() && ext[0] != '.') {
                ext = "." + ext;
            }

            std::string structInfo;
            if (!pair.second.structTypes.empty()) {
                structInfo = " (" + pair.second.structTypes[0];
                for (std::size_t i = 1; i < pair.second.structTypes.size(); i++) {
                    structInfo += ", " + pair.second.structTypes[i];
                }
                structInfo += ")";
            }

            result.push_back(ext + structInfo);
        }

        for (const auto& pair : m_exactFileNames) {
            std::string info = pair.first;

            if (!pair.second.structTypes.empty()) {
                info += " (" + pair.second.structTypes[0];
                for (std::size_t i = 1; i < pair.second.structTypes.size(); i++) {
                    info += ", " + pair.second.structTypes[i];
                }
                info += ")";
            }

            result.push_back(info + " [exact]");
        }

        std::sort(result.begin(), result.end());
        return result;
    }

    void initialize();

private:
    std::unordered_map<std::string, TypeDefinition> m_types;
    std::unordered_map<std::string, std::shared_ptr<StructDefinition>> m_structs;
    std::unordered_map<std::string, FileTypeInfo> m_fileTypes;
    std::unordered_map<std::string, FileTypeInfo> m_exactFileNames;
    std::string m_currentGameVersion = "5.3.0.103";
};

} // namespace Game
