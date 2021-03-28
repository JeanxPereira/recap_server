
// Include
#include "noun.h"
#include "config.h"

#include "utils/functions.h"
#include "utils/log.h"

#include <filesystem>

// Game
namespace Game {
	template<typename T>
	void ReadListNode(const pugi::xml_node& node, std::string_view nodeName, std::vector<T>& list) {
		for (const auto& child : node.child(nodeName.data())) {
			if (!utils::string_iequals(child.name(), "entry")) {
				continue;
			}

			decltype(auto) data = list.emplace_back();
			if constexpr (std::is_same_v<T, std::string>) {
				data = child.text().get();
			} else if constexpr (std::is_class_v<T>) {
				data.Read(child);
			} else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_enum_v<T>) {
				std::string_view nodeText = child.text().get();
				data = utils::to_number<T>(nodeText);
			}
		}
	}

	glm::vec3 ReadVec3(pugi::xml_node node) {
		glm::vec3 data;
		data.x = utils::xml_get_text_node<float>(node, "x");
		data.y = utils::xml_get_text_node<float>(node, "y");
		data.z = utils::xml_get_text_node<float>(node, "z");
		return data;
	}

	// BoundingBox
	BoundingBox::BoundingBox() {
		mCenter = glm::zero<glm::vec3>();
		mExtent = glm::zero<glm::vec3>();
	}

	BoundingBox::BoundingBox(const glm::vec3& min, const glm::vec3& max) {
		constexpr auto half = glm::vec3(0.5f);
		mCenter = (max + min) * half;
		mExtent = (max - min) * half;
	}

	bool BoundingBox::IsPoint() const {
		return mExtent == glm::zero<glm::vec3>();
	}

	bool BoundingBox::Contains(const glm::vec3& point) const {
		auto distance = glm::abs(mCenter - point);
		return glm::all(glm::lessThan(distance, mExtent));
	}

	bool BoundingBox::Contains(const BoundingBox& boundingBox) const {
		auto delta = mExtent - boundingBox.mExtent;
		if (glm::any(glm::lessThan(delta, glm::zero<glm::vec3>()))) {
			return false;
		}
		return Contains(boundingBox.mCenter);
	}

	bool BoundingBox::Intersects(const BoundingBox& boundingBox) const {
		auto distance = glm::abs(mCenter - boundingBox.mCenter);
		auto distanceRequired = mExtent + boundingBox.mExtent;
		return glm::all(glm::lessThanEqual(distance, distanceRequired));
	}

	bool BoundingBox::Touches(const BoundingBox& boundingBox) const {
		auto distance = glm::abs(mCenter - boundingBox.mCenter);
		auto distanceRequired = mExtent + boundingBox.mExtent;

		auto results = glm::equal(distance, distanceRequired);
		return (results.x + results.y + results.z) >= 2;
	}

	void BoundingBox::Transform(const glm::mat4& matrix) {
		glm::vec3 min = GetMin();
		glm::vec3 max = GetMax();

		glm::vec3 xa = glm::vec3(matrix[0] * min.x);
		glm::vec3 xb = glm::vec3(matrix[0] * max.x);
		glm::vec3 xMin = glm::min(xa, xb);
		glm::vec3 xMax = glm::max(xa, xb);

		glm::vec3 ya = glm::vec3(matrix[1] * min.y);
		glm::vec3 yb = glm::vec3(matrix[1] * max.y);
		glm::vec3 yMin = glm::min(ya, yb);
		glm::vec3 yMax = glm::max(ya, yb);

		glm::vec3 za = glm::vec3(matrix[2] * min.z);
		glm::vec3 zb = glm::vec3(matrix[2] * max.z);
		glm::vec3 zMin = glm::min(za, zb);
		glm::vec3 zMax = glm::max(za, zb);

		glm::vec3 newMin = xMin + yMin + zMin + glm::vec3(matrix[3]);
		glm::vec3 newMax = xMax + yMax + zMax + glm::vec3(matrix[3]);

		*this = BoundingBox(newMin, newMax);
	}

	glm::vec3 BoundingBox::GetMin() const {
		return mCenter - mExtent;
	}

	glm::vec3 BoundingBox::GetMax() const {
		return mCenter + mExtent;
	}

	glm::vec3 BoundingBox::GetSize() const {
		return mExtent * 2.0f;
	}

	// ClassAttributes
	void ClassAttributes::Read(pugi::xml_node node) {
		mBaseAttributes[ClassAttribute::Health] = utils::xml_get_text_node<float>(node, "baseHealth");
		mBaseAttributes[ClassAttribute::Mana] = utils::xml_get_text_node<float>(node, "baseMana");
		mBaseAttributes[ClassAttribute::Strength] = utils::xml_get_text_node<float>(node, "baseStrength");
		mBaseAttributes[ClassAttribute::Dexterity] = utils::xml_get_text_node<float>(node, "baseDexterity");
		mBaseAttributes[ClassAttribute::Mind] = utils::xml_get_text_node<float>(node, "baseMind");
		mBaseAttributes[ClassAttribute::PhysicalDefense] = utils::xml_get_text_node<float>(node, "basePhysicalDefense");
		mBaseAttributes[ClassAttribute::MagicalDefense] = utils::xml_get_text_node<float>(node, "baseMagicalDefense");
		mBaseAttributes[ClassAttribute::EnergyDefense] = utils::xml_get_text_node<float>(node, "baseEnergyDefense");
		mBaseAttributes[ClassAttribute::Critical] = utils::xml_get_text_node<float>(node, "baseCritical");
		mBaseAttributes[ClassAttribute::CombatSpeed] = utils::xml_get_text_node<float>(node, "baseCombatSpeed");
		mBaseAttributes[ClassAttribute::NonCombatSpeed] = utils::xml_get_text_node<float>(node, "baseNonCombatSpeed");
		mBaseAttributes[ClassAttribute::StealthDetection] = utils::xml_get_text_node<float>(node, "baseStealthDetection");
		mBaseAttributes[ClassAttribute::MovementSpeedBuff] = utils::xml_get_text_node<float>(node, "baseMovementSpeedBuff");

		mMaxAttributes[ClassAttribute::Health] = utils::xml_get_text_node<float>(node, "maxHealth");
		mMaxAttributes[ClassAttribute::Mana] = utils::xml_get_text_node<float>(node, "maxMana");
		mMaxAttributes[ClassAttribute::Strength] = utils::xml_get_text_node<float>(node, "maxStrength");
		mMaxAttributes[ClassAttribute::Dexterity] = utils::xml_get_text_node<float>(node, "maxDexterity");
		mMaxAttributes[ClassAttribute::Mind] = utils::xml_get_text_node<float>(node, "maxMind");
		mMaxAttributes[ClassAttribute::PhysicalDefense] = utils::xml_get_text_node<float>(node, "maxPhysicalDefense");
		mMaxAttributes[ClassAttribute::MagicalDefense] = utils::xml_get_text_node<float>(node, "maxMagicalDefense");
		mMaxAttributes[ClassAttribute::EnergyDefense] = utils::xml_get_text_node<float>(node, "maxEnergyDefense");
		mMaxAttributes[ClassAttribute::Critical] = utils::xml_get_text_node<float>(node, "maxCritical");
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
	void NpcAffix::Read(pugi::xml_node node) {
		mChildName = utils::xml_get_text_node(node, "mpChildAffix");
		mParentName = utils::xml_get_text_node(node, "mpParentAffix");
		mModifier = utils::xml_get_text_node(node, "modifier");
	}

	const std::shared_ptr<NpcAffix>& NpcAffix::GetChild() const {
		return mChild;
	}

	const std::shared_ptr<NpcAffix>& NpcAffix::GetParent() const {
		return mParent;
	}

	// EliteAffix
	void EliteAffix::Read(pugi::xml_node node) {
		mMinDifficulty = utils::xml_get_text_node<int32_t>(node, "minDifficulty");
		mMaxDifficulty = utils::xml_get_text_node<int32_t>(node, "maxDifficulty");

		auto affixName = utils::xml_get_text_node(node, "mpNPCAffix");
		if (!affixName.empty()) {
			mNpcAffix = NounDatabase::Instance().GetNpcAffix(utils::hash_id(affixName));
		}
	}

	// PlayerClass
	void PlayerClass::Read(pugi::xml_node node) {
		mName = utils::xml_get_text_node(node, "speciesName");
		mEffect = utils::xml_get_text_node(node, "mpClassEffect");

		mAbilities[0] = utils::xml_get_text_node(node, "basicAbility");
		mAbilities[1] = utils::xml_get_text_node(node, "specialAbility1");
		mAbilities[2] = utils::xml_get_text_node(node, "specialAbility2");
		mAbilities[3] = utils::xml_get_text_node(node, "specialAbility3");
		mAbilities[4] = utils::xml_get_text_node(node, "passiveAbility");

		mCreatureType = utils::xml_get_text_node<uint32_t>(node, "creatureType");
		mCreatureClass = utils::xml_get_text_node<uint32_t>(node, "creatureClass");
		mPrimaryAttribute = utils::xml_get_text_node<uint32_t>(node, "primaryAttribute");
		mUnlockLevel = utils::xml_get_text_node<int32_t>(node, "unlockLevel");

		mHomeworld = utils::xml_get_text_node<Homeworld>(node, "homeworld");

		mWeaponMinDamage = utils::xml_get_text_node<float>(node, "weaponMinDamage");
		mWeaponMaxDamage = utils::xml_get_text_node<float>(node, "weaponMaxDamage");

		mNoHands = utils::xml_get_text_node<bool>(node, "noHands");
		mNoFeet = utils::xml_get_text_node<bool>(node, "noFeet");

		auto classAttributesName = utils::xml_get_text_node(node, "mpClassAttributes");
		if (!classAttributesName.empty()) {
			mAttributes = NounDatabase::Instance().GetClassAttributes(utils::hash_id(classAttributesName));
		}
	}

	const std::shared_ptr<ClassAttributes>& PlayerClass::GetAttributes() const {
		return mAttributes;
	}

	// NonPlayerClass
	void NonPlayerClass::Read(pugi::xml_node node) {
		const auto& database = NounDatabase::Instance();
		if (auto nameNode = node.child("name")) {
			mName = utils::xml_get_text_node(nameNode, "str");
		}

		mEffect = utils::xml_get_text_node(node, "mpClassEffect");

		mCreatureType = utils::xml_get_text_node<uint32_t>(node, "creatureType");
		mNpcType = utils::xml_get_text_node<NpcType>(node, "mNPCType");

		mChallengeValue = utils::xml_get_text_node<int32_t>(node, "challengeValue");
		mNpcRank = utils::xml_get_text_node<int32_t>(node, "npcRank");

		mAggroRange = utils::xml_get_text_node<float>(node, "aggroRange");
		mAlertRange = utils::xml_get_text_node<float>(node, "alertRange");
		mDropAggroRange = utils::xml_get_text_node<float>(node, "dropAggroRange");
		mDropDelay = utils::xml_get_text_node<float>(node, "dropDelay");
		mPlayerCountHealthScale = utils::xml_get_text_node<float>(node, "playerCountHealthScale");

		mTargetable = utils::xml_get_text_node<bool>(node, "targetable");
		mIsPet = utils::xml_get_text_node<bool>(node, "playerPet");

		ReadListNode(node, "dropType", mDropTypes);
		ReadListNode(node, "eliteAffix", mEliteAffixes);

		auto classAttributesName = utils::xml_get_text_node(node, "mpClassAttributes");
		if (!classAttributesName.empty()) {
			mAttributes = database.GetClassAttributes(utils::hash_id(classAttributesName));
		}
	}

	const std::shared_ptr<ClassAttributes>& NonPlayerClass::GetAttributes() const {
		return mAttributes;
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

	/*
		noun->add("gfxStates", nullable_type, std::tuple(0x84, cGameObjectGfxStates));
		noun->add("doorDef", nullable_type, std::tuple(0x88, cDoorDef));
		noun->add("switchDef", nullable_type, std::tuple(0x8C, cSwitchDef));
		noun->add("pressureSwitchDef", nullable_type, std::tuple(0x90, cPressureSwitchDef));
		noun->add("crystalDef", nullable_type, std::tuple(0x94, CrystalDef));
		noun->add("triggerVolume", nullable_type, std::tuple(0x124, TriggerVolumeDef));
		noun->add("projectile", nullable_type, std::tuple(0x128, ProjectileDef));
		noun->add("orbit", nullable_type, std::tuple(0x12C, OrbitDef));
		noun->add("locomotionTuning", nullable_type, std::tuple(0x130, LocomotionTuning));
	*/

	// AssetProperty
	void AssetProperty::Read(pugi::xml_node node) {
		mName = utils::xml_get_text_node(node, "name");
		mValue = utils::xml_get_text_node(node, "value");

		mKey = utils::xml_get_text_node<uint32_t>(node, "key");
		mType = utils::xml_get_text_node<uint32_t>(node, "type");
	}

	// GambitDefinition
	void GambitDefinition::Read(pugi::xml_node node) {
		ReadListNode(node, "conditionProps", mConditionProperties);
		ReadListNode(node, "abilityProps", mAbilityProperties);
		
		if (const auto& value = utils::xml_get_text_node(node, "condition"); !value.empty()) {
			mCondition = utils::hash_id(value);
		}

		if (const auto& value = utils::xml_get_text_node(node, "ability"); !value.empty()) {
			mAbility = utils::hash_id(value);
		}

		mRandomizeCooldown = utils::xml_get_text_node<bool>(node, "randomizeCooldown");
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
	void Phase::Read(pugi::xml_node node) {
		ReadListNode(node, "gambit", mGambit);

		mType = utils::xml_get_text_node<PhaseType>(node, "phaseType");

		mIsStartNode = utils::xml_get_text_node<bool>(node, "startNode");
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
	void AINode::Read(pugi::xml_node node) {
		const auto& database = NounDatabase::Instance();

		ReadListNode(node, "output", mOutput);

		auto phaseName = utils::xml_get_text_node(node, "mpPhaseData");
		if (!phaseName.empty()) {
			mPhase = database.GetPhase(utils::hash_id(phaseName));
		}

		/*
		auto conditionName = utils::xml_get_text_node(node, "mpConditionData");
		if (!conditionName.empty()) {
			mCondition = database.GetCondition(utils::hash_id(conditionName));
		}
		*/

		mX = utils::xml_get_text_node<int32_t>(node, "nodeX");
		mY = utils::xml_get_text_node<int32_t>(node, "nodeY");
	}

	const std::shared_ptr<Phase>& AINode::GetPhaseData() const {
		return mPhase;
	}

	const std::shared_ptr<Condition>& AINode::GetConditionData() const {
		return mCondition;
	}

	// AIDefinition
	void AIDefinition::Read(pugi::xml_node node) {
		ReadListNode(node, "ainode", mNodes);

		// Ability
		if (const auto& value = utils::xml_get_text_node(node, "deathAbility"); !value.empty()) { mDeathAbility = utils::hash_id(value); }
		if (const auto& value = utils::xml_get_text_node(node, "deathCondition"); !value.empty()) { mDeathCondition = utils::hash_id(value); }
		if (const auto& value = utils::xml_get_text_node(node, "firstAggroAbility"); !value.empty()) { mFirstAggroAbility = utils::hash_id(value); }
		if (const auto& value = utils::xml_get_text_node(node, "firstAggroAbility2"); !value.empty()) { mSecondaryFirstAggroAbility = utils::hash_id(value); }
		if (const auto& value = utils::xml_get_text_node(node, "firstAlertAbility"); !value.empty()) { mFirstAlertAbility = utils::hash_id(value); }
		if (const auto& value = utils::xml_get_text_node(node, "subsequentAggroAbility"); !value.empty()) { mSubsequentAggroAbility = utils::hash_id(value); }
		if (const auto& value = utils::xml_get_text_node(node, "passiveAbility"); !value.empty()) { mPassiveAbility = utils::hash_id(value); }

		// Behavior
		mCombatIdle = utils::xml_get_text_node(node, "combatIdle");
		mSecondaryCombatIdle = utils::xml_get_text_node(node, "combatIdle2");
		mSecondaryCombatIdleCondition = utils::xml_get_text_node(node, "combatIdle2Condition");
		mPassiveIdle = utils::xml_get_text_node(node, "passiveIdle");
		mPreAggroIdle = utils::xml_get_text_node(node, "preAggroIdle");
		mSecondaryPreAggroIdle = utils::xml_get_text_node(node, "preAggroIdle2");
		mTargetTooFar = utils::xml_get_text_node(node, "targetTooFar");

		mAggroType = utils::xml_get_text_node<uint32_t>(node, "aggroType");
		mCombatIdleCooldown = utils::xml_get_text_node<uint32_t>(node, "combatIdleCooldown");
		mSecondaryCombatIdleCooldown = utils::xml_get_text_node<uint32_t>(node, "combatIdle2Cooldown");
		mTargetTooFarCooldown = utils::xml_get_text_node<uint32_t>(node, "targetTooFarCooldown");

		mUseSecondaryStart = utils::xml_get_text_node<float>(node, "useSecondaryStart");

		mFaceTarget = utils::xml_get_text_node<bool>(node, "faceTarget");
		mAlwaysRun = utils::xml_get_text_node<bool>(node, "alwaysRunAI");
		mRandomizeCooldowns = utils::xml_get_text_node<bool>(node, "randomizeCooldowns");
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
	void CharacterAnimation::Read(pugi::xml_node node) {
		// TODO: Change this into constexpr data + loop
		/*
			CharacterAnimation->add("gaitOverlay", uint32_t_type, 0x050);
			CharacterAnimation->add("overrideGait", char_type, std::tuple(0x000, 0x50));
			CharacterAnimation->add("ignoreGait", bool_type, 0x054);
			CharacterAnimation->add("morphology", key_type, 0x064);
		*/
		if (auto value = utils::xml_get_text_node(node, "preAggroIdleAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::PreAggro)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "idleAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::Idle)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "lobbyIdleAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::LobbyIdle)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "specialIdleAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::SpecialIdle)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "walkStopState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::WalkStop)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "victoryIdleAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::VictoryIdle)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "combatIdleAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::CombatIdle)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "moveAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::Move)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "combatMoveAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::CombatMove)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "deathAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::Death)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "aggroAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::Aggro)] = AnimationData(
				utils::hash_id(value),
				utils::xml_get_text_node<float>(node, "aggroAnimDuration")
			);
		}

		if (auto value = utils::xml_get_text_node(node, "subsequentAggroAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::SubsequentAggro)] = AnimationData(
				utils::hash_id(value),
				utils::xml_get_text_node<float>(node, "subsequentAggroAnimDuration")
			);
		}

		if (auto value = utils::xml_get_text_node(node, "enterPassiveIdleAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::EnterPassiveIdle)] = AnimationData(
				utils::hash_id(value),
				utils::xml_get_text_node<float>(node, "enterPassiveIdleAnimDuration")
			);
		}

		if (auto value = utils::xml_get_text_node(node, "danceEmoteAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::Dance)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "tauntEmoteAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::Taunt)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "meleeDeathAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::MeleeDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "meleeCritDeathAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::MeleeCriticalDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "meleeCritKnockbackDeathAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::MeleeCriticalKnockbackDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "cyberCritDeathAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::CyberCriticalDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "cyberCritKnockbackDeathAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::CyberCriticalKnockbackDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "plasmaCritDeathAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::PlasmaCriticalDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "plasmaCritKnockbackDeathAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::PlasmaCriticalKnockbackDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "bioCritDeathAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::BioCriticalDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "bioCritKnockbackDeathAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::BioCriticalKnockbackDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "necroCritDeathAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::NecroCriticalDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "necroCritKnockbackDeathAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::NecroCriticalKnockbackDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "spacetimeCritDeathAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::SpacetimeCriticalDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "spacetimeCritKnockbackDeathAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::SpacetimeCriticalKnockbackDeath)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "bodyFadeAnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::BodyFade)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "randomAbility1AnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::RandomAbility1)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "randomAbility2AnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::RandomAbility2)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "randomAbility3AnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::RandomAbility3)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "overlay1AnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::Overlay1)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "overlay2AnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::Overlay2)] = AnimationData(utils::hash_id(value), 0.f);
		}

		if (auto value = utils::xml_get_text_node(node, "overlay3AnimState"); !value.empty()) {
			mAnimationData[utils::to_underlying(CharacterAnimationType::Overlay3)] = AnimationData(utils::hash_id(value), 0.f);
		}
	}

	AnimationData CharacterAnimation::GetAnimationData(CharacterAnimationType type) const {
		auto index = utils::to_underlying(type);
		if (index < mAnimationData.size()) {
			return mAnimationData[index];
		}
		return AnimationData(0, 0.f);
	}

	// Noun
	void Noun::Read(pugi::xml_node node) {
		const auto& database = NounDatabase::Instance();
		if (node.child("nounType")) {
			// Directly parsed from .Noun files
			mType = utils::xml_get_text_node<NounType>(node, "nounType");
			mPresetExtents = utils::xml_get_text_node<PresetExtents>(node, "presetExtents");

			if (auto bboxNode = node.child("bbox")) {
				mBoundingBox = BoundingBox(
					ReadVec3(bboxNode.child("min")),
					ReadVec3(bboxNode.child("max"))
				);
			}

			auto nonPlayerClassName = utils::xml_get_text_node(node, "npcClassData");
			if (!nonPlayerClassName.empty()) {
				mNpcClassData = database.GetNonPlayerClass(utils::hash_id(nonPlayerClassName));
			}

			auto playerClassName = utils::xml_get_text_node(node, "playerClassData");
			if (!playerClassName.empty()) {
				mPlayerClassData = database.GetPlayerClass(utils::hash_id(playerClassName));
			}

			auto aidefinitionName = utils::xml_get_text_node(node, "aiDefinition");
			if (!aidefinitionName.empty()) {
				mAIDefinition = database.GetAIDefinition(utils::hash_id(aidefinitionName));
			}

			auto characterAnimationName = utils::xml_get_text_node(node, "characterAnimationData");
			if (!characterAnimationName.empty()) {
				mCharacterAnimation = database.GetCharacterAnimation(utils::hash_id(characterAnimationName));
			}

			mAssetId = utils::xml_get_text_node<uint64_t>(node, "assetId");
			ReadListNode(node, "eliteAssetIds", mEliteAssetIds);

			mIsFlora = utils::xml_get_text_node<bool>(node, "isFlora");
			mIsMineral = utils::xml_get_text_node<bool>(node, "isMineral");
			mIsCreature = utils::xml_get_text_node<bool>(node, "isCreature");
			mIsPlayer = utils::xml_get_text_node<bool>(node, "isPlayer");
			mIsSpawned = utils::xml_get_text_node<bool>(node, "isSpawned");
		} else {
			// Shortened versions
		}
	}

	const BoundingBox& Noun::GetBoundingBox() const {
		return mBoundingBox;
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

	NounType Noun::GetType() const {
		return mType;
	}

	PresetExtents Noun::GetPresetExtents() const {
		return mPresetExtents;
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
		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			const auto& path = entry.path();
			if (entry.is_regular_file() && path.extension() == ".xml") {
				pugi::xml_document document;
				if (auto parse_result = document.load_file(path.c_str())) {
					if (auto rootNode = document.child("noun")) {
						auto noun = std::make_shared<Noun>();
						noun->Read(rootNode);

						auto name = path.stem().string();
						noun->mId = utils::hash_id(name);
						noun->mName = name;

						mNouns.try_emplace(noun->mId, noun);
					}
				} else {
					std::cout << "NounDatabase: Could not load '" << path << "'." << std::endl;
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
		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			const auto& path = entry.path();
			if (entry.is_regular_file() && path.extension() == ".xml") {
				pugi::xml_document document;
				if (auto parse_result = document.load_file(path.c_str())) {
					auto data = std::make_shared<NonPlayerClass>();
					if (auto rootNode = document.child("nonplayerclass")) {
						data->Read(rootNode);

						auto name = path.stem().string();
						data->mId = utils::hash_id(name);
						data->mPath = name;

						mNonPlayerClasses.try_emplace(data->mId, data);
					}
				} else {
					std::cout << "NounDatabase: Could not load '" << path << "'." << std::endl;
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
		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			const auto& path = entry.path();
			if (entry.is_regular_file() && path.extension() == ".xml") {
				pugi::xml_document document;
				if (auto parse_result = document.load_file(path.c_str())) {
					auto data = std::make_shared<PlayerClass>();
					if (auto rootNode = document.child("playerclass")) {
						data->Read(rootNode);

						auto name = path.stem().string();
						data->mId = utils::hash_id(name);
						data->mPath = name;

						mPlayerClasses.try_emplace(data->mId, data);
					}
				} else {
					std::cout << "NounDatabase: Could not load '" << path << "'." << std::endl;
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
		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			const auto& path = entry.path();
			if (entry.is_regular_file() && path.extension() == ".xml") {
				pugi::xml_document document;
				if (auto parse_result = document.load_file(path.c_str())) {
					auto data = std::make_shared<NpcAffix>();
					if (auto rootNode = document.child("npcaffix")) {
						data->Read(rootNode);

						auto name = path.stem().string();
						data->mId = utils::hash_id(name);
						data->mPath = name;

						mNpcAffixes.try_emplace(data->mId, data);
					}
				} else {
					std::cout << "NounDatabase: Could not load '" << path << "'." << std::endl;
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
		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			const auto& path = entry.path();
			if (entry.is_regular_file() && path.extension() == ".xml") {
				pugi::xml_document document;
				if (auto parse_result = document.load_file(path.c_str())) {
					auto data = std::make_shared<ClassAttributes>();
					if (auto rootNode = document.child("classattributes")) {
						data->Read(rootNode);

						auto name = path.stem().string();
						data->mId = utils::hash_id(name);
						data->mPath = name;

						mClassAttributes.try_emplace(data->mId, data);
					}
				} else {
					std::cout << "NounDatabase: Could not load '" << path << "'." << std::endl;
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
		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			const auto& path = entry.path();
			if (entry.is_regular_file() && path.extension() == ".xml") {
				pugi::xml_document document;
				if (auto parse_result = document.load_file(path.c_str())) {
					auto data = std::make_shared<AIDefinition>();
					if (auto rootNode = document.child("aidefinition")) {
						data->Read(rootNode);

						auto name = path.stem().string();
						data->mId = utils::hash_id(name);
						data->mPath = name;

						mAIDefinitions.try_emplace(data->mId, data);
					}
				} else {
					std::cout << "NounDatabase: Could not load '" << path << "'." << std::endl;
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
		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			const auto& path = entry.path();
			if (entry.is_regular_file() && path.extension() == ".xml") {
				pugi::xml_document document;
				if (auto parse_result = document.load_file(path.c_str())) {
					auto data = std::make_shared<CharacterAnimation>();
					if (auto rootNode = document.child("characteranimation")) {
						data->Read(rootNode);

						auto name = path.stem().string();
						data->mId = utils::hash_id(name);
						data->mPath = name;

						mCharacterAnimations.try_emplace(data->mId, data);
					}
				} else {
					std::cout << "NounDatabase: Could not load '" << path << "'." << std::endl;
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
		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			const auto& path = entry.path();
			if (entry.is_regular_file() && path.extension() == ".xml") {
				pugi::xml_document document;
				if (auto parse_result = document.load_file(path.c_str())) {
					auto data = std::make_shared<Phase>();
					if (auto rootNode = document.child("phase")) {
						data->Read(rootNode);

						auto name = path.stem().string();
						data->mId = utils::hash_id(name);
						data->mPath = name;

						mPhases.try_emplace(data->mId, data);
					}
				} else {
					std::cout << "NounDatabase: Could not load '" << path << "'." << std::endl;
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
