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

#include "g_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "bg_local.h"
#include "../cgame/cg_local.h"
#include "b_local.h"

#ifdef _DEBUG
#include <float.h>
#endif //_DEBUG

extern qboolean InFront(vec3_t spot, vec3_t from, vec3_t fromAngles, float threshHold = 0.0f);
qboolean LogAccuracyHit(gentity_t* target, gentity_t* attacker);
extern qboolean PM_SaberInParry(int move);
extern qboolean PM_SaberInReflect(int move);
extern qboolean PM_SaberInIdle(int move);
extern qboolean PM_SaberInAttack(int move);
extern qboolean PM_SaberInTransitionAny(int move);
extern qboolean PM_SaberInSpecialAttack(int anim);
extern qboolean WP_DoingForcedAnimationForForcePowers(gentity_t* self);
extern qboolean G_ControlledByPlayer(gentity_t* self);
extern qboolean PM_SaberInStart(int move);
extern qboolean PM_SaberInReturn(int move);
extern qboolean WP_SaberBlockNonRandom(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
extern int WP_SaberMustBlock(gentity_t* self, gentity_t* atk, qboolean checkBBoxBlock, vec3_t point, int rSaberNum, int rBladeNum);
extern float VectorBlockDistance(vec3_t v1, vec3_t v2);
extern int WP_SaberBlockCost(gentity_t* defender, gentity_t* attacker, vec3_t hitLoc);
vec3_t g_crosshairWorldCoord = { 0, 0, 0 };
extern int Manual_Saberblocking(gentity_t* defender);
extern qboolean PM_RunningAnim(int anim);
void WP_HandleBoltBlock(gentity_t * bolt, gentity_t * blocker, trace_t * trace);
extern qboolean	PM_WalkingOrStanding(gentity_t* self);
extern void Jedi_Decloak(gentity_t* self);
extern void player_Decloak(gentity_t* self);
extern void PM_AddBlockFatigue(playerState_t* ps, int Fatigue);
extern gentity_t* Jedi_FindEnemyInCone(gentity_t* self, gentity_t* fallback, float minDot);
extern qboolean FighterIsLanded(Vehicle_t* pVeh, playerState_t* parentPS);

//-------------------------------------------------------------------------
void G_MissileBounceEffect(gentity_t* ent, vec3_t org, vec3_t dir, qboolean hitWorld)
{
	switch (ent->s.weapon)
	{
	case WP_BOWCASTER:
		if (hitWorld)
		{
			G_PlayEffect("bowcaster/bounce_wall", org, dir);
		}
		else
		{
			G_PlayEffect("bowcaster/deflect", ent->currentOrigin, dir);
		}
		break;
	case WP_BLASTER:
	case WP_BRYAR_PISTOL:
	case WP_BLASTER_PISTOL:
		G_PlayEffect("blaster/deflect", ent->currentOrigin, dir);
		break;
	case WP_REPEATER:
		G_PlayEffect("repeater/deflectblock", ent->currentOrigin, dir);
		break;
	default:
	{
		gentity_t* tent = G_TempEntity(org, EV_GRENADE_BOUNCE);
		VectorCopy(dir, tent->pos1);
		tent->s.weapon = ent->s.weapon;
	}
	break;
	}
}

void G_MissileReflectEffect(gentity_t* ent, vec3_t org, vec3_t dir)
{
	switch (ent->s.weapon)
	{
	case WP_BOWCASTER:
		G_PlayEffect("bowcaster/deflect", ent->currentOrigin, dir);
		break;
	case WP_BLASTER:
	case WP_BRYAR_PISTOL:
	case WP_BLASTER_PISTOL:
		G_PlayEffect("blaster/deflect", ent->currentOrigin, dir);
		break;
	case WP_REPEATER:
		G_PlayEffect("repeater/deflectblock", ent->currentOrigin, dir);
		break;
	default:
		G_PlayEffect("blaster/deflect", ent->currentOrigin, dir);
		break;
	}
}

//-------------------------------------------------------------------------
static void G_MissileStick(gentity_t* missile, gentity_t* other, trace_t* tr)
{
	if (other->NPC || !Q_stricmp(other->classname, "misc_model_breakable"))
	{
		vec3_t velocity;

		int hitTime = level.previousTime + (level.time - level.previousTime) * tr->fraction;

		EvaluateTrajectoryDelta(&missile->s.pos, hitTime, velocity);

		float dot = DotProduct(velocity, tr->plane.normal);
		G_SetOrigin(missile, tr->endpos);
		VectorMA(velocity, -1.6f * dot, tr->plane.normal, missile->s.pos.trDelta);
		VectorMA(missile->s.pos.trDelta, 10, tr->plane.normal, missile->s.pos.trDelta);
		missile->s.pos.trTime = level.time - 10; // move a bit on the first frame

		// check for stop
		if (tr->entityNum >= 0 && tr->entityNum < ENTITYNUM_WORLD &&
			tr->plane.normal[2] > 0.7 && missile->s.pos.trDelta[2] < 40) //this can happen even on very slightly sloped walls, so changed it from > 0 to > 0.7
		{
			missile->nextthink = level.time + 100;
		}
		else
		{
			// fall till we hit the ground
			missile->s.pos.trType = TR_GRAVITY;
		}

		return; // don't stick yet
	}

	if (missile->e_TouchFunc != touchF_NULL)
	{
		GEntity_TouchFunc(missile, other, tr);
	}

	G_AddEvent(missile, EV_MISSILE_STICK, 0);

	if (other->s.eType == ET_MOVER || other->e_DieFunc == dieF_funcBBrushDie || other->e_DieFunc == dieF_funcGlassDie)
	{
		// movers and breakable brushes need extra info...so sticky missiles can ride lifts and blow up when the thing they are attached to goes away.
		missile->s.groundEntityNum = tr->entityNum;
	}
}

/*
================
G_ReflectMissile

  Reflect the missile roughly back at it's owner
================
*/

void G_ReflectMissileNPC(gentity_t* ent, gentity_t* missile, vec3_t forward)
{
	vec3_t	bounce_dir;
	int		i;
	float	speed;
	qboolean reflected = qfalse;
	gentity_t* owner = ent;

	if (ent->owner)
	{
		owner = ent->owner;
	}

	//save the original speed
	speed = VectorNormalize(missile->s.pos.trDelta);

	if (ent && owner && owner->client && !owner->client->ps.saberInFlight)
	{//if high enough defense skill and saber in-hand (100% at level 3, 25% at level 2, 0% at level 1), reflections are perfectly deflected toward an enemy
		gentity_t* enemy;
		if (owner->enemy && Q_irand(0, 3))
		{//toward current enemy 75% of the time
			enemy = owner->enemy;
		}
		else
		{//find another enemy
			enemy = Jedi_FindEnemyInCone(owner, owner->enemy, 0.3f);
		}
		if (enemy)
		{
			vec3_t	bullseye;
			CalcEntitySpot(enemy, SPOT_CHEST, bullseye);
			bullseye[0] += Q_irand(-4, 4);
			bullseye[1] += Q_irand(-4, 4);
			bullseye[2] += Q_irand(-16, 4);
			VectorSubtract(bullseye, missile->currentOrigin, bounce_dir);
			VectorNormalize(bounce_dir);
			if (!PM_SaberInParry(owner->client->ps.saberMove)
				&& !PM_SaberInReflect(owner->client->ps.saberMove)
				&& !PM_SaberInIdle(owner->client->ps.saberMove))
			{//a bit more wild
				if (PM_SaberInAttack(owner->client->ps.saberMove)
					|| PM_SaberInTransitionAny(owner->client->ps.saberMove)
					|| PM_SaberInSpecialAttack(owner->client->ps.torsoAnim))
				{//moderately more wild
					for (i = 0; i < 3; i++)
					{
						bounce_dir[i] += Q_flrand(-0.2f, 0.2f);
					}
				}
				else
				{//mildly more wild
					for (i = 0; i < 3; i++)
					{
						bounce_dir[i] += Q_flrand(-0.1f, 0.1f);
					}
				}
			}
			VectorNormalize(bounce_dir);
			reflected = qtrue;
		}
	}
	if (!reflected)
	{
		if (owner->s.clientNum >= MAX_CLIENTS && !G_ControlledByPlayer(owner)) //not blocked but it hit the saber totally wild return
		{//bounce back at them if you can
			VectorSubtract(missile->owner->currentOrigin, missile->currentOrigin, bounce_dir);
			VectorNormalize(bounce_dir);
		}
		else
		{
			vec3_t missile_dir;

			VectorSubtract(ent->currentOrigin, missile->currentOrigin, missile_dir);
			VectorCopy(missile->s.pos.trDelta, bounce_dir);
			VectorScale(bounce_dir, DotProduct(forward, missile_dir), bounce_dir);
			VectorNormalize(bounce_dir);
		}
		if (owner->s.weapon == WP_SABER && owner->client)
		{//saber
			if (owner->client->ps.saberInFlight)
			{//reflecting off a thrown saber is totally wild
				for (i = 0; i < 3; i++)
				{
					bounce_dir[i] += Q_flrand(-0.8f, 0.8f);
				}
			}
			else if (owner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] <= FORCE_LEVEL_1)
			{// at level 1
				for (i = 0; i < 3; i++)
				{
					bounce_dir[i] += Q_flrand(-0.4f, 0.4f);
				}
			}
			else
			{// at level 2
				for (i = 0; i < 3; i++)
				{
					bounce_dir[i] += Q_flrand(-0.2f, 0.2f);
				}
			}
			if (!PM_SaberInParry(owner->client->ps.saberMove)
				&& !PM_SaberInReflect(owner->client->ps.saberMove)
				&& !PM_SaberInIdle(owner->client->ps.saberMove))
			{//a bit more wild
				if (PM_SaberInAttack(owner->client->ps.saberMove)
					|| PM_SaberInTransitionAny(owner->client->ps.saberMove)
					|| PM_SaberInSpecialAttack(owner->client->ps.torsoAnim))
				{//really wild
					for (i = 0; i < 3; i++)
					{
						bounce_dir[i] += Q_flrand(-0.3f, 0.3f);
					}
				}
				else
				{//mildly more wild
					for (i = 0; i < 3; i++)
					{
						bounce_dir[i] += Q_flrand(-0.1f, 0.1f);
					}
				}
			}
		}
		else
		{//some other kind of reflection
			for (i = 0; i < 3; i++)
			{
				bounce_dir[i] += Q_flrand(-0.2f, 0.2f);
			}
		}
	}
	VectorNormalize(bounce_dir);
	VectorScale(bounce_dir, speed, missile->s.pos.trDelta);
#ifdef _DEBUG
	assert(!Q_isnan(missile->s.pos.trDelta[0]) && !Q_isnan(missile->s.pos.trDelta[1]) && !Q_isnan(missile->s.pos.trDelta[2]));
#endif// _DEBUG
	missile->s.pos.trTime = level.time - 10;		// move a bit on the very first frame
	VectorCopy(missile->currentOrigin, missile->s.pos.trBase);
	if (missile->s.weapon != WP_SABER)
	{//you are mine, now!
		if (!missile->lastEnemy)
		{//remember who originally shot this missile
			missile->lastEnemy = missile->owner;
		}
		missile->owner = owner;
	}
	if (missile->s.weapon == WP_ROCKET_LAUNCHER)
	{//stop homing
		missile->e_ThinkFunc = thinkF_NULL;
	}
}

/*
================
G_ReflectMissile

  Reflect the missile roughly back at it's owner
================
*/

void G_ReflectMissile(gentity_t* ent, gentity_t* missile, vec3_t forward, forcePowers_t powerToUse) //COMES FROM PUSH OR SOME WAY ELSE SO ITS NOT ACCURATE
{
	vec3_t	bounce_dir;
	int		i;
	float	speed;
	qboolean perfectReflection = qfalse;
	qboolean reflected = qfalse;
	gentity_t* owner = ent;

	if (ent->owner)
	{
		owner = ent->owner;
	}

	//save the original speed
	speed = VectorNormalize(missile->s.pos.trDelta);

	if (ent && owner && owner->client &&
		(owner->client->ps.forcePowerLevel[powerToUse] > FORCE_LEVEL_2 ||
		(owner->client->ps.forcePowerLevel[powerToUse] > FORCE_LEVEL_1 &&
		!Q_irand(0, 3))))
	{
		perfectReflection = qtrue;
	}

	if (powerToUse == FP_SABER_DEFENSE)
	{
		if (owner->client->ps.saberInFlight)
		{//but need saber in-hand for perfect reflection
			perfectReflection = qfalse;
		}

		if (owner->client->ps.saberBlockingTime < level.time)
		{//but need to be blocking for perfect reflection on higher difficulties
			perfectReflection = qfalse;
		}

		if (!PM_SaberInParry(owner->client->ps.saberMove))
		{//but need to be blocking for perfect reflection on higher difficulties
			perfectReflection = qfalse;
		}

		if (!PM_WalkingOrStanding(owner))
		{//but need to be blocking for perfect reflection on higher difficulties
			perfectReflection = qfalse;
		}
	}

	if (perfectReflection)
	{
		if (owner->s.clientNum >= MAX_CLIENTS && !G_ControlledByPlayer(owner)) //not blocked but it hit the saber totally wild return
		{
			gentity_t* enemy;
			if (owner->enemy && Q_irand(0, 3))
			{//toward current enemy 75% of the time
				enemy = owner->enemy;
			}
			else
			{//find another enemy
				enemy = Jedi_FindEnemyInCone(owner, owner->enemy, 0.3f);
			}
			if (enemy)
			{
				vec3_t	bullseye;
				CalcEntitySpot(enemy, SPOT_CHEST, bullseye);
				bullseye[0] += Q_irand(-4, 4);
				bullseye[1] += Q_irand(-4, 4);
				bullseye[2] += Q_irand(-16, 4);
				VectorSubtract(bullseye, missile->currentOrigin, bounce_dir);
				VectorNormalize(bounce_dir);
				if (!PM_SaberInParry(owner->client->ps.saberMove)
					&& !PM_SaberInReflect(owner->client->ps.saberMove)
					&& !PM_SaberInIdle(owner->client->ps.saberMove))
				{//a bit more wild
					if (PM_SaberInAttack(owner->client->ps.saberMove)
						|| PM_SaberInTransitionAny(owner->client->ps.saberMove)
						|| PM_SaberInSpecialAttack(owner->client->ps.torsoAnim))
					{//moderately more wild
						for (i = 0; i < 3; i++)
						{
							bounce_dir[i] += Q_flrand(-0.2f, 0.2f);
						}
					}
					else
					{//mildly more wild
						for (i = 0; i < 3; i++)
						{
							bounce_dir[i] += Q_flrand(-0.1f, 0.1f);
						}
					}
				}
				VectorNormalize(bounce_dir);
				reflected = qtrue;
			}
		}
		else //or by where the crosshair is.
		{
			VectorSubtract(g_crosshairWorldCoord, missile->currentOrigin, bounce_dir);
			VectorNormalize(bounce_dir);

			if (!PM_SaberInParry(owner->client->ps.saberMove)
				&& !PM_SaberInReflect(owner->client->ps.saberMove)
				&& !PM_SaberInIdle(owner->client->ps.saberMove))
			{//a bit more wild
				if (PM_SaberInAttack(owner->client->ps.saberMove)
					|| PM_SaberInTransitionAny(owner->client->ps.saberMove)
					|| PM_SaberInSpecialAttack(owner->client->ps.torsoAnim))
				{//moderately more wild
					for (i = 0; i < 3; i++)
					{
						float randomFloat = (level.time - ent->client->saberProjBlockTime) / 2000.0f;
						float minimum;
						float result;
						int randomYesNo = Q_irand(0, 1);

						if (randomFloat <= 0.05f)
						{
							randomFloat = 0;
						}
						else if (randomFloat > 0.8f)
						{
							randomFloat = 0.8f;
						}
						else if (randomFloat > 0.05)
						{
							randomFloat -= 0.05f;
						}

						minimum = randomFloat / 4;

						result = Q_flrand(minimum, randomFloat);

						if (randomYesNo == 0)
						{
							bounce_dir[i] += result;
						}
						else
						{
							bounce_dir[i] -= result;
						}
					}
				}
				else
				{//mildly more wild
					for (i = 0; i < 3; i++)
					{
						bounce_dir[i] += Q_flrand(-0.1f, 0.1f);
					}
				}
			}
			VectorNormalize(bounce_dir);
			reflected = qtrue;
		}
	}
	if (!reflected)
	{
		if (owner->s.clientNum >= MAX_CLIENTS && !G_ControlledByPlayer(owner)) //not blocked but it hit the saber totally wild return
		{
			if (missile->owner && missile->s.weapon != WP_SABER)
			{//bounce back at them if you can
				VectorSubtract(missile->owner->currentOrigin, missile->currentOrigin, bounce_dir);
				VectorNormalize(bounce_dir);
			}
			else
			{
				vec3_t missile_dir;

				VectorSubtract(ent->currentOrigin, missile->currentOrigin, missile_dir);
				VectorCopy(missile->s.pos.trDelta, bounce_dir);
				VectorScale(bounce_dir, DotProduct(forward, missile_dir), bounce_dir);
				VectorNormalize(bounce_dir);
			}
		}
		else //deflect off at an angle.
		{
			vec3_t deflect_dir, missile_dir;
			float forceFactor;
			VectorSubtract(g_crosshairWorldCoord, missile->currentOrigin, deflect_dir);
			VectorCopy(missile->s.pos.trDelta, missile_dir);
			VectorNormalize(missile_dir);
			VectorNormalize(deflect_dir);

			//bigger forceFactors make the reflected shots go closer to the crosshair
			switch (owner->client->ps.forcePowerLevel[powerToUse])
			{
			case FORCE_LEVEL_1:
				forceFactor = 2.0f;
				break;
			case FORCE_LEVEL_2:
				forceFactor = 3.0f;
				break;
			default:
				forceFactor = 10.0f;
				break;
			}

			VectorMA(missile_dir, forceFactor, deflect_dir, bounce_dir);

			VectorNormalize(bounce_dir);
		}
		if (owner->s.weapon == WP_SABER && owner->client && powerToUse == FP_SABER_DEFENSE)
		{//saber
			if (owner->client->ps.saberInFlight)
			{//reflecting off a thrown saber is totally wild
				for (i = 0; i < 3; i++)
				{
					bounce_dir[i] += Q_flrand(-0.3f, 0.3f);
				}
			}
			else if (owner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] <= FORCE_LEVEL_1)
			{// at level 1 or below
				for (i = 0; i < 3; i++)
				{
					bounce_dir[i] += Q_flrand(-0.3f, 0.3f);
				}
			}
			else
			{// at level 2
				for (i = 0; i < 3; i++)
				{
					bounce_dir[i] += Q_flrand(-0.2f, 0.2f);
				}
			}
			if (!PM_SaberInParry(owner->client->ps.saberMove)
				&& !PM_SaberInReflect(owner->client->ps.saberMove)
				&& !PM_SaberInIdle(owner->client->ps.saberMove))
			{//a bit more wild
				if (PM_SaberInAttack(owner->client->ps.saberMove)
					|| PM_SaberInTransitionAny(owner->client->ps.saberMove)
					|| PM_SaberInSpecialAttack(owner->client->ps.torsoAnim))
				{//really wild
					for (i = 0; i < 3; i++)
					{
						bounce_dir[i] += Q_flrand(-0.3f, 0.3f);
					}
				}
				else
				{//mildly more wild
					for (i = 0; i < 3; i++)
					{
						bounce_dir[i] += Q_flrand(-0.1f, 0.1f);
					}
				}
			}
		}
		else
		{//some other kind of reflection
			for (i = 0; i < 3; i++)
			{
				bounce_dir[i] += Q_flrand(-0.2f, 0.2f);
			}
		}
	}
	VectorNormalize(bounce_dir);
	VectorScale(bounce_dir, speed, missile->s.pos.trDelta);
#ifdef _DEBUG
	assert(!_isnan(missile->s.pos.trDelta[0]) && !_isnan(missile->s.pos.trDelta[1]) && !_isnan(missile->s.pos.trDelta[2]));
#endif// _DEBUG
	missile->s.pos.trTime = level.time - 10;		// move a bit on the very first frame
	VectorCopy(missile->currentOrigin, missile->s.pos.trBase);
	if (missile->s.weapon != WP_SABER)
	{//you are mine, now!
		if (!missile->lastEnemy)
		{//remember who originally shot this missile
			missile->lastEnemy = missile->owner;
		}
		missile->owner = owner;
	}
	if (missile->s.weapon == WP_ROCKET_LAUNCHER)
	{//stop homing
		missile->e_ThinkFunc = thinkF_NULL;
	}
}

void G_DeflectMissile(gentity_t* ent, gentity_t* missile, vec3_t forward) //close up blocking
{
	vec3_t	bounce_dir;
	int		i;
	float	speed;
	qboolean reflected = qfalse;
	gentity_t* owner = ent;
	gentity_t* enemy;


	if (ent->owner)
	{
		owner = ent->owner;
	}

	//save the original speed
	speed = VectorNormalize(missile->s.pos.trDelta);

	if (ent && owner && owner->client && !owner->client->ps.saberInFlight)
	{
		if (owner->enemy && Q_irand(0, 3))
		{//toward current enemy 75% of the time
			enemy = owner->enemy;
		}
		else
		{//find another enemy
			enemy = Jedi_FindEnemyInCone(owner, owner->enemy, 0.3f);
		}

		if (enemy)
		{
			vec3_t	bullseye;
			CalcEntitySpot(enemy, SPOT_HEAD, bullseye);
			bullseye[0] += Q_irand(-4, 4);
			bullseye[1] += Q_irand(-4, 4);
			bullseye[2] += Q_irand(-16, 4);
			VectorSubtract(bullseye, missile->currentOrigin, bounce_dir);
			VectorNormalize(bounce_dir);

			if (!PM_SaberInReflect(owner->client->ps.saberMove)
				&& !PM_SaberInIdle(owner->client->ps.saberMove))
			{//a bit more wild
				if (PM_SaberInAttack(owner->client->ps.saberMove)
					|| PM_SaberInTransitionAny(owner->client->ps.saberMove)
					|| PM_SaberInSpecialAttack(owner->client->ps.torsoAnim))
				{//moderately more wild
					for (i = 0; i < 3; i++)
					{
						bounce_dir[i] += Q_flrand(-0.3f, 0.3f);
					}
				}
				else
				{//mildly more wild
					for (i = 0; i < 3; i++)
					{
						bounce_dir[i] += Q_flrand(-0.2f, 0.2f);
					}
				}
			}
			VectorNormalize(bounce_dir);
			reflected = qtrue;
		}
	}
	if (!reflected)//not blocked but it hit the saber totally wild return
	{
		if (missile->owner && missile->s.weapon != WP_SABER)
		{//bounce back at them if you can
			VectorSubtract(missile->owner->currentOrigin, missile->currentOrigin, bounce_dir);
			VectorNormalize(bounce_dir);
		}
		else
		{
			vec3_t missile_dir;

			VectorSubtract(ent->currentOrigin, missile->currentOrigin, missile_dir);
			VectorCopy(missile->s.pos.trDelta, bounce_dir);
			VectorScale(bounce_dir, DotProduct(forward, missile_dir), bounce_dir);
			VectorNormalize(bounce_dir);
		}

		if (owner->s.weapon == WP_SABER && owner->client)
		{//saber
			if (owner->client->ps.saberInFlight)
			{
				for (i = 0; i < 3; i++)
				{
					bounce_dir[i] += Q_flrand(-0.3f, 0.3f);
				}
			}
			else
			{
				for (i = 0; i < 3; i++)
				{
					bounce_dir[i] += Q_flrand(-0.3f, 0.3f);//still going all over the place because we didnt return it with a block/reflect
				}
			}
			if (!PM_SaberInReflect(owner->client->ps.saberMove)
				&& !PM_SaberInIdle(owner->client->ps.saberMove))
			{//a bit more wild
				if (PM_SaberInAttack(owner->client->ps.saberMove)
					|| PM_SaberInTransitionAny(owner->client->ps.saberMove)
					|| PM_SaberInSpecialAttack(owner->client->ps.torsoAnim))
				{//really wild
					for (i = 0; i < 3; i++)
					{
						bounce_dir[i] += Q_flrand(-0.3f, 0.3f);
					}
				}
				else
				{//mildly more wild
					for (i = 0; i < 3; i++)
					{
						bounce_dir[i] += Q_flrand(-0.2f, 0.2f);
					}
				}
			}
		}
		else
		{//some other kind of reflection
			for (i = 0; i < 3; i++)
			{
				bounce_dir[i] += Q_flrand(-0.2f, 0.2f);//its never going to hit anything if its just a reflect
			}
		}
	}
	VectorNormalize(bounce_dir);
	VectorScale(bounce_dir, speed, missile->s.pos.trDelta);
#ifdef _DEBUG
	assert(!_isnan(missile->s.pos.trDelta[0]) && !_isnan(missile->s.pos.trDelta[1]) && !_isnan(missile->s.pos.trDelta[2]));
#endif// _DEBUG

	missile->s.pos.trTime = level.time - 10;		// move a bit on the very first frame
	VectorCopy(missile->currentOrigin, missile->s.pos.trBase);

	if (missile->s.weapon != WP_SABER)
	{//you are mine, now!
		if (!missile->lastEnemy)
		{//remember who originally shot this missile
			missile->lastEnemy = missile->owner;
		}
		missile->owner = owner;
	}
	if (missile->s.weapon == WP_ROCKET_LAUNCHER)
	{//stop homing
		missile->e_ThinkFunc = thinkF_NULL;
	}
}

void G_ManualBlockMissile(gentity_t* ent, gentity_t* missile, vec3_t forward, forcePowers_t powerToUse)
{
	vec3_t	bounce_dir;
	int		i;
	float	speed;
	qboolean perfectReflection = qfalse;
	qboolean reflected = qfalse;
	gentity_t* owner = ent;

	if (ent->owner)
	{
		owner = ent->owner;
	}

	//save the original speed
	speed = VectorNormalize(missile->s.pos.trDelta);

	if (Manual_Saberblocking(owner) || owner->client->ps.ManualBlockingFlags & (1 << MBF_BLOCKING))
	{
		perfectReflection = qtrue;
	}

	if (powerToUse == FP_SABER_DEFENSE)
	{
		if (owner->client->ps.saberInFlight)
		{//but need saber in-hand for perfect reflection
			perfectReflection = qfalse;
		}

		if (PM_RunningAnim(owner->client->ps.legsAnim) || !PM_SaberInParry(owner->client->ps.saberMove))
		{//but need to be blocking for perfect reflection
			perfectReflection = qfalse;
		}
	}

	if (perfectReflection)
	{
		if (owner->client->ps.ManualBlockingFlags & (1 << MBF_MBLOCKING)) // missile blocking
		{
			gentity_t* enemy;
			if (owner->enemy && Q_irand(0, 3))
			{//toward current enemy 75% of the time
				enemy = owner->enemy;
			}
			else
			{//find another enemy
				enemy = Jedi_FindEnemyInCone(owner, owner->enemy, 0.3f);
			}
			if (enemy)
			{
				vec3_t	bullseye;

				speed *= 1.5;

				CalcEntitySpot(enemy, SPOT_CHEST, bullseye);
				bullseye[0] += Q_irand(-4, 4);
				bullseye[1] += Q_irand(-4, 4);
				bullseye[2] += Q_irand(-16, 4);
				VectorSubtract(bullseye, missile->currentOrigin, bounce_dir);
				VectorNormalize(bounce_dir);
				
				if (!PM_SaberInParry(owner->client->ps.saberMove)
					&& !PM_SaberInReflect(owner->client->ps.saberMove)
					&& !PM_SaberInIdle(owner->client->ps.saberMove))
				{//a bit more wild
					if (PM_SaberInAttack(owner->client->ps.saberMove)
						|| PM_SaberInTransitionAny(owner->client->ps.saberMove)
						|| PM_SaberInSpecialAttack(owner->client->ps.torsoAnim))
					{//moderately more wild
						for (i = 0; i < 3; i++)
						{
							bounce_dir[i] += Q_flrand(-0.2f, 0.2f);
						}
					}
					else
					{//mildly more wild
						for (i = 0; i < 3; i++)
						{
							bounce_dir[i] += Q_flrand(-0.1f, 0.1f);
						}
					}
				}
				VectorNormalize(bounce_dir);
				reflected = qtrue;
			}
		}
		else //or by where the crosshair is.
		{
			VectorSubtract(g_crosshairWorldCoord, missile->currentOrigin, bounce_dir);
			VectorNormalize(bounce_dir);

			if (!PM_SaberInParry(owner->client->ps.saberMove)
				&& !PM_SaberInReflect(owner->client->ps.saberMove)
				&& !PM_SaberInIdle(owner->client->ps.saberMove))
			{//a bit more wild
				if (PM_SaberInAttack(owner->client->ps.saberMove)
					|| PM_SaberInTransitionAny(owner->client->ps.saberMove)
					|| PM_SaberInSpecialAttack(owner->client->ps.torsoAnim))
				{//moderately more wild
					for (i = 0; i < 3; i++)
					{
						float randomFloat = (level.time - ent->client->saberProjBlockTime) / 2000.0f;
						float minimum;
						float result;
						int randomYesNo = Q_irand(0, 1);

						if (randomFloat <= 0.05f)
						{
							randomFloat = 0;
						}
						else if (randomFloat > 0.8f)
						{
							randomFloat = 0.8f;
						}
						else if (randomFloat > 0.05)
						{
							randomFloat -= 0.05f;
						}

						minimum = randomFloat / 4;

						result = Q_flrand(minimum, randomFloat);

						if (randomYesNo == 0)
						{
							bounce_dir[i] += result;
						}
						else
						{
							bounce_dir[i] -= result;
						}
					}
				}
				else
				{//mildly more wild
					for (i = 0; i < 3; i++)
					{
						bounce_dir[i] += Q_flrand(-0.1f, 0.1f);
					}
				}
			}
			VectorNormalize(bounce_dir);
			reflected = qtrue;
		}
	}
	if (!reflected)
	{
		if (owner->s.clientNum >= MAX_CLIENTS && !G_ControlledByPlayer(owner)) //not blocked but it hit the saber totally wild return
		{
			if (missile->owner && missile->s.weapon != WP_SABER)
			{//bounce back at them if you can
				VectorSubtract(missile->owner->currentOrigin, missile->currentOrigin, bounce_dir);
				VectorNormalize(bounce_dir);
			}
			else
			{
				vec3_t missile_dir;

				VectorSubtract(ent->currentOrigin, missile->currentOrigin, missile_dir);
				VectorCopy(missile->s.pos.trDelta, bounce_dir);
				VectorScale(bounce_dir, DotProduct(forward, missile_dir), bounce_dir);
				VectorNormalize(bounce_dir);
			}
		}
		else //deflect off at an angle.
		{
			vec3_t deflect_dir, missile_dir;
			float forceFactor;
			VectorSubtract(g_crosshairWorldCoord, missile->currentOrigin, deflect_dir);
			VectorCopy(missile->s.pos.trDelta, missile_dir);
			VectorNormalize(missile_dir);
			VectorNormalize(deflect_dir);

			//bigger forceFactors make the reflected shots go closer to the crosshair
			switch (owner->client->ps.forcePowerLevel[powerToUse])
			{
			case FORCE_LEVEL_1:
				forceFactor = 4.0f;
				break;
			case FORCE_LEVEL_2:
				forceFactor = 6.0f;
				break;
			default:
				forceFactor = 10.0f;
				break;
			}

			VectorMA(missile_dir, forceFactor, deflect_dir, bounce_dir);

			VectorNormalize(bounce_dir);
		}
		if (owner->s.weapon == WP_SABER && owner->client && powerToUse == FP_SABER_DEFENSE)
		{//saber
			if (owner->client->ps.saberInFlight)
			{//reflecting off a thrown saber is totally wild
				for (i = 0; i < 3; i++)
				{
					bounce_dir[i] += Q_flrand(-0.4f, 0.4f);
				}
			}
			else if (owner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] <= FORCE_LEVEL_1)
			{// at level 1 or below
				for (i = 0; i < 3; i++)
				{
					bounce_dir[i] += Q_flrand(-0.3f, 0.3f);
				}
			}
			else
			{// at level 2
				for (i = 0; i < 3; i++)
				{
					bounce_dir[i] += Q_flrand(-0.2f, 0.2f);
				}
			}
			if (!PM_SaberInParry(owner->client->ps.saberMove)
				&& !PM_SaberInReflect(owner->client->ps.saberMove)
				&& !PM_SaberInIdle(owner->client->ps.saberMove))
			{//a bit more wild
				if (PM_SaberInAttack(owner->client->ps.saberMove)
					|| PM_SaberInTransitionAny(owner->client->ps.saberMove)
					|| PM_SaberInSpecialAttack(owner->client->ps.torsoAnim))
				{//really wild
					for (i = 0; i < 3; i++)
					{
						bounce_dir[i] += Q_flrand(-0.3f, 0.3f);
					}
				}
				else
				{//mildly more wild
					for (i = 0; i < 3; i++)
					{
						bounce_dir[i] += Q_flrand(-0.1f, 0.1f);
					}
				}
			}
		}
		else
		{//some other kind of reflection
			for (i = 0; i < 3; i++)
			{
				bounce_dir[i] += Q_flrand(-0.2f, 0.2f);
			}
		}
	}
	VectorNormalize(bounce_dir);
	VectorScale(bounce_dir, speed, missile->s.pos.trDelta);
#ifdef _DEBUG
	assert(!_isnan(missile->s.pos.trDelta[0]) && !_isnan(missile->s.pos.trDelta[1]) && !_isnan(missile->s.pos.trDelta[2]));
#endif// _DEBUG
	missile->s.pos.trTime = level.time - 10;		// move a bit on the very first frame
	VectorCopy(missile->currentOrigin, missile->s.pos.trBase);

	if (missile->s.weapon != WP_SABER)
	{//you are mine, now!
		if (!missile->lastEnemy)
		{//remember who originally shot this missile
			missile->lastEnemy = missile->owner;
		}
		missile->owner = owner;
	}
	if (missile->s.weapon == WP_ROCKET_LAUNCHER)
	{//stop homing
		missile->e_ThinkFunc = thinkF_NULL;
	}
}

/*
================
G_BounceRollMissile

================
*/
void G_BounceRollMissile(gentity_t* ent, trace_t* trace)
{
	vec3_t	velocity, normal;
	float	dot, speedXY, velocityZ, normalZ;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + (level.time - level.previousTime) * trace->fraction;
	EvaluateTrajectoryDelta(&ent->s.pos, hitTime, velocity);
	velocityZ = velocity[2];
	velocity[2] = 0;
	speedXY = VectorLength(velocity);//friction
	VectorCopy(trace->plane.normal, normal);
	normalZ = normal[2];
	normal[2] = 0;
	dot = DotProduct(velocity, normal);
	VectorMA(velocity, -2 * dot, normal, ent->s.pos.trDelta);
	VectorSet(velocity, 0, 0, velocityZ);
	VectorSet(normal, 0, 0, normalZ);
	dot = DotProduct(velocity, normal) * -1;
	if (dot > 10)
	{
		ent->s.pos.trDelta[2] = dot * 0.3f;//not very bouncy
	}
	else
	{
		ent->s.pos.trDelta[2] = 0;
	}

	// check for stop
	if (speedXY <= 0)
	{
		G_SetOrigin(ent, trace->endpos);
		VectorCopy(ent->currentAngles, ent->s.apos.trBase);
		VectorClear(ent->s.apos.trDelta);
		ent->s.apos.trType = TR_STATIONARY;
		return;
	}

	VectorCopy(ent->currentAngles, ent->s.apos.trBase);
	VectorCopy(ent->s.pos.trDelta, ent->s.apos.trDelta);

	VectorCopy(trace->endpos, ent->currentOrigin);
	ent->s.pos.trTime = hitTime - 10;
	VectorCopy(ent->currentOrigin, ent->s.pos.trBase);
}

/*
================
G_BounceMissile

================
*/
void G_BounceMissile(gentity_t* ent, trace_t* trace)
{
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + (level.time - level.previousTime) * trace->fraction;
	EvaluateTrajectoryDelta(&ent->s.pos, hitTime, velocity);
	dot = DotProduct(velocity, trace->plane.normal);
	VectorMA(velocity, -2 * dot, trace->plane.normal, ent->s.pos.trDelta);

	if (ent->s.eFlags & EF_BOUNCE_SHRAPNEL)
	{
		VectorScale(ent->s.pos.trDelta, 0.25f, ent->s.pos.trDelta);
		ent->s.pos.trType = TR_GRAVITY;

		// check for stop
		if (trace->plane.normal[2] > 0.7 && ent->s.pos.trDelta[2] < 40) //this can happen even on very slightly sloped walls, so changed it from > 0 to > 0.7
		{
			G_SetOrigin(ent, trace->endpos);
			ent->nextthink = level.time + 100;
			return;
		}
	}
	else if (ent->s.eFlags & EF_BOUNCE_HALF)
	{
		VectorScale(ent->s.pos.trDelta, 0.5, ent->s.pos.trDelta);

		// check for stop
		if (trace->plane.normal[2] > 0.7 && ent->s.pos.trDelta[2] < 40) //this can happen even on very slightly sloped walls, so changed it from > 0 to > 0.7
		{
			if (ent->s.weapon == WP_THERMAL)
			{//roll when you "stop"
				ent->s.pos.trType = TR_INTERPOLATE;
			}
			else
			{
				G_SetOrigin(ent, trace->endpos);
				ent->nextthink = level.time + 500;
				return;
			}
		}

		if (ent->s.weapon == WP_THERMAL)
		{
			ent->has_bounced = qtrue;
		}
	}

#if 0
	// OLD--this looks so wrong.  It looked wrong in EF.  It just must be wrong.
	VectorAdd(ent->currentOrigin, trace->plane.normal, ent->currentOrigin);

	ent->s.pos.trTime = level.time - 10;
#else
	// NEW--It would seem that we want to set our trBase to the trace endpos
	//	and set the trTime to the actual time of impact....
	VectorAdd(trace->endpos, trace->plane.normal, ent->currentOrigin);
	if (hitTime >= level.time)
	{//trace fraction must have been 1
		ent->s.pos.trTime = level.time - 10;
	}
	else
	{
		ent->s.pos.trTime = hitTime - 10; // this is kinda dumb hacking, but it pushes the missile away from the impact plane a bit
	}
#endif

	VectorCopy(ent->currentOrigin, ent->s.pos.trBase);
	VectorCopy(trace->plane.normal, ent->pos1);

	if (ent->s.weapon != WP_SABER
		&& ent->s.weapon != WP_THERMAL
		&& ent->e_clThinkFunc != clThinkF_CG_Limb
		&& ent->e_ThinkFunc != thinkF_LimbThink)
	{//not a saber, bouncing thermal or limb
		//now you can damage the guy you came from
		ent->owner = NULL;
	}
}

/*
================
G_MissileImpact

================
*/

void NoghriGasCloudThink(gentity_t* self)
{
	self->nextthink = level.time + FRAMETIME;

	AddSightEvent(self->owner, self->currentOrigin, 200, AEL_DANGER, 50);

	if (self->fx_time < level.time)
	{
		vec3_t	up = { 0,0,1 };
		G_PlayEffect("noghri_stick/gas_cloud", self->currentOrigin, up);
		self->fx_time = level.time + 250;
	}

	if (level.time - self->s.time <= 2500)
	{
		if (!Q_irand(0, 3 - g_spskill->integer))
		{
			G_RadiusDamage(self->currentOrigin, self->owner, Q_irand(1, 4), self->splashRadius,
				self->owner, self->splashMethodOfDeath);
		}
	}

	if (level.time - self->s.time > 3000)
	{
		G_FreeEntity(self);
	}
}

void G_SpawnNoghriGasCloud(gentity_t* ent)
{//FIXME: force-pushable/dispersable?
	ent->freeAfterEvent = qfalse;
	ent->e_TouchFunc = touchF_NULL;

	G_SetOrigin(ent, ent->currentOrigin);
	ent->e_ThinkFunc = thinkF_NoghriGasCloudThink;
	ent->nextthink = level.time + FRAMETIME;

	vec3_t	up = { 0,0,1 };
	G_PlayEffect("noghri_stick/gas_cloud", ent->currentOrigin, up);

	ent->fx_time = level.time + 250;
	ent->s.time = level.time;
}

extern void laserTrapStick(gentity_t* ent, vec3_t endpos, vec3_t normal);
extern qboolean W_AccuracyLoggableWeapon(int weapon, qboolean alt_fire, int mod);
void G_MissileImpacted(gentity_t* ent, gentity_t* other, vec3_t impactPos, vec3_t normal, int hitLoc = HL_NONE)
{
	// impact damage
	if (other->takedamage)
	{
		if (ent->damage)
		{
			vec3_t	velocity;

			EvaluateTrajectoryDelta(&ent->s.pos, level.time, velocity);
			if (VectorLength(velocity) == 0)
			{
				velocity[2] = 1;	// stepped on a grenade
			}

			int damage = ent->damage;

			if (other->client)
			{
				class_t	npc_class = other->client->NPC_class;

				// If we are a robot and we aren't currently doing the full body electricity...
				if (npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE || npc_class == CLASS_MOUSE ||
					npc_class == CLASS_SBD || npc_class == CLASS_BATTLEDROID || npc_class == CLASS_DROIDEKA ||
					npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 || npc_class == CLASS_REMOTE ||
					npc_class == CLASS_MARK1 || npc_class == CLASS_MARK2 || npc_class == CLASS_PROTOCOL ||
					npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST || npc_class == CLASS_SENTRY)
				{// special droid only behaviors
					if (other->client->ps.powerups[PW_SHOCKED] < level.time + 100)
					{// ... do the effect for a split second for some more feedback
						other->s.powerups |= (1 << PW_SHOCKED);
						other->client->ps.powerups[PW_SHOCKED] = level.time + Q_irand(1500, 2000);
					}
				}
			}

			G_Damage(other, ent, ent->owner, velocity, impactPos, damage, ent->dflags, ent->methodOfDeath, hitLoc);

			if (ent->s.weapon == WP_DEMP2)
			{//a hit with demp2 decloaks saboteurs
				if (other && other->client && other->client->NPC_class == CLASS_SABOTEUR)
				{//FIXME: make this disabled cloak hold for some amount of time?
					Saboteur_Decloak(other, Q_irand(3000, 10000));
					if (ent->methodOfDeath == MOD_DEMP2_ALT)
					{//direct hit with alt disabled cloak forever
						if (other->NPC)
						{//permanently disable the saboteur's cloak
							other->NPC->aiFlags &= ~NPCAI_SHIELDS;
						}
					}
				}
				if (other && other->client && other->client->NPC_class == CLASS_VEHICLE)
				{//hit a vehicle
					if (other->m_pVehicle //valid vehicle ent
						&& other->m_pVehicle->m_pVehicleInfo//valid stats
						&& (other->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER//always affect speeders
							|| (other->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER && ent->classname &&
								Q_stricmp("vehicle_proj", ent->classname) == 0))//only vehicle ion weapons affect a fighter in this manner
						&& !FighterIsLanded(other->m_pVehicle, &other->client->ps)//not landed
						&& !(other->spawnflags & 2))//and not suspended
					{//vehicles hit by "ion cannons" lose control
						if (other->client->ps.electrifyTime > level.time)
						{//add onto it
							other->client->ps.electrifyTime += level.time + Q_irand(1500, 2000);
							if (other->client->ps.electrifyTime > level.time + 4000)
							{//cap it
								other->client->ps.electrifyTime = level.time + 4000;
							}
						}
						else
						{//start it
							other->client->ps.electrifyTime = level.time + Q_irand(1500, 2000);
						}
					}
				}
				else if (other && other->client && other->client->ps.powerups[PW_CLOAKED])
				{
					player_Decloak(other);
					Jedi_Decloak(other);
					if (ent->methodOfDeath == MOD_DEMP2_ALT)
					{//direct hit with alt disables cloak forever //permanently disable the saboteur's cloak
						other->client->cloakToggleTime = Q3_INFINITE;
					}
					else
					{//temp disable
						other->client->cloakToggleTime = level.time + Q_irand(3000, 10000);
					}
				}
			}
		}
	}

	if ((other->takedamage && other->client))
	{
		G_AddEvent(ent, EV_MISSILE_HIT, DirToByte(normal));
		ent->s.otherEntityNum = other->s.number;
	}
	else
	{
		G_AddEvent(ent, EV_MISSILE_MISS, DirToByte(normal));
		ent->s.otherEntityNum = other->s.number;
	}

	VectorCopy(normal, ent->pos1);

	if (ent->owner)
	{
		//Add the event
		AddSoundEvent(ent->owner, ent->currentOrigin, 256, AEL_SUSPICIOUS, qfalse, qtrue);
		AddSightEvent(ent->owner, ent->currentOrigin, 512, AEL_DISCOVERED, 75);
	}

	ent->freeAfterEvent = qtrue;

	// change over to a normal entity right at the point of impact
	ent->s.eType = ET_GENERAL;

	VectorCopy(impactPos, ent->s.pos.trBase);

	G_SetOrigin(ent, impactPos);

	// splash damage (doesn't apply to person directly hit)
	if (ent->splashDamage)
	{
		G_RadiusDamage(impactPos, ent->owner, ent->splashDamage, ent->splashRadius,
			other, ent->splashMethodOfDeath);
	}

	if (ent->s.weapon == WP_NOGHRI_STICK)
	{
		G_SpawnNoghriGasCloud(ent);
	}

	gi.linkentity(ent);
}

//------------------------------------------------
static void G_MissileAddAlerts(gentity_t * ent)
{
	//Add the event
	if (ent->s.weapon == WP_THERMAL && ((ent->delay - level.time) < 2000 || ent->s.pos.trType == TR_INTERPOLATE))
	{//a thermal about to explode or rolling
		if ((ent->delay - level.time) < 500)
		{//half a second before it explodes!
			AddSoundEvent(ent->owner, ent->currentOrigin, ent->splashRadius * 2, AEL_DANGER_GREAT, qfalse, qtrue);
			AddSightEvent(ent->owner, ent->currentOrigin, ent->splashRadius * 2, AEL_DANGER_GREAT, 20);
		}
		else
		{//2 seconds until it explodes or it's rolling
			AddSoundEvent(ent->owner, ent->currentOrigin, ent->splashRadius * 2, AEL_DANGER, qfalse, qtrue);
			AddSightEvent(ent->owner, ent->currentOrigin, ent->splashRadius * 2, AEL_DANGER, 20);
		}
	}
	else
	{
		AddSoundEvent(ent->owner, ent->currentOrigin, 128, AEL_DISCOVERED);
		AddSightEvent(ent->owner, ent->currentOrigin, 256, AEL_DISCOVERED, 40);
	}
}

//------------------------------------------------------
void G_MissileImpact(gentity_t * ent, trace_t * trace, int hitLoc = HL_NONE)
{
	gentity_t* other;
	gentity_t* otherOwner;
	vec3_t			diff;

	other = &g_entities[trace->entityNum];
	otherOwner = &g_entities[other->ownerNum];

	if (other == ent)
	{
		assert(0 && "missile hit itself!!!");
		return;
	}
	if (trace->plane.normal[0] == 0.0f &&
		trace->plane.normal[1] == 0.0f &&
		trace->plane.normal[2] == 0.0f
		)
	{//model moved into missile in flight probably...
		trace->plane.normal[0] = -ent->s.pos.trDelta[0];
		trace->plane.normal[1] = -ent->s.pos.trDelta[1];
		trace->plane.normal[2] = -ent->s.pos.trDelta[2];
		VectorNormalize(trace->plane.normal);
	}

	if (ent->owner && (other->takedamage || other->client))
	{
		if (!ent->lastEnemy || ent->lastEnemy == ent->owner)
		{//a missile that was not reflected or, if so, still is owned by original owner
			if (LogAccuracyHit(other, ent->owner))
			{
				ent->owner->client->ps.persistant[PERS_ACCURACY_HITS]++;
			}
			if (ent->owner->client && !ent->owner->s.number)
			{
				if (W_AccuracyLoggableWeapon(ent->s.weapon, qfalse, ent->methodOfDeath))
				{
					ent->owner->client->sess.missionStats.hits++;
				}
			}
		}
	}
	// check for bounce
	//OR: if the surfaceParm is has a reflect property (magnetic shielding) and the missile isn't an exploding missile
	qboolean bounce = (qboolean)((!other->takedamage && (ent->s.eFlags & (EF_BOUNCE | EF_BOUNCE_HALF))) || (((trace->surfaceFlags & SURF_FORCEFIELD) || (other->flags & FL_SHIELDED)) && !ent->splashDamage && !ent->splashRadius && ent->s.weapon != WP_NOGHRI_STICK));

	if (ent->dflags & DAMAGE_HEAVY_WEAP_CLASS)
	{
		// heavy class missiles generally never bounce.
		bounce = qfalse;
	}

	if (other->flags & (FL_DMG_BY_HEAVY_WEAP_ONLY | FL_SHIELDED))
	{
		// Dumb assumption, but I guess we must be a shielded ion_cannon??  We should probably verify
		// if it's an ion_cannon that's Heavy Weapon only, we don't want to make it shielded do we...?
		if ((!strcmp("misc_ion_cannon", other->classname)) && (other->flags & FL_SHIELDED))
		{
			// Anything will bounce off of us.
			bounce = qtrue;

			// Not exactly the debounce time, but rather the impact time for the shield effect...play effect for 1 second
			other->painDebounceTime = level.time + 1000;
		}
	}

	if (ent->s.weapon == WP_DEMP2)
	{
		// demp2 shots can never bounce
		bounce = qfalse;

		// in fact, alt-charge shots will not call the regular impact functions
		if (ent->alt_fire)
		{
			// detonate at the trace end
			VectorCopy(trace->endpos, ent->currentOrigin);
			VectorCopy(trace->plane.normal, ent->pos1);
			DEMP2_AltDetonate(ent);
			return;
		}
	}

	if (bounce)
	{
		// Check to see if there is a bounce count
		if (ent->bounceCount)
		{
			// decrement number of bounces and then see if it should be done bouncing
			if (!(--ent->bounceCount)) {
				// He (or she) will bounce no more (after this current bounce, that is).
				ent->s.eFlags &= ~(EF_BOUNCE | EF_BOUNCE_HALF);
			}
		}

		if (other->NPC)
		{
			G_Damage(other, ent, ent->owner, ent->currentOrigin, ent->s.pos.trDelta, 0, DAMAGE_NO_DAMAGE, MOD_UNKNOWN);
		}

		G_BounceMissile(ent, trace);

		if (ent->owner)
		{
			G_MissileAddAlerts(ent);
		}
		G_MissileBounceEffect(ent, trace->endpos, trace->plane.normal, (qboolean)(trace->entityNum == ENTITYNUM_WORLD));

		return;
	}

	if ((other->s.number < MAX_CLIENTS || G_ControlledByPlayer(other)))
	{// I would glom onto the EF_BOUNCE code section above, but don't feel like risking breaking something else
		if ((!other->takedamage && (ent->s.eFlags & (EF_BOUNCE_SHRAPNEL)))
			|| ((trace->surfaceFlags & SURF_FORCEFIELD) && !ent->splashDamage && !ent->splashRadius))
		{
			if (!(other->contents & CONTENTS_LIGHTSABER))
			{
				G_BounceMissile(ent, trace);

				if (--ent->bounceCount < 0)
				{
					ent->s.eFlags &= ~EF_BOUNCE_SHRAPNEL;
				}
				G_MissileBounceEffect(ent, trace->endpos, trace->plane.normal, (qboolean)(trace->entityNum == ENTITYNUM_WORLD));
				return;
			}
		}
	}
	else
	{// I would glom onto the EF_BOUNCE code section above, but don't feel like risking breaking something else
		if ((!other->takedamage && (ent->s.eFlags & (EF_BOUNCE_SHRAPNEL)))
			|| ((trace->surfaceFlags & SURF_FORCEFIELD) && !ent->splashDamage && !ent->splashRadius))
		{
			if (!(other->contents & CONTENTS_LIGHTSABER)
				|| g_spskill->integer <= 0//on easy, it reflects all shots
				|| (g_spskill->integer == 1 && ent->s.weapon != WP_FLECHETTE && ent->s.weapon != WP_DEMP2)
				|| (g_spskill->integer >= 2 && ent->s.weapon != WP_FLECHETTE && ent->s.weapon != WP_DEMP2
					&& ent->s.weapon != WP_BOWCASTER && ent->s.weapon != WP_REPEATER))
			{
				G_BounceMissile(ent, trace);

				if (--ent->bounceCount < 0)
				{
					ent->s.eFlags &= ~EF_BOUNCE_SHRAPNEL;
				}
				G_MissileBounceEffect(ent, trace->endpos, trace->plane.normal, (qboolean)(trace->entityNum == ENTITYNUM_WORLD));
				return;
			}
		}
	}

	if ((!other->takedamage || (other->client && other->health <= 0))
		&& ent->s.weapon == WP_THERMAL
		&& !ent->alt_fire)
	{//rolling thermal det - FIXME: make this an eFlag like bounce & stick!!!
		if (ent->owner)
		{
			G_MissileAddAlerts(ent);
		}
		return;
	}

	// check for sticking
	if (ent->s.eFlags & EF_MISSILE_STICK)
	{
		if (ent->owner)
		{
			//Add the event
			if (ent->s.weapon == WP_TRIP_MINE)
			{
				AddSoundEvent(ent->owner, ent->currentOrigin, ent->splashRadius / 2, AEL_DISCOVERED, qfalse, qtrue);
				AddSightEvent(ent->owner, ent->currentOrigin, ent->splashRadius * 2, AEL_DISCOVERED, 60);
			}
			else
			{
				AddSoundEvent(ent->owner, ent->currentOrigin, 128, AEL_DISCOVERED, qfalse, qtrue);
				AddSightEvent(ent->owner, ent->currentOrigin, 256, AEL_DISCOVERED, 10);
			}
		}

		G_MissileStick(ent, other, trace);
		return;
	}


	if (!strcmp(ent->classname, "hook"))
	{
		gentity_t* nent;
		vec3_t v;
		nent = G_Spawn();

		if (other->client || other->s.eType == ET_MOVER)
		{
			G_AddEvent(nent, EV_MISSILE_HIT, DirToByte(trace->plane.normal));
			nent->s.otherEntityNum2 = other->s.number;
			ent->enemy = other;
			v[0] = other->currentOrigin[0] + (other->mins[0] + other->maxs[0]) * 0.5f;
			v[1] = other->currentOrigin[1] + (other->mins[1] + other->maxs[1]) * 0.5f;
			v[2] = other->currentOrigin[2] + (other->mins[2] + other->maxs[2]) * 0.5f;
			SnapVectorTowards(v, ent->s.pos.trBase);	// save net bandwidth
		}
		else
		{
			VectorCopy(trace->endpos, v);
			G_AddEvent(nent, EV_MISSILE_MISS, DirToByte(trace->plane.normal));
			ent->enemy = NULL;
		}

		SnapVectorTowards(v, ent->s.pos.trBase);

		nent->freeAfterEvent = qtrue;
		nent->s.eType = ET_GENERAL;
		ent->s.eType = ET_GENERAL;

		G_SetOrigin(ent, v);
		G_SetOrigin(nent, v);

		ent->think = Weapon_HookThink;
		ent->nextthink = level.time + FRAMETIME;

		ent->parent->client->ps.pm_flags |= PMF_GRAPPLE_PULL;
		VectorCopy(ent->currentOrigin, ent->parent->client->ps.lastHitLoc);

		gi.linkentity(ent);
		gi.linkentity(nent);

		return;
	}

	// check for hitting a lightsaber
	if ((other->s.number < MAX_CLIENTS || G_ControlledByPlayer(other)))
	{
		if (WP_SaberMustBlock(other, ent, qfalse, trace->endpos, -1, -1)
			&& !WP_DoingForcedAnimationForForcePowers(other))
		{ //play projectile block animation
			other->client->ps.weaponTime = 0;
			if (other->client->ps.blockPoints > BLOCKPOINTS_FAIL)
			{
				WP_HandleBoltBlock(ent, other, trace);
			}
			if (other->owner && !other->owner->s.number && other->owner->client)
			{
				other->owner->client->sess.missionStats.saberBlocksCnt++;
			}
			return;
		}
		else if (other->contents & CONTENTS_LIGHTSABER)
		{
			if ((!ent->splashDamage || !ent->splashRadius) && ent->s.weapon != WP_NOGHRI_STICK)
			{
				if (!other->owner || !other->owner->client || other->owner->client->ps.saberInFlight
					|| (InFront(ent->currentOrigin, other->owner->currentOrigin, other->owner->client->ps.viewangles, SABER_REFLECT_MISSILE_CONE)
						&& !WP_DoingForcedAnimationForForcePowers(other)))
				{
					int blockChance = 0;

					switch (other->owner->client->ps.forcePowerLevel[FP_SABER_DEFENSE])
					{//level 1 reflects 50% of the time, level 2 reflects 75% of the time
					case FORCE_LEVEL_3:
						blockChance = 10;
						break;
					case FORCE_LEVEL_2:
						blockChance = 6;
						break;
					case FORCE_LEVEL_1:
						blockChance = 4;
						break;
					}
					if (Q_irand(0, blockChance))
					{
						VectorSubtract(ent->currentOrigin, other->currentOrigin, diff);
						VectorNormalize(diff);
						otherOwner->client->ps.weaponTime = 0;
						if (otherOwner->client->ps.blockPoints > BLOCKPOINTS_FAIL)
						{
							WP_HandleBoltBlock(ent, otherOwner, trace);
						}
						if (other->owner && !other->owner->s.number && other->owner->client)
						{
							other->owner->client->sess.missionStats.saberBlocksCnt++;
						}
						return;
					}
				}
			}
			else
			{//still do the bounce effect
				G_MissileReflectEffect(ent, trace->endpos, trace->plane.normal);
			}
		}
	}
	else
	{
		if (other->contents & CONTENTS_LIGHTSABER)
		{
			if (other->owner && !other->owner->s.number && other->owner->client)
			{
				other->owner->client->sess.missionStats.saberBlocksCnt++;
			}
			if ((!ent->splashDamage || !ent->splashRadius) && ent->s.weapon != WP_NOGHRI_STICK)
			{
				if (!other->owner || !other->owner->client || other->owner->client->ps.saberInFlight
					|| (InFront(ent->currentOrigin, other->owner->currentOrigin, other->owner->client->ps.viewangles, SABER_REFLECT_MISSILE_CONE)
						&& !WP_DoingForcedAnimationForForcePowers(other)))
				{
					int blockChance = 0;

					switch (other->owner->client->ps.forcePowerLevel[FP_SABER_DEFENSE])
					{//level 1 reflects 50% of the time, level 2 reflects 75% of the time
					case FORCE_LEVEL_3:
						blockChance = 10;
						break;
					case FORCE_LEVEL_2:
						blockChance = 6;
						break;
					case FORCE_LEVEL_1:
						blockChance = 4;
						break;
					}
					if (blockChance && (other->owner->client->ps.forcePowersActive & (1 << FP_SPEED)))
					{//in in force speed, better chance of deflecting the shot
						blockChance += other->owner->client->ps.forcePowerLevel[FP_SPEED] * 2;
					}
					if (Q_irand(0, blockChance))
					{
						VectorSubtract(ent->currentOrigin, other->currentOrigin, diff);
						VectorNormalize(diff);
						G_ReflectMissileNPC(other, ent, diff);
						if (other->owner && other->owner->client)
						{
							other->owner->client->ps.saberEventFlags |= SEF_DEFLECTED;
						}
						//do the effect
						VectorCopy(ent->s.pos.trDelta, diff);
						VectorNormalize(diff);
						G_MissileReflectEffect(ent, trace->endpos, trace->plane.normal);
						return;
					}
				}
			}
			else
			{//still do the bounce effect
				G_MissileReflectEffect(ent, trace->endpos, trace->plane.normal);
			}
		}
	}
	G_MissileImpacted(ent, other, trace->endpos, trace->plane.normal, hitLoc);
}

/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile(gentity_t * ent)
{
	vec3_t		dir;
	vec3_t		origin;

	EvaluateTrajectory(&ent->s.pos, level.time, origin);
	SnapVector(origin);
	G_SetOrigin(ent, origin);

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	if (ent->owner)//&& ent->owner->s.number == 0 )
	{
		//Add the event
		AddSoundEvent(ent->owner, ent->currentOrigin, 256, AEL_DISCOVERED, qfalse, qtrue);
		AddSightEvent(ent->owner, ent->currentOrigin, 512, AEL_DISCOVERED, 100);
	}

	// splash damage
	if (ent->splashDamage)
	{
		G_RadiusDamage(ent->currentOrigin, ent->owner, ent->splashDamage, ent->splashRadius, NULL, ent->splashMethodOfDeath);
	}

	G_FreeEntity(ent);
}


void G_RunStuckMissile(gentity_t * ent)
{
	if (ent->takedamage)
	{
		if (ent->s.groundEntityNum >= 0 && ent->s.groundEntityNum < ENTITYNUM_WORLD)
		{
			gentity_t* other = &g_entities[ent->s.groundEntityNum];

			if ((!VectorCompare(vec3_origin, other->s.pos.trDelta) && other->s.pos.trType != TR_STATIONARY) ||
				(!VectorCompare(vec3_origin, other->s.apos.trDelta) && other->s.apos.trType != TR_STATIONARY))
			{//thing I stuck to is moving or rotating now, kill me
				G_Damage(ent, other, other, NULL, NULL, 99999, 0, MOD_CRUSH);
				return;
			}
		}
	}
	// check think function
	G_RunThink(ent);
}

/*
==================

G_GroundTrace

==================
*/
int G_GroundTrace(gentity_t * ent, pml_t * pPml)
{
	vec3_t		point;
	trace_t		trace;

	point[0] = ent->currentOrigin[0];
	point[1] = ent->currentOrigin[1];
	point[2] = ent->currentOrigin[2] - 0.25;

	gi.trace(&trace, ent->currentOrigin, ent->mins, ent->maxs, point, ent->s.number, ent->clipmask, (EG2_Collision)0, 0);
	pPml->groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if (trace.allsolid)
	{
		pPml->groundPlane = qfalse;
		pPml->walking = qfalse;
		return ENTITYNUM_NONE;
	}

	// if the trace didn't hit anything, we are in free fall
	if (trace.fraction == 1.0)
	{
		pPml->groundPlane = qfalse;
		pPml->walking = qfalse;
		return ENTITYNUM_NONE;
	}

	// check if getting thrown off the ground
	if (ent->s.pos.trDelta[2] > 0 && DotProduct(ent->s.pos.trDelta, trace.plane.normal) > 10)
	{
		pPml->groundPlane = qfalse;
		pPml->walking = qfalse;
		return ENTITYNUM_NONE;
	}

	// slopes that are too steep will not be considered onground
	if (trace.plane.normal[2] < MIN_WALK_NORMAL)
	{
		pPml->groundPlane = qtrue;
		pPml->walking = qfalse;
		return ENTITYNUM_NONE;
	}

	pPml->groundPlane = qtrue;
	pPml->walking = qtrue;

	return trace.entityNum;
}

void G_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float	backoff;
	float	change;
	int		i;

	backoff = DotProduct(in, normal);

	if (backoff < 0)
	{
		backoff *= overbounce;
	}
	else
	{
		backoff /= overbounce;
	}

	for (i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;
	}
}
/*
==================

G_RollMissile

reworking the rolling object code,
still needs to stop bobbling up & down,
need to get roll angles right,
and need to maybe make the transfer of velocity happen on impacts?
Also need bounce sound for bounces off a floor.
Also need to not bounce as much off of enemies
Also gets stuck inside thrower if looking down when thrown

==================
*/
#define	MAX_CLIP_PLANES	5
#define	BUMPCLIP		1.5f
void G_RollMissile(gentity_t * ent)
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity;
	vec3_t		clipVelocity;
	int			i, j, k;
	trace_t	trace;
	vec3_t		end;
	float		time_left;
	float		into;
	vec3_t		endVelocity;
	vec3_t		endClipVelocity;
	pml_t		objPML;
	float		bounceAmt = BUMPCLIP;
	gentity_t* hitEnt = NULL;

	memset(&objPML, 0, sizeof(objPML));

	G_GroundTrace(ent, &objPML);

	objPML.frametime = (level.time - level.previousTime) * 0.001;

	numbumps = 4;

	VectorCopy(ent->s.pos.trDelta, primal_velocity);

	VectorCopy(ent->s.pos.trDelta, endVelocity);
	endVelocity[2] -= g_gravity->value * objPML.frametime;
	ent->s.pos.trDelta[2] = (ent->s.pos.trDelta[2] + endVelocity[2]) * 0.5;
	primal_velocity[2] = endVelocity[2];
	if (objPML.groundPlane)
	{//FIXME: never happens!
		// slide along the ground plane
		G_ClipVelocity(ent->s.pos.trDelta, objPML.groundTrace.plane.normal, ent->s.pos.trDelta, BUMPCLIP);
		VectorScale(ent->s.pos.trDelta, 0.9f, ent->s.pos.trDelta);
	}

	time_left = objPML.frametime;

	// never turn against the ground plane
	if (objPML.groundPlane)
	{
		numplanes = 1;
		VectorCopy(objPML.groundTrace.plane.normal, planes[0]);
	}
	else
	{
		numplanes = 0;
	}

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		// calculate position we are trying to move to
		VectorMA(ent->currentOrigin, time_left, ent->s.pos.trDelta, end);

		// see if we can make it there
		gi.trace(&trace, ent->currentOrigin, ent->mins, ent->maxs, end, ent->s.number, ent->clipmask, G2_RETURNONHIT, 10);

		//had to move this up above the trace.allsolid check now that for some reason ghoul2 impacts tell me I'm allsolid?!
		//this needs to be fixed, really
		if (trace.entityNum < ENTITYNUM_WORLD)
		{//hit another ent
			hitEnt = &g_entities[trace.entityNum];
			if (hitEnt && (hitEnt->takedamage || (hitEnt->contents & CONTENTS_LIGHTSABER)))
			{
				G_MissileImpact(ent, &trace);
				if (ent->s.eType == ET_GENERAL)
				{//exploded
					return;
				}
			}
		}

		if (trace.allsolid)
		{
			ent->s.pos.trDelta[2] = 0;	// don't build up falling damage, but allow sideways acceleration
			return;// qtrue;
		}

		if (trace.fraction > 0)
		{
			// actually covered some distance
			VectorCopy(trace.endpos, ent->currentOrigin);
		}

		if (trace.fraction == 1)
		{
			break;		// moved the entire distance
		}

		time_left -= time_left * trace.fraction;

		if (numplanes >= MAX_CLIP_PLANES)
		{
			// this shouldn't really happen
			VectorClear(ent->s.pos.trDelta);
			return;// qtrue;
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		for (i = 0; i < numplanes; i++)
		{
			if (DotProduct(trace.plane.normal, planes[i]) > 0.99)
			{
				VectorAdd(trace.plane.normal, ent->s.pos.trDelta, ent->s.pos.trDelta);
				break;
			}
		}
		if (i < numplanes)
		{
			continue;
		}
		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//
		if (&g_entities[trace.entityNum] != NULL && g_entities[trace.entityNum].client)
		{//hit a person, bounce off much less
			bounceAmt = OVERCLIP;
		}
		else
		{
			bounceAmt = BUMPCLIP;
		}

		// find a plane that it enters
		for (i = 0; i < numplanes; i++)
		{
			into = DotProduct(ent->s.pos.trDelta, planes[i]);
			if (into >= 0.1)
			{
				continue;		// move doesn't interact with the plane
			}

			// see how hard we are hitting things
			if (-into > pml.impactSpeed)
			{
				pml.impactSpeed = -into;
			}

			// slide along the plane
			G_ClipVelocity(ent->s.pos.trDelta, planes[i], clipVelocity, bounceAmt);

			// slide along the plane
			G_ClipVelocity(endVelocity, planes[i], endClipVelocity, bounceAmt);

			// see if there is a second plane that the new move enters
			for (j = 0; j < numplanes; j++)
			{
				if (j == i)
				{
					continue;
				}
				if (DotProduct(clipVelocity, planes[j]) >= 0.1)
				{
					continue;		// move doesn't interact with the plane
				}

				// try clipping the move to the plane
				G_ClipVelocity(clipVelocity, planes[j], clipVelocity, bounceAmt);
				G_ClipVelocity(endClipVelocity, planes[j], endClipVelocity, bounceAmt);

				// see if it goes back into the first clip plane
				if (DotProduct(clipVelocity, planes[i]) >= 0)
				{
					continue;
				}

				// slide the original velocity along the crease
				CrossProduct(planes[i], planes[j], dir);
				VectorNormalize(dir);
				d = DotProduct(dir, ent->s.pos.trDelta);
				VectorScale(dir, d, clipVelocity);

				CrossProduct(planes[i], planes[j], dir);
				VectorNormalize(dir);
				d = DotProduct(dir, endVelocity);
				VectorScale(dir, d, endClipVelocity);

				// see if there is a third plane the the new move enters
				for (k = 0; k < numplanes; k++)
				{
					if (k == i || k == j)
					{
						continue;
					}
					if (DotProduct(clipVelocity, planes[k]) >= 0.1)
					{
						continue;		// move doesn't interact with the plane
					}

					// stop dead at a triple plane interaction
					VectorClear(ent->s.pos.trDelta);
					return;// qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy(clipVelocity, ent->s.pos.trDelta);
			VectorCopy(endClipVelocity, endVelocity);
			break;
		}
		VectorScale(endVelocity, 0.975f, endVelocity);
	}

	VectorCopy(endVelocity, ent->s.pos.trDelta);

	return;
}
/*
================
G_RunMissile

================
*/
void G_MoverTouchPushTriggers(gentity_t * ent, vec3_t oldOrg);
void G_RunMissile(gentity_t * ent)
{
	vec3_t		oldOrg;
	trace_t		tr;
	int			trHitLoc = HL_NONE;

	if ((ent->s.eFlags & EF_HELD_BY_SAND_CREATURE))
	{//in a sand creature's mouth
		if (ent->activator)
		{
			mdxaBone_t	boltMatrix;
			// Getting the bolt here
			//in hand
			vec3_t scAngles = { 0 };
			scAngles[YAW] = ent->activator->currentAngles[YAW];
			gi.G2API_GetBoltMatrix(ent->activator->ghoul2, ent->activator->playerModel, ent->activator->gutBolt,
				&boltMatrix, scAngles, ent->activator->currentOrigin, (cg.time ? cg.time : level.time),
				NULL, ent->activator->s.modelScale);
			// Storing ent position, bolt position, and bolt axis
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, ent->currentOrigin);
			G_SetOrigin(ent, ent->currentOrigin);
		}
		// check think function
		G_RunThink(ent);
		return;
	}

	VectorCopy(ent->currentOrigin, oldOrg);

	// get current position
	if (ent->s.pos.trType == TR_INTERPOLATE)
	{//rolling missile?
		G_RollMissile(ent);
		if (ent->s.eType != ET_GENERAL)
		{//didn't explode
			VectorCopy(ent->currentOrigin, ent->s.pos.trBase);
			gi.trace(&tr, oldOrg, ent->mins, ent->maxs, ent->currentOrigin, ent->s.number, ent->clipmask, G2_RETURNONHIT, 10);
			if (VectorCompare(ent->s.pos.trDelta, vec3_origin))
			{
				VectorClear(ent->s.apos.trDelta);
			}
			else
			{
				vec3_t	ang, fwdDir, rtDir;
				float	speed;

				ent->s.apos.trType = TR_INTERPOLATE;
				VectorSet(ang, 0, ent->s.apos.trBase[1], 0);
				AngleVectors(ang, fwdDir, rtDir, NULL);
				speed = VectorLength(ent->s.pos.trDelta) * 4;

				//HMM, this works along an axis-aligned dir, but not along diagonals
				//This is because when roll gets to 90, pitch becomes yaw, and vice-versa
				//Maybe need to just set the angles directly?
				ent->s.apos.trDelta[0] = DotProduct(fwdDir, ent->s.pos.trDelta);
				ent->s.apos.trDelta[1] = 0;//never spin!
				ent->s.apos.trDelta[2] = DotProduct(rtDir, ent->s.pos.trDelta);

				VectorNormalize(ent->s.apos.trDelta);
				VectorScale(ent->s.apos.trDelta, speed, ent->s.apos.trDelta);

				ent->s.apos.trTime = level.previousTime;
			}
		}
	}
	else
	{
		vec3_t		origin;
		EvaluateTrajectory(&ent->s.pos, level.time, origin);
		// trace a line from the previous position to the current position,
		// ignoring interactions with the missile owner
		gi.trace(&tr, ent->currentOrigin, ent->mins, ent->maxs, origin,
			ent->owner ? ent->owner->s.number : ent->s.number, ent->clipmask, G2_COLLIDE, 10);

		if (tr.entityNum != ENTITYNUM_NONE)
		{
			gentity_t* other = &g_entities[tr.entityNum];
			// check for hitting a lightsaber
			if (other->contents & CONTENTS_LIGHTSABER)
			{//hit a lightsaber bbox
				if (other->owner
					&& other->owner->client
					&& !other->owner->client->ps.saberInFlight
					&& (Q_irand(0, (other->owner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] * other->owner->client->ps.forcePowerLevel[FP_SABER_DEFENSE])) == 0
						|| !InFront(ent->currentOrigin, other->owner->currentOrigin, other->owner->client->ps.viewangles, SABER_REFLECT_MISSILE_CONE)))//other->owner->s.number == 0 &&
				{//Jedi cannot block shots from behind!	//re-trace from here, ignoring the lightsaber
					gi.trace(&tr, tr.endpos, ent->mins, ent->maxs, origin, tr.entityNum, ent->clipmask, G2_RETURNONHIT, 10);
				}
			}
		}

		VectorCopy(tr.endpos, ent->currentOrigin);
	}

	// get current angles
	VectorMA(ent->s.apos.trBase, (level.time - ent->s.apos.trTime) * 0.001, ent->s.apos.trDelta, ent->s.apos.trBase);

	//FIXME: Rolling things hitting G2 polys is weird
	///////////////////////////////////////////////////////
//?	if ( tr.fraction != 1 )
	{
		// did we hit or go near a Ghoul2 model?
		for (int i = 0; i < MAX_G2_COLLISIONS; i++)
		{
			if (tr.G2CollisionMap[i].mEntityNum == -1)
			{
				break;
			}

			CCollisionRecord& coll = tr.G2CollisionMap[i];
			gentity_t* hitEnt = &g_entities[coll.mEntityNum];

			// process collision records here...
			// make sure we only do this once, not for all the entrance wounds we might generate
			if ((coll.mFlags & G2_FRONTFACE)/* && !(hitModel)*/ && hitEnt->health)
			{
				if (trHitLoc == HL_NONE)
				{
					G_GetHitLocFromSurfName(&g_entities[coll.mEntityNum], gi.G2API_GetSurfaceName(&g_entities[coll.mEntityNum].ghoul2[coll.mModelIndex], coll.mSurfaceIndex), &trHitLoc, coll.mCollisionPosition, NULL, NULL, ent->methodOfDeath);
				}

				break; // NOTE: the way this whole section was working, it would only get inside of this IF once anyway, might as well break out now
			}
		}
	}
	/////////////////////////////////////////////////////////

	if (tr.startsolid)
	{
		tr.fraction = 0;
	}

	gi.linkentity(ent);

	if (ent->s.pos.trType == TR_STATIONARY && (ent->s.eFlags & EF_MISSILE_STICK))
	{//stuck missiles should check some special stuff
		G_RunStuckMissile(ent);
		return;
	}

	// check think function
	G_RunThink(ent);

	if (ent->s.eType != ET_MISSILE)
	{
		return;		// exploded
	}

	if (ent->mass)
	{
		G_MoverTouchPushTriggers(ent, oldOrg);
	}

	AddSightEvent(ent->owner, ent->currentOrigin, 512, AEL_DISCOVERED, 75);//wakes them up when see a shot passes in front of them
	if (!Q_irand(0, 10))
	{//not so often...
		if (ent->splashDamage && ent->splashRadius)
		{//I'm an exploder, let people around me know danger is coming
			if (ent->s.weapon == WP_TRIP_MINE)
			{
				//???
			}
			else
			{
				if (ent->s.weapon == WP_ROCKET_LAUNCHER && ent->e_ThinkFunc == thinkF_rocketThink)
				{//homing rocket- run like hell!
					AddSightEvent(ent->owner, ent->currentOrigin, ent->splashRadius, AEL_DANGER_GREAT, 50);
				}
				else
				{
					AddSightEvent(ent->owner, ent->currentOrigin, ent->splashRadius, AEL_DANGER, 50);
				}
				AddSoundEvent(ent->owner, ent->currentOrigin, ent->splashRadius, AEL_DANGER);
			}
		}
		else
		{//makes them run from near misses
			AddSightEvent(ent->owner, ent->currentOrigin, 48, AEL_DANGER, 50);
		}
	}

	if (tr.fraction == 1)
	{
		if (ent->s.weapon == WP_THERMAL && ent->s.pos.trType == TR_INTERPOLATE)
		{//a rolling thermal that didn't hit anything
			G_MissileAddAlerts(ent);
		}
		return;
	}

	if (tr.fraction != 1)
	{// never explode or bounce on sky
		if (tr.surfaceFlags & SURF_NOIMPACT)
		{
			if (ent->parent && ent->parent->client && ent->parent->client->hook == ent)
			{
				ent->parent->client->hook = NULL;
				ent->parent->client->hookhasbeenfired = qfalse;
				ent->parent->client->fireHeld = qfalse;
			}
			else
			{
				G_FreeEntity(ent);
				return;
			}
		}
	}

	G_MissileImpact(ent, &tr, trHitLoc);
}



//===========================grapplemod===============================
#define MISSILE_PRESTEP_TIME 50
/*
=================
fire_grapple
=================
*/
gentity_t* fire_grapple(gentity_t * self, vec3_t start, vec3_t dir)
{
	gentity_t* hook;

	VectorNormalize(dir);

	hook = G_Spawn();
	hook->classname = "hook";
	hook->nextthink = level.time + 10000;
	hook->think = Weapon_HookFree;
	hook->s.eType = ET_MISSILE;
	hook->svFlags = SVF_USE_CURRENT_ORIGIN;
	hook->s.weapon = WP_MELEE;
	hook->ownerNum = self->s.number;
	hook->methodOfDeath = MOD_ELECTROCUTE;
	hook->clipmask = MASK_SHOT;
	hook->parent = self;
	hook->target_ent = NULL;
	hook->s.pos.trType = TR_LINEAR;
	hook->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;
	hook->s.otherEntityNum = self->s.number;
	VectorCopy(start, hook->s.pos.trBase);
	VectorScale(dir, 900, hook->s.pos.trDelta);
	SnapVector(hook->s.pos.trDelta);
	VectorCopy(start, hook->currentOrigin);
	self->client->hook = hook;

	return hook;
}

//=============================================================================
//bolt reflection rate without manual reflections being attempted.
int NaturalBoltReflectRate[NUM_FORCE_POWER_LEVELS] =
{
	0,	//FORCE_LEVEL_0
	10,//10,	//FORCE_LEVEL_1
	20,//20, //FORCE_LEVEL_2
	25//25
};

int ManualBoltReflectRate[NUM_FORCE_POWER_LEVELS] =
{
	0,	//FORCE_LEVEL_0
	35,//20,	//FORCE_LEVEL_1
	70,//40, //FORCE_LEVEL_2
	100//50
};

int ReflectionLevel(gentity_t * blocker)
{//determine reflection level.
	if (Manual_Saberblocking(blocker))
	{//manual reflection, bounce to the crosshair
		return FORCE_LEVEL_3;
	}
	if (PM_SaberInAttack(blocker->client->ps.saberMove)
		|| PM_SaberInStart(blocker->client->ps.saberMove)
		|| PM_SaberInReturn(blocker->client->ps.saberMove)
		&& Q_irand(0, 99) < ManualBoltReflectRate[blocker->client->ps.forcePowerLevel[FP_SABER_DEFENSE]])
	{//manual reflection, bounce to the crosshair, roughly
		return FORCE_LEVEL_3;
	}
	else if (Q_irand(0, 99) < NaturalBoltReflectRate[blocker->client->ps.forcePowerLevel[FP_SABER_DEFENSE]])
	{//natural reflection, bounce back to the attacker.
		return FORCE_LEVEL_2;
	}
	else
	{//just deflect the attack
		return FORCE_LEVEL_1;
	}
}

void WP_HandleBoltBlock(gentity_t * bolt, gentity_t * blocker, trace_t * trace)
{//handles all the behavior needed to saber block a blaster bolt.
	vec3_t fwd;
	int otherDefLevel = ReflectionLevel(blocker);
	int randMax = 0;
	gentity_t* prevOwner = &g_entities[bolt->ownerNum];
	qboolean manualDeflect = (blocker->client->ps.ManualBlockingFlags & (1 << MBF_BLOCKING) ? qtrue : qfalse);
	//create the bolt saber block effect			
	G_MissileReflectEffect(blocker, trace->endpos, trace->plane.normal);

	if (manualDeflect && blocker->client->ps.blockPoints > BLOCKPOINTS_FATIGUE)
	{
		if (blocker->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(blocker))
		{
			G_ReflectMissileNPC(blocker, bolt, fwd);
		}
		else
		{
			G_ManualBlockMissile(blocker, bolt, fwd, FP_SABER_DEFENSE);
		}
	}
	else
	{//determine reflection level.
		if (otherDefLevel == FORCE_LEVEL_3)
		{//manual reflection, bounce to the crosshair, roughly
			randMax = 2;
		}
		else if (otherDefLevel == FORCE_LEVEL_2)
		{//natural reflection, bounce back to the attacker.
			randMax = 5;
		}
		else
		{//just deflect the attack
			randMax = 7;
		}
	}

	AngleVectors(blocker->client->ps.viewangles, fwd, NULL, NULL);

	blocker->client->shotsBlocked++;

	if (blocker->client->shotsBlocked >= randMax)
	{
		gentity_t* owner = &g_entities[blocker->ownerNum];
		float distance = VectorBlockDistance(owner->currentOrigin, prevOwner->currentOrigin);
		player->client->shotsBlocked = 0;

		if (distance < 80.0f)
		{//attempted upclose exception
			if (blocker->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(blocker))
			{
				G_ReflectMissileNPC(blocker, bolt, fwd);
			}
			else
			{
				if (manualDeflect && blocker->client->ps.blockPoints > BLOCKPOINTS_FATIGUE)
				{
					G_DeflectMissile(blocker, bolt, fwd);
				}
				else
				{
					G_ReflectMissile(blocker, bolt, fwd, FP_SABER_DEFENSE);
				}
			}
		}
		else
		{
			vec3_t	bounce_dir, angs;
			float	speed;
			int i = 0;

			if (!manualDeflect)
			{
				for (i = 0; i < 3; i++)
				{
					fwd[i] = 1.5f;
				}
			}
			//add some slop factor to the manual reflections.
			if (blocker->client->pers.cmd.forwardmove >= 0)
			{
				vectoangles(fwd, angs);
				angs[PITCH] += flrand(0.5f, 1.5f);
				angs[YAW] += flrand(0.5f, 1.5f);
				AngleVectors(angs, fwd, NULL, NULL);
			}
			else
			{
				vectoangles(fwd, angs);
				AngleVectors(angs, fwd, NULL, NULL);
			}

			//save the original speed
			speed = VectorNormalize(bolt->s.pos.trDelta);
			VectorCopy(fwd, bounce_dir);
			VectorScale(bounce_dir, speed, bolt->s.pos.trDelta);
			bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
			VectorCopy(bolt->currentOrigin, bolt->s.pos.trBase);

			if (bolt->s.weapon != WP_SABER)
			{//you are mine, now!
				if (!bolt->lastEnemy)
				{//remember who originally shot this missile
					bolt->lastEnemy = bolt->owner;
				}
				bolt->owner = owner;
			}
			if (bolt->s.weapon == WP_ROCKET_LAUNCHER)
			{//stop homing
				bolt->e_ThinkFunc = thinkF_NULL;
			}
		}
	}
	else
	{
		if (blocker->s.number >= MAX_CLIENTS && !G_ControlledByPlayer(blocker))
		{
			G_ReflectMissileNPC(blocker, bolt, fwd);
		}
		else
		{
			G_ManualBlockMissile(blocker, bolt, fwd, FP_SABER_DEFENSE);
		}
	}

	blocker->client->ps.saberEventFlags |= SEF_DEFLECTED;

	bolt->activator = prevOwner;

	if (manualDeflect && blocker->client->ps.blockPoints > BLOCKPOINTS_BOLT)
	{
		if (blocker->client->ps.ManualBlockingFlags & (1 << MBF_MBLOCKING))
		{
			int amount = WP_SaberBlockCost(blocker, bolt, trace->endpos);

			amount /= 100 * 40;
			PM_AddBlockFatigue(&blocker->client->ps, amount);
		}
		else
		{
			PM_AddBlockFatigue(&blocker->client->ps, WP_SaberBlockCost(blocker, bolt, trace->endpos));
		}

		WP_SaberBlockNonRandom(blocker, bolt->currentOrigin, qtrue);
	}
	else
	{
		PM_AddBlockFatigue(&blocker->client->ps, FATIGUE_AUTOBOLTBLOCK);
	}

	blocker->client->saberProjBlockTime = level.time + (600 - (blocker->client->ps.forcePowerLevel[FP_SABER_DEFENSE] * 200));
}