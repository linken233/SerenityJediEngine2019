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

#include "g_local.h"
#include "anims.h"
#include "b_local.h"
#include "bg_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "g_vehicles.h"
#include "../qcommon/tri_coll_test.h"
#include "../cgame/cg_local.h"

//////////Defines////////////////
extern qboolean PM_KickingAnim(int anim);
extern qboolean BG_SaberInNonIdleDamageMove(playerState_t* ps, int AnimIndex);
extern qboolean InFront(vec3_t spot, vec3_t from, vec3_t fromAngles, float threshHold = 0.0f);
extern qboolean PM_SaberInKnockaway(int move);
extern qboolean PM_SaberInBounce(int move);
extern qboolean BG_InSlowBounce(playerState_t* ps);
extern qboolean G_ControlledByPlayer(gentity_t* self);
extern qboolean CheckStagger(gentity_t* defender, gentity_t* attacker);
extern qboolean PM_InKnockDown(playerState_t* ps);
extern qboolean PM_SaberInTransitionAny(int move);
extern void G_AddVoiceEvent(gentity_t* self, int event, int speakDebounceTime);
extern qboolean NPC_IsDarkJedi(gentity_t* self);
extern qboolean NPC_IsLightJedi(gentity_t* self);
extern saberMoveName_t PM_BrokenParryForAttack(int move);
extern qboolean PM_SaberInMassiveBounce(int anim);
extern qboolean PM_InGetUp(playerState_t* ps);
extern qboolean PM_InForceGetUp(playerState_t* ps);
extern qboolean WP_SaberParry(gentity_t* blocker, gentity_t* attacker, int saberNum, int bladeNum);
extern qboolean PM_SaberInParry(int move);
extern saberMoveName_t PM_KnockawayForParry(int move);
extern saberMoveName_t PM_AbsorbtheParry(int move);
extern int G_KnockawayForParry(int move);
extern int G_AbsorbtheParry(int move);
extern void G_SaberBounce(gentity_t* self, gentity_t* other, qboolean hitBody);
extern qboolean PM_VelocityForBlockedMove(playerState_t* ps, vec3_t throwDir);
extern void PM_VelocityForSaberMove(playerState_t* ps, vec3_t throwDir);
extern qboolean WP_SaberLose(gentity_t* self, vec3_t throwDir);
extern cvar_t* g_saberAutoBlocking;
extern qboolean WP_SabersCheckLock(gentity_t* ent1, gentity_t* ent2);
extern qboolean WalkCheck(gentity_t* self);
extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock);
extern qboolean SaberAttacking(gentity_t* self);
extern qboolean PM_SuperBreakWinAnim(int anim);
extern void BG_AddFatigue(playerState_t* ps, int Fatigue);
extern qboolean WP_SaberBlockNonRandom(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
extern saberMoveName_t PM_BrokenParryForParry(int move);
static vec3_t	saberHitposition = { 0,0,1.0 };
extern int WP_SaberMustBlock(gentity_t* self, gentity_t* atk, qboolean checkBBoxBlock, vec3_t point, int rSaberNum, int rBladeNum);
extern qboolean CheckManualBlocking(gentity_t* attacker, gentity_t* defender);
///////////Defines////////////////

qboolean G_InAttackParry(gentity_t* self)
{//checks to see if a player is doing an attack parry

	if ((self->client->pers.cmd.buttons & BUTTON_ATTACK)
		|| (self->client->pers.cmd.buttons & BUTTON_ALT_ATTACK))
	{//can't be pressing an attack button.
		return qfalse;
	}

	if (self->client->ps.userInt3 & (1 << FLAG_PARRIED))
	{//can't attack parry when parried.
		return qfalse;
	}

	if (PM_SaberInTransitionAny(self->client->ps.saberMove))
	{//in transition, start, or return
		return qtrue;
	}

	return qfalse;
}

qboolean G_ActiveParry(gentity_t* self, gentity_t* attacker, vec3_t hitLoc)
{//determines if self (who is blocking) is activing blocking (parrying)
	vec3_t pAngles;
	vec3_t pRight;
	vec3_t parrierMove;
	vec3_t hitPos;
	vec3_t hitFlat; //flatten 2D version of the hitPos.
	float blockDot; //the dot product of our desired parry direction vs the actual attack location.
	qboolean staggered = qfalse;
	qboolean inFront = InFront(attacker->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.25f);

	if (!(self->client->ps.ManualBlockingFlags & (1 << MBF_BLOCKING)))
	{
		return qfalse;
	}

	if (inFront)
	{
		staggered = CheckStagger(self, attacker);
	}
	else if (PM_SaberInKnockaway(self->client->ps.saberMove))
	{//already in parry move, continue parrying anything that hits us as long as 
	 //the attacker is in the same general area that we're facing.
		return qtrue;
	}

	if (PM_KickingAnim(self->client->ps.legsAnim))
	{//can't parry in kick.
		return qfalse;
	}

	if (BG_SaberInNonIdleDamageMove(&self->client->ps, self->localAnimIndex)
		|| PM_SaberInBounce(self->client->ps.saberMove) || BG_InSlowBounce(&self->client->ps))
	{//can't parry if we're transitioning into a block from an attack state.
		return qfalse;
	}

	if (self->client->ps.pm_flags & PMF_DUCKED)
	{//can't parry while ducked or running
		return qfalse;
	}

	if (staggered || PM_InKnockDown(&self->client->ps))
	{//can't block while knocked down or getting up from knockdown, or we are staggered.
		return qfalse;
	}

	//set up flatten version of the location of the incoming attack in orientation
	//to the player.
	VectorSubtract(hitLoc, self->client->ps.origin, hitPos);
	VectorSet(pAngles, 0, self->client->ps.viewangles[YAW], 0);
	AngleVectors(pAngles, NULL, pRight, NULL);
	hitFlat[0] = 0;
	hitFlat[1] = DotProduct(pRight, hitPos);

	//just bump the hit pos down for the blocking since the average left/right slice happens at about origin +10
	hitFlat[2] = hitPos[2] - 10;
	VectorNormalize(hitFlat);

	//set up the vector for the direction the player is trying to parry in.
	parrierMove[0] = 0;
	parrierMove[1] = (self->client->pers.cmd.rightmove);
	parrierMove[2] = -(self->client->pers.cmd.forwardmove);
	VectorNormalize(parrierMove);


	blockDot = DotProduct(hitFlat, parrierMove);

	if (blockDot >= .4)
	{//player successfully blocked in the right direction to do a full parry.
		return qtrue;
	}
	else
	{//player didn't parry in the correct direction, do blockPoints punishment
		if (self->NPC && !G_ControlledByPlayer(self))
		{//bots just randomly parry to make up for them not intelligently parrying.
			if (NPC_PARRYRATE * g_spskill->integer > Q_irand(0, 999))
			{
				return qtrue;
			}
		}
		return qfalse;
	}
}

qboolean G_MBlocking(gentity_t* self, gentity_t* attacker, vec3_t hitLoc)
{//determines if self (who is blocking) is activing mblocking
	vec3_t pAngles;
	vec3_t pRight;
	vec3_t parrierMove;
	vec3_t hitPos;
	vec3_t hitFlat; //flatten 2D version of the hitPos.
	float blockDot; //the dot product of our desired parry direction vs the actual attack location.
	qboolean staggered = qfalse;
	qboolean inFront = InFront(attacker->client->ps.origin, self->client->ps.origin, self->client->ps.viewangles, 0.25f);

	if (!(self->client->ps.ManualBlockingFlags & (1 << MBF_MBLOCKING)))
	{
		return qfalse;
	}

	if (inFront)
	{
		staggered = CheckStagger(self, attacker);
	}
	else if (PM_SaberInKnockaway(self->client->ps.saberMove))
	{//already in parry move, continue parrying anything that hits us as long as 
	 //the attacker is in the same general area that we're facing.
		return qtrue;
	}

	if (PM_KickingAnim(self->client->ps.legsAnim))
	{//can't parry in kick.
		return qfalse;
	}

	if (BG_SaberInNonIdleDamageMove(&self->client->ps, self->localAnimIndex)
		|| PM_SaberInBounce(self->client->ps.saberMove) || BG_InSlowBounce(&self->client->ps))
	{//can't parry if we're transitioning into a block from an attack state.
		return qfalse;
	}

	if (self->client->ps.pm_flags & PMF_DUCKED)
	{//can't parry while ducked or running
		return qfalse;
	}

	if (staggered || PM_InKnockDown(&self->client->ps))
	{//can't block while knocked down or getting up from knockdown, or we are staggered.
		return qfalse;
	}

	//set up flatten version of the location of the incoming attack in orientation
	//to the player.
	VectorSubtract(hitLoc, self->client->ps.origin, hitPos);
	VectorSet(pAngles, 0, self->client->ps.viewangles[YAW], 0);
	AngleVectors(pAngles, NULL, pRight, NULL);
	hitFlat[0] = 0;
	hitFlat[1] = DotProduct(pRight, hitPos);

	//just bump the hit pos down for the blocking since the average left/right slice happens at about origin +10
	hitFlat[2] = hitPos[2] - 10;
	VectorNormalize(hitFlat);

	//set up the vector for the direction the player is trying to parry in.
	parrierMove[0] = 0;
	parrierMove[1] = (self->client->pers.cmd.rightmove);
	parrierMove[2] = -(self->client->pers.cmd.forwardmove);
	VectorNormalize(parrierMove);


	blockDot = DotProduct(hitFlat, parrierMove);

	if (blockDot >= .4)
	{//player successfully blocked in the right direction
		return qtrue;
	}
	else
	{//player didn't parry in the correct direction, do blockPoints punishment
		self->client->ps.blockPoints -= BLOCK_POINTS_MIN;
		if (self->NPC && !G_ControlledByPlayer(self))
		{//bots just randomly parry to make up for them not intelligently parrying.
			if (NPC_PARRYRATE * g_spskill->integer > Q_irand(0, 999))
			{
				return qtrue;
			}
		}
		return qfalse;
	}
}

static QINLINE int G_GetBlockForBlock(int block)
{
	switch (block)
	{
	case BLOCKED_UPPER_RIGHT:
		return LS_PARRY_UR;
		break;
	case BLOCKED_UPPER_RIGHT_PROJ:
		return LS_K1_TR;
		break;
	case BLOCKED_UPPER_LEFT:
		return LS_PARRY_UL;
		break;
	case BLOCKED_UPPER_LEFT_PROJ:
		return LS_K1_TL;
		break;
	case BLOCKED_LOWER_RIGHT:
		return LS_PARRY_LR;
		break;
	case BLOCKED_LOWER_RIGHT_PROJ:
		return LS_REFLECT_LR;
		break;
	case BLOCKED_LOWER_LEFT:
		return LS_PARRY_LL;
		break;
	case BLOCKED_LOWER_LEFT_PROJ:
		return LS_REFLECT_LL;
		break;
	case BLOCKED_TOP:
		return LS_PARRY_UP;
		break;
	case BLOCKED_TOP_PROJ:
		return LS_REFLECT_UP;
		break;
	case BLOCKED_FRONT:
		return LS_PARRY_FRONT;
		break;
	case BLOCKED_FRONT_PROJ:
		return LS_REFLECT_FRONT;
		break;
	default:
		break;
	}

	return LS_NONE;
}

void SabBeh_CrushTheParry(gentity_t* self)
{ //This means that the attack actually hit our saber, and we went to block it.
  //But, one of the above cases says we actually can't. So we will be smashed into a broken parry instead.

	self->client->ps.userInt3 |= (1 << FLAG_SLOWBOUNCE);
	self->client->ps.userInt3 |= (1 << FLAG_OLDSLOWBOUNCE);

	if (NPC_IsDarkJedi(self))
	{// Do taunt/anger...
		G_AddEvent(self, Q_irand(EV_ANGER1, EV_ANGER3), 0);
	}
	else if (NPC_IsLightJedi(self))
	{// Do taunt...
		G_AddEvent(self, Q_irand(EV_COMBAT1, EV_COMBAT3), 0);
	}
	else
	{// Do taunt/anger...
		G_AddEvent(self, Q_irand(EV_PUSHED1, EV_PUSHED3), 0);
	}

	if (PM_SaberInParry(G_GetBlockForBlock(self->client->ps.saberMove)))
	{ //This means that the attack actually hit our saber, and we went to block it.
	  //But, one of the above cases says we actually can't. So we will be smashed into a broken parry instead.
		self->client->ps.saberMove = PM_BrokenParryForParry(G_GetBlockForBlock(self->client->ps.saberMove));
		self->client->ps.saberMove = BLOCKED_PARRY_BROKEN;
	}
}

void SabBeh_AnimateSlowBounce(gentity_t* self, gentity_t* inflictor)
{
	self->client->ps.userInt3 |= (1 << FLAG_SLOWBOUNCE);

	G_AddEvent(self, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000);

	self->client->ps.saberBounceMove = PM_BrokenParryForParry(self->client->ps.saberMove);
	self->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
}

void SabBeh_AnimateHeavySlowBounce(gentity_t * self, gentity_t * inflictor)
{
	self->client->ps.userInt3 |= (1 << FLAG_SLOWBOUNCE);
	self->client->ps.userInt3 |= (1 << FLAG_OLDSLOWBOUNCE);

	if ((self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self)))
	{
		G_AddEvent(self, Q_irand(EV_PUSHED1, EV_PUSHED3), 0);
	}
	else
	{
		G_AddEvent(inflictor, Q_irand(EV_DEFLECT1, EV_DEFLECT3), 0);
	}

	self->client->ps.saberBounceMove = PM_BrokenParryForParry(self->client->ps.saberMove);
	self->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
}

void SabBeh_AnimateMassiveBounce(gentity_t * self, gentity_t * inflictor)//attacker anim
{
	self->client->ps.userInt3 |= (1 << FLAG_SLOWBOUNCE);
	self->client->ps.userInt3 |= (1 << FLAG_OLDSLOWBOUNCE);
	self->client->ps.userInt3 |= (1 << FLAG_BLOCKEDBOUNCE);

	if ((self->s.number < MAX_CLIENTS || G_ControlledByPlayer(self)))
	{
		G_AddEvent(self, Q_irand(EV_PUSHED1, EV_PUSHED3), 0);
	}
	else
	{
		G_AddEvent(inflictor, Q_irand(EV_DEFLECT1, EV_DEFLECT3), 0);
	}

	self->client->ps.saberBounceMove = PM_BrokenParryForParry(self->client->ps.saberMove);
	self->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
}

void SabBeh_AnimateMassiveSlowBounce(gentity_t * hitEnt, qboolean move)//defender anim
{

	if (PM_SaberInMassiveBounce(hitEnt->client->ps.torsoAnim))
	{
		return;
	}

	if (PM_InGetUp(&hitEnt->client->ps) || PM_InForceGetUp(&hitEnt->client->ps))
	{
		return;
	}

	G_AddEvent(hitEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), 0);

	int useAnim = -1;

	switch (move)
	{
	case LS_PARRY_UP:
		if (Q_irand(0, 1))
		{
			useAnim = BOTH_BASHED1;
		}
		else
		{
			useAnim = BOTH_H1_S1_T_;
		}
		break;
	case LS_PARRY_UR:
		useAnim = BOTH_H1_S1_TR;
		break;
	case LS_PARRY_UL:
		useAnim = BOTH_H1_S1_TL;
		break;
	case LS_PARRY_LR:
		useAnim = BOTH_H1_S1_BR;
		break;
	case LS_PARRY_LL:
		useAnim = BOTH_H1_S1_BL;
		break;
	case LS_READY:
		useAnim = BOTH_H1_S1_B_;
		break;
	}

	NPC_SetAnim(hitEnt, SETANIM_TORSO, useAnim, AFLAG_PACE);

	if (PM_SaberInMassiveBounce(hitEnt->client->ps.torsoAnim))
	{
		hitEnt->client->ps.saberMove = LS_NONE;
		hitEnt->client->ps.saberBlocked = BLOCKED_NONE;
		hitEnt->client->ps.weaponTime = hitEnt->client->ps.torsoAnimTimer;
		hitEnt->client->MassiveBounceAnimTime = hitEnt->client->ps.torsoAnimTimer + level.time;
	}
}

void AnimateblockKnockaway(gentity_t * self, gentity_t * inflictor)
{//do an knockaway.
	if (!PM_SaberInKnockaway(self->client->ps.saberMove) && !PM_InKnockDown(&self->client->ps))
	{
		if (!PM_SaberInParry(self->client->ps.saberMove))
		{
			self->client->ps.saberMove = PM_KnockawayForParry(self->client->ps.saberBlocked);
			self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;
		}
		else
		{
			self->client->ps.saberMove = G_KnockawayForParry(self->client->ps.saberMove);
			self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;
		}
	}
}

void AnimateoldblockKnockaway(gentity_t * self, gentity_t * inflictor)
{//do an knockaway.
	if (!PM_SaberInKnockaway(self->client->ps.saberMove) && !PM_InKnockDown(&self->client->ps))
	{
		if (!PM_SaberInParry(self->client->ps.saberMove))
		{
			self->client->ps.saberMove = PM_AbsorbtheParry(self->client->ps.saberBlocked);
			self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;
		}
		else
		{
			self->client->ps.saberMove = G_AbsorbtheParry(self->client->ps.saberMove);
			self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;
		}
	}
}

void AnimateKnockdown(gentity_t * self, gentity_t * inflictor)
{
	if (self->health > 0 && !PM_InKnockDown(&self->client->ps))
	{ //if still alive knock them down
	  //use SP style knockdown
		int throwStr = Q_irand(50, 75);

		//push person away from attacker
		vec3_t pushDir;

		if (inflictor)
		{
			VectorSubtract(self->client->ps.origin, inflictor->currentOrigin, pushDir);
		}
		else
		{//this is possible in debug situations or where there's mishaps without another player (IE rarely)
			AngleVectors(self->client->ps.viewangles, pushDir, NULL, NULL);
			//reverse it
			VectorScale(pushDir, -1, pushDir);
		}
		pushDir[2] = 0;
		VectorNormalize(pushDir);
		G_Throw(self, pushDir, throwStr);

		//translate throw strength to the force used for determining knockdown anim.
		if (throwStr > 65)
		{//really got nailed, play the hard version of the knockdown anims
			throwStr = 300;
		}

		//knock them on their ass!
		G_Knockdown(self, inflictor, pushDir, throwStr, qtrue);

		//Count as kill for attacker if the other player falls to his death.
		if (inflictor && inflictor->inuse && inflictor->client)
		{
			self->client->ps.otherKiller = self->s.number;
			self->client->ps.otherKillerTime = level.time + 8000;
			self->client->ps.otherKillerDebounceTime = level.time + 100;
		}
	}
}

qboolean SabBeh_butterfingers(gentity_t * self, gentity_t * inflictor)
{
	if (!PM_InKnockDown(&self->client->ps))
	{
		vec3_t	throwDir;
		if (!PM_VelocityForBlockedMove(&inflictor->client->ps, throwDir))
		{
			PM_VelocityForSaberMove(&self->client->ps, throwDir);
		}
		WP_SaberLose(self, throwDir);
	}

	return qtrue;
}

void SabBeh_AddBalance(gentity_t * self, gentity_t * inflictor, int amount, qboolean attack)
{
	if (!WalkCheck(self))
	{//running or moving very fast, can't balance as well
		if (amount > 0)
		{
			amount *= 2;
		}
		else
		{
			amount = amount * .5f;
		}
	}

	self->client->ps.saberAttackChainCount += amount;

	if (self->client->ps.saberAttackChainCount < MISHAPLEVEL_NONE)
	{
		self->client->ps.saberAttackChainCount = MISHAPLEVEL_NONE;
	}
	else if (self->client->ps.saberAttackChainCount > MISHAPLEVEL_MAX)
	{//overflowing causes a full mishap.
		int randNum = Q_irand(0, 2);
		switch (randNum)
		{
		case 0:
			SabBeh_butterfingers(self, inflictor);
			break;
		case 1:
			AnimateKnockdown(self, inflictor);
			break;
		};
		self->client->ps.saberAttackChainCount = MISHAPLEVEL_HEAVY;
	}
	else if (self->client->ps.blockPoints < BLOCKPOINTS_WARNING)
	{//overflowing causes a full mishap.
		int randNum = Q_irand(0, 2);
		switch (randNum)
		{
		case 0:
			SabBeh_butterfingers(self, inflictor);
			break;
		case 1:
			AnimateKnockdown(self, inflictor);
			break;
		};
		self->client->ps.blockPoints = BLOCKPOINTS_HALF;
	}
}

qboolean SabBeh_AttackVsBlock(gentity_t * attacker, gentity_t * blocker, int saberNum, int bladeNum)
{//if the attack is blocked -(Im the attacker)
	qboolean atkparry = G_InAttackParry(blocker);
	qboolean perfectparry = G_MBlocking(blocker, attacker, saberHitposition);

	if (saberNum == 0)
	{//can only lose right-hand saber for now

		if (!(attacker->client->ps.saber[saberNum].saberFlags & SFL_NOT_DISARMABLE)
			&& attacker->client->ps.saberAttackChainCount >= MISHAPLEVEL_MAX
			&& blocker->client->ps.blockPoints > BLOCKPOINTS_HALF)
		{//hard mishap.
			vec3_t	throwDir;
			if (!PM_VelocityForBlockedMove(&blocker->client->ps, throwDir))
			{
				PM_VelocityForSaberMove(&attacker->client->ps, throwDir);
			}
			WP_SaberLose(attacker, throwDir);
			attacker->client->ps.saberAttackChainCount = MISHAPLEVEL_LIGHT;
		}
		else if (!(attacker->client->ps.saber[saberNum].saberFlags & SFL_NOT_DISARMABLE)
			&& perfectparry)
		{//hard mishap.
			vec3_t	throwDir;
			if (!PM_VelocityForBlockedMove(&blocker->client->ps, throwDir))
			{
				PM_VelocityForSaberMove(&attacker->client->ps, throwDir);
			}
			WP_SaberLose(attacker, throwDir);
		}
		else if (!(attacker->client->ps.saber[saberNum].saberFlags & SFL_NOT_DISARMABLE)
			&& attacker->client->ps.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_2
			&& Q_irand(0, blocker->client->ps.SaberDisarmBonus(0)) > 0
			&& (blocker->s.number || g_saberAutoBlocking->integer || !Q_irand(0, 2)))
		{//knocked the saber right out of his hand!
			vec3_t	throwDir;
			if (!PM_VelocityForBlockedMove(&blocker->client->ps, throwDir))
			{
				PM_VelocityForSaberMove(&attacker->client->ps, throwDir);
			}
			WP_SaberLose(attacker, throwDir);
		}
	}
	else if (attacker->client->ps.saberAttackChainCount >= MISHAPLEVEL_MAX)
	{//hard mishap.
		SabBeh_butterfingers(attacker, blocker);
		attacker->client->ps.saberAttackChainCount = MISHAPLEVEL_LIGHT;
		return qtrue;
	}
	else if (attacker->client->ps.saberAttackChainCount >= MISHAPLEVEL_THIRTEEN)
	{//heavy slow bounce
		SabBeh_AnimateMassiveSlowBounce(attacker, qtrue);
		return qtrue;
	}
	else if (attacker->client->ps.saberAttackChainCount >= MISHAPLEVEL_TEN)
	{//heavy slow bounce
		SabBeh_AnimateMassiveBounce(attacker, blocker);
		return qtrue;
	}
	else if (attacker->client->ps.saberAttackChainCount >= MISHAPLEVEL_SEVEN)
	{//slow bounce
		SabBeh_AnimateHeavySlowBounce(attacker, blocker);
		return qtrue;
	}
	else if (atkparry)
	{
		G_SaberBounce(attacker, blocker, qfalse);
		G_SaberBounce(blocker, attacker, qfalse);
	}
	else
	{
		SabBeh_AnimateSlowBounce(attacker, blocker);
	}

	
	return qfalse;
}

qboolean SabBeh_RollBalance(gentity_t * self, gentity_t * blocker, qboolean forceMishap)
{//JaceSolaris making it feel like EoC MP/OJP
 //if the attack is blocked -(Im the attacker)

	if (self->client->ps.saberAttackChainCount >= MISHAPLEVEL_FULL)
	{//hard mishap.
		SabBeh_butterfingers(self, blocker);
		self->client->ps.saberAttackChainCount = MISHAPLEVEL_LIGHT;
		return qtrue;
	}
	else if (self->client->ps.saberAttackChainCount >= MISHAPLEVEL_THIRTEEN)
	{//heavy slow bounce
		SabBeh_AnimateMassiveSlowBounce(self, qtrue);
		return qtrue;
	}
	else if (self->client->ps.saberAttackChainCount >= MISHAPLEVEL_TEN)
	{//heavy slow bounce
		SabBeh_AnimateMassiveBounce(self, blocker);
		return qtrue;
	}
	else if (self->client->ps.saberAttackChainCount >= MISHAPLEVEL_SEVEN)
	{//slow bounce
		SabBeh_AnimateHeavySlowBounce(self, blocker);
		return qtrue;
	}
	else if (forceMishap)
	{//perform a slow bounce even if we don't have enough mishap for it.
		SabBeh_AnimateSlowBounce(self, blocker);
		return qtrue;
	}
	else
	{
		SabBeh_AnimateSlowBounce(self, blocker);
		return qtrue;
	}

	return qfalse;
}

qboolean SabBeh_AttackVsBlockAdvanced(gentity_t * attacker, gentity_t * blocker, int saberNum, int bladeNum)
{//if the attack is blocked -(Im the attacker)
	qboolean startSaberLock = qfalse;
	qboolean atkparry = G_InAttackParry(attacker);
	qboolean perfectparry = G_MBlocking(blocker, attacker, saberHitposition);
	qboolean parried = G_ActiveParry(blocker, attacker, saberHitposition);
	qboolean atkfake = (attacker->client->ps.userInt3 & (1 << FLAG_ATTACKFAKE)) ? qtrue : qfalse;

	if ((parried || perfectparry) && blocker->NPC && !G_ControlledByPlayer(blocker)
		&& NPC_PARRYRATE * g_spskill->integer > Q_irand(0, 999))
	{//npc performed an attack parry (by cheating a bit)
		atkparry = qtrue;
	}

	if (SaberAttacking(attacker))
	{
		G_SaberBounce(attacker, blocker, qfalse);		
	}
	if (saberNum == 0)
	{//can only lose right-hand saber for now

		if (!(attacker->client->ps.saber[saberNum].saberFlags & SFL_NOT_DISARMABLE)
			&& attacker->client->ps.saberAttackChainCount > MISHAPLEVEL_MAX
			&& blocker->client->ps.blockPoints > BLOCKPOINTS_HALF)
		{//hard mishap.
			vec3_t	throwDir;
			if (!PM_VelocityForBlockedMove(&blocker->client->ps, throwDir))
			{
				PM_VelocityForSaberMove(&attacker->client->ps, throwDir);
			}
			WP_SaberLose(attacker, throwDir);
			attacker->client->ps.saberAttackChainCount = MISHAPLEVEL_LIGHT;
		}
		else if (!(attacker->client->ps.saber[saberNum].saberFlags & SFL_NOT_DISARMABLE)
			&& perfectparry)
		{//hard mishap.
			vec3_t	throwDir;
			if (!PM_VelocityForBlockedMove(&blocker->client->ps, throwDir))
			{
				PM_VelocityForSaberMove(&attacker->client->ps, throwDir);
			}
			WP_SaberLose(attacker, throwDir);
		}
		else if (!(attacker->client->ps.saber[saberNum].saberFlags & SFL_NOT_DISARMABLE)
			&& attacker->client->ps.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_2
			&& Q_irand(0, blocker->client->ps.SaberDisarmBonus(0)) > 0
			&& (blocker->s.number || g_saberAutoBlocking->integer || !Q_irand(0, 2)))
		{//knocked the saber right out of his hand!
			vec3_t	throwDir;
			if (!PM_VelocityForBlockedMove(&blocker->client->ps, throwDir))
			{
				PM_VelocityForSaberMove(&attacker->client->ps, throwDir);
			}
			WP_SaberLose(attacker, throwDir);
		}
	}
	if (PM_SuperBreakWinAnim(attacker->client->ps.torsoAnim))
	{//attacker was attempting a superbreak and he hit someone who could block the move, rail him for screwing up.
		SabBeh_RollBalance(attacker, blocker, qtrue);
		SabBeh_AddBalance(attacker, blocker, 2, qtrue);
		SabBeh_AddBalance(blocker, attacker, -1, qfalse);
	}
	else if (atkfake)
	{//attacker faked before making this attack, treat like standard attack/attack
		if ((parried || perfectparry))
		{//defender parried the attack fake.
			SabBeh_RollBalance(attacker, blocker, atkparry);
			SabBeh_AddBalance(attacker, blocker, MPCOST_PARRIED_ATTACKFAKE, qtrue);

			if (blocker->client->ps.ManualBlockingFlags & (1 << MBF_BLOCKING))
			{
				attacker->client->ps.userInt3 |= (1 << FLAG_QUICKPARRY);
			}
			else
			{
				attacker->client->ps.userInt3 |= (1 << FLAG_PARRIED);
			}

			SabBeh_AddBalance(blocker, attacker, MPCOST_PARRYING_ATTACKFAKE, qfalse);
			G_SaberBounce(attacker, blocker, qfalse);
		}
		else
		{//otherwise, the defender stands a good chance of having his defensives broken.	
			SabBeh_AddBalance(attacker, blocker, -1, qtrue);

			if (attacker->client->ps.saberAnimLevel == SS_DESANN)
			{
				SabBeh_AddBalance(blocker, attacker, 2, qfalse);
			}

			if (WP_SabersCheckLock(attacker, blocker))
			{
				attacker->client->ps.userInt3 |= (1 << FLAG_LOCKWINNER);
				attacker->client->ps.saberBlocked = BLOCKED_NONE;
				blocker->client->ps.saberBlocked = BLOCKED_NONE;
				startSaberLock = qtrue;
			}
			SabBeh_RollBalance(attacker, blocker, qtrue);
			G_SaberBounce(attacker, blocker, qfalse);
		}
	}
	else if (BG_InSlowBounce(&blocker->client->ps)
		&& blocker->client->ps.userInt3 & (1 << FLAG_OLDSLOWBOUNCE)
		&& attacker->client->ps.saberAnimLevel == SS_TAVION)
	{//blocker's saber was directly hit while in a slow bounce, disarm the blocker!
		SabBeh_butterfingers(blocker, attacker);
		blocker->client->ps.saberAttackChainCount = MISHAPLEVEL_NONE;
		//set attacker
		SabBeh_AddBalance(attacker, blocker, -3, qtrue);
		attacker->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
		G_SaberBounce(attacker, blocker, qfalse);
		SabBeh_RollBalance(attacker, blocker, qtrue);
	}
	else
	{//standard attack
	 //set blocker
		//set attacker
		if ((parried || perfectparry))
		{
			//parry values
			if (attacker->client->ps.saberMove == LS_A_LUNGE
				|| attacker->client->ps.saberMove == LS_SPINATTACK
				|| attacker->client->ps.saberMove == LS_SPINATTACK_DUAL
				|| attacker->client->ps.saberMove == LS_SPINATTACK_GRIEV)
			{//attacker's lunge was parried, force mishap.
				SabBeh_RollBalance(attacker, blocker, qtrue);
			}
			else
			{
				SabBeh_RollBalance(attacker, blocker, atkparry);
			}
			SabBeh_AddBalance(attacker, blocker, MPCOST_PARRIED, qtrue);

			if (blocker->client->ps.ManualBlockingFlags & (1 << MBF_BLOCKING))
			{
				attacker->client->ps.userInt3 |= (1 << FLAG_QUICKPARRY);
			}
			else
			{
				attacker->client->ps.userInt3 |= (1 << FLAG_PARRIED);
			}

			SabBeh_AddBalance(blocker, attacker, MPCOST_PARRYING, qfalse);
			G_SaberBounce(attacker, blocker, qfalse);

		}
		else
		{//blocked values
			SabBeh_AddBalance(attacker, blocker, -1, qtrue);
			if (attacker->client->ps.saberAnimLevel == SS_TAVION)
			{//aqua styles deals MP to players that don't parry it.
				SabBeh_AddBalance(blocker, attacker, 2, qfalse);
			}
			else if (attacker->client->ps.saberAnimLevel == SS_STRONG)
			{
				blocker->client->ps.forcePower -= 2;
			}
			G_SaberBounce(attacker, blocker, qfalse);
		}
	}

	if (!OnSameTeam(attacker, blocker))
	{//don't do parries or charge/regen DP unless we're in a situation where we can actually hurt the target.
		if (parried || perfectparry)
		{//parries don't cost any DP and they have a special animation
			AnimateblockKnockaway(blocker, attacker);
		}
		else if (!startSaberLock)
		{//normal saber blocks
			//update the blocker's block move
			WP_SaberParry(blocker, attacker, saberNum, bladeNum);
		}
	}
	return qfalse;
}

qboolean SabBeh_BlockvsAttack(gentity_t * blocker, gentity_t * attacker, int saberNum, int bladeNum)
{//make me parry -(Im the blocker)
	qboolean perfectparry = G_MBlocking(blocker, attacker, saberHitposition);

	if (blocker->client->ps.blockPoints < BLOCKPOINTS_WARNING)
	{
		WP_SaberParry(blocker, attacker, saberNum, bladeNum);
		SabBeh_CrushTheParry(blocker);
	}
	else if (blocker->client->ps.blockPoints < BLOCKPOINTS_FATIGUE)
	{
		WP_SaberParry(blocker, attacker, saberNum, bladeNum);
		SabBeh_AnimateMassiveSlowBounce(blocker, qtrue);
	}
	else if (blocker->client->ps.blockPoints < BLOCKPOINTS_HALF)
	{
		WP_SaberParry(blocker, attacker, saberNum, bladeNum);
		AnimateoldblockKnockaway(blocker, attacker);
	}
	else if (blocker->client->ps.blockPoints < BLOCKPOINTS_FULL)
	{
		WP_SaberParry(blocker, attacker, saberNum, bladeNum);
		AnimateblockKnockaway(blocker, attacker);
	}
	else if (perfectparry)
	{
		WP_SaberParry(blocker, attacker, saberNum, bladeNum);
	}
	else
	{
		WP_SaberParry(blocker, attacker, saberNum, bladeNum);
	}
	return qfalse;
}

qboolean SabBeh_AttackvsAttack(gentity_t * self, gentity_t * otherOwner, int saberNum, int bladeNum)
{ //set the saber behavior for two attacking blades hitting each other
	qboolean atkfake = (self->client->ps.userInt3 & (1 << FLAG_ATTACKFAKE)) ? qtrue : qfalse;
	qboolean otherfake = (otherOwner->client->ps.userInt3 & (1 << FLAG_ATTACKFAKE)) ? qtrue : qfalse;

	if (atkfake && !otherfake)
	{//self is sololy faking

		if (WP_SabersCheckLock(self, otherOwner))
		{
			self->client->ps.userInt3 |= (1 << FLAG_LOCKWINNER);
			self->client->ps.saberBlocked = BLOCKED_NONE;
			otherOwner->client->ps.saberBlocked = BLOCKED_NONE;
		}
		SabBeh_AddBalance(self, otherOwner, 1, qtrue);
		SabBeh_AddBalance(otherOwner, self, -1, qtrue);
	}
	else if (otherOwner && !self)
	{//only otherOwner is faking

		if (WP_SabersCheckLock(otherOwner, self))
		{
			self->client->ps.saberBlocked = BLOCKED_NONE;
			otherOwner->client->ps.userInt3 |= (1 << FLAG_LOCKWINNER);
			otherOwner->client->ps.saberBlocked = BLOCKED_NONE;
		}
		SabBeh_AddBalance(self, otherOwner, -1, qtrue);
		SabBeh_AddBalance(otherOwner, self, 1, qtrue);
	}
	else
	{//either both are faking or neither is faking.  Either way, it's cancelled out
		SabBeh_AddBalance(self, otherOwner, 1, qtrue);
		SabBeh_AddBalance(otherOwner, self, 1, qtrue);
		//bounce them off each other
		G_SaberBounce(self, otherOwner, qfalse);
		G_SaberBounce(otherOwner, self, qfalse);
	}
	return qfalse;
}

void Advanced_Saberblocking(gentity_t* self, gentity_t* otherOwner, int saberNum, int bladeNum)
{
	trace_t		tr;

	if (!otherOwner)
	{//not a saber-on-saber hit, no mishap handling.
		return;
	}

	if (BG_SaberInNonIdleDamageMove(&self->client->ps, self->localAnimIndex))
	{//self is attacking
		if (BG_SaberInNonIdleDamageMove(&otherOwner->client->ps, otherOwner->localAnimIndex))
		{//and otherOwner is attacking
			SabBeh_AttackvsAttack(self, otherOwner, saberNum, bladeNum);
		}
		else if (WP_SaberMustBlock(otherOwner, self, qfalse, tr.endpos, -1, -1))
		{//and otherOwner is blocking or parrying
		 //this is called with dual with both sabers
			SabBeh_BlockvsAttack(otherOwner, self, saberNum, bladeNum);
		}
		else
		{//otherOwner in some other state
			G_SaberBounce(otherOwner, self, qfalse);
		}
	}
	else if (WP_SaberMustBlock(self, otherOwner, qfalse, tr.endpos, -1, -1))
	{//self is blocking or parrying
		if (BG_SaberInNonIdleDamageMove(&otherOwner->client->ps, otherOwner->localAnimIndex))
		{//and otherOwner is attacking
			SabBeh_BlockvsAttack(self, otherOwner, saberNum, bladeNum);
		}
		else if (WP_SaberMustBlock(otherOwner, self, qfalse, tr.endpos, -1, -1))
		{//and otherOwner is blocking or parrying
			CheckManualBlocking(self, otherOwner);
		}
		else
		{//otherOwner in some other state
			G_SaberBounce(otherOwner, self, qfalse);
		}
	}
	else
	{//whatever other states self can be in.  (returns, bounces, or something)
		G_SaberBounce(otherOwner, self, qfalse);
	}
}