#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct ResourceKey {
private:
    std::int32_t typeID;
    std::int32_t groupID;
    std::int32_t instanceID;
public:
    ResourceKey();
    ResourceKey(std::int32_t typeId, std::int32_t groupId, std::int32_t instanceId);
    std::int32_t GetTypeID() const;
    void SetTypeID(std::int32_t v);
    std::int32_t GetGroupID() const;
    void SetGroupID(std::int32_t v);
    std::int32_t GetInstanceID() const;
    void SetInstanceID(std::int32_t v);
    bool IsEquivalent(const ResourceKey& other) const;
    bool operator==(const ResourceKey& other) const;
};

struct ResourceKeyHasher {
    std::size_t operator()(const ResourceKey& k) const noexcept;
};

class DBPFManager {
public:
    enum class SourceMode {
        XmlOnly,
        PackageBridge
    };

    struct InitConfig {
        SourceMode mode;
        std::filesystem::path asset_package;
        std::string xml_root;
        std::filesystem::path registry_dir;
        std::size_t dom_cache_capacity;
        InitConfig();
    };

    static bool init(const InitConfig& cfg);
    static void shutdown();
    static bool has_binary(const ResourceKey& key);
    static bool get_binary(const ResourceKey& key, std::vector<std::uint8_t>& out);

    static bool debug_get_first_of_type(const std::string &typeName,
                                        std::vector<std::uint8_t> &out,
                                        std::string &virtualName);
};