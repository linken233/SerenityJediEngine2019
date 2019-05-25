/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/


/// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// ///
///																																///
///																																///
///													SERENITY JEDI ENGINE														///
///										          LIGHTSABER COMBAT SYSTEM													    ///
///																																///
///						      System designed by Serenity and modded by JaceSolaris. (c) 2019 SJE   		                    ///
///								    https://www.moddb.com/mods/serenityjediengine-20											///
///																																///
/// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// ///



// g_combat.c

#include "b_local.h"
#include "bg_saga.h"
#include "g_dynmusic.h"

extern int G_ShipSurfaceForSurfName(const char* surfaceName);
extern qboolean G_FlyVehicleDestroySurface(gentity_t* veh, int surface);
extern void G_VehicleSetDamageLocFlags(gentity_t* veh, int impactDir, int deathPoint);
extern void G_VehUpdateShields(gentity_t* targ);
extern void G_LetGoOfWall(gentity_t* ent);
extern void BG_ClearRocketLock(playerState_t* ps);
void BotDamageNotification(gclient_t* bot, gentity_t* attacker);
extern qboolean Jedi_StopKnockdown(gentity_t* self, gentity_t* pusher, const vec3_t pushDir);
extern qboolean Boba_StopKnockdown(gentity_t* self, gentity_t* pusher, const vec3_t pushDir, qboolean forceKnockdown);
extern void G_AddVoiceEvent(gentity_t* self, int event, int speakDebounceTime);
extern qboolean PM_LockedAnim(int anim);
extern qboolean PM_LightningBlockAnim(int anim);
extern void NPC_SetPainEvent(gentity_t* self);
extern qboolean PM_InKnockDown(playerState_t* ps);
extern qboolean PM_RollingAnim(int anim);
extern qboolean PM_CrouchAnim(int anim);
extern qboolean BG_KnockDownAnim(int anim);
extern void ScalePlayer(gentity_t* self, int scale);
extern qboolean PM_InAnimForSaberMove(int anim, int saberMove);
extern qboolean PM_SaberInStart(int move);
extern int PM_AnimLength(int index, animNumber_t anim);
extern qboolean PM_SaberInReturn(int move);
extern int PM_PowerLevelForSaberAnims(playerState_t* ps);
extern void AddFatigueHurtBonus(gentity_t* attacker, gentity_t* victim);
extern void AddFatigueHurtBonusMax(gentity_t* attacker, gentity_t* victim);
extern void G_ATSTCheckPain(gentity_t* self, gentity_t* other, int damage);
qboolean PM_RunningAnim(int anim);
void ThrowSaberToAttacker(gentity_t* self, gentity_t* attacker);
extern int Manual_Saberblocking(gentity_t* defender);
extern void BG_ReduceBlasterMishapLevel(playerState_t* ps);

extern void SP_item_security_key(gentity_t* self);
void G_DropKey(gentity_t* self)
{//drop whatever security key I was holding
	gentity_t* dropped = G_Spawn();

	G_SetOrigin(dropped, self->client->ps.origin);

	//set up the ent
	SP_item_security_key(self);

	//Don't throw the key
	VectorClear(dropped->s.pos.trDelta);
	dropped->message = self->message;
	self->message = NULL;
}

void ObjectDie(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, int meansOfDeath)
{
	if (self->target)
	{
		G_UseTargets(self, attacker);
	}

	//remove my script_targetname
	G_FreeEntity(self);
}

qboolean G_HeavyMelee(gentity_t* attacker)
{
	if (level.gametype == GT_SIEGE
		&& attacker
		&& attacker->client
		&& attacker->client->siegeClass != -1
		&& (bgSiegeClasses[attacker->client->siegeClass].classflags & (1 << CFL_HEAVYMELEE)))
	{
		return qtrue;
	}
	return qfalse;
}

int G_GetHitLocation(gentity_t* target, vec3_t ppoint)
{
	vec3_t			point, point_dir;
	vec3_t			forward, right, up;
	vec3_t			tangles, tcenter;
	float			udot, fdot, rdot;
	int				Vertical, Forward, Lateral;
	int				HitLoc;

	// Get target forward, right and up.
	if (target->client)
	{// Ignore player's pitch and roll.
		VectorSet(tangles, 0, target->r.currentAngles[YAW], 0);
	}

	AngleVectors(tangles, forward, right, up);

	// Get center of target.
	VectorAdd(target->r.absmin, target->r.absmax, tcenter);
	VectorScale(tcenter, 0.5, tcenter);

	// Get impact point.
	if (ppoint && !VectorCompare(ppoint, vec3_origin))
	{
		VectorCopy(ppoint, point);
	}
	else
	{
		return HL_NONE;
	}
	VectorSubtract(point, tcenter, point_dir);
	VectorNormalize(point_dir);

	// Get bottom to top (vertical) position index
	udot = DotProduct(up, point_dir);
	if (udot > .800)
	{
		Vertical = 4;
	}
	else if (udot > .400)
	{
		Vertical = 3;
	}
	else if (udot > -.333)
	{
		Vertical = 2;
	}
	else if (udot > -.666)
	{
		Vertical = 1;
	}
	else
	{
		Vertical = 0;
	}

	// Get back to front (forward) position index.
	fdot = DotProduct(forward, point_dir);
	if (fdot > .666)
	{
		Forward = 4;
	}
	else if (fdot > .333)
	{
		Forward = 3;
	}
	else if (fdot > -.333)
	{
		Forward = 2;
	}
	else if (fdot > -.666)
	{
		Forward = 1;
	}
	else
	{
		Forward = 0;
	}

	// Get left to right (lateral) position index.
	rdot = DotProduct(right, point_dir);
	if (rdot > .666)
	{
		Lateral = 4;
	}
	else if (rdot > .333)
	{
		Lateral = 3;
	}
	else if (rdot > -.333)
	{
		Lateral = 2;
	}
	else if (rdot > -.666)
	{
		Lateral = 1;
	}
	else
	{
		Lateral = 0;
	}

	HitLoc = Vertical * 25 + Forward * 5 + Lateral;

	if (HitLoc <= 10)
	{
		// Feet.
		if (rdot > 0)
		{
			return HL_FOOT_RT;
		}
		else
		{
			return HL_FOOT_LT;
		}
	}
	else if (HitLoc <= 50)
	{
		// Legs.
		if (rdot > 0)
		{
			return HL_LEG_RT;
		}
		else
		{
			return HL_LEG_LT;
		}
	}
	else if (HitLoc == 56 || HitLoc == 60 || HitLoc == 61 || HitLoc == 65 || HitLoc == 66 || HitLoc == 70)
	{
		// Hands.
		if (rdot > 0)
		{
			return HL_HAND_RT;
		}
		else
		{
			return HL_HAND_LT;
		}
	}
	else if (HitLoc == 83 || HitLoc == 87 || HitLoc == 88 || HitLoc == 92 || HitLoc == 93 || HitLoc == 97)
	{
		// Arms.
		if (rdot > 0)
		{
			return HL_ARM_RT;
		}
		else
		{
			return HL_ARM_LT;
		}
	}
	else if ((HitLoc >= 107 && HitLoc <= 109) || (HitLoc >= 112 && HitLoc <= 114) || (HitLoc >= 117 && HitLoc <= 119))
	{
		// Head.
		return HL_HEAD;
	}
	else
	{
		if (udot < 0.3)
		{
			return HL_WAIST;
		}
		else if (fdot < 0)
		{
			if (rdot > 0.4)
			{
				return HL_BACK_RT;
			}
			else if (rdot < -0.4)
			{
				return HL_BACK_LT;
			}
			else if (fdot < 0)
			{
				return HL_BACK;
			}
		}
		else
		{
			if (rdot > 0.3)
			{
				return HL_CHEST_RT;
			}
			else if (rdot < -0.3)
			{
				return HL_CHEST_LT;
			}
			else if (fdot < 0)
			{
				return HL_CHEST;
			}
		}
	}
	return HL_NONE;
}

int G_PickPainAnim(gentity_t* self, vec3_t point, int damage, int hitLoc)
{
	if (hitLoc == HL_NONE)
	{
		hitLoc = G_GetHitLocation(self, point);
	}
	switch (hitLoc)
	{
	case HL_FOOT_RT:
		return BOTH_PAIN12;
		break;
	case HL_FOOT_LT:
		return -1;
		break;
	case HL_LEG_RT:
		if (!Q_irand(0, 1))
		{
			return BOTH_PAIN11;
		}
		else
		{
			return BOTH_PAIN13;
		}
		break;
	case HL_LEG_LT:
		return BOTH_PAIN14;
		break;
	case HL_BACK_RT:
		return BOTH_PAIN7;
		break;
	case HL_BACK_LT:
		return Q_irand(BOTH_PAIN15, BOTH_PAIN16);
		break;
	case HL_BACK:
		if (!Q_irand(0, 1))
		{
			return BOTH_PAIN1;
		}
		else
		{
			return BOTH_PAIN5;
		}
		break;
	case HL_CHEST_RT:
		return BOTH_PAIN3;
		break;
	case HL_CHEST_LT:
		return BOTH_PAIN2;
		break;
	case HL_WAIST:
	case HL_CHEST:
		if (!Q_irand(0, 3))
		{
			return BOTH_PAIN6;
		}
		else if (!Q_irand(0, 2))
		{
			return BOTH_PAIN8;
		}
		else if (!Q_irand(0, 1))
		{
			return BOTH_PAIN17;
		}
		else
		{
			return BOTH_PAIN18;
		}
		break;
	case HL_ARM_RT:
	case HL_HAND_RT:
		return BOTH_PAIN9;
		break;
	case HL_ARM_LT:
	case HL_HAND_LT:
		return BOTH_PAIN10;
		break;
	case HL_HEAD:
		return BOTH_PAIN4;
		break;
	default:
		return -1;
		break;
	}
}

void ExplodeDeath(gentity_t* self)
{
	vec3_t		forward;

	self->takedamage = qfalse;//stop chain reaction runaway loops

	self->s.loopSound = 0;
	self->s.loopIsSoundset = qfalse;

	VectorCopy(self->r.currentOrigin, self->s.pos.trBase);

	AngleVectors(self->s.angles, forward, NULL, NULL);


	/*if ( self->fxID > 0 )
	{
		G_PlayEffect( self->fxID, self->r.currentOrigin, forward );
	}*/

	if (self->splashDamage > 0 && self->splashRadius > 0)
	{
		gentity_t* attacker = self;
		if (self->parent)
		{
			attacker = self->parent;
		}
		G_RadiusDamage(self->r.currentOrigin, attacker, self->splashDamage, self->splashRadius,
			attacker, NULL, MOD_UNKNOWN);
	}

	ObjectDie(self, self, self, 20, 0);
}


/*
============
ScorePlum
============
*/
void ScorePlum(gentity_t* ent, vec3_t origin, int score) {
	gentity_t* plum;

	plum = G_TempEntity(origin, EV_SCOREPLUM);
	// only send this temp entity to a single client
	plum->r.svFlags |= SVF_SINGLECLIENT;
	plum->r.singleClient = ent->s.number;
	//
	plum->s.otherEntityNum = ent->s.number;
	plum->s.time = score;
}

/*
============
AddScore

Adds score to both the client and his team
============
*/
extern qboolean g_dontPenalizeTeam; //g_cmds.c
void AddScore(gentity_t* ent, vec3_t origin, int score)
{

	if (!ent->client)
	{
		return;
	}
	// no scoring during pre-match warmup
	if (level.warmupTime)
	{
		return;
	}

	// show score plum
	//ScorePlum(ent, origin, score);
	//
	ent->client->ps.persistant[PERS_SCORE] += score;
	if (level.gametype == GT_TEAM && !g_dontPenalizeTeam)
		level.teamScores[ent->client->ps.persistant[PERS_TEAM]] += score;
	CalculateRanks();
}

/*
=================
TossClientItems

rww - Toss the weapon away from the player in the specified direction
=================
*/
void TossClientWeapon(gentity_t * self, vec3_t direction, float speed)
{
	vec3_t vel;
	gitem_t* item;
	gentity_t* launched;
	int weapon = self->s.weapon;
	int ammoSub;

	if (level.gametype == GT_SIEGE)
	{ //no dropping weaps
		return;
	}

	if (weapon <= WP_BRYAR_PISTOL)
	{ //can't have this
		return;
	}

	if (weapon == WP_EMPLACED_GUN ||
		weapon == WP_TURRET)
	{
		return;
	}

	// find the item type for this weapon
	item = BG_FindItemForWeapon(weapon);

	ammoSub = (self->client->ps.ammo[weaponData[weapon].ammoIndex] - bg_itemlist[BG_GetItemIndexByTag(weapon, IT_WEAPON)].quantity);

	if (ammoSub < 0)
	{
		int ammoQuan = item->quantity;
		ammoQuan -= (-ammoSub);

		if (ammoQuan <= 0)
		{ //no ammo
			return;
		}
	}

	vel[0] = direction[0] * speed;
	vel[1] = direction[1] * speed;
	vel[2] = direction[2] * speed;

	launched = LaunchItem(item, self->client->ps.origin, vel);

	launched->s.generic1 = self->s.number;
	launched->s.powerups = level.time + 1500;

	launched->count = bg_itemlist[BG_GetItemIndexByTag(weapon, IT_WEAPON)].quantity;

	self->client->ps.ammo[weaponData[weapon].ammoIndex] -= bg_itemlist[BG_GetItemIndexByTag(weapon, IT_WEAPON)].quantity;

	if (self->client->ps.ammo[weaponData[weapon].ammoIndex] < 0)
	{
		launched->count -= (-self->client->ps.ammo[weaponData[weapon].ammoIndex]);
		self->client->ps.ammo[weaponData[weapon].ammoIndex] = 0;
	}

	if ((self->client->ps.ammo[weaponData[weapon].ammoIndex] < 1 && weapon != WP_DET_PACK) ||
		(weapon != WP_THERMAL && weapon != WP_DET_PACK && weapon != WP_TRIP_MINE))
	{
		int i = 0;
		int weap = -1;

		self->client->ps.stats[STAT_WEAPONS] &= ~(1 << weapon);

		while (i < WP_NUM_WEAPONS)
		{
			if ((self->client->ps.stats[STAT_WEAPONS] & (1 << i)) && i != WP_NONE)
			{ //this one's good
				weap = i;
				break;
			}
			i++;
		}

		if (weap != -1)
		{
			self->s.weapon = weap;
			self->client->ps.weapon = weap;
		}
		else
		{
			self->s.weapon = 0;
			self->client->ps.weapon = 0;
		}

		G_AddEvent(self, EV_NOAMMO, weapon);
	}
}

/*
=================
TossClientItems

Toss the weapon and powerups for the killed player
=================
*/
extern gitem_t* BG_FindItemForAmmo(ammo_t ammo);
extern gentity_t* WP_DropThermal(gentity_t * ent);
void TossClientItems(gentity_t * self)
{
	gitem_t* item;
	int			weapon;
	float		angle;
	int			i;
	gentity_t* drop;
	gentity_t* dropped = NULL;

	if (level.gametype == GT_SIEGE)
	{ //just don't drop anything then
		return;
	}

	if (self->client->NPC_class == CLASS_SEEKER
		|| self->client->NPC_class == CLASS_REMOTE
		|| self->client->NPC_class == CLASS_SABER_DROID
		|| self->client->NPC_class == CLASS_VEHICLE
		|| self->client->NPC_class == CLASS_ATST)
	{//these NPCs don't drop items.
	 // these things are so small that they shouldn't bother throwing anything
		return;
	}
	// drop the weapon if not a gauntlet or machinegun
	weapon = self->s.weapon;

	// make a special check to see if they are changing to a new
	// weapon that isn't the mg or gauntlet.  Without this, a client
	// can pick up a weapon, be killed, and not drop the weapon because
	// their weapon change hasn't completed yet and they are still holding the MG.
	if (weapon == WP_BRYAR_PISTOL)
	{
		if (self->client->ps.weaponstate == WEAPON_DROPPING)
		{
			weapon = self->client->pers.cmd.weapon;
		}
		if (!(self->client->ps.stats[STAT_WEAPONS] & (1 << weapon)))
		{
			weapon = WP_NONE;
		}
	}

	self->s.bolt2 = weapon;

	if (weapon == WP_SABER)
	{
		//
	}
	else if (weapon == WP_BRYAR_PISTOL)
	{
		//either drop the pistol and make the pickup only give ammo or drop ammo
	}
	else if (weapon == WP_STUN_BATON || weapon == WP_MELEE)
	{
		//never drop these
	}
	else if (weapon > WP_SABER && weapon <= MAX_PLAYER_WEAPONS)
	{
		self->s.weapon = WP_NONE;

		if (weapon == WP_THERMAL && self->client->ps.torsoAnim == BOTH_ATTACK10)
		{//we were getting ready to throw the thermal, drop it! 
			self->client->ps.weaponChargeTime = level.time - FRAMETIME;//so it just kind of drops it
			dropped = WP_DropThermal(self);
			item = NULL;
		}
		else
		{// find the item type for this weapon
			item = BG_FindItemForWeapon((weapon_t)weapon);
		}
		if (item && !dropped)
		{//we have a weapon and we haven't already dropped it.
			gentity_t* te; //temp entity used to tell the clients to remove the body's			
						   // spawn the item
			dropped = Drop_Item(self, item, 0);
			//TEST: dropped items never go away
			dropped->think = NULL;
			dropped->nextthink = -1;
			//give it ammo based on how much ammo the entity had
			dropped->count = self->client->ps.ammo[weaponData[item->giTag].ammoIndex];

			self->client->ps.ammo[weaponData[item->giTag].ammoIndex] = 0;
			// tell all clients to remove the weapon model on this guy until he respawns
			te = G_TempEntity(vec3_origin, EV_DESTROY_WEAPON_MODEL);
			te->r.svFlags |= SVF_BROADCAST;
			te->s.eventParm = self->s.number;
		}
	}
	else if (self->client->NPC_class == CLASS_MARK1)
	{
		if (Q_irand(1, 2) > 1)
		{
			item = BG_FindItemForAmmo(AMMO_METAL_BOLTS);
		}
		else
		{
			item = BG_FindItemForAmmo(AMMO_BLASTER);
		}
		Drop_Item(self, item, 0);
	}
	else if (self->client->NPC_class == CLASS_MARK2)
	{
		if (Q_irand(1, 2) > 1)
		{
			item = BG_FindItemForAmmo(AMMO_METAL_BOLTS);
		}
		else
		{
			item = BG_FindItemForAmmo(AMMO_POWERCELL);
		}
		Drop_Item(self, item, 0);
	}

	// drop all the powerups if not in teamplay
	if (level.gametype != GT_TEAM && level.gametype != GT_SIEGE)
	{
		angle = 45;
		for (i = 1; i < PW_NUM_POWERUPS; i++)
		{
			if (self->client->ps.powerups[i] > level.time)
			{
				item = BG_FindItemForPowerup(i);
				if (!item) {
					continue;
				}
				drop = Drop_Item(self, item, angle);
				// decide how many seconds it has left
				drop->count = (self->client->ps.powerups[i] - level.time) / 1000;
				if (drop->count < 1)
				{
					drop->count = 1;
				}
				angle += 45;
			}
		}
	}
}



/*
==================
LookAtKiller
==================
*/
void LookAtKiller(gentity_t * self, gentity_t * inflictor, gentity_t * attacker) {
	vec3_t		dir;

	if (attacker&& attacker != self)
		VectorSubtract(attacker->s.pos.trBase, self->s.pos.trBase, dir);
	else if (inflictor&& inflictor != self)
		VectorSubtract(inflictor->s.pos.trBase, self->s.pos.trBase, dir);
	else {
		self->client->ps.stats[STAT_DEAD_YAW] = self->s.angles[YAW];
		return;
	}

	self->client->ps.stats[STAT_DEAD_YAW] = vectoyaw(dir);
}

/*
==================
GibEntity
==================
*/
void GibEntity(gentity_t * self, gentity_t * attacker)
{
	G_AddEvent(self, EV_GIB_PLAYER, 0);
	self->takedamage = qfalse;
	self->s.eType = ET_INVISIBLE;
	self->r.contents = 0;
}
void GibEntity_Headshot(gentity_t * self, gentity_t * attacker)
{
	G_AddEvent(self, EV_GIB_PLAYER_HEADSHOT, 0);
	self->client->noHead = qtrue;
}

void BodyRid(gentity_t * ent)
{
	trap->UnlinkEntity((sharedEntity_t*)ent);
	ent->physicsObject = qfalse;
}

/*
==================
body_die
==================
*/
void body_die(gentity_t * self, gentity_t * inflictor, gentity_t * attacker, int damage, int meansOfDeath) {
	// NOTENOTE No gibbing right now, this is star wars.
	qboolean doDisint = qfalse;

	if (self->s.eType == ET_NPC)
	{ //well, just rem it then, so long as it's done with its death anim and it's not a standard weapon.
		if (self->client&& self->client->ps.torsoTimer <= 0 &&
			(meansOfDeath == MOD_UNKNOWN ||
				meansOfDeath == MOD_WATER ||
				meansOfDeath == MOD_SLIME ||
				meansOfDeath == MOD_LAVA ||
				meansOfDeath == MOD_BURNING ||
				meansOfDeath == MOD_GAS ||
				meansOfDeath == MOD_CRUSH ||
				meansOfDeath == MOD_TELEFRAG ||
				meansOfDeath == MOD_FALLING ||
				meansOfDeath == MOD_SUICIDE ||
				meansOfDeath == MOD_TARGET_LASER ||
				meansOfDeath == MOD_TRIGGER_HURT))
		{
			self->think = G_FreeEntity;
			self->nextthink = level.time;
		}
		return;
	}

	if (self->health < (GIB_HEALTH + 1))
	{
		self->health = GIB_HEALTH + 1;

		if (self->client && (level.time - self->client->respawnTime) < 2000)
		{
			doDisint = qfalse;
		}
		else
		{
			doDisint = qtrue;
		}
	}

	if (self->client && (self->client->ps.eFlags & EF_DISINTEGRATION))
	{
		return;
	}
	else if (self->s.eFlags & EF_DISINTEGRATION)
	{
		return;
	}

	if (doDisint)
	{
		if (self->client)
		{
			self->client->ps.eFlags |= EF_DISINTEGRATION;
			VectorCopy(self->client->ps.origin, self->client->ps.lastHitLoc);
		}
		else
		{
			self->s.eFlags |= EF_DISINTEGRATION;
			VectorCopy(self->r.currentOrigin, self->s.origin2);

			//since it's the corpse entity, tell it to "remove" itself
			self->think = BodyRid;
			self->nextthink = level.time + 1000;
		}
		return;
	}
}


// these are just for logging, the client prints its own messages
char* modNames[MOD_MAX] = {
	"MOD_UNKNOWN",
	"MOD_STUN_BATON",
	"MOD_MELEE",
	"MOD_SABER",
	"MOD_BRYAR_PISTOL",
	"MOD_BRYAR_PISTOL_ALT",
	"MOD_BLASTER",
	"MOD_TURBLAST",
	"MOD_DISRUPTOR",
	"MOD_DISRUPTOR_SPLASH",
	"MOD_DISRUPTOR_SNIPER",
	"MOD_BOWCASTER",
	"MOD_REPEATER",
	"MOD_REPEATER_ALT",
	"MOD_REPEATER_ALT_SPLASH",
	"MOD_DEMP2",
	"MOD_DEMP2_ALT",
	"MOD_FLECHETTE",
	"MOD_FLECHETTE_ALT_SPLASH",
	"MOD_ROCKET",
	"MOD_ROCKET_SPLASH",
	"MOD_ROCKET_HOMING",
	"MOD_ROCKET_HOMING_SPLASH",
	"MOD_THERMAL",
	"MOD_THERMAL_SPLASH",
	"MOD_TRIP_MINE_SPLASH",
	"MOD_TIMED_MINE_SPLASH",
	"MOD_DET_PACK_SPLASH",
	"MOD_VEHICLE",
	"MOD_CONC",
	"MOD_CONC_ALT",
	"MOD_FORCE_DARK",
	"MOD_SENTRY",
	"MOD_WATER",
	"MOD_SLIME",
	"MOD_LAVA",
	"MOD_CRUSH",
	"MOD_TELEFRAG",
	"MOD_FALLING",
	"MOD_SUICIDE",
	"MOD_TARGET_LASER",
	"MOD_TRIGGER_HURT",
	"MOD_TEAM_CHANGE",
	"MOD_COLLISION",
	"MOD_VEH_EXPLOSION",
	"MOD_SEEKER",
	"MOD_ELECTROCUTE",
	"MOD_SPECTATE",
	"MOD_HEADSHOT",
	"MOD_BODYSHOT",
	"MOD_FORCE_LIGHTNING",
	"MOD_BURNING",
	"MOD_GAS"
};


/*
==================
CheckAlmostCapture
==================
*/
void CheckAlmostCapture(gentity_t * self, gentity_t * attacker) {
#if 0
	gentity_t* ent;
	vec3_t		dir;
	char* classname;

	// if this player was carrying a flag
	if (self->client->ps.powerups[PW_REDFLAG] ||
		self->client->ps.powerups[PW_BLUEFLAG] ||
		self->client->ps.powerups[PW_NEUTRALFLAG]) {
		// get the goal flag this player should have been going for
		if (level.gametype == GT_CTF || level.gametype == GT_CTY) {
			if (self->client->sess.sessionTeam == TEAM_BLUE) {
				classname = "team_CTF_blueflag";
			}
			else {
				classname = "team_CTF_redflag";
			}
		}
		else {
			if (self->client->sess.sessionTeam == TEAM_BLUE) {
				classname = "team_CTF_redflag";
			}
			else {
				classname = "team_CTF_blueflag";
			}
		}
		ent = NULL;
		do
		{
			ent = G_Find(ent, FOFS(classname), classname);
		} while (ent && (ent->flags & FL_DROPPED_ITEM));
		// if we found the destination flag and it's not picked up
		if (ent && !(ent->r.svFlags & SVF_NOCLIENT)) {
			// if the player was *very* close
			VectorSubtract(self->client->ps.origin, ent->s.origin, dir);
			if (VectorLength(dir) < 200) {
				self->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				if (attacker->client) {
					attacker->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				}
			}
		}
	}
#endif
}

qboolean G_InKnockDown(playerState_t * ps)
{
	switch ((ps->legsAnim))
	{
	case BOTH_KNOCKDOWN1:
	case BOTH_KNOCKDOWN2:
	case BOTH_KNOCKDOWN3:
	case BOTH_KNOCKDOWN4:
	case BOTH_KNOCKDOWN5:
	case BOTH_SLAPDOWNRIGHT:
	case BOTH_SLAPDOWNLEFT:
		return qtrue;
		break;
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
		return qtrue;
		break;
	}
	return qfalse;
}

static int G_CheckSpecialDeathAnim(gentity_t * self, vec3_t point, int damage, int mod, int hitLoc)
{
	int deathAnim = -1;

	if (BG_InRoll(&self->client->ps, self->client->ps.legsAnim))
	{
		deathAnim = BOTH_DEATH_ROLL;		//# Death anim from a roll
	}
	else if (PM_FlippingAnim(self->client->ps.legsAnim))
	{
		deathAnim = BOTH_DEATH_FLIP;		//# Death anim from a flip
	}
	else if (G_InKnockDown(&self->client->ps))
	{//since these happen a lot, let's handle them case by case
		int animLength = bgAllAnims[self->localAnimIndex].anims[self->client->ps.legsAnim].numFrames * fabs((float)(bgHumanoidAnimations[self->client->ps.legsAnim].frameLerp));
		switch (self->client->ps.legsAnim)
		{
		case BOTH_KNOCKDOWN1:
		case BOTH_SLAPDOWNRIGHT:
		case BOTH_SLAPDOWNLEFT:
			if (animLength - self->client->ps.legsTimer > 100)
			{//on our way down
				if (self->client->ps.legsTimer > 600)
				{//still partially up
					deathAnim = BOTH_DEATH_FALLING_UP;
				}
				else
				{//down
					deathAnim = BOTH_DEATH_LYING_UP;
				}
			}
			break;
		case BOTH_KNOCKDOWN2:
			if (animLength - self->client->ps.legsTimer > 700)
			{//on our way down
				if (self->client->ps.legsTimer > 600)
				{//still partially up
					deathAnim = BOTH_DEATH_FALLING_UP;
				}
				else
				{//down
					deathAnim = BOTH_DEATH_LYING_UP;
				}
			}
			break;
		case BOTH_KNOCKDOWN3:
			if (animLength - self->client->ps.legsTimer > 100)
			{//on our way down
				if (self->client->ps.legsTimer > 1300)
				{//still partially up
					deathAnim = BOTH_DEATH_FALLING_DN;
				}
				else
				{//down
					deathAnim = BOTH_DEATH_LYING_DN;
				}
			}
			break;
		case BOTH_KNOCKDOWN4:
			if (animLength - self->client->ps.legsTimer > 300)
			{//on our way down
				if (self->client->ps.legsTimer > 350)
				{//still partially up
					deathAnim = BOTH_DEATH_FALLING_UP;
				}
				else
				{//down
					deathAnim = BOTH_DEATH_LYING_UP;
				}
			}
			else
			{//crouch death
				vec3_t fwd;
				float thrown = 0;

				AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
				thrown = DotProduct(fwd, self->client->ps.velocity);

				if (thrown < -150)
				{
					deathAnim = BOTH_DEATHBACKWARD1;	//# Death anim when crouched and thrown back
				}
				else
				{
					deathAnim = BOTH_DEATH_CROUCHED;	//# Death anim when crouched
				}
			}
			break;
		case BOTH_KNOCKDOWN5:
			if (self->client->ps.legsTimer < 750)
			{//flat
				deathAnim = BOTH_DEATH_LYING_DN;
			}
			break;
		case BOTH_GETUP1:
			if (self->client->ps.legsTimer < 350)
			{//standing up
			}
			else if (self->client->ps.legsTimer < 800)
			{//crouching
				vec3_t fwd;
				float thrown = 0;

				AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
				thrown = DotProduct(fwd, self->client->ps.velocity);
				if (thrown < -150)
				{
					deathAnim = BOTH_DEATHBACKWARD1;	//# Death anim when crouched and thrown back
				}
				else
				{
					deathAnim = BOTH_DEATH_CROUCHED;	//# Death anim when crouched
				}
			}
			else
			{//lying down
				if (animLength - self->client->ps.legsTimer > 450)
				{//partially up
					deathAnim = BOTH_DEATH_FALLING_UP;
				}
				else
				{//down
					deathAnim = BOTH_DEATH_LYING_UP;
				}
			}
			break;
		case BOTH_GETUP2:
			if (self->client->ps.legsTimer < 150)
			{//standing up
			}
			else if (self->client->ps.legsTimer < 850)
			{//crouching
				vec3_t fwd;
				float thrown = 0;

				AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
				thrown = DotProduct(fwd, self->client->ps.velocity);

				if (thrown < -150)
				{
					deathAnim = BOTH_DEATHBACKWARD1;	//# Death anim when crouched and thrown back
				}
				else
				{
					deathAnim = BOTH_DEATH_CROUCHED;	//# Death anim when crouched
				}
			}
			else
			{//lying down
				if (animLength - self->client->ps.legsTimer > 500)
				{//partially up
					deathAnim = BOTH_DEATH_FALLING_UP;
				}
				else
				{//down
					deathAnim = BOTH_DEATH_LYING_UP;
				}
			}
			break;
		case BOTH_GETUP3:
			if (self->client->ps.legsTimer < 250)
			{//standing up
			}
			else if (self->client->ps.legsTimer < 600)
			{//crouching
				vec3_t fwd;
				float thrown = 0;
				AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
				thrown = DotProduct(fwd, self->client->ps.velocity);

				if (thrown < -150)
				{
					deathAnim = BOTH_DEATHBACKWARD1;	//# Death anim when crouched and thrown back
				}
				else
				{
					deathAnim = BOTH_DEATH_CROUCHED;	//# Death anim when crouched
				}
			}
			else
			{//lying down
				if (animLength - self->client->ps.legsTimer > 150)
				{//partially up
					deathAnim = BOTH_DEATH_FALLING_DN;
				}
				else
				{//down
					deathAnim = BOTH_DEATH_LYING_DN;
				}
			}
			break;
		case BOTH_GETUP4:
			if (self->client->ps.legsTimer < 250)
			{//standing up
			}
			else if (self->client->ps.legsTimer < 600)
			{//crouching
				vec3_t fwd;
				float thrown = 0;

				AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
				thrown = DotProduct(fwd, self->client->ps.velocity);

				if (thrown < -150)
				{
					deathAnim = BOTH_DEATHBACKWARD1;	//# Death anim when crouched and thrown back
				}
				else
				{
					deathAnim = BOTH_DEATH_CROUCHED;	//# Death anim when crouched
				}
			}
			else
			{//lying down
				if (animLength - self->client->ps.legsTimer > 850)
				{//partially up
					deathAnim = BOTH_DEATH_FALLING_DN;
				}
				else
				{//down
					deathAnim = BOTH_DEATH_LYING_UP;
				}
			}
			break;
		case BOTH_GETUP5:
			if (self->client->ps.legsTimer > 850)
			{//lying down
				if (animLength - self->client->ps.legsTimer > 1500)
				{//partially up
					deathAnim = BOTH_DEATH_FALLING_DN;
				}
				else
				{//down
					deathAnim = BOTH_DEATH_LYING_DN;
				}
			}
			break;
		case BOTH_GETUP_CROUCH_B1:
			if (self->client->ps.legsTimer < 800)
			{//crouching
				vec3_t fwd;
				float thrown = 0;

				AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
				thrown = DotProduct(fwd, self->client->ps.velocity);

				if (thrown < -150)
				{
					deathAnim = BOTH_DEATHBACKWARD1;	//# Death anim when crouched and thrown back
				}
				else
				{
					deathAnim = BOTH_DEATH_CROUCHED;	//# Death anim when crouched
				}
			}
			else
			{//lying down
				if (animLength - self->client->ps.legsTimer > 400)
				{//partially up
					deathAnim = BOTH_DEATH_FALLING_UP;
				}
				else
				{//down
					deathAnim = BOTH_DEATH_LYING_UP;
				}
			}
			break;
		case BOTH_GETUP_CROUCH_F1:
			if (self->client->ps.legsTimer < 800)
			{//crouching
				vec3_t fwd;
				float thrown = 0;

				AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
				thrown = DotProduct(fwd, self->client->ps.velocity);

				if (thrown < -150)
				{
					deathAnim = BOTH_DEATHBACKWARD1;	//# Death anim when crouched and thrown back
				}
				else
				{
					deathAnim = BOTH_DEATH_CROUCHED;	//# Death anim when crouched
				}
			}
			else
			{//lying down
				if (animLength - self->client->ps.legsTimer > 150)
				{//partially up
					deathAnim = BOTH_DEATH_FALLING_DN;
				}
				else
				{//down
					deathAnim = BOTH_DEATH_LYING_DN;
				}
			}
			break;
		case BOTH_FORCE_GETUP_B1:
			if (self->client->ps.legsTimer < 325)
			{//standing up
			}
			else if (self->client->ps.legsTimer < 725)
			{//spinning up
				deathAnim = BOTH_DEATH_SPIN_180;	//# Death anim when facing backwards
			}
			else if (self->client->ps.legsTimer < 900)
			{//crouching
				vec3_t fwd;
				float thrown = 0;

				AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
				thrown = DotProduct(fwd, self->client->ps.velocity);

				if (thrown < -150)
				{
					deathAnim = BOTH_DEATHBACKWARD1;	//# Death anim when crouched and thrown back
				}
				else
				{
					deathAnim = BOTH_DEATH_CROUCHED;	//# Death anim when crouched
				}
			}
			else
			{//lying down
				if (animLength - self->client->ps.legsTimer > 50)
				{//partially up
					deathAnim = BOTH_DEATH_FALLING_UP;
				}
				else
				{//down
					deathAnim = BOTH_DEATH_LYING_UP;
				}
			}
			break;
		case BOTH_FORCE_GETUP_B2:
			if (self->client->ps.legsTimer < 575)
			{//standing up
			}
			else if (self->client->ps.legsTimer < 875)
			{//spinning up
				deathAnim = BOTH_DEATH_SPIN_180;	//# Death anim when facing backwards
			}
			else if (self->client->ps.legsTimer < 900)
			{//crouching
				vec3_t fwd;
				float thrown = 0;

				AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
				thrown = DotProduct(fwd, self->client->ps.velocity);

				if (thrown < -150)
				{
					deathAnim = BOTH_DEATHBACKWARD1;	//# Death anim when crouched and thrown back
				}
				else
				{
					deathAnim = BOTH_DEATH_CROUCHED;	//# Death anim when crouched
				}
			}
			else
			{//lying down
				//partially up
				deathAnim = BOTH_DEATH_FALLING_UP;
			}
			break;
		case BOTH_FORCE_GETUP_B3:
			if (self->client->ps.legsTimer < 150)
			{//standing up
			}
			else if (self->client->ps.legsTimer < 775)
			{//flipping
				deathAnim = BOTH_DEATHBACKWARD2; //backflip
			}
			else
			{//lying down
				//partially up
				deathAnim = BOTH_DEATH_FALLING_UP;
			}
			break;
		case BOTH_FORCE_GETUP_B4:
			if (self->client->ps.legsTimer < 325)
			{//standing up
			}
			else
			{//lying down
				if (animLength - self->client->ps.legsTimer > 150)
				{//partially up
					deathAnim = BOTH_DEATH_FALLING_UP;
				}
				else
				{//down
					deathAnim = BOTH_DEATH_LYING_UP;
				}
			}
			break;
		case BOTH_FORCE_GETUP_B5:
			if (self->client->ps.legsTimer < 550)
			{//standing up
			}
			else if (self->client->ps.legsTimer < 1025)
			{//kicking up
				deathAnim = BOTH_DEATHBACKWARD2; //backflip
			}
			else
			{//lying down
				if (animLength - self->client->ps.legsTimer > 50)
				{//partially up
					deathAnim = BOTH_DEATH_FALLING_UP;
				}
				else
				{//down
					deathAnim = BOTH_DEATH_LYING_UP;
				}
			}
			break;
		case BOTH_FORCE_GETUP_B6:
			if (self->client->ps.legsTimer < 225)
			{//standing up
			}
			else if (self->client->ps.legsTimer < 425)
			{//crouching up
				vec3_t fwd;
				float thrown = 0;

				AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
				thrown = DotProduct(fwd, self->client->ps.velocity);

				if (thrown < -150)
				{
					deathAnim = BOTH_DEATHBACKWARD1;	//# Death anim when crouched and thrown back
				}
				else
				{
					deathAnim = BOTH_DEATH_CROUCHED;	//# Death anim when crouched
				}
			}
			else if (self->client->ps.legsTimer < 825)
			{//flipping up
				deathAnim = BOTH_DEATHFORWARD3; //backflip
			}
			else
			{//lying down
				if (animLength - self->client->ps.legsTimer > 225)
				{//partially up
					deathAnim = BOTH_DEATH_FALLING_UP;
				}
				else
				{//down
					deathAnim = BOTH_DEATH_LYING_UP;
				}
			}
			break;
		case BOTH_FORCE_GETUP_F1:
			if (self->client->ps.legsTimer < 275)
			{//standing up
			}
			else if (self->client->ps.legsTimer < 750)
			{//flipping
				deathAnim = BOTH_DEATH14;
			}
			else
			{//lying down
				if (animLength - self->client->ps.legsTimer > 100)
				{//partially up
					deathAnim = BOTH_DEATH_FALLING_DN;
				}
				else
				{//down
					deathAnim = BOTH_DEATH_LYING_DN;
				}
			}
			break;
		case BOTH_FORCE_GETUP_F2:
			if (self->client->ps.legsTimer < 1200)
			{//standing
			}
			else
			{//lying down
				if (animLength - self->client->ps.legsTimer > 225)
				{//partially up
					deathAnim = BOTH_DEATH_FALLING_DN;
				}
				else
				{//down
					deathAnim = BOTH_DEATH_LYING_DN;
				}
			}
			break;
		}
	}

	return deathAnim;
}

int G_PickDeathAnim(gentity_t * self, vec3_t point, int damage, int mod, int hitLoc)
{//FIXME: play dead flop anims on body if in an appropriate _DEAD anim when this func is called
	int deathAnim = -1;
	int max_health;
	int legAnim = 0;
	vec3_t objVelocity;

	if (self->client)
	{
		max_health = self->client->ps.stats[STAT_MAX_HEALTH];

		if (self->client->inSpaceIndex && self->client->inSpaceIndex != ENTITYNUM_NONE)
		{
			if (self->client->ps.stats[STAT_HEALTH] <= 50)
			{
				return BOTH_CHOKE3;
			}
			else
			{
				return BOTH_CHOKE4;
			}

		}
	}
	else
	{
		max_health = 60;
	}

	if (self->client)
	{
		VectorCopy(self->client->ps.velocity, objVelocity);
	}
	else
	{
		VectorCopy(self->s.pos.trDelta, objVelocity);
	}

	if (hitLoc == HL_NONE)
	{
		hitLoc = G_GetHitLocation(self, point);
	}
	//dead flops...if you are already playing a death animation, I guess it can just return directly
	switch (self->client->ps.legsAnim)
	{
	case BOTH_DEATH1:		//# First Death anim
	case BOTH_DEAD1:
	case BOTH_DEATH2:			//# Second Death anim
	case BOTH_DEAD2:
	case BOTH_DEATH8:			//#
	case BOTH_DEAD8:
	case BOTH_DEATH13:			//#
	case BOTH_DEAD13:
	case BOTH_DEATH14:			//#
	case BOTH_DEAD14:
	case BOTH_DEATH16:			//#
	case BOTH_DEAD16:
	case BOTH_DEADBACKWARD1:		//# First thrown backward death finished pose
	case BOTH_DEADBACKWARD2:		//# Second thrown backward death finished pose
		deathAnim = -2;
		break;
	case BOTH_DEADFLOP2:
		return BOTH_DEADFLOP2;
		break;
	case BOTH_DEATH10:			//#
	case BOTH_DEAD10:
	case BOTH_DEATH15:			//#
	case BOTH_DEAD15:
	case BOTH_DEADFORWARD1:		//# First thrown forward death finished pose
	case BOTH_DEADFORWARD2:		//# Second thrown forward death finished pose
		deathAnim = -2;
		break;
	case BOTH_DEADFLOP1:
		return BOTH_DEADFLOP1;
		break;
	case BOTH_DEAD3:				//# Third Death finished pose
	case BOTH_DEAD4:				//# Fourth Death finished pose
	case BOTH_DEAD5:				//# Fifth Death finished pose
	case BOTH_DEAD6:				//# Sixth Death finished pose
	case BOTH_DEAD7:				//# Seventh Death finished pose
	case BOTH_DEAD9:				//#
	case BOTH_DEAD11:			//#
	case BOTH_DEAD12:			//#
	case BOTH_DEAD17:			//#
	case BOTH_DEAD18:			//#
	case BOTH_DEAD19:			//#
	case BOTH_DEAD20:			//#
	case BOTH_DEAD21:			//#
	case BOTH_DEAD22:			//#
	case BOTH_DEAD23:			//#
	case BOTH_DEAD24:			//#
	case BOTH_DEAD25:			//#
	case BOTH_LYINGDEAD1:		//# Killed lying down death finished pose
	case BOTH_STUMBLEDEAD1:		//# Stumble forward death finished pose
	case BOTH_FALLDEAD1LAND:		//# Fall forward and splat death finished pose
	case BOTH_DEATH3:			//# Third Death anim
	case BOTH_DEATH4:			//# Fourth Death anim
	case BOTH_DEATH5:			//# Fifth Death anim
	case BOTH_DEATH6:			//# Sixth Death anim
	case BOTH_DEATH7:			//# Seventh Death anim
	case BOTH_DEATH9:			//#
	case BOTH_DEATH11:			//#
	case BOTH_DEATH12:			//#
	case BOTH_DEATH17:			//#
	case BOTH_DEATH18:			//#
	case BOTH_DEATH19:			//#
	case BOTH_DEATH20:			//#
	case BOTH_DEATH21:			//#
	case BOTH_DEATH22:			//#
	case BOTH_DEATH23:			//#
	case BOTH_DEATH24:			//#
	case BOTH_DEATH25:			//#
	case BOTH_DEATHFORWARD1:		//# First Death in which they get thrown forward
	case BOTH_DEATHFORWARD2:		//# Second Death in which they get thrown forward
	case BOTH_DEATHFORWARD3:		//# Second Death in which they get thrown forward
	case BOTH_DEATHBACKWARD1:	//# First Death in which they get thrown backward
	case BOTH_DEATHBACKWARD2:	//# Second Death in which they get thrown backward
	case BOTH_DEATH1IDLE:		//# Idle while close to death
	case BOTH_LYINGDEATH1:		//# Death to play when killed lying down
	case BOTH_STUMBLEDEATH1:		//# Stumble forward and fall face first death
	case BOTH_FALLDEATH1:		//# Fall forward off a high cliff and splat death - start
	case BOTH_FALLDEATH1INAIR:	//# Fall forward off a high cliff and splat death - loop
	case BOTH_FALLDEATH1LAND:	//# Fall forward off a high cliff and splat death - hit bottom
		deathAnim = -2;
		break;
	case BOTH_DEATH_ROLL:		//# Death anim from a roll
	case BOTH_DEATH_FLIP:		//# Death anim from a flip
	case BOTH_DEATH_SPIN_90_R:	//# Death anim when facing 90 degrees right
	case BOTH_DEATH_SPIN_90_L:	//# Death anim when facing 90 degrees left
	case BOTH_DEATH_SPIN_180:	//# Death anim when facing backwards
	case BOTH_DEATH_LYING_UP:	//# Death anim when lying on back
	case BOTH_DEATH_LYING_DN:	//# Death anim when lying on front
	case BOTH_DEATH_FALLING_DN:	//# Death anim when falling on face
	case BOTH_DEATH_FALLING_UP:	//# Death anim when falling on back
	case BOTH_DEATH_CROUCHED:	//# Death anim when crouched
	case BOTH_RIGHTHANDCHOPPEDOFF:
		deathAnim = -2;
		break;
	}
	// Not currently playing a death animation, so try and get an appropriate one now.
	if (deathAnim == -1)
	{
		deathAnim = G_CheckSpecialDeathAnim(self, point, damage, mod, hitLoc);

		if (deathAnim == -1)
		{//base on hitLoc
			vec3_t fwd;
			AngleVectors(self->r.currentAngles, fwd, NULL, NULL);
			float	thrown = DotProduct(fwd, self->client->ps.velocity);
			//death anims
			switch (hitLoc)
			{
			case HL_FOOT_RT:
				if (!Q_irand(0, 2) && thrown < 250)
				{
					deathAnim = BOTH_DEATH24;//right foot trips up, spin
				}
				else if (!Q_irand(0, 1))
				{
					if (!Q_irand(0, 1))
					{
						deathAnim = BOTH_DEATH4;//back: forward
					}
					else
					{
						deathAnim = BOTH_DEATH16;//same as 1
					}
				}
				else
				{
					deathAnim = BOTH_DEATH5;//same as 4
				}
				break;
			case HL_FOOT_LT:
				if (!Q_irand(0, 2) && thrown < 250)
				{
					deathAnim = BOTH_DEATH25;//left foot trips up, spin
				}
				else if (!Q_irand(0, 1))
				{
					if (!Q_irand(0, 1))
					{
						deathAnim = BOTH_DEATH4;//back: forward
					}
					else
					{
						deathAnim = BOTH_DEATH16;//same as 1
					}
				}
				else
				{
					deathAnim = BOTH_DEATH5;//same as 4
				}
				break;
			case HL_LEG_RT:
				if (!Q_irand(0, 2) && thrown < 250)
				{
					deathAnim = BOTH_DEATH3;//right leg collapse
				}
				else if (!Q_irand(0, 1))
				{
					deathAnim = BOTH_DEATH5;//same as 4
				}
				else
				{
					if (!Q_irand(0, 1))
					{
						deathAnim = BOTH_DEATH4;//back: forward
					}
					else
					{
						deathAnim = BOTH_DEATH16;//same as 1
					}
				}
				break;
			case HL_LEG_LT:
				if (!Q_irand(0, 2) && thrown < 250)
				{
					deathAnim = BOTH_DEATH7;//left leg collapse
				}
				else if (!Q_irand(0, 1))
				{
					deathAnim = BOTH_DEATH5;//same as 4
				}
				else
				{
					if (!Q_irand(0, 1))
					{
						deathAnim = BOTH_DEATH4;//back: forward
					}
					else
					{
						deathAnim = BOTH_DEATH16;//same as 1
					}
				}
				break;
			case HL_BACK:
				if (fabs(thrown) < 50 || (fabs(thrown) < 200 && !Q_irand(0, 3)))
				{
					if (Q_irand(0, 1))
					{
						deathAnim = BOTH_DEATH17;//head/back: croak
					}
					else
					{
						deathAnim = BOTH_DEATH10;//back: bend back, fall forward
					}
				}
				else
				{
					if (!Q_irand(0, 2))
					{
						deathAnim = BOTH_DEATH4;//back: forward
					}
					else if (!Q_irand(0, 1))
					{
						deathAnim = BOTH_DEATH5;//back: forward
					}
					else
					{
						deathAnim = BOTH_DEATH16;//same as 1
					}
				}
				break;
			case HL_HAND_RT:
			case HL_CHEST_RT:
			case HL_ARM_RT:
			case HL_BACK_LT:
				if ((damage <= max_health * 0.25 && Q_irand(0, 1)) || (fabs(thrown) < 200 && !Q_irand(0, 2)) || !Q_irand(0, 10))
				{
					if (Q_irand(0, 1))
					{
						deathAnim = BOTH_DEATH9;//chest right: snap, fall forward
					}
					else
					{
						deathAnim = BOTH_DEATH20;//chest right: snap, fall forward
					}
				}
				else if ((damage <= max_health * 0.5 && Q_irand(0, 1)) || !Q_irand(0, 10))
				{
					deathAnim = BOTH_DEATH3;//chest right: back
				}
				else if ((damage <= max_health * 0.75 && Q_irand(0, 1)) || !Q_irand(0, 10))
				{
					deathAnim = BOTH_DEATH6;//chest right: spin
				}
				else
				{
					//TEMP HACK: play spinny deaths less often
					if (Q_irand(0, 1))
					{
						deathAnim = BOTH_DEATH8;//chest right: spin high
					}
					else
					{
						switch (Q_irand(0, 3))
						{
						default:
						case 0:
							deathAnim = BOTH_DEATH9;//chest right: snap, fall forward
							break;
						case 1:
							deathAnim = BOTH_DEATH3;//chest right: back
							break;
						case 2:
							deathAnim = BOTH_DEATH6;//chest right: spin
							break;
						case 3:
							deathAnim = BOTH_DEATH20;//chest right: spin
							break;
						}
					}
				}
				break;
			case HL_CHEST_LT:
			case HL_ARM_LT:
			case HL_HAND_LT:
			case HL_BACK_RT:
				if ((damage <= max_health * 0.25 && Q_irand(0, 1)) || (fabs(thrown) < 200 && !Q_irand(0, 2)) || !Q_irand(0, 10))
				{
					if (Q_irand(0, 1))
					{
						deathAnim = BOTH_DEATH11;//chest left: snap, fall forward
					}
					else
					{
						deathAnim = BOTH_DEATH21;//chest left: snap, fall forward
					}
				}
				else if ((damage <= max_health * 0.5 && Q_irand(0, 1)) || !Q_irand(0, 10))
				{
					deathAnim = BOTH_DEATH7;//chest left: back
				}
				else if ((damage <= max_health * 0.75 && Q_irand(0, 1)) || !Q_irand(0, 10))
				{
					deathAnim = BOTH_DEATH12;//chest left: spin
				}
				else
				{
					//TEMP HACK: play spinny deaths less often
					if (Q_irand(0, 1))
					{
						deathAnim = BOTH_DEATH14;//chest left: spin high
					}
					else
					{
						switch (Q_irand(0, 3))
						{
						default:
						case 0:
							deathAnim = BOTH_DEATH11;//chest left: snap, fall forward
							break;
						case 1:
							deathAnim = BOTH_DEATH7;//chest left: back
							break;
						case 2:
							deathAnim = BOTH_DEATH12;//chest left: spin
							break;
						case 3:
							deathAnim = BOTH_DEATH21;//chest left: spin
							break;
						}
					}
				}
				break;
			case HL_CHEST:
			case HL_WAIST:
				if ((damage <= max_health * 0.25 && Q_irand(0, 1)) || thrown > -50)
				{
					if (!Q_irand(0, 1))
					{
						deathAnim = BOTH_DEATH18;//gut: fall right
					}
					else
					{
						deathAnim = BOTH_DEATH19;//gut: fall left
					}
				}
				else if ((damage <= max_health * 0.5 && !Q_irand(0, 1)) || (fabs(thrown) < 200 && !Q_irand(0, 3)))
				{
					if (Q_irand(0, 2))
					{
						deathAnim = BOTH_DEATH2;//chest: backward short
					}
					else if (Q_irand(0, 1))
					{
						deathAnim = BOTH_DEATH22;//chest: backward short
					}
					else
					{
						deathAnim = BOTH_DEATH23;//chest: backward short
					}
				}
				else if (thrown < -300 && Q_irand(0, 1))
				{
					if (Q_irand(0, 1))
					{
						deathAnim = BOTH_DEATHBACKWARD1;//chest: fly back
					}
					else
					{
						deathAnim = BOTH_DEATHBACKWARD2;//chest: flip back
					}
				}
				else if (thrown < -200 && Q_irand(0, 1))
				{
					deathAnim = BOTH_DEATH15;//chest: roll backward
				}
				else
				{
					deathAnim = BOTH_DEATH1;//chest: backward med
				}
				break;
			case HL_HEAD:
				if (damage <= max_health * 0.5 && Q_irand(0, 2))
				{
					deathAnim = BOTH_DEATH17;//head/back: croak
				}
				else
				{
					if (Q_irand(0, 2))
					{
						deathAnim = BOTH_DEATH13;//head: stumble, fall back
					}
					else
					{
						deathAnim = BOTH_DEATH10;//head: stumble, fall back
					}
				}
				break;
			default:
				break;
			}
		}
	}

	// Validate.....
	if (deathAnim == -1 || !BG_HasAnimation(self->localAnimIndex, deathAnim))
	{
		if (deathAnim == BOTH_DEADFLOP1
			|| deathAnim == BOTH_DEADFLOP2)
		{//if don't have deadflop, don't do anything
			deathAnim = -1;
		}
		else
		{
			// I guess we'll take what we can get.....
			deathAnim = PM_PickAnim(self->localAnimIndex, BOTH_DEATH1, BOTH_DEATH25);
		}
	}

	return deathAnim;
}

static int G_CheckForLedge(gentity_t * self, vec3_t fallCheckDir, float checkDist);
int G_CheckLedgeDive(gentity_t * self, float checkDist, const vec3_t checkVel, qboolean tryOpposite, qboolean tryPerp)
{
	//		Intelligent Ledge-Diving Deaths:
	//		If I'm an NPC, check for nearby ledges and fall off it if possible
	//		How should/would/could this interact with knockback if we already have some?
	//		Ideally - apply knockback if there are no ledges or a ledge in that dir
	//		But if there is a ledge and it's not in the dir of my knockback, fall off the ledge instead
	vec3_t	fallForwardDir, fallRightDir;
	vec3_t	angles;
	float fallDist;
	int		cliff_fall = 0;

	VectorCopy(vec3_origin, angles);

	if (!self || !self->client)
	{
		return 0;
	}

	if (checkVel && !VectorCompare(checkVel, vec3_origin))
	{//already moving in a dir
		angles[1] = vectoyaw(self->client->ps.velocity);
		AngleVectors(angles, fallForwardDir, fallRightDir, NULL);
	}
	else
	{//try forward first
		angles[1] = self->client->ps.viewangles[1];
		AngleVectors(angles, fallForwardDir, fallRightDir, NULL);
	}
	VectorNormalize(fallForwardDir);
	fallDist = G_CheckForLedge(self, fallForwardDir, checkDist);
	if (fallDist >= 128)
	{
		VectorClear(self->client->ps.velocity);
		G_Throw(self, fallForwardDir, 85);
		self->client->ps.velocity[2] = 100;
		self->client->ps.groundEntityNum = ENTITYNUM_NONE;
	}
	else if (tryOpposite)
	{
		VectorScale(fallForwardDir, -1, fallForwardDir);
		fallDist = G_CheckForLedge(self, fallForwardDir, checkDist);
		if (fallDist >= 128)
		{
			VectorClear(self->client->ps.velocity);
			G_Throw(self, fallForwardDir, 85);
			self->client->ps.velocity[2] = 100;
			self->client->ps.groundEntityNum = ENTITYNUM_NONE;
		}
	}
	if (!cliff_fall && tryPerp)
	{//try sides
		VectorNormalize(fallRightDir);
		fallDist = G_CheckForLedge(self, fallRightDir, checkDist);
		if (fallDist >= 128)
		{
			VectorClear(self->client->ps.velocity);
			G_Throw(self, fallRightDir, 85);
			self->client->ps.velocity[2] = 100;
		}
		else
		{
			VectorScale(fallRightDir, -1, fallRightDir);
			fallDist = G_CheckForLedge(self, fallRightDir, checkDist);
			if (fallDist >= 128)
			{
				VectorClear(self->client->ps.velocity);
				G_Throw(self, fallRightDir, 85);
				self->client->ps.velocity[2] = 100;
			}
		}
	}
	if (fallDist >= 256)
	{
		cliff_fall = 2;
	}
	else if (fallDist >= 128)
	{
		cliff_fall = 1;
	}
	return cliff_fall;
}

gentity_t* G_GetJediMaster(void)
{
	int i = 0;
	gentity_t* ent;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent&& ent->inuse&& ent->client&& ent->client->ps.isJediMaster)
		{
			return ent;
		}

		i++;
	}

	return NULL;
}

/*
-------------------------
G_AlertTeam
-------------------------
*/

void G_AlertTeam(gentity_t * victim, gentity_t * attacker, float radius, float soundDist)
{
	int			radiusEnts[128];
	gentity_t* check;
	vec3_t		mins, maxs;
	int			numEnts;
	int			i;
	float		distSq, sndDistSq = (soundDist * soundDist);

	if (attacker == NULL || attacker->client == NULL)
		return;

	//Setup the bbox to search in
	for (i = 0; i < 3; i++)
	{
		mins[i] = victim->r.currentOrigin[i] - radius;
		maxs[i] = victim->r.currentOrigin[i] + radius;
	}

	//Get the number of entities in a given space
	numEnts = trap->EntitiesInBox(mins, maxs, radiusEnts, 128);

	//Cull this list
	for (i = 0; i < numEnts; i++)
	{
		check = &g_entities[radiusEnts[i]];

		//Validate clients
		if (check->client == NULL)
			continue;

		//only want NPCs
		if (check->NPC == NULL)
			continue;

		if (check->NPC->scriptFlags& SCF_IGNORE_ENEMIES)
			continue;

		//This NPC specifically flagged to ignore alerts
		if (check->NPC->scriptFlags& SCF_IGNORE_ALERTS)
			continue;

		//This NPC specifically flagged to ignore alerts
		if (!(check->NPC->scriptFlags& SCF_LOOK_FOR_ENEMIES))
			continue;

		//this ent does not participate in group AI
		if ((check->NPC->scriptFlags& SCF_NO_GROUPS))
			continue;

		//Skip the requested avoid check if present
		if (check == victim)
			continue;

		//Skip the attacker
		if (check == attacker)
			continue;

		//Must be on the same team
		if (check->client->playerTeam != victim->client->playerTeam)
			continue;

		//Must be alive
		if (check->health <= 0)
			continue;

		if (check->enemy == NULL)
		{//only do this if they're not already mad at someone
			distSq = DistanceSquared(check->r.currentOrigin, victim->r.currentOrigin);
			if (distSq > 16384 /*128 squared*/ && !trap->InPVS(victim->r.currentOrigin, check->r.currentOrigin))
			{//not even potentially visible/hearable
				continue;
			}
			//NOTE: this allows sound alerts to still go through doors/PVS if the teammate is within 128 of the victim...
			if (soundDist <= 0 || distSq > sndDistSq)
			{//out of sound range
				if (!InFOV(victim, check, check->NPC->stats.hfov, check->NPC->stats.vfov)
					|| !NPC_ClearLOS2(check, victim->r.currentOrigin))
				{//out of FOV or no LOS
					continue;
				}
			}

			//FIXME: This can have a nasty cascading effect if setup wrong...
			G_SetEnemy(check, attacker);
		}
	}
}

/*
-------------------------
G_DeathAlert
-------------------------
*/

#define	DEATH_ALERT_RADIUS			512
#define	DEATH_ALERT_SOUND_RADIUS	512

void G_DeathAlert(gentity_t * victim, gentity_t * attacker)
{//FIXME: with all the other alert stuff, do we really need this?
	G_AlertTeam(victim, attacker, DEATH_ALERT_RADIUS, DEATH_ALERT_SOUND_RADIUS);
}

/*
----------------------------------------
DeathFX

Applies appropriate special effects that occur while the entity is dying
Not to be confused with NPC_RemoveBodyEffects (NPC.cpp), which only applies effect when removing the body
----------------------------------------
*/

extern qboolean Jedi_CultistDestroyer(gentity_t * self);
void DeathFX(gentity_t * ent)
{
	vec3_t		effectPos, right;
	vec3_t		defaultDir;

	if (!ent || !ent->client)
		return;

	VectorSet(defaultDir, 0, 0, 1);

	if (Jedi_CultistDestroyer(NPCS.NPC))
	{//destroyer
		AngleVectors(ent->r.currentAngles, NULL, right, NULL);
		VectorMA(ent->r.currentOrigin, 10, right, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/cultist1/misc/anger1"));
	}

	// team no longer indicates species/race.  NPC_class should be used to identify certain npc types
	switch (ent->client->NPC_class)
	{
	case CLASS_MOUSE:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 20;
		G_PlayEffectID(G_EffectIndex("env/small_explode"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/mouse/misc/death1"));
		break;

	case CLASS_PROBE:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] += 50;
		G_PlayEffectID(G_EffectIndex("explosions/probeexplosion1"), effectPos, defaultDir);
		break;

	case CLASS_ATST:
		AngleVectors(ent->r.currentAngles, NULL, right, NULL);
		VectorMA(ent->r.currentOrigin, 20, right, effectPos);
		effectPos[2] += 180;
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -40, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		break;

	case CLASS_SEEKER:
	case CLASS_REMOTE:
		G_PlayEffectID(G_EffectIndex("env/small_explode"), ent->r.currentOrigin, defaultDir);
		break;

	case CLASS_GONK:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 5;
		G_Sound(ent, CHAN_AUTO, G_SoundIndex(va("sound/chars/gonk/misc/death%d.wav", Q_irand(1, 3))));
		G_PlayEffectID(G_EffectIndex("env/med_explode"), effectPos, defaultDir);
		break;

		// should list all remaining droids here, hope I didn't miss any
	case CLASS_R2D2:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 10;
		G_PlayEffectID(G_EffectIndex("env/med_explode"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/mark2/misc/mark2_explo"));
		break;

	case CLASS_PROTOCOL: //c3p0
	case CLASS_R5D2:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 10;
		G_PlayEffectID(G_EffectIndex("env/med_explode"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/mark2/misc/mark2_explo"));
		break;

	case CLASS_MARK2:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/mark2/misc/mark2_explo"));
		break;

	case CLASS_INTERROGATOR:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/interrogator/misc/int_droid_explo"));
		break;

	case CLASS_MARK1:
		AngleVectors(ent->r.currentAngles, NULL, right, NULL);
		VectorMA(ent->r.currentOrigin, 10, right, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/mark1/misc/mark1_explo"));
		break;

	case CLASS_SENTRY:
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/sentry/misc/sentry_explo"));
		VectorCopy(ent->r.currentOrigin, effectPos);
		G_PlayEffectID(G_EffectIndex("env/med_explode"), effectPos, defaultDir);
		break;

	case CLASS_ROCKETTROOPER:
		AngleVectors(ent->r.currentAngles, NULL, right, NULL);
		VectorMA(ent->r.currentOrigin, 10, right, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/mark1/misc/mark1_explo"));
		break;

	case CLASS_BOBAFETT:
	case CLASS_MANDO:
		AngleVectors(ent->r.currentAngles, NULL, right, NULL);
		VectorMA(ent->r.currentOrigin, 10, right, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/mark1/misc/mark1_explo"));
		break;

	case CLASS_SBD:
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/SBD/misc/death2"));
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		break;

	case CLASS_DROIDEKA:
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/battledroid/misc/death3"));
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		break;

	case CLASS_BATTLEDROID:
		VectorCopy(ent->r.currentOrigin, effectPos);
		G_PlayEffectID(G_EffectIndex("env/med_explode"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/battledroid/misc/death2"));
		break;

	case CLASS_GALAKMECH:
	case CLASS_ASSASSIN_DROID:
	case CLASS_HAZARD_TROOPER:
		AngleVectors(ent->r.currentAngles, NULL, right, NULL);
		VectorMA(ent->r.currentOrigin, 10, right, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/mark1/misc/mark1_explo"));
		break;

	case CLASS_DESANN:
	case CLASS_VADER:
	case CLASS_SITHLORD:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/desannDeath"), effectPos, defaultDir);
		break;

	case CLASS_TAVION:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/sithlorddeath"), effectPos, defaultDir);
		break;

	case CLASS_ALORA:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/sithdeath"), effectPos, defaultDir);
		break;

	case CLASS_LUKE:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/LukeDeath"), effectPos, defaultDir);
		break;

	case CLASS_SHADOWTROOPER:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/LukeDeath"), effectPos, defaultDir);
		break;

	case CLASS_MORGANKATARN:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/JediDeath"), effectPos, defaultDir);
		break;

	default:
		if (!(ent->r.svFlags& SVF_BOT))
		{
			G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/player/death.mp3"));
		}
		break;

	}

}

void DeathFXEXTRA(gentity_t * ent)
{
	vec3_t		effectPos, right;
	vec3_t		defaultDir;

	VectorSet(defaultDir, 0, 0, 1);

	if (Jedi_CultistDestroyer(NPCS.NPC))
	{//destroyer
		AngleVectors(ent->r.currentAngles, NULL, right, NULL);
		VectorMA(ent->r.currentOrigin, 10, right, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/cultist1/misc/anger1"));
	}

	switch (ent->client->NPC_class)
	{
	case CLASS_BOBAFETT:
		AngleVectors(ent->r.currentAngles, NULL, right, NULL);
		VectorMA(ent->r.currentOrigin, 10, right, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/mark1/misc/mark1_explo"));
		break;

	case CLASS_MOUSE:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 20;
		G_PlayEffectID(G_EffectIndex("env/small_explode"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/mouse/misc/death1"));
		break;

	case CLASS_PROBE:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] += 50;
		G_PlayEffectID(G_EffectIndex("explosions/probeexplosion1"), effectPos, defaultDir);
		break;

	case CLASS_ATST:
		AngleVectors(ent->r.currentAngles, NULL, right, NULL);
		VectorMA(ent->r.currentOrigin, 20, right, effectPos);
		effectPos[2] += 180;
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -40, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		break;

	case CLASS_SEEKER:
	case CLASS_REMOTE:
		G_PlayEffectID(G_EffectIndex("env/small_explode"), ent->r.currentOrigin, defaultDir);
		break;

	case CLASS_GONK:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 5;
		G_Sound(ent, CHAN_AUTO, G_SoundIndex(va("sound/chars/gonk/misc/death%d.wav", Q_irand(1, 3))));
		G_PlayEffectID(G_EffectIndex("env/med_explode"), effectPos, defaultDir);
		break;

	case CLASS_R2D2:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 10;
		G_PlayEffectID(G_EffectIndex("env/med_explode"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/mark2/misc/mark2_explo"));
		break;

	case CLASS_PROTOCOL:
	case CLASS_R5D2:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 10;
		G_PlayEffectID(G_EffectIndex("env/med_explode"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/mark2/misc/mark2_explo"));
		break;

	case CLASS_MARK2:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/mark2/misc/mark2_explo"));
		break;

	case CLASS_INTERROGATOR:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/interrogator/misc/int_droid_explo"));
		break;

	case CLASS_MARK1:
		AngleVectors(ent->r.currentAngles, NULL, right, NULL);
		VectorMA(ent->r.currentOrigin, 10, right, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/mark1/misc/mark1_explo"));
		break;

	case CLASS_SENTRY:
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/sentry/misc/sentry_explo"));
		VectorCopy(ent->r.currentOrigin, effectPos);
		G_PlayEffectID(G_EffectIndex("env/med_explode"), effectPos, defaultDir);
		break;

	case CLASS_DESANN:
	case CLASS_VADER:
	case CLASS_SITHLORD:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/desannDeath"), effectPos, defaultDir);
		break;

	case CLASS_TAVION:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/sithlorddeath"), effectPos, defaultDir);
		break;

	case CLASS_ALORA:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/sithdeath"), effectPos, defaultDir);
		break;

	case CLASS_LUKE:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/LukeDeath"), effectPos, defaultDir);
		break;

	case CLASS_SHADOWTROOPER:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/LukeDeath"), effectPos, defaultDir);
		break;

	case CLASS_MORGANKATARN:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/JediDeath"), effectPos, defaultDir);
		break;

	default:
		break;

	}
	switch (ent->client->pers.botclass)
	{
	case BCLASS_BOBAFETT:
	case BCLASS_MANDOLORIAN:
	case BCLASS_MANDOLORIAN1:
	case BCLASS_MANDOLORIAN2:
		AngleVectors(ent->r.currentAngles, NULL, right, NULL);
		VectorMA(ent->r.currentOrigin, 10, right, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		VectorMA(effectPos, -20, right, effectPos);
		G_PlayEffectID(G_EffectIndex("explosions/droidexplosion1"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/mark1/misc/mark1_explo"));
		break;

	case BCLASS_BATTLEDROID:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 10;
		G_PlayEffectID(G_EffectIndex("env/med_explode"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/battledroid/misc/death2"));
		break;

	case BCLASS_SBD:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 10;
		G_PlayEffectID(G_EffectIndex("env/med_explode"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/SBD/misc/death2"));
		break;

	case BCLASS_DROIDEKA:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 10;
		G_PlayEffectID(G_EffectIndex("env/med_explode"), effectPos, defaultDir);
		G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/chars/battledroid/misc/death3"));
		break;

	case BCLASS_DESANN:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/desannDeath"), effectPos, defaultDir);
		break;

	case BCLASS_TAVION:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/sithlorddeath"), effectPos, defaultDir);
		break;

	case BCLASS_ALORA:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/sithdeath"), effectPos, defaultDir);
		break;

	case BCLASS_LUKE:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/LukeDeath"), effectPos, defaultDir);
		break;

	case BCLASS_SHADOWTROOPER:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/LukeDeath"), effectPos, defaultDir);
		break;

	case BCLASS_MORGANKATARN:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/JediDeath"), effectPos, defaultDir);
		break;

	case BCLASS_UNSTABLESABER:
		VectorCopy(ent->r.currentOrigin, effectPos);
		effectPos[2] -= 15;
		G_PlayEffectID(G_EffectIndex("force/desannDeath"), effectPos, defaultDir);
		break;

	default:
		if (!(ent->r.svFlags& SVF_BOT))
		{
			G_Sound(ent, CHAN_AUTO, G_SoundIndex("sound/player/death.mp3"));
		}
		break;
	}
}

void G_CheckVictoryScript(gentity_t * self)
{
	if (!G_ActivateBehavior(self, BSET_VICTORY))
	{
		if (self->NPC && self->s.weapon == WP_SABER)
		{//Jedi taunt from within their AI
			self->NPC->blockedSpeechDebounceTime = 0;//get them ready to taunt
			return;
		}
		else if (self->client && self->client->NPC_class == CLASS_GALAKMECH)
		{
			self->wait = 1;
			TIMER_Set(self, "gloatTime", Q_irand(5000, 8000));
			self->NPC->blockedSpeechDebounceTime = 0;//get him ready to taunt
			return;
		}
		//FIXME: any way to not say this *right away*?  Wait for victim's death anim/scream to finish?
		if (self->NPC && self->NPC->group && self->NPC->group->commander && self->NPC->group->commander->NPC && self->NPC->group->commander->NPC->rank > self->NPC->rank && !Q_irand(0, 2))
		{//sometimes have the group commander speak instead
			self->NPC->group->commander->NPC->greetingDebounceTime = level.time + Q_irand(2000, 5000);
			G_AddVoiceEvent(self->NPC->group->commander, Q_irand(EV_VICTORY1, EV_VICTORY3), 2000);
		}
		else if (self->NPC)
		{
			self->NPC->greetingDebounceTime = level.time + Q_irand(2000, 5000);
			G_AddVoiceEvent(self, Q_irand(EV_VICTORY1, EV_VICTORY3), 2000);
		}
	}
}

void G_AddPowerDuelScore(int team, int score)
{
	int i = 0;
	gentity_t* check;

	while (i < MAX_CLIENTS)
	{
		check = &g_entities[i];
		if (check->inuse&& check->client&&
			check->client->pers.connected == CON_CONNECTED && !check->client->iAmALoser &&
			check->client->ps.stats[STAT_HEALTH] > 0 &&
			check->client->sess.sessionTeam != TEAM_SPECTATOR &&
			check->client->sess.duelTeam == team)
		{ //found a living client on the specified team
			check->client->sess.wins += score;
			ClientUserinfoChanged(check->s.number);
		}
		i++;
	}
}

void G_AddPowerDuelLoserScore(int team, int score)
{
	int i = 0;
	gentity_t* check;

	while (i < MAX_CLIENTS)
	{
		check = &g_entities[i];
		if (check->inuse&& check->client&&
			check->client->pers.connected == CON_CONNECTED &&
			(check->client->iAmALoser || (check->client->ps.stats[STAT_HEALTH] <= 0 && check->client->sess.sessionTeam != TEAM_SPECTATOR)) &&
			check->client->sess.duelTeam == team)
		{ //found a living client on the specified team
			check->client->sess.losses += score;
			ClientUserinfoChanged(check->s.number);
		}
		i++;
	}
}
extern qboolean g_noPDuelCheck;
void G_BroadcastObit(gentity_t * self, gentity_t * inflictor, gentity_t * attacker, int killer, int meansOfDeath, int wasInVehicle, qboolean wasJediMaster)
{
	// broadcast the death event to everyone
	if (self->s.eType != ET_NPC && !g_noPDuelCheck)
	{
		gentity_t* ent;

		ent = G_TempEntity(self->r.currentOrigin, EV_OBITUARY);
		ent->s.eventParm = meansOfDeath;
		ent->s.otherEntityNum = self->s.number;
		if (attacker)
		{
			ent->s.otherEntityNum2 = attacker->s.number;
		}
		else
		{//???
			ent->s.otherEntityNum2 = killer;
		}
		if (inflictor
			&& !Q_stricmp("vehicle_proj", inflictor->classname))
		{//a vehicle missile
			ent->s.eventParm = MOD_VEHICLE;
			//store index into g_vehWeaponInfo
			ent->s.weapon = inflictor->s.otherEntityNum2 + 1;
			//store generic rocket or blaster type of missile
			ent->s.generic1 = inflictor->s.weapon;
		}
		if (wasInVehicle && self->s.number < MAX_CLIENTS)
		{//target is in a vehicle, store the entnum
			ent->s.lookTarget = wasInVehicle;
		}
		if (attacker)
		{
			if (attacker->s.m_iVehicleNum&& attacker->s.number < MAX_CLIENTS)
			{//target is in a vehicle, store the entnum
				ent->s.brokenLimbs = attacker->s.m_iVehicleNum;
			}
			else if (ent->s.lookTarget
				&& !Q_stricmp("func_rotating", attacker->classname))
			{//my vehicle was killed by a func_rotating, probably an asteroid, so...
				ent->s.saberInFlight = qtrue;
			}
		}
		ent->r.svFlags = SVF_BROADCAST;	// send to everyone
		ent->s.isJediMaster = wasJediMaster;
	}
}

/*
==================
player_die
==================
*/
extern stringID_table_t animTable[MAX_ANIMATIONS + 1];

extern void AI_DeleteSelfFromGroup(gentity_t * self);
extern void AI_GroupMemberKilled(gentity_t * self);
extern void Boba_FlyStop(gentity_t * self);
extern qboolean Jedi_WaitingAmbush(gentity_t * self);
void CheckExitRules(void);
extern void Rancor_DropVictim(gentity_t * self);

extern qboolean g_dontFrickinCheck;
extern qboolean g_endPDuel;
extern qboolean g_noPDuelCheck;
extern void saberReactivate(gentity_t * saberent, gentity_t * saberOwner);
extern void saberBackToOwner(gentity_t * saberent);
void AddFatigueKillBonus(gentity_t * attacker, gentity_t * victim);
extern void BubbleShield_TurnOff(gentity_t * self);
void G_CheckForblowingup(gentity_t * ent, gentity_t * enemy, vec3_t point, int damage, int deathAnim, qboolean postDeath);

void player_die(gentity_t * self, gentity_t * inflictor, gentity_t * attacker, int damage, int meansOfDeath)
{
	gentity_t* ent;
	int			contents;
	int			anim;
	int			killer;
	int			i;
	char* killerName, * obit;
	qboolean	wasJediMaster = qfalse;
	int			sPMType = 0;
	char		buf[512] = { 0 };

	if (self->client->hook)
	{ // free it!
		Weapon_HookFree(self->client->hook);
	}

	if (attacker->NPC && attacker->client->NPC_class == CLASS_SEEKER && meansOfDeath == MOD_SEEKER)
	{//attacker was a player's seeker item
		if (attacker->client->leader //has leader
			&& attacker->client->leader->client //leader is a client
			&& attacker->client->leader->client->remote == attacker) //has us as their remote.
		{//item has valid parent, make them the true attacker
			attacker = attacker->client->leader;
		}
	}

	if (self->client->ps.pm_type == PM_DEAD)
	{
		return;
	}

	if (level.intermissiontime) {
		return;
	}

	if (!attacker)
		return;

	//check player stuff
	g_dontFrickinCheck = qfalse;

	if (level.gametype == GT_SINGLE_PLAYER && g_lms.integer == 1)
	{
		gentity_t* murderer = NULL;
		murderer = &g_entities[self->client->ps.otherKiller];
		murderer->liveExp++;
		if (murderer->liveExp >= g_liveExp.integer)
		{
			murderer->lives++;
			murderer->liveExp = 0;
#ifdef _DEBUG
			trap->Print("You now have %i lives.", murderer->lives);
#endif
		}
	}

	if (level.gametype == GT_POWERDUEL)
	{ //don't want to wait til later in the frame if this is the case
		CheckExitRules();

		if (level.intermissiontime)
		{
			return;
		}
	}

	if (self->s.eType == ET_NPC &&
		self->s.NPC_class == CLASS_VEHICLE &&
		self->m_pVehicle &&
		!self->m_pVehicle->m_pVehicleInfo->explosionDelay &&
		(self->m_pVehicle->m_pPilot || self->m_pVehicle->m_iNumPassengers > 0 || self->m_pVehicle->m_pDroidUnit))
	{ //kill everyone on board in the name of the attacker... if the vehicle has no death delay
		gentity_t* murderer = NULL;
		gentity_t* killEnt;

		if (self->client->ps.otherKillerTime >= level.time)
		{ //use the last attacker
			murderer = &g_entities[self->client->ps.otherKiller];
			if (!murderer->inuse || !murderer->client)
			{
				murderer = NULL;
			}
			else
			{
				if (murderer->s.number >= MAX_CLIENTS &&
					murderer->s.eType == ET_NPC &&
					murderer->s.NPC_class == CLASS_VEHICLE &&
					murderer->m_pVehicle &&
					murderer->m_pVehicle->m_pPilot)
				{
					gentity_t* murderPilot = &g_entities[murderer->m_pVehicle->m_pPilot->s.number];
					if (murderPilot->inuse && murderPilot->client)
					{ //give the pilot of the offending vehicle credit for the kill
						murderer = murderPilot;
					}
				}
			}
		}
		else if (attacker && attacker->inuse && attacker->client)
		{
			if (attacker->s.number >= MAX_CLIENTS &&
				attacker->s.eType == ET_NPC &&
				attacker->s.NPC_class == CLASS_VEHICLE &&
				attacker->m_pVehicle &&
				attacker->m_pVehicle->m_pPilot)
			{ //set vehicles pilot's killer as murderer
				murderer = &g_entities[attacker->m_pVehicle->m_pPilot->s.number];
				if (murderer->inuse && murderer->client && murderer->client->ps.otherKillerTime >= level.time)
				{
					murderer = &g_entities[murderer->client->ps.otherKiller];
					if (!murderer->inuse || !murderer->client)
					{
						murderer = NULL;
					}
				}
				else
				{
					murderer = NULL;
				}
			}
			else
			{
				murderer = &g_entities[attacker->s.number];
			}
		}
		else if (self->m_pVehicle->m_pPilot)
		{
			murderer = (gentity_t*)self->m_pVehicle->m_pPilot;
			if (!murderer->inuse || !murderer->client)
			{
				murderer = NULL;
			}
		}

		//no valid murderer.. just use self I guess
		if (!murderer)
		{
			murderer = self;
		}

		if (self->m_pVehicle->m_pVehicleInfo->hideRider)
		{//pilot is *inside* me, so kill him, too
			killEnt = (gentity_t*)self->m_pVehicle->m_pPilot;
			if (killEnt && killEnt->inuse && killEnt->client)
			{
				G_Damage(killEnt, murderer, murderer, NULL, killEnt->client->ps.origin, 99999, DAMAGE_NO_PROTECTION, MOD_BLASTER);
			}
			if (self->m_pVehicle->m_pVehicleInfo)
			{//FIXME: this wile got stuck in an endless loop, that's BAD!!  This method SUCKS (not initting "i", not incrementing it or using it directly, all sorts of badness), so I'm rewriting it
				//while (i < self->m_pVehicle->m_iNumPassengers)
				int numPass = self->m_pVehicle->m_iNumPassengers;
				for (i = 0; i < numPass && self->m_pVehicle->m_iNumPassengers; i++)
				{//go through and eject the last passenger
					killEnt = (gentity_t*)self->m_pVehicle->m_ppPassengers[self->m_pVehicle->m_iNumPassengers - 1];
					if (killEnt)
					{
						self->m_pVehicle->m_pVehicleInfo->Eject(self->m_pVehicle, (bgEntity_t*)killEnt, qtrue);
						if (killEnt->inuse&& killEnt->client)
						{
							G_Damage(killEnt, murderer, murderer, NULL, killEnt->client->ps.origin, 99999, DAMAGE_NO_PROTECTION, MOD_BLASTER);
						}
					}
				}
			}
		}
		killEnt = (gentity_t*)self->m_pVehicle->m_pDroidUnit;
		if (killEnt&& killEnt->inuse&& killEnt->client)
		{
			killEnt->flags &= ~FL_UNDYING;
			G_Damage(killEnt, murderer, murderer, NULL, killEnt->client->ps.origin, 99999, DAMAGE_NO_PROTECTION, MOD_BLASTER);
		}
	}

	self->client->ps.emplacedIndex = 0;

	G_BreakArm(self, 0); //unbreak anything we have broken
	self->client->ps.saberEntityNum = self->client->saberStoredIndex; //in case we died while our saber was knocked away.

	if (self->client->ps.weapon == WP_SABER && self->client->saberKnockedTime)
	{
		gentity_t* saberEnt = &g_entities[self->client->ps.saberEntityNum];
		self->client->saberKnockedTime = 0;
		saberReactivate(saberEnt, self);
		saberEnt->r.contents = CONTENTS_LIGHTSABER;
		saberEnt->think = saberBackToOwner;
		saberEnt->nextthink = level.time;
		G_RunObject(saberEnt);
	}

	self->client->bodyGrabIndex = ENTITYNUM_NONE;
	self->client->bodyGrabTime = 0;

	if (self->client->holdingObjectiveItem > 0)
	{ //carrying a siege objective item - make sure it updates and removes itself from us now in case this is an instant death-respawn situation
		gentity_t* objectiveItem = &g_entities[self->client->holdingObjectiveItem];

		if (objectiveItem->inuse && objectiveItem->think)
		{
			objectiveItem->think(objectiveItem);
		}
	}

	if ((self->client->inSpaceIndex && self->client->inSpaceIndex != ENTITYNUM_NONE) ||
		(self->client->ps.eFlags2 & EF2_SHIP_DEATH))
	{
		self->client->noCorpse = qtrue;
	}

	if (self->client->NPC_class != CLASS_VEHICLE
		&& self->client->ps.m_iVehicleNum)
	{ //I'm riding a vehicle
		//tell it I'm getting off
		gentity_t* veh = &g_entities[self->client->ps.m_iVehicleNum];

		if (veh->inuse && veh->client && veh->m_pVehicle)
		{
			veh->m_pVehicle->m_pVehicleInfo->Eject(veh->m_pVehicle, (bgEntity_t*)self, qtrue);

			if (veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
			{ //go into "die in ship" mode with flag
				self->client->ps.eFlags2 |= EF2_SHIP_DEATH;

				//put me over where my vehicle exploded
				G_SetOrigin(self, veh->client->ps.origin);
				VectorCopy(veh->client->ps.origin, self->client->ps.origin);
			}
		}
		//droids throw heads if they haven't yet
		switch (self->client->NPC_class)
		{
		case CLASS_R2D2:
			if (!trap->G2API_GetSurfaceRenderStatus(self->ghoul2, 0, "head"))
			{
				vec3_t	up;
				AngleVectors(self->r.currentAngles, NULL, NULL, up);
				G_PlayEffectID(G_EffectIndex("chunks/r2d2head_veh"), self->r.currentOrigin, up);
			}
			break;

		case CLASS_R5D2:
			if (!trap->G2API_GetSurfaceRenderStatus(self->ghoul2, 0, "head"))
			{
				vec3_t	up;
				AngleVectors(self->r.currentAngles, NULL, NULL, up);
				G_PlayEffectID(G_EffectIndex("chunks/r5d2head_veh"), self->r.currentOrigin, up);
			}
			break;
		default:
			break;
		}
	}

	if (self->NPC)
	{
		//turn off shielding for the assassin droids
		if (self->client&& self->client->NPC_class == CLASS_ASSASSIN_DROID)
		{
			BubbleShield_TurnOff(self);
		}
		if (self->client&& Jedi_WaitingAmbush(self))
		{//ambushing trooper
			self->client->noclip = qfalse;
		}
		NPC_FreeCombatPoint(self->NPC->combatPoint, qfalse);
		if (self->NPC->group)
		{
			//lastInGroup = (self->NPC->group->numGroup < 2);
			AI_GroupMemberKilled(self);
			AI_DeleteSelfFromGroup(self);
		}

		if (self->NPC->tempGoal)
		{
			G_FreeEntity(self->NPC->tempGoal);
			self->NPC->tempGoal = NULL;
		}
		//if this is a player's seeker droid, remove the item from their inv
		if (self->client && self->client->NPC_class == CLASS_SEEKER)
		{//this could be a player's seeker item
			if (self->client->leader //has leader
				&& self->client->leader->client //leader is a client
				&& self->client->leader->client->remote == self) //has us as their remote.
			{//yep, this is a player's seeker, switch the attacker entity pointer
				self->client->leader->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_SEEKER);
			}
		}
		if (0)
		{
			Boba_FlyStop(self);
		}
		if (self->s.NPC_class == CLASS_RANCOR || self->s.NPC_class == CLASS_WAMPA || self->s.NPC_class == CLASS_SAND_CREATURE)
		{
			Rancor_DropVictim(self);
		}
	}
	if (attacker && attacker->NPC && attacker->NPC->group && attacker->NPC->group->enemy == self)
	{
		attacker->NPC->group->enemy = NULL;
	}

	//Cheap method until/if I decide to put fancier stuff in (e.g. sabers falling out of hand and slowly
	//holstering on death like sp)
	if (self->client->ps.weapon == WP_SABER &&
		!self->client->ps.saberHolstered &&
		self->client->ps.saberEntityNum)
	{
		if (!self->client->ps.saberInFlight&&
			self->client->saber[0].soundOff)
		{
			G_Sound(self, CHAN_AUTO, self->client->saber[0].soundOff);
		}
		if (self->client->saber[1].soundOff&&
			self->client->saber[1].model[0])
		{
			G_Sound(self, CHAN_AUTO, self->client->saber[1].soundOff);
		}
	}

	//Use any target we had
	G_UseTargets(self, self);

	if (g_slowmoDuelEnd.integer && (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) && attacker&& attacker->inuse&& attacker->client)
	{
		if (!gDoSlowMoDuel)
		{
			gDoSlowMoDuel = qtrue;
			gSlowMoDuelTime = level.time;
		}
	}

	//Make sure the jetpack is turned off.
	Jetpack_Off(self);

	self->client->ps.heldByClient = 0;
	self->client->beingThrown = 0;
	self->client->doingThrow = 0;
	BG_ClearRocketLock(&self->client->ps);
	self->client->isHacking = 0;
	self->client->ps.hackingTime = 0;

	if (inflictor&& inflictor->activator && !inflictor->client && !attacker->client&&
		inflictor->activator->client&& inflictor->activator->inuse&&
		inflictor->s.weapon == WP_TURRET)
	{
		attacker = inflictor->activator;
	}

	if (self->client&& self->client->ps.isJediMaster)
	{
		wasJediMaster = qtrue;
	}

	//turn off flamethrower
	self->client->flameTime = 0;
	self->client->ps.userInt3 &= ~(1 << FLAG_FLAMETHROWER);
	self->client->ps.userInt3 &= ~(1 << FLAG_WRISTBLASTER);

	//this is set earlier since some of the previous function calls depend on this being
	//set for the g_spawn emergency entity override stuff.
	if (self->NPC)
	{
		self->NPC->timeOfDeath = level.time;//this will change - used for debouncing post-death events
	}
	//if he was charging or anything else, kill the sound
	G_MuteSound(self->s.number, CHAN_WEAPON);

	//FIXME: this may not be enough
	if (level.gametype == GT_SIEGE && meansOfDeath == MOD_TEAM_CHANGE)
		RemoveDetpacks(self);
	else
		BlowDetpacks(self); //blow detpacks if they're planted

	self->client->ps.fd.forceDeactivateAll = 1;

	if ((self == attacker || (attacker && !attacker->client)) &&
		(meansOfDeath == MOD_CRUSH || meansOfDeath == MOD_FALLING || meansOfDeath == MOD_TRIGGER_HURT || meansOfDeath == MOD_UNKNOWN) &&
		self->client->ps.otherKillerTime > level.time)
	{
		attacker = &g_entities[self->client->ps.otherKiller];
	}

	// check for an almost capture
	CheckAlmostCapture(self, attacker);

	self->client->ps.pm_type = PM_DEAD;
	self->client->ps.pm_flags &= ~PMF_STUCK_TO_WALL;

	if (attacker) {
		killer = attacker->s.number;
		if (attacker->client) {
			killerName = attacker->client->pers.netname;
		}
		else {
			killerName = "<non-client>";
		}
	}
	else {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if (killer < 0 || killer >= MAX_CLIENTS) {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if (meansOfDeath < 0 || meansOfDeath >= sizeof(modNames) / sizeof(modNames[0])) {
		obit = "<bad obituary>";
	}
	else {
		obit = modNames[meansOfDeath];
	}

	// log the victim and attacker's names with the method of death
	Com_sprintf(buf, sizeof(buf), "Kill: %i %i %i: %s killed ", killer, self->s.number, meansOfDeath, killerName);
	if (self->s.eType == ET_NPC) {
		// check for named NPCs
		if (self->targetname)
			Q_strcat(buf, sizeof(buf), va("%s (%s) by %s\n", self->NPC_type, self->targetname, obit));
		else
			Q_strcat(buf, sizeof(buf), va("%s by %s\n", self->NPC_type, obit));
	}
	else
		Q_strcat(buf, sizeof(buf), va("%s by %s\n", self->client->pers.netname, obit));
	G_LogPrintf("%s", buf);

	if (g_austrian.integer
		&& level.gametype == GT_DUEL
		&& level.numPlayingClients >= 2)
	{
		int spawnTime = (level.clients[level.sortedClients[0]].respawnTime > level.clients[level.sortedClients[1]].respawnTime) ? level.clients[level.sortedClients[0]].respawnTime : level.clients[level.sortedClients[1]].respawnTime;
		G_LogPrintf("Duel Kill Details:\n");
		G_LogPrintf("Kill Time: %d\n", level.time - spawnTime);
		G_LogPrintf("victim: %s, hits on enemy %d\n", self->client->pers.netname, self->client->ps.persistant[PERS_HITS]);
		if (attacker&& attacker->client)
		{
			G_LogPrintf("killer: %s, hits on enemy %d, health: %d\n", attacker->client->pers.netname, attacker->client->ps.persistant[PERS_HITS], attacker->health);
			//also - if MOD_SABER, list the animation and saber style
			if (meansOfDeath == MOD_SABER)
			{
				G_LogPrintf("killer saber style: %d, killer saber anim %s\n", attacker->client->ps.fd.saberAnimLevel, animTable[(attacker->client->ps.torsoAnim)].name);
			}
		}
	}

	G_LogWeaponKill(killer, meansOfDeath);
	G_LogWeaponDeath(self->s.number, self->s.weapon);
	if (attacker && attacker->client && attacker->inuse)
	{
		G_LogWeaponFrag(killer, self->s.number);
	}

	// broadcast the death event to everyone
	if (self->s.eType != ET_NPC && !g_noPDuelCheck)
	{
		ent = G_TempEntity(self->r.currentOrigin, EV_OBITUARY);
		ent->s.eventParm = meansOfDeath;
		ent->s.otherEntityNum = self->s.number;
		ent->s.otherEntityNum2 = killer;
		ent->r.svFlags = SVF_BROADCAST;	// send to everyone
		ent->s.isJediMaster = wasJediMaster;
	}

	self->enemy = attacker;

	self->client->ps.persistant[PERS_KILLED]++;

	if (self == attacker)
	{
		self->client->ps.fd.suicides++;
	}

	if (attacker && attacker->client)
	{
		if (self->s.number < MAX_CLIENTS)
		{//only remember real clients
			attacker->client->lastkilled_client = self->s.number;
		}

		G_CheckVictoryScript(attacker);

		if (self->s.number >= MAX_CLIENTS//not a player client
			&& self->client //an NPC client
			&& self->client->NPC_class != CLASS_VEHICLE //not a vehicle
			&& self->s.m_iVehicleNum)//a droid in a vehicle
		{
			//no credit for droid, you do get credit for the vehicle kill and the pilot (2 points)
		}
		else if (meansOfDeath == MOD_COLLISION || meansOfDeath == MOD_VEH_EXPLOSION)
		{
			//no credit for veh-veh collisions?
		}
		else if (attacker == self || OnSameTeam(self, attacker))
		{
			if (level.gametype == GT_DUEL)
			{ //in duel, if you kill yourself, the person you are dueling against gets a kill for it
				int otherClNum = -1;
				if (level.sortedClients[0] == self->s.number)
				{
					otherClNum = level.sortedClients[1];
				}
				else if (level.sortedClients[1] == self->s.number)
				{
					otherClNum = level.sortedClients[0];
				}

				if (otherClNum >= 0 && otherClNum < MAX_CLIENTS &&
					g_entities[otherClNum].inuse && g_entities[otherClNum].client &&
					otherClNum != attacker->s.number)
				{
					AddScore(&g_entities[otherClNum], self->r.currentOrigin, 1);
				}
				else
				{
					AddScore(attacker, self->r.currentOrigin, -1);
				}
			}
			else
			{
				AddScore(attacker, self->r.currentOrigin, -1);
			}
			if (level.gametype == GT_JEDIMASTER)
			{
				if (self->client && self->client->ps.isJediMaster)
				{ //killed ourself so return the saber to the original position
				  //(to avoid people jumping off ledges and making the saber
				  //unreachable for 60 seconds)
					ThrowSaberToAttacker(self, NULL);
					self->client->ps.isJediMaster = qfalse;
				}
			}
		}
		else
		{
			if (level.gametype == GT_JEDIMASTER)
			{
				if ((attacker->client && attacker->client->ps.isJediMaster) ||
					(self->client && self->client->ps.isJediMaster))
				{
					AddScore(attacker, self->r.currentOrigin, 1);
					AddFatigueKillBonus(attacker, self);

					if (self->client && self->client->ps.isJediMaster)
					{
						ThrowSaberToAttacker(self, attacker);
						self->client->ps.isJediMaster = qfalse;
					}
				}
				else
				{
					gentity_t* jmEnt = G_GetJediMaster();

					if (jmEnt && jmEnt->client)
					{
						AddScore(jmEnt, self->r.currentOrigin, 1);
						AddFatigueKillBonus(attacker, self);
					}
				}
			}
			else
			{
				AddScore(attacker, self->r.currentOrigin, 1);
				AddFatigueKillBonus(attacker, self);
			}

			if (meansOfDeath == MOD_FORCE_LIGHTNING && !(self->r.svFlags & SVF_BOT || self->s.eType == ET_NPC))
			{// play humiliation on player
				attacker->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
				attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;
				// also play humiliation on target
				self->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_GAUNTLETREWARD;
			}

			// check for two kills in a short amount of time
			// if this is close enough to the last kill, give a reward sound
			if (level.time - attacker->client->lastKillTime < CARNAGE_REWARD_TIME)
			{// play excellent on player
				attacker->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
				attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;
			}
			attacker->client->lastKillTime = level.time;
		}
	}
	else
	{
		if (self->client && self->client->ps.isJediMaster)
		{ //killed ourself so return the saber to the original position
		  //(to avoid people jumping off ledges and making the saber
		  //unreachable for 60 seconds)
			ThrowSaberToAttacker(self, NULL);
			self->client->ps.isJediMaster = qfalse;
		}

		if (level.gametype == GT_DUEL)
		{ //in duel, if you kill yourself, the person you are dueling against gets a kill for it
			int otherClNum = -1;
			if (level.sortedClients[0] == self->s.number)
			{
				otherClNum = level.sortedClients[1];
			}
			else if (level.sortedClients[1] == self->s.number)
			{
				otherClNum = level.sortedClients[0];
			}

			if (otherClNum >= 0 && otherClNum < MAX_CLIENTS &&
				g_entities[otherClNum].inuse && g_entities[otherClNum].client &&
				otherClNum != self->s.number)
			{
				AddScore(&g_entities[otherClNum], self->r.currentOrigin, 1);
			}
			else
			{
				AddScore(self, self->r.currentOrigin, -1);
			}
		}
		else
		{
			AddScore(self, self->r.currentOrigin, -1);
		}
	}

	// Add team bonuses
	Team_FragBonuses(self, inflictor, attacker);

	self->s.powerups &= ~PW_REMOVE_AT_DEATH;//removes everything but electricity and force push

	// if I committed suicide, the flag does not fall, it returns.
	if (meansOfDeath == MOD_SUICIDE) {
		if (self->client->ps.powerups[PW_NEUTRALFLAG]) {		// only happens in One Flag CTF
			Team_ReturnFlag(TEAM_FREE);
			self->client->ps.powerups[PW_NEUTRALFLAG] = 0;
		}
		else if (self->client->ps.powerups[PW_REDFLAG]) {		// only happens in standard CTF
			Team_ReturnFlag(TEAM_RED);
			self->client->ps.powerups[PW_REDFLAG] = 0;
		}
		else if (self->client->ps.powerups[PW_BLUEFLAG]) {	// only happens in standard CTF
			Team_ReturnFlag(TEAM_BLUE);
			self->client->ps.powerups[PW_BLUEFLAG] = 0;
		}
	}

	contents = trap->PointContents(self->r.currentOrigin, -1);
	if (!(contents & CONTENTS_NODROP) && !self->client->ps.fallingToDeath || self->client->NPC_class != CLASS_GALAKMECH)
	{
		TossClientItems(self);
	}
	else {
		if (self->client->ps.powerups[PW_NEUTRALFLAG]) {		// only happens in One Flag CTF
			Team_ReturnFlag(TEAM_FREE);
			self->client->ps.powerups[PW_NEUTRALFLAG] = 0;
		}
		else if (self->client->ps.powerups[PW_REDFLAG]) {		// only happens in standard CTF
			Team_ReturnFlag(TEAM_RED);
			self->client->ps.powerups[PW_REDFLAG] = 0;
		}
		else if (self->client->ps.powerups[PW_BLUEFLAG]) {	// only happens in standard CTF
			Team_ReturnFlag(TEAM_BLUE);
			self->client->ps.powerups[PW_BLUEFLAG] = 0;
		}
	}

	if (MOD_TEAM_CHANGE == meansOfDeath)
	{
		// Give them back a point since they didn't really die.
		AddScore(self, self->r.currentOrigin, 1);
	}
	else
	{
		Cmd_Score_f(self);		// show scores
	}

	if (MOD_SPECTATE == meansOfDeath)
	{
		if (g_spectate_keep_score.integer >= 1
			&& level.gametype != GT_DUEL
			&& level.gametype != GT_POWERDUEL
			&& level.gametype != GT_SIEGE)
		{
			AddScore(self, self->r.currentOrigin, 1);
			self->client->sess.losses -= 1;
		}
	}

	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for (i = 0; i < level.maxclients; i++) {
		gclient_t* cl = &level.clients[i];
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		if (cl->sess.sessionTeam != TEAM_SPECTATOR) {
			continue;
		}
		if (cl->sess.spectatorClient == self->s.number) {
			Cmd_Score_f(g_entities + i);
		}
	}

	self->takedamage = qtrue;	// can still be gibbed

	self->s.weapon = WP_NONE;
	self->s.powerups = 0;
	if (self->s.eType != ET_NPC)
	{ //handled differently for NPCs
		self->r.contents = CONTENTS_CORPSE;
	}
	self->client->ps.zoomMode = 0;	// Turn off zooming when we die

	self->s.loopSound = 0;
	self->s.loopIsSoundset = qfalse;

	if (self->s.eType != ET_NPC)
	{ //handled differently for NPCs
		self->r.maxs[2] = -8;
		//since a player died we need to play the death music if DMS is on
		SetDMSState(DM_DEATH);
	}

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	self->client->respawnTime = level.time + 1700;
	ScalePlayer(self, self->client->pers.botmodelscale);

	// remove powerups
	memset(self->client->ps.powerups, 0, sizeof(self->client->ps.powerups));

	self->client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
	self->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;


	{
		// normal death

		static int deathAnim;

		anim = G_PickDeathAnim(self, self->pos1, damage, meansOfDeath, HL_NONE);

		if (anim >= 1)
		{ //Some droids don't have death anims
			// for the no-blood option, we need to prevent the health
			// from going to gib level
			if (self->health <= GIB_HEALTH)
			{
				self->health = GIB_HEALTH + 1;
			}

			self->client->respawnTime = level.time + 1000;//((self->client->animations[anim].numFrames*40)/(50.0f / self->client->animations[anim].frameLerp))+300;

			sPMType = self->client->ps.pm_type;
			self->client->ps.pm_type = PM_NORMAL; //don't want pm type interfering with our setanim calls.

			if (self->inuse)
			{ //not disconnecting
				G_SetAnim(self, NULL, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0);
			}

			self->client->ps.pm_type = sPMType;

			//if ((dflags&DAMAGE_DISMEMBER)
			//	&& G_DoDismemberment(self, self->pos1, meansOfDeath, damage, hitLoc))
			//{//we did dismemberment and our death anim is okay to override
			//	if (hitLoc == HL_HAND_RT && self->locationDamage[hitLoc] >= Q3_INFINITE && self->client->ps.groundEntityNum != ENTITYNUM_NONE)
			//	{//just lost our right hand and we're on the ground, use the special anim
			//		NPC_SetAnim(self, SETANIM_BOTH, BOTH_RIGHTHANDCHOPPEDOFF, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			//	}
			//}

			if (meansOfDeath == MOD_SABER || (meansOfDeath == MOD_TURBLAST) || (meansOfDeath == MOD_FLECHETTE) || (meansOfDeath == MOD_FLECHETTE_ALT_SPLASH) || (meansOfDeath == MOD_CONC_ALT) || (meansOfDeath == MOD_THERMAL_SPLASH) || (meansOfDeath == MOD_TRIP_MINE_SPLASH) || (meansOfDeath == MOD_TIMED_MINE_SPLASH) || (meansOfDeath == MOD_TELEFRAG) || (meansOfDeath == MOD_CRUSH) || (meansOfDeath == MOD_MELEE && G_HeavyMelee(attacker)))//saber or heavy melee (claws)
			{ //update the anim on the actual skeleton (so bolt point will reflect the correct position) and then check for dismem
				G_UpdateClientAnims(self, 1.0f);
				G_CheckForDismemberment(self, attacker, self->pos1, damage, anim, qfalse);
			}
			else if (meansOfDeath == MOD_ROCKET || (meansOfDeath == MOD_ROCKET_SPLASH) || (meansOfDeath == MOD_ROCKET_HOMING) || (meansOfDeath == MOD_ROCKET_HOMING_SPLASH) || (meansOfDeath == MOD_THERMAL) || (meansOfDeath == MOD_DET_PACK_SPLASH) || (meansOfDeath == MOD_TELEFRAG) || (meansOfDeath == MOD_TRIGGER_HURT) || (meansOfDeath == MOD_LAVA))
			{
				G_UpdateClientAnims(self, 1.0f);
				G_CheckForblowingup(self, attacker, self->pos1, damage, anim, qfalse);
			}
			else if (meansOfDeath == MOD_BODYSHOT)
			{
				GibEntity(self, attacker);
				G_UpdateClientAnims(self, 1.0f);
				G_CheckForDismemberment(self, attacker, self->pos1, damage, anim, qfalse);
			}
			else if (meansOfDeath == MOD_HEADSHOT)
			{
				GibEntity_Headshot(self, attacker);
				G_UpdateClientAnims(self, 1.0f);
				G_CheckForDismemberment(self, attacker, self->pos1, damage, anim, qfalse);
			}
			else
			{
				self->client->noHead = qfalse;
			}
		}


		if (wasJediMaster)
		{
			G_AddEvent(self, Q_irand(EV_DEATH1, EV_DEATH3) + deathAnim, 1);
		}
		else
		{
			G_AddEvent(self, Q_irand(EV_DEATH1, EV_DEATH3) + deathAnim, 0);
		}

		if (self != attacker)
		{ //don't make NPCs want to murder you on respawn for killing yourself!
			G_DeathAlert(self, attacker);
		}

		// the body can still be gibbed
		if (!self->NPC)
		{ //don't remove NPCs like this!
			self->die = body_die;
		}

		//It won't gib, it will disintegrate (because this is Star Wars).
		self->takedamage = qtrue;

		// globally cycle through the different death animations
		deathAnim = (deathAnim + 1) % 3;
	}

	if (self->NPC)
	{//If an NPC, make sure we start running our scripts again- this gets set to infinite while we fall to our deaths
		self->NPC->nextBStateThink = level.time;
	}

	if (G_ActivateBehavior(self, BSET_DEATH))
	{
		//deathScript = qtrue;
	}

	if (self->NPC && (self->NPC->scriptFlags & SCF_FFDEATH))
	{
		if (G_ActivateBehavior(self, BSET_FFDEATH))
		{//FIXME: should running this preclude running the normal deathscript?
			//deathScript = qtrue;
		}
		G_UseTargets2(self, self, self->target4);
	}

	/*
	if ( !deathScript && !(self->svFlags&SVF_KILLED_SELF) )
	{
		//Should no longer run scripts
		//WARNING!!! DO NOT DO THIS WHILE RUNNING A SCRIPT, ICARUS WILL CRASH!!!
		//FIXME: shouldn't ICARUS handle this internally?
		ICARUS_FreeEnt(self);
	}
	*/
	//rwwFIXMEFIXME: Do this too?

	// Free up any timers we may have on us.
	TIMER_Clear2(self);

	trap->LinkEntity((sharedEntity_t*)self);

	if (level.gametype == GT_SINGLE_PLAYER)
	{// Start any necessary death fx for this entity
		DeathFX(self);
	}
	else
	{// Start any necessary death fx for this entity
		if (g_deathfx.integer)//As requested by players and mattfiler
		{// Start any necessary death fx for this entity
			DeathFXEXTRA(self);
		}
		else
		{
			DeathFX(self);
		}
	}


	if (level.gametype == GT_POWERDUEL && !g_noPDuelCheck)
	{ //powerduel checks
		if (self->client->sess.duelTeam == DUELTEAM_LONE)
		{ //automatically means a win as there is only one
			G_AddPowerDuelScore(DUELTEAM_DOUBLE, 1);
			G_AddPowerDuelLoserScore(DUELTEAM_LONE, 1);
			g_endPDuel = qtrue;
		}
		else if (self->client->sess.duelTeam == DUELTEAM_DOUBLE)
		{
			gentity_t* check;
			qboolean heLives = qfalse;

			for (i = 0; i < MAX_CLIENTS; i++)
			{
				check = &g_entities[i];
				if (check->inuse && check->client && check->s.number != self->s.number &&
					check->client->pers.connected == CON_CONNECTED && !check->client->iAmALoser &&
					check->client->ps.stats[STAT_HEALTH] > 0 &&
					check->client->sess.sessionTeam != TEAM_SPECTATOR &&
					check->client->sess.duelTeam == DUELTEAM_DOUBLE)
				{ //still an active living paired duelist so it's not over yet.
					heLives = qtrue;
					break;
				}
			}

			if (!heLives)
			{ //they're all dead, give the lone duelist the win.
				G_AddPowerDuelScore(DUELTEAM_LONE, 1);
				G_AddPowerDuelLoserScore(DUELTEAM_DOUBLE, 1);
				g_endPDuel = qtrue;
			}
		}
	}
}


qboolean G_CheckForStrongAttackMomentum(gentity_t * self)
{//see if our saber attack has too much momentum to be interrupted
	if (PM_PowerLevelForSaberAnims(&self->client->ps) > FORCE_LEVEL_2)
	{//strong attacks can't be interrupted
		if (PM_InAnimForSaberMove(self->client->ps.torsoAnim, self->client->ps.saberMove))
		{//our saberMove was not already interupted by some other anim (like pain)
			if (PM_SaberInStart(self->client->ps.saberMove))
			{
				float animLength = PM_AnimLength(0, (animNumber_t)self->client->ps.torsoAnim);
				if (animLength - self->client->ps.torsoTimer > 750)
				{//start anim is already 3/4 of a second into it, can't interrupt it now
					return qtrue;
				}
			}
			else if (PM_SaberInReturn(self->client->ps.saberMove))
			{
				if (self->client->ps.torsoTimer > 750)
				{//still have a good amount of time left in the return anim, can't interrupt it
					return qtrue;
				}
			}
			else
			{//cannot interrupt actual transitions and attacks
				return qtrue;
			}
		}
	}
	return qfalse;
}

extern void G_SpeechEvent(gentity_t * self, int event);
void PlayerPain(gentity_t * self, gentity_t * attacker, int damage)
{
	gentity_t* other;
	other = g_entities;

	if (self->client->NPC_class == CLASS_ATST)
	{//different kind of pain checking altogether
		G_ATSTCheckPain(self, other, damage);

		//int blasterTest = trap->G2API_GetSurfaceRenderStatus(self->ghoul2, 0, "head_light_blaster_cann");
		//int chargerTest = trap->G2API_GetSurfaceRenderStatus(self->ghoul2, 0, "head_concussion_charger");
		//
		//if (blasterTest && chargerTest)
		//{//lost both side guns
		// //take away that weapon
		//	//self->client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_ATST_SIDE);
		//	////switch to primary guns
		//	//if (self->client->ps.weapon == WP_ATST_SIDE)
		//	//{
		//	//	CG_ChangeWeapon(WP_ATST_MAIN);
		//	//}
		//}
	}
	else
	{
		// play an apropriate pain sound
		if (level.time > self->painDebounceTime && !(self->flags & FL_GODMODE))
		{//first time hit this frame and not in godmode
			self->client->ps.damageEvent++;
			if (!trap->ICARUS_TaskIDPending((sharedEntity_t*)self, TID_CHAN_VOICE) && self->client)
			{
				if (self->client->damage_blood)
				{//took damage myself, not just armor
					if (self->methodOfDeath == MOD_GAS)
					{
						//SIGH... because our choke sounds are inappropriately long, I have to debounce them in code!
						if (TIMER_Done(self, "gasChokeSound"))
						{
							TIMER_Set(self, "gasChokeSound", Q_irand(1000, 2000));
							G_SpeechEvent(self, Q_irand(EV_CHOKE1, EV_CHOKE3));
						}
						if (self->painDebounceTime <= level.time)
						{
							self->painDebounceTime = level.time + 50;
						}
					}
					else
					{
						G_AddEvent(self, EV_PAIN, self->health);
					}
				}
			}
		}
		if (damage != -1 && (self->methodOfDeath == MOD_MELEE || damage == 0 || (Q_irand(0, 10) <= damage && self->client->damage_blood)))
		{//-1 == don't play pain anim
			if ((((self->methodOfDeath == MOD_SABER || self->methodOfDeath == MOD_MELEE) && self->client->damage_blood) || self->methodOfDeath == MOD_CRUSH) && (self->s.weapon == WP_SABER || self->s.weapon == WP_MELEE))//FIXME: not only if using saber, but if in third person at all?  But then 1st/third person functionality is different...
			{//FIXME: only strong-level saber attacks should make me play pain anim?
				if (!G_CheckForStrongAttackMomentum(self) && !PM_SpinningSaberAnim(self->client->ps.legsAnim)
					&& !PM_SaberInSpecialAttack(self->client->ps.torsoAnim)
					&& !PM_InKnockDown(&self->client->ps))
				{//strong attacks and spins cannot be interrupted by pain, no pain when in knockdown
					int	parts = SETANIM_BOTH;
					if (self->client->ps.groundEntityNum != ENTITYNUM_NONE &&
						!PM_SpinningSaberAnim(self->client->ps.legsAnim) &&
						!PM_FlippingAnim(self->client->ps.legsAnim) &&
						!PM_InSpecialJump(self->client->ps.legsAnim) &&
						!PM_RollingAnim(self->client->ps.legsAnim) &&
						!PM_CrouchAnim(self->client->ps.legsAnim) &&
						!PM_RunningAnim(self->client->ps.legsAnim))
					{//if on a surface and not in a spin or flip, play full body pain
						parts = SETANIM_BOTH;
					}
					else
					{//play pain just in torso
						parts = SETANIM_TORSO;
					}
					if (self->painDebounceTime < level.time)
					{
						//temp HACK: these are the only 2 pain anims that look good when holding a saber
						NPC_SetAnim(self, parts, PM_PickAnim(self->localAnimIndex, BOTH_PAIN2, BOTH_PAIN3), SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						self->client->ps.saberMove = LS_READY;//don't finish whatever saber move you may have been in
						 //WTF - insn't working
						/*if (self->health < 10 && d_slowmodeath->integer > 5)
						{
							G_StartMatrixEffect(self);
						}*/
					}
					if (parts == SETANIM_BOTH && (damage > 30 || (self->painDebounceTime > level.time && damage > 10)))
					{//took a lot of damage in 1 hit //or took 2 hits in quick succession
						self->aimDebounceTime = level.time + self->client->ps.torsoTimer;
						self->client->ps.pm_time = self->client->ps.torsoTimer;
						//self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
					}
					self->client->ps.weaponTime = self->client->ps.torsoTimer;
					self->attackDebounceTime = level.time + self->client->ps.torsoTimer;
				}
				self->painDebounceTime = level.time + self->client->ps.torsoTimer;
			}
		}
	}
	if (self->methodOfDeath != MOD_GAS && self->painDebounceTime <= level.time)
	{
		self->painDebounceTime = level.time + 700;
	}
}

/*
================
CheckArmor
================
*/
int CheckArmor(gentity_t * ent, int damage, int dflags, int mod)
{
	gclient_t* client;
	int			save;
	int			count;

	if (!damage)
		return 0;

	client = ent->client;

	if (!client)
		return 0;

	if ((dflags & DAMAGE_NO_ARMOR))
	{
		// If this isn't a vehicle, leave.
		if (client->NPC_class != CLASS_VEHICLE)
		{
			return 0;
		}
	}

	if (client->NPC_class == CLASS_ASSASSIN_DROID)
	{
		// The Assassin Always Completely Ignores These Damage Types
		//-----------------------------------------------------------
		if (mod == MOD_GAS || mod == MOD_BURNING || mod == MOD_LAVA || mod == MOD_SLIME || mod == MOD_WATER ||
			/*mod==MOD_FORCE_GRIP || mod==MOD_FORCE_DRAIN ||*/ mod == MOD_SEEKER || mod == MOD_MELEE ||
			mod == MOD_BOWCASTER || mod == MOD_BRYAR_PISTOL || mod == MOD_BRYAR_PISTOL_ALT || mod == MOD_BLASTER ||
			mod == MOD_DISRUPTOR_SNIPER || mod == MOD_BOWCASTER || mod == MOD_REPEATER || mod == MOD_REPEATER_ALT)
		{
			return damage;
		}

		// The Assassin Always Takes Half Of These Damage Types
		//------------------------------------------------------
		if (mod == MOD_GAS || mod == MOD_LAVA || mod == MOD_SLIME || mod == MOD_WATER || mod == MOD_BURNING)
		{
			return damage / 2;
		}

		// If The Shield Is Not On, No Additional Protection
		//---------------------------------------------------
		if (!(ent->flags & FL_SHIELDED))
		{
			// He Does Ignore Half Saber Damage, Even Shield Down
			//----------------------------------------------------
			if (mod == MOD_SABER)
			{
				return (int)((float)(damage) * 0.75f);
			}
			return 0;
		}

		// If The Shield Is Up, He Ignores These Damage Types
		//----------------------------------------------------
		if (mod == MOD_SABER || mod == MOD_FLECHETTE || mod == MOD_FLECHETTE_ALT_SPLASH || mod == MOD_DISRUPTOR)
		{
			return damage;
		}

		// The Demp Completely Destroys The Shield
		//-----------------------------------------
		if (mod == MOD_DEMP2 || mod == MOD_DEMP2_ALT)
		{
			client->ps.stats[STAT_ARMOR] = 0;
			return 0;
		}

		// Otherwise, The Shield Absorbs As Much Damage As Possible
		//----------------------------------------------------------
		int	previousArmor = client->ps.stats[STAT_ARMOR];
		client->ps.stats[STAT_ARMOR] -= damage;
		if (client->ps.stats[STAT_ARMOR] < 0)
		{
			client->ps.stats[STAT_ARMOR] = 0;
		}
		return (previousArmor - client->ps.stats[STAT_ARMOR]);
	}



	if (client->NPC_class == CLASS_GALAKMECH)
	{//special case
		if (client->ps.stats[STAT_ARMOR] <= 0)
		{//no shields
			client->ps.powerups[PW_GALAK_SHIELD] = 0;
			return 0;
		}
		else
		{//shields take all the damage
			client->ps.stats[STAT_ARMOR] -= damage;
			if (client->ps.stats[STAT_ARMOR] <= 0)
			{
				client->ps.powerups[PW_GALAK_SHIELD] = 0;
				client->ps.stats[STAT_ARMOR] = 0;
			}
			return damage;
		}
	}
	else
	{
		if (client->NPC_class == CLASS_VEHICLE
			&& ent->m_pVehicle
			&& ent->client->ps.electrifyTime > level.time)
		{//ion-cannon has disabled this ship's shields, take damage on hull!
			return 0;
		}
		// armor
		count = client->ps.stats[STAT_ARMOR];

		// No damage to entity until armor is at less than 50% strength
		if (count > (client->ps.stats[STAT_MAX_HEALTH] / 2)) // MAX_HEALTH is considered max armor. Or so I'm told.
		{
			save = damage;
		}
		else
		{
			if (!ent->s.number && client->NPC_class == CLASS_ATST)
			{//player in ATST... armor takes *all* the damage
				save = damage;
			}
			else
			{
				save = ceil((float)damage * ARMOR_PROTECTION);
			}
		}

		//Always round up
		if (damage == 1)
		{
			if (client->ps.stats[STAT_ARMOR] > 0)
				client->ps.stats[STAT_ARMOR] -= save;
			//WTF? returns false 0 if armor absorbs only 1 point of damage
			return 0;
		}

		if (save >= count)
			save = count;

		if (!save)
			return 0;

		if (dflags& DAMAGE_HALF_ARMOR_REDUCTION)		// Armor isn't whittled so easily by sniper shots.
		{
			client->ps.stats[STAT_ARMOR] -= (int)(save * ARMOR_REDUCTION_FACTOR);
		}
		else
		{
			client->ps.stats[STAT_ARMOR] -= save;
		}

		return save;
	}
}

void G_Knockdown(gentity_t * self, gentity_t * attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock);
extern qboolean G_StandardHumanoid(gentity_t * self);
void G_CheckKnockdown(gentity_t * targ, gentity_t * attacker, vec3_t newDir, int dflags, int mod)
{
	if (!targ || !attacker)
	{
		return;
	}
	if (!(dflags & DAMAGE_RADIUS))
	{//not inherently explosive damage, check mod
		if (mod != MOD_REPEATER_ALT
			&& mod != MOD_FLECHETTE_ALT_SPLASH
			&& mod != MOD_ROCKET
			&& mod != MOD_ROCKET_HOMING
			&& mod != MOD_CONC
			&& mod != MOD_CONC_ALT
			&& mod != MOD_THERMAL
			&& mod != MOD_DET_PACK_SPLASH
			&& mod != MOD_TRIP_MINE_SPLASH
			&& mod != MOD_VEH_EXPLOSION)
		{
			return;
		}
	}

	if (!targ->client || targ->client->NPC_class == CLASS_PROTOCOL || !G_StandardHumanoid(targ))
	{
		return;
	}

	if (targ->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{//already in air
		return;
	}

	if (!targ->s.number)
	{//player less likely to be knocked down
		if (targ->client->ps.zoomMode)
		{//never if not in chase camera view (so more likely with saber out)
			return;
		}
		if (Q_irand(0, 1))
		{
			return;
		}
	}

	float strength = VectorLength(targ->client->ps.velocity);
	if (targ->client->ps.velocity[2] > 100 && strength > Q_irand(150, 350))
	{//explosive concussion possibly do a knockdown?
		G_Knockdown(targ, attacker, newDir, strength, qtrue);
	}
}

void G_ApplyKnockback(gentity_t * targ, vec3_t newDir, float knockback)
{
	vec3_t	kvel;
	float	mass;

	if (targ
		&& targ->client
		&& (targ->client->NPC_class == CLASS_ATST
			|| targ->client->NPC_class == CLASS_RANCOR
			|| targ->client->NPC_class == CLASS_SAND_CREATURE
			|| targ->client->NPC_class == CLASS_WAMPA))
	{//much to large to *ever* throw
		return;
	}

	//--- TEMP TEST
	if (newDir[2] <= 0.0f)
	{

		newDir[2] += ((0.0f - newDir[2]) * 1.2f);
	}

	knockback *= 2.0f;

	if (knockback > 120)
	{
		knockback = 120;
	}
	//--- TEMP TEST

	if (targ->physicsBounce > 0.0f)	//overide the mass
		mass = targ->physicsBounce;
	else
		mass = 200;

	if (g_gravity.value > 0)
	{
		VectorScale(newDir, g_knockback.value * (float)knockback / mass * 0.8, kvel);
		kvel[2] = newDir[2] * g_knockback.value * (float)knockback / mass * 1.5;
	}
	else
	{
		VectorScale(newDir, g_knockback.value * (float)knockback / mass, kvel);
	}

	if (targ->client)
	{
		VectorAdd(targ->client->ps.velocity, kvel, targ->client->ps.velocity);
	}
	else if (targ->s.pos.trType != TR_STATIONARY && targ->s.pos.trType != TR_LINEAR_STOP && targ->s.pos.trType != TR_NONLINEAR_STOP)
	{
		VectorAdd(targ->s.pos.trDelta, kvel, targ->s.pos.trDelta);
		VectorCopy(targ->r.currentOrigin, targ->s.pos.trBase);
		targ->s.pos.trTime = level.time;
	}

	// set the timer so that the other client can't cancel
	// out the movement immediately
	if (targ->client && !targ->client->ps.pm_time)
	{
		int		t;

		t = knockback * 2;
		if (t < 50) {
			t = 50;
		}
		if (t > 200) {
			t = 200;
		}
		targ->client->ps.pm_time = t;
		targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}
}

static int G_CheckForLedge(gentity_t * self, vec3_t fallCheckDir, float checkDist)
{//racc - this function checks to see if there is a ledge/cliff/empty in fallCheckDir @ checkDist away from the player.
 //This also checks to see if there's a clear path between the player and the point from which the fall check is made.
 //Returns the length of the drop.
	vec3_t	start, end;
	trace_t	tr;

	VectorMA(self->r.currentOrigin, checkDist, fallCheckDir, end);
	//Should have clip burshes masked out by now and have bbox resized to death size
	trap->Trace(&tr, self->r.currentOrigin, self->r.mins, self->r.maxs, end, self->s.number, self->clipmask, qfalse, 0, 0);
	if (tr.allsolid || tr.startsolid)
	{
		return 0;
	}
	VectorCopy(tr.endpos, start);
	VectorCopy(start, end);
	end[2] -= 256;

	trap->Trace(&tr, start, self->r.mins, self->r.maxs, end, self->s.number, self->clipmask, qfalse, 0, 0);
	if (tr.allsolid || tr.startsolid)
	{
		return 0;
	}
	if (tr.fraction >= 1.0)
	{
		return (start[2] - tr.endpos[2]);
	}
	return 0;
}

/*
================
RaySphereIntersections
================
*/
int RaySphereIntersections(vec3_t origin, float radius, vec3_t point, vec3_t dir, vec3_t intersections[2]) {
	float b, c, d, t;

	//	| origin - (point + t * dir) | = radius
	//	a = dir[0]^2 + dir[1]^2 + dir[2]^2;
	//	b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	//	c = (point[0] - origin[0])^2 + (point[1] - origin[1])^2 + (point[2] - origin[2])^2 - radius^2;

	// normalize dir so a = 1
	VectorNormalize(dir);
	b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	c = (point[0] - origin[0]) * (point[0] - origin[0]) +
		(point[1] - origin[1]) * (point[1] - origin[1]) +
		(point[2] - origin[2]) * (point[2] - origin[2]) -
		radius * radius;

	d = b * b - 4 * c;
	if (d > 0) {
		t = (-b + sqrt(d)) / 2;
		VectorMA(point, t, dir, intersections[0]);
		t = (-b - sqrt(d)) / 2;
		VectorMA(point, t, dir, intersections[1]);
		return 2;
	}
	else if (d == 0) {
		t = (-b) / 2;
		VectorMA(point, t, dir, intersections[0]);
		return 1;
	}
	return 0;
}

/*
===================================
rww - beginning of the majority of the dismemberment and location based damage code.
===================================
*/
char* hitLocName[HL_MAX] =
{
	"none",	//HL_NONE = 0,
	"right foot",	//HL_FOOT_RT,
	"left foot",	//HL_FOOT_LT,
	"right leg",	//HL_LEG_RT,
	"left leg",	//HL_LEG_LT,
	"waist",	//HL_WAIST,
	"back right shoulder",	//HL_BACK_RT,
	"back left shoulder",	//HL_BACK_LT,
	"back",	//HL_BACK,
	"front right shouler",	//HL_CHEST_RT,
	"front left shoulder",	//HL_CHEST_LT,
	"chest",	//HL_CHEST,
	"right arm",	//HL_ARM_RT,
	"left arm",	//HL_ARM_LT,
	"right hand",	//HL_HAND_RT,
	"left hand",	//HL_HAND_LT,
	"head",	//HL_HEAD
	"generic1",	//HL_GENERIC1,
	"generic2",	//HL_GENERIC2,
	"generic3",	//HL_GENERIC3,
	"generic4",	//HL_GENERIC4,
	"generic5",	//HL_GENERIC5,
	"generic6"	//HL_GENERIC6
};

void G_GetDismemberLoc(gentity_t * self, vec3_t boltPoint, int limbType)
{ //Just get the general area without using server-side ghoul2
	vec3_t fwd, right, up;

	AngleVectors(self->r.currentAngles, fwd, right, up);

	VectorCopy(self->r.currentOrigin, boltPoint);

	switch (limbType)
	{
	case G2_MODELPART_HEAD:
		boltPoint[0] += up[0] * 24;
		boltPoint[1] += up[1] * 24;
		boltPoint[2] += up[2] * 24;
		break;
	case G2_MODELPART_WAIST:
		boltPoint[0] += up[0] * 4;
		boltPoint[1] += up[1] * 4;
		boltPoint[2] += up[2] * 4;
		break;
	case G2_MODELPART_LARM:
		boltPoint[0] += up[0] * 18;
		boltPoint[1] += up[1] * 18;
		boltPoint[2] += up[2] * 18;

		boltPoint[0] -= right[0] * 10;
		boltPoint[1] -= right[1] * 10;
		boltPoint[2] -= right[2] * 10;
		break;
	case G2_MODELPART_RARM:
		boltPoint[0] += up[0] * 18;
		boltPoint[1] += up[1] * 18;
		boltPoint[2] += up[2] * 18;

		boltPoint[0] += right[0] * 10;
		boltPoint[1] += right[1] * 10;
		boltPoint[2] += right[2] * 10;
		break;
	case G2_MODELPART_RHAND:
		boltPoint[0] += up[0] * 8;
		boltPoint[1] += up[1] * 8;
		boltPoint[2] += up[2] * 8;

		boltPoint[0] += right[0] * 10;
		boltPoint[1] += right[1] * 10;
		boltPoint[2] += right[2] * 10;
		break;
	case G2_MODELPART_LLEG:
		boltPoint[0] -= up[0] * 4;
		boltPoint[1] -= up[1] * 4;
		boltPoint[2] -= up[2] * 4;

		boltPoint[0] -= right[0] * 10;
		boltPoint[1] -= right[1] * 10;
		boltPoint[2] -= right[2] * 10;
		break;
	case G2_MODELPART_RLEG:
		boltPoint[0] -= up[0] * 4;
		boltPoint[1] -= up[1] * 4;
		boltPoint[2] -= up[2] * 4;

		boltPoint[0] += right[0] * 10;
		boltPoint[1] += right[1] * 10;
		boltPoint[2] += right[2] * 10;
		break;
	default:
		break;
	}

	return;
}

void G_GetDismemberBolt(gentity_t * self, vec3_t boltPoint, int limbType)
{
	int useBolt = self->genericValue5;
	vec3_t properOrigin, properAngles, addVel;
	//matrix3_t legAxis;
	mdxaBone_t	boltMatrix;
	float fVSpeed = 0;
	char* rotateBone = NULL;

	switch (limbType)
	{
	case G2_MODELPART_HEAD:
		rotateBone = "cranium";
		break;
	case G2_MODELPART_WAIST:
		if (self->localAnimIndex <= 1)
		{ //humanoid
			rotateBone = "thoracic";
		}
		else
		{
			rotateBone = "pelvis";
		}
		break;
	case G2_MODELPART_LARM:
		rotateBone = "lradius";
		break;
	case G2_MODELPART_RARM:
		rotateBone = "rradius";
		break;
	case G2_MODELPART_RHAND:
		rotateBone = "rhand";
		break;
	case G2_MODELPART_LLEG:
		rotateBone = "ltibia";
		break;
	case G2_MODELPART_RLEG:
		rotateBone = "rtibia";
		break;
	default:
		rotateBone = "rtibia";
		break;
	}

	useBolt = trap->G2API_AddBolt(self->ghoul2, 0, rotateBone);

	VectorCopy(self->client->ps.origin, properOrigin);
	VectorCopy(self->client->ps.viewangles, properAngles);

	//try to predict the origin based on velocity so it's more like what the client is seeing
	VectorCopy(self->client->ps.velocity, addVel);
	VectorNormalize(addVel);

	if (self->client->ps.velocity[0] < 0)
	{
		fVSpeed += (-self->client->ps.velocity[0]);
	}
	else
	{
		fVSpeed += self->client->ps.velocity[0];
	}
	if (self->client->ps.velocity[1] < 0)
	{
		fVSpeed += (-self->client->ps.velocity[1]);
	}
	else
	{
		fVSpeed += self->client->ps.velocity[1];
	}
	if (self->client->ps.velocity[2] < 0)
	{
		fVSpeed += (-self->client->ps.velocity[2]);
	}
	else
	{
		fVSpeed += self->client->ps.velocity[2];
	}

	fVSpeed *= 0.08f;

	properOrigin[0] += addVel[0] * fVSpeed;
	properOrigin[1] += addVel[1] * fVSpeed;
	properOrigin[2] += addVel[2] * fVSpeed;

	properAngles[0] = 0;
	properAngles[1] = self->client->ps.viewangles[YAW];
	properAngles[2] = 0;

	trap->G2API_GetBoltMatrix(self->ghoul2, 0, useBolt, &boltMatrix, properAngles, properOrigin, level.time, NULL, self->modelScale);

	boltPoint[0] = boltMatrix.matrix[0][3];
	boltPoint[1] = boltMatrix.matrix[1][3];
	boltPoint[2] = boltMatrix.matrix[2][3];

	trap->G2API_GetBoltMatrix(self->ghoul2, 1, 0, &boltMatrix, properAngles, properOrigin, level.time, NULL, self->modelScale);

	if (self->client && limbType == G2_MODELPART_RHAND)
	{ //Make some saber hit sparks over the severed wrist area
		vec3_t boltAngles;
		gentity_t* te;

		boltAngles[0] = -boltMatrix.matrix[0][1];
		boltAngles[1] = -boltMatrix.matrix[1][1];
		boltAngles[2] = -boltMatrix.matrix[2][1];

		te = G_TempEntity(boltPoint, EV_SABER_BODY_HIT);
		te->s.otherEntityNum = self->s.number;
		te->s.otherEntityNum2 = ENTITYNUM_NONE;
		te->s.weapon = 0;//saberNum
		te->s.legsAnim = 0;//bladeNum

		VectorCopy(boltPoint, te->s.origin);
		VectorCopy(boltAngles, te->s.angles);

		if (!te->s.angles[0] && !te->s.angles[1] && !te->s.angles[2])
		{ //don't let it play with no direction
			te->s.angles[1] = 1;
		}

		te->s.eventParm = 16; //lots of sparks
	}
}

void LimbTouch(gentity_t * self, gentity_t * other, trace_t * trace)
{
}

void LimbThink(gentity_t * ent)
{
	float gravity = 3.0f;
	float mass = 0.09f;
	float bounce = 1.3f;

	switch (ent->s.modelGhoul2)
	{
	case G2_MODELPART_HEAD:
		mass = 0.08f;
		bounce = 1.4f;
		break;
	case G2_MODELPART_WAIST:
		mass = 0.1f;
		bounce = 1.2f;
		break;
	case G2_MODELPART_LARM:
	case G2_MODELPART_RARM:
	case G2_MODELPART_RHAND:
	case G2_MODELPART_LLEG:
	case G2_MODELPART_RLEG:
	default:
		break;
	}

	if (ent->speed < level.time)
	{
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
		return;
	}

	if (ent->genericValue5 <= level.time)
	{ //this will be every frame by standard, but we want to compensate in case sv_fps is not 20.
		G_RunExPhys(ent, gravity, mass, bounce, qtrue, NULL, 0);
		ent->genericValue5 = level.time + 50;
	}

	ent->nextthink = level.time;
}

extern qboolean BG_GetRootSurfNameWithVariant(void* ghoul2, const char* rootSurfName, char* returnSurfName, int returnSize);

void G_Dismember(gentity_t * ent, gentity_t * enemy, vec3_t point, int limbType, float limbRollBase, float limbPitchBase, int deathAnim, qboolean postDeath)
{
	vec3_t	newPoint, dir, vel;
	gentity_t* limb;
	char	limbName[MAX_QPATH];
	char	stubName[MAX_QPATH];
	char	stubCapName[MAX_QPATH];

	if (limbType == G2_MODELPART_HEAD)
	{
		Q_strncpyz(limbName, "head", sizeof(limbName));
		Q_strncpyz(stubCapName, "torso_cap_head", sizeof(stubCapName));
	}
	else if (limbType == G2_MODELPART_WAIST)
	{
		Q_strncpyz(limbName, "torso", sizeof(limbName));
		Q_strncpyz(stubCapName, "hips_cap_torso", sizeof(stubCapName));
	}
	else if (limbType == G2_MODELPART_LARM)
	{
		BG_GetRootSurfNameWithVariant(ent->ghoul2, "l_arm", limbName, sizeof(limbName));
		BG_GetRootSurfNameWithVariant(ent->ghoul2, "torso", stubName, sizeof(stubName));
		Com_sprintf(stubCapName, sizeof(stubCapName), "%s_cap_l_arm", stubName);
	}
	else if (limbType == G2_MODELPART_RARM)
	{
		BG_GetRootSurfNameWithVariant(ent->ghoul2, "r_arm", limbName, sizeof(limbName));
		BG_GetRootSurfNameWithVariant(ent->ghoul2, "torso", stubName, sizeof(stubName));
		Com_sprintf(stubCapName, sizeof(stubCapName), "%s_cap_r_arm", stubName);
	}
	else if (limbType == G2_MODELPART_RHAND)
	{
		BG_GetRootSurfNameWithVariant(ent->ghoul2, "r_hand", limbName, sizeof(limbName));
		BG_GetRootSurfNameWithVariant(ent->ghoul2, "r_arm", stubName, sizeof(stubName));
		Com_sprintf(stubCapName, sizeof(stubCapName), "%s_cap_r_hand", stubName);
	}
	else if (limbType == G2_MODELPART_LLEG)
	{
		BG_GetRootSurfNameWithVariant(ent->ghoul2, "l_leg", limbName, sizeof(limbName));
		BG_GetRootSurfNameWithVariant(ent->ghoul2, "hips", stubName, sizeof(stubName));
		Com_sprintf(stubCapName, sizeof(stubCapName), "%s_cap_l_leg", stubName);
	}
	else if (limbType == G2_MODELPART_RLEG)
	{
		BG_GetRootSurfNameWithVariant(ent->ghoul2, "r_leg", limbName, sizeof(limbName));
		BG_GetRootSurfNameWithVariant(ent->ghoul2, "hips", stubName, sizeof(stubName));
		Com_sprintf(stubCapName, sizeof(stubCapName), "%s_cap_r_leg", stubName);
	}
	else
	{//umm... just default to the right leg, I guess (same as on client)
		BG_GetRootSurfNameWithVariant(ent->ghoul2, "r_leg", limbName, sizeof(limbName));
		BG_GetRootSurfNameWithVariant(ent->ghoul2, "hips", stubName, sizeof(stubName));
		Com_sprintf(stubCapName, sizeof(stubCapName), "%s_cap_r_leg", stubName);
	}

	if (ent->ghoul2&& limbName[0] && trap->G2API_GetSurfaceRenderStatus(ent->ghoul2, 0, limbName))
	{ //is it already off? If so there's no reason to be doing it again, so get out of here.
		return;
	}

	VectorCopy(point, newPoint);
	limb = G_Spawn();
	limb->classname = "playerlimb";

	G_SetOrigin(limb, newPoint);
	VectorCopy(newPoint, limb->s.pos.trBase);
	limb->think = LimbThink;
	limb->touch = LimbTouch;
	limb->speed = level.time + Q_irand(8000, 16000);
	limb->nextthink = level.time + FRAMETIME;

	limb->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	limb->clipmask = MASK_SOLID;
	limb->r.contents = CONTENTS_TRIGGER;
	limb->physicsObject = qtrue;
	VectorSet(limb->r.mins, -6.0f, -6.0f, -3.0f);
	VectorSet(limb->r.maxs, 6.0f, 6.0f, 6.0f);

	limb->s.g2radius = 200;

	limb->s.eType = ET_GENERAL;
	limb->s.weapon = G2_MODEL_PART;
	limb->s.modelGhoul2 = limbType;
	limb->s.modelindex = ent->s.number;
	if (!ent->client)
	{
		limb->s.modelindex = -1;
		limb->s.otherEntityNum2 = ent->s.number;
	}

	VectorClear(limb->s.apos.trDelta);

	if (ent->client)
	{
		VectorCopy(ent->client->ps.viewangles, limb->r.currentAngles);
		VectorCopy(ent->client->ps.viewangles, limb->s.apos.trBase);
	}
	else
	{
		VectorCopy(ent->r.currentAngles, limb->r.currentAngles);
		VectorCopy(ent->r.currentAngles, limb->s.apos.trBase);
	}

	//Set up the ExPhys values for the entity.
	limb->epGravFactor = 0;
	VectorClear(limb->epVelocity);
	VectorSubtract(point, ent->r.currentOrigin, dir);
	VectorNormalize(dir);
	if (ent->client)
	{
		VectorCopy(ent->client->ps.velocity, vel);
	}
	else
	{
		VectorCopy(ent->s.pos.trDelta, vel);
	}
	VectorMA(vel, 80, dir, limb->epVelocity);

	//add some vertical velocity
	if (limbType == G2_MODELPART_HEAD ||
		limbType == G2_MODELPART_WAIST)
	{
		limb->epVelocity[2] += 10;
	}

	if (enemy && enemy->client && ent && ent != enemy && ent->s.number != enemy->s.number &&
		enemy->client->ps.weapon == WP_SABER && enemy->client->olderIsValid &&
		(level.time - enemy->client->lastSaberStorageTime) < 200)
	{ //The enemy has valid saber positions between this and last frame. Use them to factor in direction of the limb.
		vec3_t dif;
		float totalDistance;
		const float distScale = 1.2f;

		//scale down the initial velocity first, which is based on the speed of the limb owner.
		//ExPhys object velocity operates on a slightly different scale than Q3-based physics velocity.
		VectorScale(limb->epVelocity, 0.4f, limb->epVelocity);

		VectorSubtract(enemy->client->lastSaberBase_Always, enemy->client->olderSaberBase, dif);
		totalDistance = VectorNormalize(dif);

		VectorScale(dif, totalDistance * distScale, dif);
		VectorAdd(limb->epVelocity, dif, limb->epVelocity);

		if (ent->client && (ent->client->ps.torsoTimer > 0 || !BG_InDeathAnim(ent->client->ps.torsoAnim)))
		{ //if he's done with his death anim we don't actually want the limbs going far
			vec3_t preVel;

			VectorCopy(limb->epVelocity, preVel);
			preVel[2] = 0;
			totalDistance = VectorNormalize(preVel);

			if (totalDistance < 40.0f)
			{
				float mAmt = 40.0f;//60.0f/totalDistance;

				limb->epVelocity[0] = preVel[0] * mAmt;
				limb->epVelocity[1] = preVel[1] * mAmt;
			}
		}
		else if (ent->client)
		{
			VectorScale(limb->epVelocity, 0.3f, limb->epVelocity);
		}
	}

	if (ent->s.eType == ET_NPC && ent->ghoul2 && limbName[0] && stubCapName[0])
	{ //if it's an npc remove these surfs on the server too. For players we don't even care cause there's no further dismemberment after death.
		trap->G2API_SetSurfaceOnOff(ent->ghoul2, limbName, 0x00000100);
		trap->G2API_SetSurfaceOnOff(ent->ghoul2, stubCapName, 0);
	}

	if (level.gametype >= GT_TEAM && ent->s.eType != ET_NPC)
	{//Team game
		switch (ent->client->sess.sessionTeam)
		{
		case TEAM_RED:
			limb->s.customRGBA[0] = 255;
			limb->s.customRGBA[1] = 0;
			limb->s.customRGBA[2] = 0;
			break;

		case TEAM_BLUE:
			limb->s.customRGBA[0] = 0;
			limb->s.customRGBA[1] = 0;
			limb->s.customRGBA[2] = 255;
			break;

		default:
			limb->s.customRGBA[0] = ent->s.customRGBA[0];
			limb->s.customRGBA[1] = ent->s.customRGBA[1];
			limb->s.customRGBA[2] = ent->s.customRGBA[2];
			limb->s.customRGBA[3] = ent->s.customRGBA[3];
			break;
		}
	}
	else
	{//FFA
		limb->s.customRGBA[0] = ent->s.customRGBA[0];
		limb->s.customRGBA[1] = ent->s.customRGBA[1];
		limb->s.customRGBA[2] = ent->s.customRGBA[2];
		limb->s.customRGBA[3] = ent->s.customRGBA[3];
	}

	trap->LinkEntity((sharedEntity_t*)limb);
}

void DismembermentTest(gentity_t * self)
{
	int sect = G2_MODELPART_HEAD;
	vec3_t boltPoint;

	while (sect <= G2_MODELPART_RLEG)
	{
		G_GetDismemberBolt(self, boltPoint, sect);
		G_Dismember(self, self, boltPoint, sect, 90, 0, BOTH_DEATH1, qfalse);
		sect++;
	}
}

void DismembermentByNum(gentity_t * self, int num)
{
	int sect = G2_MODELPART_HEAD;
	vec3_t boltPoint;

	switch (num)
	{
	case 0:
		sect = G2_MODELPART_HEAD;
		break;
	case 1:
		sect = G2_MODELPART_WAIST;
		break;
	case 2:
		sect = G2_MODELPART_LARM;
		break;
	case 3:
		sect = G2_MODELPART_RARM;
		break;
	case 4:
		sect = G2_MODELPART_RHAND;
		break;
	case 5:
		sect = G2_MODELPART_LLEG;
		break;
	case 6:
		sect = G2_MODELPART_RLEG;
		break;
	default:
		break;
	}

	G_GetDismemberBolt(self, boltPoint, sect);
	G_Dismember(self, self, boltPoint, sect, 90, 0, BOTH_DEATH1, qfalse);
}

int G_GetHitQuad(gentity_t * self, vec3_t hitloc)
{
	vec3_t diff, fwdangles = { 0,0,0 }, right;
	vec3_t clEye;
	float rightdot;
	float zdiff;
	int hitLoc = gPainHitLoc;

	if (self->client)
	{
		VectorCopy(self->client->ps.origin, clEye);
		clEye[2] += self->client->ps.viewheight;
	}
	else
	{
		VectorCopy(self->s.pos.trBase, clEye);
		clEye[2] += 16;
	}

	VectorSubtract(hitloc, clEye, diff);
	diff[2] = 0;
	VectorNormalize(diff);

	if (self->client)
	{
		fwdangles[1] = self->client->ps.viewangles[1];
	}
	else
	{
		fwdangles[1] = self->s.apos.trBase[1];
	}
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors(fwdangles, NULL, right, NULL);

	rightdot = DotProduct(right, diff);
	zdiff = hitloc[2] - clEye[2];

	if (zdiff > 0)
	{
		if (rightdot > 0.3)
		{
			hitLoc = G2_MODELPART_RARM;
		}
		else if (rightdot < -0.3)
		{
			hitLoc = G2_MODELPART_LARM;
		}
		else
		{
			hitLoc = G2_MODELPART_HEAD;
		}
	}
	else if (zdiff > -20)
	{
		if (rightdot > 0.1)
		{
			hitLoc = G2_MODELPART_RARM;
		}
		else if (rightdot < -0.1)
		{
			hitLoc = G2_MODELPART_LARM;
		}
		else
		{
			hitLoc = G2_MODELPART_HEAD;
		}
	}
	else
	{
		if (rightdot >= 0)
		{
			hitLoc = G2_MODELPART_RLEG;
		}
		else
		{
			hitLoc = G2_MODELPART_LLEG;
		}
	}

	return hitLoc;
}

int gGAvoidDismember = 0;

void UpdateClientRenderBolts(gentity_t * self, vec3_t renderOrigin, vec3_t renderAngles);

qboolean G_GetHitLocFromSurfName(gentity_t * ent, const char* surfName, int* hitLoc, vec3_t point, vec3_t dir, vec3_t bladeDir, int mod)
{
	qboolean dismember = qfalse;
	int actualTime;
	int kneeLBolt = -1;
	int kneeRBolt = -1;
	int handRBolt = -1;
	int handLBolt = -1;
	int footRBolt = -1;
	int footLBolt = -1;

	*hitLoc = HL_NONE;

	if (!surfName || !surfName[0])
	{
		return qfalse;
	}

	if (!ent->client)
	{
		return qfalse;
	}

	if (!point)
	{
		return qfalse;
	}

	if (ent->client
		&& (ent->client->NPC_class == CLASS_R2D2
			|| ent->client->NPC_class == CLASS_R5D2
			|| ent->client->NPC_class == CLASS_GONK
			|| ent->client->NPC_class == CLASS_MOUSE
			|| ent->client->NPC_class == CLASS_SENTRY
			|| ent->client->NPC_class == CLASS_INTERROGATOR
			|| ent->client->NPC_class == CLASS_PROBE
			|| ent->client->NPC_class == CLASS_SBD
			|| ent->client->NPC_class == CLASS_DROIDEKA))
	{//we don't care about per-surface hit-locations or dismemberment for these guys
		return qfalse;
	}

	if (ent->localAnimIndex <= 1)
	{ //humanoid
		handLBolt = trap->G2API_AddBolt(ent->ghoul2, 0, "*l_hand");
		handRBolt = trap->G2API_AddBolt(ent->ghoul2, 0, "*r_hand");
		kneeLBolt = trap->G2API_AddBolt(ent->ghoul2, 0, "*hips_l_knee");
		kneeRBolt = trap->G2API_AddBolt(ent->ghoul2, 0, "*hips_r_knee");
		footLBolt = trap->G2API_AddBolt(ent->ghoul2, 0, "*l_leg_foot");
		footRBolt = trap->G2API_AddBolt(ent->ghoul2, 0, "*r_leg_foot");
	}

	if (ent->client && (ent->client->NPC_class == CLASS_ATST))
	{
		//FIXME: almost impossible to hit these... perhaps we should
		//		check for splashDamage and do radius damage to these parts?
		//		Or, if we ever get bbox G2 traces, that may fix it, too
		if (!Q_stricmp("head_light_blaster_cann", surfName))
		{
			*hitLoc = HL_ARM_LT;
		}
		else if (!Q_stricmp("head_concussion_charger", surfName))
		{
			*hitLoc = HL_ARM_RT;
		}
		return(qfalse);
	}
	else if (ent->client && (ent->client->NPC_class == CLASS_MARK1))
	{
		if (!Q_stricmp("l_arm", surfName))
		{
			*hitLoc = HL_ARM_LT;
		}
		else if (!Q_stricmp("r_arm", surfName))
		{
			*hitLoc = HL_ARM_RT;
		}
		else if (!Q_stricmp("torso_front", surfName))
		{
			*hitLoc = HL_CHEST;
		}
		else if (!Q_stricmp("torso_tube1", surfName))
		{
			*hitLoc = HL_GENERIC1;
		}
		else if (!Q_stricmp("torso_tube2", surfName))
		{
			*hitLoc = HL_GENERIC2;
		}
		else if (!Q_stricmp("torso_tube3", surfName))
		{
			*hitLoc = HL_GENERIC3;
		}
		else if (!Q_stricmp("torso_tube4", surfName))
		{
			*hitLoc = HL_GENERIC4;
		}
		else if (!Q_stricmp("torso_tube5", surfName))
		{
			*hitLoc = HL_GENERIC5;
		}
		else if (!Q_stricmp("torso_tube6", surfName))
		{
			*hitLoc = HL_GENERIC6;
		}
		return(qfalse);
	}
	else if (ent->client && (ent->client->NPC_class == CLASS_MARK2))
	{
		if (!Q_stricmp("torso_canister1", surfName))
		{
			*hitLoc = HL_GENERIC1;
		}
		else if (!Q_stricmp("torso_canister2", surfName))
		{
			*hitLoc = HL_GENERIC2;
		}
		else if (!Q_stricmp("torso_canister3", surfName))
		{
			*hitLoc = HL_GENERIC3;
		}
		return(qfalse);
	}
	else if (ent->client && (ent->client->NPC_class == CLASS_GALAKMECH))
	{
		if (!Q_stricmp("torso_antenna", surfName) || !Q_stricmp("torso_antenna_base", surfName))
		{
			*hitLoc = HL_GENERIC1;
		}
		else if (!Q_stricmp("torso_shield", surfName))
		{
			*hitLoc = HL_GENERIC2;
		}
		else
		{
			*hitLoc = HL_CHEST;
		}
		return(qfalse);
	}

	//FIXME: check the hitLoc and hitDir against the cap tag for the place
	//where the split will be- if the hit dir is roughly perpendicular to
	//the direction of the cap, then the split is allowed, otherwise we
	//hit it at the wrong angle and should not dismember...
	actualTime = level.time;
	if (!Q_strncmp("hips", surfName, 4))
	{//FIXME: test properly for legs
		*hitLoc = HL_WAIST;
		if (ent->client != NULL && ent->ghoul2)
		{
			mdxaBone_t	boltMatrix;
			vec3_t	tagOrg, angles;

			VectorSet(angles, 0, ent->r.currentAngles[YAW], 0);
			if (kneeLBolt >= 0)
			{
				trap->G2API_GetBoltMatrix(ent->ghoul2, 0, kneeLBolt,
					&boltMatrix, angles, ent->r.currentOrigin,
					actualTime, NULL, ent->modelScale);
				BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, tagOrg);
				if (DistanceSquared(point, tagOrg) < 100)
				{//actually hit the knee
					*hitLoc = HL_LEG_LT;
				}
			}
			if (*hitLoc == HL_WAIST)
			{
				if (kneeRBolt >= 0)
				{
					trap->G2API_GetBoltMatrix(ent->ghoul2, 0, kneeRBolt,
						&boltMatrix, angles, ent->r.currentOrigin,
						actualTime, NULL, ent->modelScale);
					BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, tagOrg);
					if (DistanceSquared(point, tagOrg) < 100)
					{//actually hit the knee
						*hitLoc = HL_LEG_RT;
					}
				}
			}
		}
	}
	else if (!Q_strncmp("torso", surfName, 5))
	{
		if (!ent->client)
		{
			*hitLoc = HL_CHEST;
		}
		else
		{
			vec3_t	t_fwd, t_rt, t_up, dirToImpact;
			float frontSide, rightSide, upSide;
			AngleVectors(ent->client->renderInfo.torsoAngles, t_fwd, t_rt, t_up);

			if (ent->client->renderInfo.boltValidityTime != level.time)
			{
				vec3_t renderAng;

				renderAng[0] = 0;
				renderAng[1] = ent->client->ps.viewangles[YAW];
				renderAng[2] = 0;

				UpdateClientRenderBolts(ent, ent->client->ps.origin, renderAng);
			}

			VectorSubtract(point, ent->client->renderInfo.torsoPoint, dirToImpact);
			frontSide = DotProduct(t_fwd, dirToImpact);
			rightSide = DotProduct(t_rt, dirToImpact);
			upSide = DotProduct(t_up, dirToImpact);
			if (upSide < -10)
			{//hit at waist
				*hitLoc = HL_WAIST;
			}
			else
			{//hit on upper torso
				if (rightSide > 4)
				{
					*hitLoc = HL_ARM_RT;
				}
				else if (rightSide < -4)
				{
					*hitLoc = HL_ARM_LT;
				}
				else if (rightSide > 2)
				{
					if (frontSide > 0)
					{
						*hitLoc = HL_CHEST_RT;
					}
					else
					{
						*hitLoc = HL_BACK_RT;
					}
				}
				else if (rightSide < -2)
				{
					if (frontSide > 0)
					{
						*hitLoc = HL_CHEST_LT;
					}
					else
					{
						*hitLoc = HL_BACK_LT;
					}
				}
				else if (upSide > -3 && mod == MOD_SABER)
				{
					*hitLoc = HL_HEAD;
				}
				else if (frontSide > 0)
				{
					*hitLoc = HL_CHEST;
				}
				else
				{
					*hitLoc = HL_BACK;
				}
			}
		}
	}
	else if (!Q_strncmp("head", surfName, 4))
	{
		*hitLoc = HL_HEAD;
	}
	else if (!Q_strncmp("r_arm", surfName, 5))
	{
		*hitLoc = HL_ARM_RT;
		if (ent->client != NULL && ent->ghoul2)
		{
			mdxaBone_t	boltMatrix;
			vec3_t	tagOrg, angles;

			VectorSet(angles, 0, ent->r.currentAngles[YAW], 0);
			if (handRBolt >= 0)
			{
				trap->G2API_GetBoltMatrix(ent->ghoul2, 0, handRBolt,
					&boltMatrix, angles, ent->r.currentOrigin,
					actualTime, NULL, ent->modelScale);
				BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, tagOrg);
				if (DistanceSquared(point, tagOrg) < 256)
				{//actually hit the hand
					*hitLoc = HL_HAND_RT;
				}
			}
		}
	}
	else if (!Q_strncmp("l_arm", surfName, 5))
	{
		*hitLoc = HL_ARM_LT;
		if (ent->client != NULL && ent->ghoul2)
		{
			mdxaBone_t	boltMatrix;
			vec3_t	tagOrg, angles;

			VectorSet(angles, 0, ent->r.currentAngles[YAW], 0);
			if (handLBolt >= 0)
			{
				trap->G2API_GetBoltMatrix(ent->ghoul2, 0, handLBolt,
					&boltMatrix, angles, ent->r.currentOrigin,
					actualTime, NULL, ent->modelScale);
				BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, tagOrg);
				if (DistanceSquared(point, tagOrg) < 256)
				{//actually hit the hand
					*hitLoc = HL_HAND_LT;
				}
			}
		}
	}
	else if (!Q_strncmp("r_leg", surfName, 5))
	{
		*hitLoc = HL_LEG_RT;
		if (ent->client != NULL && ent->ghoul2)
		{
			mdxaBone_t	boltMatrix;
			vec3_t	tagOrg, angles;

			VectorSet(angles, 0, ent->r.currentAngles[YAW], 0);
			if (footRBolt >= 0)
			{
				trap->G2API_GetBoltMatrix(ent->ghoul2, 0, footRBolt,
					&boltMatrix, angles, ent->r.currentOrigin,
					actualTime, NULL, ent->modelScale);
				BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, tagOrg);
				if (DistanceSquared(point, tagOrg) < 100)
				{//actually hit the foot
					*hitLoc = HL_FOOT_RT;
				}
			}
		}
	}
	else if (!Q_strncmp("l_leg", surfName, 5))
	{
		*hitLoc = HL_LEG_LT;
		if (ent->client != NULL && ent->ghoul2)
		{
			mdxaBone_t	boltMatrix;
			vec3_t	tagOrg, angles;

			VectorSet(angles, 0, ent->r.currentAngles[YAW], 0);
			if (footLBolt >= 0)
			{
				trap->G2API_GetBoltMatrix(ent->ghoul2, 0, footLBolt,
					&boltMatrix, angles, ent->r.currentOrigin,
					actualTime, NULL, ent->modelScale);
				BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, tagOrg);
				if (DistanceSquared(point, tagOrg) < 100)
				{//actually hit the foot
					*hitLoc = HL_FOOT_LT;
				}
			}
		}
	}
	else if (!Q_strncmp("r_hand", surfName, 6) || !Q_strncmp("w_", surfName, 2))
	{//right hand or weapon
		*hitLoc = HL_HAND_RT;
	}
	else if (!Q_strncmp("l_hand", surfName, 6))
	{
		*hitLoc = HL_HAND_LT;
	}
	//added force_shield as a hit area
	else if (ent->flags & FL_SHIELDED && !Q_stricmp("force_shield", surfName))
	{
		*hitLoc = HL_GENERIC2;

	}

	if (g_dismember.integer == 100)
	{ //full probability...
		if (ent->client && ent->client->NPC_class == CLASS_PROTOCOL)
		{
			dismember = qtrue;
		}
		else if (ent->client && ent->client->NPC_class == CLASS_ASSASSIN_DROID)
		{
			dismember = qtrue;
		}
		else if (ent->client && ent->client->NPC_class == CLASS_SABER_DROID)
		{
			dismember = qtrue;
		}
		else if (dir && (dir[0] || dir[1] || dir[2]) &&
			bladeDir && (bladeDir[0] || bladeDir[1] || bladeDir[2]))
		{//we care about direction (presumably for dismemberment)
			if (1) //Fix me?
			{//either we don't care about probabilties or the probability let us continue
				char* tagName = NULL;
				float	aoa = 0.5f;
				//dir must be roughly perpendicular to the hitLoc's cap bolt
				switch (*hitLoc)
				{
				case HL_LEG_RT:
					tagName = "*hips_cap_r_leg";
					break;
				case HL_LEG_LT:
					tagName = "*hips_cap_l_leg";
					break;
				case HL_WAIST:
					tagName = "*hips_cap_torso";
					aoa = 0.25f;
					break;
				case HL_CHEST_RT:
				case HL_ARM_RT:
				case HL_BACK_LT:
					tagName = "*torso_cap_r_arm";
					break;
				case HL_CHEST_LT:
				case HL_ARM_LT:
				case HL_BACK_RT:
					tagName = "*torso_cap_l_arm";
					break;
				case HL_HAND_RT:
					tagName = "*r_arm_cap_r_hand";
					break;
				case HL_HAND_LT:
					tagName = "*l_arm_cap_l_hand";
					break;
				case HL_HEAD:
					tagName = "*torso_cap_head";
					aoa = 0.25f;
					break;
				case HL_CHEST:
				case HL_BACK:
				case HL_FOOT_RT:
				case HL_FOOT_LT:
				default:
					//no dismemberment possible with these, so no checks needed
					break;
				}
				if (tagName)
				{
					int tagBolt = trap->G2API_AddBolt(ent->ghoul2, 0, tagName);
					if (tagBolt != -1)
					{
						mdxaBone_t	boltMatrix;
						vec3_t	tagOrg, tagDir, angles;

						VectorSet(angles, 0, ent->r.currentAngles[YAW], 0);
						trap->G2API_GetBoltMatrix(ent->ghoul2, 0, tagBolt,
							&boltMatrix, angles, ent->r.currentOrigin,
							actualTime, NULL, ent->modelScale);
						BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, tagOrg);
						BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, tagDir);
						if (DistanceSquared(point, tagOrg) < 256)
						{//hit close
							float dot = DotProduct(dir, tagDir);
							if (dot < aoa&& dot > -aoa)
							{//hit roughly perpendicular
								dot = DotProduct(bladeDir, tagDir);
								if (dot < aoa&& dot > -aoa)
								{//blade was roughly perpendicular
									dismember = qtrue;
								}
							}
						}
					}
				}
			}
		}
		else
		{ //hmm, no direction supplied.
			dismember = qtrue;
		}
	}
	return dismember;
}

void G_CheckForDismemberment(gentity_t * ent, gentity_t * enemy, vec3_t point, int damage, int deathAnim, qboolean postDeath)
{
	int hitLoc = -1, hitLocUse = -1;
	vec3_t boltPoint;
	int dismember = g_dismember.integer;

	if (ent->localAnimIndex > 1)
	{
		if (!ent->NPC)
		{
			return;
		}

		if (ent->client->NPC_class != CLASS_PROTOCOL)
		{ //this is the only non-humanoid allowed to do dismemberment.
			return;
		}
	}

	if (!dismember)
	{
		return;
	}

	if (gGAvoidDismember == 1)
	{
		return;
	}

	if (gGAvoidDismember != 2)
	{ //this means do the dismemberment regardless of randomness and damage
		if (Q_irand(0, 100) > dismember)
		{
			return;
		}

		if (damage < 5)
		{
			return;
		}
	}

	if (gGAvoidDismember == 2)
	{
		hitLoc = HL_HAND_RT;
	}
	else
	{
		if (d_saberGhoul2Collision.integer && ent->client && ent->client->g2LastSurfaceTime == level.time)
		{
			char hitSurface[MAX_QPATH];

			trap->G2API_GetSurfaceName(ent->ghoul2, ent->client->g2LastSurfaceHit, 0, hitSurface);

			if (hitSurface[0])
			{
				G_GetHitLocFromSurfName(ent, hitSurface, &hitLoc, point, vec3_origin, vec3_origin, MOD_UNKNOWN);
			}
		}

		if (hitLoc == -1)
		{
			hitLoc = G_GetHitLocation(ent, point);
		}
	}

	switch (hitLoc)
	{
	case HL_FOOT_RT:
	case HL_LEG_RT:
		hitLocUse = G2_MODELPART_RLEG;
		break;
	case HL_FOOT_LT:
	case HL_LEG_LT:
		hitLocUse = G2_MODELPART_LLEG;
		break;
	case HL_WAIST:
		hitLocUse = G2_MODELPART_WAIST;
		break;
	case HL_ARM_RT:
		hitLocUse = G2_MODELPART_RARM;
		break;
	case HL_HAND_RT:
		hitLocUse = G2_MODELPART_RHAND;
		break;
	case HL_ARM_LT:
	case HL_HAND_LT:
		hitLocUse = G2_MODELPART_LARM;
		break;
	case HL_HEAD:
		hitLocUse = G2_MODELPART_HEAD;
		break;
	default:
		hitLocUse = G_GetHitQuad(ent, point);
		break;
	}

	if (hitLocUse == -1)
	{
		return;
	}

	if (ent->client)
	{
		G_GetDismemberBolt(ent, boltPoint, hitLocUse);
		if (g_austrian.integer
			&& (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL))
		{
			G_LogPrintf("Duel Dismemberment: %s dismembered at %s\n", ent->client->pers.netname, hitLocName[hitLoc]);
		}
	}
	else
	{
		G_GetDismemberLoc(ent, boltPoint, hitLocUse);
	}
	G_Dismember(ent, enemy, boltPoint, hitLocUse, 90, 0, deathAnim, postDeath);
}

void G_CheckForblowingup(gentity_t * ent, gentity_t * enemy, vec3_t point, int damage, int deathAnim, qboolean postDeath)
{
	vec3_t boltPoint;
	int dismember = g_dismember.integer;

	if (ent->localAnimIndex > 1)
	{
		if (!ent->NPC)
		{
			return;
		}

		if (ent->client->NPC_class != CLASS_PROTOCOL)
		{ //this is the only non-humanoid allowed to do dismemberment.
			return;
		}
	}

	if (!dismember)
	{
		return;
	}

	if (gGAvoidDismember == 1)
	{
		return;
	}

	if (gGAvoidDismember != 2)
	{ //this means do the dismemberment regardless of randomness and damage
		if (Q_irand(0, 100) > dismember)
		{
			return;
		}

		if (damage < 5)
		{
			return;
		}
	}
	G_GetDismemberBolt(ent, boltPoint, G2_MODELPART_HEAD);
	G_Dismember(ent, enemy, boltPoint, G2_MODELPART_HEAD, 90, 0, deathAnim, postDeath);
	G_GetDismemberBolt(ent, boltPoint, G2_MODELPART_LARM);
	G_Dismember(ent, enemy, boltPoint, G2_MODELPART_LARM, 90, 0, deathAnim, postDeath);
	G_GetDismemberBolt(ent, boltPoint, G2_MODELPART_RARM);
	G_Dismember(ent, enemy, boltPoint, G2_MODELPART_RARM, 90, 0, deathAnim, postDeath);
	G_GetDismemberBolt(ent, boltPoint, G2_MODELPART_LLEG);
	G_Dismember(ent, enemy, boltPoint, G2_MODELPART_LLEG, 90, 0, deathAnim, postDeath);
	G_GetDismemberBolt(ent, boltPoint, G2_MODELPART_RLEG);
	G_Dismember(ent, enemy, boltPoint, G2_MODELPART_RLEG, 90, 0, deathAnim, postDeath);
	G_GetDismemberBolt(ent, boltPoint, G2_MODELPART_WAIST);
	G_Dismember(ent, enemy, boltPoint, G2_MODELPART_WAIST, 90, 0, deathAnim, postDeath);

}

void G_LocationBasedDamageModifier(gentity_t * ent, vec3_t point, int mod, int dflags, int* damage)
{
	int hitLoc = -1;

	if (!g_locationBasedDamage.integer)
	{ //then leave it alone
		return;
	}

	if ((dflags & DAMAGE_NO_HIT_LOC))
	{ //then leave it alone
		return;
	}

	if (mod == MOD_SABER && *damage <= 1)
	{ //don't bother for idle damage
		return;
	}

	if (!point)
	{
		return;
	}

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{//no location-based damage on vehicles
		return;
	}

	if ((d_saberGhoul2Collision.integer && ent->client && ent->client->g2LastSurfaceTime == level.time && mod == MOD_SABER) || //using ghoul2 collision? Then if the mod is a saber we should have surface data from the last hit (unless thrown).
		(d_projectileGhoul2Collision.integer && ent->client && ent->client->g2LastSurfaceTime == level.time)) //It's safe to assume we died from the projectile that just set our surface index. So, go ahead and use that as the surf I guess.
	{
		char hitSurface[MAX_QPATH];

		trap->G2API_GetSurfaceName(ent->ghoul2, ent->client->g2LastSurfaceHit, 0, hitSurface);

		if (hitSurface[0])
		{
			G_GetHitLocFromSurfName(ent, hitSurface, &hitLoc, point, vec3_origin, vec3_origin, MOD_UNKNOWN);
		}
	}

	if (hitLoc == -1)
	{
		hitLoc = G_GetHitLocation(ent, point);
	}

	switch (hitLoc)
	{
	case HL_FOOT_RT:
	case HL_FOOT_LT:
		*damage *= 0.5;
		break;
	case HL_LEG_RT:
	case HL_LEG_LT:
		*damage *= 0.7;
		break;
	case HL_WAIST:
	case HL_BACK_RT:
	case HL_BACK_LT:
	case HL_BACK:
	case HL_CHEST_RT:
	case HL_CHEST_LT:
	case HL_CHEST:
		break; //normal damage
	case HL_ARM_RT:
	case HL_ARM_LT:
		*damage *= 0.85;
		break;
	case HL_HAND_RT:
	case HL_HAND_LT:
		*damage *= 0.6;
		break;
	case HL_HEAD:
		*damage *= 1.3;
		break;
	default:
		break; //do nothing then
	}
}
/*
===================================
rww - end dismemberment/lbd
===================================
*/

qboolean G_ThereIsAMaster(void)
{
	int i = 0;
	gentity_t* ent;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent&& ent->client&& ent->client->ps.isJediMaster)
		{
			return qtrue;
		}

		i++;
	}

	return qfalse;
}

extern qboolean Rosh_BeingHealed(gentity_t * self);
void G_Knockdown(gentity_t * self, gentity_t * attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock)
{
	if (!self || !self->client)
	{
		return;
	}

	if (self->client->NPC_class == CLASS_ROCKETTROOPER)
	{
		return;
	}

	if (Boba_StopKnockdown(self, attacker, pushDir, qfalse))
	{
		return;
	}
	else if (Jedi_StopKnockdown(self, attacker, pushDir))
	{//They can sometimes backflip instead of be knocked down
		return;
	}
	else if (PM_LockedAnim(self->client->ps.legsAnim))
	{//stuck doing something else
		return;
	}
	else if (PM_LightningBlockAnim(self->client->ps.torsoAnim))
	{//Protect against lightning
		return;
	}
	else if (Rosh_BeingHealed(self))
	{
		return;
	}
	strength *= 3;

	//break out of a saberLock?
	if (self->client->ps.saberLockTime > level.time)
	{
		if (breakSaberLock)
		{
			self->client->ps.saberLockTime = 0;
			self->client->ps.saberLockEnemy = ENTITYNUM_NONE;
		}
		else
		{
			return;
		}
	}

	if (self->health > 0)
	{
		//racc - make a pain noise whenever you're knocked down.
		if (self->s.number < MAX_CLIENTS)
		{
			NPC_SetPainEvent(self);
		}
		else
		{//npc
			NPC_Pain(self, attacker, 0);
		}

		//racc - check to see if an NPC should get conventently kicked off a nearby cliff.
		G_CheckLedgeDive(self, 72, pushDir, qfalse, qfalse);

		if (!PM_RollingAnim(self->client->ps.legsAnim)
			&& !PM_InKnockDown(&self->client->ps))
		{
			int knockAnim = BOTH_KNOCKDOWN1;//default knockdown

			if (PM_CrouchAnim(self->client->ps.legsAnim))
			{//crouched knockdown
				knockAnim = BOTH_KNOCKDOWN4;
			}
			else
			{//plain old knockdown
				vec3_t pLFwd, pLAngles;
				VectorSet(pLAngles, 0, self->client->ps.viewangles[YAW], 0);
				AngleVectors(pLAngles, pLFwd, NULL, NULL);
				if (DotProduct(pLFwd, pushDir) > 0.2f)
				{//pushing him from behind
					knockAnim = BOTH_KNOCKDOWN3;
				}
				else
				{//pushing him from front
					knockAnim = BOTH_KNOCKDOWN1;
				}
			}
			if (knockAnim == BOTH_KNOCKDOWN1 && strength > 100)
			{//push *hard*
				knockAnim = BOTH_KNOCKDOWN2;
			}
			NPC_SetAnim(self, SETANIM_BOTH, knockAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

			if (self->s.number >= MAX_CLIENTS)
			{//randomize getup times
				int addTime = Q_irand(-200, 200);
				self->client->ps.legsTimer += addTime;
				self->client->ps.torsoTimer += addTime;
			}
			else
			{//player holds extra long so you have more time to decide to do the quick getup
				if (BG_KnockDownAnim(self->client->ps.legsAnim))
				{
					if (!(self->r.svFlags & SVF_BOT))
					{
						self->client->ps.legsTimer += PLAYER_KNOCKDOWN_HOLD_EXTRA_TIME;
						self->client->ps.torsoTimer += PLAYER_KNOCKDOWN_HOLD_EXTRA_TIME;
					}
					else if (self->NPC)
					{
						self->client->ps.legsTimer += NPC_KNOCKDOWN_HOLD_EXTRA_TIME;
						self->client->ps.torsoTimer += NPC_KNOCKDOWN_HOLD_EXTRA_TIME;
					}
					else
					{
						self->client->ps.legsTimer += BOT_KNOCKDOWN_HOLD_EXTRA_TIME;
						self->client->ps.torsoTimer += BOT_KNOCKDOWN_HOLD_EXTRA_TIME;
					}
				}
			}

			if (attacker->NPC&& attacker->enemy == self)
			{//pushed pushed down his enemy
				G_AddVoiceEvent(attacker, Q_irand(EV_GLOAT1, EV_GLOAT3), 3000);
				attacker->NPC->blockedSpeechDebounceTime = level.time + 3000;
			}
			BG_ReduceBlasterMishapLevel(&self->client->ps);
		}
	}
}

void G_KnockOver(gentity_t * self, gentity_t * attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock)
{
	if (!self || !self->client)
	{
		return;
	}

	if (self->client->NPC_class == CLASS_ROCKETTROOPER)
	{
		return;
	}

	if (Boba_StopKnockdown(self, attacker, pushDir, qfalse))
	{
		return;
	}
	else if (Jedi_StopKnockdown(self, attacker, pushDir))
	{//They can sometimes backflip instead of be knocked down
		return;
	}
	else if (PM_LockedAnim(self->client->ps.legsAnim))
	{//stuck doing something else
		return;
	}
	else if (PM_LightningBlockAnim(self->client->ps.torsoAnim))
	{//Protect against lightning
		return;
	}
	strength *= 3;

	//break out of a saberLock?
	if (self->client->ps.saberLockTime > level.time)
	{
		if (breakSaberLock)
		{
			self->client->ps.saberLockTime = 0;
			self->client->ps.saberLockEnemy = ENTITYNUM_NONE;
		}
		else
		{
			return;
		}
	}

	if (self->health > 0)
	{
		if (self->s.number < MAX_CLIENTS)
		{
			NPC_SetPainEvent(self);
		}
		else
		{//npc
			NPC_Pain(self, attacker, 0);
		}
		G_CheckLedgeDive(self, 72, pushDir, qfalse, qfalse);

		if (!PM_RollingAnim(self->client->ps.legsAnim)
			&& !PM_InKnockDown(&self->client->ps))
		{
			int knockAnim = BOTH_KNOCKDOWN1;//default knockdown

			if (PM_CrouchAnim(self->client->ps.legsAnim))
			{//crouched knockdown
				knockAnim = BOTH_KNOCKDOWN4;
			}
			else
			{//plain old knockdown
				vec3_t pLFwd, pLAngles;
				VectorSet(pLAngles, 0, self->client->ps.viewangles[YAW], 0);
				AngleVectors(pLAngles, pLFwd, NULL, NULL);
				if (DotProduct(pLFwd, pushDir) > 0.2f)
				{//pushing him from behind
					knockAnim = BOTH_KNOCKDOWN3;
				}
				else
				{//pushing him from front
					knockAnim = BOTH_KNOCKDOWN1;
				}
			}
			if (knockAnim == BOTH_KNOCKDOWN1 && strength > 100)
			{//push *hard*
				knockAnim = BOTH_KNOCKDOWN2;
			}
			NPC_SetAnim(self, SETANIM_BOTH, knockAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

			if (self->s.number >= MAX_CLIENTS)
			{//randomize getup times
				int addTime = Q_irand(-200, 200);
				self->client->ps.legsTimer += addTime;
				self->client->ps.torsoTimer += addTime;
			}
			else
			{//player holds extra long so you have more time to decide to do the quick getup
				if (BG_KnockDownAnim(self->client->ps.legsAnim))
				{
					/*if (!(self->r.svFlags & SVF_BOT))
					{
						self->client->ps.legsTimer += PLAYER_KNOCKOVER_HOLD_EXTRA_TIME;
						self->client->ps.torsoTimer += PLAYER_KNOCKOVER_HOLD_EXTRA_TIME;
					}
					else if (self->NPC)
					{
						self->client->ps.legsTimer += NPC_KNOCKOVER_HOLD_EXTRA_TIME;
						self->client->ps.torsoTimer += NPC_KNOCKOVER_HOLD_EXTRA_TIME;
					}
					else
					{
						self->client->ps.legsTimer += BOT_KNOCKOVER_HOLD_EXTRA_TIME;
						self->client->ps.torsoTimer += BOT_KNOCKOVER_HOLD_EXTRA_TIME;
					}*/
					self->client->ps.legsTimer += BOT_KNOCKOVER_HOLD_EXTRA_TIME;
					self->client->ps.torsoTimer += BOT_KNOCKOVER_HOLD_EXTRA_TIME;
				}
			}

			if (attacker->NPC&& attacker->enemy == self)
			{//pushed pushed down his enemy
				G_AddVoiceEvent(attacker, Q_irand(EV_GLOAT1, EV_GLOAT3), 3000);
				attacker->NPC->blockedSpeechDebounceTime = level.time + 3000;
			}
			BG_ReduceBlasterMishapLevel(&self->client->ps);
		}
	}
}

/*
============
G_LocationDamage
============
*/
int G_LocationDamage(vec3_t point, gentity_t * targ, gentity_t * attacker, int take)
{
	vec3_t bulletPath;
	vec3_t bulletAngle;

	int clientHeight;
	int clientFeetZ;
	int clientRotation;
	int bulletHeight;
	int bulletRotation;	// Degrees rotation around client.				
	int impactRotation;// used to check Back of head vs. Face


					   // First things first.  If we're not damaging them, why are we here? 
	if (!take)
	{
		return 0;
	}

	// Point[2] is the REAL world Z. We want Z relative to the clients feet

	// Where the feet are at [real Z]
	clientFeetZ = targ->r.currentOrigin[2] + targ->r.mins[2];
	// How tall the client is [Relative Z]
	clientHeight = targ->r.maxs[2] - targ->r.mins[2];
	// Where the bullet struck [Relative Z]
	bulletHeight = point[2] - clientFeetZ;
	// Get a vector aiming from the client to the bullet hit 
	VectorSubtract(targ->r.currentOrigin, point, bulletPath);
	// Convert it into PITCH, ROLL, YAW
	vectoangles(bulletPath, bulletAngle);
	clientRotation = targ->client->ps.viewangles[YAW];
	bulletRotation = bulletAngle[YAW];
	impactRotation = abs(clientRotation - bulletRotation);
	impactRotation += 45; // just to make it easier to work with
	impactRotation = impactRotation % 360; // Keep it in the 0-359 range

	if (impactRotation < 90)
	{
		targ->client->lasthurt_location = LOCATION_BACK;
	}
	else if (impactRotation < 180)
	{
		targ->client->lasthurt_location = LOCATION_RIGHT;
	}
	else if (impactRotation < 270)
	{
		targ->client->lasthurt_location = LOCATION_FRONT;
	}
	else if (impactRotation < 360)
	{
		targ->client->lasthurt_location = LOCATION_LEFT;
	}
	else
	{
		targ->client->lasthurt_location = LOCATION_NONE;
	}
	// The upper body never changes height, just distance from the feet
	if (bulletHeight > clientHeight - 2)
	{
		targ->client->lasthurt_location |= LOCATION_HEAD;
	}
	else if (bulletHeight > clientHeight - 8)
	{
		targ->client->lasthurt_location |= LOCATION_FACE;
	}
	else if (bulletHeight > clientHeight - 10)
	{
		targ->client->lasthurt_location |= LOCATION_SHOULDER;
	}
	else if (bulletHeight > clientHeight - 16)
	{
		targ->client->lasthurt_location |= LOCATION_CHEST;
	}
	else if (bulletHeight > clientHeight - 26)
	{
		targ->client->lasthurt_location |= LOCATION_STOMACH;
	}
	else if (bulletHeight > clientHeight - 29)
	{
		targ->client->lasthurt_location |= LOCATION_GROIN;
	}
	else if (bulletHeight < 4)
	{
		targ->client->lasthurt_location |= LOCATION_FOOT;
	}
	else
	{
		// The leg is the only thing that changes size when you duck,
		// so we check for every other parts RELATIVE location, and
		// whats left over must be the leg. 
		targ->client->lasthurt_location |= LOCATION_LEG;
	}

	// Check the location ignoring the rotation info
	switch (targ->client->lasthurt_location & ~(LOCATION_BACK | LOCATION_LEFT | LOCATION_RIGHT | LOCATION_FRONT))
	{
	case LOCATION_HEAD:
		take *= 2.8;
		break;
	case LOCATION_FACE:
		if (targ->client->lasthurt_location& LOCATION_FRONT)
			take *= 5.0; // Faceshots REALLY suck
		else
			take *= 2.8;
		break;
	case LOCATION_SHOULDER:
		if (targ->client->lasthurt_location& (LOCATION_FRONT | LOCATION_BACK))
			take *= 2.4; // Throat or nape of neck
		else
			take *= 2.1; // Shoulders
		break;
	case LOCATION_CHEST:
		if (targ->client->lasthurt_location& (LOCATION_FRONT | LOCATION_BACK))
			take *= 3.3; // Belly or back
		else
			take *= 2.8; // Arms
		break;
	case LOCATION_STOMACH:
		take *= 3;
		break;
	case LOCATION_GROIN:
		if (targ->client->lasthurt_location& LOCATION_FRONT)
			take *= 3; // Groin shot
		break;
	case LOCATION_LEG:
		take *= 2;
		break;
	case LOCATION_FOOT:
		take *= 1.5;
		break;
	}
	return take;
}

void G_ApplyVehicleOtherKiller(gentity_t * targ, gentity_t * inflictor, gentity_t * attacker, int mod, qboolean vehicleDying)
{
	if (targ && targ->client && attacker)
	{
		if (targ->client->ps.otherKillerDebounceTime > level.time)
		{//wait a minute, I already have a last damager
			if (targ->health < 0
				|| (targ->m_pVehicle && targ->m_pVehicle->m_iRemovedSurfaces))
			{//already dying?  don't let credit transfer to anyone else
				return;
			}
			//otherwise, still alive, so, fine, use this damager...
		}

		targ->client->ps.otherKiller = attacker->s.number;
		targ->client->ps.otherKillerTime = level.time + 25000;
		targ->client->ps.otherKillerDebounceTime = level.time + 25000;
		targ->client->otherKillerMOD = mod;
		if (inflictor && !Q_stricmp("vehicle_proj", inflictor->classname))
		{
			targ->client->otherKillerVehWeapon = inflictor->s.otherEntityNum2 + 1;
			targ->client->otherKillerWeaponType = inflictor->s.weapon;
		}
		else
		{
			targ->client->otherKillerVehWeapon = 0;
			targ->client->otherKillerWeaponType = WP_NONE;
		}
		if (vehicleDying)
		{
			//propogate otherkiller down to pilot and passengers so that proper credit is given if they suicide or eject...
			if (targ->m_pVehicle)
			{
				int passNum;
				if (targ->m_pVehicle->m_pPilot)
				{
					gentity_t* pilot = &g_entities[targ->m_pVehicle->m_pPilot->s.number];
					if (pilot->client)
					{
						pilot->client->ps.otherKiller = targ->client->ps.otherKiller;
						pilot->client->ps.otherKillerTime = targ->client->ps.otherKillerTime;
						pilot->client->ps.otherKillerDebounceTime = targ->client->ps.otherKillerDebounceTime;
						pilot->client->otherKillerMOD = targ->client->otherKillerMOD;
						pilot->client->otherKillerVehWeapon = targ->client->otherKillerVehWeapon;
						pilot->client->otherKillerWeaponType = targ->client->otherKillerWeaponType;
					}
				}
				for (passNum = 0; passNum < targ->m_pVehicle->m_iNumPassengers; passNum++)
				{
					gentity_t* pass = &g_entities[targ->m_pVehicle->m_ppPassengers[passNum]->s.number];
					if (pass->client)
					{
						pass->client->ps.otherKiller = targ->client->ps.otherKiller;
						pass->client->ps.otherKillerTime = targ->client->ps.otherKillerTime;
						pass->client->ps.otherKillerDebounceTime = targ->client->ps.otherKillerDebounceTime;
						pass->client->otherKillerMOD = targ->client->otherKillerMOD;
						pass->client->otherKillerVehWeapon = targ->client->otherKillerVehWeapon;
						pass->client->otherKillerWeaponType = targ->client->otherKillerWeaponType;
					}
				}
			}
		}
	}
}

/*
============
G_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

inflictor, attacker, dir, and point can be NULL for environmental effects

dflags		these flags are used to control how G_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
	DAMAGE_HALF_ABSORB		half shields, half health
	DAMAGE_HALF_ARMOR_REDUCTION		Any damage that shields incur is halved
============
*/
extern qboolean gSiegeRoundBegun;

int gPainMOD = 0;
int gPainHitLoc = -1;
vec3_t gPainPoint;

extern void AnimateStun(gentity_t * self, gentity_t * inflictor, vec3_t impact);
void G_Damage(gentity_t * targ, gentity_t * inflictor, gentity_t * attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod)
{
	gclient_t* client;
	int			take;
	int			asave;
	int			max;
	int			subamt = 0;
	int			knockback;
	float		famt = 0;
	float		hamt = 0;
	float		shieldAbsorbed = 0;
	float	    z_ratio;
	float	    z_rel;
	int	        height;
	float	    targ_maxs2;
	vec3_t		newDir;
	gentity_t* tent;

	if (!targ)
		return;

	if (targ && targ->damageRedirect)
	{
		G_Damage(&g_entities[targ->damageRedirectTo], inflictor, attacker, dir, point, damage, dflags, mod);
		return;
	}

	if (mod == MOD_DEMP2 && targ && targ->inuse && targ->client)
	{
		if (targ->client->ps.electrifyTime < level.time)
		{//electrocution effect
			if (targ->s.eType == ET_NPC && targ->s.NPC_class == CLASS_VEHICLE &&
				targ->m_pVehicle && (targ->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER || targ->m_pVehicle->m_pVehicleInfo->type == VH_WALKER))
			{ //do some extra stuff to speeders/walkers
				targ->client->ps.electrifyTime = level.time + Q_irand(3000, 4000);
			}
			else if (targ->s.NPC_class != CLASS_VEHICLE
				|| (targ->m_pVehicle && targ->m_pVehicle->m_pVehicleInfo->type != VH_FIGHTER))
			{//don't do this to fighters
				targ->client->ps.electrifyTime = level.time + Q_irand(300, 800);
			}
		}
	}

	if (level.gametype == GT_SIEGE &&
		!gSiegeRoundBegun)
	{ //nothing can be damaged til the round starts.
		return;
	}

	if (!targ->takedamage)
	{
		return;
	}


	if ((targ->flags & FL_SHIELDED) && mod != MOD_SABER && !targ->client)
	{//magnetically protected, this thing can only be damaged by lightsabers
		return;
	}

	if ((mod == MOD_BRYAR_PISTOL_ALT || mod == MOD_SEEKER || mod == MOD_DEMP2) && targ && targ->inuse && targ->client)
	{//it acts like a stun hit

		int mpDamage = (float)inflictor->s.generic1 / BRYAR_MAX_CHARGE * 7;

		WP_ForcePowerDrain(&targ->client->ps, FP_ABSORB, 20);

		targ->client->ps.saberAttackChainCount += mpDamage;

		if (targ->client->ps.saberAttackChainCount >= MISHAPLEVEL_HEAVY || targ->client->ps.fd.forcePower < 50)
		{//knockdown
			if (targ->client->ps.saberAttackChainCount >= MISHAPLEVEL_SEVEN)
			{
				G_Knockdown(targ, attacker, dir, 50, qtrue);
			}
			else if (targ->client->ps.fd.forcePower < 25)
			{
				G_Knockdown(targ, attacker, dir, 50, qtrue);
			}
		}
		else if (targ->client->ps.saberAttackChainCount >= MISHAPLEVEL_LIGHT)
		{//stumble
			AnimateStun(targ, attacker, point);
		}
		else
		{
			G_SetAnim(targ, NULL, SETANIM_TORSO, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 100);
			targ->client->ps.torsoTimer += 100;
			targ->client->ps.weaponTime = targ->client->ps.torsoTimer;
		}

		if (targ->client->ps.electrifyTime < level.time + 100)
		{// ... do the effect for a split second for some more feedback
			targ->client->ps.electrifyTime = level.time + 3000;
		}

		if (targ->client->ps.electrifyTime > level.time + 100 && !(targ->s.eType == ET_NPC || targ->r.svFlags & SVF_BOT))
		{
			targ->client->stunDamage = 9;
			targ->client->stunTime = level.time + 1000;

			tent = G_TempEntity(targ->r.currentOrigin, EV_STUNNED);
			tent->owner = targ;
		}
		return;
	}

	if ((targ->flags & FL_DMG_BY_SABER_ONLY) && mod != MOD_SABER)
	{ //saber-only damage
		return;
	}

	if (targ->client)
	{//don't take damage when in a walker, or fighter
		//unless the walker/fighter is dead!!! -rww
		if (targ->client->ps.clientNum < MAX_CLIENTS && targ->client->ps.m_iVehicleNum)
		{
			gentity_t* veh = &g_entities[targ->client->ps.m_iVehicleNum];
			if (veh->m_pVehicle && veh->health > 0)
			{
				if (veh->m_pVehicle->m_pVehicleInfo->type == VH_WALKER ||
					veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
				{
					if (!(dflags & DAMAGE_NO_PROTECTION))
					{
						return;
					}
				}
			}
		}
	}

	if ((targ->flags & FL_DMG_BY_HEAVY_WEAP_ONLY))
	{ //only take damage from explosives and such
		if (mod != MOD_REPEATER_ALT &&
			mod != MOD_ROCKET &&
			mod != MOD_FLECHETTE_ALT_SPLASH &&
			mod != MOD_ROCKET_HOMING &&
			mod != MOD_THERMAL &&
			mod != MOD_THERMAL_SPLASH &&
			mod != MOD_TRIP_MINE_SPLASH &&
			mod != MOD_TIMED_MINE_SPLASH &&
			mod != MOD_DET_PACK_SPLASH &&
			mod != MOD_VEHICLE &&
			mod != MOD_CONC &&
			mod != MOD_CONC_ALT &&
			mod != MOD_SABER &&
			mod != MOD_TURBLAST &&
			mod != MOD_TARGET_LASER &&
			mod != MOD_SUICIDE &&
			mod != MOD_FALLING &&
			mod != MOD_CRUSH &&
			mod != MOD_TELEFRAG &&
			mod != MOD_COLLISION &&
			mod != MOD_VEH_EXPLOSION &&
			mod != MOD_TRIGGER_HURT)
		{
			if (mod != MOD_MELEE || !G_HeavyMelee(attacker))
			{ //let classes with heavy melee ability damage heavy wpn dmg doors with fists
				return;
			}
		}
	}

	if (targ->flags & FL_BBRUSH)
	{
		if (mod == MOD_DEMP2 ||
			mod == MOD_DEMP2_ALT ||
			mod == MOD_BRYAR_PISTOL ||
			mod == MOD_BRYAR_PISTOL_ALT ||
			mod == MOD_MELEE)
		{ //these don't damage bbrushes.. ever
			if (mod != MOD_MELEE || !G_HeavyMelee(attacker))
			{ //let classes with heavy melee ability damage breakable brushes with fists
				return;
			}
		}
	}

	if (targ && targ->client && targ->client->ps.duelInProgress)
	{
		if (attacker && attacker->client && attacker->s.number != targ->client->ps.duelIndex)
		{
			return;
		}
		else if (attacker && attacker->client && mod != MOD_SABER)
		{
			return;
		}
	}
	if (attacker && attacker->client && attacker->client->ps.duelInProgress)
	{
		if (targ && targ->client && targ->s.number != attacker->client->ps.duelIndex)
		{
			return;
		}
		else if (targ && targ->client && mod != MOD_SABER)
		{
			return;
		}
	}

	if (!(dflags & DAMAGE_NO_PROTECTION))
	{//rage overridden by no_protection
		if (targ && targ->client && (targ->client->ps.fd.forcePowersActive & (1 << FP_RAGE)))
		{
			damage *= 0.5;
		}
	}

	// the intermission has allready been qualified for, so don't
	// allow any extra scoring
	if (level.intermissionQueued) {
		return;
	}
	if (!inflictor) {
		inflictor = &g_entities[ENTITYNUM_WORLD];
	}
	if (!attacker) {
		attacker = &g_entities[ENTITYNUM_WORLD];
	}

	// shootable doors / buttons don't actually have any health

	//if genericValue4 == 1 then it's glass or a breakable and those do have health
	if (targ->s.eType == ET_MOVER && targ->genericValue4 != 1) {
		if (targ->use&& targ->moverState == MOVER_POS1) {
			GlobalUse(targ, inflictor, attacker);
		}
		return;
	}
	// reduce damage by the attacker's handicap value
	// unless they are rocket jumping
	if (attacker->client
		&& attacker != targ
		&& attacker->s.eType == ET_PLAYER
		&& level.gametype != GT_SIEGE)
	{
		max = attacker->client->ps.stats[STAT_MAX_HEALTH];
		damage = damage * max / 100;
	}

	if (!(dflags & DAMAGE_NO_HIT_LOC))
	{//see if we should modify it by damage location
		if (targ->inuse && (targ->client || targ->s.eType == ET_NPC) &&
			attacker->inuse && (attacker->client || attacker->s.eType == ET_NPC))
		{ //check for location based damage stuff.
			G_LocationBasedDamageModifier(targ, point, mod, dflags, &damage);
		}
	}

	if (targ->client
		&& targ->client->NPC_class == CLASS_RANCOR
		&& (!attacker || !attacker->client || attacker->client->NPC_class != CLASS_RANCOR))
	{
		// I guess always do 10 points of damage...feel free to tweak as needed
		if (damage < 10)
		{//ignore piddly little damage
			damage = 0;
		}
		else if (damage >= 10)
		{
			damage = 10;
		}
	}

	client = targ->client;

	if (client) {
		if (client->noclip) {
			return;
		}
	}

	if (!dir)
	{
		dflags |= DAMAGE_NO_KNOCKBACK;
	}
	else
	{
		VectorNormalize(dir);
	}

	knockback = damage;

	//Attempt to apply extra knockback
	if (dflags & DAMAGE_EXTRA_KNOCKBACK)
	{
		knockback *= 2;
	}

	if (knockback > 200)
	{
		knockback = 200;
	}

	if (targ->client
		&& (targ->client->ps.fd.forcePowersActive & (1 << FP_PROTECT))
		&& targ->client->ps.fd.forcePowerLevel[FP_PROTECT] == FORCE_LEVEL_3)
	{//pretend there was no damage?
		knockback = 0;
	}
	else if (mod == MOD_CRUSH)
	{
		knockback = 0;
	}
	else if (targ->flags & FL_NO_KNOCKBACK)
	{
		knockback = 0;
	}
	else if (targ->NPC && targ->NPC->jumpState == JS_JUMPING)
	{
		knockback = 0;
	}
	else if (attacker->s.number >= MAX_CLIENTS//an NPC fired
		&& targ->client //hit a client
		&& attacker->client //attacker is a client
		&& targ->client->playerTeam == attacker->client->playerTeam)//on same team
	{//crap, ignore knockback
		knockback = 0;
	}
	else if (dflags & DAMAGE_NO_KNOCKBACK)
	{
		knockback = 0;
	}
	else if (mod == MOD_ROCKET || mod == MOD_ROCKET_SPLASH)
	{
		knockback *= 3.5;
	}

	if (knockback && targ->client)
	{
		vec3_t	kvel;
		float	mass;

		mass = 200;

		if (mod == MOD_SABER)
		{
			float saberKnockbackScale = g_saberDmgVelocityScale.value;
			if ((dflags & DAMAGE_SABER_KNOCKBACK1)
				|| (dflags & DAMAGE_SABER_KNOCKBACK2))
			{//saber does knockback, scale it by the right number
				if (!saberKnockbackScale)
				{
					saberKnockbackScale = 1.0f;
				}
				if (attacker
					&& attacker->client)
				{
					if ((dflags & DAMAGE_SABER_KNOCKBACK1))
					{
						if (attacker && attacker->client)
						{
							saberKnockbackScale *= attacker->client->saber[0].knockbackScale;
						}
					}
					if ((dflags & DAMAGE_SABER_KNOCKBACK1_B2))
					{
						if (attacker && attacker->client)
						{
							saberKnockbackScale *= attacker->client->saber[0].knockbackScale2;
						}
					}
					if ((dflags & DAMAGE_SABER_KNOCKBACK2))
					{
						if (attacker&& attacker->client)
						{
							saberKnockbackScale *= attacker->client->saber[1].knockbackScale;
						}
					}
					if ((dflags& DAMAGE_SABER_KNOCKBACK2_B2))
					{
						if (attacker&& attacker->client)
						{
							saberKnockbackScale *= attacker->client->saber[1].knockbackScale2;
						}
					}
				}
			}
			VectorScale(dir, (g_knockback.value * (float)knockback / mass) * saberKnockbackScale, kvel);
		}
		else
		{
			VectorScale(dir, g_knockback.value * (float)knockback / mass, kvel);
			G_CheckKnockdown(targ, attacker, newDir, dflags, mod);
		}
		VectorAdd(targ->client->ps.velocity, kvel, targ->client->ps.velocity);

		if (attacker && attacker->client && attacker != targ)
		{
			float dur = 5000;
			float dur2 = 100;
			if (targ->client&& targ->s.eType == ET_NPC && targ->s.NPC_class == CLASS_VEHICLE)
			{
				dur = 25000;
				dur2 = 25000;
			}
			targ->client->ps.otherKiller = attacker->s.number;
			targ->client->ps.otherKillerTime = level.time + dur;
			targ->client->ps.otherKillerDebounceTime = level.time + dur2;
		}
		// set the timer so that the other client can't cancel
		// out the movement immediately
		if (!targ->client->ps.pm_time && (g_saberDmgVelocityScale.integer || mod != MOD_SABER || (dflags & DAMAGE_SABER_KNOCKBACK1) || (dflags & DAMAGE_SABER_KNOCKBACK2) || (dflags & DAMAGE_SABER_KNOCKBACK1_B2) || (dflags & DAMAGE_SABER_KNOCKBACK2_B2)))
		{
			int		t;

			t = knockback = 0;
			if (t < 50)
			{
				t = 50;
			}
			if (t > 200)
			{
				t = 50;
			}
			targ->client->ps.pm_time = t;
			targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}
	}
	else if (targ->client && targ->s.eType == ET_NPC && targ->s.NPC_class == CLASS_VEHICLE && attacker != targ)
	{
		targ->client->ps.otherKiller = attacker->s.number;
		targ->client->ps.otherKillerTime = level.time + 25000;
		targ->client->ps.otherKillerDebounceTime = level.time + 25000;
	}


	if ((g_jediVmerc.integer || level.gametype == GT_SIEGE)
		&& client)
	{//less explosive damage for jedi, more saber damage for non-jedi
		if (client->ps.trueJedi
			|| (level.gametype == GT_SIEGE && client->ps.weapon == WP_SABER))
		{//if the target is a trueJedi, reduce splash and explosive damage to 1/2
			switch (mod)
			{
			case MOD_REPEATER_ALT:
			case MOD_REPEATER_ALT_SPLASH:
			case MOD_DEMP2_ALT:
			case MOD_FLECHETTE_ALT_SPLASH:
			case MOD_ROCKET:
			case MOD_ROCKET_SPLASH:
			case MOD_ROCKET_HOMING:
			case MOD_ROCKET_HOMING_SPLASH:
			case MOD_THERMAL:
			case MOD_THERMAL_SPLASH:
			case MOD_TRIP_MINE_SPLASH:
			case MOD_TIMED_MINE_SPLASH:
			case MOD_DET_PACK_SPLASH:
				damage *= 0.75;
				break;
			}
		}
		else if ((client->ps.trueNonJedi || (level.gametype == GT_SIEGE && client->ps.weapon != WP_SABER))
			&& mod == MOD_SABER)
		{//if the target is a trueNonJedi, take more saber damage... combined with the 1.5 in the w_saber stuff, this is 6 times damage!
			if (damage < 100)
			{
				damage *= 4;
				if (damage > 100)
				{
					damage = 100;
				}
			}
		}
	}

	if (attacker->client && targ->client && level.gametype == GT_SIEGE &&
		targ->client->siegeClass != -1 && (bgSiegeClasses[targ->client->siegeClass].classflags & (1 << CFL_STRONGAGAINSTPHYSICAL)))
	{ //this class is flagged to take less damage from physical attacks.
		//For now I'm just decreasing against any client-based attack, this can be changed later I guess.
		damage *= 0.5;
	}

	// check for completely getting out of the damage
	if (((targ->flags & FL_GODMODE) || (targ->client && targ->client->ps.powerups[PW_INVINCIBLE] > level.time))
		&& !(dflags & DAMAGE_NO_PROTECTION))
	{

		// if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
		// if the attacker was on the same team
		if (targ != attacker)
		{
			if (OnSameTeam(targ, attacker))
			{
				if (!g_friendlyFire.integer)
				{
					return;
				}
			}
			else if (attacker && attacker->inuse &&
				!attacker->client && attacker->activator &&
				targ != attacker->activator &&
				attacker->activator->inuse && attacker->activator->client)
			{ //emplaced guns don't hurt teammates of user
				if (OnSameTeam(targ, attacker->activator))
				{
					if (!g_friendlyFire.integer)
					{
						return;
					}
				}
			}
			else if (targ->inuse && targ->client &&
				level.gametype >= GT_TEAM &&
				attacker->s.number >= MAX_CLIENTS &&
				attacker->alliedTeam &&
				targ->client->sess.sessionTeam == attacker->alliedTeam &&
				!g_friendlyFire.integer)
			{ //things allied with my team should't hurt me.. I guess
				return;
			}
		}

		if (level.gametype == GT_JEDIMASTER && !g_friendlyFire.integer &&
			targ && targ->client && attacker && attacker->client &&
			targ != attacker && !targ->client->ps.isJediMaster && !attacker->client->ps.isJediMaster &&
			G_ThereIsAMaster())
		{
			return;
		}

		if (targ->s.number >= MAX_CLIENTS && targ->client
			&& targ->s.shouldtarget && targ->s.teamowner &&
			attacker && attacker->inuse && attacker->client && targ->s.owner >= 0 && targ->s.owner < MAX_CLIENTS)
		{
			gentity_t* targown = &g_entities[targ->s.owner];

			if (targown && targown->inuse && targown->client && OnSameTeam(targown, attacker))
			{
				if (!g_friendlyFire.integer)
				{
					return;
				}
			}
		}

		// check for godmode
		if ((targ->flags & FL_GODMODE) && targ->s.eType != ET_NPC)
		{
			return;
		}

		if (targ && targ->client && (targ->client->ps.eFlags & EF_INVULNERABLE) &&
			attacker && attacker->client && targ != attacker)
		{
			if (targ->client->invulnerableTimer <= level.time)
			{
				targ->client->ps.eFlags &= ~EF_INVULNERABLE;
			}
			else
			{
				return;
			}
		}
	}

	//NOTE: non-client objects hitting clients (and clients hitting clients) purposely doesn't obey this teamnodmg (for emplaced guns)
	if (attacker && !targ->client)
	{//attacker hit a non-client
		if (level.gametype == GT_SIEGE &&
			!g_ff_objectives.integer)
		{//in siege mode (and...?)
			if (targ->teamnodmg)
			{//targ shouldn't take damage from a certain team
				if (attacker->client)
				{//a client hit a non-client object
					if (targ->teamnodmg == attacker->client->sess.sessionTeam)
					{
						return;
					}
				}
				else if (attacker->teamnodmg)
				{//a non-client hit a non-client object
					//FIXME: maybe check alliedTeam instead?
					if (targ->teamnodmg == attacker->teamnodmg)
					{
						if (attacker->activator &&
							attacker->activator->inuse &&
							attacker->activator->s.number < MAX_CLIENTS &&
							attacker->activator->client &&
							attacker->activator->client->sess.sessionTeam != targ->teamnodmg)
						{ //uh, let them damage it I guess.
						}
						else
						{
							return;
						}
					}
				}
			}
		}
	}
	// battlesuit protects from all radius damage (but takes knockback)
	// and protects 50% against all damage
	if (client && client->ps.powerups[PW_BATTLESUIT])
	{
		G_AddEvent(targ, EV_POWERUP_BATTLESUIT, 0);
		if ((dflags& DAMAGE_RADIUS) || (mod == MOD_FALLING))
		{
			return;
		}
		damage *= 0.5;
	}

	// add to the attacker's hit counter (if the target isn't a general entity like a prox mine)
	if (attacker->client && targ != attacker && targ->health > 0
		&& targ->s.eType != ET_MISSILE
		&& targ->s.eType != ET_GENERAL
		&& client) {
		if (OnSameTeam(targ, attacker))
		{
			attacker->client->ps.persistant[PERS_HITS]--;
		}
		else
		{
			attacker->client->ps.persistant[PERS_HITS]++;
		}
		attacker->client->ps.persistant[PERS_ATTACKEE_ARMOR] = (targ->health << 8) | (client->ps.stats[STAT_ARMOR]);
	}

	// always give half damage if hurting self... but not in siege.  Heavy weapons need a counter.
	// calculated after knockback, so rocket jumping works
	if (targ == attacker && !(dflags & DAMAGE_NO_SELF_PROTECTION))
	{
		if (level.gametype == GT_SIEGE)
		{
			damage *= 1.5;
		}
		else
		{
			damage *= 0.5;
		}
	}


	if (damage < 1)
	{
		damage = 1;
	}
	//store the attacker as the potential killer in case we die by other means
	if (targ->client&& attacker&& attacker != targ)
	{
		if (targ->s.eType == ET_NPC && targ->s.NPC_class == CLASS_VEHICLE)
		{//vehicle
			G_ApplyVehicleOtherKiller(targ, inflictor, attacker, mod, qfalse);
		}
		else
		{//other client
			targ->client->ps.otherKiller = attacker->s.number;
			targ->client->ps.otherKillerTime = level.time + 5000;
			targ->client->ps.otherKillerDebounceTime = level.time + 100;
			targ->client->otherKillerMOD = mod;
			targ->client->otherKillerVehWeapon = 0;
			targ->client->otherKillerWeaponType = WP_NONE;
		}
	}
	take = damage;

	// save some from armor
	asave = CheckArmor(targ, take, dflags, mod);

	if (asave)
	{
		shieldAbsorbed = asave;
	}

	take -= asave;
	if (targ->client)
	{//update vehicle shields and armor, check for explode
		if (targ->client->NPC_class == CLASS_VEHICLE &&
			targ->m_pVehicle)
		{//FIXME: should be in its own function in g_vehicles.c now, too big to be here
			int surface = -1;
			if (attacker)
			{//so we know the last guy who shot at us
				targ->enemy = attacker;
			}

			if (targ->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
			{
				//((CVehicleNPC *)targ->NPC)->m_ulFlags |= CVehicleNPC::VEH_BUCKING;
			}

			targ->m_pVehicle->m_iShields = targ->client->ps.stats[STAT_ARMOR];
			G_VehUpdateShields(targ);
			targ->m_pVehicle->m_iArmor -= take;
			if (targ->m_pVehicle->m_iArmor <= 0)
			{
				targ->s.eFlags |= EF_DEAD;
				targ->client->ps.eFlags |= EF_DEAD;
				targ->m_pVehicle->m_iArmor = 0;
			}
			if (targ->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
			{//get the last surf that was hit
				if (targ->client && targ->client->g2LastSurfaceTime == level.time)
				{
					char hitSurface[MAX_QPATH];

					trap->G2API_GetSurfaceName(targ->ghoul2, targ->client->g2LastSurfaceHit, 0, hitSurface);

					if (hitSurface[0])
					{
						surface = G_ShipSurfaceForSurfName(&hitSurface[0]);

						if (take && surface > 0)
						{//hit a certain part of the ship
							int deathPoint = 0;

							targ->locationDamage[surface] += take;

							switch (surface)
							{
							case SHIPSURF_FRONT:
								deathPoint = targ->m_pVehicle->m_pVehicleInfo->health_front;
								break;
							case SHIPSURF_BACK:
								deathPoint = targ->m_pVehicle->m_pVehicleInfo->health_back;
								break;
							case SHIPSURF_RIGHT:
								deathPoint = targ->m_pVehicle->m_pVehicleInfo->health_right;
								break;
							case SHIPSURF_LEFT:
								deathPoint = targ->m_pVehicle->m_pVehicleInfo->health_left;
								break;
							default:
								break;
							}

							//presume 0 means it wasn't set and so it should never die.
							if (deathPoint)
							{
								if (targ->locationDamage[surface] >= deathPoint)
								{ //this area of the ship is now dead
									qboolean wasDying = (targ->m_pVehicle->m_iRemovedSurfaces != 0);
									if (G_FlyVehicleDestroySurface(targ, surface))
									{//actually took off a surface
										G_VehicleSetDamageLocFlags(targ, surface, deathPoint);
										if (!wasDying)
										{//always take credit for kill if they were healthy before
											targ->client->ps.otherKillerDebounceTime = 0;
										}
										G_ApplyVehicleOtherKiller(targ, inflictor, attacker, mod, qtrue);
									}
								}
								else
								{
									G_VehicleSetDamageLocFlags(targ, surface, deathPoint);
								}
							}
						}
					}
				}
			}
			if (targ->m_pVehicle->m_pVehicleInfo->type != VH_ANIMAL)
			{
				if (attacker
					&& targ != attacker
					&& point
					&& !VectorCompare(targ->client->ps.origin, point)
					&& targ->m_pVehicle->m_LandTrace.fraction >= 1.0f)
				{//just took a hit, knock us around
					vec3_t	vUp, impactDir;
					float	impactStrength = (damage / 200.0f) * 10.0f;
					float	dot = 0.0f;
					if (impactStrength > 10.0f)
					{
						impactStrength = 10.0f;
					}
					//pitch or roll us based on where we were hit
					AngleVectors(targ->m_pVehicle->m_vOrientation, NULL, NULL, vUp);
					VectorSubtract(point, targ->r.currentOrigin, impactDir);
					VectorNormalize(impactDir);
					if (surface <= 0)
					{//no surf guess where we were hit, then
						vec3_t	vFwd, vRight;
						AngleVectors(targ->m_pVehicle->m_vOrientation, vFwd, vRight, vUp);
						dot = DotProduct(vRight, impactDir);
						if (dot > 0.4f)
						{
							surface = SHIPSURF_RIGHT;
						}
						else if (dot < -0.4f)
						{
							surface = SHIPSURF_LEFT;
						}
						else
						{
							dot = DotProduct(vFwd, impactDir);
							if (dot > 0.0f)
							{
								surface = SHIPSURF_FRONT;
							}
							else
							{
								surface = SHIPSURF_BACK;
							}
						}
					}
					switch (surface)
					{
					case SHIPSURF_FRONT:
						dot = DotProduct(vUp, impactDir);
						if (dot > 0)
						{
							targ->m_pVehicle->m_vOrientation[PITCH] += impactStrength;
						}
						else
						{
							targ->m_pVehicle->m_vOrientation[PITCH] -= impactStrength;
						}
						break;
					case SHIPSURF_BACK:
						dot = DotProduct(vUp, impactDir);
						if (dot > 0)
						{
							targ->m_pVehicle->m_vOrientation[PITCH] -= impactStrength;
						}
						else
						{
							targ->m_pVehicle->m_vOrientation[PITCH] += impactStrength;
						}
						break;
					case SHIPSURF_RIGHT:
						dot = DotProduct(vUp, impactDir);
						if (dot > 0)
						{
							targ->m_pVehicle->m_vOrientation[ROLL] -= impactStrength;
						}
						else
						{
							targ->m_pVehicle->m_vOrientation[ROLL] += impactStrength;
						}
						break;
					case SHIPSURF_LEFT:
						dot = DotProduct(vUp, impactDir);
						if (dot > 0)
						{
							targ->m_pVehicle->m_vOrientation[ROLL] += impactStrength;
						}
						else
						{
							targ->m_pVehicle->m_vOrientation[ROLL] -= impactStrength;
						}
						break;
					}

				}
			}
		}
	}

	if (targ->NPC && client && (mod == MOD_DEMP2 || mod == MOD_DEMP2_ALT))
	{//NPCs get stunned by demps
		if (client->NPC_class == CLASS_SABER_DROID ||
			client->NPC_class == CLASS_ASSASSIN_DROID ||
			client->NPC_class == CLASS_GONK ||
			client->NPC_class == CLASS_MOUSE ||
			client->NPC_class == CLASS_PROBE ||
			client->NPC_class == CLASS_PROTOCOL ||
			client->NPC_class == CLASS_R2D2 ||
			client->NPC_class == CLASS_R5D2 ||
			client->NPC_class == CLASS_SEEKER ||
			client->NPC_class == CLASS_INTERROGATOR)
		{//these NPC_types take way more damage from demps
			take *= 7;
		}
		TIMER_Set(targ, "DEMP2_StunTime", Q_irand(1000, 2000));
	}
	if (mod == MOD_DEMP2 || mod == MOD_DEMP2_ALT)
	{
		if (client)
		{
			if (client->NPC_class == CLASS_VEHICLE
				&& targ->m_pVehicle
				&& targ->m_pVehicle->m_pVehicleInfo
				&& targ->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
			{//all damage goes into the disruption of shields and systems
				take = 0;
			}
			else
			{

				if (client->jetPackOn)
				{ //disable jetpack temporarily
					Jetpack_Off(targ);
					client->jetPackToggleTime = level.time + Q_irand(3000, 10000);
				}

				if (client->NPC_class == CLASS_BOBAFETT)
				{ // DEMP2 also disables npc jetpack
					Boba_FlyStop(targ);
				}

				if (client->nextbotclass == BCLASS_PROTOCOL || client->nextbotclass == BCLASS_SEEKER ||
					client->nextbotclass == BCLASS_R2D2 || client->nextbotclass == BCLASS_R5D2 ||
					client->nextbotclass == BCLASS_MOUSE || client->nextbotclass == BCLASS_GONK)
				{
					// DEMP2 does more damage to these guys.
					take *= 2;
				}
				else if (client->nextbotclass == BCLASS_PROBE || client->nextbotclass == BCLASS_INTERROGATOR ||
					client->nextbotclass == BCLASS_MARK1 || client->nextbotclass == BCLASS_MARK2 ||
					client->nextbotclass == BCLASS_SENTRY || client->nextbotclass == BCLASS_SBD ||
					client->nextbotclass == BCLASS_BATTLEDROID || client->nextbotclass == BCLASS_DROIDEKA)
				{
					// DEMP2 does way more damage to these guys.
					take *= 5;
				}
				else if (client->NPC_class == CLASS_PROTOCOL || client->NPC_class == CLASS_SEEKER ||
					client->NPC_class == CLASS_R2D2 || client->NPC_class == CLASS_R5D2 ||
					client->NPC_class == CLASS_MOUSE || client->NPC_class == CLASS_GONK)
				{
					// DEMP2 does more damage to these guys.
					take *= 2;
				}
				else if (client->NPC_class == CLASS_PROBE || client->NPC_class == CLASS_INTERROGATOR ||
					client->NPC_class == CLASS_MARK1 || client->NPC_class == CLASS_MARK2 ||
					client->NPC_class == CLASS_SENTRY || client->NPC_class == CLASS_ATST)
				{
					// DEMP2 does way more damage to these guys.
					take *= 5;
				}
				else
				{
					if (take > 0)
					{
						take /= 3;
						if (take < 1)
						{
							take = 1;
						}
					}
				}
			}
		}
	}

	if (mod == MOD_SLIME || mod == MOD_LAVA)
	{
		// Hazard Troopers Don't Mind Acid Rain
		if (targ->client->NPC_class == CLASS_HAZARD_TROOPER && !(dflags & DAMAGE_NO_PROTECTION))
		{
			take = 0;
		}

		/*if (mod == MOD_SLIME)
		{
			trace_t		testTrace;
			vec3_t		testDirection;
			vec3_t		testStartPos;
			vec3_t		testEndPos;
			
			testDirection[0] = (Q_flrand(0.0f, 1.0f) * 0.5f) - 0.25f;
			testDirection[1] = (Q_flrand(0.0f, 1.0f) * 0.5f) - 0.25f;
			testDirection[2] = 1.0f;
			VectorMA(targ->r.currentOrigin, 60.0f, testDirection, testStartPos);
			VectorCopy(targ->r.currentOrigin, testEndPos);
			testEndPos[0] += (Q_flrand(0.0f, 1.0f) * 8.0f) - 4.0f;
			testEndPos[1] += (Q_flrand(0.0f, 1.0f) * 8.0f) - 4.0f;
			testEndPos[2] += (Q_flrand(0.0f, 1.0f) * 8.0f);
			
			trap->Trace(&testTrace, testStartPos, NULL, NULL, testEndPos, ENTITYNUM_NONE, MASK_SHOT, qfalse, 0, 0);

			float chanceOfFizz = flrand(0.0f, 1.0f);
			int rSaberNum = 0;
			int rBladeNum = 0;
			targ->client->saber[rSaberNum].blade[rBladeNum].storageTime = level.time;
			
			if (!testTrace.startsolid &&
				!testTrace.allsolid &&
				testTrace.entityNum == targ->s.number &&
				testTrace.G2CollisionMap[0].mEntityNum != -1)
			{
				if (chanceOfFizz > 0)
				{
					vec3_t ang = { 0, 0, 0 };
					ang[0] = flrand(0, 360);
					ang[1] = flrand(0, 360);
					ang[2] = flrand(0, 360);
					
					VectorMA(targ->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, targ->client->saber[rSaberNum].blade[rBladeNum].lengthMax * flrand(0, 1), targ->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, testEndPos);
					G_PlayEffectID(G_EffectIndex("world / acid_fizz.efx"), testEndPos, ang);

					targ->client->poisonDamage = 18;
					targ->client->poisonTime = level.time + 1000;
				}
			}
			TIMER_Set(targ, "AcidPainDebounce", 200 + (10000.0f * Q_flrand(0.0f, 1.0f) * chanceOfFizz));
		}*/
	}

	if (g_debugDamage.integer)
	{
		trap->Print("%i: client:%i health:%i damage:%i armor:%i\n", level.time, targ->s.number,	targ->health, take, asave);
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if (client) 
	{
		if (attacker)
		{
			client->ps.persistant[PERS_ATTACKER] = attacker->s.number;
		}
		else
		{
			client->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;
		}
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
		if (dir) 
		{
			VectorCopy(dir, client->damage_from);
			client->damage_fromWorld = qfalse;
		}
		else
		{
			VectorCopy(targ->r.currentOrigin, client->damage_from);
			client->damage_fromWorld = qtrue;
		}

		if (attacker && attacker->client)
		{
			BotDamageNotification(client, attacker);
		}
		else if (inflictor && inflictor->client)
		{
			BotDamageNotification(client, inflictor);
		}
	}

	// See if it's the player hurting the emeny flag carrier
	if (level.gametype == GT_CTF || level.gametype == GT_CTY) {
		Team_CheckHurtCarrier(targ, attacker);
	}

	if (targ->client && G_StandardHumanoid(targ))
	{// set the last client who damaged the target
		targ->client->lasthurt_client = attacker->s.number;
		targ->client->lasthurt_mod = mod;
		// Modify the damage for location damage
		if (point && targ && targ->health > 0 && attacker && take)
		{
			take = G_LocationDamage(point, targ, attacker, take);
		}
		else
		{
			targ->client->lasthurt_location = LOCATION_NONE;
		}
	}

	if (targ->client && attacker->client && targ->health > 0 && G_StandardHumanoid(targ))
	{ //do head shots
		if (inflictor->s.weapon == WP_BLASTER
			|| (inflictor->s.weapon == WP_FLECHETTE)
			|| (inflictor->s.weapon == WP_BRYAR_PISTOL)
			|| (inflictor->s.weapon == WP_TURRET)
			|| (inflictor->s.weapon == WP_BRYAR_OLD)
			|| (inflictor->s.weapon == WP_EMPLACED_GUN))
		{
			targ_maxs2 = targ->maxs[2];
			// handling crouching
			if (targ->client->ps.pm_flags& PMF_DUCKED)
			{
				height = (abs(targ->mins[2]) + targ_maxs2) * (0.75);
			}
			else
			{
				height = abs(targ->mins[2]) + targ_maxs2;
			}
			// project the z component of point 
			// onto the z component of the model's origin
			// this results in the z component from the origin at 0
			z_rel = point[2] - targ->r.currentOrigin[2] + abs(targ->mins[2]);
			z_ratio = z_rel / height;

			if (z_ratio > 0.90)
			{
				take = G_LocationDamage(point, targ, attacker, take);
				mod = MOD_HEADSHOT;
			}
			else
			{
				take = G_LocationDamage(point, targ, attacker, take);
				mod = MOD_BODYSHOT;
			}
		}
	}

	if (!(dflags & DAMAGE_NO_PROTECTION))
	{//protect overridden by no_protection
		if (take && targ->client && (targ->client->ps.fd.forcePowersActive & (1 << FP_PROTECT)))
		{
			if (targ->client->ps.fd.forcePower)
			{
				int maxtake = take;

				if (targ->client->forcePowerSoundDebounce < level.time)
				{
					G_PreDefSound(targ->client->ps.origin, PDSOUND_PROTECTHIT);
					targ->client->forcePowerSoundDebounce = level.time + 400;
				}

				if (targ->client->ps.fd.forcePowerLevel[FP_PROTECT] == FORCE_LEVEL_1)
				{
					famt = 1;
					hamt = 0.40f;

					if (maxtake > 100)
					{
						maxtake = 100;
					}
				}
				else if (targ->client->ps.fd.forcePowerLevel[FP_PROTECT] == FORCE_LEVEL_2)
				{
					famt = 0.5f;
					hamt = 0.60f;

					if (maxtake > 200)
					{
						maxtake = 200;
					}
				}
				else if (targ->client->ps.fd.forcePowerLevel[FP_PROTECT] == FORCE_LEVEL_3)
				{
					famt = 0.25f;
					hamt = 0.80f;

					if (maxtake > 400)
					{
						maxtake = 400;
					}
				}

				if (!targ->client->ps.powerups[PW_FORCE_BOON])
				{
					targ->client->ps.fd.forcePower -= maxtake * famt;
				}
				else
				{
					targ->client->ps.fd.forcePower -= (maxtake * famt) / 2;
				}
				subamt = (maxtake * hamt) + (take - maxtake);
				if (targ->client->ps.fd.forcePower < 0)
				{
					subamt += targ->client->ps.fd.forcePower;
					targ->client->ps.fd.forcePower = 0;
				}
				if (subamt)
				{
					take -= subamt;

					if (take < 0)
					{
						take = 0;
					}
				}
			}
		}
	}

	if (shieldAbsorbed)
	{
		gentity_t* evEnt;

		// Send off an event to show a shield shell on the player, pointing in the right direction.
		evEnt = G_TempEntity(targ->r.currentOrigin, EV_SHIELD_HIT);
		evEnt->s.otherEntityNum = targ->s.number;
		evEnt->s.eventParm = DirToByte(dir);
		evEnt->s.time2 = shieldAbsorbed;
	}

	// do the damage
	if (take)
	{
		if (targ->client && targ->s.number < MAX_CLIENTS &&
			(mod == MOD_DEMP2 || mod == MOD_DEMP2_ALT))
		{ //uh.. shock them or something. what the hell, I don't know.
			if (targ->client->ps.weaponTime <= 0)
			{ //yeah, we were supposed to be beta a week ago, I don't feel like
				//breaking the game so I'm gonna be safe and only do this only
				//if your weapon is not busy
				targ->client->ps.weaponTime = 2000;
				targ->client->ps.electrifyTime = level.time + 2000;
				if (targ->client->ps.weaponstate == WEAPON_CHARGING ||
					targ->client->ps.weaponstate == WEAPON_CHARGING_ALT)
				{
					targ->client->ps.weaponstate = WEAPON_READY;
				}
			}
		}

		if (!(dflags & DAMAGE_NO_PROTECTION))
		{//rage overridden by no_protection
			if (targ->client && (targ->client->ps.fd.forcePowersActive & (1 << FP_RAGE)) && (inflictor->client || attacker->client))
			{
				take /= (targ->client->ps.fd.forcePowerLevel[FP_RAGE] + 1);
			}
		}

		if (targ->client&& targ->health > 0 && attacker&& attacker->client&& mod != MOD_FORCE_LIGHTNING)
		{//targ is creature and attacker is creature
			if (take > targ->health)
			{//damage is greated than target's health, only give experience for damage used to kill victim
				AddFatigueHurtBonusMax(attacker, targ);
			}
			else
			{
				AddFatigueHurtBonus(attacker, targ);
			}
		}

		targ->health = targ->health - take;

		if ((targ->flags & FL_UNDYING))
		{//take damage down to 1, but never die
			if (targ->health < 1)
			{
				//trigger death script (this flag is normally used for NPC bosses who you want to lose, but not die onscreen before the script is run.
				G_ActivateBehavior(targ, BSET_DEATH);
				targ->health = 1;
			}
		}

		if (targ->client) {
			targ->client->ps.stats[STAT_HEALTH] = targ->health;
		}

		if (!(dflags & DAMAGE_NO_PROTECTION))
		{//rage overridden by no_protection
			if (targ->client && (targ->client->ps.fd.forcePowersActive & (1 << FP_RAGE)) && (inflictor->client || attacker->client))
			{
				if (targ->health <= 0)
				{
					targ->health = 1;
				}
				if (targ->client->ps.stats[STAT_HEALTH] <= 0)
				{
					targ->client->ps.stats[STAT_HEALTH] = 1;
				}
			}
		}

		//We want to go ahead and set gPainHitLoc regardless of if we have a pain func,
		//so we can adjust the location damage too.
		if (targ->client && targ->ghoul2 && targ->client->g2LastSurfaceTime == level.time)
		{ //We updated the hit surface this frame, so it's valid.
			char hitSurface[MAX_QPATH];

			trap->G2API_GetSurfaceName(targ->ghoul2, targ->client->g2LastSurfaceHit, 0, hitSurface);

			if (hitSurface[0])
			{
				G_GetHitLocFromSurfName(targ, hitSurface, &gPainHitLoc, point, dir, vec3_origin, mod);
			}
			else
			{
				gPainHitLoc = -1;
			}

			if (gPainHitLoc < HL_MAX && gPainHitLoc >= 0 && targ->locationDamage[gPainHitLoc] < Q3_INFINITE &&
				(targ->s.eType == ET_PLAYER || targ->s.NPC_class != CLASS_VEHICLE))
			{
				targ->locationDamage[gPainHitLoc] += take;

				if (g_armBreakage.integer && !targ->client->ps.brokenLimbs &&
					targ->client->ps.stats[STAT_HEALTH] > 0 && targ->health > 0 &&
					!(targ->s.eFlags & EF_DEAD))
				{ //check for breakage
					if (targ->locationDamage[HL_ARM_RT] + targ->locationDamage[HL_HAND_RT] >= 80)
					{
						G_BreakArm(targ, BROKENLIMB_RARM);
					}
					else if (targ->locationDamage[HL_ARM_LT] + targ->locationDamage[HL_HAND_LT] >= 80)
					{
						G_BreakArm(targ, BROKENLIMB_LARM);
					}
				}
			}
		}
		else
		{
			gPainHitLoc = -1;
		}

		if (targ->maxHealth)
		{ //if this is non-zero this guy should be updated his s.health to send to the client
			G_ScaleNetHealth(targ);
		}

		if (targ->health <= 0)
		{
			if (knockback && (dflags & DAMAGE_DEATH_KNOCKBACK))
			{//only do knockback on death
				if (mod == MOD_BLASTER)
				{//special case because this is shotgun-ish damage, we need to multiply the knockback
					knockback *= 8;
				}
				if (mod == MOD_FLECHETTE || mod == MOD_BOWCASTER)
				{//special case because this is shotgun-ish damage, we need to multiply the knockback
					knockback *= 12;
				}
				if (mod == MOD_CONC || mod == MOD_CONC_ALT)
				{//special case because this is shotgun-ish damage, we need to multiply the knockback
					knockback *= 16;
				}
				G_ApplyKnockback(targ, newDir, knockback);
			}
			if (client)
			{
				targ->flags |= FL_NO_KNOCKBACK;

				if (point)
				{
					VectorCopy(point, targ->pos1);
				}
				else
				{
					VectorCopy(targ->client->ps.origin, targ->pos1);
				}
			}
			else if (targ->s.eType == ET_NPC)
			{ //g2animent
				VectorCopy(point, targ->pos1);
			}

			if (targ->health < -999)
				targ->health = -999;

			// If we are a breaking glass brush, store the damage point so we can do cool things with it.
			if (targ->r.svFlags & SVF_GLASS_BRUSH)
			{
				VectorCopy(point, targ->pos1);
				if (dir)
				{
					VectorCopy(dir, targ->pos2);
				}
				else
				{
					VectorClear(targ->pos2);
				}
			}

			if (targ->s.eType == ET_NPC &&
				targ->client &&
				(targ->s.eFlags & EF_DEAD))
			{ //an NPC that's already dead. Maybe we can cut some more limbs off!
				if ((mod == MOD_SABER || (mod == MOD_MELEE && G_HeavyMelee(attacker)))//saber or heavy melee (claws)
					&& take > 2
					&& !(dflags & DAMAGE_NO_DISMEMBER))
				{
					G_CheckForDismemberment(targ, attacker, targ->pos1, take, targ->client->ps.torsoAnim, qtrue);
				}
			}

			targ->enemy = attacker;
			targ->die(targ, inflictor, attacker, take, mod);
			G_ActivateBehavior(targ, BSET_DEATH);
			return;
		}
		else
		{
			if (0)
			{//getting hurt makes you let go of the wall
				if (targ->client && (targ->client->ps.pm_flags & PMF_STUCK_TO_WALL))
				{
					G_LetGoOfWall(targ);
				}
			}
			if (targ->pain)
			{
				if (targ->s.eType != ET_NPC || mod != MOD_SABER || take > 1)
				{ //don't even notify NPCs of pain if it's just idle saber damage
					gPainMOD = mod;
					if (point)
					{
						VectorCopy(point, gPainPoint);
					}
					else
					{
						VectorCopy(targ->r.currentOrigin, gPainPoint);
					}
					targ->pain(targ, attacker, take);
				}
			}
		}

		G_LogWeaponDamage(attacker->s.number, mod, take);
	}

}

void G_DamageFromKiller(gentity_t * pEnt, gentity_t * pVehEnt, gentity_t * attacker, vec3_t org, int damage, int dflags, int mod)
{
	gentity_t* killer = attacker, * inflictor = attacker;
	qboolean tempInflictor = qfalse;
	if (!pEnt || !pVehEnt || !pVehEnt->client)
	{
		return;
	}
	if (pVehEnt->client->ps.otherKiller < ENTITYNUM_WORLD &&
		pVehEnt->client->ps.otherKillerTime > level.time)
	{
		gentity_t* potentialKiller = &g_entities[pVehEnt->client->ps.otherKiller];

		if (potentialKiller->inuse)
		{ //he's valid I guess
			killer = potentialKiller;
			mod = pVehEnt->client->otherKillerMOD;
			inflictor = killer;
			if (pVehEnt->client->otherKillerVehWeapon > 0)
			{
				inflictor = G_Spawn();
				if (inflictor)
				{//fake up the inflictor
					tempInflictor = qtrue;
					inflictor->classname = "vehicle_proj";
					inflictor->s.otherEntityNum2 = pVehEnt->client->otherKillerVehWeapon - 1;
					inflictor->s.weapon = pVehEnt->client->otherKillerWeaponType;
				}
			}
		}
	}
	if (killer&& killer->s.eType == ET_NPC && killer->s.NPC_class == CLASS_VEHICLE && killer->m_pVehicle && killer->m_pVehicle->m_pPilot)
	{
		killer = (gentity_t*)killer->m_pVehicle->m_pPilot;
	}
	G_Damage(pEnt, inflictor, killer, NULL, org, damage, dflags, mod);
	if (tempInflictor)
	{
		G_FreeEntity(inflictor);
	}
}

/*
============
CanDamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage(gentity_t * targ, vec3_t origin) {
	vec3_t	dest;
	trace_t	tr;
	vec3_t	midpoint;

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	VectorAdd(targ->r.absmin, targ->r.absmax, midpoint);
	VectorScale(midpoint, 0.5, midpoint);

	VectorCopy(midpoint, dest);
	trap->Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);
	if (tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;

	// this should probably check in the plane of projection,
	// rather than in world coordinate, and also include Z
	VectorCopy(midpoint, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trap->Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trap->Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trap->Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trap->Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);
	if (tr.fraction == 1.0)
		return qtrue;


	return qfalse;
}


/*
============
G_RadiusDamage
============
*/
qboolean G_DoDodge(gentity_t * self, gentity_t * shooter, vec3_t impactPoint, int hitLoc, int* dmg, int mod);
qboolean G_RadiusDamage(vec3_t origin, gentity_t * attacker, float damage, float radius,
	gentity_t * ignore, gentity_t * missile, int mod) {
	float		points, dist;
	gentity_t* ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;
	qboolean	hitClient = qfalse;
	qboolean	roastPeople = qfalse;
	int			dodgeDmg;
	int			dFlags = DAMAGE_RADIUS;

	//oh well.. maybe sometime? I am trying to cut down on tempent use.

	if (radius < 1) {
		radius = 1;
	}

	for (i = 0; i < 3; i++) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	if (mod == MOD_GAS)
	{
		dFlags |= DAMAGE_NO_KNOCKBACK;
	}

	numListedEntities = trap->EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	for (e = 0; e < numListedEntities; e++) {
		ent = &g_entities[entityList[e]];

		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		// find the distance from the edge of the bounding box
		for (i = 0; i < 3; i++) {
			if (origin[i] < ent->r.absmin[i]) {
				v[i] = ent->r.absmin[i] - origin[i];
			}
			else if (origin[i] > ent->r.absmax[i]) {
				v[i] = origin[i] - ent->r.absmax[i];
			}
			else {
				v[i] = 0;
			}
		}

		dist = VectorLength(v);
		if (dist >= radius) {
			continue;
		}

		points = damage * (1.0 - dist / radius);
		dodgeDmg = (int)points;

		if (CanDamage(ent, origin) && !G_DoDodge(ent, attacker, origin, -1, &dodgeDmg, mod))
		{
			points = (float)dodgeDmg;

			if (LogAccuracyHit(ent, attacker))
			{
				hitClient = qtrue;
			}
			VectorSubtract(ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;
			if (attacker && attacker->inuse && attacker->client &&
				attacker->s.eType == ET_NPC && attacker->s.NPC_class == CLASS_VEHICLE &&
				attacker->m_pVehicle && attacker->m_pVehicle->m_pPilot)
			{ //say my pilot did it.
				G_Damage(ent, missile, (gentity_t*)attacker->m_pVehicle->m_pPilot, dir, origin, (int)points, DAMAGE_RADIUS, mod);
			}
			else
			{
				G_Damage(ent, missile, attacker, dir, origin, (int)points, DAMAGE_RADIUS, mod);
			}

			if (ent && ent->client && roastPeople && missile &&
				!VectorCompare(ent->r.currentOrigin, missile->r.currentOrigin))
			{ //the thing calling this function can create burn marks on people, so create an event to do so
				gentity_t* evEnt = G_TempEntity(ent->r.currentOrigin, EV_GHOUL2_MARK);

				evEnt->s.otherEntityNum = ent->s.number; //the entity the mark should be placed on
				evEnt->s.weapon = WP_ROCKET_LAUNCHER; //always say it's rocket so we make the right mark

				//Try to place the decal by going from the missile location to the location of the person that was hit
				VectorCopy(missile->r.currentOrigin, evEnt->s.origin);
				VectorCopy(ent->r.currentOrigin, evEnt->s.origin2);

				//it's hacky, but we want to move it up so it's more likely to hit
				//the torso.
				if (missile->r.currentOrigin[2] < ent->r.currentOrigin[2])
				{ //move it up less so the decal is placed lower on the model then
					evEnt->s.origin2[2] += 8;
				}
				else
				{
					evEnt->s.origin2[2] += 24;
				}

				//Special col check
				evEnt->s.eventParm = 1;
			}
		}
	}

	return hitClient;
}

#define FATIGUE_KILLBONUS 75 //the FP bonus you get for killing another player
extern void WP_ForcePowerRegenerate(gentity_t * self, int overrideAmt);
void AddFatigueKillBonus(gentity_t * attacker, gentity_t * victim)
{//get a small bonus for killing an enemy
	if (!attacker || !attacker->client || !victim || !victim->client)
	{
		return;
	}

	if (Manual_Saberblocking(attacker))//DONT GET THIS BONUS IF YOUR A BLOCK SPAMMER
	{
		return;
	}

	//add bonus
	WP_ForcePowerRegenerate(attacker, FATIGUE_KILLBONUS);
	attacker->client->ps.fd.blockPoints += FATIGUE_KILLBONUS;
}

#define FATIGUE_DAMAGEBONUS 25 //the FP bonus you get for killing another player
void AddFatigueDamageBonus(gentity_t * attacker, gentity_t * victim)
{//get a small bonus for killing an enemy
	if (!attacker || !attacker->client || !victim || !victim->client)
	{
		return;
	}

	if (Manual_Saberblocking(attacker))//DONT GET THIS BONUS IF YOUR A BLOCK SPAMMER
	{
		return;
	}

	//add bonus
	WP_ForcePowerRegenerate(attacker, FATIGUE_DAMAGEBONUS);
	attacker->client->ps.fd.blockPoints += FATIGUE_DAMAGEBONUS;
}

#define FATIGUE_HURTBONUS 5 //the FP bonus you get for killing another player
void AddFatigueHurtBonus(gentity_t * attacker, gentity_t * victim)
{//get a small bonus for killing an enemy
	if (!attacker || !attacker->client || !victim || !victim->client)
	{
		return;
	}

	if (Manual_Saberblocking(attacker))//DONT GET THIS BONUS IF YOUR A BLOCK SPAMMER
	{
		return;
	}

	//add bonus
	WP_ForcePowerRegenerate(attacker, FATIGUE_HURTBONUS);
	attacker->client->ps.fd.blockPoints += FATIGUE_HURTBONUS;
}

#define FATIGUE_HURTBONUSMAX 10 //the FP bonus you get for killing another player
void AddFatigueHurtBonusMax(gentity_t * attacker, gentity_t * victim)
{//get a small bonus for killing an enemy
	if (!attacker || !attacker->client || !victim || !victim->client)
	{
		return;
	}

	if (Manual_Saberblocking(attacker))//DONT GET THIS BONUS IF YOUR A BLOCK SPAMMER
	{
		return;
	}

	//add bonus
	WP_ForcePowerRegenerate(attacker, FATIGUE_HURTBONUSMAX);
	attacker->client->ps.fd.blockPoints += FATIGUE_HURTBONUSMAX;
}

void G_DodgeDrain(gentity_t * victim, gentity_t * attacker, int amount)
{//drains DP from victim.  Also awards experience points to the attacker.
	if (!g_friendlyFire.integer && OnSameTeam(victim, attacker))
	{//don't drain DP if we're hit by a team member
		return;
	}

	if (victim->flags& FL_GODMODE)
		return;

	if (victim->client&& victim->client->ps.fd.forcePowersActive& (1 << FP_PROTECT))
	{
		amount /= 2;
		if (amount < 1)
			amount = 1;
	}

	victim->client->ps.stats[STAT_DODGE] -= amount;

	if (attacker->client && attacker->client->ps.torsoAnim == saberMoveData[16].animToUse)
	{//In DFA?
		victim->client->ps.saberAttackChainCount += MISHAPLEVEL_OVERLOAD;
	}

	if (victim->client->ps.stats[STAT_DODGE] < 0)
	{
		victim->client->ps.stats[STAT_DODGE] = 0;
	}
}


