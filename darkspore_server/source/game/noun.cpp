// Include
#include "noun.h"
#include "config.h"

#include "utils/functions.h"
#include "utils/log.h"

#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <type_traits>

// Game
namespace Game {
	template<typename T>
	void ReadListNode(const YAML::Node& node, std::string_view nodeName, std::vector<T>& list) {
		if (node[nodeName.data()] && node[nodeName.data()].IsSequence()) {
			for (const auto& child : node[nodeName.data()]) {
				decltype(auto) data = list.emplace_back();
				if constexpr (std::is_same_v<T, std::string>) {
					data = child.as<std::string>("");
				} else if constexpr (std::is_class_v<T>) {
					data.Read(child);
				} else if constexpr (std::is_enum_v<T>) {
					data = static_cast<T>(child.as<std::underlying_type_t<T>>(0));
				} else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
					data = child.as<T>(T{});
				}
			}
		}
	}

	auto ReadAssetString(const YAML::Node& node, std::string_view nodeName) {
		// Haven't checked all files if its just "AssetStrings!hash_id(str)" but if it is you could just ignore that one.
		using namespace std::string_literals;
		if (node[nodeName.data()]) {
			return std::make_tuple(
				true,
				node[nodeName.data()]["str"].as<std::string>(""),
				node[nodeName.data()]["id"].as<std::string>("")
			);
		}
		return std::make_tuple(false, ""s, ""s);
	}

	glm::vec3 ReadVec3(const YAML::Node& node) {
		glm::vec3 data {};
		data.x = node["x"].as<float>(0.0f);
		data.y = node["y"].as<float>(0.0f);
		data.z = node["z"].as<float>(0.0f);
		return data;
	}

	// DoorData
	void DoorData::Read(const YAML::Node& node) {
		mClickToOpen = node["clickToOpen"].as<bool>(false);
		mClickToClose = node["clickToClose"].as<bool>(false);
		mInitialState = static_cast<DoorState>(node["initialState"].as<uint8_t>(0));
	}

	// SwitchData
	void SwitchData::Read(const YAML::Node& node) {
		// Empty
	}

	// PressureSwitchData
	void PressureSwitchData::Read(const YAML::Node& node) {
		if (node["volume"]) {
			const auto& volumeNode = node["volume"];
			switch (volumeNode["shape"].as<uint32_t>(0)) {
				// Sphere
				case 2: {
					mShape.sphere = BoundingSphere(
						glm::zero<glm::vec3>(),
						volumeNode["sphereRadius"].as<float>(1.0f)
					);
					break;
				}

				// Box
				case 3: {
					mShape.box = BoundingBox();
					mShape.box.extent = 0.5f * glm::vec3(
						volumeNode["boxWidth"].as<float>(1.0f),
						volumeNode["boxHeight"].as<float>(1.0f),
						volumeNode["boxLength"].as<float>(1.0f)
					);
					break;
				}

				// Capsule
				case 4: {
					mShape.capsule = BoundingCapsule(
						glm::zero<glm::vec3>(),
						volumeNode["capsuleRadius"].as<float>(1.0f),
						volumeNode["capsuleHeight"].as<float>(1.0f)
					);
					break;
				}
			}
		}
	}

	// CollisionVolume
	void CollisionVolume::Read(const YAML::Node& node) {
		mBoxExtents.x = node["boxWidth"].as<float>(1.0f);
		mBoxExtents.y = node["boxLength"].as<float>(1.0f);
		mBoxExtents.z = node["boxHeight"].as<float>(1.0f);
		mSphereRadius = node["sphereRadius"].as<float>(1.0f);
		mShape = static_cast<CollisionShape>(node["shape"].as<uint8_t>(0));
	}

	const glm::vec3& CollisionVolume::GetBoxExtents() const {
		return mBoxExtents;
	}

	float CollisionVolume::GetSphereRadius() const {
		return mSphereRadius;
	}

	CollisionShape CollisionVolume::GetShape() const {
		return mShape;
	}

	// ProjectileData
	void ProjectileData::Read(const YAML::Node& node) {
		if (node["creatureCollisionVolume"]) {
			mCreatureCollisionVolume = std::make_unique<CollisionVolume>();
			mCreatureCollisionVolume->Read(node["creatureCollisionVolume"]);
		}

		if (node["otherCollisionVolume"]) {
			mOtherCollisionVolume = std::make_unique<CollisionVolume>();
			mOtherCollisionVolume->Read(node["otherCollisionVolume"]);
		}

		mTargetType = static_cast<TargetType>(node["targetType"].as<uint32_t>(0));
	}

	const std::unique_ptr<CollisionVolume>& ProjectileData::GetCreatureCollisionVolume() const {
		return mCreatureCollisionVolume;
	}

	const std::unique_ptr<CollisionVolume>& ProjectileData::GetOtherCollisionVolume() const {
		return mOtherCollisionVolume;
	}

	TargetType ProjectileData::GetTargetType() const {
		return mTargetType;
	}

	// OrbitData
	void OrbitData::Read(const YAML::Node& node) {
		mHeight = node["orbitHeight"].as<float>(0.0f);
		mRadius = node["orbitRadius"].as<float>(0.0f);
		mSpeed = node["orbitSpeed"].as<float>(0.0f);
	}

	float OrbitData::GetHeight() const {
		return mHeight;
	}

	float OrbitData::GetRadius() const {
		return mRadius;
	}

	float OrbitData::GetSpeed() const {
		return mSpeed;
	}

	// ClassAttributes
	void ClassAttributes::Read(const YAML::Node& node) {
		mBaseAttributes[ClassAttribute::Health] = node["baseHealth"].as<float>(0.0f);
		mBaseAttributes[ClassAttribute::Mana] = node["baseMana"].as<float>(0.0f);
		mBaseAttributes[ClassAttribute::Strength] = node["baseStrength"].as<float>(0.0f);
		mBaseAttributes[ClassAttribute::Dexterity] = node["baseDexterity"].as<float>(0.0f);
		mBaseAttributes[ClassAttribute::Mind] = node["baseMind"].as<float>(0.0f);
		mBaseAttributes[ClassAttribute::PhysicalDefense] = node["basePhysicalDefense"].as<float>(0.0f);
		mBaseAttributes[ClassAttribute::MagicalDefense] = node["baseMagicalDefense"].as<float>(0.0f);
		mBaseAttributes[ClassAttribute::EnergyDefense] = node["baseEnergyDefense"].as<float>(0.0f);
		mBaseAttributes[ClassAttribute::Critical] = node["baseCritical"].as<float>(0.0f);
		mBaseAttributes[ClassAttribute::CombatSpeed] = node["baseCombatSpeed"].as<float>(0.0f);
		mBaseAttributes[ClassAttribute::NonCombatSpeed] = node["baseNonCombatSpeed"].as<float>(0.0f);
		mBaseAttributes[ClassAttribute::StealthDetection] = node["baseStealthDetection"].as<float>(0.0f);
		mBaseAttributes[ClassAttribute::MovementSpeedBuff] = node["baseMovementSpeedBuff"].as<float>(0.0f);

		mMaxAttributes[ClassAttribute::Health] = node["maxHealth"].as<float>(0.0f);
		mMaxAttributes[ClassAttribute::Mana] = node["maxMana"].as<float>(0.0f);
		mMaxAttributes[ClassAttribute::Strength] = node["maxStrength"].as<float>(0.0f);
		mMaxAttributes[ClassAttribute::Dexterity] = node["maxDexterity"].as<float>(0.0f);
		mMaxAttributes[ClassAttribute::Mind] = node["maxMind"].as<float>(0.0f);
		mMaxAttributes[ClassAttribute::PhysicalDefense] = node["maxPhysicalDefense"].as<float>(0.0f);
		mMaxAttributes[ClassAttribute::MagicalDefense] = node["maxMagicalDefense"].as<float>(0.0f);
		mMaxAttributes[ClassAttribute::EnergyDefense] = node["maxEnergyDefense"].as<float>(0.0f);
		mMaxAttributes[ClassAttribute::Critical] = node["maxCritical"].as<float>(0.0f);
	}

	float ClassAttributes::GetBaseAttribute(ClassAttribute::Key key) const {
		if (key < mBaseAttributes.size()) {
			return mBaseAttributes[key];
		}
		return 0.f;
	}

	float ClassAttributes::GetMaxAttribute(ClassAttribute::Key key) const {
		if (key < mMaxAttributes.size()) {
			return mMaxAttributes[key];
		}
		return 0.f;
	}

	// NpcAffix
	void NpcAffix::Read(const YAML::Node& node) {
		mChildName = node["mpChildAffix"].as<std::string>("");
		mParentName = node["mpParentAffix"].as<std::string>("");
		mModifier = node["modifier"].as<std::string>("");
	}

	const std::shared_ptr<NpcAffix>& NpcAffix::GetChild() const {
		return mChild;
	}

	const std::shared_ptr<NpcAffix>& NpcAffix::GetParent() const {
		return mParent;
	}

	// EliteAffix
	void EliteAffix::Read(const YAML::Node& node) {
		mMinDifficulty = node["minDifficulty"].as<int32_t>(0);
		mMaxDifficulty = node["maxDifficulty"].as<int32_t>(0);

		auto affixName = node["mpNPCAffix"].as<std::string>("");
		if (!affixName.empty()) {
			mNpcAffix = NounDatabase::Instance().GetNpcAffix(utils::hash_id(affixName));
		}
	}

	// PlayerClass
	void PlayerClass::Read(const YAML::Node& node) {
		mName = node["speciesName"].as<std::string>("");
		mEffect = node["mpClassEffect"].as<std::string>("");

		mAbilities[0] = node["basicAbility"].as<std::string>("");
		mAbilities[1] = node["specialAbility1"].as<std::string>("");
		mAbilities[2] = node["specialAbility2"].as<std::string>("");
		mAbilities[3] = node["specialAbility3"].as<std::string>("");
		mAbilities[4] = node["passiveAbility"].as<std::string>("");

		if (node["sharedAbilityOffset"]) {
			mSharedAbilityOffset = ReadVec3(node["sharedAbilityOffset"]);
		}

		mCreatureType = node["creatureType"].as<uint32_t>(0);
		mCreatureClass = node["creatureClass"].as<uint32_t>(0);
		mPrimaryAttribute = static_cast<PrimaryAttribute>(node["primaryAttribute"].as<uint32_t>(0xFFFFFFFF));
		mUnlockLevel = node["unlockLevel"].as<int32_t>(0);

		mHomeworld = static_cast<Homeworld>(node["homeworld"].as<uint32_t>(0));

		mWeaponMinDamage = node["weaponMinDamage"].as<float>(0.0f);
		mWeaponMaxDamage = node["weaponMaxDamage"].as<float>(0.0f);

		mNoHands = node["noHands"].as<bool>(false);
		mNoFeet = node["noFeet"].as<bool>(false);

		auto classAttributesName = node["mpClassAttributes"].as<std::string>("");
		if (!classAttributesName.empty()) {
			mAttributes = NounDatabase::Instance().GetClassAttributes(utils::hash_id(classAttributesName));
		}
	}

	const std::shared_ptr<ClassAttributes>& PlayerClass::GetAttributes() const {
		return mAttributes;
	}

	uint32_t PlayerClass::GetAbility(PlayerAbility ability) const {
		const auto index = static_cast<uint8_t>(ability);
		return index < mAbilities.size() ? utils::hash_id(mAbilities[index]) : 0;
	}

	const glm::vec3& PlayerClass::GetSharedAbilityOffset() const {
		return mSharedAbilityOffset;
	}

	Homeworld PlayerClass::GetHomeworld() const {
		return mHomeworld;
	}

	PrimaryAttribute PlayerClass::GetPrimaryAttribute() const {
		return mPrimaryAttribute;
	}

	// NonPlayerClass
	void NonPlayerClass::Read(const YAML::Node& node) {
		const auto& database = NounDatabase::Instance();
		if (const auto& [ok, str, id] = ReadAssetString(node, "name"); ok) {
			mName = str;
		}

		mEffect = node["mpClassEffect"].as<std::string>("");

		mCreatureType = node["creatureType"].as<uint32_t>(0);
		mNpcType = static_cast<NpcType>(node["mNPCType"].as<uint32_t>(0));

		mChallengeValue = node["challengeValue"].as<int32_t>(0);
		mNpcRank = node["npcRank"].as<int32_t>(0);

		mAggroRange = node["aggroRange"].as<float>(0.0f);
		mAlertRange = node["alertRange"].as<float>(0.0f);
		mDropAggroRange = node["dropAggroRange"].as<float>(0.0f);
		mDropDelay = node["dropDelay"].as<float>(0.0f);
		mPlayerCountHealthScale = node["playerCountHealthScale"].as<float>(0.0f);

		mTargetable = node["targetable"].as<bool>(false);
		mIsPet = node["playerPet"].as<bool>(false);

		ReadListNode(node, "dropType", mDropTypes);
		ReadListNode(node, "eliteAffix", mEliteAffixes);

		if (const auto& [ok, str, id] = ReadAssetString(node, "description"); ok) {
			mDescription = str;
		}

		if (node["longDescription"] && node["longDescription"].IsSequence()) {
			for (const auto& child : node["longDescription"]) {
				if (const auto& [ok, str, id] = ReadAssetString(child, "description"); ok) {
					mLongDescriptions.emplace(utils::hash_id(str), str);
				}
			}
		}

		auto classAttributesName = node["mpClassAttributes"].as<std::string>("");
		if (!classAttributesName.empty()) {
			mAttributes = database.GetClassAttributes(utils::hash_id(classAttributesName));
		}
	}

	const std::shared_ptr<ClassAttributes>& NonPlayerClass::GetAttributes() const {
		return mAttributes;
	}

	const std::string& NonPlayerClass::GetName() const {
		return mName;
	}

	const std::string& NonPlayerClass::GetDescription() const {
		return mDescription;
	}

	NpcType NonPlayerClass::GetType() const {
		return mNpcType;
	}

	float NonPlayerClass::GetAggroRange() const {
		return mAggroRange;
	}

	float NonPlayerClass::GetAlertRange() const {
		return mAlertRange;
	}

	float NonPlayerClass::GetDropAggroRange() const {
		return mDropAggroRange;
	}

	float NonPlayerClass::GetDropDelay() const {
		return mDropDelay;
	}

	bool NonPlayerClass::IsTargetable() const {
		return mTargetable;
	}

	bool NonPlayerClass::IsPet() const {
		return mIsPet;
	}

	// AssetProperty
	void AssetProperty::Read(const YAML::Node& node) {
		mName = node["name"].as<std::string>("");
		mValue = node["value"].as<std::string>("");

		mKey = node["key"].as<uint32_t>(0);
		mType = node["type"].as<uint32_t>(0);
	}

	// GambitDefinition
	void GambitDefinition::Read(const YAML::Node& node) {
		ReadListNode(node, "conditionProps", mConditionProperties);
		ReadListNode(node, "abilityProps", mAbilityProperties);
		
		if (const auto& value = node["condition"].as<std::string>(""); !value.empty()) {
			mCondition = utils::hash_id(value);
		}

		if (const auto& value = node["ability"].as<std::string>(""); !value.empty()) {
			mAbility = utils::hash_id(value);
		}

		mRandomizeCooldown = node["randomizeCooldown"].as<bool>(false);
	}

	const std::vector<AssetProperty>& GambitDefinition::GetConditionProperties() const {
		return mConditionProperties;
	}

	const std::vector<AssetProperty>& GambitDefinition::GetAbilityProperties() const {
		return mAbilityProperties;
	}

	uint32_t GambitDefinition::GetCondition() const {
		return mCondition;
	}

	uint32_t GambitDefinition::GetAbility() const {
		return mAbility;
	}

	// Phase
	void Phase::Read(const YAML::Node& node) {
		ReadListNode(node, "gambit", mGambit);

		mType = static_cast<PhaseType>(node["phaseType"].as<uint8_t>(0));

		mIsStartNode = node["startNode"].as<bool>(false);
	}

	const std::vector<GambitDefinition>& Phase::GetGambit() const {
		return mGambit;
	}

	PhaseType Phase::GetType() const {
		return mType;
	}

	bool Phase::IsStartNode() const {
		return mIsStartNode;
	}

	// AINode
	void AINode::Read(const YAML::Node& node) {
		const auto& database = NounDatabase::Instance();

		ReadListNode(node, "output", mOutput);

		auto phaseName = node["mpPhaseData"].as<std::string>("");
		if (!phaseName.empty()) {
			mPhase = database.GetPhase(utils::hash_id(phaseName));
		}

		/*
		auto conditionName = node["mpConditionData"].as<std::string>("");
		if (!conditionName.empty()) {
			mCondition = database.GetCondition(utils::hash_id(conditionName));
		}
		*/

		mX = node["nodeX"].as<int32_t>(0);
		mY = node["nodeY"].as<int32_t>(0);
	}

	const std::shared_ptr<Phase>& AINode::GetPhaseData() const {
		return mPhase;
	}

	const std::shared_ptr<Condition>& AINode::GetConditionData() const {
		return mCondition;
	}

	// AIDefinition
	void AIDefinition::Read(const YAML::Node& node) {
		ReadListNode(node, "ainode", mNodes);

		// Ability
		if (const auto& value = node["deathAbility"].as<std::string>(""); !value.empty()) { mDeathAbility = utils::hash_id(value); }
		if (const auto& value = node["deathCondition"].as<std::string>(""); !value.empty()) { mDeathCondition = utils::hash_id(value); }
		if (const auto& value = node["firstAggroAbility"].as<std::string>(""); !value.empty()) { mFirstAggroAbility = utils::hash_id(value); }
		if (const auto& value = node["firstAggroAbility2"].as<std::string>(""); !value.empty()) { mSecondaryFirstAggroAbility = utils::hash_id(value); }
		if (const auto& value = node["firstAlertAbility"].as<std::string>(""); !value.empty()) { mFirstAlertAbility = utils::hash_id(value); }
		if (const auto& value = node["subsequentAggroAbility"].as<std::string>(""); !value.empty()) { mSubsequentAggroAbility = utils::hash_id(value); }
		if (const auto& value = node["passiveAbility"].as<std::string>(""); !value.empty()) { mPassiveAbility = utils::hash_id(value); }

		// Behavior
		mCombatIdle = node["combatIdle"].as<std::string>("");
		mSecondaryCombatIdle = node["combatIdle2"].as<std::string>("");
		mSecondaryCombatIdleCondition = node["combatIdle2Condition"].as<std::string>("");
		mPassiveIdle = node["passiveIdle"].as<std::string>("");
		mPreAggroIdle = node["preAggroIdle"].as<std::string>("");
		mSecondaryPreAggroIdle = node["preAggroIdle2"].as<std::string>("");
		mTargetTooFar = node["targetTooFar"].as<std::string>("");

		mAggroType = node["aggroType"].as<uint32_t>(0);
		mCombatIdleCooldown = node["combatIdleCooldown"].as<uint32_t>(0);
		mSecondaryCombatIdleCooldown = node["combatIdle2Cooldown"].as<uint32_t>(0);
		mTargetTooFarCooldown = node["targetTooFarCooldown"].as<uint32_t>(0);

		mUseSecondaryStart = node["useSecondaryStart"].as<float>(0.0f);

		mFaceTarget = node["faceTarget"].as<bool>(false);
		mAlwaysRun = node["alwaysRunAI"].as<bool>(false);
		mRandomizeCooldowns = node["randomizeCooldowns"].as<bool>(false);
	}

	const std::vector<AINode>& AIDefinition::GetNodes() const {
		return mNodes;
	}

	uint32_t AIDefinition::GetDeathAbility() const {
		return mDeathAbility;
	}

	uint32_t AIDefinition::GetDeathCondition() const {
		return mDeathCondition;
	}

	uint32_t AIDefinition::GetFirstAggroAbility() const {
		return mFirstAggroAbility;
	}

	uint32_t AIDefinition::GetSecondaryFirstAggroAbility() const {
		return mSecondaryFirstAggroAbility;
	}

	uint32_t AIDefinition::GetFirstAlertAbility() const {
		return mFirstAlertAbility;
	}

	uint32_t AIDefinition::GetSubsequentAggroAbility() const {
		return mSubsequentAggroAbility;
	}

	uint32_t AIDefinition::GetPassiveAbility() const {
		return mPassiveAbility;
	}

	bool AIDefinition::FaceTarget() const {
		return mFaceTarget;
	}

	bool AIDefinition::AlwaysRun() const {
		return mAlwaysRun;
	}

	bool AIDefinition::RandomizeCooldowns() const {
		return mRandomizeCooldowns;
	}

	// CharacterAnimation
	void CharacterAnimation::Read(const YAML::Node& node) {
		// TODO: Change this into constexpr data + loop
		/*
			CharacterAnimation->add("gaitOverlay", uint32_t_type, 0x050);
			CharacterAnimation->add("overrideGait", char_type, std::tuple(0x000, 0x50));
			CharacterAnimation->add("ignoreGait", bool_type, 0x054);
			CharacterAnimation->add("morphology", key_type, 0x064);
		*/
		if (auto value = node["preAggroIdleAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::PreAggro)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["idleAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::Idle)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["lobbyIdleAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::LobbyIdle)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["specialIdleAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::SpecialIdle)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["walkStopState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::WalkStop)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["victoryIdleAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::VictoryIdle)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["combatIdleAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::CombatIdle)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["moveAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::Move)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["combatMoveAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::CombatMove)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["deathAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::Death)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["aggroAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::Aggro)] = AnimationData(
				utils::hash_id(value),
				node["aggroAnimDuration"].as<float>(0.0f)
			);
		}

		if (auto value = node["subsequentAggroAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::SubsequentAggro)] = AnimationData(
				utils::hash_id(value),
				node["subsequentAggroAnimDuration"].as<float>(0.0f)
			);
		}

		if (auto value = node["enterPassiveIdleAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::EnterPassiveIdle)] = AnimationData(
				utils::hash_id(value),
				node["enterPassiveIdleAnimDuration"].as<float>(0.0f)
			);
		}

		if (auto value = node["danceEmoteAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::Dance)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["tauntEmoteAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::Taunt)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["meleeDeathAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::MeleeDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["meleeCritDeathAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::MeleeCriticalDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["meleeCritKnockbackDeathAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::MeleeCriticalKnockbackDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["cyberCritDeathAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::CyberCriticalDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["cyberCritKnockbackDeathAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::CyberCriticalKnockbackDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["plasmaCritDeathAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::PlasmaCriticalDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["plasmaCritKnockbackDeathAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::PlasmaCriticalKnockbackDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["bioCritDeathAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::BioCriticalDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["bioCritKnockbackDeathAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::BioCriticalKnockbackDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["necroCritDeathAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::NecroCriticalDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["necroCritKnockbackDeathAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::NecroCriticalKnockbackDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["spacetimeCritDeathAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::SpacetimeCriticalDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["spacetimeCritKnockbackDeathAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::SpacetimeCriticalKnockbackDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["bodyFadeAnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::BodyFade)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["randomAbility1AnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::RandomAbility1)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["randomAbility2AnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::RandomAbility2)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["randomAbility3AnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::RandomAbility3)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["overlay1AnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::Overlay1)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["overlay2AnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::Overlay2)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = node["overlay3AnimState"].as<std::string>(""); !value.empty()) {
			mAnimationData[std::to_underlying(CharacterAnimationType::Overlay3)] = AnimationData(utils::hash_id(value), 0.f);
		}
	}

	AnimationData CharacterAnimation::GetAnimationData(CharacterAnimationType type) const {
		auto index = std::to_underlying(type);
		if (index < mAnimationData.size()) {
			return mAnimationData[index];
		}
		return AnimationData(0, 0.f);
	}

	// Noun
	void Noun::Read(const YAML::Node& node) {
		const auto& database = NounDatabase::Instance();
		if (node["nounType"]) {
			// Directly parsed from .Noun files
			mType = static_cast<NounType>(node["nounType"].as<uint32_t>(0));
			mPresetExtents = static_cast<PresetExtents>(node["presetExtents"].as<uint32_t>(0));
			mMovementType = static_cast<MovementType>(node["locomotionType"].as<uint8_t>(0));

			if (node["bbox"]) {
				const auto& bboxNode = node["bbox"];
				mBoundingBox = BoundingBox(
					ReadVec3(bboxNode["min"]),
					ReadVec3(bboxNode["max"])
				);
			}

			auto nonPlayerClassName = node["npcClassData"].as<std::string>("");
			if (!nonPlayerClassName.empty()) {
				mNpcClassData = database.GetNonPlayerClass(utils::hash_id(nonPlayerClassName));
			}

			auto playerClassName = node["playerClassData"].as<std::string>("");
			if (!playerClassName.empty()) {
				mPlayerClassData = database.GetPlayerClass(utils::hash_id(playerClassName));
			}

			auto aidefinitionName = node["aiDefinition"].as<std::string>("");
			if (!aidefinitionName.empty()) {
				mAIDefinition = database.GetAIDefinition(utils::hash_id(aidefinitionName));
			}

			auto characterAnimationName = node["characterAnimationData"].as<std::string>("");
			if (!characterAnimationName.empty()) {
				mCharacterAnimation = database.GetCharacterAnimation(utils::hash_id(characterAnimationName));
			}

			mAssetId = node["assetId"].as<uint64_t>(0);
			mTeam = node["spawnTeamId"].as<uint8_t>(0);
			ReadListNode(node, "eliteAssetIds", mEliteAssetIds);

			if (node["doorDef"]) {
				mDoorData = std::make_unique<DoorData>();
				mDoorData->Read(node["doorDef"]);
			}

			if (node["switchDef"]) {
				mSwitchData = std::make_unique<SwitchData>();
				mSwitchData->Read(node["switchDef"]);
			}

			if (node["pressureSwitchDef"]) {
				mPressureSwitchData = std::make_unique<PressureSwitchData>();
				mPressureSwitchData->Read(node["pressureSwitchDef"]);
			}

			if (node["projectile"]) {
				mProjectileData = std::make_unique<ProjectileData>();
				mProjectileData->Read(node["projectile"]);
			}

			if (node["orbit"]) {
				mOrbitData = std::make_unique<OrbitData>();
				mOrbitData->Read(node["orbit"]);
			}

			mHasLocomotion = node["hasLocomotion"].as<bool>(false);
			mIsFlora = node["isFlora"].as<bool>(false);
			mIsMineral = node["isMineral"].as<bool>(false);
			mIsCreature = node["isCreature"].as<bool>(false);
			mIsPlayer = node["isPlayer"].as<bool>(false);
			mIsSpawned = node["isSpawned"].as<bool>(false);
		} else {
			// Shortened versions
		}
	}

	std::tuple<glm::vec3, float> Noun::GetExtents(PresetExtents preset) {
		constexpr std::array<std::tuple<glm::vec3, float>, 17> objectExtents {
			// Critter
			std::make_tuple(glm::vec3(1, 1, 1), -10.f),
			std::make_tuple(glm::vec3(1, 1, 1), -10.f),

			// Minion
			std::make_tuple(glm::vec3(1.5f, 1.5f, 2.25f), 1.f),
			std::make_tuple(glm::vec3(1.6f, 1.6f, 1.5f), 1.f),

			// Elite minion
			std::make_tuple(glm::vec3(1.5f, 1.5f, 2.5f), 1.f),
			std::make_tuple(glm::vec3(1.75f, 1.75f, 1.75f), 1.f),

			// Player (ravager, tempest, sentinel)
			std::make_tuple(glm::vec3(1.5f, 1.5f, 3.75f), 1.f),
			std::make_tuple(glm::vec3(1.75f, 1.75f, 3.5f), 1.f),
			std::make_tuple(glm::vec3(2, 2, 4), 1.f),

			// Lieutenant
			std::make_tuple(glm::vec3(2.2f, 2.2f, 4), 1.f),
			std::make_tuple(glm::vec3(2.5f, 2.5f, 3.5f), 1.f),

			// Elite lieutenant
			std::make_tuple(glm::vec3(2.25f, 2.25f, 5.5f), 1.f),
			std::make_tuple(glm::vec3(2.5f, 2.5f, 4.5f), 1.f),

			// Captain
			std::make_tuple(glm::vec3(3, 3, 5), 1.f),
			std::make_tuple(glm::vec3(3.5f, 3.5f, 4.5f), 1.f),

			// Boss
			std::make_tuple(glm::vec3(4.5f, 4.5f, 9), 1.f),
			std::make_tuple(glm::vec3(5.5f, 5.5f, 8), 1.f)
		};

		const auto index = static_cast<size_t>(preset) - 1;
		if (index < objectExtents.size()) {
			return objectExtents[index];
		}

		return std::make_tuple(glm::zero<glm::vec3>(), 0.f);
	}

	const BoundingBox& Noun::GetBoundingBox() const {
		return mBoundingBox;
	}

	const std::unique_ptr<DoorData>& Noun::GetDoorData() const {
		return mDoorData;
	}

	const std::unique_ptr<SwitchData>& Noun::GetSwitchData() const {
		return mSwitchData;
	}

	const std::unique_ptr<PressureSwitchData>& Noun::GetPressureSwitchData() const {
		return mPressureSwitchData;
	}

	const std::unique_ptr<ProjectileData>& Noun::GetProjectileData() const {
		return mProjectileData;
	}

	const std::unique_ptr<OrbitData>& Noun::GetOrbitData() const {
		return mOrbitData;
	}

	const std::shared_ptr<NonPlayerClass>& Noun::GetNonPlayerClassData() const {
		return mNpcClassData;
	}

	const std::shared_ptr<PlayerClass>& Noun::GetPlayerClassData() const {
		return mPlayerClassData;
	}

	const std::shared_ptr<AIDefinition>& Noun::GetAIDefinition() const {
		return mAIDefinition;
	}

	const std::shared_ptr<CharacterAnimation>& Noun::GetCharacterAnimation() const {
		return mCharacterAnimation;
	}

	const std::string& Noun::GetName() const {
		return mName;
	}

	uint64_t Noun::GetAssetId() const {
		return mAssetId;
	}

	uint32_t Noun::GetId() const {
		return mId;
	}

	uint8_t Noun::GetTeam() const {
		return mTeam;
	}

	NounType Noun::GetType() const {
		return mType;
	}

	PresetExtents Noun::GetPresetExtents() const {
		return mPresetExtents;
	}

	MovementType Noun::GetMovementType() const {
		return mMovementType;
	}

	bool Noun::HasLocomotion() const {
		return mHasLocomotion;
	}

	bool Noun::IsCreature() const {
		return mIsCreature;
	}

	bool Noun::IsPlayer() const {
		return mIsPlayer;
	}

	// NounDatabase
	NounDatabase& NounDatabase::Instance() {
		static NounDatabase instance;
		if (!instance.mLoaded) {
			instance.Load();
		}
		return instance;
	}

	bool NounDatabase::Load() {
		mLoaded = true;
		if (!LoadPhases()) { return false; }
		if (!LoadCharacterAnimations()) { return false; }
		if (!LoadAIDefinitions()) { return false; }
		if (!LoadClassAttributes()) { return false; }
		if (!LoadNpcAffixes()) { return false; }
		if (!LoadPlayerClasses()) { return false; }
		if (!LoadNonPlayerClasses()) { return false; }
		if (!LoadNouns()) { return false; }
		return true;
	}

	NounPtr NounDatabase::Get(uint32_t id) const {
		if (auto it = mNouns.find(id); it != mNouns.end()) {
			return it->second;
		}
		return nullptr;
	}

	std::shared_ptr<NonPlayerClass> NounDatabase::GetNonPlayerClass(uint32_t id) const {
		if (auto it = mNonPlayerClasses.find(id); it != mNonPlayerClasses.end()) {
			return it->second;
		}
		return nullptr;
	}

	std::shared_ptr<PlayerClass> NounDatabase::GetPlayerClass(uint32_t id) const {
		if (auto it = mPlayerClasses.find(id); it != mPlayerClasses.end()) {
			return it->second;
		}
		return nullptr;
	}

	std::shared_ptr<NpcAffix> NounDatabase::GetNpcAffix(uint32_t id) const {
		if (auto it = mNpcAffixes.find(id); it != mNpcAffixes.end()) {
			return it->second;
		}
		return nullptr;
	}

	std::shared_ptr<ClassAttributes> NounDatabase::GetClassAttributes(uint32_t id) const {
		if (auto it = mClassAttributes.find(id); it != mClassAttributes.end()) {
			return it->second;
		}
		return nullptr;
	}

	std::shared_ptr<AIDefinition> NounDatabase::GetAIDefinition(uint32_t id) const {
		if (auto it = mAIDefinitions.find(id); it != mAIDefinitions.end()) {
			return it->second;
		}
		return nullptr;
	}

	std::shared_ptr<CharacterAnimation> NounDatabase::GetCharacterAnimation(uint32_t id) const {
		if (auto it = mCharacterAnimations.find(id); it != mCharacterAnimations.end()) {
			return it->second;
		}
		return nullptr;
	}

	std::shared_ptr<Phase> NounDatabase::GetPhase(uint32_t id) const {
		if (auto it = mPhases.find(id); it != mPhases.end()) {
			return it->second;
		}
		return nullptr;
	}

	bool NounDatabase::LoadNouns() {
		std::cout << "Loading nouns...";

		size_t failed = 0;

		const auto& dataPath = "data/noun/";
		if (!std::filesystem::exists(dataPath)) {
			std::cout << "Cannot open directory: " << dataPath << std::endl;
			return false;
		}

		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			const auto& path = entry.path();
			if (entry.is_regular_file() && path.extension() == ".yaml") {
				try {
					YAML::Node document = YAML::LoadFile(path.string());
					if (document["noun"]) {
						auto noun = std::make_shared<Noun>();
						noun->Read(document["noun"]);

						auto name = path.stem().string();
						noun->mId = utils::hash_id(name);
						noun->mName = name;

						mNouns.try_emplace(noun->mId, noun);
					}
				} catch (const YAML::Exception& e) {
					std::cout << "NounDatabase: Could not load '" << path << "': " << e.what() << std::endl;
					failed++;
				}
			}
		}

		if (mNouns.empty()) {
			std::cout << " failed." << std::endl;
			return false;
		} else {
			std::cout << " done. (loaded: " << mNouns.size() << ", failed: " << failed << ")" << std::endl;
			return true;
		}
	}

	bool NounDatabase::LoadNonPlayerClasses() {
		std::cout << "Loading nonplayerclasses...";

		size_t failed = 0;

		const auto& dataPath = "data/nonplayerclass/";
		if (!std::filesystem::exists(dataPath)) {
			std::cout << "Cannot open directory: " << dataPath << std::endl;
			return false;
		}

		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			const auto& path = entry.path();
			if (entry.is_regular_file() && path.extension() == ".yaml") {
				try {
					YAML::Node document = YAML::LoadFile(path.string());
					auto data = std::make_shared<NonPlayerClass>();
					if (document["nonplayerclass"]) {
						data->Read(document["nonplayerclass"]);

						auto name = path.stem().string();
						data->mId = utils::hash_id(name);
						data->mPath = name;

						mNonPlayerClasses.try_emplace(data->mId, data);
					}
				} catch (const YAML::Exception& e) {
					std::cout << "NounDatabase: Could not load '" << path << "': " << e.what() << std::endl;
					failed++;
				}
			}
		}

		if (mNonPlayerClasses.empty()) {
			std::cout << " failed." << std::endl;
			return false;
		} else {
			std::cout << " done. (loaded: " << mNonPlayerClasses.size() << ", failed: " << failed << ")" << std::endl;
			return true;
		}
	}

	bool NounDatabase::LoadPlayerClasses() {
		std::cout << "Loading playerclasses...";

		size_t failed = 0;

		const auto& dataPath = "data/playerclass/";
		if (!std::filesystem::exists(dataPath)) {
			std::cout << "Cannot open directory: " << dataPath << std::endl;
			return false;
		}

		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			const auto& path = entry.path();
			if (entry.is_regular_file() && path.extension() == ".yaml") {
				try {
					YAML::Node document = YAML::LoadFile(path.string());
					auto data = std::make_shared<PlayerClass>();
					if (document["playerclass"]) {
						data->Read(document["playerclass"]);

						auto name = path.stem().string();
						data->mId = utils::hash_id(name);
						data->mPath = name;

						mPlayerClasses.try_emplace(data->mId, data);
					}
				} catch (const YAML::Exception& e) {
					std::cout << "NounDatabase: Could not load '" << path << "': " << e.what() << std::endl;
					failed++;
				}
			}
		}

		if (mPlayerClasses.empty()) {
			std::cout << " failed." << std::endl;
			return false;
		} else {
			std::cout << " done. (loaded: " << mPlayerClasses.size() << ", failed: " << failed << ")" << std::endl;
			return true;
		}
	}

	bool NounDatabase::LoadNpcAffixes() {
		std::cout << "Loading npcaffixes...";

		size_t failed = 0;

		const auto& dataPath = "data/npcaffix/";
		if (!std::filesystem::exists(dataPath)) {
			std::cout << "Cannot open directory: " << dataPath << std::endl;
			return false;
		}

		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			const auto& path = entry.path();
			if (entry.is_regular_file() && path.extension() == ".yaml") {
				try {
					YAML::Node document = YAML::LoadFile(path.string());
					auto data = std::make_shared<NpcAffix>();
					if (document["npcaffix"]) {
						data->Read(document["npcaffix"]);

						auto name = path.stem().string();
						data->mId = utils::hash_id(name);
						data->mPath = name;

						mNpcAffixes.try_emplace(data->mId, data);
					}
				} catch (const YAML::Exception& e) {
					std::cout << "NounDatabase: Could not load '" << path << "': " << e.what() << std::endl;
					failed++;
				}
			}
		}

		if (mNpcAffixes.empty()) {
			std::cout << " failed." << std::endl;
			return false;
		} else {
			for (const auto& [_, affix] : mNpcAffixes) {
				affix->mChild = GetNpcAffix(utils::hash_id(affix->mChildName));
				affix->mParent = GetNpcAffix(utils::hash_id(affix->mParentName));
			}
			std::cout << " done. (loaded: " << mNpcAffixes.size() << ", failed: " << failed << ")" << std::endl;
			return true;
		}
	}

	bool NounDatabase::LoadClassAttributes() {
		std::cout << "Loading classattributes...";

		size_t failed = 0;

		const auto& dataPath = "data/classattributes/";
		if (!std::filesystem::exists(dataPath)) {
			std::cout << "Cannot open directory: " << dataPath << std::endl;
			return false;
		}

		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			const auto& path = entry.path();
			if (entry.is_regular_file() && path.extension() == ".yaml") {
				try {
					YAML::Node document = YAML::LoadFile(path.string());
					auto data = std::make_shared<ClassAttributes>();
					if (document["classattributes"]) {
						data->Read(document["classattributes"]);

						auto name = path.stem().string();
						data->mId = utils::hash_id(name);
						data->mPath = name;

						mClassAttributes.try_emplace(data->mId, data);
					}
				} catch (const YAML::Exception& e) {
					std::cout << "NounDatabase: Could not load '" << path << "': " << e.what() << std::endl;
					failed++;
				}
			}
		}

		if (mClassAttributes.empty()) {
			std::cout << " failed." << std::endl;
			return false;
		} else {
			std::cout << " done. (loaded: " << mClassAttributes.size() << ", failed: " << failed << ")" << std::endl;
			return true;
		}
	}

	bool NounDatabase::LoadAIDefinitions() {
		std::cout << "Loading aidefinitions...";

		size_t failed = 0;

		const auto& dataPath = "data/aidefinition/";
		if (!std::filesystem::exists(dataPath)) {
			std::cout << "Cannot open directory: " << dataPath << std::endl;
			return false;
		}

		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			const auto& path = entry.path();
			if (entry.is_regular_file() && path.extension() == ".yaml") {
				try {
					YAML::Node document = YAML::LoadFile(path.string());
					auto data = std::make_shared<AIDefinition>();
					if (document["aidefinition"]) {
						data->Read(document["aidefinition"]);

						auto name = path.stem().string();
						data->mId = utils::hash_id(name);
						data->mPath = name;

						mAIDefinitions.try_emplace(data->mId, data);
					}
				} catch (const YAML::Exception& e) {
					std::cout << "NounDatabase: Could not load '" << path << "': " << e.what() << std::endl;
					failed++;
				}
			}
		}

		if (mAIDefinitions.empty()) {
			std::cout << " failed." << std::endl;
			return false;
		} else {
			std::cout << " done. (loaded: " << mAIDefinitions.size() << ", failed: " << failed << ")" << std::endl;
			return true;
		}
	}

	bool NounDatabase::LoadCharacterAnimations() {
		std::cout << "Loading animations...";

		size_t failed = 0;

		const auto& dataPath = "data/animation/";
		if (!std::filesystem::exists(dataPath)) {
			std::cout << "Cannot open directory: " << dataPath << std::endl;
			return false;
		}

		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			const auto& path = entry.path();
			if (entry.is_regular_file() && path.extension() == ".yaml") {
				try {
					YAML::Node document = YAML::LoadFile(path.string());
					auto data = std::make_shared<CharacterAnimation>();
					if (document["characteranimation"]) {
						data->Read(document["characteranimation"]);

						auto name = path.stem().string();
						data->mId = utils::hash_id(name);
						data->mPath = name;

						mCharacterAnimations.try_emplace(data->mId, data);
					}
				} catch (const YAML::Exception& e) {
					std::cout << "NounDatabase: Could not load '" << path << "': " << e.what() << std::endl;
					failed++;
				}
			}
		}

		if (mCharacterAnimations.empty()) {
			std::cout << " failed." << std::endl;
			return false;
		} else {
			std::cout << " done. (loaded: " << mCharacterAnimations.size() << ", failed: " << failed << ")" << std::endl;
			return true;
		}
	}

	bool NounDatabase::LoadPhases() {
		std::cout << "Loading phases...";

		size_t failed = 0;

		const auto& dataPath = "data/phase/";
		if (!std::filesystem::exists(dataPath)) {
			std::cout << "Cannot open directory: " << dataPath << std::endl;
			return false;
		}

		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			const auto& path = entry.path();
			if (entry.is_regular_file() && path.extension() == ".yaml") {
				try {
					YAML::Node document = YAML::LoadFile(path.string());
					auto data = std::make_shared<Phase>();
					if (document["phase"]) {
						data->Read(document["phase"]);

						auto name = path.stem().string();
						data->mId = utils::hash_id(name);
						data->mPath = name;

						mPhases.try_emplace(data->mId, data);
					}
				} catch (const YAML::Exception& e) {
					std::cout << "NounDatabase: Could not load '" << path << "': " << e.what() << std::endl;
					failed++;
				}
			}
		}

		if (mPhases.empty()) {
			std::cout << " failed." << std::endl;
			return false;
		} else {
			std::cout << " done. (loaded: " << mPhases.size() << ", failed: " << failed << ")" << std::endl;
			return true;
		}
	}
}