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

#include "g_local.h"
#include "b_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "w_local.h"

//-------------------
//	Heavy Repeater
//-------------------

extern qboolean WalkCheck(gentity_t * self);
extern qboolean PM_CrouchAnim(int anim);
//---------------------------------------------------------
static void WP_RepeaterMainFire( gentity_t *ent, vec3_t dir )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage	= weaponData[WP_REPEATER].damage;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t *missile = CreateMissile( start, dir, REPEATER_VELOCITY, 10000, ent );

	missile->classname = "repeater_proj";
	missile->s.weapon = WP_REPEATER;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = REPEATER_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = REPEATER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = REPEATER_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_REPEATER;
	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
static void WP_RepeaterAltFire( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage	= weaponData[WP_REPEATER].altDamage;
	gentity_t *missile = NULL;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	if ( ent->client && ent->client->NPC_class == CLASS_GALAKMECH )
	{
		missile = CreateMissile( start, ent->client->hiddenDir, ent->client->hiddenDist, 10000, ent, qtrue );
	}
	else
	{
		WP_MissileTargetHint(ent, start, forwardVec);
		missile = CreateMissile( start, forwardVec, REPEATER_ALT_VELOCITY, 10000, ent, qtrue );
	}

	missile->classname = "repeater_alt_proj";
	missile->s.weapon = WP_REPEATER;
	missile->mass = 10;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = REPEATER_ALT_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = REPEATER_ALT_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = REPEATER_ALT_NPC_DAMAGE_HARD;
		}
	}

	VectorSet( missile->maxs, REPEATER_ALT_SIZE, REPEATER_ALT_SIZE, REPEATER_ALT_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );
	missile->s.pos.trType = TR_GRAVITY;
	missile->s.pos.trDelta[2] += 40.0f;

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_EXTRA_KNOCKBACK;
	missile->methodOfDeath = MOD_REPEATER_ALT;
	missile->splashMethodOfDeath = MOD_REPEATER_ALT;
	missile->clipmask = MASK_SHOT;
	missile->splashDamage = weaponData[WP_REPEATER].altSplashDamage;
	missile->splashRadius = weaponData[WP_REPEATER].altSplashRadius;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireRepeater( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t	dir, angs;

	vectoangles( forwardVec, angs );

	if (alt_fire)
	{
		WP_RepeaterAltFire(ent);
	}
	else
	{
		if (!(ent->client->ps.forcePowersActive&(1 << FP_SEE)) || ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2)
		{//force sight 2+ gives perfect aim
			if (ent->client && ent->NPC &&
				(ent->client->NPC_class == CLASS_STORMTROOPER ||
					ent->client->NPC_class == CLASS_CLONETROOPER ||
					ent->client->NPC_class == CLASS_STORMCOMMANDO ||
					ent->client->NPC_class == CLASS_SWAMPTROOPER ||
					ent->client->NPC_class == CLASS_DROIDEKA ||
					ent->client->NPC_class == CLASS_SBD ||
					ent->client->NPC_class == CLASS_IMPWORKER ||
					ent->client->NPC_class == CLASS_REBEL ||
					ent->client->NPC_class == CLASS_WOOKIE ||
					ent->client->NPC_class == CLASS_BATTLEDROID))
			{
				angs[PITCH] += (Q_flrand(-1.0f, 1.0f) * (REPEATER_NPC_SPREAD + (6 - ent->NPC->currentAim)*0.25f));//was 0.5f
				angs[YAW] += (Q_flrand(-1.0f, 1.0f) * (REPEATER_NPC_SPREAD + (6 - ent->NPC->currentAim)*0.25f));//was 0.5
			}
			else if (!WalkCheck(ent))//if running aim is shit
			{
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * (RUNNING_SPREAD + 1.5f);
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * (RUNNING_SPREAD + 1.5f);
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_FULL)
			{// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-5.0f, 5.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-3.0f, 3.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (ent->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
			{// add some slop to the fire direction
				angs[PITCH] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-2.0f, 2.0f) * BLASTER_MAIN_SPREAD;
			}
			else if (PM_CrouchAnim(ent->client->ps.legsAnim))
			{// add some slop to the main-fire direction
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * BLASTER_MAIN_SPREAD;
			}
			else
			{// add some slop to the main-fire direction
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * REPEATER_SPREAD;
				angs[YAW] += Q_flrand(-1.0f, 1.0f) * REPEATER_SPREAD;
			}
		}

		AngleVectors( angs, dir, NULL, NULL );

		// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
		WP_RepeaterMainFire( ent, dir );
	}
}