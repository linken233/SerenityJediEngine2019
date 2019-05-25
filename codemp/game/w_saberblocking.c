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
#include "bg_local.h"
#include "w_saber.h"
#include "ai_main.h"
#include "ghoul2/G2.h"

//////////Defines////////////////
extern qboolean BG_SaberInNonIdleDamageMove(playerState_t* ps, int AnimIndex);
extern qboolean PM_SaberInBounce(int move);
extern qboolean BG_InSlowBounce(playerState_t* ps);
extern bot_state_t* botstates[MAX_CLIENTS];
extern qboolean CheckStagger(gentity_t* defender, gentity_t* attacker);
extern qboolean PM_SaberInTransitionAny(int move);
extern qboolean PM_SuperBreakWinAnim(int anim);
extern qboolean WalkCheck(gentity_t* self);
extern void G_SaberBounce(gentity_t* self, gentity_t* other, qboolean hitBody);
extern qboolean WP_SabersCheckLock(gentity_t* ent1, gentity_t* ent2);
extern qboolean WP_SaberBlockNonRandom(gentity_t* self, vec3_t hitloc, qboolean missileBlock);
extern int WP_SaberBlockCost(gentity_t* defender, gentity_t* attacker, vec3_t hitLoc);
extern void BG_AddFatigue(playerState_t* ps, int Fatigue);
extern void G_AddVoiceEvent(gentity_t* self, int event, int speakDebounceTime);
extern qboolean NPC_IsDarkJedi(gentity_t* self);
extern qboolean NPC_IsLightJedi(gentity_t* self);
extern saberMoveName_t PM_BrokenParryForParry(int move);
extern qboolean PM_SaberInMassiveBounce(int anim);
extern saberMoveName_t PM_BrokenParryForAttack(int move);
extern qboolean PM_InGetUp(playerState_t* ps);
extern qboolean PM_InForceGetUp(playerState_t* ps);
extern qboolean SaberAttacking(gentity_t* self);
extern qboolean G_ControlledByPlayer(gentity_t* self);
//////////Defines////////////////


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

	if (BG_KickingAnim(self->client->ps.legsAnim))
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
		if (self->r.svFlags & SVF_BOT)
		{//bots just randomly parry to make up for them not intelligently parrying.
			if (BOT_PARRYRATE * botstates[self->s.number]->settings.skill > Q_irand(0, 999))
			{
				return qtrue;
			}
		}
		else if (self->s.eType == ET_NPC)
		{
			if (BOT_PARRYRATE * g_npcspskill.integer > Q_irand(0, 999))
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

	if (BG_KickingAnim(self->client->ps.legsAnim))
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
		self->client->ps.fd.blockPoints -= BLOCK_POINTS_MIN;
		if (self->r.svFlags & SVF_BOT)
		{//bots just randomly parry to make up for them not intelligently parrying.
			if (BOT_PARRYRATE * botstates[self->s.number]->settings.skill > Q_irand(0, 999))
			{
				return qtrue;
			}
		}
		else if (self->s.eType == ET_NPC)
		{
			if (BOT_PARRYRATE * g_npcspskill.integer > Q_irand(0, 999))
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
  //But, one of the cases says we actually can't. So we will be smashed into a broken parry instead.

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

	if (PM_SaberInParry(G_GetBlockForBlock(self->client->ps.saberBlocked)))
	{ //This means that the attack actually hit our saber, and we went to block it.
	  //But, one of the above cases says we actually can't. So we will be smashed into a broken parry instead.
		self->client->ps.saberMove = PM_BrokenParryForParry(G_GetBlockForBlock(self->client->ps.saberBlocked));
		self->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
	}
}

void SabBeh_AnimateSlowBounce(gentity_t* self, gentity_t* inflictor)
{
	self->client->ps.userInt3 |= (1 << FLAG_SLOWBOUNCE);

	G_AddEvent(self, Q_irand(EV_PUSHED1, EV_PUSHED3), 0);

	self->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
}

void SabBeh_AnimateHeavySlowBounce(gentity_t* self, gentity_t* inflictor)
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

	self->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
}

void SabBeh_AnimateMassiveBounce(gentity_t* self, gentity_t* inflictor)//attacker anim
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

	self->client->ps.saberMove = PM_BrokenParryForParry(self->client->ps.saberMove);
	self->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
}

void SabBeh_AnimateMassiveSlowBounce(gentity_t* hitEnt, qboolean move)//defender anim
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

	G_SetAnim(hitEnt, &(hitEnt->client->pers.cmd), SETANIM_TORSO, useAnim, AFLAG_PACE, 0);

	if (PM_SaberInMassiveBounce(hitEnt->client->ps.torsoAnim))
	{
		hitEnt->client->ps.saberMove = LS_NONE;
		hitEnt->client->ps.saberBlocked = BLOCKED_NONE;
		hitEnt->client->ps.weaponTime = hitEnt->client->ps.torsoTimer;
		hitEnt->client->MassiveBounceAnimTime = hitEnt->client->ps.torsoTimer + level.time;
	}
}

qboolean SabBeh_RollBalance(gentity_t* self, sabmech_t* mechSelf, qboolean forceMishap)
{//JaceSolaris making it feel like EoC MP/OJP
 //if the attack is blocked -(Im the attacker)

	if (self->client->ps.saberAttackChainCount >= MISHAPLEVEL_FULL)
	{//hard mishap.
		mechSelf->doButterFingers = qtrue;
		self->client->ps.saberAttackChainCount = MISHAPLEVEL_LIGHT;
		return qtrue;
	}
	else if (self->client->ps.stats[STAT_DODGE] < DODGE_CRITICALLEVEL)
	{//heavy slow bounce
		mechSelf->doMassiveSlowBounce = qtrue;
		return qtrue;
	}
	else if (self->client->ps.saberAttackChainCount >= MISHAPLEVEL_THIRTEEN)
	{//heavy slow bounce
		mechSelf->doMassiveSlowBounce = qtrue;
		return qtrue;
	}
	else if (self->client->ps.saberAttackChainCount >= MISHAPLEVEL_TEN)
	{//heavy slow bounce
		mechSelf->doMassiveBounce = qtrue;
		return qtrue;
	}
	else if (self->client->ps.saberAttackChainCount >= MISHAPLEVEL_SEVEN)
	{//slow bounce
		mechSelf->doHeavySlowBounce = qtrue;
		return qtrue;
	}
	else if (forceMishap)
	{//perform a slow bounce even if we don't have enough mishap for it.
		mechSelf->doSlowBounce = qtrue;
		return qtrue;
	}
	else
	{
		mechSelf->doSlowBounce = qtrue;
		return qtrue;
	}

	return qfalse;
}

void SabBeh_AddBalance(gentity_t* self, sabmech_t* mechSelf, int amount, qboolean attack)
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
			mechSelf->doButterFingers = qtrue;
			break;
		case 1:
			mechSelf->doKnockdown = qtrue;
			break;
		};
		self->client->ps.saberAttackChainCount = MISHAPLEVEL_HEAVY;
	}
	else if (self->client->ps.fd.blockPoints < BLOCKPOINTS_WARNING)
	{//overflowing causes a full mishap.
		int randNum = Q_irand(0, 2);
		switch (randNum)
		{
		case 0:
			mechSelf->doButterFingers = qtrue;
			break;
		case 1:
			mechSelf->doKnockdown = qtrue;
			break;
		};
		self->client->ps.fd.blockPoints = BLOCKPOINTS_HALF;
	}
}

void SabBeh_AttackVsBlock(gentity_t* attacker, sabmech_t* mechAttacker, gentity_t* blocker, sabmech_t* mechBlocker, vec3_t hitLoc, qboolean hitSaberBlade, qboolean* attackerMishap, qboolean* blockerMishap)
{//if the attack is blocked -(Im the attacker)
	qboolean startSaberLock = qfalse;
	qboolean parried = G_ActiveParry(blocker, attacker, hitLoc);
	qboolean perfectparry = G_MBlocking(blocker, attacker, hitLoc);
	qboolean atkparry = G_InAttackParry(blocker);
	qboolean atkfake = (attacker->client->ps.userInt3 & (1 << FLAG_ATTACKFAKE)) ? qtrue : qfalse;

	if ((parried || perfectparry) && blocker->r.svFlags & SVF_BOT
		&& BOT_ATTACKPARRYRATE * botstates[blocker->s.number]->settings.skill > Q_irand(0, 999))
	{//bot performed an attack parry (by cheating a bit)
		atkparry = qtrue;
	}

	if ((parried || perfectparry) && blocker->s.eType == ET_NPC
		&& BOT_ATTACKPARRYRATE* g_npcspskill.integer > Q_irand(0, 999))
	{//bot performed an attack parry (by cheating a bit)
		atkparry = qtrue;
	}

	if (SaberAttacking(attacker))
	{
		G_SaberBounce(attacker, blocker, qfalse);
	}
	if (PM_SuperBreakWinAnim(attacker->client->ps.torsoAnim))
	{//attacker was attempting a superbreak and he hit someone who could block the move, rail him for screwing up.
		*attackerMishap = SabBeh_RollBalance(attacker, mechAttacker, qtrue);
		SabBeh_AddBalance(attacker, mechAttacker, 2, qtrue);
#ifdef _DEBUG
		mechAttacker->behaveMode = SABBEHAVE_ATTACKPARRIED;
#endif

		SabBeh_AddBalance(blocker, mechBlocker, -1, qfalse);
#ifdef _DEBUG
		mechBlocker->behaveMode = SABBEHAVE_BLOCK;
#endif
	}
	else if (atkfake)
	{//attacker faked before making this attack, treat like standard attack/attack
		if ((parried || perfectparry))
		{//defender parried the attack fake.
			*attackerMishap = SabBeh_RollBalance(attacker, mechAttacker, atkparry);
			SabBeh_AddBalance(attacker, mechAttacker, MPCOST_PARRIED_ATTACKFAKE, qtrue);
#ifdef _DEBUG
			mechAttacker->behaveMode = SABBEHAVE_ATTACK;
#endif
			if (blocker->client->ps.ManualBlockingFlags & (1 << MBF_BLOCKING))
			{
				attacker->client->ps.userInt3 |= (1 << FLAG_QUICKPARRY);
			}
			else
			{
				attacker->client->ps.userInt3 |= (1 << FLAG_PARRIED);
			}

			SabBeh_AddBalance(blocker, mechBlocker, MPCOST_PARRYING_ATTACKFAKE, qfalse);
#ifdef _DEBUG
			mechBlocker->behaveMode = SABBEHAVE_BLOCK;
#endif
			G_SaberBounce(attacker, blocker, qfalse);
		}
		else
		{//otherwise, the defender stands a good chance of having his defensives broken.	
			SabBeh_AddBalance(attacker, mechAttacker, -1, qtrue);

			if (attacker->client->ps.fd.saberAnimLevel == SS_DESANN)
			{
				SabBeh_AddBalance(blocker, mechBlocker, 2, qfalse);
			}

#ifdef _DEBUG
			mechAttacker->behaveMode = SABBEHAVE_ATTACK;
#endif
			if (WP_SabersCheckLock(attacker, blocker))
			{
				attacker->client->ps.userInt3 |= (1 << FLAG_LOCKWINNER);
				attacker->client->ps.saberBlocked = BLOCKED_NONE;
				blocker->client->ps.saberBlocked = BLOCKED_NONE;
				startSaberLock = qtrue;
			}

#ifdef _DEBUG
			mechBlocker->behaveMode = SABBEHAVE_BLOCKFAKED;
#endif
			G_SaberBounce(attacker, blocker, qfalse);
		}

	}
	else if (hitSaberBlade && BG_InSlowBounce(&blocker->client->ps)
		&& blocker->client->ps.userInt3 & (1 << FLAG_OLDSLOWBOUNCE)
		&& attacker->client->ps.fd.saberAnimLevel == SS_TAVION)
	{//blocker's saber was directly hit while in a slow bounce, disarm the blocker!
		mechBlocker->doButterFingers = qtrue;
		blocker->client->ps.saberAttackChainCount = MISHAPLEVEL_NONE;
#ifdef _DEBUG
		mechBlocker->behaveMode = SABBEHAVE_BLOCKFAKED;
#endif

		//set attacker
		SabBeh_AddBalance(attacker, mechAttacker, -3, qtrue);
#ifdef _DEBUG
		mechAttacker->behaveMode = SABBEHAVE_ATTACK;
#endif
		G_SaberBounce(attacker, blocker, qfalse);
	}
	else
	{//standard attack
	 //set blocker
#ifdef _DEBUG
		mechBlocker->behaveMode = SABBEHAVE_BLOCK;
#endif
		//set attacker
		if ((parried || perfectparry))
		{
			//parry values
			if (attacker->client->ps.saberMove == LS_A_LUNGE
				|| attacker->client->ps.saberMove == LS_SPINATTACK
				|| attacker->client->ps.saberMove == LS_SPINATTACK_DUAL
				|| attacker->client->ps.saberMove == LS_SPINATTACK_GRIEV)
			{//attacker's lunge was parried, force mishap.
				*attackerMishap = SabBeh_RollBalance(attacker, mechAttacker, qtrue);
			}
			else
			{
				*attackerMishap = SabBeh_RollBalance(attacker, mechAttacker, atkparry);
			}
			SabBeh_AddBalance(attacker, mechAttacker, MPCOST_PARRIED, qtrue);
#ifdef _DEBUG
			mechAttacker->behaveMode = SABBEHAVE_ATTACKPARRIED;
#endif		
			if (blocker->client->ps.ManualBlockingFlags & (1 << MBF_BLOCKING))
			{
				attacker->client->ps.userInt3 |= (1 << FLAG_QUICKPARRY);
			}
			else
			{
				attacker->client->ps.userInt3 |= (1 << FLAG_PARRIED);
			}

			SabBeh_AddBalance(blocker, mechBlocker, MPCOST_PARRYING, qfalse);
			G_SaberBounce(attacker, blocker, qfalse);

		}
		else
		{//blocked values
			SabBeh_AddBalance(attacker, mechAttacker, -1, qtrue);
			if (attacker->client->ps.fd.saberAnimLevel == SS_TAVION)
			{//aqua styles deals MP to players that don't parry it.
				SabBeh_AddBalance(blocker, mechBlocker, 2, qfalse);
			}
			else if (attacker->client->ps.fd.saberAnimLevel == SS_STRONG)
			{
				blocker->client->ps.fd.forcePower -= 2;
			}

#ifdef _DEBUG
			mechAttacker->behaveMode = SABBEHAVE_ATTACKBLOCKED;
#endif
			G_SaberBounce(attacker, blocker, qfalse);
		}
	}

	if (!OnSameTeam(attacker, blocker) || g_friendlySaber.integer)
	{//don't do parries or charge/regen DP unless we're in a situation where we can actually hurt the target.
		if (parried)
		{//parries don't cost any DP and they have a special animation
			if (blocker->client->ps.fd.blockPoints < BLOCKPOINTS_WARNING)
			{
				mechBlocker->doCrushTheParry = qtrue;
			}
			else if (blocker->client->ps.fd.blockPoints <= BLOCKPOINTS_FATIGUE)
			{
				mechBlocker->doMassiveSlowBounce = qtrue;
			}
			else if (blocker->client->ps.fd.blockPoints <= BLOCKPOINTS_HALF)
			{
				mechBlocker->dofatiguedParry = qtrue;
			}
			else if (blocker->client->ps.fd.blockPoints <= BLOCKPOINTS_FULL)
			{
				mechBlocker->doParry = qtrue;
			}
			else if (perfectparry)
			{
				WP_SaberBlockNonRandom(blocker, hitLoc, qfalse);
			}
			else
			{
				WP_SaberBlockNonRandom(blocker, hitLoc, qfalse);
			}
			G_SaberBounce(attacker, blocker, qfalse);
		}
		else if (!startSaberLock)
		{//normal saber blocks
			blocker->client->ps.saberLockFrame = 0; //break out of saberlocks.
			WP_SaberBlockNonRandom(blocker, hitLoc, qfalse);
			G_SaberBounce(attacker, blocker, qfalse);
		}
	}

	//costs
	G_DodgeDrain(blocker, attacker, WP_SaberBlockCost(blocker, attacker, hitLoc));
	BG_AddFatigue(&blocker->client->ps, FATIGUE_BLOCKED);
}

void SabBeh_AttackVsAttack(gentity_t* self, sabmech_t* mechSelf, gentity_t* otherOwner, sabmech_t* mechOther, qboolean* selfMishap, qboolean* otherMishap)
{//set the saber behavior for two attacking blades hitting each other
	qboolean atkfake = (self->client->ps.userInt3 & (1 << FLAG_ATTACKFAKE)) ? qtrue : qfalse;
	qboolean otherfake = (otherOwner->client->ps.userInt3 & (1 << FLAG_ATTACKFAKE)) ? qtrue : qfalse;

	if (atkfake && !otherfake)
	{//self is sololy faking
	 //set self
		SabBeh_AddBalance(self, mechSelf, 1, qtrue);
#ifdef _DEBUG
		mechSelf->behaveMode = SABBEHAVE_BLOCKFAKED;
#endif

		//set otherOwner
		if (WP_SabersCheckLock(self, otherOwner))
		{
			self->client->ps.userInt3 |= (1 << FLAG_LOCKWINNER);
			self->client->ps.saberBlocked = BLOCKED_NONE;
			otherOwner->client->ps.saberBlocked = BLOCKED_NONE;
		}
		SabBeh_AddBalance(otherOwner, mechOther, -1, qtrue);
#ifdef _DEBUG
		mechOther->behaveMode = SABBEHAVE_ATTACK;
#endif
	}
	else if (otherfake && !atkfake)
	{//only otherOwner is faking
	 //set self
		if (WP_SabersCheckLock(otherOwner, self))
		{
			self->client->ps.saberBlocked = BLOCKED_NONE;
			otherOwner->client->ps.userInt3 |= (1 << FLAG_LOCKWINNER);
			otherOwner->client->ps.saberBlocked = BLOCKED_NONE;
		}
		SabBeh_AddBalance(self, mechSelf, -1, qtrue);
#ifdef _DEBUG
		mechSelf->behaveMode = SABBEHAVE_ATTACK;
#endif

		//set otherOwner
		SabBeh_AddBalance(otherOwner, mechOther, 1, qtrue);
#ifdef _DEBUG
		mechOther->behaveMode = SABBEHAVE_BLOCKFAKED;
#endif
	}
	else
	{//either both are faking or neither is faking.  Either way, it's cancelled out
	 //set self
		SabBeh_AddBalance(self, mechSelf, 1, qtrue);
#ifdef _DEBUG
		mechSelf->behaveMode = SABBEHAVE_ATTACK;
#endif

		//set otherOwner
		SabBeh_AddBalance(otherOwner, mechOther, 1, qtrue);
#ifdef _DEBUG
		mechOther->behaveMode = SABBEHAVE_ATTACK;
#endif
		//bounce them off each other
		G_SaberBounce(self, otherOwner, qfalse);
		G_SaberBounce(otherOwner, self, qfalse);
	}
}