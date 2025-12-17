#include <cstring>
#include <stack>
#include <vector>
#include <memory>
#include <iostream>
#include <filesystem>
#include <algorithm>

#include <fmt/core.h>

#include "AssetData.h"
#include "AssetCatalog.h"
#include "DBPFManager.h"

namespace Game
{

class FormatExporter
{
public:
    virtual ~FormatExporter() = default;

    virtual void beginDocument() {}
    virtual void endDocument() {}

    virtual void beginNode(const std::string &) {}
    virtual void endNode() {}

    virtual void exportBool(const std::string &, bool) {}
    virtual void exportInt(const std::string &, int) {}
    virtual void exportUInt8(const std::string &, std::uint8_t) {}
    virtual void exportUInt16(const std::string &, std::uint16_t) {}
    virtual void exportUInt32(const std::string &, std::uint32_t) {}
    virtual void exportUInt64(const std::string &, std::uint64_t) {}
    virtual void exportInt64(const std::string &, std::int64_t) {}
    virtual void exportFloat(const std::string &, float) {}
    virtual void exportString(const std::string &, const std::string &) {}

    virtual void exportGuid(const std::string &, const std::string &) {}
    virtual void exportVector2(const std::string &, float, float) {}
    virtual void exportVector3(const std::string &, float, float, float) {}
    virtual void exportQuaternion(const std::string &, float, float, float, float) {}

    virtual void beginArray(const std::string &) {}
    virtual void beginArrayEntry() {}
    virtual void endArrayEntry() {}
    virtual void endArray() {}

    virtual bool saveToFile(const std::string &) { return false; }
};

class ExporterFactory
{
public:
    static std::unique_ptr<FormatExporter> createExporter(const std::string &)
    {
        return nullptr;
    }
};

class Parser
{
public:
    Parser(const AssetCatalog &catalog,
           const std::string &virtualName,
           const std::uint8_t *data,
           std::size_t size,
           bool silentMode,
           bool debugMode,
           const std::string &exportFormat)
        : catalog(catalog),
          offsetManager(data, size),
          filename(virtualName),
          silentMode(silentMode),
          debugMode(debugMode),
          exportMode(exportFormat != "none")
    {
        if (exportMode)
        {
            exporter = ExporterFactory::createExporter(exportFormat);
        }
    }

    bool parse();
    void exportToFile(const std::string &outputFile);

private:
    const AssetCatalog &catalog;
    OffsetManager offsetManager;
    std::string filename;

    std::size_t totalArraySize = 0;

    bool isInsideNullable = false;
    bool secOffsetStruct = false;
    std::stack<std::size_t> structOffsetStack;

    std::size_t structBaseStartOffset = 0;
    std::size_t startNullableOffset = 0;
    std::size_t structBaseOffset = 0;
    std::size_t currentStructBaseOffset = 0;
    std::stack<std::size_t> structBaseOffsetStack;

    bool processingArrayElement = false;
    bool isProcessingRootTag = false;
    bool silentMode = false;
    bool debugMode = false;
    bool exportMode = false;
    int indentLevel = 0;

    std::unique_ptr<FormatExporter> exporter;

    std::string getIndent() const
    {
        return std::string(static_cast<std::size_t>(indentLevel) * 4, ' ');
    }

    void logParse(const std::string &message)
    {
        if (silentMode)
        {
            return;
        }

        if (debugMode)
        {
            fmt::print("({}, {}) {}{}\n",
                       offsetManager.getPrimaryOffset(),
                       offsetManager.getSecondaryOffset(),
                       getIndent(),
                       message);
        }
        else
        {
            fmt::print("{}{}\n", getIndent(), message);
        }
    }

    void parseStruct(const std::string &structName, int arrayIndex = -1);
    void parseMember(const StructMember &member,
                     const std::shared_ptr<StructDefinition> &parentStruct,
                     std::size_t arraySize = 0);
};

bool Parser::parse()
{
    std::string extension = std::filesystem::path(filename).extension().string();

    const FileTypeInfo *fileType = catalog.getFileType(extension);
    if (!fileType)
    {
        fileType = catalog.getFileTypeByName(filename);
    }

    if (!fileType)
    {
        return false;
    }

    const VersionedFileTypeInfo *versionedInfo = catalog.getVersionedFileTypeInfo(fileType);

    std::vector<std::string> structTypes;
    std::size_t secondaryOffsetStart = 0;

    if (versionedInfo)
    {
        structTypes = versionedInfo->structTypes;
        secondaryOffsetStart = versionedInfo->secondaryOffsetStart;
    }
    else
    {
        structTypes = fileType->structTypes;
        secondaryOffsetStart = fileType->secondaryOffsetStart;
    }

    offsetManager.setPrimaryOffset(0);
    offsetManager.setSecondaryOffset(secondaryOffsetStart);

    if (exportMode && exporter)
    {
        exporter->beginDocument();
    }

    for (const auto &structType : structTypes)
    {
        isProcessingRootTag = true;
        parseStruct(structType);
        isProcessingRootTag = false;
    }

    if (exportMode && exporter)
    {
        exporter->endDocument();
    }

    return true;
}

void Parser::exportToFile(const std::string &outputFile)
{
    if (exportMode && exporter)
    {
        exporter->saveToFile(outputFile);
    }
}

void Parser::parseStruct(const std::string &structName, int arrayIndex)
{
    try
    {
        auto structDef = catalog.getStruct(structName);
        if (!structDef)
        {
            std::cerr << "Unknown struct: " << structName << std::endl;
            return;
        }

        if (arrayIndex >= 0)
        {
            logParse(fmt::format("parse_struct({}, [{}])", structName, arrayIndex));
        }
        else
        {
            logParse(fmt::format("parse_struct({})", structName));
        }

        bool shouldEndNode = false;

        if (exportMode && exporter)
        {
            if (isProcessingRootTag)
            {
                std::string lowerStructName = structName;
                std::transform(lowerStructName.begin(),
                               lowerStructName.end(),
                               lowerStructName.begin(),
                               [](unsigned char c)
                               { return static_cast<char>(std::tolower(c)); });
                exporter->beginNode(lowerStructName);
                isProcessingRootTag = false;
                shouldEndNode = true;
            }
            else if (arrayIndex < 0 && !isInsideNullable)
            {
                exporter->beginNode(structName);
                shouldEndNode = true;
            }
        }

        std::size_t previousStructBaseOffset = currentStructBaseOffset;
        std::size_t structBaseOffsetLocal = offsetManager.getPrimaryOffset();

        if (secOffsetStruct)
        {
            structBaseOffsetStack.push(previousStructBaseOffset);
            currentStructBaseOffset = offsetManager.getSecondaryOffset();

            if (!processingArrayElement)
            {
                offsetManager.setSecondaryOffset(offsetManager.getSecondaryOffset() + structDef->getFixedSize());
            }
        }

        indentLevel++;

        std::size_t structStartOffset = offsetManager.getPrimaryOffset();
        for (const auto &member : structDef->getMembers())
        {
            if (processingArrayElement)
            {
                offsetManager.setPrimaryOffset(structStartOffset);
            }
            parseMember(member, structDef);
        }

        indentLevel--;

        if (secOffsetStruct)
        {
            if (!structBaseOffsetStack.empty())
            {
                currentStructBaseOffset = structBaseOffsetStack.top();
                structBaseOffsetStack.pop();
            }
            else
            {
                currentStructBaseOffset = 0;
            }
        }
        else
        {
            currentStructBaseOffset = previousStructBaseOffset;
        }

        if (exportMode && exporter && shouldEndNode)
        {
            exporter->endNode();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in parse_struct(" << structName << "): "
                  << e.what() << " at position ("
                  << offsetManager.getPrimaryOffset() << ", "
                  << offsetManager.getSecondaryOffset() << ")" << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unknown error in parse_struct(" << structName << ") at position ("
                  << offsetManager.getPrimaryOffset() << ", "
                  << offsetManager.getSecondaryOffset() << ")" << std::endl;
    }
}

void Parser::parseMember(const StructMember &member,
                         const std::shared_ptr<StructDefinition> &parentStruct,
                         std::size_t arraySize)
{
    const TypeDefinition *typeDef = catalog.getType(member.typeName);
    if (!typeDef)
    {
        std::cerr << "Unknown type: " << member.typeName << std::endl;
        return;
    }

    std::size_t originalSecondaryOffset = offsetManager.getRealSecondaryOffset();
    std::size_t arrayStructOffset = offsetManager.getPrimaryOffset();

    if (member.typeName == "array")
    {
        std::size_t structStartOffsetLocal = currentStructBaseOffset;

        std::size_t arrayStartOffset;
        if (secOffsetStruct)
        {
            if (processingArrayElement)
            {
                arrayStartOffset = arrayStructOffset + member.offset;
            }
            else if (isInsideNullable)
            {
                arrayStartOffset = startNullableOffset + member.offset;
            }
            else
            {
                arrayStartOffset = structStartOffsetLocal + member.offset;
            }
        }
        else if (member.useSecondaryOffset)
        {
            arrayStartOffset = member.offset;
        }
        else
        {
            arrayStartOffset = currentStructBaseOffset + member.offset;
        }

        offsetManager.setPrimaryOffset(arrayStartOffset);
        std::uint32_t hasValue = offsetManager.readPrimary<std::uint32_t>();

        bool useSecondaryForElements = secOffsetStruct || (!secOffsetStruct && !processingArrayElement);
        std::size_t arrayDataOffset = useSecondaryForElements
                                          ? offsetManager.getSecondaryOffset()
                                          : offsetManager.getPrimaryOffset();

        if (hasValue != 0)
        {
            std::uint32_t count;
            if (member.countOffset > 0)
            {
                std::size_t countOffset = startNullableOffset + member.offset + member.countOffset;
                count = offsetManager.readAt<std::uint32_t>(countOffset);
            }
            else
            {
                count = offsetManager.readPrimary<std::uint32_t>();
            }

            const TypeDefinition *elemTypeDef = catalog.getType(member.elementType);
            auto structDef = catalog.getStruct(member.elementType);

            logParse(fmt::format("parse_member_array({}, {})", member.name, count));
            indentLevel++;

            if (exportMode && exporter)
            {
                exporter->beginArray(member.name);
            }

            if (structDef)
            {
                std::size_t elementSize = structDef->getFixedSize();
                std::size_t elementBaseOffset = offsetManager.getPrimaryOffset();

                totalArraySize = structDef->getFixedSize() * count;
                offsetManager.setSecondaryOffset(originalSecondaryOffset + totalArraySize);

                for (std::uint32_t i = 0; i < count; i++)
                {
                    if (exportMode && exporter)
                    {
                        exporter->beginArrayEntry();
                    }

                    if (useSecondaryForElements)
                    {
                        offsetManager.setPrimaryOffset(arrayDataOffset);
                        bool oldSecOffsetStruct = secOffsetStruct;
                        secOffsetStruct = true;
                        processingArrayElement = true;
                        parseStruct(member.elementType, static_cast<int>(i));
                        secOffsetStruct = oldSecOffsetStruct;
                        arrayDataOffset += structDef->getFixedSize();
                    }
                    else
                    {
                        offsetManager.setPrimaryOffset(elementBaseOffset);
                        processingArrayElement = true;
                        parseStruct(member.elementType, static_cast<int>(i));
                        elementBaseOffset += elementSize;
                    }

                    if (exportMode && exporter)
                    {
                        exporter->endArrayEntry();
                    }
                }
            }
            else
            {
                bool useSecondaryForElementsLocal = !secOffsetStruct && !processingArrayElement;
                std::size_t elementBaseOffset = useSecondaryForElementsLocal
                                                    ? offsetManager.getSecondaryOffset()
                                                    : offsetManager.getPrimaryOffset();

                std::size_t totalElementSize = count * (elemTypeDef ? elemTypeDef->size : 0);
                if (useSecondaryForElementsLocal)
                {
                    offsetManager.setSecondaryOffset(originalSecondaryOffset + totalElementSize);
                }

                for (std::size_t i = 0; i < count; i++)
                {
                    std::size_t elementSize = 0;
                    if (elemTypeDef)
                    {
                        elementSize = elemTypeDef->size;

                        if (useSecondaryForElementsLocal)
                        {
                            offsetManager.setPrimaryOffset(elementBaseOffset);
                            elementBaseOffset += elementSize;
                        }

                        StructMember elementMember("entry",
                                                   member.elementType,
                                                   offsetManager.getPrimaryOffset(),
                                                   useSecondaryForElementsLocal);

                        if (exportMode && exporter)
                        {
                            exporter->beginArrayEntry();
                        }

                        parseMember(elementMember, parentStruct);

                        if (exportMode && exporter)
                        {
                            exporter->endArrayEntry();
                        }

                        if (!useSecondaryForElementsLocal)
                        {
                            offsetManager.advancePrimary(elementSize);
                        }
                    }
                }
            }

            if (exportMode && exporter)
            {
                exporter->endArray();
            }

            processingArrayElement = false;
            indentLevel--;
        }

        return;
    }

    if (secOffsetStruct)
    {
        if (processingArrayElement)
        {
            offsetManager.setPrimaryOffset(arrayStructOffset + member.offset);
        }
        else
        {
            offsetManager.setPrimaryOffset(currentStructBaseOffset + member.offset);
        }
    }
    else if (member.useSecondaryOffset)
    {
        offsetManager.setPrimaryOffset(member.offset);
    }
    else
    {
        offsetManager.setPrimaryOffset(currentStructBaseOffset + member.offset);
    }

    std::string valueStr;
    std::string logMessage;

    switch (typeDef->type)
    {
    case DataType::BOOL:
    {
        bool value = offsetManager.readPrimary<bool>();
        valueStr = value ? "true" : "false";
        logMessage = fmt::format("parse_member_bool({}, {})", member.name, valueStr);
        if (exportMode && exporter)
        {
            exporter->exportBool(member.name, value);
        }
        break;
    }
    case DataType::INT:
    {
        int value = offsetManager.readPrimary<int>();
        valueStr = std::to_string(value);
        logMessage = fmt::format("parse_member_int({}, {})", member.name, valueStr);
        if (exportMode && exporter)
        {
            exporter->exportInt(member.name, value);
        }
        break;
    }
    case DataType::FLOAT:
    {
        float value = offsetManager.readPrimary<float>();
        valueStr = fmt::format("{:.5f}", value);
        logMessage = fmt::format("parse_member_float({}, {})", member.name, valueStr);
        if (exportMode && exporter)
        {
            exporter->exportFloat(member.name, value);
        }
        break;
    }
    case DataType::GUID:
    {
        std::uint32_t data1 = offsetManager.readPrimary<std::uint32_t>();
        std::uint16_t data2 = offsetManager.readPrimary<std::uint16_t>();
        std::uint16_t data3 = offsetManager.readPrimary<std::uint16_t>();
        std::uint64_t data4 = offsetManager.readPrimary<std::uint64_t>();

        valueStr = fmt::format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}",
                               data1,
                               data2,
                               data3,
                               (data4 >> 48) & 0xFFFFu,
                               data4 & 0xFFFFFFFFFFFFULL);

        logMessage = fmt::format("parse_member_guid({}, {})", member.name, valueStr);
        if (exportMode && exporter)
        {
            exporter->exportGuid(member.name, valueStr);
        }
        break;
    }
    case DataType::VECTOR2:
    {
        float x = offsetManager.readPrimary<float>();
        float y = offsetManager.readPrimary<float>();
        valueStr = fmt::format("x: {:.5f}, y: {:.5f}", x, y);
        logMessage = fmt::format("parse_member_cSPVector2({}, {})", member.name, valueStr);
        if (exportMode && exporter)
        {
            exporter->exportVector2(member.name, x, y);
        }
        break;
    }
    case DataType::VECTOR3:
    {
        float x = offsetManager.readPrimary<float>();
        float y = offsetManager.readPrimary<float>();
        float z = offsetManager.readPrimary<float>();
        valueStr = fmt::format("x: {:.5f}, y: {:.5f}, z: {:.5f}", x, y, z);
        logMessage = fmt::format("parse_member_cSPVector3({}, {})", member.name, valueStr);
        if (exportMode && exporter)
        {
            exporter->exportVector3(member.name, x, y, z);
        }
        break;
    }
    case DataType::QUATERNION:
    {
        float w = offsetManager.readPrimary<float>();
        float x = offsetManager.readPrimary<float>();
        float y = offsetManager.readPrimary<float>();
        float z = offsetManager.readPrimary<float>();
        valueStr = fmt::format("w: {:.5f}, x: {:.5f}, y: {:.5f}, z: {:.5f}", w, x, y, z);
        logMessage = fmt::format("parse_member_cSPVector4({}, {})", member.name, valueStr);
        if (exportMode && exporter)
        {
            exporter->exportQuaternion(member.name, w, x, y, z);
        }
        break;
    }
    case DataType::KEY:
    {
        std::uint32_t offset = offsetManager.readPrimary<std::uint32_t>();
        if (offset != 0)
        {
            std::string key = offsetManager.readString(true);
            valueStr = key;
            logMessage = fmt::format("parse_member_key({}, {})", member.name, valueStr);
            if (exportMode && exporter)
            {
                exporter->exportString(member.name, valueStr);
            }
        }
        else
        {
            return;
        }
        break;
    }
    case DataType::CKEYASSET:
    {
        std::uint32_t offset = offsetManager.readPrimary<std::uint32_t>();
        if (offset != 0)
        {
            std::string key = offsetManager.readString(true);
            valueStr = key;
            logMessage = fmt::format("parse_member_cKeyAsset({}, {})", member.name, valueStr);
            if (exportMode && exporter)
            {
                exporter->exportString(member.name, valueStr);
            }
        }
        else
        {
            return;
        }
        break;
    }
    case DataType::LOCALIZEDASSETSTRING:
    {
        std::uint32_t offset = offsetManager.readPrimary<std::uint32_t>();
        std::uint32_t assetString = offsetManager.readPrimary<std::uint32_t>();
        if (offset != 0)
        {
            std::string str = offsetManager.readString(true);
            if (assetString != 0)
            {
                std::string id = offsetManager.readString(true);
                logMessage = fmt::format("parse_member_cLocalizedAssetString({}, {}, {})",
                                         member.name, str, id);
                if (exportMode && exporter)
                {
                    exporter->beginNode(member.name);
                    exporter->exportString("text", str);
                    exporter->exportString("id", id);
                    exporter->endNode();
                }
            }
            else
            {
                logMessage = fmt::format("parse_member_cLocalizedAssetString({}, {})",
                                         member.name, str);
                if (exportMode && exporter)
                {
                    exporter->exportString(member.name, str);
                }
            }
        }
        else
        {
            return;
        }
        break;
    }
    case DataType::ASSET:
    {
        std::uint32_t offset = offsetManager.readPrimary<std::uint32_t>();
        if (offset > 0)
        {
            std::string asset = offsetManager.readString(true);
            valueStr = asset;
            logMessage = fmt::format("parse_member_asset({}, {})", member.name, valueStr);
            if (exportMode && exporter)
            {
                exporter->exportString(member.name, valueStr);
            }
        }
        else
        {
            return;
        }
        break;
    }
    case DataType::CHAR_PTR:
    {
        std::uint32_t offset = offsetManager.readPrimary<std::uint32_t>();
        if (offset > 0)
        {
            std::string charPtr = offsetManager.readString(true);
            valueStr = charPtr;
            logMessage = fmt::format("parse_member_char*({}, {})", member.name, valueStr);
            if (exportMode && exporter)
            {
                exporter->exportString(member.name, valueStr);
            }
        }
        else
        {
            return;
        }
        break;
    }
    case DataType::CHAR:
    {
        std::string stringValue = offsetManager.readString();
        valueStr = stringValue;
        if (!stringValue.empty() && stringValue != "0")
        {
            logMessage = fmt::format("parse_member_char({}, {})", member.name, valueStr);
            if (exportMode && exporter)
            {
                exporter->exportString(member.name, valueStr);
            }
        }
        else
        {
            return;
        }
        break;
    }
    case DataType::ENUM:
    {
        std::uint32_t value = offsetManager.readPrimary<std::uint32_t>();
        valueStr = std::to_string(value);
        logMessage = fmt::format("parse_member_enum({}, {})", member.name, valueStr);
        if (exportMode && exporter)
        {
            exporter->exportUInt32(member.name, value);
        }
        break;
    }
    case DataType::UINT8:
    {
        std::uint8_t value = offsetManager.readPrimary<std::uint8_t>();
        valueStr = std::to_string(value);
        logMessage = fmt::format("parse_member_uint8_t({}, {})", member.name, valueStr);
        if (exportMode && exporter)
        {
            exporter->exportUInt8(member.name, value);
        }
        break;
    }
    case DataType::UINT16:
    {
        std::uint16_t value = offsetManager.readPrimary<std::uint16_t>();
        valueStr = std::to_string(value);
        logMessage = fmt::format("parse_member_uint16_t({}, {})", member.name, valueStr);
        if (exportMode && exporter)
        {
            exporter->exportUInt16(member.name, value);
        }
        break;
    }
    case DataType::UINT32:
    {
        std::uint32_t value = offsetManager.readPrimary<std::uint32_t>();
        valueStr = std::to_string(value);
        logMessage = fmt::format("parse_member_uint32_t({}, {})", member.name, valueStr);
        if (exportMode && exporter)
        {
            exporter->exportUInt32(member.name, value);
        }
        break;
    }
    case DataType::UINT64:
    {
        std::uint64_t value = offsetManager.readPrimary<std::uint64_t>();
        valueStr = fmt::format("0x{:X}", value);
        logMessage = fmt::format("parse_member_uint64_t({}, {})", member.name, valueStr);
        if (exportMode && exporter)
        {
            exporter->exportUInt64(member.name, value);
        }
        break;
    }
    case DataType::INT64:
    {
        std::int64_t value = offsetManager.readPrimary<std::int64_t>();
        valueStr = fmt::format("0x{:X}", value);
        logMessage = fmt::format("parse_member_int64_t({}, {})", member.name, valueStr);
        if (exportMode && exporter)
        {
            exporter->exportInt64(member.name, value);
        }
        break;
    }
    case DataType::NULLABLE:
    {
        std::size_t startOffset = offsetManager.getPrimaryOffset();
        std::uint32_t hasValue = offsetManager.readPrimary<std::uint32_t>();

        if (hasValue > 0 && !typeDef->targetType.empty())
        {
            auto targetStruct = catalog.getStruct(typeDef->targetType);
            if (targetStruct)
            {
                if (member.hasCustomName)
                {
                    logParse(fmt::format("parse_member_nullable({}, {})",
                                         member.name, typeDef->targetType));
                }
                else
                {
                    logParse(fmt::format("parse_member_nullable({})",
                                         typeDef->targetType));
                }

                startNullableOffset = offsetManager.getRealSecondaryOffset();

                bool oldSecOffsetStruct = secOffsetStruct;
                std::size_t oldStructBaseOffset = currentStructBaseOffset;
                bool oldProcessingArrayElement = processingArrayElement;

                secOffsetStruct = true;
                processingArrayElement = true;
                isInsideNullable = true;

                offsetManager.setPrimaryOffset(offsetManager.getSecondaryOffset());
                offsetManager.setSecondaryOffset(originalSecondaryOffset + targetStruct->getFixedSize());

                if (exportMode && exporter)
                {
                    exporter->beginNode(member.name);
                    parseStruct(typeDef->targetType);
                    exporter->endNode();
                }
                else
                {
                    parseStruct(typeDef->targetType);
                }

                processingArrayElement = oldProcessingArrayElement;
                secOffsetStruct = oldSecOffsetStruct;
                currentStructBaseOffset = oldStructBaseOffset;
                isInsideNullable = false;

                offsetManager.setPrimaryOffset(startOffset + 4);
                return;
            }
        }
        else
        {
            offsetManager.setPrimaryOffset(startOffset + 4);
            return;
        }
        break;
    }
    case DataType::STRUCT:
    {
        if (member.hasCustomName)
        {
            logMessage = fmt::format("parse_member_struct({}, {})", member.name, typeDef->targetType);
        }
        else
        {
            logMessage = fmt::format("parse_member_struct({})", typeDef->targetType);
        }

        logParse(logMessage);

        std::size_t currentOffset = offsetManager.getPrimaryOffset();
        std::size_t previousBaseOffset = currentStructBaseOffset;
        currentStructBaseOffset = currentOffset;

        if (exportMode && exporter && member.hasCustomName)
        {
            exporter->beginNode(member.name);
            parseStruct(typeDef->targetType);
            exporter->endNode();
        }
        else
        {
            parseStruct(typeDef->targetType);
        }

        currentStructBaseOffset = previousBaseOffset;
        return;
    }
    default:
    {
        valueStr = "unknown";
        logMessage = fmt::format("parse_member_unknown({}, {})", member.name, valueStr);
        break;
    }
    }

    logParse(logMessage);
}

// -------- AssetData implementation --------

AssetData::AssetData()
    : m_catalog()
{
    m_catalog.initialize();
}

AssetData& AssetData::Instance()
{
    static AssetData instance;
    return instance;
}

bool AssetData::Initialize()
{
    return true;
}

bool AssetData::Parse(const AssetBufferView& view,
                      const std::string& exportFormat,
                      bool silent,
                      bool debug) const
{
    if (!view.data || view.size == 0)
    {
        return false;
    }

    Parser parser(m_catalog,
                  view.virtualName,
                  view.data,
                  view.size,
                  silent,
                  debug,
                  exportFormat);
    return parser.parse();
}

bool AssetData::ParseAndExport(const AssetBufferView& view,
                               const std::string& outputFile,
                               const std::string& exportFormat) const
{
    if (!view.data || view.size == 0)
    {
        return false;
    }

    Parser parser(m_catalog,
                  view.virtualName,
                  view.data,
                  view.size,
                  true,
                  false,
                  exportFormat);

    bool ok = parser.parse();
    if (!ok)
    {
        return false;
    }

    parser.exportToFile(outputFile);
    return true;
}

void AssetData::DebugTestNounParsing()
{
    std::vector<std::uint8_t> buffer;
    std::string virtualName;

    if (!DBPFManager::debug_get_first_of_type("Noun", buffer, virtualName))
    {
        std::cout << "[AssetData] DebugTestNounParsing: no Noun resource found or DBPF disabled.\n";
        return;
    }

    if (buffer.empty())
    {
        std::cout << "[AssetData] DebugTestNounParsing: Noun buffer is empty.\n";
        return;
    }

    if (virtualName.empty())
    {
        virtualName = "debug_noun.Noun";
    }

    AssetBufferView view;
    view.data = buffer.data();
    view.size = buffer.size();
    view.virtualName = virtualName;

    std::cout << "[AssetData] Debug parsing Noun resource: " << view.virtualName
              << " (size=" << view.size << " bytes)\n";

    bool ok = Parse(view, "none", false, true);

    std::cout << "[AssetData] Debug Noun parse result: "
              << (ok ? "OK" : "FAILED") << "\n";
}

} // namespace Game
