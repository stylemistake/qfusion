#ifndef QFUSION_AI_BASE_ENEMY_POOL_H
#define QFUSION_AI_BASE_ENEMY_POOL_H

#include "ai_frame_aware_updatable.h"
#include "static_deque.h"
#include "static_vector.h"
#include "vec3.h"
#include "../../gameshared/q_comref.h"
#include <limits>

template <int Weapon>
struct WeaponAmmo {
	static constexpr int strongAmmoTag = AMMO_NONE;
	static constexpr int weakAmmoTag = AMMO_NONE;
};

template<>
struct WeaponAmmo<WEAP_NONE>{
	static constexpr int strongAmmoTag = AMMO_NONE;
	static constexpr int weakAmmoTag = AMMO_NONE;
};

template<>
struct WeaponAmmo<WEAP_GUNBLADE>{
	static constexpr int strongAmmoTag = AMMO_GUNBLADE;
	static constexpr int weakAmmoTag = AMMO_WEAK_GUNBLADE;
};

template<>
struct WeaponAmmo<WEAP_RIOTGUN>{
	static constexpr int strongAmmoTag = AMMO_SHELLS;
	static constexpr int weakAmmoTag = AMMO_WEAK_SHELLS;
};

template<>
struct WeaponAmmo<WEAP_GRENADELAUNCHER>{
	static constexpr int strongAmmoTag = AMMO_GRENADES;
	static constexpr int weakAmmoTag = AMMO_WEAK_GRENADES;
};

template<>
struct WeaponAmmo<WEAP_ROCKETLAUNCHER>{
	static constexpr int strongAmmoTag = AMMO_ROCKETS;
	static constexpr int weakAmmoTag = AMMO_WEAK_ROCKETS;
};

template<>
struct WeaponAmmo<WEAP_PLASMAGUN>{
	static constexpr int strongAmmoTag = AMMO_PLASMA;
	static constexpr int weakAmmoTag = AMMO_WEAK_PLASMA;
};

template<>
struct WeaponAmmo<WEAP_LASERGUN>{
	static constexpr int strongAmmoTag = AMMO_LASERS;
	static constexpr int weakAmmoTag = AMMO_WEAK_LASERS;
};

template<>
struct WeaponAmmo<WEAP_MACHINEGUN>{
	static constexpr int strongAmmoTag = AMMO_BULLETS;
	static constexpr int weakAmmoTag = AMMO_WEAK_BULLETS;
};

template<>
struct WeaponAmmo<WEAP_ELECTROBOLT>{
	static constexpr int strongAmmoTag = AMMO_BOLTS;
	static constexpr int weakAmmoTag = AMMO_WEAK_BOLTS;
};

inline bool HasQuad( const edict_t *ent ) {
	return ent && ent->r.client && ent->r.client->ps.inventory[POWERUP_QUAD];
}

inline bool HasShell( const edict_t *ent ) {
	return ent && ent->r.client && ent->r.client->ps.inventory[POWERUP_SHELL];
}

inline bool HasPowerups( const edict_t *ent ) {
	if( !ent || !ent->r.client ) {
		return false;
	}
	return ent->r.client->ps.inventory[POWERUP_QUAD] && ent->r.client->ps.inventory[POWERUP_SHELL];
}

inline bool IsCarrier( const edict_t *ent ) {
	return ent && ent->r.client && ent->s.effects & EF_CARRIER;
}

float DamageToKill( const edict_t *ent, float armorProtection, float armorDegradation );

class Enemy
{
public:
	Enemy() : ent( nullptr ), lastSeenPosition( NAN, NAN, NAN ), lastSeenVelocity( NAN, NAN, NAN ) {
		Clear();
	}

	const edict_t *ent;  // If null, the enemy slot is unused

	static constexpr unsigned MAX_TRACKED_POSITIONS = 16;

	float weight;
	float avgPositiveWeight;
	float maxPositiveWeight;
	unsigned positiveWeightsCount;

	unsigned registeredAt;

	void Clear();
	void OnViewed();

	inline const char *Nick() const {
		if( !ent ) {
			return "???";
		}
		return ent->r.client ? ent->r.client->netname : ent->classname;
	}

	inline bool HasQuad() const { return ::HasQuad( ent ); }
	inline bool HasShell() const { return ::HasShell( ent ); }
	inline bool HasPowerups() const { return ::HasPowerups( ent ); }
	inline bool IsCarrier() const { return ::IsCarrier( ent ); }

	template<int Weapon>
	inline int AmmoReadyToFireCount() const {
		if( !ent->r.client ) {
			return 0;
		}
		const int *inventory = ent->r.client->ps.inventory;
		if( !inventory[Weapon] ) {
			return 0;
		}
		return inventory[WeaponAmmo < Weapon > ::strongAmmoTag] + inventory[WeaponAmmo < Weapon > ::weakAmmoTag];
	}

	inline int ShellsReadyToFireCount() const { return AmmoReadyToFireCount<WEAP_RIOTGUN>(); }
	inline int GrenadesReadyToFireCount() const { return AmmoReadyToFireCount<WEAP_GRENADELAUNCHER>(); }
	inline int RocketsReadyToFireCount() const { return AmmoReadyToFireCount<WEAP_ROCKETLAUNCHER>(); }
	inline int PlasmasReadyToFireCount() const { return AmmoReadyToFireCount<WEAP_PLASMAGUN>(); }
	inline int BulletsReadyToFireCount() const { return AmmoReadyToFireCount<WEAP_MACHINEGUN>(); }
	inline int LasersReadyToFireCount() const { return AmmoReadyToFireCount<WEAP_LASERGUN>(); }
	inline int BoltsReadyToFireCount() const { return AmmoReadyToFireCount<WEAP_ELECTROBOLT>(); }

	inline int PendingWeapon() const {
		// TODO: It does not check ammo
		return ent->r.client ? ent->r.client->ps.stats[STAT_PENDING_WEAPON] : WEAP_NONE;
	}

	inline unsigned LastSeenAt() const { return lastSeenAt; }
	inline const Vec3 &LastSeenPosition() const { return lastSeenPosition; }
	inline const Vec3 &LastSeenVelocity() const { return lastSeenVelocity; }

	inline bool IsValid() const { return ent != nullptr; }

	inline Vec3 LookDir() const {
		vec3_t forward;
		AngleVectors( ent->s.angles, forward, nullptr, nullptr );
		return Vec3( forward );
	}

	inline Vec3 Angles() const { return Vec3( ent->s.angles ); }

	// TODO: Fuse in a single array of some struct
	// Array of last seen timestamps
	StaticDeque<unsigned, MAX_TRACKED_POSITIONS> lastSeenTimestamps;
	// Array of last seen positions
	StaticDeque<Vec3, MAX_TRACKED_POSITIONS> lastSeenPositions;
	// Array of last seen enemy velocities
	StaticDeque<Vec3, MAX_TRACKED_POSITIONS> lastSeenVelocities;

private:
	// Same as front() of lastSeenPositions, used for faster access
	Vec3 lastSeenPosition;
	// Same as front() of lastSeenVelocities, used for faster access
	Vec3 lastSeenVelocity;
	// Same as front() of lastSeenTimestamps, used for faster access
	unsigned lastSeenAt;
};

class AttackStats
{
	friend class AiBaseEnemyPool;

	// Very close to 4 game seconds
	static constexpr unsigned MAX_KEPT_FRAMES = 64 * 4;

	static_assert( ( MAX_KEPT_FRAMES & ( MAX_KEPT_FRAMES - 1 ) ) == 0, "Should be a power of 2 for fast modulo computation" );

	float frameDamages[MAX_KEPT_FRAMES];

	unsigned frameIndex;
	unsigned totalAttacks;
	unsigned lastDamageAt;
	unsigned lastTouchAt;
	float totalDamage;

	const edict_t *ent;

	AttackStats() { Clear(); }

public:
	void Clear() {
		ent = nullptr;
		totalDamage = 0;
		totalAttacks = 0;
		lastDamageAt = 0;
		lastTouchAt = level.time;
		frameIndex = 0;
		memset( frameDamages, 0, sizeof( frameDamages ) );
	}

	// Call it once in a game frame
	void Frame() {
		float overwrittenDamage = frameDamages[frameIndex];
		frameIndex = ( frameIndex + 1 ) % MAX_KEPT_FRAMES;
		totalDamage -= overwrittenDamage;
		frameDamages[frameIndex] = 0.0f;
		if( overwrittenDamage > 0 ) {
			totalAttacks--;
		}
	}

	// Call it after Frame() in the same frame
	void OnDamage( float damage ) {
		frameDamages[frameIndex] = damage;
		totalDamage += damage;
		totalAttacks++;
		lastDamageAt = level.time;
	}

	// Call it after Frame() in the same frame if damage is not registered
	// but you want to mark frame as a frame of activity anyway
	void Touch() { lastTouchAt = level.time; }

	unsigned LastActivityAt() const { return std::max( lastDamageAt, lastTouchAt ); }
};

class AiBaseEnemyPool : public AiFrameAwareUpdatable
{
public:
	static constexpr unsigned MAX_TRACKED_ENEMIES = 10;
	static constexpr unsigned MAX_TRACKED_ATTACKERS = 5;
	static constexpr unsigned MAX_TRACKED_TARGETS = 5;
	// Ensure we always will have at least 3 free slots for new enemies
	// (quad/shell owners and carrier) FOR MAXIMAL SKILL
	static_assert( MAX_TRACKED_ATTACKERS + 3 <= MAX_TRACKED_ENEMIES, "Leave at least 3 free slots for ordinary enemies" );
	static_assert( MAX_TRACKED_TARGETS + 3 <= MAX_TRACKED_ENEMIES, "Leave at least 3 free slots for ordinary enemies" );

	static constexpr unsigned NOT_SEEN_TIMEOUT = 4000;
	static constexpr unsigned ATTACKER_TIMEOUT = 3000;
	static constexpr unsigned TARGET_TIMEOUT = 3000;

	static constexpr unsigned MAX_ACTIVE_ENEMIES = 3;

private:
	float avgSkill; // (0..1)
	float decisionRandom; // [0, 1]
	unsigned decisionRandomUpdateAt;

	// All known (viewed and not forgotten) enemies
	Enemy trackedEnemies[MAX_TRACKED_ENEMIES];
	// Active enemies (potential targets) in order of importance
	StaticVector<Enemy *, MAX_ACTIVE_ENEMIES> activeEnemies;
	// Scores of active enemies updated during enemies assignation
	// (a single vector of pairs (enemy, score) can't be used
	// because external users need a vector of enemies only)
	StaticVector<float, MAX_ACTIVE_ENEMIES> activeEnemiesScores;

	unsigned trackedEnemiesCount;
	const unsigned maxTrackedEnemies;
	const unsigned maxTrackedAttackers;
	const unsigned maxTrackedTargets;
	const unsigned maxActiveEnemies;

	const unsigned reactionTime;

	unsigned prevThinkLevelTime;

	StaticVector<AttackStats, MAX_TRACKED_ATTACKERS> attackers;
	StaticVector<AttackStats, MAX_TRACKED_TARGETS> targets;

	void EmplaceEnemy( const edict_t *enemy, int slot );
	void RemoveEnemy( Enemy &enemy );

	void UpdateEnemyWeight( Enemy &enemy );
	float ComputeRawEnemyWeight( const edict_t *enemy );

	// Returns attacker slot number
	int EnqueueAttacker( const edict_t *attacker, int damage );

	// Precache results of virtual Check* calls in these vars in PreThink()
	bool hasQuad;
	bool hasShell;
	float damageToBeKilled;

protected:
	virtual void OnNewThreat( const edict_t *newThreat ) = 0;
	virtual bool CheckHasQuad() const = 0;
	virtual bool CheckHasShell() const = 0;
	virtual void OnEnemyRemoved( const Enemy *enemy ) = 0;
	// Used to compare enemy strength and pool owner
	virtual float ComputeDamageToBeKilled() const = 0;
	// Contains enemy eviction code
	virtual void TryPushNewEnemy( const edict_t *enemy ) = 0;
	// Overridden method may give some additional weight to an enemy
	// (Useful for case when a bot should have some reinforcements)
	virtual float GetAdditionalEnemyWeight( const edict_t *bot, const edict_t *enemy ) const = 0;
	virtual void OnBotEnemyAssigned( const edict_t *bot, const Enemy *enemy ) = 0;

	inline bool HasQuad() const { return hasQuad; }
	inline bool HasShell() const { return hasShell; }

	inline float DamageToBeKilled() const { return damageToBeKilled; }

	inline static float DamageToKill( const edict_t *ent ) {
		return ::DamageToKill( ent, g_armor_protection->value, g_armor_degradation->value );
	}

	inline float AvgSkill() const { return avgSkill; }
	inline float DecisionRandom() const { return decisionRandom; }

	void TryPushEnemyOfSingleBot( const edict_t *bot, const edict_t *enemy );

public:
	AiBaseEnemyPool( float avgSkill_ );
	virtual ~AiBaseEnemyPool() {}

	// If a weight is set > 0, this bot requires reinforcements
	virtual void SetBotRoleWeight( const edict_t *bot, float weight ) = 0;

	inline unsigned MaxTrackedEnemies() const { return maxTrackedEnemies; }
	// Note that enemies in this array should be validated by IsValid() before access to their properties
	inline const Enemy *TrackedEnemiesBuffer() const { return trackedEnemies; };
	inline unsigned TrackedEnemiesBufferSize() const { return maxTrackedEnemies; }
	inline const StaticVector<Enemy*, MAX_ACTIVE_ENEMIES> &ActiveEnemies() const { return activeEnemies; };

	virtual void Frame() override;

	virtual void PreThink() override;
	virtual void Think() override;
	virtual void PostThink() override {
		prevThinkLevelTime = level.time;
	}

	void OnEnemyViewed( const edict_t *enemy );

	// Force the pool to forget the enemy (for example, when bot attitude to an enemy has been changed)
	void Forget( const edict_t *enemy );

	bool WillAssignAimEnemy() const;

	// Note that these methods modify this object state
	const Enemy *ChooseAimEnemy( const edict_t *challenger );
	const Enemy *ChooseHiddenEnemy( const edict_t *challenger );

	void OnPain( const edict_t *bot, const edict_t *enemy, float kick, int damage );
	void OnEnemyDamaged( const edict_t *bot, const edict_t *target, int damage );

	void EnqueueTarget( const edict_t *target );

	// Returns zero if ent not found
	unsigned LastAttackedByTime( const edict_t *ent ) const;
	unsigned LastTargetTime( const edict_t *ent ) const;
};

#endif
