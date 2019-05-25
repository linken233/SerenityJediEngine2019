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



#include "g_local.h"
#include "w_saber.h"
#include "qcommon/q_shared.h"

#define	MISSILE_PRESTEP_TIME	50

extern void laserTrapStick(gentity_t* ent, vec3_t endpos, vec3_t normal);
extern void Jedi_Decloak(gentity_t* self);
extern qboolean FighterIsLanded(Vehicle_t* pVeh, playerState_t* parentPS);
extern void PM_AddBlockFatigue(playerState_t* ps, int Fatigue);
extern int WP_SaberBlockCost(gentity_t* defender, gentity_t* attacker, vec3_t hitLoc);
extern float VectorDistance(vec3_t v1, vec3_t v2);
extern int Manual_Saberblocking(gentity_t* defender);
qboolean PM_SaberInStart(int move);
extern qboolean PM_SaberInReturn(int move);
extern qboolean WP_SaberBlockNonRandom(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
void WP_HandleBoltBlock(gentity_t * bolt, gentity_t * blocker, trace_t * trace);
extern int WP_SaberMustBlock(gentity_t* self, gentity_t* atk, qboolean checkBBoxBlock, vec3_t point, int rSaberNum, int rBladeNum);
void WP_flechette_alt_blow(gentity_t* ent);
extern qboolean G_DoDodge(gentity_t* self, gentity_t* shooter, vec3_t dmgOrigin, int hitLoc, int* dmg, int mod);
extern qboolean WP_DoingForcedAnimationForForcePowers(gentity_t* self);
extern qboolean PM_RunningAnim(int anim);
vec3_t g_crosshairWorldCoord = { 0, 0, 0 };
extern qboolean PM_SaberInParry(int move);
extern gentity_t* Jedi_FindEnemyInCone(gentity_t* self, gentity_t* fallback, float minDot);
extern qboolean PM_SaberInReflect(int move);
extern qboolean PM_SaberInIdle(int move);
extern qboolean PM_SaberInAttack(int move);
extern qboolean PM_SaberInTransitionAny(int move);
extern qboolean PM_SaberInSpecialAttack(int anim);
extern void CalcEntitySpot(const gentity_t* ent, const spot_t spot, vec3_t point);
extern qboolean	PM_WalkingOrStanding(gentity_t* self);
extern qboolean G_ControlledByPlayer(gentity_t* self);


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
		(owner->client->ps.fd.forcePowerLevel[powerToUse] > FORCE_LEVEL_2 ||
		(owner->client->ps.fd.forcePowerLevel[powerToUse] > FORCE_LEVEL_1 &&
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
				VectorSubtract(bullseye, missile->r.currentOrigin, bounce_dir);
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
			VectorSubtract(g_crosshairWorldCoord, missile->r.currentOrigin, bounce_dir);
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
				VectorSubtract(missile->owner->r.currentOrigin, missile->r.currentOrigin, bounce_dir);
				VectorNormalize(bounce_dir);
			}
			else
			{
				vec3_t missile_dir;

				VectorSubtract(ent->r.currentOrigin, missile->r.currentOrigin, missile_dir);
				VectorCopy(missile->s.pos.trDelta, bounce_dir);
				VectorScale(bounce_dir, DotProduct(forward, missile_dir), bounce_dir);
				VectorNormalize(bounce_dir);
			}
		}
		else //deflect off at an angle.
		{
			vec3_t deflect_dir, missile_dir;
			float forceFactor;
			VectorSubtract(g_crosshairWorldCoord, missile->r.currentOrigin, deflect_dir);
			VectorCopy(missile->s.pos.trDelta, missile_dir);
			VectorNormalize(missile_dir);
			VectorNormalize(deflect_dir);

			//bigger forceFactors make the reflected shots go closer to the crosshair
			switch (owner->client->ps.fd.forcePowerLevel[powerToUse])
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
			else if (owner->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] <= FORCE_LEVEL_1)
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
	missile->s.pos.trTime = level.time - 10;		// move a bit on the very first frame
	VectorCopy(missile->r.currentOrigin, missile->s.pos.trBase);

	if (missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART)
	{//you are mine, now!
		missile->r.ownerNum = ent->s.number;
	}
	if (missile->s.weapon == WP_ROCKET_LAUNCHER)
	{//stop homing
		missile->think = 0;
		missile->nextthink = 0;
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
			VectorSubtract(bullseye, missile->r.currentOrigin, bounce_dir);
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
			VectorSubtract(missile->owner->r.currentOrigin, missile->r.currentOrigin, bounce_dir);
			VectorNormalize(bounce_dir);
		}
		else
		{
			vec3_t missile_dir;

			VectorSubtract(ent->r.currentOrigin, missile->r.currentOrigin, missile_dir);
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

	missile->s.pos.trTime = level.time - 10;		// move a bit on the very first frame
	VectorCopy(missile->r.currentOrigin, missile->s.pos.trBase);

	if (missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART)
	{//you are mine, now!
		missile->r.ownerNum = ent->s.number;
	}
	if (missile->s.weapon == WP_ROCKET_LAUNCHER)
	{//stop homing
		missile->think = 0;
		missile->nextthink = 0;
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
				VectorSubtract(bullseye, missile->r.currentOrigin, bounce_dir);
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
			VectorSubtract(g_crosshairWorldCoord, missile->r.currentOrigin, bounce_dir);
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
				VectorSubtract(missile->owner->r.currentOrigin, missile->r.currentOrigin, bounce_dir);
				VectorNormalize(bounce_dir);
			}
			else
			{
				vec3_t missile_dir;

				VectorSubtract(ent->r.currentOrigin, missile->r.currentOrigin, missile_dir);
				VectorCopy(missile->s.pos.trDelta, bounce_dir);
				VectorScale(bounce_dir, DotProduct(forward, missile_dir), bounce_dir);
				VectorNormalize(bounce_dir);
			}
		}
		else //deflect off at an angle.
		{
			vec3_t deflect_dir, missile_dir;
			float forceFactor;
			VectorSubtract(g_crosshairWorldCoord, missile->r.currentOrigin, deflect_dir);
			VectorCopy(missile->s.pos.trDelta, missile_dir);
			VectorNormalize(missile_dir);
			VectorNormalize(deflect_dir);

			//bigger forceFactors make the reflected shots go closer to the crosshair
			switch (owner->client->ps.fd.forcePowerLevel[powerToUse])
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
			else if (owner->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] <= FORCE_LEVEL_1)
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
	missile->s.pos.trTime = level.time - 10;		// move a bit on the very first frame
	VectorCopy(missile->r.currentOrigin, missile->s.pos.trBase);

	if (missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART)
	{//you are mine, now!
		missile->r.ownerNum = ent->s.number;
	}
	if (missile->s.weapon == WP_ROCKET_LAUNCHER)
	{//stop homing
		missile->think = 0;
		missile->nextthink = 0;
	}
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
	BG_EvaluateTrajectoryDelta(&ent->s.pos, hitTime, velocity);
	dot = DotProduct(velocity, trace->plane.normal);
	VectorMA(velocity, -2 * dot, trace->plane.normal, ent->s.pos.trDelta);


	if (ent->flags & FL_BOUNCE_SHRAPNEL)
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
	else if (ent->flags & FL_BOUNCE_HALF)
	{
		VectorScale(ent->s.pos.trDelta, 0.65f, ent->s.pos.trDelta);
		// check for stop
		if (trace->plane.normal[2] > 0.2 && VectorLength(ent->s.pos.trDelta) < 40)
		{
			G_SetOrigin(ent, trace->endpos);
			return;
		}
	}

	if (ent->s.weapon == WP_THERMAL)
	{ //slight hack for hit sound
		G_Sound(ent, CHAN_BODY, G_SoundIndex(va("sound/weapons/thermal/bounce%i.wav", Q_irand(1, 2))));
	}
	else if (ent->s.weapon == WP_SABER)
	{
		G_Sound(ent, CHAN_BODY, G_SoundIndex(va("sound/weapons/saber/bounce%i.wav", Q_irand(1, 3))));
	}
	else if (ent->s.weapon == G2_MODEL_PART)
	{
		//Limb bounce sound?
	}

	VectorAdd(ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
	ent->s.pos.trTime = level.time;

	if (ent->bounceCount != -5)
	{
		ent->bounceCount--;
	}
}


/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile(gentity_t* ent)
{
	vec3_t		dir;
	vec3_t		origin;

	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);
	SnapVector(origin);
	G_SetOrigin(ent, origin);

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eType = ET_GENERAL;
	G_AddEvent(ent, EV_MISSILE_MISS, DirToByte(dir));

	ent->freeAfterEvent = qtrue;

	ent->takedamage = qfalse;
	// splash damage
	if (ent->splashDamage)
	{
		if (ent->s.eType == ET_MISSILE//missile
			&& (ent->s.eFlags & EF_JETPACK_ACTIVE)//vehicle missile
			&& ent->r.ownerNum < MAX_CLIENTS)//valid client owner
		{//set my parent to my owner for purposes of damage credit...
			ent->parent = &g_entities[ent->r.ownerNum];
		}
		if (G_RadiusDamage(ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent,
			ent, ent->splashMethodOfDeath))
		{
			if (ent->parent)
			{
				g_entities[ent->parent->s.number].client->accuracy_hits++;
			}
			else if (ent->activator)
			{
				g_entities[ent->activator->s.number].client->accuracy_hits++;
			}
		}
	}

	trap->LinkEntity((sharedEntity_t*)ent);
}

void G_RunStuckMissile(gentity_t* ent)
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
================
G_BounceProjectile
================
*/
void G_BounceProjectile(vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout) {
	vec3_t v, newv;
	float dot;

	VectorSubtract(impact, start, v);
	dot = DotProduct(v, dir);
	VectorMA(v, -2 * dot, dir, newv);

	VectorNormalize(newv);
	VectorMA(impact, 8192, newv, endout);
}


//-----------------------------------------------------------------------------
gentity_t* CreateMissile(vec3_t org, vec3_t dir, float vel, int life,
	gentity_t* owner, qboolean altFire)
	//-----------------------------------------------------------------------------
{
	gentity_t* missile;

	missile = G_Spawn();

	missile->nextthink = level.time + life;
	missile->think = G_FreeEntity;
	missile->s.eType = ET_MISSILE;
	missile->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	missile->parent = owner;
	missile->r.ownerNum = owner->s.number;

	if (altFire)
	{
		missile->s.eFlags |= EF_ALT_FIRING;
	}

	missile->s.pos.trType = TR_LINEAR;
	missile->s.pos.trTime = level.time;// - MISSILE_PRESTEP_TIME;	// NOTENOTE This is a Quake 3 addition over JK2
	missile->target_ent = NULL;

	SnapVector(org);
	VectorCopy(org, missile->s.pos.trBase);
	VectorScale(dir, vel, missile->s.pos.trDelta);
	VectorCopy(org, missile->r.currentOrigin);
	SnapVector(missile->s.pos.trDelta);

	return missile;
}

void G_MissileBounceEffect(gentity_t* ent, vec3_t org, vec3_t dir, qboolean hitWorld)
{
	switch (ent->s.weapon)
	{
	case WP_BOWCASTER:
		if (hitWorld)
		{
			G_PlayEffectID(G_EffectIndex("bowcaster/bounce_wall"), ent->r.currentOrigin, dir);
		}
		else
		{
			G_PlayEffectID(G_EffectIndex("bowcaster/deflect"), ent->r.currentOrigin, dir);
		}
		break;
	case WP_BLASTER:
	case WP_BRYAR_PISTOL:
	case WP_BRYAR_OLD:
		G_PlayEffectID(G_EffectIndex("blaster/deflect"), ent->r.currentOrigin, dir);
		break;
	case WP_REPEATER:
		G_PlayEffectID(G_EffectIndex("repeater/deflectblock"), ent->r.currentOrigin, dir);
		break;
	default:
	{
		gentity_t* te = G_TempEntity(org, EV_GRENADE_BOUNCE);
		VectorCopy(org, te->s.origin);
		VectorCopy(dir, te->s.angles);
		te->s.eventParm = 0;
		te->s.weapon = 0;//saberNum
		te->s.legsAnim = 0;//bladeNum
	}
	break;
	}
}

void G_MissileReflectEffect(gentity_t* ent, vec3_t org, vec3_t dir)
{
	switch (ent->s.weapon)
	{
	case WP_BOWCASTER:
		G_PlayEffectID(G_EffectIndex("bowcaster/deflect"), ent->r.currentOrigin, dir);
		break;
	case WP_BLASTER:
	case WP_BRYAR_PISTOL:
	case WP_BRYAR_OLD:
		G_PlayEffectID(G_EffectIndex("blaster/deflect"), ent->r.currentOrigin, dir);
		break;
	case WP_REPEATER:
		G_PlayEffectID(G_EffectIndex("repeater/deflectblock"), ent->r.currentOrigin, dir);
		break;
	default:
		G_PlayEffectID(G_EffectIndex("blaster/deflect"), ent->r.currentOrigin, dir);
		break;
	}
}

/*
================
G_MissileImpact
================
*/
qboolean G_MissileImpact(gentity_t* ent, trace_t* trace)
{
	gentity_t* other;
	qboolean		hitClient = qfalse;
	qboolean		isKnockedSaber = qfalse;
	int missileDmg;
	other = &g_entities[trace->entityNum];

	// check for bounce
	if (!other->takedamage &&
		(ent->bounceCount > 0 || ent->bounceCount == -5) &&
		(ent->flags & (FL_BOUNCE | FL_BOUNCE_HALF))) {
		G_BounceMissile(ent, trace);
		G_AddEvent(ent, EV_GRENADE_BOUNCE, 0);
		return qtrue;
	}
	else if (ent->neverFree && ent->s.weapon == WP_SABER && (ent->flags & FL_BOUNCE_HALF))
	{ //this is a knocked-away saber
		if (ent->bounceCount > 0 || ent->bounceCount == -5)
		{
			G_BounceMissile(ent, trace);
			G_AddEvent(ent, EV_GRENADE_BOUNCE, 0);
			return qtrue;
		}

		isKnockedSaber = qtrue;
	}

	// I would glom onto the FL_BOUNCE code section above, but don't feel like risking breaking something else
	if ((!other->takedamage && (ent->bounceCount > 0 || ent->bounceCount == -5) && (ent->flags & (FL_BOUNCE_SHRAPNEL))) || ((trace->surfaceFlags & SURF_FORCEFIELD) && !ent->splashDamage && !ent->splashRadius && (ent->bounceCount > 0 || ent->bounceCount == -5)))
	{
		G_BounceMissile(ent, trace);

		if (ent->bounceCount < 1)
		{
			ent->flags &= ~FL_BOUNCE_SHRAPNEL;
		}
		return qtrue;
	}

	if ((other->r.contents & CONTENTS_LIGHTSABER) && !isKnockedSaber)
	{ //hit this person's saber, so..
		gentity_t* otherOwner = &g_entities[other->r.ownerNum];

		if (otherOwner->takedamage && otherOwner->client && otherOwner->client->ps.duelInProgress &&
			otherOwner->client->ps.duelIndex != ent->r.ownerNum)
		{
			goto killProj;
		}
	}
	else if (!isKnockedSaber)
	{
		if (other->takedamage && other->client && other->client->ps.duelInProgress &&
			other->client->ps.duelIndex != ent->r.ownerNum)
		{
			goto killProj;
		}
	}

	if (other->flags & FL_DMG_BY_HEAVY_WEAP_ONLY)
	{
		if (ent->methodOfDeath != MOD_REPEATER_ALT &&
			ent->methodOfDeath != MOD_ROCKET &&
			ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
			ent->methodOfDeath != MOD_ROCKET_HOMING &&
			ent->methodOfDeath != MOD_THERMAL &&
			ent->methodOfDeath != MOD_THERMAL_SPLASH &&
			ent->methodOfDeath != MOD_TRIP_MINE_SPLASH &&
			ent->methodOfDeath != MOD_TIMED_MINE_SPLASH &&
			ent->methodOfDeath != MOD_DET_PACK_SPLASH &&
			ent->methodOfDeath != MOD_VEHICLE &&
			ent->methodOfDeath != MOD_CONC &&
			ent->methodOfDeath != MOD_CONC_ALT &&
			ent->methodOfDeath != MOD_SABER &&
			ent->methodOfDeath != MOD_TURBLAST &&
			ent->methodOfDeath != MOD_TARGET_LASER)
		{
			vec3_t fwd;

			if (trace)
			{
				VectorCopy(trace->plane.normal, fwd);
			}
			else
			{ //oh well
				AngleVectors(other->r.currentAngles, fwd, NULL, NULL);
			}
			if (Manual_Saberblocking(ent) && ent->client->ps.fd.blockPoints > BLOCKPOINTS_FATIGUE)
			{
				G_ManualBlockMissile(other, ent, fwd, FP_SABER_DEFENSE);
			}
			else
			{
				G_ReflectMissile(other, ent, fwd, FP_SABER_DEFENSE);
			}
			G_MissileBounceEffect(ent, ent->r.currentOrigin, fwd, (qboolean)(trace->entityNum == ENTITYNUM_WORLD));
			return qtrue;
		}
	}

	if ((other->flags & FL_SHIELDED) &&
		ent->s.weapon != WP_ROCKET_LAUNCHER &&
		ent->s.weapon != WP_THERMAL &&
		ent->s.weapon != WP_TRIP_MINE &&
		ent->s.weapon != WP_DET_PACK &&
		ent->s.weapon != WP_EMPLACED_GUN &&
		ent->methodOfDeath != MOD_REPEATER_ALT &&
		ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
		ent->methodOfDeath != MOD_TURBLAST &&
		ent->methodOfDeath != MOD_VEHICLE &&
		ent->methodOfDeath != MOD_CONC &&
		ent->methodOfDeath != MOD_CONC_ALT &&
		!(ent->dflags & DAMAGE_HEAVY_WEAP_CLASS))
	{
		vec3_t fwd;

		if (other->client)
		{
			AngleVectors(other->client->ps.viewangles, fwd, NULL, NULL);
		}
		else
		{
			AngleVectors(other->r.currentAngles, fwd, NULL, NULL);
		}
		if (Manual_Saberblocking(ent) && ent->client->ps.fd.blockPoints > BLOCKPOINTS_FATIGUE)
		{
			G_ManualBlockMissile(other, ent, fwd, FP_SABER_DEFENSE);
		}
		else
		{
			G_ReflectMissile(other, ent, fwd, FP_SABER_DEFENSE);
		}
		G_MissileBounceEffect(ent, ent->r.currentOrigin, fwd, (qboolean)(trace->entityNum == ENTITYNUM_WORLD));
		return qtrue;
	}

	if (WP_SaberMustBlock(other, ent, qfalse, trace->endpos, -1, -1)
		&& !WP_DoingForcedAnimationForForcePowers(other))
	{ //only block one projectile per 200ms (to prevent giant swarms of projectiles being blocked)
		other->client->ps.weaponTime = 0;
		if (other->client->ps.fd.blockPoints > BLOCKPOINTS_FAIL)
		{
			WP_HandleBoltBlock(ent, other, trace);
		}
		return qtrue;
	}
	else if ((other->r.contents & CONTENTS_LIGHTSABER) && !isKnockedSaber)
	{ //hit this person's saber, so..
		gentity_t* otherOwner = &g_entities[other->r.ownerNum];

		if (otherOwner->takedamage && otherOwner->client &&
			ent->s.weapon != WP_ROCKET_LAUNCHER &&
			ent->s.weapon != WP_THERMAL &&
			ent->s.weapon != WP_TRIP_MINE &&
			ent->s.weapon != WP_DET_PACK &&
			ent->methodOfDeath != MOD_REPEATER_ALT &&
			ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
			ent->methodOfDeath != MOD_CONC &&
			ent->methodOfDeath != MOD_CONC_ALT)
		{
			if ((otherOwner->client
				&& !PM_SaberInAttack(otherOwner->client->ps.saberMove))
				|| (otherOwner->client && (pm->cmd.buttons & BUTTON_FORCEPOWER
					|| pm->cmd.buttons & BUTTON_FORCEGRIP
					|| pm->cmd.buttons & BUTTON_DASH
					|| pm->cmd.buttons & BUTTON_FORCE_LIGHTNING))
				&& !WP_DoingForcedAnimationForForcePowers(other))
			{//racc - play projectile block animation even in .
				otherOwner->client->ps.weaponTime = 0;
			}
			if (otherOwner->client->ps.fd.blockPoints > BLOCKPOINTS_FAIL)
			{
				WP_HandleBoltBlock(ent, otherOwner, trace);
			}
			return qtrue;
		}
	}

	// check for sticking
	if (!other->takedamage && (ent->s.eFlags & EF_MISSILE_STICK)
		&& ent->s.weapon != WP_SABER)
	{
		laserTrapStick(ent, trace->endpos, trace->plane.normal);
		G_AddEvent(ent, EV_MISSILE_STICK, 0);
		return qtrue;
	}

	// impact damage
	if (other->takedamage && !isKnockedSaber)
	{
		missileDmg = ent->damage;
		if (G_DoDodge(other, &g_entities[other->r.ownerNum], trace->endpos, -1, &missileDmg, ent->methodOfDeath))
		{//player dodged the damage, have missile continue moving.
			return qfalse;
		}
		if (missileDmg)
		{
			vec3_t	velocity;
			qboolean didDmg = qfalse;

			if (LogAccuracyHit(other, &g_entities[ent->r.ownerNum]))
			{
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
				hitClient = qtrue;
			}
			BG_EvaluateTrajectoryDelta(&ent->s.pos, level.time, velocity);
			if (VectorLength(velocity) == 0)
			{
				velocity[2] = 1;	// stepped on a grenade
			}

			if (ent->s.weapon == WP_BOWCASTER || ent->s.weapon == WP_FLECHETTE ||
				ent->s.weapon == WP_ROCKET_LAUNCHER)
			{
				if (ent->s.weapon == WP_FLECHETTE && (ent->s.eFlags & EF_ALT_FIRING))
				{
					if (ent->think == WP_flechette_alt_blow)
						ent->think(ent);
				}
				else
				{
					G_Damage(other, ent, &g_entities[ent->r.ownerNum], velocity, ent->r.currentOrigin, missileDmg, DAMAGE_HALF_ABSORB, ent->methodOfDeath);
					didDmg = qtrue;
				}
			}
			else
			{
				gentity_t* owner = &g_entities[ent->r.ownerNum];
				float distance = VectorDistance(owner->r.currentOrigin, other->r.currentOrigin);
				if (distance <= 100.0f)
				{
					G_Damage(other, ent, owner, velocity, ent->r.currentOrigin, missileDmg * 2, 0, ent->methodOfDeath);
				}
				else if (distance <= 300.0f)
				{
					G_Damage(other, ent, owner, velocity, ent->r.currentOrigin, missileDmg * 1.5, 0, ent->methodOfDeath);
				}
				else
				{
					G_Damage(other, ent, &g_entities[ent->r.ownerNum], velocity, ent->r.currentOrigin, missileDmg, 0, ent->methodOfDeath);
				}
				didDmg = qtrue;
			}

			if (didDmg && other && other->client)
			{ //What I'm wondering is why this isn't in the NPC pain funcs. But this is what SP does, so whatever.
				class_t	npc_class = other->client->NPC_class;
				bclass_t bot_class = other->client->botclass;

				// If we are a robot and we aren't currently doing the full body electricity...
				if (npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE || npc_class == CLASS_MOUSE ||
					npc_class == CLASS_SBD || npc_class == CLASS_BATTLEDROID || npc_class == CLASS_DROIDEKA ||
					npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 || npc_class == CLASS_REMOTE ||
					npc_class == CLASS_MARK1 || npc_class == CLASS_MARK2 || npc_class == CLASS_PROTOCOL ||
					npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST || npc_class == CLASS_SENTRY)
				{// special droid only behaviors
					if (other->client->ps.electrifyTime < level.time + 100)
					{
						// ... do the effect for a split second for some more feedback
						other->client->ps.electrifyTime = level.time + Q_irand(1500, 2000);
					}
				}
				if (bot_class == BCLASS_BATTLEDROID || bot_class == BCLASS_DROIDEKA || bot_class == BCLASS_SBD ||
					bot_class == BCLASS_REMOTE || bot_class == BCLASS_R2D2 || bot_class == BCLASS_R5D2 || bot_class == BCLASS_PROTOCOL)
				{// special droid only behaviors
					if (other->client->ps.electrifyTime < level.time + 100)
					{// ... do the effect for a split second for some more feedback
						other->client->ps.electrifyTime = level.time + Q_irand(1500, 2000);
					}
				}
			}
		}

		if (ent->s.weapon == WP_DEMP2)
		{//a hit with demp2 decloaks people, disables ships
			if (other && other->client && other->client->NPC_class == CLASS_VEHICLE)
			{//hit a vehicle
				if (other->m_pVehicle //valid vehicle ent
					&& other->m_pVehicle->m_pVehicleInfo//valid stats
					&& (other->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER//always affect speeders
						|| (other->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER && ent->classname && Q_stricmp("vehicle_proj", ent->classname) == 0))//only vehicle ion weapons affect a fighter in this manner
					&& !FighterIsLanded(other->m_pVehicle, &other->client->ps)//not landed
					&& !(other->spawnflags & 2))//and not suspended
				{//vehicles hit by "ion cannons" lose control
					if (other->client->ps.electrifyTime > level.time)
					{//add onto it
					 //FIXME: extern the length of the "out of control" time?
						other->client->ps.electrifyTime += Q_irand(200, 500);
						if (other->client->ps.electrifyTime > level.time + 4000)
						{//cap it
							other->client->ps.electrifyTime = level.time + 4000;
						}
					}
					else
					{//start it
					 //FIXME: extern the length of the "out of control" time?
						other->client->ps.electrifyTime = level.time + Q_irand(200, 500);
					}
				}
			}
			else if (other && other->client && other->client->ps.powerups[PW_CLOAKED])
			{
				Jedi_Decloak(other);
				if (ent->methodOfDeath == MOD_DEMP2_ALT)
				{//direct hit with alt disables cloak forever
				 //permanently disable the saboteur's cloak
					other->client->cloakToggleTime = Q3_INFINITE;
				}
				else
				{//temp disable
					other->client->cloakToggleTime = level.time + Q_irand(3000, 10000);
				}
			}
		}
	}
killProj:

	if (!strcmp(ent->classname, "hook"))
	{
		gentity_t* nent;
		vec3_t v;
		nent = G_Spawn();

		if (other->takedamage || other->client || other->s.eType == ET_MOVER)
		{
			G_AddEvent(nent, EV_MISSILE_HIT, DirToByte(trace->plane.normal));
			nent->s.otherEntityNum2 = other->s.number;
			ent->enemy = other;
			v[0] = other->r.currentOrigin[0] + (other->r.mins[0] + other->r.maxs[0]) * 0.5f;
			v[1] = other->r.currentOrigin[1] + (other->r.mins[1] + other->r.maxs[1]) * 0.5f;
			v[2] = other->r.currentOrigin[2] + (other->r.mins[2] + other->r.maxs[2]) * 0.5f;
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
		VectorCopy(ent->r.currentOrigin, ent->parent->client->ps.lastHitLoc);

		trap->LinkEntity((sharedEntity_t*)ent);
		trap->LinkEntity((sharedEntity_t*)nent);

		return qfalse;
	}

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if (other->takedamage && other->client && !isKnockedSaber) {
		G_AddEvent(ent, EV_MISSILE_HIT, DirToByte(trace->plane.normal));
		ent->s.otherEntityNum = other->s.number;
	}
	else if (trace->surfaceFlags & SURF_METALSTEPS) {
		G_AddEvent(ent, EV_MISSILE_MISS_METAL, DirToByte(trace->plane.normal));
	}
	else if (ent->s.weapon != G2_MODEL_PART && !isKnockedSaber) {
		G_AddEvent(ent, EV_MISSILE_MISS, DirToByte(trace->plane.normal));
	}

	if (!isKnockedSaber)
	{
		ent->freeAfterEvent = qtrue;

		// change over to a normal entity right at the point of impact
		ent->s.eType = ET_GENERAL;
	}

	SnapVectorTowards(trace->endpos, ent->s.pos.trBase);	// save net bandwidth

	G_SetOrigin(ent, trace->endpos);

	ent->takedamage = qfalse;
	// splash damage (doesn't apply to person directly hit)
	if (ent->splashDamage) {
		if (G_RadiusDamage(trace->endpos, ent->parent, ent->splashDamage, ent->splashRadius,
			other, ent, ent->splashMethodOfDeath))
		{
			if (!hitClient
				&& g_entities[ent->r.ownerNum].client)
			{
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
			}
		}
	}

	if (ent->s.weapon == G2_MODEL_PART)
	{
		ent->freeAfterEvent = qfalse; //it will free itself
	}

	trap->LinkEntity((sharedEntity_t*)ent);

	return qtrue;
}

/*
================
G_RunMissile
================
*/
extern int G_RealTrace(gentity_t * SaberAttacker, trace_t * tr, vec3_t start, vec3_t mins,
	vec3_t maxs, vec3_t end, int passEntityNum, int contentmask, int rSaberNum, int rBladeNum);

void G_RunMissile(gentity_t * ent)
{
	vec3_t		origin, groundSpot;
	trace_t		tr;
	int			passent;
	qboolean	isKnockedSaber = qfalse;

	if (ent->neverFree && ent->s.weapon == WP_SABER && (ent->flags & FL_BOUNCE_HALF))
	{
		isKnockedSaber = qtrue;
		if (!(ent->s.eFlags & EF_MISSILE_STICK))
		{//only go into gravity mode if we're not stuck to something
			ent->s.pos.trType = TR_GRAVITY;
		}
	}

	// get current position
	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);

	// if this missile bounced off an invulnerability sphere
	if (ent->target_ent) {
		passent = ent->target_ent->s.number;
	}
	else {
		// ignore interactions with the missile owner
		if ((ent->r.svFlags & SVF_OWNERNOTSHARED)
			&& (ent->s.eFlags & EF_JETPACK_ACTIVE))
		{//A vehicle missile that should be solid to its owner
		 //I don't care about hitting my owner
			passent = ent->s.number;
		}
		else
		{
			passent = ent->r.ownerNum;
		}
	}

	// trace a line from the previous position to the current position
	G_RealTrace(ent, &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask, -1, -1);

	if (tr.startsolid || tr.allsolid)
	{
		// make sure the tr.entityNum is set to the entity we're stuck in
	}
	else
	{
		VectorCopy(tr.endpos, ent->r.currentOrigin);
	}

	if (ent->passThroughNum && tr.entityNum == (ent->passThroughNum - 1))
	{
		VectorCopy(origin, ent->r.currentOrigin);
		trap->LinkEntity((sharedEntity_t*)ent);
		goto passthrough;
	}

	trap->LinkEntity((sharedEntity_t*)ent);

	if (ent->s.weapon == G2_MODEL_PART && !ent->bounceCount)
	{
		vec3_t lowerOrg;
		trace_t trG;

		VectorCopy(ent->r.currentOrigin, lowerOrg);
		lowerOrg[2] -= 1;
		trap->Trace(&trG, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, lowerOrg, passent, ent->clipmask, qfalse, 0, 0);

		VectorCopy(trG.endpos, groundSpot);

		if (!trG.startsolid && !trG.allsolid && trG.entityNum == ENTITYNUM_WORLD)
		{
			ent->s.groundEntityNum = trG.entityNum;
		}
		else
		{
			ent->s.groundEntityNum = ENTITYNUM_NONE;
		}
	}

	if (tr.fraction != 1)
	{
		// never explode or bounce on sky
		if (tr.surfaceFlags & SURF_NOIMPACT)
		{
			// If grapple, reset owner
			if (ent->parent && ent->parent->client && ent->parent->client->hook == ent)
			{
				ent->parent->client->hook = NULL;
				ent->parent->client->hookhasbeenfired = qfalse;
				ent->parent->client->fireHeld = qfalse;
			}

			if ((ent->s.weapon == WP_SABER && ent->isSaberEntity) || isKnockedSaber)
			{
				G_RunThink(ent);
				return;
			}
			else if (ent->s.weapon != G2_MODEL_PART)
			{
				G_FreeEntity(ent);
				return;
			}
		}

#if 0 //will get stomped with missile impact event...
		if (ent->s.weapon > WP_NONE && ent->s.weapon < WP_NUM_WEAPONS &&
			(tr.entityNum < MAX_CLIENTS || g_entities[tr.entityNum].s.eType == ET_NPC))
		{ //player or NPC, try making a mark on him
		  //ok, let's try adding it to the missile ent instead (tempents bad!)
			G_AddEvent(ent, EV_GHOUL2_MARK, 0);

			//copy current pos to s.origin, and current projected traj to origin2
			VectorCopy(ent->r.currentOrigin, ent->s.origin);
			BG_EvaluateTrajectory(&ent->s.pos, level.time, ent->s.origin2);

			//the index for whoever we are hitting
			ent->s.otherEntityNum = tr.entityNum;

			if (VectorCompare(ent->s.origin, ent->s.origin2))
			{
				ent->s.origin2[2] += 2.0f; //whatever, at least it won't mess up.
			}
		}
#else
		if (ent->s.weapon > WP_NONE && ent->s.weapon < WP_NUM_WEAPONS &&
			(tr.entityNum < MAX_CLIENTS || g_entities[tr.entityNum].s.eType == ET_NPC))
		{ //player or NPC, try making a mark on him
		  //copy current pos to s.origin, and current projected traj to origin2
			VectorCopy(ent->r.currentOrigin, ent->s.origin);
			BG_EvaluateTrajectory(&ent->s.pos, level.time, ent->s.origin2);

			if (VectorCompare(ent->s.origin, ent->s.origin2))
			{
				ent->s.origin2[2] += 2.0f; //whatever, at least it won't mess up.
			}
		}
#endif

		if (!G_MissileImpact(ent, &tr))
		{//target dodged the damage.
			VectorCopy(origin, ent->r.currentOrigin);
			trap->LinkEntity((sharedEntity_t*)ent);
			return;
		}

		if (tr.entityNum == ent->s.otherEntityNum)
		{ //if the impact event other and the trace ent match then it's ok to do the g2 mark
			ent->s.trickedentindex = 1;
		}

		if (ent->s.eType != ET_MISSILE && ent->s.weapon != G2_MODEL_PART)
		{
			return;		// exploded
		}
	}

passthrough:
	if (ent->s.pos.trType == TR_STATIONARY && (ent->s.eFlags & EF_MISSILE_STICK))
	{//stuck missiles should check some special stuff
		G_RunStuckMissile(ent);
		return;
	}

	if (ent->s.weapon == G2_MODEL_PART)
	{
		if (ent->s.groundEntityNum == ENTITYNUM_WORLD)
		{
			ent->s.pos.trType = TR_LINEAR;
			VectorClear(ent->s.pos.trDelta);
			ent->s.pos.trTime = level.time;

			VectorCopy(groundSpot, ent->s.pos.trBase);
			VectorCopy(groundSpot, ent->r.currentOrigin);

			if (ent->s.apos.trType != TR_STATIONARY)
			{
				ent->s.apos.trType = TR_STATIONARY;
				ent->s.apos.trTime = level.time;

				ent->s.apos.trBase[ROLL] = 0;
				ent->s.apos.trBase[PITCH] = 0;
			}
		}
	}

	// check think function after bouncing
	G_RunThink(ent);
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

int ReflectionLevel(gentity_t * player)
{//handles all the behavior needed to saber block a blaster bolt.

 //determine reflection level.
	if (Manual_Saberblocking(player))
	{//manual reflection, bounce to the crosshair, roughly
		return FORCE_LEVEL_3;
	}
	if (PM_SaberInAttack(player->client->ps.saberMove)
		|| PM_SaberInStart(player->client->ps.saberMove)
		|| PM_SaberInReturn(player->client->ps.saberMove)
		&& Q_irand(0, 99) < ManualBoltReflectRate[player->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE]])
	{//manual reflection, bounce to the crosshair, roughly
		return FORCE_LEVEL_3;
	}
	else if (Q_irand(0, 99) < NaturalBoltReflectRate[player->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE]])
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
	gentity_t* prevOwner = &g_entities[bolt->r.ownerNum];
	qboolean manualDeflect = (blocker->client->ps.ManualBlockingFlags & (1 << MBF_BLOCKING) ? qtrue : qfalse);
	//create the bolt saber block effect			
	G_MissileReflectEffect(blocker, trace->endpos, trace->plane.normal);

	if (manualDeflect && blocker->client->ps.fd.blockPoints > BLOCKPOINTS_FATIGUE)
	{
		G_ManualBlockMissile(blocker, bolt, fwd, FP_SABER_DEFENSE);
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
		gentity_t* owner = &g_entities[blocker->r.ownerNum];
		float distance = VectorDistance(owner->r.currentOrigin, prevOwner->r.currentOrigin);
		blocker->client->shotsBlocked = 0;

		if (distance < 80.0f)
		{//attempted upclose exception
			if (manualDeflect && blocker->client->ps.fd.blockPoints > BLOCKPOINTS_FATIGUE)
			{
				G_DeflectMissile(blocker, bolt, fwd);
			}
			else
			{
				G_ReflectMissile(blocker, bolt, fwd, FP_SABER_DEFENSE);
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
			VectorCopy(bolt->r.currentOrigin, bolt->s.pos.trBase);

			if (bolt->s.weapon != WP_SABER && bolt->s.weapon != G2_MODEL_PART)
			{//you are mine, now!
				bolt->r.ownerNum = blocker->s.number;
			}
			if (bolt->s.weapon == WP_ROCKET_LAUNCHER)
			{//stop homing
				bolt->think = 0;
				bolt->nextthink = 0;
			}
		}
	}
	else
	{
		G_ManualBlockMissile(blocker, bolt, fwd, FP_SABER_DEFENSE);
	}

	blocker->client->ps.saberEventFlags |= SEF_DEFLECTED;

	bolt->activator = prevOwner;

	if (manualDeflect && blocker->client->ps.fd.blockPoints > BLOCKPOINTS_BOLT)
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

		WP_SaberBlockNonRandom(blocker, bolt->r.currentOrigin, qtrue);
	}
	else
	{
		PM_AddBlockFatigue(&blocker->client->ps, FATIGUE_AUTOBOLTBLOCK);
	}


	blocker->client->ps.saberLockFrame = 0; //break out of saberlocks.

	blocker->client->saberProjBlockTime = level.time + (600 - (blocker->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE] * 200));

}



//===========================grapplemod===============================
#define MISSILE_PRESTEP_TIME 50
/*
=================
fire_grapple
=================
*/
gentity_t * fire_grapple(gentity_t * self, vec3_t start, vec3_t dir)
{
	gentity_t* hook;

	VectorNormalize(dir);

	hook = G_Spawn();
	hook->classname = "hook";
	hook->nextthink = level.time + 10000;
	hook->think = Weapon_HookFree;
	hook->s.eType = ET_MISSILE;
	hook->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	hook->s.weapon = WP_MELEE;
	hook->r.ownerNum = self->s.number;
	hook->methodOfDeath = MOD_ELECTROCUTE;
	hook->clipmask = MASK_SHOT;
	hook->parent = self;
	hook->target_ent = NULL;
	hook->s.pos.trType = TR_LINEAR;
	hook->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	hook->s.otherEntityNum = self->s.number; // use to match beam in client
	VectorCopy(start, hook->s.pos.trBase);
	VectorScale(dir, g_grapple_shoot_speed.integer, hook->s.pos.trDelta); // lmo scale speed!
	SnapVector(hook->s.pos.trDelta);			// save net bandwidth
	VectorCopy(start, hook->r.currentOrigin);
	self->client->hook = hook;

	return hook;
}



