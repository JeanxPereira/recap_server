#ifndef _GAME_LEVEL_HEADER
#define _GAME_LEVEL_HEADER

// Include
#include "noun.h"

#include <unordered_map>
#include <memory>
#include <string>
#include <sstream>

// Forward declare for diagnostics
namespace pugi {
    class xml_node;
}

// Game
namespace Game {
    // Add log macros if they don't exist in utils/log.h
    #ifndef GAME_LOG_DEBUG
    #define GAME_LOG_DEBUG(format, ...) utils::log::debug(format, ##__VA_ARGS__)
    #endif

    #ifndef GAME_LOG_INFO
    #define GAME_LOG_INFO(format, ...) utils::log::info(format, ##__VA_ARGS__)
    #endif

    #ifndef GAME_LOG_WARNING
    #define GAME_LOG_WARNING(format, ...) utils::log::warning(format, ##__VA_ARGS__)
    #endif

    #ifndef GAME_LOG_ERROR
    #define GAME_LOG_ERROR(format, ...) utils::log::error(format, ##__VA_ARGS__)
    #endif

    // Helper diagnostics functions
    namespace DiagHelpers {
        inline std::string NodePathToString(const pugi::xml_node& node) {
            std::stringstream path;
            for (auto current = node; current; current = current.parent()) {
                if (path.str().empty()) {
                    path << current.name();
                } else {
                    path << "/" << current.name();
                }
            }
            return path.str();
        }

        inline bool ValidateNode(const pugi::xml_node& node, const char* expected) {
            if (!node) {
                GAME_LOG_ERROR("Null node when expecting '{}'", expected);
                return false;
            }
            if (strcmp(node.name(), expected) != 0) {
                GAME_LOG_ERROR("Expected node '{}', got '{}'", expected, node.name());
                return false;
            }
            return true;
        }
    }

	// TriggerVolumeShape
	enum class TriggerVolumeShape : uint32_t {
		Sphere = 0,
		Box,
		Capsule
	};

	// TriggerVolumeActivationType
	enum class TriggerVolumeActivationType : uint32_t {
		EachObject = 0,
		EachPlayer,
		AnyObject,
		AllPlayers
	};

	// TriggerVolumeEvents
	class TriggerVolumeEvents {
		public:
			void Read(pugi::xml_node node);
			
			const std::string& GetOnEnterEvent() const { return mOnEnter; }
			const std::string& GetOnExitEvent() const { return mOnExit; }
			
			// Debug info helper
			std::string GetDebugInfo() const {
				std::stringstream ss;
				ss << "OnEnter: '" << mOnEnter << "', OnExit: '" << mOnExit << "'";
				return ss.str();
			}

		private:
			std::string mOnEnter;
			std::string mOnExit;
	};

	// TriggerVolumeData
	class TriggerVolumeData {
		public:
			void Read(pugi::xml_node node);

			const BoundingBox& GetBoundingBox() const;
			const glm::vec3& GetOffset() const;
			TriggerVolumeShape GetShape() const;
			TriggerVolumeActivationType GetActivationType() const;
			float GetTimeToActivate() const;
			
			// Debug helper
			std::string GetDebugInfo() const {
				std::stringstream ss;
				ss << "Shape: " << static_cast<int>(mShape)
				   << ", Activation: " << static_cast<int>(mActivationType)
				   << ", Offset: [" << mOffset.x << ", " << mOffset.y << ", " << mOffset.z << "]"
				   << ", TimeToActivate: " << mTimeToActivate;
				return ss.str();
			}

		private:
			BoundingBox mBoundingBox;
			TriggerVolumeEvents mEvents;
			glm::vec3 mOffset {};

			std::string mOnEnter;
			std::string mOnExit;
			std::string mOnStay;
			std::string mLuaCallbackOnEnter;
			std::string mLuaCallbackOnExit;
			std::string mLuaCallbackOnStay;

			TriggerVolumeShape mShape = TriggerVolumeShape::Sphere;
			TriggerVolumeActivationType mActivationType = TriggerVolumeActivationType::EachObject;

			float mTimeToActivate = 0;

			bool mUseObjectDimensions = false;
			bool mIsKinematic = false;
			bool mPersistentTimer = false;
			bool mTriggerOnceOnly = false;
			bool mTriggerIfNotBeaten = false;
	};

	// TeleporterData
	class TeleporterData {
		public:
			void Read(pugi::xml_node node);

			const std::unique_ptr<TriggerVolumeData>& GetTriggerVolumeData() const;
			uint32_t GetDestinationId() const;
			bool DeferTriggerCreation() const;
			
			// Debug helper
			std::string GetDebugInfo() const {
				std::stringstream ss;
				ss << "DestinationMarkerId: " << mDestinationMarkerId
				   << ", DeferTrigger: " << (mDeferTriggerCreation ? "true" : "false")
				   << ", HasTriggerVolume: " << (mTriggerVolumeData ? "true" : "false");
				if (mTriggerVolumeData) {
					ss << " - " << mTriggerVolumeData->GetDebugInfo();
				}
				return ss.str();
			}

		private:
			std::unique_ptr<TriggerVolumeData> mTriggerVolumeData;
			uint32_t mDestinationMarkerId = 0;
			bool mDeferTriggerCreation = false;
	};

	// MarkerInteractableData
	class MarkerInteractableData {
		public:
			void Read(pugi::xml_node node);

			const std::string& GetAbility() const;
			const std::string& GetStartInteractEvent() const;
			const std::string& GetEndInteractEvent() const;
			const std::string& GetOptionalInteractEvent() const;

			int32_t GetUsesAllowed() const;
			int32_t GetChallengeValue() const;
			
			// Debug helper
			std::string GetDebugInfo() const {
				std::stringstream ss;
				ss << "Ability: '" << mAbility << "'"
				   << ", UsesAllowed: " << mUsesAllowed
				   << ", ChallengeValue: " << mChallengeValue;
				return ss.str();
			}

		private:
			std::string mAbility;
			std::string mStartInteractEvent;
			std::string mEndInteractEvent;
			std::string mOptionalInteractEvent;

			int32_t mUsesAllowed = 0;
			int32_t mChallengeValue = 0;
	};

	// Marker
	class Marker {
		public:
			void Read(pugi::xml_node node);

			const std::unique_ptr<TeleporterData>& GetTeleporterData() const;
			const std::unique_ptr<MarkerInteractableData>& GetInteractableData() const;

			const glm::vec3& GetPosition() const;
			const glm::vec3& GetRotation() const;
			const std::string& GetNounName() const;
			const std::string& GetName() const { return mName; }

			uint64_t GetOverrideAssetId() const;
			uint32_t GetId() const;
			uint32_t GetNoun() const;
			uint32_t GetTargetId() const;
			float GetScale() const;
			
			bool HasCollision() const { return mHasCollision; }
			bool IsVisible() const { return mVisible; }
			
			// Debug helper
			std::string GetDebugInfo() const {
				std::stringstream ss;
				ss << "Name: '" << mName << "', ID: " << mId
				   << ", Noun: '" << mNounName << "' (" << mNoun << ")"
				   << ", Pos: [" << mPosition.x << ", " << mPosition.y << ", " << mPosition.z << "]"
				   << ", Rot: [" << mRotation.x << ", " << mRotation.y << ", " << mRotation.z << "]"
				   << ", Scale: " << mScale
				   << ", Visible: " << (mVisible ? "yes" : "no")
				   << ", Collision: " << (mHasCollision ? "yes" : "no");
				
				if (mOverrideAssetId) ss << ", AssetOverride: " << mOverrideAssetId;
				if (mTargetId) ss << ", TargetID: " << mTargetId;
				if (mTeleporterData) ss << ", Teleporter: {" << mTeleporterData->GetDebugInfo() << "}";
				if (mInteractableData) ss << ", Interactable: {" << mInteractableData->GetDebugInfo() << "}";
				
				return ss.str();
			}

		private:
			std::unique_ptr<TeleporterData> mTeleporterData;
			std::unique_ptr<MarkerInteractableData> mInteractableData;

			glm::vec3 mPosition;
			glm::vec3 mRotation;

			std::string mName;
			std::string mNounName;

			uint64_t mOverrideAssetId = 0;
			uint32_t mId = 0;
			uint32_t mNoun = 0;
			uint32_t mTargetId = 0;

			float mScale = 0;

			bool mVisible = false;
			bool mHasCollision = false;

			friend class Markerset;
	};

	using MarkerPtr = std::shared_ptr<Marker>;

	// Markerset
	class Markerset {
		public:
			bool Load(const std::string& path);
			void Read(pugi::xml_node node);

			const std::vector<MarkerPtr>& GetMarkers() const;
			std::vector<MarkerPtr> GetMarkersByType(uint32_t noun) const;
			
			const std::string& GetName() const { return mName; }
			
			// Helper methods for diagnostics
			void DumpMarkerInfo() const {
				GAME_LOG_INFO("Markerset '{}' contains {} markers:", mName, mMarkers.size());
				for (const auto& marker : mMarkers) {
					GAME_LOG_INFO("  - {}", marker->GetDebugInfo());
				}
				
				GAME_LOG_INFO("Marker count by noun type:");
				for (const auto& [noun, markers] : mMarkersByNoun) {
					GAME_LOG_INFO("  - Noun {}: {} markers", noun, markers.size());
				}
			}
			
			MarkerPtr FindMarkerById(uint32_t id) const {
				for (const auto& marker : mMarkers) {
					if (marker->GetId() == id) {
						return marker;
					}
				}
				return nullptr;
			}

		private:
			std::unordered_map<uint32_t, std::vector<MarkerPtr>> mMarkersByNoun;
			std::vector<MarkerPtr> mMarkers;
			std::string mName;

			friend class Level;
	};

	// DirectorClass
	class DirectorClass {
		public:
			void Read(pugi::xml_node node);

			const std::string& GetNounName() const;
			int32_t GetMinLevel() const;
			int32_t GetMaxLevel() const;
			bool IsHordeLegal() const;
			
			// Debug helper
			std::string GetDebugInfo() const {
				std::stringstream ss;
				ss << "NounName: '" << mNounName << "', MinLevel: " << mMinLevel
				   << ", MaxLevel: " << mMaxLevel
				   << ", HordeLegal: " << (mHordeLegal ? "yes" : "no");
				return ss.str();
			}

		private:
			std::string mNounName;
			int32_t mMinLevel = 0;
			int32_t mMaxLevel = 100;
			bool mHordeLegal = true;
	};

	// LevelConfig
	class LevelConfig {
		public:
			void Read(pugi::xml_node node);

			DirectorClass GetMinion(size_t idx) const;
			DirectorClass GetSpecial(size_t idx) const;
			DirectorClass GetBoss(size_t idx) const;
			DirectorClass GetAgent(size_t idx) const;
			DirectorClass GetCaptain(size_t idx) const;
			
			// Debug helpers
			size_t GetMinionCount() const { return mMinions.size(); }
			size_t GetSpecialCount() const { return mSpecial.size(); }
			size_t GetBossCount() const { return mBoss.size(); }
			size_t GetAgentCount() const { return mAgent.size(); }
			size_t GetCaptainCount() const { return mCaptain.size(); }
			
			void DumpDebugInfo() const {
				GAME_LOG_INFO("LevelConfig: {} minions, {} special, {} boss, {} agent, {} captain", 
					mMinions.size(), mSpecial.size(), mBoss.size(), mAgent.size(), mCaptain.size());
				
				if (!mMinions.empty()) {
					GAME_LOG_INFO("Minions:");
					for (size_t i = 0; i < mMinions.size(); i++) {
						GAME_LOG_INFO("  [{}] {}", i, mMinions[i].GetDebugInfo());
					}
				}
				
				if (!mSpecial.empty()) {
					GAME_LOG_INFO("Special:");
					for (size_t i = 0; i < mSpecial.size(); i++) {
						GAME_LOG_INFO("  [{}] {}", i, mSpecial[i].GetDebugInfo());
					}
				}
				
				if (!mBoss.empty()) {
					GAME_LOG_INFO("Boss:");
					for (size_t i = 0; i < mBoss.size(); i++) {
						GAME_LOG_INFO("  [{}] {}", i, mBoss[i].GetDebugInfo());
					}
				}
			}

		private:
			std::vector<DirectorClass> mMinions;
			std::vector<DirectorClass> mSpecial;
			std::vector<DirectorClass> mBoss;
			std::vector<DirectorClass> mAgent;
			std::vector<DirectorClass> mCaptain;
	};

	// Level
	class Level {
		public:
			Level();
			~Level();

			bool Load(const std::string& difficultyName, const std::string& levelName);
			bool GetMarkerset(const std::string& name, Markerset& markerset) const;

			const LevelConfig& GetConfig() const;
			const LevelConfig& GetFirstTimeConfig() const;
			const LevelConfig& GetPlanetConfig() const;
			
			// Debug helpers
			size_t GetMarkersetCount() const { return mMarkersets.size(); }
			
			std::vector<std::string> GetMarkersetNames() const {
				std::vector<std::string> names;
				for (const auto& [_, markerset] : mMarkersets) {
					names.push_back(markerset.mName);
				}
				return names;
			}
			
			void DumpDebugInfo() const {
				GAME_LOG_INFO("Level loaded with {} markersets", mMarkersets.size());
				
				// List all markersets
				GAME_LOG_INFO("Markersets:");
				for (const auto& [hash, markerset] : mMarkersets) {
					GAME_LOG_INFO("  - {} (hash: {}) with {} markers", 
						markerset.mName, hash, markerset.GetMarkers().size());
				}
				
				// Dump configs
				GAME_LOG_INFO("Level Configuration:");
				mConfig.DumpDebugInfo();
				
				GAME_LOG_INFO("First Time Configuration:");
				mFirstTimeConfig.DumpDebugInfo();
				
				// Count total markers
				size_t totalMarkers = 0;
				for (const auto& [_, markerset] : mMarkersets) {
					totalMarkers += markerset.GetMarkers().size();
				}
				GAME_LOG_INFO("Total markers across all markersets: {}", totalMarkers);
			}
			
			// Validate teleporter markers - find orphaned teleporters
			std::vector<MarkerPtr> ValidateTeleporterDestinations() const {
				std::vector<MarkerPtr> orphanedTeleporters;
				std::unordered_map<uint32_t, MarkerPtr> allMarkers;
				
				// First collect all markers by ID
				for (const auto& [_, markerset] : mMarkersets) {
					for (const auto& marker : markerset.GetMarkers()) {
						allMarkers[marker->GetId()] = marker;
					}
				}
				
				// Now check teleporters
				for (const auto& [_, markerset] : mMarkersets) {
					for (const auto& marker : markerset.GetMarkers()) {
						auto teleporter = marker->GetTeleporterData();
						if (teleporter) {
							uint32_t destId = teleporter->GetDestinationId();
							if (destId != 0 && allMarkers.find(destId) == allMarkers.end()) {
								GAME_LOG_WARNING("Teleporter marker '{}' (ID: {}) has invalid destination ID: {}", 
									marker->GetName(), marker->GetId(), destId);
								orphanedTeleporters.push_back(marker);
							}
						}
					}
				}
				
				return orphanedTeleporters;
			}

		private:
			std::unordered_map<uint32_t, Markerset> mMarkersets;
			LevelConfig mConfig;
			LevelConfig mFirstTimeConfig;
			LevelConfig mPlanetConfig;
	};
}

#endif