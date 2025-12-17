#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

#include "AssetCatalog.h"

namespace Game {

struct AssetBufferView {
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;
    std::string virtualName;
};

class AssetData {
public:
    AssetData();

    const AssetCatalog& GetCatalog() const {
        return m_catalog;
    }

    AssetCatalog& GetCatalog() {
        return m_catalog;
    }

    static AssetData& Instance();

    bool Initialize();

    bool Parse(const AssetBufferView& view,
               const std::string& exportFormat = "none",
               bool silent = true,
               bool debug = false) const;

    bool ParseAndExport(const AssetBufferView& view,
                        const std::string& outputFile,
                        const std::string& exportFormat) const;

    void DebugTestNounParsing();

private:
    AssetCatalog m_catalog;
};

} // namespace Game
