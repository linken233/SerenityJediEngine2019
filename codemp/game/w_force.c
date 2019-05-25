/*
===========================================================================
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



#include "b_local.h"
#include "w_saber.h"
#include "ai_main.h"
#include "ghoul2/G2.h"

#define METROID_JUMP 1

//NEEDED FOR MIND-TRICK on NPCS=========================================================
extern void NPC_PlayConfusionSound(gentity_t* self);
extern void NPC_Jedi_PlayConfusionSound(gentity_t* self);
extern void NPC_UseResponse(gentity_t* self, gentity_t* user, qboolean useWhenDone);
//NEEDED FOR MIND-TRICK on NPCS=========================================================
extern void Jedi_Decloak(gentity_t* self);
extern qboolean WalkCheck(gentity_t* self);
extern int Manual_Saberblocking(gentity_t* defender);
extern int Manual_HandBlocking(gentity_t* defender);
extern qboolean BG_FullBodyTauntAnim(int anim);
extern qboolean PM_SaberInBrokenParry(int move);
extern qboolean BG_InSlowBounce(playerState_t* ps);
extern qboolean PM_LightningBlockAnim(int anim);
extern void G_KnockOver(gentity_t* self, gentity_t* attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock);
extern qboolean PM_InKnockDown(playerState_t* ps);
extern void G_AddMercBalance(gentity_t* self, int amount);//add balance to flamethrower
extern qboolean BG_SaberAttacking(playerState_t* ps);
extern qboolean PM_SaberInTransitionAny(int move);
extern qboolean PM_CrouchAnim(int anim);
extern qboolean PM_RestAnim(int anim);
extern void BG_ReduceSaberMishapLevel(playerState_t* ps);
extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock);
extern qboolean PM_SaberInParry(int move);
extern void BG_AddFatigue(playerState_t* ps, int Fatigue);
extern void Boba_FlyStart(gentity_t* self);
extern qboolean	in_camera;
extern qboolean PM_RunningAnim(int anim);
extern void G_ManualBlockMissile(gentity_t* ent, gentity_t* missile, vec3_t forward, forcePowers_t powerToUse);
extern void G_ReflectMissile(gentity_t* ent, gentity_t* missile, vec3_t forward, forcePowers_t powerToUse);
extern void BG_ReduceBlasterMishapLevel(playerState_t* ps);
extern void G_SoundOnEnt(gentity_t* ent, soundChannel_t channel, const char* soundPath);
extern saberInfo_t* BG_MySaber(int clientNum, int saberNum);
int IsPressingDashButton(gentity_t* self);
extern qboolean G_ControlledByPlayer(gentity_t* self);
extern qboolean PM_SaberCanInterruptMove(int move, int anim);
extern void	Boba_FireWristMissile(gentity_t* self, int whichMissile);
extern void Boba_EndWristMissile(gentity_t* self, int whichMissile);
extern qboolean PM_RollingAnim(int anim);
extern void Jedi_PlayBlockedPushSound(gentity_t* self);
extern bot_state_t* botstates[MAX_CLIENTS];
extern void Touch_Button(gentity_t* ent, gentity_t* other, trace_t* trace);
extern void player_Freeze(gentity_t* self);
extern void Player_CheckFreeze(gentity_t* self);

int		speedLoopSound = 0;
int		rageLoopSound = 0;
int		protectLoopSound = 0;
int		absorbLoopSound = 0;
int		seeLoopSound = 0;
int		ysalamiriLoopSound = 0;

#define FORCE_VELOCITY_DAMAGE 0

#define FORCE_SPEED_DURATION_FORCE_LEVEL_1 1500.0f
#define FORCE_SPEED_DURATION_FORCE_LEVEL_2 2000.0f
#define FORCE_SPEED_DURATION_FORCE_LEVEL_3 10000.0f

int ForceShootDrain(gentity_t* self);

gentity_t* G_PreDefSound(vec3_t org, int pdSound)
{
	gentity_t* te;

	te = G_TempEntity(org, EV_PREDEFSOUND);
	te->s.eventParm = pdSound;
	VectorCopy(org, te->s.origin);

	return te;
}

qboolean CheckPushItem(gentity_t* ent)
{
	if (!ent->item)
		return qfalse;

	if (ent->item->giType == IT_AMMO ||
		ent->item->giType == IT_HEALTH ||
		ent->item->giType == IT_ARMOR ||
		ent->item->giType == IT_HOLDABLE)
	{
		return qtrue; // these don't have placeholders
	}

	if (ent->item->giType == IT_WEAPON
		|| (g_pushitems.integer == 2 && ent->item->giType == IT_TEAM) || ent->item->giType == IT_POWERUP)
	{// check for if dropped item
		if (ent->r.svFlags& EF_DROPPEDWEAPON)
		{
			return qtrue; // these don't have placeholders
		}
		if (ent->flags& FL_DROPPED_ITEM)
		{
			return qtrue; // these don't have placeholders
		}
	}

	return qfalse;
}

const int forcePowerMinRank[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS] = //0 == neutral
{
	{
		999,//FP_HEAL,//instant
		999,//FP_LEVITATION,//hold/duration
		999,//FP_SPEED,//duration
		999,//FP_PUSH,//hold/duration
		999,//FP_PULL,//hold/duration
		999,//FP_TELEPATHY,//instant
		999,//FP_GRIP,//hold/duration
		999,//FP_LIGHTNING,//hold/duration
		999,//FP_RAGE,//duration
		999,//FP_PROTECT,//duration
		999,//FP_ABSORB,//duration
		999,//FP_TEAM_HEAL,//instant
		999,//FP_TEAM_FORCE,//instant
		999,//FP_DRAIN,//hold/duration
		999,//FP_SEE,//duration
		999,//FP_SABER_OFFENSE,
		999,//FP_SABER_DEFENSE,
		999//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		10,//FP_HEAL,//instant
		0,//FP_LEVITATION,//hold/duration
		0,//FP_SPEED,//duration
		0,//FP_PUSH,//hold/duration
		0,//FP_PULL,//hold/duration
		10,//FP_TELEPATHY,//instant
		15,//FP_GRIP,//hold/duration
		10,//FP_LIGHTNING,//hold/duration
		15,//FP_RAGE,//duration
		15,//FP_PROTECT,//duration
		15,//FP_ABSORB,//duration
		10,//FP_TEAM_HEAL,//instant
		10,//FP_TEAM_FORCE,//instant
		10,//FP_DRAIN,//hold/duration
		5,//FP_SEE,//duration
		0,//FP_SABER_OFFENSE,
		0,//FP_SABER_DEFENSE,
		0//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		10,//FP_HEAL,//instant
		0,//FP_LEVITATION,//hold/duration
		0,//FP_SPEED,//duration
		0,//FP_PUSH,//hold/duration
		0,//FP_PULL,//hold/duration
		10,//FP_TELEPATHY,//instant
		15,//FP_GRIP,//hold/duration
		10,//FP_LIGHTNING,//hold/duration
		15,//FP_RAGE,//duration
		15,//FP_PROTECT,//duration
		15,//FP_ABSORB,//duration
		10,//FP_TEAM_HEAL,//instant
		10,//FP_TEAM_FORCE,//instant
		10,//FP_DRAIN,//hold/duration
		5,//FP_SEE,//duration
		5,//FP_SABER_OFFENSE,
		5,//FP_SABER_DEFENSE,
		5//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		10,//FP_HEAL,//instant
		0,//FP_LEVITATION,//hold/duration
		0,//FP_SPEED,//duration
		0,//FP_PUSH,//hold/duration
		0,//FP_PULL,//hold/duration
		10,//FP_TELEPATHY,//instant
		15,//FP_GRIP,//hold/duration
		10,//FP_LIGHTNING,//hold/duration
		15,//FP_RAGE,//duration
		15,//FP_PROTECT,//duration
		15,//FP_ABSORB,//duration
		10,//FP_TEAM_HEAL,//instant
		10,//FP_TEAM_FORCE,//instant
		10,//FP_DRAIN,//hold/duration
		5,//FP_SEE,//duration
		10,//FP_SABER_OFFENSE,
		10,//FP_SABER_DEFENSE,
		10//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	}
};

const int mindTrickTime[NUM_FORCE_POWER_LEVELS] =
{
	0,//none
	5000,
	10000,
	15000
};

float forcePushCone[NUM_FORCE_POWER_LEVELS] =
{
	1.0f,//none
	1.0f,
	0.8f,
	0.6f
};

float forcePullCone[NUM_FORCE_POWER_LEVELS] =
{
	1.0f,//none
	1.0f,
	1.0f,
	0.8f
};

#define SK_DP_FORFORCE		.5f	//determines the number of DP points players get for each skill point dedicated to Force Powers.
#define SK_DP_FORMERC		1/6.0f	//determines the number of DP points get for each skill point dedicated to gunner/merc skills.
void DetermineDodgeMax(gentity_t * ent)
{//sets the maximum number of dodge points this player should have.  This is based on their skill point allociation.
	int i;
	int skillCount;
	float dodgeMax = 0;

	assert(ent && ent->client);

	if (ent->client->ps.isJediMaster)
	{//jedi masters have much more DP and don't actually have skills.
		ent->client->ps.stats[STAT_MAX_DODGE] = 100;
		return;
	}
	else if (ent->s.number < MAX_CLIENTS)
	{//players get a initial DP bonus.
		dodgeMax = 50;
	}

	//force powers
	for (i = 0; i < NUM_FORCE_POWERS; i++)
	{
		if (ent->client->ps.fd.forcePowerLevel[i])
		{//has points in this skill
			for (skillCount = FORCE_LEVEL_1; skillCount <= ent->client->ps.fd.forcePowerLevel[i]; skillCount++)
			{
				dodgeMax += bgForcePowerCost[i][skillCount] * SK_DP_FORFORCE;
			}
		}
	}

	//additional skills
	for (i = 0; i < NUM_SKILLS; i++)
	{
		if (ent->client->skillLevel[i])
		{//has points in this skill
			for (skillCount = FORCE_LEVEL_1; skillCount <= ent->client->skillLevel[i]; skillCount++)
			{
				if (i >= SK_BLUESTYLE && i <= SK_STAFFSTYLE)
				{//styles count as force powers
					dodgeMax += bgForcePowerCost[i + NUM_FORCE_POWERS][skillCount] * SK_DP_FORFORCE;
				}
				else
				{
					dodgeMax += bgForcePowerCost[i + NUM_FORCE_POWERS][skillCount] * SK_DP_FORMERC;
				}
			}
		}
	}

	ent->client->ps.stats[STAT_MAX_DODGE] = (int)dodgeMax;
}

void WP_InitForcePowers(gentity_t * ent)
{
	int i, i_r, lastFPKnown = -1;
	qboolean warnClient = qfalse, warnClientLimit = qfalse, didEvent = qfalse;
	char userinfo[MAX_INFO_STRING], forcePowers[DEFAULT_FORCEPOWERS_LEN + 1], readBuf[DEFAULT_FORCEPOWERS_LEN + 1];

	// if server has no max rank, default to max (50)
	if (g_maxForceRank.integer <= 0 || g_maxForceRank.integer >= NUM_FORCE_MASTERY_LEVELS) {
		// ack, prevent user from being dumb
		trap->Cvar_Set("g_maxForceRank", va("%i", FORCE_MASTERY_JEDI_MASTER));
		trap->Cvar_Update(&g_maxForceRank);
	}

	if (!ent || !ent->client)
		return;

	ent->client->ps.fd.saberAnimLevel = ent->client->sess.saberLevel;

	if (ent->client->ps.fd.saberAnimLevel < FORCE_LEVEL_1 || ent->client->ps.fd.saberAnimLevel > FORCE_LEVEL_3)
		ent->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;

	// so that the client configstring is already modified with this when we need it
	if (!speedLoopSound)
		speedLoopSound = G_SoundIndex("sound/weapons/force/speedloop.wav");
	if (!rageLoopSound)
		rageLoopSound = G_SoundIndex("sound/weapons/force/rageloop.wav");
	if (!absorbLoopSound)
		absorbLoopSound = G_SoundIndex("sound/weapons/force/absorbloop.wav");
	if (!protectLoopSound)
		protectLoopSound = G_SoundIndex("sound/weapons/force/protectloop.wav");
	if (!seeLoopSound)
		seeLoopSound = G_SoundIndex("sound/weapons/force/seeloop.wav");
	if (!ysalamiriLoopSound)
		ysalamiriLoopSound = G_SoundIndex("sound/player/nullifyloop.wav");

	if (ent->s.eType == ET_NPC)
		return;

	for (i = 0; i < NUM_FORCE_POWERS; i++) {
		ent->client->ps.fd.forcePowerLevel[i] = 0;
		ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
	}

	ent->client->ps.fd.forcePowerSelected = -1;
	ent->client->ps.fd.forceSide = 0;

	// if in siege, then use the powers for this class, and skip all this nonsense.
	if (level.gametype == GT_SIEGE && ent->client->siegeClass != -1) {
		for (i = 0; i < NUM_FORCE_POWERS; i++) {
			ent->client->ps.fd.forcePowerLevel[i] = bgSiegeClasses[ent->client->siegeClass].forcePowerLevels[i];
			if (!ent->client->ps.fd.forcePowerLevel[i])
				ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
			else
				ent->client->ps.fd.forcePowersKnown |= (1 << i);
		}

		// bring up the class selection menu
		if (!ent->client->sess.setForce)
			trap->SendServerCommand(ent - g_entities, "scl");
		ent->client->sess.setForce = qtrue;
		DetermineDodgeMax(ent);

		return;
	}

	//rwwFIXMEFIXME: Temp
	if (ent->s.eType == ET_NPC && ent->s.number >= MAX_CLIENTS)
		Q_strncpyz(userinfo, "forcepowers\\7-1-333003000313003120", sizeof(userinfo));
	else
		trap->GetUserinfo(ent->s.number, userinfo, sizeof(userinfo));

	Q_strncpyz(forcePowers, Info_ValueForKey(userinfo, "forcepowers"), sizeof(forcePowers));

	if (strlen(forcePowers) != DEFAULT_FORCEPOWERS_LEN) {
		Q_strncpyz(forcePowers, DEFAULT_FORCEPOWERS, sizeof(forcePowers));
		trap->SendServerCommand(ent - g_entities, "print \"" S_COLOR_RED "Invalid forcepowers string, setting default\n\"");
	}

	//if it's a bot just copy the info directly from its personality
	if ((ent->r.svFlags& SVF_BOT) && botstates[ent->s.number])
		Q_strncpyz(forcePowers, botstates[ent->s.number]->forceinfo, sizeof(forcePowers));

	if (g_forceBasedTeams.integer) {
		if (ent->client->sess.sessionTeam == TEAM_RED)
			warnClient = !(BG_LegalizedForcePowers(forcePowers, sizeof(forcePowers), g_maxForceRank.integer, HasSetSaberOnly(), FORCE_DARKSIDE, level.gametype, g_forcePowerDisable.integer));
		else if (ent->client->sess.sessionTeam == TEAM_BLUE)
			warnClient = !(BG_LegalizedForcePowers(forcePowers, sizeof(forcePowers), g_maxForceRank.integer, HasSetSaberOnly(), FORCE_LIGHTSIDE, level.gametype, g_forcePowerDisable.integer));
		else
			warnClient = !(BG_LegalizedForcePowers(forcePowers, sizeof(forcePowers), g_maxForceRank.integer, HasSetSaberOnly(), 0, level.gametype, g_forcePowerDisable.integer));
	}
	else
		warnClient = !(BG_LegalizedForcePowers(forcePowers, sizeof(forcePowers), g_maxForceRank.integer, HasSetSaberOnly(), 0, level.gametype, g_forcePowerDisable.integer));

	//rww - parse through the string manually and eat out all the appropriate data
	i = 0;
	i_r = 0;
	while (forcePowers[i] && forcePowers[i] != '-') {
		readBuf[i_r] = forcePowers[i];
		i_r++;
		i++;
	}
	readBuf[i_r] = 0;
	//THE RANK
	ent->client->ps.fd.forceRank = atoi(readBuf);
	i++;

	i_r = 0;
	while (forcePowers[i] && forcePowers[i] != '-') {
		readBuf[i_r] = forcePowers[i];
		i_r++;
		i++;
	}
	readBuf[i_r] = 0;
	//THE SIDE
	ent->client->ps.fd.forceSide = atoi(readBuf);
	i++;

	if (level.gametype != GT_SIEGE && (ent->r.svFlags & SVF_BOT) && ent->client->ps.weapon == WP_SABER && botstates[ent->s.number])
	{// hmm..I'm going to cheat here.

		int oldI = i;
		i_r = 0;

		while (forcePowers[i] && forcePowers[i] != '\n' && i_r < NUM_FORCE_POWERS)
		{
			if (ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
			{
				if (i_r == FP_ABSORB)
				{
					forcePowers[i] = '3';
				}
				if (botstates[ent->s.number]->settings.skill >= 4)
				{
					// cheat and give them more stuff
					if (i_r == FP_HEAL)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_PROTECT)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_TELEPATHY)
					{
						forcePowers[i] = '3';
					}
				}
			}
			else if (ent->client->ps.fd.forceSide == FORCE_DARKSIDE)
			{
				if (botstates[ent->s.number]->settings.skill >= 4)
				{
					if (i_r == FP_GRIP)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_LIGHTNING)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_RAGE)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_DRAIN)
					{
						forcePowers[i] = '3';
					}
				}
			}

			if (i_r == FP_PUSH)
			{
				forcePowers[i] = '3';
			}
			else if (i_r == FP_PULL)
			{
				forcePowers[i] = '3';
			}
			else if (i_r == FP_SABER_DEFENSE)
			{
				forcePowers[i] = '3';
			}
			else if (i_r == FP_SPEED)
			{
				forcePowers[i] = '1';
			}

			i++;
			i_r++;
		}
		i = oldI;
	}
	else if (level.gametype != GT_SIEGE && level.gametype == GT_TEAM && (ent->r.svFlags & SVF_BOT) && ent->client->ps.weapon == WP_SABER && botstates[ent->s.number])
	{// hmm..I'm going to cheat here.

		int oldI = i;
		i_r = 0;

		while (forcePowers[i] && forcePowers[i] != '\n' && i_r < NUM_FORCE_POWERS)
		{
			if (ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
			{
				if (i_r == FP_ABSORB)
				{
					forcePowers[i] = '3';
				}
				if (botstates[ent->s.number]->settings.skill >= 4)
				{
					// cheat and give them more stuff
					if (i_r == FP_HEAL)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_PROTECT)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_TELEPATHY)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_TEAM_HEAL)
					{
						forcePowers[i] = '3';
					}
				}
			}
			else if (ent->client->ps.fd.forceSide == FORCE_DARKSIDE)
			{
				if (botstates[ent->s.number]->settings.skill >= 4)
				{
					if (i_r == FP_GRIP)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_LIGHTNING)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_RAGE)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_DRAIN)
					{
						forcePowers[i] = '3';
					}
					else if (i_r == FP_TEAM_FORCE)
					{
						forcePowers[i] = '3';
					}
				}
			}

			if (i_r == FP_PUSH)
			{
				forcePowers[i] = '3';
			}
			else if (i_r == FP_PULL)
			{
				forcePowers[i] = '3';
			}
			else if (i_r == FP_SABER_DEFENSE)
			{
				forcePowers[i] = '3';
			}
			else if (i_r == FP_SPEED)
			{
				forcePowers[i] = '1';
			}

			i++;
			i_r++;
		}
		i = oldI;
	}

	i_r = 0;
	while (forcePowers[i] && forcePowers[i] != '\n' &&
		i_r < NUM_FORCE_POWERS)
	{
		readBuf[0] = forcePowers[i];
		readBuf[1] = 0;

		ent->client->ps.fd.forcePowerLevel[i_r] = atoi(readBuf);
		if (ent->client->ps.fd.forcePowerLevel[i_r])
		{
			ent->client->ps.fd.forcePowersKnown |= (1 << i_r);
		}
		else
		{
			ent->client->ps.fd.forcePowersKnown &= ~(1 << i_r);
		}
		i++;
		i_r++;
	}
	//THE POWERS

	if (ent->s.eType != ET_NPC) {
		if (HasSetSaberOnly()) {
			gentity_t* te = G_TempEntity(vec3_origin, EV_SET_FREE_SABER);
			te->r.svFlags |= SVF_BROADCAST;
			te->s.eventParm = 1;
		}
		else {
			gentity_t* te = G_TempEntity(vec3_origin, EV_SET_FREE_SABER);
			te->r.svFlags |= SVF_BROADCAST;
			te->s.eventParm = 0;
		}

		if (g_forcePowerDisable.integer) {
			gentity_t* te = G_TempEntity(vec3_origin, EV_SET_FORCE_DISABLE);
			te->r.svFlags |= SVF_BROADCAST;
			te->s.eventParm = 1;
		}
		else {
			gentity_t* te = G_TempEntity(vec3_origin, EV_SET_FORCE_DISABLE);
			te->r.svFlags |= SVF_BROADCAST;
			te->s.eventParm = 0;
		}
	}

	if (ent->s.eType == ET_NPC || ent->r.svFlags & SVF_BOT)
		ent->client->sess.setForce = qtrue;
	else if (level.gametype == GT_SIEGE) {
		if (!ent->client->sess.setForce) {
			ent->client->sess.setForce = qtrue;
			// bring up the class selection menu
			trap->SendServerCommand(ent - g_entities, "scl");
		}
	}
	else {
		if (warnClient || !ent->client->sess.setForce) {
			// the client's rank is too high for the server and has been autocapped, so tell them
			if (level.gametype != GT_HOLOCRON && level.gametype != GT_JEDIMASTER) {
				didEvent = qtrue;

				if (!(ent->r.svFlags& SVF_BOT) && ent->s.eType != ET_NPC) {
					if (!g_teamAutoJoin.integer) {
						// make them a spectator so they can set their powerups up without being bothered.
						ent->client->sess.sessionTeam = TEAM_SPECTATOR;
						ent->client->sess.spectatorState = SPECTATOR_FREE;
						ent->client->sess.spectatorClient = 0;

						ent->client->pers.teamState.state = TEAM_BEGIN;
						trap->SendServerCommand(ent - g_entities, "spc"); // Fire up the profile menu
					}
				}

				// event isn't very reliable, I made it a string. This way I can send it to just one client also,
				//	as opposed to making a broadcast event.
				trap->SendServerCommand(ent->s.number, va("nfr %i %i %i", g_maxForceRank.integer, 1, ent->client->sess.sessionTeam));
				// arg1 is new max rank, arg2 is non-0 if force menu should be shown, arg3 is the current team
			}
			ent->client->sess.setForce = qtrue;
		}

		if (!didEvent)
			trap->SendServerCommand(ent->s.number, va("nfr %i %i %i", g_maxForceRank.integer, 0, ent->client->sess.sessionTeam));

		// the server has one or more force powers disabled and the client is using them in his config
		if (warnClientLimit)
			trap->SendServerCommand(ent - g_entities, va("print \"The server has one or more force powers that you have chosen disabled.\nYou will not be able to use the disable force power(s) while playing on this server.\n\""));
	}

	for (i = 0; i < NUM_FORCE_POWERS; i++) {
		if ((ent->client->ps.fd.forcePowersKnown& (1 << i)) && !ent->client->ps.fd.forcePowerLevel[i])
			ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
		else if (i != FP_LEVITATION && i != FP_SABER_OFFENSE && i != FP_SABER_DEFENSE && i != FP_SABERTHROW)
			lastFPKnown = i;
	}

	if (ent->client->ps.fd.forcePowersKnown& ent->client->sess.selectedFP)
		ent->client->ps.fd.forcePowerSelected = ent->client->sess.selectedFP;

	if (!(ent->client->ps.fd.forcePowersKnown& (1 << ent->client->ps.fd.forcePowerSelected))) {
		if (lastFPKnown != -1)
			ent->client->ps.fd.forcePowerSelected = lastFPKnown;
		else
			ent->client->ps.fd.forcePowerSelected = 0;
	}

	for ( /*i=0*/; i < NUM_FORCE_POWERS; i++)
		ent->client->ps.fd.forcePowerBaseLevel[i] = ent->client->ps.fd.forcePowerLevel[i];
	ent->client->ps.fd.forceUsingAdded = 0;
	DetermineDodgeMax(ent);
}

void WP_SpawnInitForcePowers(gentity_t * ent)
{
	int i = 0;

	ent->client->ps.saberAttackChainCount = MISHAPLEVEL_NONE;

	ent->client->ps.BlasterAttackChainCount = BLASTERMISHAPLEVEL_NONE;

	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		if (ent->client->ps.fd.forcePowersActive& (1 << i))
		{
			WP_ForcePowerStop(ent, i);
		}

		i++;
	}

	ent->client->ps.fd.forceDeactivateAll = 0;

	ent->client->ps.fd.blockPoints = ent->client->ps.fd.blockPointsMax = BLOCK_POINTS_MAX;
	ent->client->ps.fd.forcePower = ent->client->ps.fd.forcePowerMax = FORCE_POWER_MAX;
	ent->client->ps.fd.forcePowerRegenDebounceTime = level.time;
	ent->client->ps.fd.forceGripEntityNum = ENTITYNUM_NONE;
	ent->client->ps.fd.forceMindtrickTargetIndex = 0;
	ent->client->ps.fd.forceMindtrickTargetIndex2 = 0;
	ent->client->ps.fd.forceMindtrickTargetIndex3 = 0;
	ent->client->ps.fd.forceMindtrickTargetIndex4 = 0;

	ent->client->ps.holocronBits = 0;

	i = 0;
	while (i < NUM_FORCE_POWERS)
	{
		ent->client->ps.holocronsCarried[i] = 0;
		i++;
	}

	if (level.gametype == GT_HOLOCRON)
	{
		i = 0;
		while (i < NUM_FORCE_POWERS)
		{
			ent->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_0;
			i++;
		}

		if (HasSetSaberOnly())
		{
			if (ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] < FORCE_LEVEL_1)
			{
				ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_1;
			}
			if (ent->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1)
			{
				ent->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_1;
			}
		}
	}

	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		ent->client->ps.fd.forcePowerDebounce[i] = 0;
		ent->client->ps.fd.forcePowerDuration[i] = 0;

		i++;
	}

	ent->client->ps.fd.forceJumpZStart = 0;
	ent->client->ps.fd.forceJumpCharge = 0;
	ent->client->ps.fd.forceJumpSound = 0;
	ent->client->ps.fd.forceGripDamageDebounceTime = 0;
	ent->client->ps.fd.forceGripBeingGripped = 0;
	ent->client->ps.fd.forceGripCripple = 0;
	ent->client->ps.fd.forceGripUseTime = 0;
	ent->client->ps.fd.forceGripSoundTime = 0;
	ent->client->ps.fd.forceGripStarted = 0;
	ent->client->ps.fd.forceHealTime = 0;
	ent->client->ps.fd.forceHealAmount = 0;
	ent->client->ps.fd.forceRageRecoveryTime = 0;
	ent->client->ps.fd.forceSpeedRecoveryTime = 0;
	ent->client->ps.fd.forceDrainEntNum = ENTITYNUM_NONE;
	ent->client->ps.fd.forceDrainTime = 0;

	i = 0;
	while (i < NUM_FORCE_POWERS)
	{
		if ((ent->client->ps.fd.forcePowersKnown& (1 << i)) &&
			!ent->client->ps.fd.forcePowerLevel[i])
		{ //make sure all known powers are cleared if we have level 0 in them
			ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
		}

		i++;
	}

	if (level.gametype == GT_SIEGE &&
		ent->client->siegeClass != -1)
	{ //Then use the powers for this class.
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			ent->client->ps.fd.forcePowerLevel[i] = bgSiegeClasses[ent->client->siegeClass].forcePowerLevels[i];

			if (!ent->client->ps.fd.forcePowerLevel[i])
			{
				ent->client->ps.fd.forcePowersKnown &= ~(1 << i);
			}
			else
			{
				ent->client->ps.fd.forcePowersKnown |= (1 << i);
			}
			i++;
		}
	}
}

qboolean IsMerc(gentity_t * ent)
{
	if (!ent->client)
	{
		return qfalse;
	}

	if (ent->client->skillLevel[SK_JETPACK])
	{
		return qtrue;
	}
	else if (ent->client->skillLevel[SK_PISTOL])
	{
		return qtrue;
	}
	else if (ent->client->skillLevel[SK_BLASTER])
	{
		return qtrue;
	}
	else if (ent->client->skillLevel[SK_THERMAL])
	{
		return qtrue;
	}
	else if (ent->client->skillLevel[SK_ROCKET])
	{
		return qtrue;
	}
	else if (ent->client->skillLevel[SK_BACTA])
	{
		return qtrue;
	}
	else if (ent->client->skillLevel[SK_FLAMETHROWER])
	{
		return qtrue;
	}
	else if (ent->client->skillLevel[SK_BOWCASTER])
	{
		return qtrue;
	}
	else if (ent->client->skillLevel[SK_FORCEFIELD])
	{
		return qtrue;
	}
	else if (ent->client->skillLevel[SK_CLOAK])
	{
		return qtrue;
	}
	else if (ent->client->skillLevel[SK_SEEKER])
	{
		return qtrue;
	}
	else if (ent->client->skillLevel[SK_SENTRY])
	{
		return qtrue;
	}
	else if (ent->client->skillLevel[SK_DETPACK])
	{
		return qtrue;
	}
	else if (ent->client->skillLevel[SK_REPEATER])
	{
		return qtrue;
	}
	else if (ent->client->skillLevel[SK_DISRUPTOR])
	{
		return qtrue;
	}

	return qfalse;
}

extern qboolean BG_InKnockDown(int anim); //bg_pmove.c

int ForcePowerUsableOn(gentity_t * attacker, gentity_t * other, forcePowers_t forcePower)
{
	if (other->client && (other->client->ps.inAirAnim || other->client->ps.groundEntityNum == ENTITYNUM_NONE))
	{
		return 1;
	}

	if (other && other->client && BG_HasYsalamiri(level.gametype, &other->client->ps))
	{
		return 0;
	}

	if (attacker && attacker->client && !BG_CanUseFPNow(level.gametype, &attacker->client->ps, level.time, forcePower))
	{
		return 0;
	}

	if (IsMerc(other) && other->client->ps.fd.forcePowerLevel[FP_SEE] >= FORCE_LEVEL_1)
	{
		return 1;
	}

	//Dueling fighters cannot use force powers on others, with the exception of force push when locked with each other
	if (attacker && attacker->client && attacker->client->ps.duelInProgress)
	{
		return 0;
	}

	if (other && other->client && other->client->ps.duelInProgress)
	{
		return 0;
	}

	if (other->client && other->client->ps.fd.saberAnimLevel == SS_DESANN && other->client->ps.saberAttackChainCount >= MISHAPLEVEL_HEAVY)
	{
		return 0;
	}

	if (forcePower == FP_TELEPATHY && other->client)
	{
		switch (other->client->ps.fd.forcePowerLevel[FP_ABSORB])
		{
		case FORCE_LEVEL_5:
		case FORCE_LEVEL_4:
		case FORCE_LEVEL_3:
			return 0;
			break;
		case FORCE_LEVEL_2:
			if (!WalkCheck(other) && PM_RunningAnim(other->client->ps.legsAnim))
				return 0;
			break;

		case FORCE_LEVEL_1:
			if (!WalkCheck(other))
				return 0;
			break;
		}
	}
	else if (forcePower == FP_GRIP && other->client)
	{
		switch (other->client->ps.fd.forcePowerLevel[FP_ABSORB])
		{
		case FORCE_LEVEL_1://Can only block if walking
			if (!WalkCheck(other))
				return 1;
			break;
		case FORCE_LEVEL_2://Can block if walking or running
			if (other->client->ps.inAirAnim || other->client->ps.groundEntityNum == ENTITYNUM_NONE)
				return 1;
			break;

		case FORCE_LEVEL_3:
		case FORCE_LEVEL_5:
		case FORCE_LEVEL_4:
			return 0;

		default:
			return 1;
		}
	}

	if (other && other->client && forcePower == FP_PUSH)
	{
		if (other->client->ps.stats[STAT_HEALTH] <= 0 || (other->client->ps.eFlags & EF_DEAD))
		{
			return 0;
		}
	}
	else if (other && other->client && forcePower == FP_PULL)
	{
		if (g_AllowKnockDownPull.integer == 0)
		{
			if (BG_InKnockDown(other->client->ps.legsAnim))
			{
				return 0;
			}
		}
		if (other->client->ps.stats[STAT_HEALTH] <= 0 || (other->client->ps.eFlags & EF_DEAD))
		{
			return 0;
		}
	}

	if (other && other->client && other->s.eType == ET_NPC &&
		other->s.NPC_class == CLASS_VEHICLE)
	{ //can't use the force on vehicles.. except lightning
		if (forcePower == FP_LIGHTNING)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	if (other&& other->client&& other->s.eType == ET_NPC &&
		level.gametype == GT_SIEGE)
	{ //can't use powers at all on npc's normally in siege...
		return 0;
	}

	return 1;
}

qboolean WP_ForcePowerAvailable(gentity_t * self, forcePowers_t forcePower, int overrideAmt)
{
	int	drain = overrideAmt ? overrideAmt :
		forcePowerNeeded[self->client->ps.fd.forcePowerLevel[forcePower]][forcePower];

	if (self->client->ps.fd.forcePowersActive & (1 << forcePower))
	{ //we're probably going to deactivate it..
		return qtrue;
	}
	if (forcePower == FP_LEVITATION)
	{
		return qtrue;
	}
	if (!drain)
	{
		return qtrue;
	}
	if ((forcePower == FP_DRAIN || forcePower == FP_LIGHTNING) &&
		self->client->ps.fd.forcePower >= 25)
	{ //it's ok then, drain/lightning are actually duration
		return qtrue;
	}
	if (self->client->ps.fd.forcePower < drain)
	{
		return qfalse;
	}
	return qtrue;
}

qboolean WP_ForcePowerInUse(gentity_t * self, forcePowers_t forcePower)
{
	if ((self->client->ps.fd.forcePowersActive& (1 << forcePower)))
	{//already using this power
		return qtrue;
	}

	return qfalse;
}

qboolean Class_IsGunner(gentity_t * self)
{
	switch (self->client->ps.weapon)
	{
	case WP_STUN_BATON:
	case WP_BRYAR_PISTOL:
	case WP_BLASTER:
	case WP_DISRUPTOR:
	case WP_BOWCASTER:
	case WP_REPEATER:
	case WP_DEMP2:
	case WP_FLECHETTE:
	case WP_ROCKET_LAUNCHER:
	case WP_THERMAL:
	case WP_TRIP_MINE:
	case WP_DET_PACK:
	case WP_CONCUSSION:
	case WP_BRYAR_OLD:
	case WP_EMPLACED_GUN:
	case WP_TURRET:
		// Is Gunner...
		return qtrue;
		break;
	default:
		// NOT Gunner...
		break;
	}

	return qfalse;
}

qboolean WP_ForcePowerUsable(gentity_t * self, forcePowers_t forcePower)
{
	if (BG_HasYsalamiri(level.gametype, &self->client->ps))
	{
		return qfalse;
	}

	if (self->health <= 0 || self->client->ps.stats[STAT_HEALTH] <= 0 ||
		(self->client->ps.eFlags & EF_DEAD))
	{
		return qfalse;
	}

	if (self->client->ps.pm_flags & PMF_FOLLOW)
	{ //specs can't use powers through people
		return qfalse;
	}
	if (self->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return qfalse;
	}
	if (self->client->tempSpectate >= level.time)
	{
		return qfalse;
	}

	if (!BG_CanUseFPNow(level.gametype, &self->client->ps, level.time, forcePower))
	{
		return qfalse;
	}

	if (!(self->client->ps.fd.forcePowersKnown & (1 << forcePower)))
	{//don't know this power
		return qfalse;
	}

	if ((self->client->ps.fd.forcePowersActive & (1 << forcePower)))
	{//already using this power
		if (forcePower != FP_LEVITATION)
		{
			return qfalse;
		}
	}

	if (forcePower == FP_LEVITATION && self->client->fjDidJump)
	{
		return qfalse;
	}

	if (!self->client->ps.fd.forcePowerLevel[forcePower])
	{
		return qfalse;
	}

	if ((self->client->ps.pm_flags & PMF_STUCK_TO_WALL))
	{//no offensive force powers when stuck to wall
		switch (forcePower)
		{
		case FP_GRIP:
		case FP_LIGHTNING:
		case FP_DRAIN:
		case FP_SABER_OFFENSE:
		case FP_SABER_DEFENSE:
		case FP_SABERTHROW:
			return qfalse;
			break;
		default:
			break;
		}
	}

	if (!self->client->ps.saberHolstered)
	{
		if ((self->client->saber[0].saberFlags & SFL_TWO_HANDED))
		{
			if (g_saberRestrictForce.integer)
			{
				switch (forcePower)
				{
				case FP_PUSH:
				case FP_PULL:
				case FP_TELEPATHY:
				case FP_GRIP:
				case FP_LIGHTNING:
				case FP_DRAIN:
					return qfalse;
					break;
				default:
					break;
				}
			}
		}

		if ((self->client->saber[0].saberFlags & SFL_TWO_HANDED)
			|| self->client->saber[0].model[0])
		{//this saber requires the use of two hands OR our other hand is using an active saber too
			if ((self->client->saber[0].forceRestrictions & (1 << forcePower)))
			{//this power is verboten when using this saber
				return qfalse;
			}
		}

		if (self->client->saber[0].model[0])
		{//both sabers on
			if (g_saberRestrictForce.integer)
			{
				switch (forcePower)
				{
				case FP_PUSH:
				case FP_PULL:
				case FP_TELEPATHY:
				case FP_GRIP:
				case FP_LIGHTNING:
				case FP_DRAIN:
					return qfalse;
					break;
				default:
					break;
				}
			}
			if ((self->client->saber[1].forceRestrictions& (1 << forcePower)))
			{//this power is verboten when using this saber
				return qfalse;
			}
		}
	}
	return WP_ForcePowerAvailable(self, forcePower, 0);	// OVERRIDEFIXME
}

int WP_AbsorbConversion(gentity_t * attacked, int atdAbsLevel, gentity_t * attacker, int atPower, int atPowerLevel, int atForceSpent)
{
	int getLevel = 0;
	int addTot = 0;
	gentity_t* abSound;

	if (atPower != FP_DRAIN &&
		atPower != FP_GRIP)
	{ //Only these powers can be absorbed
		return -1;
	}

	if (!atdAbsLevel)
	{ //looks like attacker doesn't have any absorb power
		return -1;
	}

	if (!(attacked->client->ps.fd.forcePowersActive& (1 << FP_ABSORB)))
	{ //absorb is not active
		return -1;
	}

	//Subtract absorb power level from the offensive force power
	getLevel = atPowerLevel;
	getLevel -= atdAbsLevel;

	if (getLevel < 0)
	{
		getLevel = 0;
	}

	//let the attacker absorb an amount of force used in this attack based on his level of absorb
	addTot = (atForceSpent / 3) * attacked->client->ps.fd.forcePowerLevel[FP_ABSORB];

	if (addTot < 1 && atForceSpent >= 1)
	{
		addTot = 1;
	}
	attacked->client->ps.fd.forcePower += addTot;
	if (attacked->client->ps.fd.forcePower > attacked->client->ps.fd.forcePowerMax)
	{
		attacked->client->ps.fd.forcePower = attacked->client->ps.fd.forcePowerMax;
	}

	//play sound indicating that attack was absorbed
	if (attacked->client->forcePowerSoundDebounce < level.time)
	{
		abSound = G_PreDefSound(attacked->client->ps.origin, PDSOUND_ABSORBHIT);
		abSound->s.trickedentindex = attacked->s.number;

		attacked->client->forcePowerSoundDebounce = level.time + 400;
	}

	return getLevel;
}

void WP_ForcePowerRegenerate(gentity_t * self, int overrideAmt)
{ //called on a regular interval to regenerate force power.
	if (!self->client)
	{
		return;
	}

	if (overrideAmt)
	{ //custom regen amount
		self->client->ps.fd.forcePower += overrideAmt;
	}
	else
	{ //otherwise, just 1
		self->client->ps.fd.forcePower++;
	}

	if (self->client->ps.fd.forcePower > self->client->ps.fd.forcePowerMax)
	{ //cap it off at the max (default 100)
		self->client->ps.fd.forcePower = self->client->ps.fd.forcePowerMax;
	}
}

void WP_ForcePowerStart(gentity_t * self, forcePowers_t forcePower, int overrideAmt)
{ //activate the given force power
	int	duration = 0;
	qboolean hearable = qfalse;
	float hearDist = 0;

	if (!WP_ForcePowerAvailable(self, forcePower, overrideAmt))
	{
		return;
	}

	if (BG_FullBodyTauntAnim(self->client->ps.legsAnim))
	{//stop taunt
		self->client->ps.legsTimer = 0;
	}
	if (BG_FullBodyTauntAnim(self->client->ps.torsoAnim))
	{//stop taunt
		self->client->ps.torsoTimer = 0;
	}
	//hearable and hearDist are merely for the benefit of bots, and not related to if a sound is actually played.
	//If duration is set, the force power will assume to be timer-based.
	switch ((int)forcePower)
	{
	case FP_HEAL:
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= (1 << forcePower);
		break;
	case FP_LEVITATION:
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= (1 << forcePower);
		break;
	case FP_SPEED:
		hearable = qtrue;
		hearDist = 256;
		if (self->client->ps.fd.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_1)
		{
			duration = FORCE_SPEED_DURATION_FORCE_LEVEL_1;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_2)
		{
			if (IsPressingDashButton(self))
			{
				duration = FORCE_SPEED_DURATION_FORCE_LEVEL_1;
			}
			else
			{
				duration = FORCE_SPEED_DURATION_FORCE_LEVEL_2;
			}
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_3)
		{
			if (IsPressingDashButton(self))
			{
				duration = FORCE_SPEED_DURATION_FORCE_LEVEL_1;
			}
			else
			{
				duration = FORCE_SPEED_DURATION_FORCE_LEVEL_3;
			}
		}
		else //shouldn't get here
		{
			break;
		}

		if (overrideAmt)
		{
			duration = overrideAmt;
		}

		self->client->ps.fd.forcePowersActive |= (1 << forcePower);
		break;
	case FP_PUSH:
		hearable = qtrue;
		hearDist = 256;
		break;
	case FP_PULL:
		hearable = qtrue;
		hearDist = 256;
		break;
	case FP_TELEPATHY:
		hearable = qtrue;
		hearDist = 256;
		if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_1)
		{
			duration = 20000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_2)
		{
			duration = 25000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_3)
		{
			duration = 30000;
		}
		else //shouldn't get here
		{
			break;
		}

		self->client->ps.fd.forcePowersActive |= (1 << forcePower);
		break;
	case FP_GRIP:
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= (1 << forcePower);
		self->client->ps.powerups[PW_DISINT_4] = level.time + 60000;
		break;
	case FP_LIGHTNING:
		hearable = qtrue;
		hearDist = 512;
		duration = overrideAmt;
		overrideAmt = 0;
		self->client->ps.fd.forcePowersActive |= (1 << forcePower);
		self->client->ps.activeForcePass = self->client->ps.fd.forcePowerLevel[FP_LIGHTNING];
		break;
	case FP_RAGE:
		hearable = qtrue;
		hearDist = 256;
		if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_1)
		{
			duration = 8000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_2)
		{
			duration = 14000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_3)
		{
			duration = 20000;
		}
		else //shouldn't get here
		{
			break;
		}

		self->client->ps.fd.forcePowersActive |= (1 << forcePower);
		break;
	case FP_PROTECT:
		hearable = qtrue;
		hearDist = 256;
		duration = 20000;
		self->client->ps.fd.forcePowersActive |= (1 << forcePower);
		break;
	case FP_ABSORB:
		hearable = qtrue;
		hearDist = 256;
		duration = 20000;
		self->client->ps.fd.forcePowersActive |= (1 << forcePower);
		break;
	case FP_TEAM_HEAL:
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= (1 << forcePower);
		break;
	case FP_TEAM_FORCE:
		hearable = qtrue;
		hearDist = 256;
		self->client->ps.fd.forcePowersActive |= (1 << forcePower);
		break;
	case FP_DRAIN:
		hearable = qtrue;
		hearDist = 256;
		duration = overrideAmt;
		overrideAmt = 0;
		self->client->ps.fd.forcePowersActive |= (1 << forcePower);
		break;
	case FP_SEE:
		hearable = qtrue;
		hearDist = 256;
		if (self->client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_1)
		{
			duration = 10000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_2)
		{
			duration = 20000;
		}
		else if (self->client->ps.fd.forcePowerLevel[FP_SEE] == FORCE_LEVEL_3)
		{
			duration = 30000;
		}
		else //shouldn't get here
		{
			break;
		}

		self->client->ps.fd.forcePowersActive |= (1 << forcePower);
		break;
	case FP_SABER_OFFENSE:
		break;
	case FP_SABER_DEFENSE:
		break;
	case FP_SABERTHROW:
		break;
	default:
		break;
	}

	if (duration)
	{
		self->client->ps.fd.forcePowerDuration[forcePower] = level.time + duration;
	}
	else
	{
		self->client->ps.fd.forcePowerDuration[forcePower] = 0;
	}

	if (hearable)
	{
		self->client->ps.otherSoundLen = hearDist;
		self->client->ps.otherSoundTime = level.time + 100;
	}

	self->client->ps.fd.forcePowerDebounce[forcePower] = 0;

	if ((int)forcePower == FP_SPEED && overrideAmt)
	{
		WP_ForcePowerDrain(&self->client->ps, forcePower, overrideAmt * 0.025);
	}
	else if ((int)forcePower != FP_GRIP && (int)forcePower != FP_DRAIN)
	{ //grip and drain drain as damage is done
		WP_ForcePowerDrain(&self->client->ps, forcePower, overrideAmt);
	}
}

void ForceHeal(gentity_t * self)
{
	if (self->health <= 0 || self->client->ps.stats[STAT_MAX_HEALTH] <= self->health)
	{
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_HEAL))
	{
		return;
	}

	if (self->painDebounceTime > level.time || (self->client->ps.weaponTime && self->client->ps.weapon != WP_NONE))
	{//can't initiate a heal while taking pain or attacking
		return;
	}

	if (self->client->ps.saberLockTime > level.time)
	{//FIXME: can this be a way to break out?
		return;
	}

	if (self->health >= self->client->ps.stats[STAT_MAX_HEALTH])
	{
		// Shield Heal skill. Done when player has full HP
		if (self->client->ps.stats[STAT_ARMOR] < 100)
		{
			self->client->ps.stats[STAT_ARMOR] += 25;

			if (self->client->ps.stats[STAT_ARMOR] > 100)
			{
				self->client->ps.stats[STAT_ARMOR] = 100;
			}


			WP_ForcePowerDrain(&self->client->ps, FP_HEAL, 0);

			G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/player/pickupshield.wav"));
		}
		return;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_3)
	{
		self->health += 25; //This was 50, but that angered the Balance God.

		if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
		{
			self->health = self->client->ps.stats[STAT_MAX_HEALTH];
		}
		WP_ForcePowerDrain(&self->client->ps, FP_HEAL, 0);
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_2)
	{
		self->health += 10;

		if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
		{
			self->health = self->client->ps.stats[STAT_MAX_HEALTH];
		}
		WP_ForcePowerDrain(&self->client->ps, FP_HEAL, 0);
	}
	else
	{
		self->health += 5;

		if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
		{
			self->health = self->client->ps.stats[STAT_MAX_HEALTH];
		}
		WP_ForcePowerDrain(&self->client->ps, FP_HEAL, 0);
	}

	if (self->client->ps.fd.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_2)
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_FORCEHEAL_START, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
		self->client->ps.saberBlocked = BLOCKED_NONE;
	}
	else
	{//just a quick gesture
		NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCEHEAL_QUICK, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
		self->client->ps.saberBlocked = BLOCKED_NONE;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_2)
	{

		G_Sound(self, CHAN_ITEM, G_SoundIndex("sound/weapons/force/heal.wav"));
	}
	else
	{
		G_Sound(self, CHAN_ITEM, G_SoundIndex("sound/player/injecthealth.mp3"));
	}

	G_PlayBoltedEffect(G_EffectIndex("force/heal2.efx"), self, "thoracic");
}

void WP_AddToClientBitflags(gentity_t * ent, int entNum)
{
	if (!ent)
	{
		return;
	}

	if (entNum > 47)
	{
		ent->s.trickedentindex4 |= (1 << (entNum - 48));
	}
	else if (entNum > 31)
	{
		ent->s.trickedentindex3 |= (1 << (entNum - 32));
	}
	else if (entNum > 15)
	{
		ent->s.trickedentindex2 |= (1 << (entNum - 16));
	}
	else
	{
		ent->s.trickedentindex |= (1 << entNum);
	}
}

void ForceTeamHeal(gentity_t * self)
{
	float radius = 256;
	int i = 0;
	gentity_t* ent;
	vec3_t a;
	int numpl = 0;
	int pl[MAX_CLIENTS];
	int healthadd = 0;
	gentity_t* te = NULL;

	if (self->health <= 0)
	{
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_TEAM_HEAL))
	{
		return;
	}

	if (self->client->ps.fd.forcePowerDebounce[FP_TEAM_HEAL] >= level.time)
	{
		return;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_2)
	{
		radius *= 1.5;
	}
	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL] == FORCE_LEVEL_3)
	{
		radius *= 2;
	}

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent&& ent->client&& self != ent && OnSameTeam(self, ent) && ent->client->ps.stats[STAT_HEALTH] < ent->client->ps.stats[STAT_MAX_HEALTH] && ent->client->ps.stats[STAT_HEALTH] > 0 && ForcePowerUsableOn(self, ent, FP_TEAM_HEAL) &&
			trap->InPVS(self->client->ps.origin, ent->client->ps.origin))
		{
			VectorSubtract(self->client->ps.origin, ent->client->ps.origin, a);

			if (VectorLength(a) <= radius)
			{
				pl[numpl] = i;
				numpl++;
			}
		}

		i++;
	}

	if (numpl < 1)
	{
		return;
	}

	if (numpl == 1)
	{
		healthadd = 50;
	}
	else if (numpl == 2)
	{
		healthadd = 33;
	}
	else
	{
		healthadd = 25;
	}

	self->client->ps.fd.forcePowerDebounce[FP_TEAM_HEAL] = level.time + 2000;
	i = 0;

	while (i < numpl)
	{
		if (g_entities[pl[i]].client->ps.stats[STAT_HEALTH] > 0 &&
			g_entities[pl[i]].health > 0)
		{
			g_entities[pl[i]].client->ps.stats[STAT_HEALTH] += healthadd;
			if (g_entities[pl[i]].client->ps.stats[STAT_HEALTH] > g_entities[pl[i]].client->ps.stats[STAT_MAX_HEALTH])
			{
				g_entities[pl[i]].client->ps.stats[STAT_HEALTH] = g_entities[pl[i]].client->ps.stats[STAT_MAX_HEALTH];
			}

			g_entities[pl[i]].health = g_entities[pl[i]].client->ps.stats[STAT_HEALTH];

			//At this point we know we got one, so add him into the collective event client bitflag
			if (!te)
			{
				te = G_TempEntity(self->client->ps.origin, EV_TEAM_POWER);
				te->s.eventParm = 1; //eventParm 1 is heal, eventParm 2 is force regen

				//since we had an extra check above, do the drain now because we got at least one guy
				WP_ForcePowerDrain(&self->client->ps, FP_TEAM_HEAL, forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL]);
			}

			WP_AddToClientBitflags(te, pl[i]);
			//Now cramming it all into one event.. doing this many g_sound events at once was a Bad Thing.
		}
		i++;
	}
}

void ForceTeamForceReplenish(gentity_t * self)
{
	float radius = 256;
	int i = 0;
	gentity_t* ent;
	vec3_t a;
	int numpl = 0;
	int pl[MAX_CLIENTS];
	int poweradd = 0;
	gentity_t* te = NULL;

	if (self->health <= 0)
	{
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_TEAM_FORCE))
	{
		return;
	}

	if (self->client->ps.fd.forcePowerDebounce[FP_TEAM_FORCE] >= level.time)
	{
		return;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_2)
	{
		radius *= 1.5;
	}
	if (self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE] == FORCE_LEVEL_3)
	{
		radius *= 2;
	}

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent&& ent->client&& self != ent && OnSameTeam(self, ent) && ent->client->ps.fd.forcePower < 100 && ForcePowerUsableOn(self, ent, FP_TEAM_FORCE) &&
			trap->InPVS(self->client->ps.origin, ent->client->ps.origin))
		{
			VectorSubtract(self->client->ps.origin, ent->client->ps.origin, a);

			if (VectorLength(a) <= radius)
			{
				pl[numpl] = i;
				numpl++;
			}
		}

		i++;
	}

	if (numpl < 1)
	{
		return;
	}

	if (numpl == 1)
	{
		poweradd = 50;
	}
	else if (numpl == 2)
	{
		poweradd = 33;
	}
	else
	{
		poweradd = 25;
	}
	self->client->ps.fd.forcePowerDebounce[FP_TEAM_FORCE] = level.time + 2000;

	WP_ForcePowerDrain(&self->client->ps, FP_TEAM_FORCE, forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE]);

	i = 0;

	while (i < numpl)
	{
		g_entities[pl[i]].client->ps.fd.forcePower += poweradd;
		if (g_entities[pl[i]].client->ps.fd.forcePower > g_entities[pl[i]].client->ps.fd.forcePowerMax)
		{
			g_entities[pl[i]].client->ps.fd.forcePower = g_entities[pl[i]].client->ps.fd.forcePowerMax;
		}

		//At this point we know we got one, so add him into the collective event client bitflag
		if (!te)
		{
			te = G_TempEntity(self->client->ps.origin, EV_TEAM_POWER);
			te->s.eventParm = 2; //eventParm 1 is heal, eventParm 2 is force regen
		}

		WP_AddToClientBitflags(te, pl[i]);
		//Now cramming it all into one event.. doing this many g_sound events at once was a Bad Thing.

		i++;
	}
}


qboolean IsHybrid(gentity_t * ent)
{
	qboolean jedi = qfalse, merc = qfalse;

	if (ent->client->ps.fd.forcePowersKnown& (1 << FP_SEE))
	{
		jedi = qtrue;
	}
		

	if (ent->client->skillLevel[SK_JETPACK])
	{
		merc = qtrue;
	}		
	else if (ent->client->skillLevel[SK_PISTOL])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_BLASTER])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_THERMAL])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_ROCKET])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_BACTA])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_FLAMETHROWER])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_BOWCASTER])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_FORCEFIELD])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_CLOAK])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_SEEKER])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_SENTRY])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_DETPACK])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_REPEATER])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_DISRUPTOR])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_ACROBATICS])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_TUSKEN_RIFLE])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_GRENADE])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_SMOKEGRENADE])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_FLASHGRENADE])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_CRYOBAN])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_EMP])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_VEHICLE])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_EOPIE])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_TAUNTAUN])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_SWOOP])
	{
		merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_DROIDEKA])
	{
    	merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_RANCOR])
	{
	    merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_CONCUSSION])
	{
	    merc = qtrue;
	}
	else if (ent->client->skillLevel[SK_ELECTROPULSE])
	{
	    merc = qtrue;
	}

	if (jedi && merc)
	{
		return qtrue;
	}		

	return qfalse;
}

qboolean WP_CounterForce(gentity_t * attacker, gentity_t * defender, int attackPower)
{//generically checks to see if the defender is able to block an attack from this attacker
	int abilityDef;		//the difference in skill between the defender's defend power and the attacker's attack power.	
	float blockFactor = 0;

	if (!(defender->client->buttons & BUTTON_BLOCK))
	{
		if (defender->r.svFlags & SVF_BOT || defender->s.eType == ET_NPC)
		{//bots just randomly dodge to make up for them not intelligently dodgeing
			blockFactor = 0.5f;
		}
		else
		{
			return qfalse;
		}
	}

	if (BG_IsUsingHeavyWeap(&defender->client->ps))
	{//can't block force powers while using heavy weapons
		return qfalse;
	}

	if (!(defender->client->ps.fd.forcePowersKnown& (1 << attackPower))	&& !(defender->client->ps.fd.forcePowersKnown& (1 << FP_ABSORB)))
	{//doesn't have absorb or same power as the attack power.
		return qfalse;
	}

	//determine ability difference
	abilityDef = attacker->client->ps.fd.forcePowerLevel[attackPower] - defender->client->ps.fd.forcePowerLevel[attackPower];

	if (abilityDef > attacker->client->ps.fd.forcePowerLevel[attackPower] - defender->client->ps.fd.forcePowerLevel[FP_ABSORB])
	{//defender's absorb ability is stronger than their attackPower ability, use that instead.
		abilityDef = attacker->client->ps.fd.forcePowerLevel[attackPower] - defender->client->ps.fd.forcePowerLevel[FP_ABSORB];
	}

	if (abilityDef >= 2)
	{//defender is largely weaker than the attacker (2 levels)
		if (!WalkCheck(defender) || defender->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{//can't block much stronger Force power while running or in mid-air
			return qfalse;
		}
	}
	else if (abilityDef >= 1)
	{//defender is slightly weaker than their attacker
		if (defender->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			return qfalse;
		}
	}

	if (PM_SaberInBrokenParry(defender->client->ps.saberMove))
	{//can't block while stunned
		return qfalse;
	}

	if (BG_InSlowBounce(&defender->client->ps) && defender->client->ps.userInt3 & (1 << FLAG_OLDSLOWBOUNCE))
	{//can't block lightning while in the heavier slow bounces.
		return qfalse;
	}

	if (defender->client->ps.fd.blockPoints <= BLOCKPOINTS_HALF)
	{//can't block if we're too off balance.
		return qfalse;
	}

	if (defender->client->ps.saberAttackChainCount >= MISHAPLEVEL_HEAVY)
	{//can't block if we're too off balance.
		return qfalse;
	}
	if (defender->client->ps.saberAttackChainCount >= MISHAPLEVEL_LIGHT
		&& attacker->client->ps.fd.saberAnimLevel == SS_DESANN)
	{//can't block if we're too off balance and they are using Juyo's perk
		return qfalse;
	}
	if (defender->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{//can block force while using forceHandExtend. This may be a temporary bug fix.

		return qtrue;
	}

	if (IsHybrid(defender))
	{
		defender->client->ps.userInt3 |= (1 << FLAG_BLOCKING);
		defender->client->blockTime = level.time + 1000;
	}
	return qtrue;
}

void ForceGrip(gentity_t * self)
{
	trace_t tr;
	vec3_t tfrom, tto, fwd;

	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	if (self->client->ps.userInt3 & (1 << FLAG_PREBLOCK))
	{
		return;
	}

	//allow during preblocks
	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saberMove) || !(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK))))
	{
		return;
	}

	if (self->client->ps.fd.forceGripUseTime > level.time)
	{
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_GRIP))
	{
		return;
	}

	VectorCopy(self->client->ps.origin, tfrom);
	tfrom[2] += self->client->ps.viewheight;
	AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
	tto[0] = tfrom[0] + fwd[0] * MAX_GRIP_DISTANCE;
	tto[1] = tfrom[1] + fwd[1] * MAX_GRIP_DISTANCE;
	tto[2] = tfrom[2] + fwd[2] * MAX_GRIP_DISTANCE;

	trap->Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction != 1.0 &&
		tr.entityNum != ENTITYNUM_NONE &&
		g_entities[tr.entityNum].client &&
		!g_entities[tr.entityNum].client->ps.fd.forceGripCripple &&
		g_entities[tr.entityNum].client->ps.fd.forceGripBeingGripped < level.time &&
		ForcePowerUsableOn(self, &g_entities[tr.entityNum], FP_GRIP) &&
		!WP_CounterForce(self, &g_entities[tr.entityNum], FP_GRIP) &&
		(g_friendlyFire.integer || !OnSameTeam(self, &g_entities[tr.entityNum]))) //don't grip someone who's still crippled
	{
		if (g_entities[tr.entityNum].s.number < MAX_CLIENTS&& g_entities[tr.entityNum].client->ps.m_iVehicleNum)
		{ //a player on a vehicle
			gentity_t* vehEnt = &g_entities[g_entities[tr.entityNum].client->ps.m_iVehicleNum];
			if (vehEnt->inuse&& vehEnt->client&& vehEnt->m_pVehicle)
			{
				if (vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER ||
					vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
				{ //push the guy off
					vehEnt->m_pVehicle->m_pVehicleInfo->Eject(vehEnt->m_pVehicle, (bgEntity_t*)& g_entities[tr.entityNum], qfalse);
				}
			}
		}
		self->client->ps.fd.forceGripEntityNum = tr.entityNum;
		g_entities[tr.entityNum].client->ps.fd.forceGripStarted = level.time;
		self->client->ps.fd.forceGripDamageDebounceTime = 0;

		self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
		self->client->ps.forceHandExtendTime = level.time + 5000;
	}
	else
	{
		self->client->ps.fd.forceGripEntityNum = ENTITYNUM_NONE;
		return;
	}
}

int IsPressingDashButton(gentity_t * self)
{
	if (self->client->buttons& BUTTON_DASH)
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
	return qfalse;
}

void WP_DebounceForceDeactivateTime(gentity_t * self)
{
	if (self && self->client)
	{
		if (self->client->ps.fd.forcePowersActive & (1 << FP_SPEED)
			|| self->client->ps.fd.forcePowersActive & (1 << FP_PROTECT)
			|| self->client->ps.fd.forcePowersActive & (1 << FP_ABSORB)
			|| self->client->ps.fd.forcePowersActive & (1 << FP_RAGE)
			|| self->client->ps.fd.forcePowersActive & (1 << FP_SEE))
		{//already running another power that can be manually, stopped don't debounce so long
			self->client->ps.forceAllowDeactivateTime = level.time + 500;
		}
		else
		{//not running one of the interruptable powers
			self->client->ps.forceAllowDeactivateTime = level.time + 1500;
		}
	}
}

void WP_DebounceForceDashDeactivateTime(gentity_t * self)
{
	if (self && self->client)
	{
		if (self->client->ps.fd.forcePowersActive & (1 << FP_SPEED)
			|| self->client->ps.fd.forcePowersActive & (1 << FP_PROTECT)
			|| self->client->ps.fd.forcePowersActive & (1 << FP_ABSORB)
			|| self->client->ps.fd.forcePowersActive & (1 << FP_RAGE)
			|| self->client->ps.fd.forcePowersActive & (1 << FP_SEE))
		{//already running another power that can be manually, stopped don't debounce so long
			self->client->ps.forceAllowDeactivateTime = 100;
		}
		else
		{//not running one of the interruptable powers
			self->client->ps.forceAllowDeactivateTime = 100;
		}
	}
}

void ForceSpeedAnim(gentity_t * self)
{
	if (self->client->pers.cmd.rightmove > 0)
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_HOP_R, AFLAG_PACE);
	}
	else if (self->client->pers.cmd.rightmove < 0)
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_HOP_L, AFLAG_PACE);
	}
	else if (self->client->pers.cmd.forwardmove < 0)
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_HOP_B, AFLAG_PACE);
	}
	else
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_HOP_F, AFLAG_PACE);
	}
}

void ForceSpeedAnimUnstable(gentity_t * self)
{
	if (self->client->pers.cmd.rightmove > 0)
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_DASH_R, AFLAG_PACE);
	}
	else if (self->client->pers.cmd.rightmove < 0)
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_DASH_L, AFLAG_PACE);
	}
	else if (self->client->pers.cmd.forwardmove < 0)
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_DASH_B, AFLAG_PACE);
	}
	else
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_DASH_F, AFLAG_PACE);
	}
}

void ForceSpeed(gentity_t * self, int forceDuration)
{
	if (self->health <= 0)
	{
		return;
	}

	if (BG_InKnockDown(self->client->ps.legsAnim) || PM_InKnockDown(&self->client->ps))
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_SPEED)))
	{
		WP_ForcePowerStop(self, FP_SPEED);
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_SPEED))
	{
		return;
	}

	if (self->client->ps.saberLockTime > level.time)
	{
		return;
	}

	if (self->client->ps.fd.forcePowersActive & (1 << FP_SPEED))
	{//it's already turned on.  turn it off.
		WP_ForcePowerStop(self, FP_SPEED);
		return;
	}

	if (self->client->holdingObjectiveItem >= MAX_CLIENTS
		&& self->client->holdingObjectiveItem < ENTITYNUM_WORLD)
	{//holding Siege item
		if (g_entities[self->client->holdingObjectiveItem].genericValue15)
		{//disables force powers
			return;
		}
	}

	WP_DebounceForceDeactivateTime(self);
	WP_ForcePowerStart(self, FP_SPEED, forceDuration);

	if (self->client->ps.fd.forcePowerLevel[FP_SPEED] < FORCE_LEVEL_3)
	{//short burst
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/speed.mp3"));
	}
	else
	{//holding it
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/speedloop.wav"));
	}
	G_PlayBoltedEffect(G_EffectIndex("misc/breath.efx"), self, "*head_front");
}

void ForceSpeedDash(gentity_t * self, int forceDuration)
{
	vec3_t dir;

	saberInfo_t* saber1;

	saber1 = BG_MySaber(self->clientNum, 0);

	if (self->health <= 0)
	{
		return;
	}
	if (self->client->ps.fd.forcePowerDebounce[FP_SPEED] > level.time)
	{//stops it while using it and also after using it, up to 3 second delay
		return;
	}

	if (self->client->ps.fd.forceSpeedRecoveryTime >= level.time)
	{
		return;
	}

	if (BG_InKnockDown(self->client->ps.legsAnim) || PM_InKnockDown(&self->client->ps))
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_SPEED)))
	{
		WP_ForcePowerStop(self, FP_SPEED);
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_SPEED))
	{
		return;
	}

	if (self->client->ps.saberLockTime > level.time)
	{
		return;
	}

	if (self->client->ps.fd.forcePowersActive & (1 << FP_SPEED))
	{//it's already turned on.  turn it off.
		WP_ForcePowerStop(self, FP_SPEED);
		return;
	}

	if (self->client->ps.fd.forcePowersActive & (1 << FP_SPEED) && (!(IsPressingDashButton(self))))
	{//it's already turned on.  turn it off.
		WP_ForcePowerStop(self, FP_SPEED);
		return;
	}

	if (self->client->ps.fd.forcePowersActive & (1 << FP_SPEED) &&
		self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_FORCEINAIR1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		WP_ForcePowerStop(self, FP_SPEED);
	}

	if (self->client->holdingObjectiveItem >= MAX_CLIENTS
		&& self->client->holdingObjectiveItem < ENTITYNUM_WORLD)
	{//holding Siege item
		if (g_entities[self->client->holdingObjectiveItem].genericValue15)
		{//disables force powers
			return;
		}
	}

	WP_DebounceForceDashDeactivateTime(self);
	WP_ForcePowerStart(self, FP_SPEED, 0);

	if (self->client->ps.fd.forcePowersActive & (1 << FP_SPEED) &&
		self->client->ps.groundEntityNum != ENTITYNUM_NONE)
	{
		AngleVectors(self->client->ps.viewangles, dir, NULL, NULL);
		self->client->ps.velocity[0] = self->client->ps.velocity[0] * 7;
		self->client->ps.velocity[1] = self->client->ps.velocity[1] * 7;

		if (saber1->type == SABER_UNSTABLE//saber kylo
			|| saber1->type == SABER_STAFF_UNSTABLE
			|| saber1->type == SABER_BACKHAND
			|| saber1->type == SABER_ASBACKHAND
			|| saber1->type == SABER_ANAKIN
			|| saber1->type == SABER_PALP
			|| saber1->type == SABER_DOOKU
			|| saber1->type == SABER_YODA)//saber yoda
		{
			ForceSpeedAnimUnstable(self);
		}
		else
		{
			ForceSpeedAnim(self);
		}
	}
	else if (self->client->ps.fd.forcePowersActive& (1 << FP_SPEED) &&	self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_FORCEINAIR1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		WP_ForcePowerStop(self, FP_SPEED);
	}

	if (forceDuration)
	{
		self->client->ps.fd.forcePowerDuration[FP_SPEED] = level.time + forceDuration;
	}


	G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/dash.mp3"));
	G_PlayBoltedEffect(G_EffectIndex("misc/breath.efx"), self, "*head_front");
}

void ForceSeeing(gentity_t * self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive& (1 << FP_SEE)))
	{
		WP_ForcePowerStop(self, FP_SEE);
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_SEE))
	{
		return;
	}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart(self, FP_SEE, 0);

	if (self->client->ps.saberLockTime < level.time && !PM_InKnockDown(&self->client->ps))
	{
		NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_DRAIN_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		self->client->ps.weaponTime = self->client->ps.torsoTimer;
		self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
		self->client->ps.saberBlocked = BLOCKED_NONE;
	}

	G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/see.wav"));
	G_Sound(self, TRACK_CHANNEL_5, seeLoopSound);
}

void ForceProtect(gentity_t * self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_PROTECT)))
	{
		WP_ForcePowerStop(self, FP_PROTECT);
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_PROTECT))
	{
		return;
	}

	// Make sure to turn off Force Rage.
	if (self->client->ps.fd.forcePowersActive& (1 << FP_RAGE))
	{
		WP_ForcePowerStop(self, FP_RAGE);
	}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart(self, FP_PROTECT, 0);

	if (self->client->ps.saberLockTime < level.time && !PM_InKnockDown(&self->client->ps))
	{
		if (self->client->ps.fd.forcePowerLevel[FP_PROTECT] > FORCE_LEVEL_1)
		{//level 2 only does it on torso (can keep running)
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_PROTECT_FAST, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		else
		{
			NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_PROTECT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		self->client->ps.weaponTime = self->client->ps.torsoTimer;
		self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
		self->client->ps.saberBlocked = BLOCKED_NONE;
	}

	G_PreDefSound(self->client->ps.origin, PDSOUND_PROTECT);
	G_Sound(self, TRACK_CHANNEL_3, protectLoopSound);
}

void ForceAbsorb(gentity_t * self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_ABSORB)))
	{
		WP_ForcePowerStop(self, FP_ABSORB);
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_ABSORB))
	{
		return;
	}

	// Make sure to turn off Force Rage and Force Protection.
	if (self->client->ps.fd.forcePowersActive& (1 << FP_RAGE))
	{
		WP_ForcePowerStop(self, FP_RAGE);
	}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart(self, FP_ABSORB, 0);

	if (self->client->ps.saberLockTime < level.time && !PM_InKnockDown(&self->client->ps))
	{
		NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_ABSORB, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		self->client->ps.weaponTime = self->client->ps.torsoTimer;
		self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
		self->client->ps.saberBlocked = BLOCKED_NONE;
	}

	G_PreDefSound(self->client->ps.origin, PDSOUND_ABSORB);
	G_Sound(self, TRACK_CHANNEL_3, absorbLoopSound);
}

void ForceRage(gentity_t * self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive & (1 << FP_RAGE)))
	{
		WP_ForcePowerStop(self, FP_RAGE);
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_RAGE))
	{
		return;
	}

	if (self->client->ps.fd.forceRageRecoveryTime >= level.time)
	{
		return;
	}

	if (self->s.number < MAX_CLIENTS
		&& self->health < 25)
	{//have to have at least 25 health to start it
		return;
	}

	if (self->health < 10)
	{
		return;
	}

	// Make sure to turn off Force Protection and Force Absorb.
	if (self->client->ps.fd.forcePowersActive& (1 << FP_PROTECT))
	{
		WP_ForcePowerStop(self, FP_PROTECT);
	}
	if (self->client->ps.fd.forcePowersActive& (1 << FP_ABSORB))
	{
		WP_ForcePowerStop(self, FP_ABSORB);
	}

	self->client->ps.forceAllowDeactivateTime = level.time + 1500;

	WP_ForcePowerStart(self, FP_RAGE, 0);

	if (self->client->ps.saberLockTime < level.time && !PM_InKnockDown(&self->client->ps))
	{
		NPC_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_RAGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		self->client->ps.weaponTime = self->client->ps.torsoTimer;
		self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
		self->client->ps.saberBlocked = BLOCKED_NONE;
	}

	if (self->client->pers.botclass == BCLASS_CHEWIE
		|| self->client->pers.botclass == BCLASS_WOOKIE
		|| self->client->pers.botclass == BCLASS_WOOKIEMELEE || self->client->NPC_class == CLASS_WOOKIE)
	{
		G_SoundOnEnt(self, CHAN_VOICE, "sound/chars/chewbacca/misc/death2.mp3");
	}

	G_Sound(self, TRACK_CHANNEL_4, G_SoundIndex("sound/weapons/force/rage.wav"));
	G_Sound(self, TRACK_CHANNEL_3, rageLoopSound);
	G_PlayBoltedEffect(G_EffectIndex("misc/breathSith.efx"), self, "*head_front");
}

void ForceLightning(gentity_t * self)
{
	if (self->health <= 0)
	{
		return;
	}
	if (BG_InKnockDown(self->client->ps.legsAnim) || self->client->ps.leanofs || PM_InKnockDown(&self->client->ps))
	{
		return;
	}
	if (self->client->ps.fd.forcePower < 25 || !WP_ForcePowerUsable(self, FP_LIGHTNING))
	{
		return;
	}
	if (self->client->ps.fd.forcePowerDebounce[FP_LIGHTNING] > level.time)
	{//stops it while using it and also after using it, up to 3 second delay
		return;
	}
	if (self->client->ps.saberLockTime > level.time)
	{//FIXME: can this be a way to break out?
		return;
	}

	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saberMove) || !(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK))))
	{
		return;
	}
	// Make sure to turn off Force Protection and Force Absorb.
	if (self->client->ps.fd.forcePowersActive& (1 << FP_PROTECT))
	{
		WP_ForcePowerStop(self, FP_PROTECT);
	}
	if (self->client->ps.fd.forcePowersActive& (1 << FP_ABSORB))
	{
		WP_ForcePowerStop(self, FP_ABSORB);
	}
	if (self->client->ps.fd.forcePowersActive& (1 << FP_SEE))
	{
		WP_ForcePowerStop(self, FP_SEE);
	}

	// fix: rocket lock bug
	BG_ClearRocketLock(&self->client->ps);

	//Shoot lightning from hand
	//using grip anim now, to extend the burst time
	self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	self->client->ps.forceHandExtendTime = level.time + 20000;


	if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_2)
	{//short burst
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/lightning2.wav"));
	}
	else
	{//holding it
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/lightning.mp3"));
	}

	WP_ForcePowerStart(self, FP_LIGHTNING, 500);
}

qboolean Melee_BlockLightning_CounterForce(gentity_t * attacker, gentity_t * defender, int attackPower)
{//generically checks to see if the defender is able to block an attack from this attacker 

	if (!Manual_HandBlocking(defender))
	{
		return qfalse;
	}

	if (!WalkCheck(defender) || defender->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{//can't block  Force power while running or in mid-air
		return qfalse;
	}
	if (!(defender->client->ps.fd.forcePowersKnown& (1 << FP_ABSORB)))
	{//doesn't have absorb 
		return qfalse;
	}

	return qtrue;
}

qboolean Melee_BlockLightning(gentity_t * attacker, gentity_t * defender, vec3_t impactPoint, int damage)
{//defender is attempting to block lightning.  Try to do it.
	int fpBlockCost;
	float blockFactor = 0;
	qboolean MeleeLightBlock = qtrue;

	if (defender->client->ps.weapon == WP_SABER
		|| defender->client->ps.saberHolstered == 2
		|| defender->client->ps.saberInFlight)
	{
		return qfalse;
	}
	if (defender->client->ps.weapon != WP_MELEE)
	{
		return qfalse;
	}

	if (!(defender->client->buttons & BUTTON_BLOCK))
	{
		if (defender->r.svFlags & SVF_BOT || defender->s.eType == ET_NPC)
		{//bots just randomly dodge to make up for them not intelligently dodgeing
			blockFactor = 0.7f;
		}
		else
		{
			return qfalse;
		}
	}

	if (!Melee_BlockLightning_CounterForce(attacker, defender, FP_LIGHTNING))
	{//failed generic Force countering effect
		return qfalse;
	}

	if (!defender || !defender->client || !attacker || !attacker->client)
	{//bad infor state
		return qfalse;
	}

	if (!InFront(attacker->client->ps.origin, defender->client->ps.origin, defender->client->ps.viewangles, -0.7f)
		|| !MeleeLightBlock) //can't block behind us while hand blocking.
	{//not facing the lightning attacker 
		return qfalse;
	}
	//determine the cost to block the lightning
	fpBlockCost = 1;

	if (defender->client->ps.fd.forcePower < 20)
	{
		return qfalse;
	}

	if (MeleeLightBlock)
	{
		NPC_SetAnim(defender, SETANIM_TORSO, BOTH_FORCE_2HANDEDLIGHTNING, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		defender->client->ps.weaponTime = Q_irand(300, 600);
		G_PlayBoltedEffect(G_EffectIndex("force/confusion.efx"), defender, "*r_hand");
		WP_ForcePowerDrain(&defender->client->ps, FP_ABSORB, fpBlockCost);
		G_PlayBoltedEffect(G_EffectIndex("force/confusion.efx"), defender, "*l_hand");
	}
	return qtrue;
}

qboolean Saber_BlockLightning(gentity_t * attacker, gentity_t * defender, vec3_t impactPoint, int damage)
{//defender is attempting to block lightning.  Try to do it.
	int fpBlockCost;
	qboolean saberLightBlock = qtrue;

	if (!Manual_Saberblocking(defender))
	{
		return qfalse;
	}

	if (!defender || !defender->client || !attacker || !attacker->client)
	{//bad infor state
		return qfalse;
	}

	if (!InFront(attacker->client->ps.origin, defender->client->ps.origin, defender->client->ps.viewangles, -0.7f)
		|| !saberLightBlock) //can't block behind us while hand blocking.
	{//not facing the lightning attacker 
		return qfalse;
	}

	//determine the cost to block the lightning
	fpBlockCost = 1;

	if (defender->client->ps.fd.forcePower < 10)
	{
		return qfalse;
	}

	if (saberLightBlock)
	{
		defender->client->ps.saberBlocked = BLOCKED_TOP;
		defender->client->ps.weaponTime = Q_irand(300, 600);
		if (defender->client->ps.fd.blockPoints < BLOCKPOINTS_HALF)
		{
			WP_ForcePowerDrain(&defender->client->ps, FP_ABSORB, fpBlockCost);
		}
		defender->client->ps.fd.blockPoints--;
	}
	return qtrue;

}

void ForceLightningDamage(gentity_t * self, gentity_t * traceEnt, vec3_t dir, float dist, float dot, vec3_t impactPoint)
{
	gentity_t* tent;
	qboolean saberLightningBlocked = qfalse;
	qboolean handLightningBlocked = qfalse;
	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	if (traceEnt && traceEnt->takedamage)
	{
		if (!traceEnt->client && traceEnt->s.eType == ET_NPC)
		{ //g2animent
			if (traceEnt->s.genericenemyindex < level.time)
			{
				traceEnt->s.genericenemyindex = level.time + 2000;
			}
		}
		if (traceEnt->client)
		{
			int	dmg;

			if (traceEnt->client->noLightningTime >= level.time)
			{ //give them power and don't hurt them.
				traceEnt->client->ps.fd.forcePower++;
				if (traceEnt->client->ps.fd.forcePower > traceEnt->client->ps.fd.forcePowerMax)
				{
					traceEnt->client->ps.fd.forcePower = traceEnt->client->ps.fd.forcePowerMax;
				}
				return;
			}
			if (ForcePowerUsableOn(self, traceEnt, FP_LIGHTNING))
			{
				dmg = 1;

				if (self->client->NPC_class == CLASS_REBORN && self->client->ps.weapon == WP_NONE)
				{
					//Cultist: looks fancy, but does less damage
				}
				else
				{
					if (dist < 100)
					{
						dmg += 2;
					}
					else if (dist < 200)
					{
						dmg += 1;
					}
					if (dot > 0.9f)
					{
						dmg += 2;
					}
					else if (dot > 0.7f)
					{
						dmg += 1;
					}
				}

				if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_OLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_NEW
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
				{//jackin' 'em up, Palpatine-style
					dmg *= 2;
				}
				else
				{
					dmg = 1;
				}
				saberLightningBlocked = Saber_BlockLightning(self, traceEnt, impactPoint, dmg);
				handLightningBlocked = Melee_BlockLightning(self, traceEnt, impactPoint, dmg);

				if (dmg && !saberLightningBlocked && !handLightningBlocked)
				{
					G_Damage(traceEnt, self, self, dir, impactPoint, dmg, 0, MOD_FORCE_LIGHTNING);

					if ((traceEnt->NPC || traceEnt->s.eType == ET_PLAYER || self->r.svFlags& SVF_BOT)
						&& !(traceEnt->s.weapon == WP_EMPLACED_GUN))
					{
						if (traceEnt && (traceEnt->s.number < MAX_CLIENTS || G_ControlledByPlayer(traceEnt)))
						{
							traceEnt->client->stunDamage = 9;
							traceEnt->client->stunTime = level.time + 1000;

							tent = G_TempEntity(traceEnt->r.currentOrigin, EV_STUNNED);
							tent->s.owner = traceEnt->s.number;
						}

						if (traceEnt->client->ps.stats[STAT_HEALTH] <= 0)
						{
							VectorClear(traceEnt->client->ps.lastHitLoc);
							VectorClear(traceEnt->client->ps.velocity);

							traceEnt->client->ps.eFlags |= EF_DISINTEGRATION;
							traceEnt->r.contents = 0;

							traceEnt->think = G_FreeEntity;
							traceEnt->nextthink = level.time;
						}

						if (PM_RunningAnim(traceEnt->client->ps.legsAnim) && traceEnt->client->ps.stats[STAT_HEALTH] > 1)
						{
							G_KnockOver(traceEnt, self, dir, 25, qtrue);
						}
						else
						{
							if (!PM_RunningAnim(traceEnt->client->ps.legsAnim) && traceEnt->client->ps.stats[STAT_HEALTH] > 1)
							{
								if (traceEnt->client->ps.stats[STAT_HEALTH] < 70)
								{
									G_SetAnim(traceEnt, &self->client->pers.cmd, SETANIM_TORSO, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
								}
								else
								{
									G_SetAnim(traceEnt, &self->client->pers.cmd, SETANIM_TORSO, BOTH_WIND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
								}
							}
							else
							{
								if (traceEnt->client->ps.stats[STAT_HEALTH] < 2)
								{
									vec3_t	defaultDir;
									VectorSet(defaultDir, 0, 0, 1);
									G_PlayEffectID(G_EffectIndex("force/Lightningkill.efx"), traceEnt->r.currentOrigin, defaultDir);
								}
							}
						}
					}
				}

				if (traceEnt->client)
				{
					if (!Q_irand(0, 2))
					{
						G_Sound(traceEnt, CHAN_BODY, G_SoundIndex(va("sound/weapons/force/lightninghit%d.mp3", Q_irand(1, 3))));
					}

					if (traceEnt->client->ps.powerups[PW_CLOAKED])
					{//disable cloak temporarily
						Jedi_Decloak(traceEnt);
						traceEnt->client->cloakToggleTime = level.time + Q_irand(3000, 10000);
					}

					class_t npc_class = traceEnt->client->NPC_class;
					bclass_t botclass = traceEnt->client->botclass;

					if (traceEnt->health <= 0 || (botclass == BCLASS_SEEKER || botclass == BCLASS_SBD ||
						botclass == BCLASS_MANDOLORIAN || botclass == BCLASS_MANDOLORIAN1 || botclass == BCLASS_MANDOLORIAN2 ||
						botclass == BCLASS_BOBAFETT || botclass == BCLASS_BATTLEDROID || botclass == BCLASS_DROIDEKA))
					{
						traceEnt->client->ps.electrifyTime = level.time + 4000;
					}
					else if (traceEnt->health <= 0 || (npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE ||
						npc_class == CLASS_MOUSE || npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_REMOTE ||
						npc_class == CLASS_R5D2 || npc_class == CLASS_PROTOCOL || npc_class == CLASS_MARK1 ||
						npc_class == CLASS_SBD || npc_class == CLASS_BATTLEDROID || npc_class == CLASS_DROIDEKA ||
						npc_class == CLASS_MARK2 || npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST ||
						npc_class == CLASS_SENTRY))
					{// special droid only behaviors
						traceEnt->client->ps.electrifyTime = level.time + 4000;
					}
					else if (traceEnt->client->ps.electrifyTime < (level.time + 400) && !saberLightningBlocked && !handLightningBlocked)
					{ //only update every 400ms to reduce bandwidth usage (as it is passing a 32-bit time value)
						traceEnt->client->ps.electrifyTime = level.time + 3000;
					}

					if (traceEnt->client
						&& traceEnt->health > 0
						&& traceEnt->NPC
						&& (traceEnt->client->ps.weapon == WP_SABER))
					{
						if (Manual_Saberblocking(traceEnt)
							&& traceEnt->client->ps.fd.forcePower > 20
							&& !traceEnt->client->ps.saberHolstered
							&& !traceEnt->client->ps.saberInFlight//saber in hand
							&& InFOV3(self->r.currentOrigin, traceEnt->r.currentOrigin, traceEnt->client->ps.viewangles, 20, 35) //I'm in front of them
							&& !PM_InKnockDown(&traceEnt->client->ps))
						{//saber can block lightning
							int rSaberNum = 0;
							int rBladeNum = 0;
							traceEnt->client->saber[rSaberNum].blade[rBladeNum].storageTime = level.time;
							if (saberLightningBlocked && !handLightningBlocked)
							{
								float chanceOfFizz = flrand(0.0f, 1.0f);
								vec3_t	end2;
								vec3_t ang = { 0, 0, 0 };
								ang[0] = flrand(0, 360);
								ang[1] = flrand(0, 360);
								ang[2] = flrand(0, 360);
								if (chanceOfFizz > 0)
								{
									VectorMA(traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, traceEnt->client->saber[rSaberNum].blade[rBladeNum].lengthMax * flrand(0, 1), traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, end2);
									G_PlayEffectID(G_EffectIndex("saber/saber_Lightninghit.efx"), end2, ang);
								}
								traceEnt->client->ps.saberBlocked = BLOCKED_TOP;
								traceEnt->client->ps.weaponTime = Q_irand(300, 600);
							}
							else
							{
								dmg = 2;
							}
						}
					}
					else if (traceEnt->client->ps.weapon == WP_SABER)
					{//Serenitysabersystems saber can block lightning
						int rSaberNum = 0;
						int rBladeNum = 0;
						traceEnt->client->saber[rSaberNum].blade[rBladeNum].storageTime = level.time;
						if (saberLightningBlocked && !handLightningBlocked)
						{
							float chanceOfFizz = flrand(0.0f, 1.0f);
							vec3_t	end2;
							vec3_t ang = { 0, 0, 0 };
							ang[0] = flrand(0, 360);
							ang[1] = flrand(0, 360);
							ang[2] = flrand(0, 360);
							if (chanceOfFizz > 0)
							{
								VectorMA(traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzlePoint, traceEnt->client->saber[rSaberNum].blade[rBladeNum].lengthMax * flrand(0, 1), traceEnt->client->saber[rSaberNum].blade[rBladeNum].muzzleDir, end2);
								G_PlayEffectID(G_EffectIndex("saber/saber_Lightninghit.efx"), end2, ang);
							}
							traceEnt->client->ps.saberBlocked = BLOCKED_TOP;
							traceEnt->client->ps.weaponTime = Q_irand(300, 600);
						}
						else
						{
							dmg = 1;
						}
					}
					else if (traceEnt->client->ps.weapon == WP_NONE
						|| traceEnt->client->ps.weapon == WP_MELEE
						&& traceEnt->client->ps.fd.forcePower > 20
						&& InFront(self->client->ps.origin, traceEnt->client->ps.origin, traceEnt->client->ps.viewangles, -0.7f))
					{
						if (handLightningBlocked && !saberLightningBlocked)
						{
							dmg = 0;
						}
						else
						{
							dmg = 1;
						}
					}
					else if (traceEnt && traceEnt->client && traceEnt->client->NPC_class == CLASS_GALAK)
					{
						if (traceEnt->client->ps.powerups[PW_GALAK_SHIELD])
						{//has shield up
							dmg = 0;
						}
					}
				}
			}
		}
	}
}

void ForceShootLightning(gentity_t * self)
{
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t* traceEnt;

	if (self->health <= 0)
	{
		return;
	}
	AngleVectors(self->client->ps.viewangles, forward, NULL, NULL);
	VectorNormalize(forward);

	if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_2)
	{//arc
		vec3_t	center, mins, maxs, dir, ent_org, size, v;
		float	radius = FORCE_LIGHTNING_RADIUS, dot, dist;
		gentity_t* entityList[MAX_GENTITIES];
		int			iEntityList[MAX_GENTITIES];
		int		e, numListedEntities, i;

		VectorCopy(self->client->ps.origin, center);
		for (i = 0; i < 3; i++)
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
		}
		numListedEntities = trap->EntitiesInBox(mins, maxs, iEntityList, MAX_GENTITIES);

		i = 0;
		while (i < numListedEntities)
		{
			entityList[i] = &g_entities[iEntityList[i]];

			i++;
		}

		for (e = 0; e < numListedEntities; e++)
		{
			traceEnt = entityList[e];

			if (!traceEnt)
				continue;
			if (traceEnt == self)
				continue;
			if (traceEnt->r.ownerNum == self->s.number && traceEnt->s.weapon != WP_THERMAL)//can push your own thermals
				continue;
			if (!traceEnt->inuse)
				continue;
			if (!traceEnt->takedamage)
				continue;
			if (traceEnt->health <= 0)//no torturing corpses
				continue;
			if (!g_friendlyFire.integer&& OnSameTeam(self, traceEnt))
				continue;
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for (i = 0; i < 3; i++)
			{
				if (center[i] < traceEnt->r.absmin[i])
				{
					v[i] = traceEnt->r.absmin[i] - center[i];
				}
				else if (center[i] > traceEnt->r.absmax[i])
				{
					v[i] = center[i] - traceEnt->r.absmax[i];
				}
				else
				{
					v[i] = 0;
				}
			}

			VectorSubtract(traceEnt->r.absmax, traceEnt->r.absmin, size);
			VectorMA(traceEnt->r.absmin, 0.5, size, ent_org);

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract(ent_org, center, dir);
			VectorNormalize(dir);
			if ((dot = DotProduct(dir, forward)) < 0.5)
				continue;

			//must be close enough
			dist = VectorLength(v);
			if (dist >= radius)
			{
				continue;
			}

			//in PVS?
			if (!traceEnt->r.bmodel && !trap->InPVS(ent_org, self->client->ps.origin))
			{//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
			trap->Trace(&tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT, qfalse, 0, 0);
			if (tr.fraction < 1.0f&& tr.entityNum != traceEnt->s.number)
			{//must have clear LOS
				continue;
			}

			// ok, we are within the radius, add us to the incoming list
			ForceLightningDamage(self, traceEnt, dir, dist, dot, ent_org);
		}
	}
	else
	{//trace-line
		VectorMA(self->client->ps.origin, 2048, forward, end);

		trap->Trace(&tr, self->client->ps.origin, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, qfalse, 0, 0);
		if (tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
		{
			return;
		}

		traceEnt = &g_entities[tr.entityNum];
		ForceLightningDamage(self, traceEnt, forward, 0, 0, tr.endpos);
	}
}

void ForceDrain(gentity_t * self)
{
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return;
	}

	if (self->client->ps.fd.forcePower < 25 || !WP_ForcePowerUsable(self, FP_DRAIN))
	{
		return;
	}
	if (self->client->ps.fd.forcePowerDebounce[FP_DRAIN] > level.time)
	{//stops it while using it and also after using it, up to 3 second delay
		return;
	}

	//	self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
	//	self->client->ps.forceHandExtendTime = level.time + 1000;
	self->client->ps.forceHandExtend = HANDEXTEND_FORCE_HOLD;
	self->client->ps.forceHandExtendTime = level.time + 20000;

	G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/drain.wav"));

	WP_ForcePowerStart(self, FP_DRAIN, 500);
}

void ForceDrainDamage(gentity_t * self, gentity_t * traceEnt, vec3_t dir, vec3_t impactPoint)
{
	gentity_t* tent;

	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	if (traceEnt && traceEnt->takedamage)
	{
		if (traceEnt->client && (!OnSameTeam(self, traceEnt) || g_friendlyFire.integer) && self->client->ps.fd.forceDrainTime < level.time && traceEnt->client->ps.fd.forcePower)
		{//an enemy or object
			if (!traceEnt->client && traceEnt->s.eType == ET_NPC)
			{ //g2animent
				if (traceEnt->s.genericenemyindex < level.time)
				{
					traceEnt->s.genericenemyindex = level.time + 2000;
				}
			}
			if (ForcePowerUsableOn(self, traceEnt, FP_DRAIN))
			{
				int modPowerLevel = -1;
				int	dmg = 0; //Q_irand( 1, 3 );
				if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] == FORCE_LEVEL_1)
				{
					dmg = 2; //because it's one-shot
				}
				else if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] == FORCE_LEVEL_2)
				{
					dmg = 3;
				}
				else if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] == FORCE_LEVEL_3)
				{
					dmg = 4;
				}

				if (traceEnt->client)
				{
					modPowerLevel = WP_AbsorbConversion(traceEnt, traceEnt->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_DRAIN, self->client->ps.fd.forcePowerLevel[FP_DRAIN], 1);
				}

				if (modPowerLevel != -1)
				{
					if (!modPowerLevel)
					{
						dmg = 0;
					}
					else if (modPowerLevel == 1)
					{
						dmg = 1;
					}
					else if (modPowerLevel == 2)
					{
						dmg = 2;
					}
				}

				if (dmg)
				{
					traceEnt->client->ps.fd.forcePower -= (dmg);
				}
				if (traceEnt->client->ps.fd.forcePower < 0)
				{
					traceEnt->client->ps.fd.forcePower = 0;
				}

				if (self->client->ps.stats[STAT_HEALTH] < self->client->ps.stats[STAT_MAX_HEALTH] &&
					self->health > 0 && self->client->ps.stats[STAT_HEALTH] > 0)
				{
					self->health += dmg;
					if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
					{
						self->health = self->client->ps.stats[STAT_MAX_HEALTH];
					}
					self->client->ps.stats[STAT_HEALTH] = self->health;
				}

				traceEnt->client->ps.fd.forcePowerRegenDebounceTime = level.time + 800; //don't let the client being drained get force power back right away

				if (traceEnt->client->forcePowerSoundDebounce < level.time)
				{
					tent = G_TempEntity(impactPoint, EV_FORCE_DRAINED);
					tent->s.eventParm = DirToByte(dir);
					tent->s.owner = traceEnt->s.number;

					traceEnt->client->forcePowerSoundDebounce = level.time + 400;
				}
			}
		}
	}
}

int ForceShootDrain(gentity_t * self)
{
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t* traceEnt;
	int			gotOneOrMore = 0;

	if (self->health <= 0)
	{
		return 0;
	}
	AngleVectors(self->client->ps.viewangles, forward, NULL, NULL);
	VectorNormalize(forward);

	if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_2)
	{//arc
		vec3_t	center, mins, maxs, dir, ent_org, size, v;
		float	radius = MAX_DRAIN_DISTANCE, dot, dist;
		gentity_t* entityList[MAX_GENTITIES];
		int			iEntityList[MAX_GENTITIES];
		int		e, numListedEntities, i;

		VectorCopy(self->client->ps.origin, center);
		for (i = 0; i < 3; i++)
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
		}
		numListedEntities = trap->EntitiesInBox(mins, maxs, iEntityList, MAX_GENTITIES);

		i = 0;
		while (i < numListedEntities)
		{
			entityList[i] = &g_entities[iEntityList[i]];

			i++;
		}

		for (e = 0; e < numListedEntities; e++)
		{
			traceEnt = entityList[e];

			if (!traceEnt)
				continue;
			if (traceEnt == self)
				continue;
			if (!traceEnt->inuse)
				continue;
			if (!traceEnt->takedamage)
				continue;
			if (traceEnt->health <= 0)//no torturing corpses
				continue;
			if (!traceEnt->client)
				continue;
			if (!traceEnt->client->ps.fd.forcePower)
				continue;
			if (OnSameTeam(self, traceEnt) && !g_friendlyFire.integer)
				continue;
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for (i = 0; i < 3; i++)
			{
				if (center[i] < traceEnt->r.absmin[i])
				{
					v[i] = traceEnt->r.absmin[i] - center[i];
				}
				else if (center[i] > traceEnt->r.absmax[i])
				{
					v[i] = center[i] - traceEnt->r.absmax[i];
				}
				else
				{
					v[i] = 0;
				}
			}

			VectorSubtract(traceEnt->r.absmax, traceEnt->r.absmin, size);
			VectorMA(traceEnt->r.absmin, 0.5, size, ent_org);

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract(ent_org, center, dir);
			VectorNormalize(dir);
			if ((dot = DotProduct(dir, forward)) < 0.5)
				continue;

			//must be close enough
			dist = VectorLength(v);
			if (dist >= radius)
			{
				continue;
			}

			//in PVS?
			if (!traceEnt->r.bmodel && !trap->InPVS(ent_org, self->client->ps.origin))
			{//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
			trap->Trace(&tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT, qfalse, 0, 0);
			if (tr.fraction < 1.0f&& tr.entityNum != traceEnt->s.number)
			{//must have clear LOS
				continue;
			}

			// ok, we are within the radius, add us to the incoming list
			ForceDrainDamage(self, traceEnt, dir, ent_org);
			gotOneOrMore = 1;
		}
	}
	else
	{//trace-line
		VectorMA(self->client->ps.origin, 2048, forward, end);

		trap->Trace(&tr, self->client->ps.origin, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, qfalse, 0, 0);
		if (tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid || !g_entities[tr.entityNum].client || !g_entities[tr.entityNum].inuse)
		{
			return 0;
		}

		traceEnt = &g_entities[tr.entityNum];
		ForceDrainDamage(self, traceEnt, forward, tr.endpos);
		gotOneOrMore = 1;
	}

	self->client->ps.activeForcePass = self->client->ps.fd.forcePowerLevel[FP_DRAIN] + FORCE_LEVEL_3;

	WP_ForcePowerDrain(&self->client->ps, FP_DRAIN, 5); //used to be 1, but this did, too, anger the God of Balance.

	self->client->ps.fd.forcePowerRegenDebounceTime = level.time + 500;

	return gotOneOrMore;
}

void ForceJumpCharge(gentity_t * self, usercmd_t * ucmd)
{ //I guess this is unused now. Was used for the "charge" jump type.
	float forceJumpChargeInterval = forceJumpStrength[0] / (FORCE_JUMP_CHARGE_TIME / FRAMETIME);

	if (self->health <= 0)
	{
		return;
	}

	if (!self->client->ps.fd.forceJumpCharge && self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		return;
	}

	if (self->client->ps.fd.forcePower < forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]][FP_LEVITATION])
	{
		G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1 - 50], CHAN_VOICE);
		return;
	}

	if (!self->client->ps.fd.forceJumpCharge)
	{
		self->client->ps.fd.forceJumpAddTime = 0;
	}

	if (self->client->ps.fd.forceJumpAddTime >= level.time)
	{
		return;
	}

	//need to play sound
	if (!self->client->ps.fd.forceJumpCharge)
	{
		G_Sound(self, TRACK_CHANNEL_1, G_SoundIndex("sound/weapons/force/jumpbuild.wav"));
	}

	//Increment
	if (self->client->ps.fd.forceJumpAddTime < level.time)
	{
		self->client->ps.fd.forceJumpCharge += forceJumpChargeInterval * 50;
		self->client->ps.fd.forceJumpAddTime = level.time + 500;
	}

	//clamp to max strength for current level
	if (self->client->ps.fd.forceJumpCharge > forceJumpStrength[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]])
	{
		self->client->ps.fd.forceJumpCharge = forceJumpStrength[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]];
		G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1 - 50], CHAN_VOICE);
	}

	//clamp to max available force power
	if (self->client->ps.fd.forceJumpCharge / forceJumpChargeInterval / (FORCE_JUMP_CHARGE_TIME / FRAMETIME) * forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]][FP_LEVITATION] > self->client->ps.fd.forcePower)
	{//can't use more than you have
		G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1 - 50], CHAN_VOICE);
		self->client->ps.fd.forceJumpCharge = self->client->ps.fd.forcePower * forceJumpChargeInterval / (FORCE_JUMP_CHARGE_TIME / FRAMETIME);
	}
}

int WP_GetVelocityForForceJump(gentity_t * self, vec3_t jumpVel, usercmd_t * ucmd)
{
	float pushFwd = 0, pushRt = 0;
	vec3_t	view, forward, right;
	VectorCopy(self->client->ps.viewangles, view);
	view[0] = 0;
	AngleVectors(view, forward, right, NULL);
	if (ucmd->forwardmove && ucmd->rightmove)
	{
		if (ucmd->forwardmove > 0)
		{
			pushFwd = 50;
		}
		else
		{
			pushFwd = -50;
		}
		if (ucmd->rightmove > 0)
		{
			pushRt = 50;
		}
		else
		{
			pushRt = -50;
		}
	}
	else if (ucmd->forwardmove || ucmd->rightmove)
	{
		if (ucmd->forwardmove > 0)
		{
			pushFwd = 100;
		}
		else if (ucmd->forwardmove < 0)
		{
			pushFwd = -100;
		}
		else if (ucmd->rightmove > 0)
		{
			pushRt = 100;
		}
		else if (ucmd->rightmove < 0)
		{
			pushRt = -100;
		}
	}

	G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1 - 50], CHAN_VOICE);


	if (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
	{//short burst
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/jumpsmall.mp3"));
	}
	else
	{//holding it
		if (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
		{//short burst
			G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/jumpsmall.mp3"));
		}
		else
		{//holding it
			G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEJUMP);
		}
	}


	if (self->client->ps.fd.forceJumpCharge < JUMP_VELOCITY + 40)
	{ //give him at least a tiny boost from just a tap
		self->client->ps.fd.forceJumpCharge = JUMP_VELOCITY + 400;
	}

	if (self->client->ps.velocity[2] < -30)
	{ //so that we can get a good boost when force jumping in a fall
		self->client->ps.velocity[2] = -30;
	}

	VectorMA(self->client->ps.velocity, pushFwd, forward, jumpVel);
	VectorMA(self->client->ps.velocity, pushRt, right, jumpVel);
	jumpVel[2] += self->client->ps.fd.forceJumpCharge;
	if (pushFwd > 0 && self->client->ps.fd.forceJumpCharge > 200)
	{
		return FJ_FORWARD;
	}
	else if (pushFwd < 0 && self->client->ps.fd.forceJumpCharge > 200)
	{
		return FJ_BACKWARD;
	}
	else if (pushRt > 0 && self->client->ps.fd.forceJumpCharge > 200)
	{
		return FJ_RIGHT;
	}
	else if (pushRt < 0 && self->client->ps.fd.forceJumpCharge > 200)
	{
		return FJ_LEFT;
	}
	else
	{
		return FJ_UP;
	}
}

void ForceJump(gentity_t * self, usercmd_t * ucmd)
{
	float forceJumpChargeInterval;
	vec3_t	jumpVel;

	if (self->client->ps.fd.forcePowerDuration[FP_LEVITATION] > level.time)
	{
		return;
	}
	if (!WP_ForcePowerUsable(self, FP_LEVITATION))
	{
		return;
	}
	if (self->s.groundEntityNum == ENTITYNUM_NONE)
	{
		return;
	}
	if (self->health <= 0)
	{
		return;
	}

	if (self->client->pers.botclass == BCLASS_MANDOLORIAN
		|| (self->client->pers.botclass == BCLASS_BOBAFETT)
		|| (self->client->pers.botclass == BCLASS_MANDOLORIAN1)
		|| (self->client->pers.botclass == BCLASS_MANDOLORIAN2))
	{
		Boba_FlyStart(self);
	}

	self->client->fjDidJump = qtrue;

	forceJumpChargeInterval = forceJumpStrength[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]] / (FORCE_JUMP_CHARGE_TIME / FRAMETIME);

	WP_GetVelocityForForceJump(self, jumpVel, ucmd);

	//FIXME: sound effect
	self->client->ps.fd.forceJumpZStart = self->client->ps.origin[2];//remember this for when we land
	VectorCopy(jumpVel, self->client->ps.velocity);

	WP_ForcePowerStart(self, FP_LEVITATION, self->client->ps.fd.forceJumpCharge / forceJumpChargeInterval / (FORCE_JUMP_CHARGE_TIME / FRAMETIME) * forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_LEVITATION]][FP_LEVITATION]);
	self->client->ps.fd.forceJumpCharge = 0;
	self->client->ps.forceJumpFlip = qtrue;

	// We've just jumped, we're not gonna be on the ground in the following frames.
	// This makes sure npc's wont trigger the ForceJump function multiple times before detecting that they have left the ground.
	self->client->ps.groundEntityNum = ENTITYNUM_NONE;
}

void WP_AddAsMindtricked(forcedata_t * fd, int entNum)
{
	if (!fd)
	{
		return;
	}

	if (entNum > 47)
	{
		fd->forceMindtrickTargetIndex4 |= (1 << (entNum - 48));
	}
	else if (entNum > 31)
	{
		fd->forceMindtrickTargetIndex3 |= (1 << (entNum - 32));
	}
	else if (entNum > 15)
	{
		fd->forceMindtrickTargetIndex2 |= (1 << (entNum - 16));
	}
	else
	{
		fd->forceMindtrickTargetIndex |= (1 << entNum);
	}
}

qboolean ForceTelepathyCheckDirectNPCTarget(gentity_t * self, trace_t * tr, qboolean * tookPower)
{
	gentity_t* traceEnt;
	qboolean	targetLive = qfalse;
	vec3_t		tfrom, tto, fwd;
	float		radius = MAX_TRICK_DISTANCE;

	//Check for a direct usage on NPCs first
	VectorCopy(self->client->ps.origin, tfrom);
	tfrom[2] += self->client->ps.viewheight;
	AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
	tto[0] = tfrom[0] + fwd[0] * radius / 2;
	tto[1] = tfrom[1] + fwd[1] * radius / 2;
	tto[2] = tfrom[2] + fwd[2] * radius / 2;

	trap->Trace(tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);


	if (tr->entityNum == ENTITYNUM_NONE
		|| tr->fraction == 1.0f
		|| tr->allsolid
		|| tr->startsolid)
	{
		return qfalse;
	}

	traceEnt = &g_entities[tr->entityNum];

	if (traceEnt->NPC
		&& traceEnt->NPC->scriptFlags & SCF_NO_FORCE)
	{
		return qfalse;
	}

	if (traceEnt && traceEnt->client)
	{
		switch (traceEnt->client->NPC_class)
		{
		case CLASS_GALAKMECH://cant grip him, he's in armor
		case CLASS_ATST://much too big to grip!
		//no droids either
		case CLASS_PROBE:
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_MOUSE:
		case CLASS_SEEKER:
		case CLASS_REMOTE:
		case CLASS_PROTOCOL:
		case CLASS_BOBAFETT:
		case CLASS_RANCOR:
			break;
		default:
			targetLive = qtrue;
			break;
		}
	}

	if (traceEnt->s.number < MAX_CLIENTS)
	{//a regular client
		return qfalse;
	}

	if (targetLive && traceEnt->NPC)
	{//hit an organic non-player
		vec3_t	eyeDir;
		if (G_ActivateBehavior(traceEnt, BSET_MINDTRICK))
		{//activated a script on him
			//FIXME: do the visual sparkles effect on their heads, still?
			WP_ForcePowerStart(self, FP_TELEPATHY, 0);
		}
		else if ((self->NPC && traceEnt->client->playerTeam != self->client->playerTeam)
			|| (!self->NPC && traceEnt->client->playerTeam != (npcteam_t)self->client->sess.sessionTeam))
		{//an enemy
			int override = 0;
			if ((traceEnt->NPC->scriptFlags & SCF_NO_MIND_TRICK))
			{
			}
			else if (traceEnt->s.weapon != WP_SABER
				&& traceEnt->client->NPC_class != CLASS_REBORN)
			{//haha!  Jedi aren't easily confused!
				if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_2)
				{//turn them to our side
					//if mind trick 3 and aiming at an enemy need more force power
					if (traceEnt->s.weapon != WP_NONE)
					{//don't charm people who aren't capable of fighting... like ugnaughts and droids
						int newPlayerTeam, newEnemyTeam;

						if (traceEnt->enemy)
						{
							G_ClearEnemy(traceEnt);
						}
						if (traceEnt->NPC)
						{
							//traceEnt->NPC->tempBehavior = BS_FOLLOW_LEADER;
							traceEnt->client->leader = self;
						}
						//FIXME: maybe pick an enemy right here?
						if (self->NPC)
						{//NPC
							newPlayerTeam = self->client->playerTeam;
							newEnemyTeam = self->client->enemyTeam;
						}
						else
						{//client/bot
							if (self->client->sess.sessionTeam == TEAM_BLUE)
							{//rebel
								newPlayerTeam = NPCTEAM_PLAYER;
								newEnemyTeam = NPCTEAM_ENEMY;
							}
							else if (self->client->sess.sessionTeam == TEAM_RED)
							{//imperial
								newPlayerTeam = NPCTEAM_ENEMY;
								newEnemyTeam = NPCTEAM_PLAYER;
							}
							else
							{//neutral - wan't attack anyone
								newPlayerTeam = NPCTEAM_NEUTRAL;
								newEnemyTeam = NPCTEAM_NEUTRAL;
							}
						}
						//store these for retrieval later
						traceEnt->genericValue1 = traceEnt->client->playerTeam;
						traceEnt->genericValue2 = traceEnt->client->enemyTeam;
						traceEnt->genericValue3 = traceEnt->s.teamowner;
						//set the new values
						traceEnt->client->playerTeam = newPlayerTeam;
						traceEnt->client->enemyTeam = newEnemyTeam;
						traceEnt->s.teamowner = newPlayerTeam;
						traceEnt->NPC->charmedTime = level.time + mindTrickTime[self->client->ps.fd.forcePowerLevel[FP_TELEPATHY]];
					}
				}
				else
				{//just confuse them
					//somehow confuse them?  Set don't fire to true for a while?  Drop their aggression?  Maybe just take their enemy away and don't let them pick one up for a while unless shot?
					traceEnt->NPC->confusionTime = level.time + mindTrickTime[self->client->ps.fd.forcePowerLevel[FP_TELEPATHY]];//confused for about 10 seconds
					NPC_PlayConfusionSound(traceEnt);
					if (traceEnt->enemy)
					{
						G_ClearEnemy(traceEnt);
					}
				}
			}
			else
			{
				NPC_Jedi_PlayConfusionSound(traceEnt);
			}
			WP_ForcePowerStart(self, FP_TELEPATHY, override);
		}
		else if (traceEnt->client->playerTeam == self->client->playerTeam)
		{//an ally
			//maybe just have him look at you?  Respond?  Take your enemy?
			if (traceEnt->client->ps.pm_type < PM_DEAD&& traceEnt->NPC != NULL && !(traceEnt->NPC->scriptFlags & SCF_NO_RESPONSE))
			{
				NPC_UseResponse(traceEnt, self, qfalse);
				WP_ForcePowerStart(self, FP_TELEPATHY, 1);
			}
		}//NOTE: no effect on TEAM_NEUTRAL?
		AngleVectors(traceEnt->client->renderInfo.eyeAngles, eyeDir, NULL, NULL);
		VectorNormalize(eyeDir);
		G_PlayEffectID(G_EffectIndex("force/force_touch"), traceEnt->client->renderInfo.eyePoint, eyeDir);
	}
	else
	{
		if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_1&& tr->fraction * 2048 > 64)
		{//don't create a diversion less than 64 from you of if at power level 1
			//use distraction anim instead
			G_PlayEffectID(G_EffectIndex("force/force_touch"), tr->endpos, tr->plane.normal);
			AddSoundEvent(self, tr->endpos, 512, AEL_SUSPICIOUS, qtrue, qtrue);
			AddSightEvent(self, tr->endpos, 512, AEL_SUSPICIOUS, 50);
			WP_ForcePowerStart(self, FP_TELEPATHY, 0);
			*tookPower = qtrue;
		}
	}
	self->client->ps.saberBlocked = BLOCKED_NONE;
	self->client->ps.weaponTime = 1000;
	return qtrue;
}

const int STASIS_TIME = 5000;
const int STASISJEDI_TIME = 2500;
#define STASIS_SPLASH_RAD	500

void ForceTelepathy(gentity_t * self)
{
	trace_t tr;
	vec3_t tto, thispush_org, a;
	vec3_t mins, maxs, fwdangles, forward, right, center;
	int i;
	float visionArc = 0;
	float radius = MAX_TRICK_DISTANCE;
	qboolean	tookPower = qfalse;

	if (self->health <= 0)
	{
		return;
	}

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return;
	}

	if (self->client->ps.powerups[PW_REDFLAG] ||
		self->client->ps.powerups[PW_BLUEFLAG])
	{ //can't mindtrick while carrying the flag
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.fd.forcePowersActive& (1 << FP_TELEPATHY)))
	{
		WP_ForcePowerStop(self, FP_TELEPATHY);
		return;
	}

	if (!WP_ForcePowerUsable(self, FP_TELEPATHY))
	{
		return;
	}

	// fix: rocket lock bug
	BG_ClearRocketLock(&self->client->ps);

	if (ForceTelepathyCheckDirectNPCTarget(self, &tr, &tookPower))
	{//hit an NPC directly
		self->client->ps.forceAllowDeactivateTime = level.time + 1500;
		G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distract.wav"));
		self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
		self->client->ps.forceHandExtendTime = level.time + 1000;
		return;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_2)
	{
		visionArc = 180;
	}
	else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_3)
	{
		visionArc = 360;
		radius = MAX_TRICK_DISTANCE * 2.0f;
	}

	VectorCopy(self->client->ps.viewangles, fwdangles);
	AngleVectors(fwdangles, forward, right, NULL);
	VectorCopy(self->client->ps.origin, center);

	for (i = 0; i < 3; i++)
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] == FORCE_LEVEL_1)
	{
		if (tr.fraction != 1.0 &&
			tr.entityNum != ENTITYNUM_NONE &&
			g_entities[tr.entityNum].inuse &&
			g_entities[tr.entityNum].client &&
			g_entities[tr.entityNum].client->pers.connected &&
			g_entities[tr.entityNum].client->sess.sessionTeam != TEAM_SPECTATOR)
		{
			WP_AddAsMindtricked(&self->client->ps.fd, tr.entityNum);
			if (!tookPower)
			{
				WP_ForcePowerStart(self, FP_TELEPATHY, 0);
			}

			G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distract.wav"));

			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			self->client->ps.forceHandExtendTime = level.time + 1000;

			return;
		}
		else
		{
			return;
		}
	}
	else	//level 2 & 3
	{
		gentity_t* ent;
		int entityList[MAX_GENTITIES];
		int numListedEntities;
		int e = 0;
		qboolean gotatleastone = qfalse;
		vec3_t	dir = { 0,0,1 };

		numListedEntities = trap->EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

		while (e < numListedEntities)
		{
			ent = &g_entities[entityList[e]];

			if (ent)
			{ //not in the arc, don't consider it
				if (ent->client)
				{
					VectorCopy(ent->client->ps.origin, thispush_org);
				}
				else
				{
					VectorCopy(ent->s.pos.trBase, thispush_org);
				}
				VectorCopy(self->client->ps.origin, tto);
				tto[2] += self->client->ps.viewheight;
				VectorSubtract(thispush_org, tto, a);
				vectoangles(a, a);

				if (!ent->client)
				{
					entityList[e] = ENTITYNUM_NONE;
				}
				else if (!InFieldOfVision(self->client->ps.viewangles, visionArc, a))
				{ //only bother with arc rules if the victim is a client
					entityList[e] = ENTITYNUM_NONE;
				}
				else if (!ForcePowerUsableOn(self, ent, FP_TELEPATHY))
				{
					entityList[e] = ENTITYNUM_NONE;
				}
				else if (OnSameTeam(self, ent))
				{
					entityList[e] = ENTITYNUM_NONE;
				}
			}
			ent = &g_entities[entityList[e]];

			if (ent && ent != self && ent->client)
			{
				gotatleastone = qtrue;
				WP_AddAsMindtricked(&self->client->ps.fd, ent->s.number);

				if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_2 && ent->s.weapon != WP_SABER)
				{
					ent->client->frozenTime = level.time + STASIS_TIME;
					ent->client->ps.userInt3 |= (1 << FLAG_FROZEN);
					ent->client->ps.userInt1 |= LOCK_UP;
					ent->client->ps.userInt1 |= LOCK_DOWN;
					ent->client->ps.userInt1 |= LOCK_RIGHT;
					ent->client->ps.userInt1 |= LOCK_LEFT;
					ent->client->viewLockTime = level.time + STASIS_TIME;
					ent->client->ps.legsTimer = ent->client->ps.torsoTimer = level.time + STASIS_TIME;
					G_SetAnim(ent, NULL, SETANIM_BOTH, WeaponReadyAnim[ent->client->ps.weapon], SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 100);
					G_AddEvent(ent, EV_STASIS, DirToByte(dir));
					G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/stasis.wav"));
					ent->client->ps.saberMove = ent->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
					ent->client->ps.saberBlocked = BLOCKED_NONE;
					player_Freeze(ent);
				}
				else if (self->client->ps.fd.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_2 && ent->s.weapon == WP_SABER && ent->client->ps.fd.forcePower <= 75)
				{
					ent->client->frozenTime = level.time + STASISJEDI_TIME;
					ent->client->ps.userInt3 |= (1 << FLAG_FROZEN);
					ent->client->ps.userInt1 |= LOCK_UP;
					ent->client->ps.userInt1 |= LOCK_DOWN;
					ent->client->ps.userInt1 |= LOCK_RIGHT;
					ent->client->ps.userInt1 |= LOCK_LEFT;
					ent->client->viewLockTime = level.time + STASISJEDI_TIME;
					ent->client->ps.legsTimer = ent->client->ps.torsoTimer = level.time + STASISJEDI_TIME;
					G_SetAnim(ent, NULL, SETANIM_BOTH, WeaponReadyAnim[ent->client->ps.weapon], SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 100);
					G_AddEvent(ent, EV_STASIS, DirToByte(dir));
					G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/stasis.wav"));
					ent->client->ps.saberMove = ent->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
					ent->client->ps.saberBlocked = BLOCKED_NONE;
					player_Freeze(ent);
				}
				else
				{
					if (ent->client->ps.userInt3 |= (1 << FLAG_FROZEN))
					{
						ent->client->ps.userInt3 &= ~(1 << FLAG_FROZEN);
					}
					Player_CheckFreeze(ent);
				}
			}
			e++;
		}

		if (gotatleastone)
		{
			self->client->ps.forceAllowDeactivateTime = level.time + 1500;

			if (!tookPower)
			{
				WP_ForcePowerStart(self, FP_TELEPATHY, 0);
			}

			G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distract.wav"));

			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			self->client->ps.forceHandExtendTime = level.time + 1000;
		}
	}

}

void GEntity_UseFunc(gentity_t * self, gentity_t * other, gentity_t * activator)
{
	GlobalUse(self, other, activator);
}

qboolean CanCounterThrow(gentity_t * self, gentity_t * thrower, qboolean pull)
{
	int powerUse = 0;

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return 0;
	}

	if (self->client->ps.weaponTime > 0)
	{
		return 0;
	}

	if (self->health <= 0)
	{
		return 0;
	}

	if (self->client->ps.powerups[PW_DISINT_4] > level.time)
	{
		return 0;
	}

	if (!WP_CounterForce(thrower, self, (pull ? FP_PULL : FP_PUSH)))
	{//wasn't able to counter due to generic counter issue
		return 0;
	}

	if (!InFront(thrower->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.0f))
	{//not facing the attacker 
		return 0;
	}

	if (self->client->ps.weaponstate == WEAPON_CHARGING ||
		self->client->ps.weaponstate == WEAPON_CHARGING_ALT)
	{ //don't autodefend when charging a weapon
		return 0;
	}

	if (level.gametype == GT_SIEGE &&
		pull &&
		thrower && thrower->client)
	{ //in siege, pull will affect people if they are not facing you, so they can't run away so much
		vec3_t d;
		float a;

		VectorSubtract(thrower->client->ps.origin, self->client->ps.origin, d);
		vectoangles(d, d);

		a = AngleSubtract(d[YAW], self->client->ps.viewangles[YAW]);

		if (a > 60.0f || a < -60.0f)
		{ //if facing more than 60 degrees away they cannot defend
			return 0;
		}
	}

	if (pull)
	{
		powerUse = FP_PULL;
	}
	else
	{
		powerUse = FP_PUSH;
	}

	if (!WP_ForcePowerUsable(self, powerUse))
	{
		return 0;
	}

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{ //you cannot counter a push/pull if you're in the air
		return 0;
	}

	return 1;
}

qboolean G_InGetUpAnim(playerState_t * ps)
{
	switch ((ps->legsAnim))
	{
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
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		return qtrue;
	}

	switch ((ps->torsoAnim))
	{
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
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		return qtrue;
	}

	return qfalse;
}

void G_LetGoOfWall(gentity_t * ent)
{
	if (!ent || !ent->client)
	{
		return;
	}
	ent->client->ps.pm_flags &= ~PMF_STUCK_TO_WALL;
	if (BG_InReboundJump(ent->client->ps.legsAnim)
		|| BG_InReboundHold(ent->client->ps.legsAnim))
	{
		ent->client->ps.legsTimer = 0;
	}
	if (BG_InReboundJump(ent->client->ps.torsoAnim)
		|| BG_InReboundHold(ent->client->ps.torsoAnim))
	{
		ent->client->ps.torsoTimer = 0;
	}
}

float forcePushPullRadius[NUM_FORCE_POWER_LEVELS] =
{
	0,//none
	384,//256,
	448,//384,
	512
};

void WP_ResistForcePush(gentity_t* self, gentity_t* pusher, qboolean noPenalty)
{
	int parts;
	qboolean runningResist = qfalse;

	if (!self || self->health <= 0 || !self->client || !pusher || !pusher->client)
	{
		return;
	}

	if (!PM_SaberCanInterruptMove(self->client->ps.saberMove, self->client->ps.torsoAnim))
	{//can't interrupt my current torso anim/sabermove with this, so ignore it entirely!
		return;
	}

	if ((!self->s.number
		|| (self->NPC && (self->NPC->aiFlags & NPCAI_BOSS_CHARACTER))
		|| (self->client && self->client->NPC_class == CLASS_SHADOWTROOPER))
		&& (VectorLengthSquared(self->client->ps.velocity) > 10000 || self->client->ps.fd.forcePowerLevel[FP_PUSH] >= FORCE_LEVEL_3 || self->client->ps.fd.forcePowerLevel[FP_PULL] >= FORCE_LEVEL_3))
	{
		runningResist = qtrue;
	}

	if (!runningResist
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE
		&& !PM_SpinningSaberAnim(self->client->ps.legsAnim)
		&& !PM_FlippingAnim(self->client->ps.legsAnim)
		&& !PM_RollingAnim(self->client->ps.legsAnim)
		&& !PM_InKnockDown(&self->client->ps)
		&& !PM_CrouchAnim(self->client->ps.legsAnim))
	{//if on a surface and not in a spin or flip, play full body resist
		parts = SETANIM_BOTH;
	}
	else
	{//play resist just in torso
		parts = SETANIM_TORSO;
	}

	if (self->client->ps.fd.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_2)
	{
		NPC_SetAnim(self, parts, BOTH_RESISTPUSH, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	else if (self->client->saber[0].type == SABER_YODA)
	{
		NPC_SetAnim(self, parts, BOTH_YODA_RESISTFORCE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	else
	{
		NPC_SetAnim(self, parts, BOTH_RESISTPUSH2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}

	if (!noPenalty)
	{
		char buf[128];
		float tFVal = 0;

		trap->Cvar_VariableStringBuffer("timescale", buf, sizeof(buf));

		tFVal = atof(buf);

		if (!runningResist)
		{
			VectorClear(self->client->ps.velocity);
			self->client->ps.weaponTime = 1000;
			if (self->client->ps.fd.forcePowersActive & (1 << FP_SPEED))
			{
				self->client->ps.weaponTime = floor(self->client->ps.weaponTime * tFVal);
			}
			self->client->ps.pm_time = self->client->ps.weaponTime;
			self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			//play the full body push effect on me
			self->forcePushTime = level.time + 600; // let the push effect last for 600 ms
		}
		else
		{
			self->client->ps.weaponTime = 600;
			if (self->client->ps.fd.forcePowersActive & (1 << FP_SPEED))
			{
				self->client->ps.weaponTime = floor(self->client->ps.weaponTime * tFVal);
			}
		}
	}
	//play my force push effect on my hand
	self->client->ps.powerups[PW_DISINT_4] = level.time + self->client->ps.torsoTimer + 500;
	self->client->ps.powerups[PW_PULL] = 0;
	Jedi_PlayBlockedPushSound(self);
}

void ForceThrow(gentity_t * self, qboolean pull)
{
	//shove things in front of you away
	float		dist;
	gentity_t* ent;
	int			entityList[MAX_GENTITIES];
	gentity_t* push_list[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	int			i, e;
	int			ent_count = 0;
	int			radius = 1024; //since it's view-based now. //350;
	int			powerLevel;
	int			visionArc;
	int			pushPower;
	int			pushPowerMod;
	vec3_t		center, ent_org, size, forward, right, end, dir, fwdangles = { 0 };
	float		dot1, cone;
	trace_t		tr;
	int			x;
	vec3_t		pushDir;
	vec3_t		thispush_org;
	vec3_t		tfrom, tto, fwd, a;
	int			powerUse = 0;
	gentity_t* aimingAt;
	qboolean iGrip = qfalse;

	saberInfo_t* saber1;

	saber1 = BG_MySaber(self->clientNum, 0);

	visionArc = 0;

	if (self->client->ps.forceHandExtend != HANDEXTEND_NONE && (self->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN || !G_InGetUpAnim(&self->client->ps)) && !(self->client->ps.fd.forcePowersActive & (1 << FP_GRIP)))
	{
		return;
	}

	if (self->client->ps.userInt3 & (1 << FLAG_PREBLOCK))
	{
		return;
	}

	//allow push/pull during preblocks
	if (self->client->ps.weaponTime > 0 && (!PM_SaberInParry(self->client->ps.saberMove) || !(self->client->ps.userInt3 & (1 << FLAG_PREBLOCK))))
	{
		return;
	}

	if (self->health <= 0)
	{
		return;
	}

	if (pull)
	{
		powerUse = FP_PULL;
		cone = forcePullCone[self->client->ps.fd.forcePowerLevel[FP_PULL]];
	}
	else
	{
		powerUse = FP_PUSH;
		cone = forcePushCone[self->client->ps.fd.forcePowerLevel[FP_PUSH]];
	}

	if (!WP_ForcePowerUsable(self, powerUse))
	{
		return;
	}

	if (self->client->ps.fd.forcePowersActive & (1 << FP_GRIP))
	{
		WP_ForcePowerStop(self, FP_GRIP);
		iGrip = qtrue;
	}

	if (!pull && self->client->ps.saberLockTime > level.time && self->client->ps.saberLockFrame)
	{
		if (saber1->type == SABER_UNSTABLE//saber kylo
			|| saber1->type == SABER_STAFF_UNSTABLE
			|| saber1->type == SABER_BACKHAND
			|| saber1->type == SABER_ASBACKHAND
			|| saber1->type == SABER_ANAKIN
			|| saber1->type == SABER_PALP
			|| saber1->type == SABER_DOOKU
			|| saber1->type == SABER_YODA)//saber yoda
		{
			G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/push.mp3"));
		}
		else
		{
			if (self->client->ps.fd.forcePower < 50)
			{
				G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/pushyoda.mp3"));
			}
			else
			{
				G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/pushlow.mp3"));
			}
		}
		self->client->ps.powerups[PW_DISINT_4] = level.time + 1500;

		self->client->ps.saberLockHits += self->client->ps.fd.forcePowerLevel[FP_PUSH] * 2;

		WP_ForcePowerStart(self, FP_PUSH, 0);
		return;
	}

	if (self->client->ps.legsAnim == BOTH_KNOCKDOWN3
		|| (self->client->ps.torsoAnim == BOTH_FORCE_GETUP_F1 && self->client->ps.torsoTimer > 400)
		|| (self->client->ps.torsoAnim == BOTH_FORCE_GETUP_F2 && self->client->ps.torsoTimer > 900)
		|| (self->client->ps.torsoAnim == BOTH_GETUP3 && self->client->ps.torsoTimer > 500)
		|| (self->client->ps.torsoAnim == BOTH_GETUP4 && self->client->ps.torsoTimer > 300)
		|| (self->client->ps.torsoAnim == BOTH_GETUP5 && self->client->ps.torsoTimer > 500))
	{//we're face-down, so we'd only be force-push/pulling the floor
		return;
	}

	WP_ForcePowerStart(self, powerUse, 0);

	//make sure this plays and that you cannot press fire for about 1 second after this
	if (pull)
	{//RACC - force pull
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/pull.wav"));
		if (self->client->ps.forceHandExtend == HANDEXTEND_NONE)
		{
			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPULL;
			self->client->ps.forceHandExtendTime = level.time + 600;
		}
		self->client->ps.powerups[PW_DISINT_4] = self->client->ps.forceHandExtendTime + 200;
		self->client->ps.powerups[PW_PULL] = self->client->ps.powerups[PW_DISINT_4];
	}
	else
	{//RACC - Force Push
		if (self->client->ps.weapon == WP_MELEE ||
			self->client->ps.weapon == WP_NONE ||
			self->client->ps.weapon == WP_SABER && self->client->ps.saberHolstered)
		{//2-handed PUSH
			if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
			{
				G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/repulsepush.wav"));
			}
			else
			{
				G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/pushhard.mp3"));
			}
		}
		else
		{
			if (saber1->type == SABER_UNSTABLE//saber kylo
				|| saber1->type == SABER_STAFF_UNSTABLE
				|| saber1->type == SABER_BACKHAND
				|| saber1->type == SABER_ASBACKHAND
				|| saber1->type == SABER_ANAKIN
				|| saber1->type == SABER_PALP
				|| saber1->type == SABER_DOOKU)//saber yoda
			{
				G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/push.mp3"));
			}
			else if (saber1->type == SABER_YODA)//saber yoda
			{
				G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/pushyoda.mp3"));
			}
			else
			{
				if (self->client->ps.fd.forcePower < 50)
				{
					G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/pushyoda.mp3"));
				}
				else
				{
					G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/pushlow.mp3"));
				}
			}
		}
		if (self->client->ps.forceHandExtend == HANDEXTEND_NONE)
		{
			self->client->ps.forceHandExtend = HANDEXTEND_FORCEPUSH;
			self->client->ps.forceHandExtendTime = level.time + 650;
		}
		else if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN && G_InGetUpAnim(&self->client->ps))
		{//RACC - Force Pushed while in get up animation.
			if (self->client->ps.forceDodgeAnim > 4)
			{
				self->client->ps.forceDodgeAnim -= 8;
			}
			self->client->ps.forceDodgeAnim += 8; //special case, play push on upper torso, but keep playing current knockdown anim on legs
		}
		self->client->ps.powerups[PW_DISINT_4] = level.time + 1100;
		self->client->ps.powerups[PW_PULL] = 0;
	}

	VectorCopy(self->client->ps.viewangles, fwdangles);
	AngleVectors(fwdangles, forward, right, NULL);
	VectorCopy(self->client->ps.origin, center);

	for (i = 0; i < 3; i++)
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}


	if (pull)
	{
		powerLevel = self->client->ps.fd.forcePowerLevel[FP_PULL];
		pushPower = 256 * self->client->ps.fd.forcePowerLevel[FP_PULL];
	}
	else
	{
		powerLevel = self->client->ps.fd.forcePowerLevel[FP_PUSH];
		pushPower = 256 * self->client->ps.fd.forcePowerLevel[FP_PUSH];
	}

	if (!powerLevel)
	{ //Shouldn't have made it here..
		return;
	}

	if (powerLevel == FORCE_LEVEL_2)
	{
		visionArc = 60;
	}
	else if (powerLevel == FORCE_LEVEL_3)
	{
		visionArc = 180;
	}

	if (powerLevel == FORCE_LEVEL_1)
	{ //can only push/pull targeted things at level 1
		VectorCopy(self->client->ps.origin, tfrom);
		tfrom[2] += self->client->ps.viewheight;
		AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
		tto[0] = tfrom[0] + fwd[0] * radius / 2;
		tto[1] = tfrom[1] + fwd[1] * radius / 2;
		tto[2] = tfrom[2] + fwd[2] * radius / 2;

		trap->Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID | CONTENTS_TRIGGER, qfalse, 0, 0);

		if (tr.fraction != 1.0 &&
			tr.entityNum != ENTITYNUM_NONE)
		{
			if (!g_entities[tr.entityNum].client&& g_entities[tr.entityNum].s.eType == ET_NPC)
			{ //g2animent
				if (g_entities[tr.entityNum].s.genericenemyindex < level.time)
				{
					g_entities[tr.entityNum].s.genericenemyindex = level.time + 2000;
				}
			}
			numListedEntities = 0;
			entityList[numListedEntities] = tr.entityNum;

			if (pull)
			{
				if (!ForcePowerUsableOn(self, &g_entities[tr.entityNum], FP_PULL))
				{
					return;
				}
			}
			else
			{
				if (!ForcePowerUsableOn(self, &g_entities[tr.entityNum], FP_PUSH))
				{
					return;
				}
			}

			// if a CONTENTS_TRIGGER was traced, must be an item
			if (g_entities[tr.entityNum].r.contents == CONTENTS_TRIGGER && g_entities[tr.entityNum].s.eType != ET_ITEM)
				return;

			numListedEntities++;
		}
		else
		{
			//didn't get anything, so just
			return;
		}

		//now check for brush based force moveable objects.
		{
			int i;
			int funcNum;
			int	funcList[MAX_GENTITIES];
			gentity_t* funcEnt = NULL;

			funcNum = trap->EntitiesInBox(mins, maxs, funcList, MAX_GENTITIES);

			if (numListedEntities <= 0)
				return;

			for (i = 0; i < funcNum; i++)
			{
				funcEnt = &g_entities[funcList[i]];
				if (!funcEnt->client && funcEnt->s.number != tr.entityNum)
				{//we have one, add it to the actual push list.
					entityList[numListedEntities] = funcEnt->s.number;
					numListedEntities++;
				}
			}
		}
	}
	else
	{
		numListedEntities = trap->EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

		e = 0;

		while (e < numListedEntities)
		{
			ent = &g_entities[entityList[e]];

			if (!ent->client && ent->s.eType == ET_NPC)
			{ //g2animent
				if (ent->s.genericenemyindex < level.time)
				{
					ent->s.genericenemyindex = level.time + 2000;
				}
			}
			if (Q_stricmp(ent->classname, "sentryGun") == 0)
			{
				VectorCopy(self->client->ps.origin, tfrom);
				tfrom[2] += self->client->ps.viewheight;
				AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
				tto[0] = tfrom[0] + fwd[0] * radius / 2;
				tto[1] = tfrom[1] + fwd[1] * radius / 2;
				tto[2] = tfrom[2] + fwd[2] * radius / 2;

				trap->Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);

				if (tr.fraction != 1.0 &&
					tr.entityNum != ENTITYNUM_NONE)
				{
					if (!g_entities[tr.entityNum].client&& g_entities[tr.entityNum].s.eType == ET_NPC)
					{ //g2animent
						if (g_entities[tr.entityNum].s.genericenemyindex < level.time)
						{
							g_entities[tr.entityNum].s.genericenemyindex = level.time + 2000;
						}
					}
					aimingAt = &g_entities[tr.entityNum];
					if (ent == aimingAt)//Looking at the sentry...so deactivate for 10 seconds
						ent->nextthink = level.time + 5000;
				}
			}

			if (ent)
			{
				if (ent->client)
				{
					VectorCopy(ent->client->ps.origin, thispush_org);
				}
				else
				{
					VectorCopy(ent->s.pos.trBase, thispush_org);
				}
			}

			if (ent)
			{ //not in the arc, don't consider it
				VectorCopy(self->client->ps.origin, tto);
				tto[2] += self->client->ps.viewheight;
				VectorSubtract(thispush_org, tto, a);
				vectoangles(a, a);

				if (ent->client && !InFieldOfVision(self->client->ps.viewangles, visionArc, a) &&
					ForcePowerUsableOn(self, ent, powerUse))
				{ //only bother with arc rules if the victim is a client
					entityList[e] = ENTITYNUM_NONE;
				}
				else if (ent->client)
				{
					if (pull)
					{
						if (!ForcePowerUsableOn(self, ent, FP_PULL))
						{
							entityList[e] = ENTITYNUM_NONE;
						}
					}
					else
					{
						if (!ForcePowerUsableOn(self, ent, FP_PUSH))
						{
							entityList[e] = ENTITYNUM_NONE;
						}
					}
				}
			}
			e++;
		}
	}

	for (e = 0; e < numListedEntities; e++)
	{
		if (entityList[e] != ENTITYNUM_NONE &&
			entityList[e] >= 0 &&
			entityList[e] < MAX_GENTITIES)
		{
			ent = &g_entities[entityList[e]];
		}
		else
		{
			ent = NULL;
		}

		if (!ent)
			continue;
		if (ent == self)
			continue;
		if (ent->client && OnSameTeam(ent, self))
		{
			continue;
		}
		if (!(ent->inuse))
			continue;
		if (ent->s.eType != ET_MISSILE)
		{
			if (g_pushitems.integer || ent->s.eType != ET_ITEM)
			{
				//FIXME: need pushable objects
				if (Q_stricmp("func_button", ent->classname) == 0)
				{//we might push it
					if (pull || !(ent->spawnflags & SPF_BUTTON_FPUSHABLE))
					{//not force-pushable, never pullable
						continue;
					}
				}
				else
				{
					if (ent->s.eFlags & EF_NODRAW)
					{
						continue;
					}
					if (!ent->client)
					{
						if (!g_pushitems.integer)
						{
							if (Q_stricmp("lightsaber", ent->classname) != 0)
							{//not a lightsaber 
								if (Q_stricmp("func_door", ent->classname) != 0 || !(ent->spawnflags & 2))
								{//not a force-usable door
									if (Q_stricmp("func_static", ent->classname) != 0 || (!(ent->spawnflags & 1) && !(ent->spawnflags & 2)))
									{//not a force-usable func_static
										if (Q_stricmp("limb", ent->classname))
										{//not a limb
											continue;
										}
									}
								}
								else if (ent->moverState != MOVER_POS1 && ent->moverState != MOVER_POS2)
								{//not at rest
									continue;
								}
							}
						}
					}
					else if (ent->client->NPC_class == CLASS_GALAKMECH
						|| ent->client->NPC_class == CLASS_ATST
						|| ent->client->NPC_class == CLASS_SBD
						|| ent->client->NPC_class == CLASS_RANCOR
						|| ent->client->NPC_class == CLASS_WAMPA
						|| ent->client->NPC_class == CLASS_SAND_CREATURE)
					{//can't push ATST or Galak or Rancor
						continue;
					}
				}
			}
		}
		else
		{//RACC - Missiles
			if (ent->s.pos.trType == TR_STATIONARY && (ent->s.eFlags & EF_MISSILE_STICK))
			{//can't force-push/pull stuck missiles (detpacks, tripmines)
				continue;
			}
			if (ent->s.pos.trType == TR_STATIONARY && ent->s.weapon != WP_THERMAL)
			{//only thermal detonators can be pushed once stopped
				continue;
			}
		}

		//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
		// find the distance from the edge of the bounding box
		for (i = 0; i < 3; i++)
		{
			if (center[i] < ent->r.absmin[i])
			{
				v[i] = ent->r.absmin[i] - center[i];
			}
			else if (center[i] > ent->r.absmax[i])
			{
				v[i] = center[i] - ent->r.absmax[i];
			}
			else
			{
				v[i] = 0;
			}
		}

		VectorSubtract(ent->r.absmax, ent->r.absmin, size);
		VectorMA(ent->r.absmin, 0.5, size, ent_org);

		VectorSubtract(ent_org, center, dir);
		VectorNormalize(dir);

		if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			if (cone < 1.0f)
			{//must be within the forward cone
				if (ent->client && !pull
					&& ent->client->ps.fd.forceGripEntityNum == self->s.number)
				{//this is the guy that's force-gripping me, use a wider cone regardless of force power level
					if ((dot1 = DotProduct(dir, forward)) < cone - 0.3f)
						continue;
				}
				else if (ent->s.eType == ET_MISSILE)
				{//missiles are easier to force-push, never require direct trace (FIXME: maybe also items and general physics objects)
					if ((dot1 = DotProduct(dir, forward)) < cone - 0.3f)
						continue;
				}
				else if ((dot1 = DotProduct(dir, forward)) < cone)
				{
					continue;
				}
			}
			else if (ent->s.eType == ET_MISSILE)
			{//a missile and we're at force level 1... just use a small cone, but not ridiculously small
				if ((dot1 = DotProduct(dir, forward)) < 0.75f)
				{
					continue;
				}
			}
		}

		dist = VectorLength(v);

		if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{//if within a certain range, deflect it
			if (ent->s.eType == ET_MISSILE && cone >= 1.0f)
			{//smaller radius on missile checks at force push 1
				if (dist >= 192)
				{
					continue;
				}
			}
			else if (dist >= radius)
			{
				continue;
			}
		}
		else if (dist >= radius)
		{
			continue;
		}

		//in PVS?
		if (!ent->r.bmodel && !trap->InPVS(ent_org, self->client->ps.origin))
		{//must be in PVS
			continue;
		}

		//really should have a clear LOS to this thing...
		trap->Trace(&tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT, qfalse, 0, 0);
		if (tr.fraction < 1.0f&& tr.entityNum != ent->s.number)
		{//must have clear LOS
		 //try from eyes too before you give up
			vec3_t eyePoint;
			VectorCopy(self->client->ps.origin, eyePoint);
			eyePoint[2] += self->client->ps.viewheight;
			trap->Trace(&tr, eyePoint, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT, qfalse, 0, 0);

			if (tr.fraction < 1.0f&& tr.entityNum != ent->s.number)
			{
				continue;
			}
		}

		// ok, we are within the radius, add us to the incoming list
		push_list[ent_count] = ent;
		ent_count++;
	}

	if (ent_count)
	{
		//method1:
		for (x = 0; x < ent_count; x++)
		{
			int modPowerLevel = powerLevel;

			pushPower = 256 * modPowerLevel;

			if (push_list[x]->client)
			{
				VectorCopy(push_list[x]->client->ps.origin, thispush_org);
			}
			else
			{
				VectorCopy(push_list[x]->s.origin, thispush_org);
			}

			if (push_list[x]->client)
			{
				qboolean wasWallGrabbing = qfalse;
				qboolean canPullWeapon = qtrue;
				float dirLen = 0;

				if ((push_list[x]->client->ps.pm_flags& PMF_STUCK_TO_WALL))
				{//no resistance if stuck to wall
					wasWallGrabbing = qtrue;
					G_LetGoOfWall(push_list[x]);
				}

				pushPowerMod = pushPower;

				//switched to more logical wasWallGrabbing toggle.
				if (!wasWallGrabbing && CanCounterThrow(push_list[x], self, pull))
				{//racc - player blocked the throw.
					if (pull)
					{
						G_Sound(push_list[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pull.wav"));
						push_list[x]->client->ps.forceHandExtendTime = level.time + 600;
					}
					else
					{
						if (self->client->ps.weapon == WP_MELEE ||
							self->client->ps.weapon == WP_NONE ||
							self->client->ps.weapon == WP_SABER && self->client->ps.saberHolstered)
						{//2-handed PUSH
							if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
							{
								G_Sound(push_list[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/repulsepush.wav"));
							}
							else
							{
								G_Sound(push_list[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pushhard.mp3"));
							}
						}
						else
						{
							if (saber1->type == SABER_UNSTABLE//saber kylo
								|| saber1->type == SABER_STAFF_UNSTABLE
								|| saber1->type == SABER_BACKHAND
								|| saber1->type == SABER_ASBACKHAND
								|| saber1->type == SABER_ANAKIN
								|| saber1->type == SABER_PALP
								|| saber1->type == SABER_DOOKU)
							{
								G_Sound(push_list[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/push.mp3"));
							}
							else if (saber1->type == SABER_YODA)//saber yoda
							{
								G_Sound(push_list[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pushyoda.mp3"));
							}
							else
							{
								G_Sound(push_list[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pushlow.mp3"));
							}
						}
						push_list[x]->client->ps.forceHandExtendTime = level.time + 650;
					}
					push_list[x]->client->ps.powerups[PW_DISINT_4] = push_list[x]->client->ps.forceHandExtendTime + 200;

					if (pull)
					{
						push_list[x]->client->ps.powerups[PW_PULL] = push_list[x]->client->ps.powerups[PW_DISINT_4];
					}
					else
					{
						push_list[x]->client->ps.powerups[PW_PULL] = 0;
					}

					//Make a counter-throw effect
					WP_ResistForcePush(push_list[x], self, qfalse);
					continue;
				}

				//shove them
				if (pull)
				{
					VectorSubtract(self->client->ps.origin, thispush_org, pushDir);

					if (push_list[x]->client && VectorLength(pushDir) <= 256)
					{
						if (!OnSameTeam(self, push_list[x]) && push_list[x]->client->ps.saberAttackChainCount < MISHAPLEVEL_HEAVY)
						{
							canPullWeapon = qfalse;
						}
						if (!OnSameTeam(self, push_list[x]) && canPullWeapon)
						{// pull the weapon out of the player's hand.
							vec3_t uorg, vecnorm;
							VectorCopy(self->client->ps.origin, tfrom);
							tfrom[2] += self->client->ps.viewheight;
							AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
							tto[0] = tfrom[0] + fwd[0] * radius / 2;
							tto[1] = tfrom[1] + fwd[1] * radius / 2;
							tto[2] = tfrom[2] + fwd[2] * radius / 2;

							trap->Trace(&tr, tfrom, NULL, NULL, tto, self->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);

							if (tr.fraction != 1.0
								&& tr.entityNum == push_list[x]->s.number)
							{
								VectorCopy(self->client->ps.origin, uorg);
								uorg[2] += 64;

								VectorSubtract(uorg, thispush_org, vecnorm);
								VectorNormalize(vecnorm);

								TossClientWeapon(push_list[x], vecnorm, 500);
							}
						}
					}
				}
				else
				{
					VectorSubtract(thispush_org, self->client->ps.origin, pushDir);
				}

				if (!dirLen)
				{
					dirLen = VectorLength(pushDir);
				}

				VectorNormalize(pushDir);

				if (push_list[x]->client)
				{//escape a force grip if we're in one
					if (self->client->ps.fd.forceGripBeingGripped > level.time)
					{ //force the enemy to stop gripping me if I managed to push him
						if (push_list[x]->client->ps.fd.forceGripEntityNum == self->s.number)
						{
							WP_ForcePowerStop(push_list[x], FP_GRIP);
							self->client->ps.fd.forceGripBeingGripped = 0;
							push_list[x]->client->ps.fd.forceGripUseTime = level.time + 1000; //since we just broke out of it..
						}
					}

					push_list[x]->client->ps.otherKiller = self->s.number;
					push_list[x]->client->ps.otherKillerTime = level.time + 5000;
					push_list[x]->client->ps.otherKillerDebounceTime = level.time + 100;
					push_list[x]->client->otherKillerMOD = MOD_UNKNOWN;
					push_list[x]->client->otherKillerVehWeapon = 0;
					push_list[x]->client->otherKillerWeaponType = WP_NONE;

					pushPowerMod -= (dirLen * 0.7);

					if (pushPowerMod < 16)
					{
						pushPowerMod = 16;
					}

					if (pushPowerMod > 250)
					{//got pushed hard, get knockdowned or knocked off a animals or speeders if riding one.
						if (BG_KnockDownable(&push_list[x]->client->ps) && dirLen <= 128.0f)
						{ //can only do a knockdown if running
							G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
						}
						else if (push_list[x]->client->ps.m_iVehicleNum && dirLen <= 128.0f)
						{ //a player on a vehicle
							gentity_t* vehEnt = &g_entities[push_list[x]->client->ps.m_iVehicleNum];

							if (vehEnt->inuse&& vehEnt->client&& vehEnt->m_pVehicle)
							{
								if (vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER ||
									vehEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
								{ //push the guy off
									vehEnt->m_pVehicle->m_pVehicleInfo->Eject(vehEnt->m_pVehicle, (bgEntity_t*)push_list[x], qfalse);
								}
							}
						}
					}
					//fullbody push effect
					push_list[x]->client->pushEffectTime = level.time + 600;
					G_Sound(push_list[x], CHAN_BODY, G_SoundIndex("sound/weapons/force/pushed.mp3"));

					if (!pull)
					{
						if ((WalkCheck(push_list[x])
							|| (push_list[x]->client->ps.fd.blockPoints >= BLOCKPOINTS_HALF && push_list[x]->client->pers.cmd.buttons & BUTTON_BLOCK)
							|| (push_list[x]->client->ps.BlasterAttackChainCount <= BLASTERMISHAPLEVEL_HEAVYER && !BG_IsUsingHeavyWeap(&push_list[x]->client->ps))
							&& InFront(push_list[x]->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, -0.7f)))
						{
							WP_ResistForcePush(push_list[x], self, qfalse);
							push_list[x]->client->ps.saberMove = push_list[x]->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
							push_list[x]->client->ps.saberBlocked = BLOCKED_NONE;
						}
						else if (PM_SaberInBrokenParry(push_list[x]->client->ps.saberMove) && dirLen <= 64.0f )
						{ //do a knockdown if fairly close
							G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
							pushPowerMod = 200;
						}
						else if (BG_KnockDownable(&push_list[x]->client->ps) && dirLen <= 128.0f && !WalkCheck(push_list[x]))
						{ //can only do a knockdown if fairly close
							G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
							pushPowerMod = 250;
						}
						else if (!InFront(push_list[x]->r.currentOrigin, self->r.currentOrigin, self->client->ps.viewangles, 0.3f) && !WalkCheck(push_list[x]))
						{ 
							G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
							pushPowerMod = 200;
						}
						else
						{
							G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
							pushPowerMod = 200;
						}
					}
					else if (pull)
					{
						if ((WalkCheck(push_list[x])
							|| (push_list[x]->client->ps.fd.blockPoints >= BLOCKPOINTS_HALF && push_list[x]->client->pers.cmd.buttons & BUTTON_BLOCK)
							|| (push_list[x]->client->ps.BlasterAttackChainCount <= BLASTERMISHAPLEVEL_HEAVYER && !BG_IsUsingHeavyWeap(&push_list[x]->client->ps))
							&& InFront(push_list[x]->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, -0.7f)))
						{
							WP_ResistForcePush(push_list[x], self, qfalse);
							push_list[x]->client->ps.saberMove = push_list[x]->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
							push_list[x]->client->ps.saberBlocked = BLOCKED_NONE;
						}
						else if (PM_SaberInBrokenParry(push_list[x]->client->ps.saberMove) && dirLen <= 64.0f)
						{ //do a knockdown if fairly close
							G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
							pushPowerMod = 200;
						}
						else if (BG_KnockDownable(&push_list[x]->client->ps) && dirLen <= 128.0f && !WalkCheck(push_list[x]))
						{ //can only do a knockdown if fairly close
							G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
							pushPowerMod = 250;
						}
						else if (!InFront(push_list[x]->r.currentOrigin, self->r.currentOrigin, self->client->ps.viewangles, 0.3f) && !WalkCheck(push_list[x]))
						{
							G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
							pushPowerMod = 200;
						}
						else
						{
							G_Knockdown(push_list[x], self, pushDir, 300, qtrue);
							pushPowerMod = 200;
						}
					}

					if (iGrip)
					{
						pushPowerMod *= 2;
					}

					push_list[x]->client->ps.velocity[0] = pushDir[0] * pushPowerMod;
					push_list[x]->client->ps.velocity[1] = pushDir[1] * pushPowerMod;

					if ((int)push_list[x]->client->ps.velocity[2] == 0)
					{ //if not going anywhere vertically, boost them up a bit
						push_list[x]->client->ps.velocity[2] = pushDir[2] * pushPowerMod + 200;

						if (push_list[x]->client->ps.velocity[2] < 128)
						{
							push_list[x]->client->ps.velocity[2] = 128;
						}
					}
					else
					{
						push_list[x]->client->ps.velocity[2] = pushDir[2] * pushPowerMod;
					}
				}
			}
			else if (push_list[x]->s.eType == ET_MISSILE && push_list[x]->s.pos.trType != TR_STATIONARY && (push_list[x]->s.pos.trType != TR_INTERPOLATE || push_list[x]->s.weapon != WP_THERMAL))//rolling and stationary thermal detonators are dealt with below
			{
				if (pull)
				{
					//deflect rather than reflect?
				}
				else
				{
					if (Manual_Saberblocking(self) && self->client->ps.fd.blockPoints > BLOCKPOINTS_FATIGUE)
					{
						G_ManualBlockMissile(self, push_list[x], forward, FP_SABER_DEFENSE);
					}
					else
					{
						G_ReflectMissile(self, push_list[x], forward, FP_SABER_DEFENSE);
					}
				}
			}
			else if (g_pushitems.integer && CheckPushItem(push_list[x]))
			{
				if (push_list[x]->item->giType == IT_TEAM)
				{
					push_list[x]->nextthink = level.time + CTF_FLAG_RETURN_TIME;
					push_list[x]->think = ResetItem;//incase it falls off a cliff
				}
				else
				{
					push_list[x]->nextthink = level.time + 30000;
					push_list[x]->think = ResetItem;//incase it falls off a cliff
				}

				if (pull)
				{//pull the item

					push_list[x]->s.pos.trType = TR_GRAVITY;
					push_list[x]->s.apos.trType = TR_GRAVITY;
					VectorScale(forward, -650.0f, push_list[x]->s.pos.trDelta);
					VectorScale(forward, -650.0f, push_list[x]->s.apos.trDelta);
					push_list[x]->s.pos.trTime = level.time;		// move a bit on the very first frame
					push_list[x]->s.apos.trTime = level.time;		// move a bit on the very first frame
					VectorCopy(push_list[x]->r.currentOrigin, push_list[x]->s.pos.trBase);
					VectorCopy(push_list[x]->r.currentOrigin, push_list[x]->s.apos.trBase);
					push_list[x]->s.eFlags = FL_BOUNCE_HALF;
				}
				else
				{
					push_list[x]->s.pos.trType = TR_GRAVITY;
					push_list[x]->s.apos.trType = TR_GRAVITY;
					VectorScale(forward, 650.0f, push_list[x]->s.pos.trDelta);
					VectorScale(forward, 650.0f, push_list[x]->s.apos.trDelta);
					push_list[x]->s.pos.trTime = level.time;		// move a bit on the very first frame
					push_list[x]->s.apos.trTime = level.time;		// move a bit on the very first frame
					VectorCopy(push_list[x]->r.currentOrigin, push_list[x]->s.pos.trBase);
					VectorCopy(push_list[x]->r.currentOrigin, push_list[x]->s.apos.trBase);
					push_list[x]->s.eFlags = FL_BOUNCE_HALF;
				}
			}
			else if (!Q_stricmp("func_static", push_list[x]->classname))
			{//force-usable func_static
				if (!pull && (push_list[x]->spawnflags & 1/*F_PUSH*/))
				{
					GEntity_UseFunc(push_list[x], self, self);
				}
				else if (pull && (push_list[x]->spawnflags & 2/*F_PULL*/))
				{
					GEntity_UseFunc(push_list[x], self, self);
				}
			}
			else if (!Q_stricmp("func_door", push_list[x]->classname) && (push_list[x]->spawnflags & 2))
			{//push/pull the door
				vec3_t	pos1, pos2;
				vec3_t	trFrom;

				VectorCopy(self->client->ps.origin, trFrom);
				trFrom[2] += self->client->ps.viewheight;

				AngleVectors(self->client->ps.viewangles, forward, NULL, NULL);
				VectorNormalize(forward);
				VectorMA(trFrom, radius, forward, end);
				trap->Trace(&tr, trFrom, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, qfalse, 0, 0);
				if (tr.entityNum != push_list[x]->s.number || tr.fraction == 1.0 || tr.allsolid || tr.startsolid)
				{//must be pointing right at it
					continue;
				}

				if (VectorCompare(vec3_origin, push_list[x]->s.origin))
				{//does not have an origin brush, so pos1 & pos2 are relative to world origin, need to calc center
					VectorSubtract(push_list[x]->r.absmax, push_list[x]->r.absmin, size);
					VectorMA(push_list[x]->r.absmin, 0.5, size, center);
					if ((push_list[x]->spawnflags & 1) && push_list[x]->moverState == MOVER_POS1)
					{//if at pos1 and started open, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
						VectorSubtract(center, push_list[x]->pos1, center);
					}
					else if (!(push_list[x]->spawnflags & 1) && push_list[x]->moverState == MOVER_POS2)
					{//if at pos2, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
						VectorSubtract(center, push_list[x]->pos2, center);
					}
					VectorAdd(center, push_list[x]->pos1, pos1);
					VectorAdd(center, push_list[x]->pos2, pos2);
				}
				else
				{//actually has an origin, pos1 and pos2 are absolute
					VectorCopy(push_list[x]->r.currentOrigin, center);
					VectorCopy(push_list[x]->pos1, pos1);
					VectorCopy(push_list[x]->pos2, pos2);
				}

				if (Distance(pos1, trFrom) < Distance(pos2, trFrom))
				{//pos1 is closer
					if (push_list[x]->moverState == MOVER_POS1)
					{//at the closest pos
						if (pull)
						{//trying to pull, but already at closest point, so screw it
							continue;
						}
					}
					else if (push_list[x]->moverState == MOVER_POS2)
					{//at farthest pos
						if (!pull)
						{//trying to push, but already at farthest point, so screw it
							continue;
						}
					}
				}
				else
				{//pos2 is closer
					if (push_list[x]->moverState == MOVER_POS1)
					{//at the farthest pos
						if (!pull)
						{//trying to push, but already at farthest point, so screw it
							continue;
						}
					}
					else if (push_list[x]->moverState == MOVER_POS2)
					{//at closest pos
						if (pull)
						{//trying to pull, but already at closest point, so screw it
							continue;
						}
					}
				}
				GEntity_UseFunc(push_list[x], self, self);
			}
			else if (Q_stricmp("func_button", push_list[x]->classname) == 0)
			{//pretend you pushed it
				Touch_Button(push_list[x], self, NULL);
				continue;
			}
		}
	}

	//attempt to break any leftover grips
	//if we're still in a current grip that wasn't broken by the push, it will still remain
	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	if (self->client->ps.fd.forceGripBeingGripped > level.time)
	{
		self->client->ps.fd.forceGripBeingGripped = 0;
	}
}

void WP_ForcePowerStop(gentity_t * self, forcePowers_t forcePower)
{
	int wasActive = self->client->ps.fd.forcePowersActive;

	self->client->ps.fd.forcePowersActive &= ~(1 << forcePower);

	switch ((int)forcePower)
	{
	case FP_HEAL:
		self->client->ps.fd.forceHealAmount = 0;
		self->client->ps.fd.forceHealTime = 0;
		break;
	case FP_LEVITATION:
		break;
	case FP_SPEED:
		if (wasActive& (1 << FP_SPEED))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_2 - 50], CHAN_VOICE);
		}
		if (self->client->ps.fd.forcePowerLevel[FP_SPEED] < FORCE_LEVEL_2)
		{
			self->client->ps.fd.forceSpeedRecoveryTime = level.time + 1500;//recover for 1.5 seconds
		}
		else
		{
			self->client->ps.fd.forceSpeedRecoveryTime = level.time + 1000;//recover for 1 seconds
		}
		break;
	case FP_PUSH:
		break;
	case FP_PULL:
		break;
	case FP_TELEPATHY:
		if (wasActive& (1 << FP_TELEPATHY))
		{
			G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/force/distractstop.wav"));
		}
		self->client->ps.fd.forceMindtrickTargetIndex = 0;
		self->client->ps.fd.forceMindtrickTargetIndex2 = 0;
		self->client->ps.fd.forceMindtrickTargetIndex3 = 0;
		self->client->ps.fd.forceMindtrickTargetIndex4 = 0;
		break;
	case FP_SEE:
		if (wasActive& (1 << FP_SEE))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_5 - 50], CHAN_VOICE);
		}
		break;
	case FP_GRIP:
		self->client->ps.fd.forceGripUseTime = level.time + 3000;
		if (self->client->ps.fd.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 &&
			g_entities[self->client->ps.fd.forceGripEntityNum].client &&
			g_entities[self->client->ps.fd.forceGripEntityNum].health > 0 &&
			g_entities[self->client->ps.fd.forceGripEntityNum].inuse &&
			(level.time - g_entities[self->client->ps.fd.forceGripEntityNum].client->ps.fd.forceGripStarted) > 500)
		{ //if we had our throat crushed in for more than half a second, gasp for air when we're let go
			if (wasActive & (1 << FP_GRIP))
			{
				G_EntitySound(&g_entities[self->client->ps.fd.forceGripEntityNum], CHAN_VOICE, G_SoundIndex("*gasp.wav"));
			}
		}

		if (g_entities[self->client->ps.fd.forceGripEntityNum].client&&
			g_entities[self->client->ps.fd.forceGripEntityNum].inuse)
		{

			g_entities[self->client->ps.fd.forceGripEntityNum].client->ps.forceGripChangeMovetype = PM_NORMAL;
		}

		if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		{
			self->client->ps.forceHandExtendTime = 0;
		}

		self->client->ps.fd.forceGripEntityNum = ENTITYNUM_NONE;

		self->client->ps.powerups[PW_DISINT_4] = 0;
		break;
	case FP_LIGHTNING:
		if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] < FORCE_LEVEL_2)
		{//don't do it again for 3 seconds, minimum... FIXME: this should be automatic once regeneration is slower (normal)
			self->client->ps.fd.forcePowerDebounce[FP_LIGHTNING] = level.time + 3000;
		}
		else
		{
			self->client->ps.fd.forcePowerDebounce[FP_LIGHTNING] = level.time + 1500;
		}
		if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		{
			self->client->ps.forceHandExtendTime = 0; //reset hand position
		}

		self->client->ps.activeForcePass = 0;
		break;
	case FP_RAGE:
		if (self->client->ps.fd.forcePowerLevel[FP_RAGE] < FORCE_LEVEL_2)
		{
			self->client->ps.fd.forceRageRecoveryTime = level.time + 10000;//recover for 10 seconds
		}
		else
		{
			self->client->ps.fd.forceRageRecoveryTime = level.time + 5000;//recover for 5 seconds
		}

		if (wasActive& (1 << FP_RAGE))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_3 - 50], CHAN_VOICE);
		}
		break;
	case FP_ABSORB:
		if (wasActive& (1 << FP_ABSORB))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_3 - 50], CHAN_VOICE);
		}
		break;
	case FP_PROTECT:
		if (wasActive& (1 << FP_PROTECT))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_3 - 50], CHAN_VOICE);
		}
		break;
	case FP_DRAIN:
		if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] < FORCE_LEVEL_2)
		{//don't do it again for 3 seconds, minimum...
			self->client->ps.fd.forcePowerDebounce[FP_DRAIN] = level.time + 3000;
		}
		else
		{
			self->client->ps.fd.forcePowerDebounce[FP_DRAIN] = level.time + 1500;
		}

		if (self->client->ps.forceHandExtend == HANDEXTEND_FORCE_HOLD)
		{
			self->client->ps.forceHandExtendTime = 0; //reset hand position
		}

		self->client->ps.activeForcePass = 0;
	default:
		break;
	}
}

extern qboolean Boba_Flying(gentity_t * self);
extern void Boba_FlyStop(gentity_t * self);
void DoGripAction(gentity_t * self, forcePowers_t forcePower)
{
	gentity_t* gripEnt;
	int gripLevel = 0;
	trace_t tr;
	vec3_t a;
	vec3_t fwd, fwd_o, start_o, nvel;

	self->client->dangerTime = level.time;
	self->client->ps.eFlags &= ~EF_INVULNERABLE;
	self->client->invulnerableTimer = 0;

	gripEnt = &g_entities[self->client->ps.fd.forceGripEntityNum];

	if (!gripEnt || !gripEnt->client || !gripEnt->inuse || gripEnt->health < 1 || !ForcePowerUsableOn(self, gripEnt, FP_GRIP))
	{
		WP_ForcePowerStop(self, forcePower);
		self->client->ps.fd.forceGripEntityNum = ENTITYNUM_NONE;

		if (gripEnt&& gripEnt->client&& gripEnt->inuse)
		{
			gripEnt->client->ps.forceGripChangeMovetype = PM_NORMAL;
		}
		return;
	}

	VectorSubtract(gripEnt->client->ps.origin, self->client->ps.origin, a);

	trap->Trace(&tr, self->client->ps.origin, NULL, NULL, gripEnt->client->ps.origin, self->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);

	gripLevel = WP_AbsorbConversion(gripEnt, gripEnt->client->ps.fd.forcePowerLevel[FP_ABSORB], self, FP_GRIP, self->client->ps.fd.forcePowerLevel[FP_GRIP], forcePowerNeeded[self->client->ps.fd.forcePowerLevel[FP_GRIP]][FP_GRIP]);

	if (gripLevel == -1)
	{
		gripLevel = self->client->ps.fd.forcePowerLevel[FP_GRIP];
	}

	if (!gripLevel)
	{
		WP_ForcePowerStop(self, forcePower);
		return;
	}

	if (VectorLength(a) > MAX_GRIP_DISTANCE)
	{
		WP_ForcePowerStop(self, forcePower);
		return;
	}

	if (!InFront(gripEnt->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.9f) &&
		gripLevel < FORCE_LEVEL_3)
	{
		WP_ForcePowerStop(self, forcePower);
		return;
	}

	if (tr.fraction != 1.0f &&
		tr.entityNum != gripEnt->s.number /*&&
		gripLevel < FORCE_LEVEL_3*/)
	{
		WP_ForcePowerStop(self, forcePower);
		return;
	}

	if (self->client->ps.fd.forcePowerDebounce[FP_GRIP] < level.time)
	{ //2 damage per second while choking, resulting in 10 damage total (not including The Squeeze<tm>)
		self->client->ps.fd.forcePowerDebounce[FP_GRIP] = level.time + 1000;
		G_Damage(gripEnt, self, self, NULL, NULL, 2, DAMAGE_NO_ARMOR, MOD_FORCE_DARK);
	}

	Jetpack_Off(gripEnt); //make sure the guy being gripped has his jetpack off.

	if (Boba_Flying(gripEnt))
	{
		Boba_FlyStop(gripEnt);
	}

	if (gripLevel == FORCE_LEVEL_1)
	{
		gripEnt->client->ps.fd.forceGripBeingGripped = level.time + 1000;

		if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 5000)
		{
			WP_ForcePowerStop(self, forcePower);
		}
		return;
	}

	if (gripLevel == FORCE_LEVEL_2)
	{
		gripEnt->client->ps.fd.forceGripBeingGripped = level.time + 1000;

		if (gripEnt->client->ps.forceGripMoveInterval < level.time)
		{
			gripEnt->client->ps.velocity[2] = 30;

			gripEnt->client->ps.forceGripMoveInterval = level.time + 300; //only update velocity every 300ms, so as to avoid heavy bandwidth usage
		}

		gripEnt->client->ps.otherKiller = self->s.number;
		gripEnt->client->ps.otherKillerTime = level.time + 5000;
		gripEnt->client->ps.otherKillerDebounceTime = level.time + 100;
		gripEnt->client->otherKillerMOD = MOD_UNKNOWN;
		gripEnt->client->otherKillerVehWeapon = 0;
		gripEnt->client->otherKillerWeaponType = WP_NONE;

		gripEnt->client->ps.forceGripChangeMovetype = PM_FLOAT;

		if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 3000 && !self->client->ps.fd.forceGripDamageDebounceTime)
		{ //if we managed to lift him into the air for 2 seconds, give him a crack
			self->client->ps.fd.forceGripDamageDebounceTime = 1;
			G_Damage(gripEnt, self, self, NULL, NULL, 20, DAMAGE_NO_ARMOR, MOD_FORCE_DARK);

			//Must play custom sounds on the actual entity. Don't use G_Sound (it creates a temp entity for the sound)
			G_EntitySound(gripEnt, CHAN_VOICE, G_SoundIndex(va("*choke%d.wav", Q_irand(1, 3))));

			gripEnt->client->ps.forceHandExtend = HANDEXTEND_CHOKE;
			gripEnt->client->ps.forceHandExtendTime = level.time + 2000;

			if (gripEnt->client->ps.fd.forcePowersActive& (1 << FP_GRIP))
			{ //choking, so don't let him keep gripping himself
				WP_ForcePowerStop(gripEnt, FP_GRIP);
			}
		}
		else if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 4000)
		{
			WP_ForcePowerStop(self, forcePower);
		}
		return;
	}

	if (gripLevel == FORCE_LEVEL_3)
	{
		gripEnt->client->ps.fd.forceGripBeingGripped = level.time + 1000;

		gripEnt->client->ps.otherKiller = self->s.number;
		gripEnt->client->ps.otherKillerTime = level.time + 5000;
		gripEnt->client->ps.otherKillerDebounceTime = level.time + 100;
		gripEnt->client->otherKillerMOD = MOD_UNKNOWN;
		gripEnt->client->otherKillerVehWeapon = 0;
		gripEnt->client->otherKillerWeaponType = WP_NONE;

		gripEnt->client->ps.forceGripChangeMovetype = PM_FLOAT;

		if (gripEnt->client->ps.forceGripMoveInterval < level.time)
		{
			float nvLen = 0;

			VectorCopy(gripEnt->client->ps.origin, start_o);
			AngleVectors(self->client->ps.viewangles, fwd, NULL, NULL);
			fwd_o[0] = self->client->ps.origin[0] + fwd[0] * 128;
			fwd_o[1] = self->client->ps.origin[1] + fwd[1] * 128;
			fwd_o[2] = self->client->ps.origin[2] + fwd[2] * 128;
			fwd_o[2] += 16;
			VectorSubtract(fwd_o, start_o, nvel);

			nvLen = VectorLength(nvel);

			if (nvLen < 16)
			{ //within x units of desired spot
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0] * 8;
				gripEnt->client->ps.velocity[1] = nvel[1] * 8;
				gripEnt->client->ps.velocity[2] = nvel[2] * 8;
			}
			else if (nvLen < 64)
			{
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0] * 128;
				gripEnt->client->ps.velocity[1] = nvel[1] * 128;
				gripEnt->client->ps.velocity[2] = nvel[2] * 128;
			}
			else if (nvLen < 128)
			{
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0] * 256;
				gripEnt->client->ps.velocity[1] = nvel[1] * 256;
				gripEnt->client->ps.velocity[2] = nvel[2] * 256;
			}
			else if (nvLen < 200)
			{
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0] * 512;
				gripEnt->client->ps.velocity[1] = nvel[1] * 512;
				gripEnt->client->ps.velocity[2] = nvel[2] * 512;
			}
			else
			{
				VectorNormalize(nvel);
				gripEnt->client->ps.velocity[0] = nvel[0] * 700;
				gripEnt->client->ps.velocity[1] = nvel[1] * 700;
				gripEnt->client->ps.velocity[2] = nvel[2] * 700;
			}

			gripEnt->client->ps.forceGripMoveInterval = level.time + 300; //only update velocity every 300ms, so as to avoid heavy bandwidth usage
		}

		if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 3000 && !self->client->ps.fd.forceGripDamageDebounceTime)
		{ //if we managed to lift him into the air for 2 seconds, give him a crack
			self->client->ps.fd.forceGripDamageDebounceTime = 1;
			G_Damage(gripEnt, self, self, NULL, NULL, 40, DAMAGE_NO_ARMOR, MOD_FORCE_DARK);

			//Must play custom sounds on the actual entity. Don't use G_Sound (it creates a temp entity for the sound)
			G_EntitySound(gripEnt, CHAN_VOICE, G_SoundIndex(va("*choke%d.wav", Q_irand(1, 3))));

			gripEnt->client->ps.forceHandExtend = HANDEXTEND_CHOKE;
			gripEnt->client->ps.forceHandExtendTime = level.time + 2000;

			if (gripEnt->client->ps.fd.forcePowersActive& (1 << FP_GRIP))
			{ //choking, so don't let him keep gripping himself
				WP_ForcePowerStop(gripEnt, FP_GRIP);
			}
		}
		else if ((level.time - gripEnt->client->ps.fd.forceGripStarted) > 4000)
		{
			WP_ForcePowerStop(self, forcePower);
		}
		return;
	}
}

qboolean G_IsMindTricked(forcedata_t * fd, int client)
{
	int checkIn;
	int trickIndex1, trickIndex2, trickIndex3, trickIndex4;
	int sub = 0;

	if (!fd)
	{
		return qfalse;
	}

	trickIndex1 = fd->forceMindtrickTargetIndex;
	trickIndex2 = fd->forceMindtrickTargetIndex2;
	trickIndex3 = fd->forceMindtrickTargetIndex3;
	trickIndex4 = fd->forceMindtrickTargetIndex4;

	if (client > 47)
	{
		checkIn = trickIndex4;
		sub = 48;
	}
	else if (client > 31)
	{
		checkIn = trickIndex3;
		sub = 32;
	}
	else if (client > 15)
	{
		checkIn = trickIndex2;
		sub = 16;
	}
	else
	{
		checkIn = trickIndex1;
	}

	if (checkIn& (1 << (client - sub)))
	{
		return qtrue;
	}

	return qfalse;
}

static void RemoveTrickedEnt(forcedata_t * fd, int client)
{
	if (!fd)
	{
		return;
	}

	if (client > 47)
	{
		fd->forceMindtrickTargetIndex4 &= ~(1 << (client - 48));
	}
	else if (client > 31)
	{
		fd->forceMindtrickTargetIndex3 &= ~(1 << (client - 32));
	}
	else if (client > 15)
	{
		fd->forceMindtrickTargetIndex2 &= ~(1 << (client - 16));
	}
	else
	{
		fd->forceMindtrickTargetIndex &= ~(1 << client);
	}
}

extern int g_LastFrameTime;
extern int g_TimeSinceLastFrame;

static void WP_UpdateMindtrickEnts(gentity_t * self)
{
	int i = 0;

	while (i < MAX_CLIENTS)
	{
		if (G_IsMindTricked(&self->client->ps.fd, i))
		{
			gentity_t* ent = &g_entities[i];

			if (!ent || !ent->client || !ent->inuse || ent->health < 1 ||
				(ent->client->ps.fd.forcePowersActive & (1 << FP_SEE)))
			{
				RemoveTrickedEnt(&self->client->ps.fd, i);
			}
			else if ((level.time - self->client->dangerTime) < g_TimeSinceLastFrame * 4)
			{ //Untrick this entity if the tricker (self) fires while in his fov
				if (trap->InPVS(ent->client->ps.origin, self->client->ps.origin) &&
					OrgVisible(ent->client->ps.origin, self->client->ps.origin, ent->s.number))
				{
					RemoveTrickedEnt(&self->client->ps.fd, i);
				}
			}
			else if (BG_HasYsalamiri(level.gametype, &ent->client->ps))
			{
				RemoveTrickedEnt(&self->client->ps.fd, i);
			}
		}

		i++;
	}

	if (!self->client->ps.fd.forceMindtrickTargetIndex &&
		!self->client->ps.fd.forceMindtrickTargetIndex2 &&
		!self->client->ps.fd.forceMindtrickTargetIndex3 &&
		!self->client->ps.fd.forceMindtrickTargetIndex4)
	{ //everyone who we had tricked is no longer tricked, so stop the power
		WP_ForcePowerStop(self, FP_TELEPATHY);
	}
	else if (self->client->ps.powerups[PW_REDFLAG] ||
		self->client->ps.powerups[PW_BLUEFLAG])
	{
		WP_ForcePowerStop(self, FP_TELEPATHY);
	}
}

#define FORCE_DEBOUNCE_TIME 50 // sv_fps 20 = 50msec frametime, basejka balance/timing

static int FlamethrowerDebounceTime = 0;
#define FLAMETHROWERDEBOUNCE		50 

static void WP_ForcePowerRun(gentity_t * self, forcePowers_t forcePower, usercmd_t * cmd)
{
	//	extern usercmd_t	ucmd;

	switch ((int)forcePower)
	{
	case FP_HEAL:
		if (self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_1)
		{
			if (self->client->ps.velocity[0] || self->client->ps.velocity[1] || self->client->ps.velocity[2])
			{
				WP_ForcePowerStop(self, forcePower);
				break;
			}
		}

		if (self->health < 1 || self->client->ps.stats[STAT_HEALTH] < 1)
		{
			WP_ForcePowerStop(self, forcePower);
			break;
		}

		if (self->client->ps.fd.forceHealTime > level.time)
		{
			break;
		}
		if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])
		{ //rww - we might start out over max_health and we don't want force heal taking us down to 100 or whatever max_health is
			WP_ForcePowerStop(self, forcePower);
			break;
		}
		self->client->ps.fd.forceHealTime = level.time + 1000;
		self->health++;
		self->client->ps.fd.forceHealAmount++;

		if (self->health > self->client->ps.stats[STAT_MAX_HEALTH])	// Past max health
		{
			self->health = self->client->ps.stats[STAT_MAX_HEALTH];
			WP_ForcePowerStop(self, forcePower);
		}

		if ((self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_1 && self->client->ps.fd.forceHealAmount >= 25) ||
			(self->client->ps.fd.forcePowerLevel[FP_HEAL] == FORCE_LEVEL_2 && self->client->ps.fd.forceHealAmount >= 33))
		{
			WP_ForcePowerStop(self, forcePower);
		}
		break;
	case FP_SPEED:
		//This is handled in PM_WalkMove and PM_StepSlideMove
		if (self->client->holdingObjectiveItem >= MAX_CLIENTS
			&& self->client->holdingObjectiveItem < ENTITYNUM_WORLD)
		{
			if (g_entities[self->client->holdingObjectiveItem].genericValue15)
			{//disables force powers
				WP_ForcePowerStop(self, forcePower);
			}
		}
		break;
	case FP_GRIP:
		if (self->client->ps.forceHandExtend != HANDEXTEND_FORCE_HOLD)
		{
			WP_ForcePowerStop(self, FP_GRIP);
			break;
		}

		if (self->client->ps.fd.forcePowerDebounce[FP_PULL] < level.time)
		{ //This is sort of not ideal. Using the debounce value reserved for pull for this because pull doesn't need it.
			WP_ForcePowerDrain(&self->client->ps, forcePower, 1);
			self->client->ps.fd.forcePowerDebounce[FP_PULL] = level.time + 100;
		}

		if (self->client->ps.fd.forcePower < 1)
		{
			WP_ForcePowerStop(self, FP_GRIP);
			break;
		}

		DoGripAction(self, forcePower);
		break;
	case FP_LEVITATION:
		if (self->client->ps.groundEntityNum != ENTITYNUM_NONE && !self->client->ps.fd.forceJumpZStart)
		{//done with jump
			WP_ForcePowerStop(self, forcePower);
		}
		break;
	case FP_RAGE:
		if (self->health < 1)
		{
			WP_ForcePowerStop(self, forcePower);
			break;
		}
		if (self->client->ps.forceRageDrainTime < level.time)
		{
			int addTime = 400;

			self->health -= 2;

			if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_1)
			{
				addTime = 150;
			}
			else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_2)
			{
				addTime = 300;
			}
			else if (self->client->ps.fd.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_3)
			{
				addTime = 450;
			}
			self->client->ps.forceRageDrainTime = level.time + addTime;
		}

		if (self->health < 1)
		{
			self->health = 1;
			WP_ForcePowerStop(self, forcePower);
		}

		self->client->ps.stats[STAT_HEALTH] = self->health;
		break;
	case FP_DRAIN:
		if (self->client->ps.forceHandExtend != HANDEXTEND_FORCE_HOLD)
		{
			WP_ForcePowerStop(self, forcePower);
			break;
		}

		if (self->client->ps.fd.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_1)
		{//higher than level 1
			if ((cmd->buttons & BUTTON_FORCE_DRAIN) || ((cmd->buttons & BUTTON_FORCEPOWER) && self->client->ps.fd.forcePowerSelected == FP_DRAIN))
			{//holding it keeps it going
				self->client->ps.fd.forcePowerDuration[FP_DRAIN] = level.time + 500;
			}
		}
		// OVERRIDEFIXME
		if (!WP_ForcePowerAvailable(self, forcePower, 0) || self->client->ps.fd.forcePowerDuration[FP_DRAIN] < level.time ||
			self->client->ps.fd.forcePower < 25)
		{
			WP_ForcePowerStop(self, forcePower);
		}
		else
		{
			while (self->client->force.drainDebounce < level.time)
			{
				ForceShootDrain(self);
				self->client->force.drainDebounce += FORCE_DEBOUNCE_TIME;
			}
		}
		break;
	case FP_LIGHTNING:
		if (self->client->ps.forceHandExtend != HANDEXTEND_FORCE_HOLD)
		{ //Animation for hand extend doesn't end with hand out, so we have to limit lightning intervals by animation intervals (once hand starts to go in in animation, lightning should stop)
			WP_ForcePowerStop(self, forcePower);
			break;
		}

		if (self->client->ps.fd.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1)
		{//higher than level 1
			if ((cmd->buttons & BUTTON_FORCE_LIGHTNING) || ((cmd->buttons & BUTTON_FORCEPOWER) && self->client->ps.fd.forcePowerSelected == FP_LIGHTNING))
			{//holding it keeps it going
				self->client->ps.fd.forcePowerDuration[FP_LIGHTNING] = level.time + 500;
			}
		}

		if (!WP_ForcePowerAvailable(self, forcePower, 0)
			|| self->client->ps.fd.forcePowerDuration[FP_LIGHTNING] < level.time
			|| self->client->ps.fd.forcePower < 25)
		{
			WP_ForcePowerStop(self, forcePower);
		}
		else
		{
			while (self->client->force.lightningDebounce < level.time)
			{
				ForceShootLightning(self);

				if (self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_OLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_NEW
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE)
				{//jackin' 'em up, Palpatine-style
					WP_ForcePowerDrain(&self->client->ps, forcePower, 0);
				}
				WP_ForcePowerDrain(&self->client->ps, forcePower, 0);

				self->client->force.lightningDebounce += FORCE_DEBOUNCE_TIME;
			}
		}
		break;
	case FP_TELEPATHY:
		if (self->client->holdingObjectiveItem >= MAX_CLIENTS
			&& self->client->holdingObjectiveItem < ENTITYNUM_WORLD
			&& g_entities[self->client->holdingObjectiveItem].genericValue15)
		{ //if force hindered can't mindtrick whilst carrying a siege item
			WP_ForcePowerStop(self, FP_TELEPATHY);
		}
		else
		{
			WP_UpdateMindtrickEnts(self);
		}
		break;
	case FP_SABER_OFFENSE:
		break;
	case FP_SABER_DEFENSE:
		break;
	case FP_SABERTHROW:
		break;
	case FP_PROTECT:
		if (self->client->ps.fd.forcePowerDebounce[forcePower] < level.time)
		{
			WP_ForcePowerDrain(&self->client->ps, forcePower, 1);
			if (self->client->ps.fd.forcePower < 1)
			{
				WP_ForcePowerStop(self, forcePower);
			}

			self->client->ps.fd.forcePowerDebounce[forcePower] = level.time + 300;
		}
		break;
	case FP_ABSORB:
		if (self->client->ps.fd.forcePowerDebounce[forcePower] < level.time)
		{
			WP_ForcePowerDrain(&self->client->ps, forcePower, 1);
			if (self->client->ps.fd.forcePower < 1)
			{
				WP_ForcePowerStop(self, forcePower);
			}

			self->client->ps.fd.forcePowerDebounce[forcePower] = level.time + 600;
		}
		break;
	default:
		break;
	}
}

int WP_DoSpecificPower(gentity_t * self, usercmd_t * ucmd, forcePowers_t forcepower)
{
	int powerSucceeded;

	powerSucceeded = 1;

	// OVERRIDEFIXME
	if (!WP_ForcePowerAvailable(self, forcepower, 0))
	{
		return 0;
	}

	switch (forcepower)
	{
	case FP_HEAL:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceHeal(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_LEVITATION:
		//if leave the ground by some other means, cancel the force jump so we don't suddenly jump when we land.

		if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			self->client->ps.fd.forceJumpCharge = 0;
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1 - 50], CHAN_VOICE);
			//This only happens if the groundEntityNum == ENTITYNUM_NONE when the button is actually released
		}
		else
		{//still on ground, so jump
			ForceJump(self, ucmd);
		}
		break;
	case FP_SPEED:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		if (IsPressingDashButton(self))
		{
			ForceSpeedDash(self, 0);
		}
		else
		{
			ForceSpeed(self, 0);
		}
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_GRIP:
		if (self->client->ps.fd.forceGripEntityNum == ENTITYNUM_NONE)
		{
			ForceGrip(self);
		}

		if (self->client->ps.fd.forceGripEntityNum != ENTITYNUM_NONE)
		{
			if (!(self->client->ps.fd.forcePowersActive& (1 << FP_GRIP)))
			{
				WP_ForcePowerStart(self, FP_GRIP, 0);
				WP_ForcePowerDrain(&self->client->ps, FP_GRIP, GRIP_DRAIN_AMOUNT);
			}
		}
		else
		{
			powerSucceeded = 0;
		}
		break;
	case FP_LIGHTNING:
		ForceLightning(self);
		break;
	case FP_PUSH:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease && !(self->r.svFlags& SVF_BOT))
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceThrow(self, qfalse);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_PULL:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceThrow(self, qtrue);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_TELEPATHY:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceTelepathy(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_RAGE:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceRage(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_PROTECT:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceProtect(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_ABSORB:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceAbsorb(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_TEAM_HEAL:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceTeamHeal(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_TEAM_FORCE:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceTeamForceReplenish(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_DRAIN:
		ForceDrain(self);
		break;
	case FP_SEE:
		powerSucceeded = 0; //always 0 for nonhold powers
		if (self->client->ps.fd.forceButtonNeedRelease)
		{ //need to release before we can use nonhold powers again
			break;
		}
		ForceSeeing(self);
		self->client->ps.fd.forceButtonNeedRelease = 1;
		break;
	case FP_SABER_OFFENSE:
		break;
	case FP_SABER_DEFENSE:
		break;
	case FP_SABERTHROW:
		break;
	default:
		break;
	}

	return powerSucceeded;
}

void FindGenericEnemyIndex(gentity_t * self)
{ //Find another client that would be considered a threat.
	int i = 0;
	float tlen;
	gentity_t* ent;
	gentity_t* besten = NULL;
	float blen = 99999999.9f;
	vec3_t a;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent&& ent->client&& ent->s.number != self->s.number && ent->health > 0 && !OnSameTeam(self, ent) && ent->client->ps.pm_type != PM_INTERMISSION && ent->client->ps.pm_type != PM_SPECTATOR)
		{
			VectorSubtract(ent->client->ps.origin, self->client->ps.origin, a);
			tlen = VectorLength(a);

			if (tlen < blen&&
				InFront(ent->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.8f) &&
				OrgVisible(self->client->ps.origin, ent->client->ps.origin, self->s.number))
			{
				blen = tlen;
				besten = ent;
			}
		}

		i++;
	}

	if (!besten)
	{
		return;
	}

	self->client->ps.genericEnemyIndex = besten->s.number;
}

void SeekerDroneUpdate(gentity_t * self)
{
	vec3_t org, elevated, dir, a, endir;
	gentity_t* en;
	float angle;
	float prefig = 0;
	trace_t tr;

	if (!(self->client->ps.eFlags & EF_SEEKERDRONE))
	{
		self->client->ps.genericEnemyIndex = -1;
		return;
	}

	if (self->health < 1)
	{
		VectorCopy(self->client->ps.origin, elevated);
		elevated[2] += 40;

		angle = ((level.time / 12) & 255) * (M_PI * 2) / 255; //magical numbers make magic happen
		dir[0] = cos(angle) * 20;
		dir[1] = sin(angle) * 20;
		dir[2] = cos(angle) * 5;
		VectorAdd(elevated, dir, org);

		a[ROLL] = 0;
		a[YAW] = 0;
		a[PITCH] = 1;

		G_PlayEffect(EFFECT_SPARK_EXPLOSION, org, a);

		self->client->ps.eFlags &= ~EF_SEEKERDRONE;
		self->client->ps.genericEnemyIndex = -1;

		return;
	}

	if (self->client->ps.droneExistTime >= level.time &&
		self->client->ps.droneExistTime < (level.time + 5000))
	{
		self->client->ps.genericEnemyIndex = 1024 + self->client->ps.droneExistTime;
		if (self->client->ps.droneFireTime < level.time)
		{
			G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/laser_trap/warning.wav"));
			self->client->ps.droneFireTime = level.time + 100;
		}
		return;
	}
	else if (self->client->ps.droneExistTime < level.time)
	{
		VectorCopy(self->client->ps.origin, elevated);
		elevated[2] += 40;

		prefig = (self->client->ps.droneExistTime - level.time) / 80;

		if (prefig > 55)
		{
			prefig = 55;
		}
		else if (prefig < 1)
		{
			prefig = 1;
		}

		elevated[2] -= 55 - prefig;

		angle = ((level.time / 12) & 255) * (M_PI * 2) / 255; //magical numbers make magic happen
		dir[0] = cos(angle) * 20;
		dir[1] = sin(angle) * 20;
		dir[2] = cos(angle) * 5;
		VectorAdd(elevated, dir, org);

		a[ROLL] = 0;
		a[YAW] = 0;
		a[PITCH] = 1;

		G_PlayEffect(EFFECT_SPARK_EXPLOSION, org, a);

		self->client->ps.eFlags &= ~EF_SEEKERDRONE;
		self->client->ps.genericEnemyIndex = -1;

		return;
	}

	if (self->client->ps.genericEnemyIndex == -1)
	{
		self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
	}

	if (self->client->ps.genericEnemyIndex != ENTITYNUM_NONE && self->client->ps.genericEnemyIndex != -1)
	{
		en = &g_entities[self->client->ps.genericEnemyIndex];

		if (!en || !en->client)
		{
			self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
		}
		else if (en->s.number == self->s.number)
		{
			self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
		}
		else if (en->health < 1)
		{
			self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
		}
		else if (OnSameTeam(self, en))
		{
			self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
		}
		else
		{
			if (!InFront(en->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.8f))
			{
				self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
			}
			else if (!OrgVisible(self->client->ps.origin, en->client->ps.origin, self->s.number))
			{
				self->client->ps.genericEnemyIndex = ENTITYNUM_NONE;
			}
		}
	}

	if (self->client->ps.genericEnemyIndex == ENTITYNUM_NONE || self->client->ps.genericEnemyIndex == -1)
	{
		FindGenericEnemyIndex(self);
	}

	if (self->client->ps.genericEnemyIndex != ENTITYNUM_NONE && self->client->ps.genericEnemyIndex != -1)
	{
		en = &g_entities[self->client->ps.genericEnemyIndex];

		VectorCopy(self->client->ps.origin, elevated);
		elevated[2] += 40;

		angle = ((level.time / 12) & 255) * (M_PI * 2) / 255; //magical numbers make magic happen
		dir[0] = cos(angle) * 20;
		dir[1] = sin(angle) * 20;
		dir[2] = cos(angle) * 5;
		VectorAdd(elevated, dir, org);

		//org is now where the thing should be client-side because it uses the same time-based offset
		if (self->client->ps.droneFireTime < level.time)
		{
			trap->Trace(&tr, org, NULL, NULL, en->client->ps.origin, -1, MASK_SOLID, qfalse, 0, 0);

			if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
			{
				VectorSubtract(en->client->ps.origin, org, endir);
				VectorNormalize(endir);

				WP_FireGenericBlasterMissile(self, org, endir, 0, 15, 2000, MOD_BLASTER);
				G_SoundAtLoc(org, CHAN_WEAPON, G_SoundIndex("sound/weapons/bryar/fire.wav"));

				self->client->ps.droneFireTime = level.time + Q_irand(400, 700);
			}
		}
	}
}

void HolocronUpdate(gentity_t * self)
{ //keep holocron status updated in holocron mode
	int i = 0;
	int noHRank = 0;

	if (noHRank < FORCE_LEVEL_0)
	{
		noHRank = FORCE_LEVEL_0;
	}
	if (noHRank > FORCE_LEVEL_3)
	{
		noHRank = FORCE_LEVEL_3;
	}

	trap->Cvar_Update(&g_maxHolocronCarry);

	while (i < NUM_FORCE_POWERS)
	{
		if (self->client->ps.holocronsCarried[i])
		{ //carrying it, make sure we have the power
			self->client->ps.holocronBits |= (1 << i);
			self->client->ps.fd.forcePowersKnown |= (1 << i);
			self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3;
		}
		else
		{ //otherwise, make sure the power is cleared from us
			self->client->ps.fd.forcePowerLevel[i] = 0;
			if (self->client->ps.holocronBits & (1 << i))
			{
				self->client->ps.holocronBits -= (1 << i);
			}

			if ((self->client->ps.fd.forcePowersKnown & (1 << i)) && i != FP_LEVITATION && i != FP_SABER_OFFENSE)
			{
				self->client->ps.fd.forcePowersKnown -= (1 << i);
			}

			if ((self->client->ps.fd.forcePowersActive & (1 << i)) && i != FP_LEVITATION && i != FP_SABER_OFFENSE)
			{
				WP_ForcePowerStop(self, i);
			}

			if (i == FP_LEVITATION)
			{
				if (noHRank >= FORCE_LEVEL_1)
				{
					self->client->ps.fd.forcePowerLevel[i] = noHRank;
				}
				else
				{
					self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_1;
				}
			}
			else if (i == FP_SABER_OFFENSE)
			{
				self->client->ps.fd.forcePowersKnown |= (1 << i);

				if (noHRank >= FORCE_LEVEL_1)
				{
					self->client->ps.fd.forcePowerLevel[i] = noHRank;
				}
				else
				{
					self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_1;
				}

				//make sure that the player's saber stance is reset so they can't continue to use that stance when they don't have the skill for it anymore.
				if (self->client->saber[0].model[0] && self->client->saber[1].model[0])
				{ //dual
					self->client->ps.fd.saberAnimLevelBase = self->client->ps.fd.saberAnimLevel = self->client->ps.fd.saberDrawAnimLevel = SS_DUAL;
				}
				else if ((self->client->saber[0].saberFlags& SFL_TWO_HANDED))
				{ //staff
					self->client->ps.fd.saberAnimLevel = self->client->ps.fd.saberDrawAnimLevel = SS_STAFF;
				}
				else
				{
					self->client->ps.fd.saberAnimLevelBase = self->client->ps.fd.saberAnimLevel = self->client->ps.fd.saberDrawAnimLevel = SS_MEDIUM;
				}
			}
			else
			{
				self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_0;
			}
		}

		i++;
	}

	if (HasSetSaberOnly())
	{ //if saberonly, we get these powers no matter what (still need the holocrons for level 3)
		if (self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] < FORCE_LEVEL_1)
		{
			self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_1;
		}
		if (self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1)
		{
			self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_1;
		}
	}
}

void JediMasterUpdate(gentity_t * self)
{ //keep jedi master status updated for JM gametype
	int i = 0;

	trap->Cvar_Update(&g_maxHolocronCarry);

	while (i < NUM_FORCE_POWERS)
	{
		if (self->client->ps.isJediMaster)
		{
			self->client->ps.fd.forcePowersKnown |= (1 << i);
			self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3;

			if (i == FP_TEAM_HEAL || i == FP_TEAM_FORCE ||
				i == FP_DRAIN || i == FP_ABSORB)
			{ //team powers are useless in JM, absorb is too because no one else has powers to absorb. Drain is just
			  //relatively useless in comparison, because its main intent is not to heal, but rather to cripple others
			  //by draining their force at the same time. And no one needs force in JM except the JM himself.
				self->client->ps.fd.forcePowersKnown &= ~(1 << i);
				self->client->ps.fd.forcePowerLevel[i] = 0;
			}

			if (i == FP_TELEPATHY)
			{ //this decision was made because level 3 mindtrick allows the JM to just hide too much, and no one else has force
			  //sight to counteract it. Since the JM himself is the focus of gameplay in this mode, having him hidden for large
			  //durations is indeed a bad thing.
				self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_2;
			}
		}
		else
		{
			if ((self->client->ps.fd.forcePowersKnown & (1 << i)) && i != FP_LEVITATION)
			{
				self->client->ps.fd.forcePowersKnown -= (1 << i);
			}

			if ((self->client->ps.fd.forcePowersActive& (1 << i)) && i != FP_LEVITATION)
			{
				WP_ForcePowerStop(self, i);
			}

			if (i == FP_LEVITATION)
			{
				self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_1;
			}
			else
			{
				self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_0;
			}
		}

		i++;
	}
}

qboolean WP_HasForcePowers(const playerState_t * ps)
{
	int i;
	if (ps)
	{
		for (i = 0; i < NUM_FORCE_POWERS; i++)
		{
			if (i == FP_LEVITATION)
			{
				if (ps->fd.forcePowerLevel[i] > FORCE_LEVEL_1)
				{
					return qtrue;
				}
			}
			else if (ps->fd.forcePowerLevel[i] > FORCE_LEVEL_0)
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}

//try a special roll getup move
qboolean G_SpecialRollGetup(gentity_t * self)
{ //fixme: currently no knockdown will actually land you on your front... so froll's are pretty useless at the moment.
	qboolean rolled = qfalse;

	/*
	if (self->client->ps.weapon != WP_SABER &&
		self->client->ps.weapon != WP_MELEE)
	{ //can't do acrobatics without saber selected
		return qfalse;
	}
	*/

	if (/*!self->client->pers.cmd.upmove &&*/
		self->client->pers.cmd.rightmove > 0 &&
		!self->client->pers.cmd.forwardmove)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_GETUP_BROLL_R, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		rolled = qtrue;
	}
	else if (/*!self->client->pers.cmd.upmove &&*/
		self->client->pers.cmd.rightmove < 0 &&
		!self->client->pers.cmd.forwardmove)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_GETUP_BROLL_L, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		rolled = qtrue;
	}
	else if (/*self->client->pers.cmd.upmove > 0 &&*/
		!self->client->pers.cmd.rightmove &&
		self->client->pers.cmd.forwardmove > 0)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_GETUP_BROLL_F, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		rolled = qtrue;
	}
	else if (/*self->client->pers.cmd.upmove > 0 &&*/
		!self->client->pers.cmd.rightmove &&
		self->client->pers.cmd.forwardmove < 0)
	{
		G_SetAnim(self, &self->client->pers.cmd, SETANIM_BOTH, BOTH_GETUP_BROLL_B, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
		rolled = qtrue;
	}
	else if (self->client->pers.cmd.upmove)
	{
		if (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
		{//short burst
			G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/jumpsmall.mp3"));
		}
		else
		{//holding it
			G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEJUMP);
		}
		self->client->ps.forceDodgeAnim = 2;
		self->client->ps.forceHandExtendTime = level.time + 500;
	}

	if (rolled)
	{
		G_EntitySound(self, CHAN_VOICE, G_SoundIndex("*jump1.wav"));
	}

	return rolled;
}

static QINLINE qboolean MeditateCheck(gentity_t * self)
{
	int	anim;

	if (self->client->saber[0].meditateAnim != -1)
	{
		anim = self->client->saber[0].meditateAnim;
	}
	else if (self->client->saber[1].model
		&& self->client->saber[1].model[0]
		&& self->client->saber[1].meditateAnim != -1)
	{
		anim = self->client->saber[1].meditateAnim;
	}
	else
	{
		anim = BOTH_MEDITATE;
	}

	//we don't use a HANDEXTEND_TAUNT check anymore since 
	//it's not always consistantly on during a meditate taunt.
	if (self->client->ps.torsoAnim == anim && self->client->ps.torsoTimer <= 100)
	{
		return qtrue;
	}

	return qfalse;
}

extern qboolean PM_SaberInBrokenParry(int move);
extern qboolean PM_InKnockDown(playerState_t * ps);
void Flamethrower_Fire(gentity_t * self);
void WP_ForcePowersUpdate(gentity_t * self, usercmd_t * ucmd)
{
	int			i, holo, holoregen;
	int			prepower = 0;

	//see if any force powers are running
	if (!self)
	{
		return;
	}

	if (!self->client)
	{
		return;
	}

	if (self->client->ps.pm_flags & PMF_FOLLOW)
	{ //not a "real" game client, it's a spectator following someone
		return;
	}
	if (self->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		return;
	}

	//The stance in relation to power level is no longer applicable with the crazy new akimbo/staff stances.
	if (!self->client->ps.fd.saberAnimLevel)
	{
		self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
	}

	if (level.gametype != GT_SIEGE)
	{
		if (!(self->client->ps.fd.forcePowersKnown & (1 << FP_LEVITATION)))
		{
			self->client->ps.fd.forcePowersKnown |= (1 << FP_LEVITATION);
		}

		if (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_1)
		{
			self->client->ps.fd.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_1;
		}
	}

	if (self->client->ps.fd.forcePowerSelected < 0 || self->client->ps.fd.forcePowerSelected >= NUM_FORCE_POWERS)
	{ //bad
		self->client->ps.fd.forcePowerSelected = 0;
	}
	if (self->client->ps.fd.forcePower <= (self->client->ps.fd.forcePowerMax * FATIGUEDTHRESHHOLD))
	{//Pop the Fatigued flag
		self->client->ps.userInt3 |= (1 << FLAG_FATIGUED);
	}

	if (((self->client->sess.selectedFP != self->client->ps.fd.forcePowerSelected) ||
		(self->client->sess.saberLevel != self->client->ps.fd.saberAnimLevel)) &&
		!(self->r.svFlags & SVF_BOT))
	{
		if (self->client->sess.updateUITime < level.time)
		{ //a bit hackish, but we don't want the client to flood with userinfo updates if they rapidly cycle
		  //through their force powers or saber attack levels

			self->client->sess.selectedFP = self->client->ps.fd.forcePowerSelected;
			self->client->sess.saberLevel = self->client->ps.fd.saberAnimLevel;
		}
	}

	if (!g_LastFrameTime)
	{
		g_LastFrameTime = level.time;
	}

	if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN)
	{
		self->client->ps.zoomFov = 0;
		self->client->ps.zoomMode = 0;
		self->client->ps.zoomLocked = qfalse;
		self->client->ps.zoomTime = 0;
	}

	if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN &&
		self->client->ps.forceHandExtendTime >= level.time)
	{
		self->client->ps.saberMove = 0;
		self->client->ps.saberBlocking = 0;
		self->client->ps.saberBlocked = 0;
		self->client->ps.weaponTime = 0;
		self->client->ps.weaponstate = WEAPON_READY;
	}
	else if (self->client->ps.forceHandExtend != HANDEXTEND_NONE &&
		self->client->ps.forceHandExtendTime < level.time)
	{
		if (self->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN &&
			!self->client->ps.forceDodgeAnim)
		{
			if (self->health < 1 || (self->client->ps.eFlags & EF_DEAD))
			{
				self->client->ps.forceHandExtend = HANDEXTEND_NONE;
			}
			else if (G_SpecialRollGetup(self))
			{
				self->client->ps.forceHandExtend = HANDEXTEND_NONE;
			}
			else
			{ //hmm.. ok.. no more getting up on your own, you've gotta push something, unless..
				if ((level.time - self->client->ps.forceHandExtendTime) > 4000)
				{ //4 seconds elapsed, I guess they're too dumb to push something to get up!
					if (self->client->pers.cmd.upmove &&
						self->client->ps.fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1)
					{ //force getup
						if (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
						{//short burst
							G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/jumpsmall.mp3"));
						}
						else
						{//holding it
							G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEJUMP);
						}
						self->client->ps.forceDodgeAnim = 2;
						self->client->ps.forceHandExtendTime = level.time + 500;
					}
					else if (self->client->ps.quickerGetup)
					{
						G_EntitySound(self, CHAN_VOICE, G_SoundIndex("*jump1.wav"));
						self->client->ps.forceDodgeAnim = 3;
						self->client->ps.forceHandExtendTime = level.time + 500;
						self->client->ps.velocity[2] = 300;
					}
					else
					{
						self->client->ps.forceDodgeAnim = 1;
						self->client->ps.forceHandExtendTime = level.time + 1000;
					}
				}
			}
			self->client->ps.quickerGetup = qfalse;
		}
		else if (self->client->ps.forceHandExtend == HANDEXTEND_POSTTHROWN)
		{
			if (self->health < 1 || (self->client->ps.eFlags & EF_DEAD))
			{
				self->client->ps.forceHandExtend = HANDEXTEND_NONE;
			}
			else if (self->client->ps.groundEntityNum != ENTITYNUM_NONE && !self->client->ps.forceDodgeAnim)
			{
				self->client->ps.forceDodgeAnim = 1;
				self->client->ps.forceHandExtendTime = level.time + 1000;
				G_EntitySound(self, CHAN_VOICE, G_SoundIndex("*jump1.wav"));
				self->client->ps.velocity[2] = 100;
			}
			else if (!self->client->ps.forceDodgeAnim)
			{
				self->client->ps.forceHandExtendTime = level.time + 100;
			}
			else
			{
				self->client->ps.forceHandExtend = HANDEXTEND_WEAPONREADY;
			}
		}
		else if (self->client->ps.forceHandExtend == HANDEXTEND_DODGE)
		{//don't do the HANDEXTEND_WEAPONREADY since it screws up our saber block code.
			self->client->ps.forceHandExtend = HANDEXTEND_NONE;
		}
		else
		{
			self->client->ps.forceHandExtend = HANDEXTEND_WEAPONREADY;
		}
	}

	if (level.gametype == GT_HOLOCRON)
	{
		HolocronUpdate(self);
	}
	if (level.gametype == GT_JEDIMASTER)
	{
		JediMasterUpdate(self);
	}

	SeekerDroneUpdate(self);

	if (self->client->ps.powerups[PW_FORCE_BOON])
	{
		prepower = self->client->ps.fd.forcePower;
	}

	if (self && self->client && (BG_HasYsalamiri(level.gametype, &self->client->ps) ||
		self->client->ps.fd.forceDeactivateAll || self->client->tempSpectate >= level.time))
	{ //has ysalamiri.. or we want to forcefully stop all his active powers
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			if ((self->client->ps.fd.forcePowersActive& (1 << i)) && i != FP_LEVITATION)
			{
				WP_ForcePowerStop(self, i);
			}

			i++;
		}

		if (self->client->tempSpectate >= level.time)
		{
			self->client->ps.fd.forcePower = 100;
			self->client->ps.fd.forceRageRecoveryTime = 0;
			self->client->ps.fd.forceSpeedRecoveryTime = 0;
		}

		self->client->ps.fd.forceDeactivateAll = 0;

		if (self->client->ps.fd.forceJumpCharge)
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1 - 50], CHAN_VOICE);
			self->client->ps.fd.forceJumpCharge = 0;
		}
	}
	else
	{ //otherwise just do a check through them all to see if they need to be stopped for any reason.
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			if ((self->client->ps.fd.forcePowersActive& (1 << i)) && i != FP_LEVITATION &&
				!BG_CanUseFPNow(level.gametype, &self->client->ps, level.time, i))
			{
				WP_ForcePowerStop(self, i);
			}

			i++;
		}
	}

	i = 0;

	if (self->client->ps.powerups[PW_FORCE_ENLIGHTENED_LIGHT] || self->client->ps.powerups[PW_FORCE_ENLIGHTENED_DARK])
	{ //enlightenment
		if (!self->client->ps.fd.forceUsingAdded)
		{
			i = 0;

			while (i < NUM_FORCE_POWERS)
			{
				self->client->ps.fd.forcePowerBaseLevel[i] = self->client->ps.fd.forcePowerLevel[i];

				if (!forcePowerDarkLight[i] ||
					self->client->ps.fd.forceSide == forcePowerDarkLight[i])
				{
					self->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3;
					self->client->ps.fd.forcePowersKnown |= (1 << i);
				}

				i++;
			}

			self->client->ps.fd.forceUsingAdded = 1;
		}
	}
	else if (self->client->ps.fd.forceUsingAdded)
	{ //we don't have enlightenment but we're still using enlightened powers, so clear them back to how they should be.
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			self->client->ps.fd.forcePowerLevel[i] = self->client->ps.fd.forcePowerBaseLevel[i];
			if (!self->client->ps.fd.forcePowerLevel[i])
			{
				if (self->client->ps.fd.forcePowersActive& (1 << i))
				{
					WP_ForcePowerStop(self, i);
				}
				self->client->ps.fd.forcePowersKnown &= ~(1 << i);
			}

			i++;
		}

		self->client->ps.fd.forceUsingAdded = 0;
	}

	i = 0;

	if (!(self->client->ps.fd.forcePowersActive & (1 << FP_TELEPATHY)))
	{ //clear the mindtrick index values
		self->client->ps.fd.forceMindtrickTargetIndex = 0;
		self->client->ps.fd.forceMindtrickTargetIndex2 = 0;
		self->client->ps.fd.forceMindtrickTargetIndex3 = 0;
		self->client->ps.fd.forceMindtrickTargetIndex4 = 0;
	}

	if (self->health < 1)
	{
		self->client->ps.fd.forceGripBeingGripped = 0;
	}

	if (self->client->ps.fd.forceGripBeingGripped > level.time)
	{
		self->client->ps.fd.forceGripCripple = 1;

		//keep the saber off during this period
		if (self->client->ps.weapon == WP_SABER && !self->client->ps.saberHolstered)
		{
			Cmd_ToggleSaber_f(self);
		}
	}
	else
	{
		self->client->ps.fd.forceGripCripple = 0;
	}

	if (self->client->ps.fd.forceJumpSound)
	{
		if (self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_3)
		{//short burst
			G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/force/jumpsmall.mp3"));
		}
		else
		{//holding it
			G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEJUMP);
		}
		self->client->ps.fd.forceJumpSound = 0;
	}

	if (self->client->ps.fd.forceGripCripple)
	{
		if (self->client->ps.fd.forceGripSoundTime < level.time)
		{
			G_PreDefSound(self->client->ps.origin, PDSOUND_FORCEGRIP);
			self->client->ps.fd.forceGripSoundTime = level.time + 1000;
		}
	}

	if (self->client->ps.fd.forcePowersActive & (1 << FP_SPEED))
	{
		self->client->ps.powerups[PW_SPEED] = level.time + 100;
	}

	if (self->health <= 0)
	{//if dead, deactivate any active force powers
		for (i = 0; i < NUM_FORCE_POWERS; i++)
		{
			if (self->client->ps.fd.forcePowerDuration[i] || (self->client->ps.fd.forcePowersActive & (1 << i)))
			{
				WP_ForcePowerStop(self, (forcePowers_t)i);
				self->client->ps.fd.forcePowerDuration[i] = 0;
			}
		}
		goto powersetcheck;
	}

	if (self->client->ps.groundEntityNum != ENTITYNUM_NONE)
	{
		self->client->fjDidJump = qfalse;
	}

	if (self->client->ps.fd.forceJumpCharge && self->client->ps.groundEntityNum == ENTITYNUM_NONE && self->client->fjDidJump)
	{ //this was for the "charge" jump method... I guess
		if (ucmd->upmove < 10 && (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_LEVITATION))
		{
			G_MuteSound(self->client->ps.fd.killSoundEntIndex[TRACK_CHANNEL_1 - 50], CHAN_VOICE);
			self->client->ps.fd.forceJumpCharge = 0;
		}
	}

#ifndef METROID_JUMP
	else if ((ucmd->upmove > 10) && (self->client->ps.pm_flags & PMF_JUMP_HELD) && self->client->ps.groundTime && (level.time - self->client->ps.groundTime) > 150 && !BG_HasYsalamiri(level.gametype, &self->client->ps) && BG_CanUseFPNow(level.gametype, &self->client->ps, level.time, FP_LEVITATION))
	{//just charging up
		ForceJumpCharge(self, ucmd);
	}
	else if (ucmd->upmove < 10 && self->client->ps.groundEntityNum == ENTITYNUM_NONE && self->client->ps.fd.forceJumpCharge)
	{
		self->client->ps.pm_flags &= ~(PMF_JUMP_HELD);
	}
#endif

	if (!(self->client->ps.pm_flags & PMF_JUMP_HELD) && self->client->ps.fd.forceJumpCharge)
	{
		if (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_LEVITATION)
		{
			WP_DoSpecificPower(self, ucmd, FP_LEVITATION);
		}
	}

	if ((self->client->flameTime > level.time)
		|| (self->client->pers.botclass == BCLASS_MANDOLORIAN
		|| self->client->pers.botclass == BCLASS_BOBAFETT
		|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
		|| self->client->pers.botclass == BCLASS_MANDOLORIAN2) && ucmd->buttons& BUTTON_FORCE_LIGHTNING)
	{//flamethrower is active, flip active flamethrower flag

		if (self->client->ps.jetpackFuel < FLAMETHROWER_FUELCOST)
		{//not enough gas, turn it off.
			self->client->flameTime = 0;
			self->client->ps.userInt3 &= ~(1 << FLAG_FLAMETHROWER);
		}
		else
		{//fire flamethrower
			self->client->ps.userInt3 |= (1 << FLAG_FLAMETHROWER);
			self->client->ps.forceHandExtend = HANDEXTEND_FLAMETHROWER_HOLD;
			self->client->ps.forceHandExtendTime = level.time + 100;

			if (FlamethrowerDebounceTime == level.time //someone already advanced the timer this frame
				|| (level.time - FlamethrowerDebounceTime >= FLAMETHROWERDEBOUNCE))
			{
				G_Sound(self, CHAN_WEAPON, G_SoundIndex("sound/effects/fireburst"));
				Flamethrower_Fire(self);
				FlamethrowerDebounceTime = level.time;
				if (!Q_irand(0, 1))
				{
					G_AddMercBalance(self, BLASTERMISHAPLEVEL_MIN);
				}
				self->client->ps.jetpackFuel -= FLAMETHROWER_FUELCOST;
			}
		}
	}
	else
	{
		self->client->ps.userInt3 &= ~(1 << FLAG_FLAMETHROWER);
	}
	
	if (self->client->pers.botclass == BCLASS_MANDOLORIAN
		|| self->client->pers.botclass == BCLASS_BOBAFETT
		|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
		|| self->client->pers.botclass == BCLASS_MANDOLORIAN2)
	{//Boba Fett
		if (ucmd->buttons & BUTTON_FORCEGRIP)
		{//start wrist laser
			Boba_FireWristMissile(self, BOBA_MISSILE_LASER);
			return;
		}
		else if (self->client->ps.fd.forcePowerDuration[FP_GRIP])
		{
			Boba_EndWristMissile(self, BOBA_MISSILE_LASER);
			return;
		}
	}
	else if (ucmd->buttons & BUTTON_FORCEGRIP)
	{ //grip is one of the powers with its own button.. if it's held, call the specific grip power function.
		WP_DoSpecificPower(self, ucmd, FP_GRIP);
	}
	else
	{ //see if we're using it generically.. if not, stop.
		if (self->client->ps.fd.forcePowersActive & (1 << FP_GRIP))
		{
			if (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_GRIP)
			{
				WP_ForcePowerStop(self, FP_GRIP);
			}
		}
	}

	if (IsPressingDashButton(self))
	{ //dash is one of the powers with its own button.. if it's held, call the specific dash power function.
		WP_DoSpecificPower(self, ucmd, FP_SPEED);
	}
	else
	{ //see if we're using it generically.. if not, stop.
		if (self->client->ps.fd.forcePowersActive & (1 << FP_SPEED))
		{
			if (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_SPEED)
			{
				WP_ForcePowerStop(self, FP_SPEED);
			}
		}
	}

	if (ucmd->buttons & BUTTON_FORCE_LIGHTNING)
	{ //lightning
		WP_DoSpecificPower(self, ucmd, FP_LIGHTNING);
	}
	else
	{ //see if we're using it generically.. if not, stop.
		if (self->client->ps.fd.forcePowersActive & (1 << FP_LIGHTNING))
		{
			if (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_LIGHTNING)
			{
				WP_ForcePowerStop(self, FP_LIGHTNING);
			}
		}
	}

	if (ucmd->buttons & BUTTON_FORCE_DRAIN)
	{ //drain
		WP_DoSpecificPower(self, ucmd, FP_DRAIN);
	}
	else
	{ //see if we're using it generically.. if not, stop.
		if (self->client->ps.fd.forcePowersActive & (1 << FP_DRAIN))
		{
			if (!(ucmd->buttons & BUTTON_FORCEPOWER) || self->client->ps.fd.forcePowerSelected != FP_DRAIN)
			{
				WP_ForcePowerStop(self, FP_DRAIN);
			}
		}
	}

	if ((ucmd->buttons & BUTTON_FORCEPOWER) &&
		BG_CanUseFPNow(level.gametype, &self->client->ps, level.time, self->client->ps.fd.forcePowerSelected))
	{
		if (self->client->ps.fd.forcePowerSelected == FP_LEVITATION)
			ForceJumpCharge(self, ucmd);
		else
		{
			WP_DoSpecificPower(self, ucmd, self->client->ps.fd.forcePowerSelected);
		}
	}
	else
	{
		self->client->ps.fd.forceButtonNeedRelease = 0;
	}

	for (i = 0; i < NUM_FORCE_POWERS; i++)
	{
		if (self->client->ps.fd.forcePowerDuration[i])
		{
			if (self->client->ps.fd.forcePowerDuration[i] < level.time)
			{
				if ((self->client->ps.fd.forcePowersActive& (1 << i)))
				{//turn it off
					WP_ForcePowerStop(self, (forcePowers_t)i);
				}
				self->client->ps.fd.forcePowerDuration[i] = 0;
			}
		}
		if ((self->client->ps.fd.forcePowersActive & (1 << i)))
		{
			WP_ForcePowerRun(self, (forcePowers_t)i, ucmd);
		}
	}

	if (!(self->client->ps.fd.forcePowersActive & (1 << FP_DRAIN)))
	{
		self->client->force.drainDebounce = level.time;
	}
	if (!(self->client->ps.fd.forcePowersActive & (1 << FP_LIGHTNING)))
	{
		self->client->force.lightningDebounce = level.time;
	}

	if ((!self->client->ps.fd.forcePowersActive || self->client->ps.fd.forcePowersActive == (1 << FP_DRAIN)) &&
		!self->client->ps.saberInFlight && (self->client->ps.weapon != WP_SABER || !PM_SaberInSpecial(self->client->ps.saberMove)))
	{//when not using the force, regenerate at 1 point per half second

		while (self->client->ps.fd.forcePowerRegenDebounceTime < level.time)
		{
			if (level.gametype != GT_HOLOCRON || g_maxHolocronCarry.value)
			{
				if (self->client->ps.powerups[PW_FORCE_BOON])
				{
					WP_ForcePowerRegenerate(self, 6);
				}
				else if (self->client->ps.isJediMaster && level.gametype == GT_JEDIMASTER)
				{
					WP_ForcePowerRegenerate(self, 4); //jedi master regenerates 4 times as fast
				}
				else if (PM_RestAnim(self->client->ps.legsAnim))
				{
					WP_ForcePowerRegenerate(self, 10);
					BG_ReduceSaberMishapLevel(&self->client->ps);
					BG_ReduceBlasterMishapLevel(&self->client->ps);
				}
				else if (PM_CrouchAnim(self->client->ps.legsAnim))
				{
					WP_ForcePowerRegenerate(self, 8);
					BG_ReduceSaberMishapLevel(&self->client->ps);
					BG_ReduceBlasterMishapLevel(&self->client->ps);
				}
				else if (self->client->pers.cmd.buttons & BUTTON_BLOCK)
				{//regen half as fast
					self->client->ps.fd.forcePowerRegenDebounceTime += 2000;//1 point per 1 seconds.. super slow
					WP_ForcePowerRegenerate(self, 1);
				}
				else
				{
					WP_ForcePowerRegenerate(self, 0);
				}
			}
			else
			{ //regenerate based on the number of holocrons carried
				holoregen = 0;
				holo = 0;
				while (holo < NUM_FORCE_POWERS)
				{
					if (self->client->ps.holocronsCarried[holo])
						holoregen++;
					holo++;
				}

				WP_ForcePowerRegenerate(self, holoregen);
			}

			if (level.gametype == GT_SIEGE)
			{
				if (self->client->holdingObjectiveItem&& g_entities[self->client->holdingObjectiveItem].inuse&& g_entities[self->client->holdingObjectiveItem].genericValue15)
					self->client->ps.fd.forcePowerRegenDebounceTime += 7000; //1 point per 7 seconds.. super slow
				else if (self->client->siegeClass != -1 && (bgSiegeClasses[self->client->siegeClass].classflags & (1 << CFL_FASTFORCEREGEN)))
					self->client->ps.fd.forcePowerRegenDebounceTime += Q_max(g_forceRegenTime.integer * 0.2, 1); //if this is siege and our player class has the fast force regen ability, then recharge with 1/5th the usual delay
				else
					self->client->ps.fd.forcePowerRegenDebounceTime += Q_max(g_forceRegenTime.integer, 1);
			}
			else
			{
				if (level.gametype == GT_POWERDUEL && self->client->sess.duelTeam == DUELTEAM_LONE)
				{
					if (duel_fraglimit.integer)
						self->client->ps.fd.forcePowerRegenDebounceTime += Q_max(g_forceRegenTime.integer * (0.6 + (.3 * (float)self->client->sess.wins / (float)duel_fraglimit.integer)), 1);
					else
						self->client->ps.fd.forcePowerRegenDebounceTime += Q_max(g_forceRegenTime.integer * 0.7, 1);
				}
				else
					self->client->ps.fd.forcePowerRegenDebounceTime += Q_max(g_forceRegenTime.integer, 1);
			}
		}
		if (self->client->ps.fd.forcePower > (self->client->ps.fd.forcePowerMax * FATIGUEDTHRESHHOLD))
		{//You gained some FP back.  Cancel the Fatigue status.
			self->client->ps.userInt3 &= ~(1 << FLAG_FATIGUED);
		}
	}
	else
	{
		self->client->ps.fd.forcePowerRegenDebounceTime = level.time;
	}

	if (self->client->DodgeDebounce < level.time
		&& !BG_InSlowBounce(&self->client->ps) && !PM_SaberInBrokenParry(self->client->ps.saberMove)
		&& !PM_InKnockDown(&self->client->ps) && self->client->ps.forceHandExtend != HANDEXTEND_DODGE
		&& self->client->ps.saberLockTime < level.time	//not in a saber lock.
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE //can't regen while in the air.
		&& WalkCheck(self))
	{
		if ((self->client->ps.fd.forcePower > (self->client->ps.fd.forcePowerMax* FATIGUEDTHRESHHOLD) + 1)
			&& self->client->ps.stats[STAT_DODGE] < self->client->ps.stats[STAT_MAX_DODGE])
		{//you have enough fatigue to transfer to Dodge
			if (self->client->ps.stats[STAT_MAX_DODGE] - self->client->ps.stats[STAT_DODGE] < DODGE_FATIGUE)
			{
				self->client->ps.stats[STAT_DODGE] = self->client->ps.stats[STAT_MAX_DODGE];
			}
			else
			{
				self->client->ps.stats[STAT_DODGE] += DODGE_FATIGUE;
			}
			self->client->ps.fd.forcePower--;
		}

		self->client->DodgeDebounce = level.time + g_dodgeRegenTime.integer;
	}

	if (self->client->MishapDebounce < level.time
		&& !BG_InSlowBounce(&self->client->ps) && !PM_SaberInBrokenParry(self->client->ps.saberMove)
		&& !PM_InKnockDown(&self->client->ps) && self->client->ps.forceHandExtend != HANDEXTEND_DODGE
		&& self->client->ps.saberLockTime < level.time	//not in a saber lock.
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE)  //can't regen while in the air.
	{
		if (self->client->ps.saberAttackChainCount > MISHAPLEVEL_NONE)
		{
			self->client->ps.saberAttackChainCount--;
		}

		if (self->client->ps.weapon == WP_SABER)
		{//saberer regens slower since they use MP differently
			if (self->client->ps.fd.saberAnimLevel == SS_MEDIUM)
			{//yellow style is more "centered" and recovers MP faster.
				self->client->MishapDebounce = level.time + (g_mishapRegenTime.integer * .75);
			}
			else
			{
				self->client->MishapDebounce = level.time + g_mishapRegenTime.integer;
			}
		}
		else
		{//gunner regen faster
			self->client->MishapDebounce = level.time + g_mishapRegenTime.integer / 5;
		}
	}

powersetcheck:

	if (prepower && self->client->ps.fd.forcePower < prepower)
	{
		int dif = ((prepower - self->client->ps.fd.forcePower) / 2);
		if (dif < 1)
		{
			dif = 1;
		}

		self->client->ps.fd.forcePower = (prepower - dif);
	}
}

qboolean Jedi_DodgeEvasion(gentity_t * self, gentity_t * shooter, trace_t * tr, int hitLoc)
{
	int	dodgeAnim = -1;

	if (in_camera)
	{
		return qfalse;
	}

	if (!self || !self->client || self->health <= 0)
	{
		return qfalse;
	}

	if (self->NPC
		&& (self->client->NPC_class == CLASS_STORMTROOPER ||
			self->client->NPC_class == CLASS_STORMCOMMANDO ||
			self->client->NPC_class == CLASS_SWAMPTROOPER ||
			self->client->NPC_class == CLASS_CLONETROOPER ||
			self->client->NPC_class == CLASS_IMPWORKER ||
			self->client->NPC_class == CLASS_IMPERIAL ||
			self->client->NPC_class == CLASS_SABER_DROID ||
			self->client->NPC_class == CLASS_ASSASSIN_DROID ||
			self->client->NPC_class == CLASS_GONK ||
			self->client->NPC_class == CLASS_MOUSE ||
			self->client->NPC_class == CLASS_PROBE ||
			self->client->NPC_class == CLASS_PROTOCOL ||
			self->client->NPC_class == CLASS_R2D2 ||
			self->client->NPC_class == CLASS_R5D2 ||
			self->client->NPC_class == CLASS_SEEKER ||
			self->client->NPC_class == CLASS_INTERROGATOR))
	{// don't get Dodge.
		return qfalse;
	}

	if (self->r.svFlags & SVF_BOT
		&& (self->client->botclass == BCLASS_SBD ||
			self->client->botclass == BCLASS_BATTLEDROID ||
			self->client->botclass == BCLASS_DROIDEKA ||
			self->client->botclass == BCLASS_PROTOCOL ||
			self->client->botclass == BCLASS_JAWA))
	{// don't get Dodge.
		return qfalse;
	}

	if (self->client->ps.weapon == WP_BRYAR_OLD)
	{
		return qfalse;//dont do 
	}

	if (!g_forceDodge.integer)
	{
		return qfalse;
	}

	if (g_forceDodge.integer != 2)
	{
		if (!(self->client->ps.fd.forcePowersActive & (1 << FP_SEE)))
		{
			return qfalse;
		}
	}

	//check for private duel conditions
	if (shooter && shooter->client)
	{
		if (shooter->client->ps.duelInProgress && shooter->client->ps.duelIndex != self->s.number)
		{//enemy client is in duel with someone else.
			return qfalse;
		}

		if (self->client->ps.duelInProgress && self->client->ps.duelIndex != shooter->s.number)
		{//we're in a duel with someone else.
			return qfalse;
		}
	}

	if (PM_InKnockDown(&self->client->ps) ||
		(BG_InRoll(&self->client->ps, self->client->ps.legsAnim) ||
		(BG_InKnockDown(self->client->ps.legsAnim))))
	{//can't Dodge while knocked down or getting up from knockdown.
		return qfalse;
	}

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{//can't dodge in mid-air
		return qfalse;
	}

	if (self->client->ps.forceHandExtend == HANDEXTEND_CHOKE
		|| BG_InGrappleMove(self->client->ps.torsoAnim) > 1)
	{//in some effect that stops me from moving on my own
		return qfalse;
	}

	if (self->client->ps.legsAnim == BOTH_MEDITATE)
	{//can't dodge while meditating.
		return qfalse;
	}

	if (self->client->ps.legsAnim == BOTH_MEDITATE1)
	{//can't dodge while meditating.
		return qfalse;
	}

	if (self->client->ps.weapon == WP_SABER)
	{
		return qfalse;//dont do THIS IS FOR GUNNERS ONLY NOW LIKE SP
	}

	if (self->client->ps.userInt3 |= (1 << FLAG_FROZEN))
	{
		return qfalse;
	}

	if (PM_InKnockDown(&self->client->ps) ||
		(BG_InRoll(&self->client->ps, self->client->ps.legsAnim) ||
		(BG_InKnockDown(self->client->ps.legsAnim))))
	{//can't Dodge while knocked down or getting up from knockdown.
		return qfalse;
	}

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{//can't dodge in mid-air
		return qfalse;
	}

	if (self->r.svFlags & SVF_BOT
		&& (self->client->botclass != BCLASS_MANDOLORIAN &&
			self->client->botclass != BCLASS_MANDOLORIAN1 &&
			self->client->botclass != BCLASS_MANDOLORIAN2 &&
			self->client->botclass != BCLASS_BOBAFETT) &&
		self->client->ps.fd.forcePower < FATIGUE_DODGEINGBOT)
	{//must have enough force power
		return qfalse;
	}
	else if (self->client->ps.fd.forcePower < FATIGUE_DODGE)
	{//Not enough dodge
		return qfalse;
	}

	if (self->r.svFlags & SVF_BOT
		&& (self->client->botclass != BCLASS_MANDOLORIAN &&
			self->client->botclass != BCLASS_MANDOLORIAN1 &&
			self->client->botclass != BCLASS_MANDOLORIAN2 &&
			self->client->botclass != BCLASS_BOBAFETT))
	{
		BG_AddFatigue(&self->client->ps, FATIGUE_DODGEINGBOT);
	}
	else
	{
		BG_AddFatigue(&self->client->ps, FATIGUE_DODGEING);
	}

	if (self->client->ps.weaponTime > 0 || self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{//in some effect that stops me from moving on my own
		return qfalse;
	}


	if (g_forceDodge.integer == 2)
	{
		if (Q_irand(1, 7) > self->client->ps.fd.forcePowerLevel[FP_SPEED])
		{//more likely to fail on lower force speed level
			return qfalse;
		}
		if (!WP_ForcePowerUsable(self, FP_SPEED))
		{//make sure we have it and have enough force power
			return qfalse;
		}
		if (self->client->ps.fd.forcePowersActive)
		{ //for now just don't let us dodge if we're using a force power at all
			return qfalse;
		}
	}
	else
	{
		//We now dodge all the time, but only on level 3
		if (self->client->ps.fd.forcePowerLevel[FP_SEE] < FORCE_LEVEL_3)
		{//more likely to fail on lower force sight level
			return qfalse;
		}
	}

	switch (hitLoc)
	{
	case HL_NONE:
		return qfalse;
		break;

	case HL_FOOT_RT:
	case HL_FOOT_LT:
		dodgeAnim = Q_irand(BOTH_HOP_L, BOTH_HOP_R);
		break;
	case HL_LEG_RT:
	case HL_LEG_LT:
		if (self->client->pers.botclass == BCLASS_MANDOLORIAN
			|| self->client->pers.botclass == BCLASS_BOBAFETT
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN2)
		{
			self->client->jetPackOn = qtrue;
			self->client->ps.eFlags |= EF_JETPACK_ACTIVE;
			self->client->ps.eFlags |= EF_JETPACK_FLAMING;
			self->client->ps.eFlags |= EF_JETPACK_HOVER;
			Boba_FlyStart(self);
			self->client->ps.fd.forceJumpCharge = 280;
			self->client->jetPackTime = level.time + 30000;
		}
		else
		{
			dodgeAnim = Q_irand(BOTH_DASH_L, BOTH_DASH_R);
		}
		break;

	case HL_BACK_RT:
		dodgeAnim = BOTH_DODGE_FL;
		break;
	case HL_CHEST_RT:
		dodgeAnim = BOTH_DODGE_FR;
		break;
	case HL_BACK_LT:
		dodgeAnim = BOTH_DODGE_FR;
		break;
	case HL_CHEST_LT:
		dodgeAnim = BOTH_DODGE_FR;
		break;
	case HL_BACK:
		if (self->client->pers.botclass == BCLASS_MANDOLORIAN
			|| self->client->pers.botclass == BCLASS_BOBAFETT
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN2)
		{
			self->client->jetPackOn = qtrue;
			self->client->ps.eFlags |= EF_JETPACK_ACTIVE;
			self->client->ps.eFlags |= EF_JETPACK_FLAMING;
			self->client->ps.eFlags |= EF_JETPACK_HOVER;
			Boba_FlyStart(self);
			self->client->ps.fd.forceJumpCharge = 280;
			self->client->jetPackTime = ((self->client->jetPackTime + level.time) / 2) + 10000;
		}
		else
		{
			dodgeAnim = Q_irand(BOTH_DODGE_FL, BOTH_DODGE_FR);
		}
		break;
	case HL_CHEST:
		if (self->client->pers.botclass == BCLASS_MANDOLORIAN
			|| self->client->pers.botclass == BCLASS_BOBAFETT
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN2)
		{
			self->client->jetPackOn = qtrue;
			self->client->ps.eFlags |= EF_JETPACK_ACTIVE;
			self->client->ps.eFlags |= EF_JETPACK_FLAMING;
			self->client->ps.eFlags |= EF_JETPACK_HOVER;
			Boba_FlyStart(self);
			self->client->ps.fd.forceJumpCharge = 280;
			self->client->jetPackTime = ((self->client->jetPackTime + level.time) / 2) + 10000;
		}
		else
		{
			dodgeAnim = BOTH_DODGE_B;
		}
		break;
	case HL_WAIST:
		if (self->client->pers.botclass == BCLASS_MANDOLORIAN
			|| self->client->pers.botclass == BCLASS_BOBAFETT
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN2)
		{
			self->client->jetPackOn = qtrue;
			self->client->ps.eFlags |= EF_JETPACK_ACTIVE;
			self->client->ps.eFlags |= EF_JETPACK_FLAMING;
			self->client->ps.eFlags |= EF_JETPACK_HOVER;
			Boba_FlyStart(self);
			self->client->ps.fd.forceJumpCharge = 280;
			self->client->jetPackTime = ((self->client->jetPackTime + level.time) / 2) + 10000;
		}
		else
		{
			dodgeAnim = Q_irand(BOTH_DODGE_L, BOTH_DODGE_R);
		}
		break;
	case HL_ARM_RT:
	case HL_HAND_RT:
		dodgeAnim = BOTH_DODGE_L;
		break;
	case HL_ARM_LT:
	case HL_HAND_LT:
		dodgeAnim = BOTH_DODGE_R;
		break;
	case HL_HEAD:
		dodgeAnim = BOTH_CROUCHDODGE;
		break;
	default:
		return qfalse;
	}

	if (dodgeAnim != -1)
	{
		//Our own happy way of forcing an anim:
		self->client->ps.forceHandExtend = HANDEXTEND_DODGE;
		self->client->ps.forceDodgeAnim = dodgeAnim;
		self->client->ps.forceHandExtendTime = level.time + 300;
		self->client->ps.weaponTime = 300;
		self->client->ps.saberMove = LS_NONE;
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/melee/swing4.wav"));
		return qtrue;
	}
	return qfalse;
}

qboolean Jedi_DisruptorDodgeEvasion(gentity_t * self, gentity_t * shooter, trace_t * tr, int hitLoc)
{
	int	dodgeAnim = -1;

	if (!self || !self->client || self->health <= 0)
	{
		return qfalse;
	}

	if (self->NPC
		&& (self->client->NPC_class == CLASS_STORMTROOPER ||
			self->client->NPC_class == CLASS_STORMCOMMANDO ||
			self->client->NPC_class == CLASS_SWAMPTROOPER ||
			self->client->NPC_class == CLASS_CLONETROOPER ||
			self->client->NPC_class == CLASS_IMPWORKER ||
			self->client->NPC_class == CLASS_IMPERIAL ||
			self->client->NPC_class == CLASS_BATTLEDROID ||
			self->client->NPC_class == CLASS_SBD ||
			self->client->NPC_class == CLASS_SABER_DROID ||
			self->client->NPC_class == CLASS_SBD ||
			self->client->NPC_class == CLASS_ASSASSIN_DROID ||
			self->client->NPC_class == CLASS_GONK ||
			self->client->NPC_class == CLASS_MOUSE ||
			self->client->NPC_class == CLASS_PROBE ||
			self->client->NPC_class == CLASS_PROTOCOL ||
			self->client->NPC_class == CLASS_R2D2 ||
			self->client->NPC_class == CLASS_R5D2 ||
			self->client->NPC_class == CLASS_SEEKER ||
			self->client->NPC_class == CLASS_INTERROGATOR))
	{// don't get Dodge.
		return qfalse;
	}

	if (self->r.svFlags & SVF_BOT
		&& (self->client->botclass == BCLASS_SBD ||
			self->client->botclass == BCLASS_BATTLEDROID ||
			self->client->botclass == BCLASS_DROIDEKA ||
			self->client->botclass == BCLASS_PROTOCOL ||
			self->client->botclass == BCLASS_JAWA))
	{// don't get Dodge.
		return qfalse;
	}

	if (self->client->ps.weapon == WP_BRYAR_OLD)
	{
		return qfalse;//dont do 
	}

	if (self->client->ps.userInt3 |= (1 << FLAG_FROZEN))
	{
		return qfalse;
	}

	if (!g_forceDodge.integer)
	{
		return qfalse;
	}

	if (g_forceDodge.integer != 2)
	{
		if (!(self->client->ps.fd.forcePowersActive & (1 << FP_SEE)))
		{
			return qfalse;
		}
	}

	//check for private duel conditions
	if (shooter && shooter->client)
	{
		if (shooter->client->ps.duelInProgress && shooter->client->ps.duelIndex != self->s.number)
		{//enemy client is in duel with someone else.
			return qfalse;
		}

		if (self->client->ps.duelInProgress && self->client->ps.duelIndex != shooter->s.number)
		{//we're in a duel with someone else.
			return qfalse;
		}
	}

	if (PM_InKnockDown(&self->client->ps) ||
		(BG_InRoll(&self->client->ps, self->client->ps.legsAnim) ||
		(BG_InKnockDown(self->client->ps.legsAnim))))
	{//can't Dodge while knocked down or getting up from knockdown.
		return qfalse;
	}

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{//can't dodge in mid-air
		return qfalse;
	}

	if (self->client->ps.forceHandExtend == HANDEXTEND_CHOKE
		|| BG_InGrappleMove(self->client->ps.torsoAnim) > 1)
	{//in some effect that stops me from moving on my own
		return qfalse;
	}

	if (self->client->ps.legsAnim == BOTH_MEDITATE)
	{//can't dodge while meditating.
		return qfalse;
	}

	if (self->client->ps.legsAnim == BOTH_MEDITATE1)
	{//can't dodge while meditating.
		return qfalse;
	}

	if (PM_InKnockDown(&self->client->ps) ||
		(BG_InRoll(&self->client->ps, self->client->ps.legsAnim) ||
		(BG_InKnockDown(self->client->ps.legsAnim))))
	{//can't Dodge while knocked down or getting up from knockdown.
		return qfalse;
	}

	if (self->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{//can't dodge in mid-air
		return qfalse;
	}

	if (self->r.svFlags & SVF_BOT
		&& (self->client->botclass != BCLASS_MANDOLORIAN &&
			self->client->botclass != BCLASS_MANDOLORIAN1 &&
			self->client->botclass != BCLASS_MANDOLORIAN2 &&
			self->client->botclass != BCLASS_BOBAFETT) &&
		self->client->ps.fd.forcePower < FATIGUE_DODGEINGBOT)
	{//must have enough force power
		return qfalse;
	}
	else if (self->client->ps.fd.forcePower < FATIGUE_DODGE)
	{//Not enough dodge
		return qfalse;
	}

	if (self->r.svFlags & SVF_BOT
		&& (self->client->botclass != BCLASS_MANDOLORIAN &&
			self->client->botclass != BCLASS_MANDOLORIAN1 &&
			self->client->botclass != BCLASS_MANDOLORIAN2 &&
			self->client->botclass != BCLASS_BOBAFETT))
	{
		BG_AddFatigue(&self->client->ps, FATIGUE_DODGEINGBOT);
	}
	else
	{
		BG_AddFatigue(&self->client->ps, FATIGUE_DODGEING);
	}

	if (self->client->ps.weaponTime > 0 || self->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{//in some effect that stops me from moving on my own
		return qfalse;
	}


	if (g_forceDodge.integer == 2)
	{
		if (Q_irand(1, 7) > self->client->ps.fd.forcePowerLevel[FP_SPEED])
		{//more likely to fail on lower force speed level
			return qfalse;
		}
		if (!WP_ForcePowerUsable(self, FP_SPEED))
		{//make sure we have it and have enough force power
			return qfalse;
		}
		if (self->client->ps.fd.forcePowersActive)
		{ //for now just don't let us dodge if we're using a force power at all
			return qfalse;
		}
	}
	else
	{
		//We now dodge all the time, but only on level 3
		if (self->client->ps.fd.forcePowerLevel[FP_SEE] < FORCE_LEVEL_3)
		{//more likely to fail on lower force sight level
			return qfalse;
		}
	}

	switch (hitLoc)
	{
	case HL_NONE:
		return qfalse;
		break;

	case HL_FOOT_RT:
	case HL_FOOT_LT:
		dodgeAnim = Q_irand(BOTH_HOP_L, BOTH_HOP_R);
		break;
	case HL_LEG_RT:
	case HL_LEG_LT:
		if (self->client->pers.botclass == BCLASS_MANDOLORIAN
			|| self->client->pers.botclass == BCLASS_BOBAFETT
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN2)
		{
			self->client->jetPackOn = qtrue;
			self->client->ps.eFlags |= EF_JETPACK_ACTIVE;
			self->client->ps.eFlags |= EF_JETPACK_FLAMING;
			self->client->ps.eFlags |= EF_JETPACK_HOVER;
			Boba_FlyStart(self);
			self->client->ps.fd.forceJumpCharge = 280;
			self->client->jetPackTime = level.time + 30000;
		}
		else
		{
			dodgeAnim = Q_irand(BOTH_DASH_L, BOTH_DASH_R);
		}
		break;

	case HL_BACK_RT:
		dodgeAnim = BOTH_DODGE_FL;
		break;
	case HL_CHEST_RT:
		dodgeAnim = BOTH_DODGE_FR;
		break;
	case HL_BACK_LT:
		dodgeAnim = BOTH_DODGE_FR;
		break;
	case HL_CHEST_LT:
		dodgeAnim = BOTH_DODGE_FR;
		break;
	case HL_BACK:
		if (self->client->pers.botclass == BCLASS_MANDOLORIAN
			|| self->client->pers.botclass == BCLASS_BOBAFETT
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN2)
		{
			self->client->jetPackOn = qtrue;
			self->client->ps.eFlags |= EF_JETPACK_ACTIVE;
			self->client->ps.eFlags |= EF_JETPACK_FLAMING;
			self->client->ps.eFlags |= EF_JETPACK_HOVER;
			Boba_FlyStart(self);
			self->client->ps.fd.forceJumpCharge = 280;
			self->client->jetPackTime = ((self->client->jetPackTime + level.time) / 2) + 10000;
		}
		else
		{
			dodgeAnim = Q_irand(BOTH_DODGE_FL, BOTH_DODGE_FR);
		}
		break;
	case HL_CHEST:
		if (self->client->pers.botclass == BCLASS_MANDOLORIAN
			|| self->client->pers.botclass == BCLASS_BOBAFETT
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN2)
		{
			self->client->jetPackOn = qtrue;
			self->client->ps.eFlags |= EF_JETPACK_ACTIVE;
			self->client->ps.eFlags |= EF_JETPACK_FLAMING;
			self->client->ps.eFlags |= EF_JETPACK_HOVER;
			Boba_FlyStart(self);
			self->client->ps.fd.forceJumpCharge = 280;
			self->client->jetPackTime = ((self->client->jetPackTime + level.time) / 2) + 10000;
		}
		else
		{
			dodgeAnim = BOTH_DODGE_B;
		}
		break;
	case HL_WAIST:
		if (self->client->pers.botclass == BCLASS_MANDOLORIAN
			|| self->client->pers.botclass == BCLASS_BOBAFETT
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN1
			|| self->client->pers.botclass == BCLASS_MANDOLORIAN2)
		{
			self->client->jetPackOn = qtrue;
			self->client->ps.eFlags |= EF_JETPACK_ACTIVE;
			self->client->ps.eFlags |= EF_JETPACK_FLAMING;
			self->client->ps.eFlags |= EF_JETPACK_HOVER;
			Boba_FlyStart(self);
			self->client->ps.fd.forceJumpCharge = 280;
			self->client->jetPackTime = ((self->client->jetPackTime + level.time) / 2) + 10000;
		}
		else
		{
			dodgeAnim = Q_irand(BOTH_DODGE_L, BOTH_DODGE_R);
		}
		break;
	case HL_ARM_RT:
	case HL_HAND_RT:
		dodgeAnim = BOTH_DODGE_L;
		break;
	case HL_ARM_LT:
	case HL_HAND_LT:
		dodgeAnim = BOTH_DODGE_R;
		break;
	case HL_HEAD:
		dodgeAnim = BOTH_CROUCHDODGE;
		break;
	default:
		return qfalse;
	}

	if (dodgeAnim != -1)
	{
		//Our own happy way of forcing an anim:
		self->client->ps.forceHandExtend = HANDEXTEND_DODGE;
		self->client->ps.forceDodgeAnim = dodgeAnim;
		self->client->ps.forceHandExtendTime = level.time + 300;
		self->client->ps.weaponTime = 300;
		self->client->ps.saberMove = LS_NONE;
		G_Sound(self, CHAN_BODY, G_SoundIndex("sound/weapons/melee/swing4.wav"));
		return qtrue;
	}
	return qfalse;
}
