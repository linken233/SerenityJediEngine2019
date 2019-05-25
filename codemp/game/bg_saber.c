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



#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "w_saber.h"

#ifdef _GAME
#include "g_local.h"
extern stringID_table_t animTable[MAX_ANIMATIONS + 1];
extern stringID_table_t SaberMoveTable[];
#endif

extern qboolean BG_SabersOff(playerState_t* ps);
saberInfo_t* BG_MySaber(int clientNum, int saberNum);
extern qboolean PM_SaberInDamageMove(int move);
extern int PM_DualStanceForSingleStance(int anim);
extern int PM_StaffStanceForSingleStance(int anim);
extern int PM_MediumStanceForSingleStance(int anim);
void BG_AddFatigue(playerState_t* ps, int Fatigue);
extern qboolean PM_InCartwheel(int anim);
extern qboolean PM_SaberInDeflect(int move);
extern void PM_AddEventWithParm(int newEvent, int parm);
extern qboolean PM_CrouchingAnim(int anim);
extern qboolean PM_MeleeblockHoldAnim(int anim);
extern qboolean PM_MeleeblockAnim(int anim);
extern qboolean PM_WalkingOrRunningAnim(int anim);
extern qboolean PM_SaberInSpecial(int move);
saberMoveName_t PM_SaberLungeAttackMove(qboolean noSpecials);
extern qboolean ValidAnimFileIndex(int index);
extern qboolean PM_InOnGroundAnims(playerState_t* ps);
extern qboolean PM_LockedAnim(int anim);
extern qboolean PM_SaberInReturn(int move);
qboolean PM_WalkingAnim(int anim);
qboolean PM_SwimmingAnim(int anim);
extern saberMoveName_t PM_SaberBounceForAttack(int move);
qboolean PM_SuperBreakLoseAnim(int anim);
qboolean PM_SuperBreakWinAnim(int anim);
extern bgEntity_t* pm_entSelf;
extern saberMoveName_t PM_KnockawayForParry(int move);
extern saberMoveName_t PM_AbsorbtheParry(int move);
extern qboolean PM_SaberInbackblock(int move);
extern qboolean PM_BlockAnim(int anim);
extern qboolean PM_BlockHoldAnim(int anim);

int PM_irand_timesync(int val1, int val2)
{
	int i;

	i = (val1 - 1) + (Q_random(&pm->cmd.serverTime) * (val2 - val1)) + 1;
	if (i < val1)
	{
		i = val1;
	}
	if (i > val2)
	{
		i = val2;
	}

	return i;
}

void WP_ForcePowerDrain(playerState_t* ps, forcePowers_t forcePower, int overrideAmt)
{
	//take away the power
	int	drain = overrideAmt;

	if (!drain)
	{
		if (ps->fd.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_1 &&
			ps->fd.forcePowersActive & (1 << FP_SPEED))
		{
			drain = forcePowerNeededlevel1[ps->fd.forcePowerLevel[forcePower]][forcePower];
		}
		else if (ps->fd.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_2 &&
			ps->fd.forcePowersActive & (1 << FP_SPEED))
		{
			drain = forcePowerNeededlevel2[ps->fd.forcePowerLevel[forcePower]][forcePower];
		}
		else if (ps->fd.forcePowerLevel[FP_SPEED] == FORCE_LEVEL_3 &&
			ps->fd.forcePowersActive & (1 << FP_SPEED))
		{
			drain = forcePowerNeeded[ps->fd.forcePowerLevel[forcePower]][forcePower];
		}
		else
		{
			drain = forcePowerNeeded[ps->fd.forcePowerLevel[forcePower]][forcePower];
		}
	}

	if (!drain)
	{
		return;
	}

	ps->fd.forcePower -= drain;

	if (ps->fd.forcePower < 0)
	{
		ps->fd.forcePower = 0;
	}

	if (ps->fd.forcePower <= (ps->fd.forcePowerMax * FATIGUEDTHRESHHOLD))
	{//Pop the Fatigued flag
		ps->userInt3 |= (1 << FLAG_FATIGUED);
	}
}

void BG_ForcePowerKill(playerState_t * ps, forcePowers_t forcePower)
{
	if (ps->fd.forcePower < 5)
	{
		ps->fd.forcePower = 5;
	}
	if (ps->fd.forcePower <= (ps->fd.forcePowerMax * FATIGUEDTHRESHHOLD))
	{
		ps->userInt3 |= (1 << FLAG_FATIGUED);
	}
	if (pm->ps->fd.forcePowersActive & (1 << FP_SPEED))
	{
		pm->ps->fd.forcePowersActive &= ~(1 << FP_SPEED);
	}
}

qboolean BG_EnoughForcePowerForMove(int cost)
{
	if (pm->ps->fd.forcePower < cost)
	{
		PM_AddEvent(EV_NOAMMO);
		return qfalse;
	}

	return qtrue;
}

// Silly, but I'm replacing these macros so they are shorter!
#define AFLAG_IDLE	(SETANIM_FLAG_NORMAL)
#define AFLAG_ACTIVE (SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS)
#define AFLAG_WAIT (SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS)
#define AFLAG_FINISH (SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS)

//FIXME: add the alternate anims for each style?
saberMoveData_t	saberMoveData[LS_MOVE_MAX] = {//							NB:randomized
	// name			anim(do all styles?)startQ	endQ	setanimflag		blend,	blocking	chain_idle		chain_attack	trailLen
	{ "None",		BOTH_STAND1,		Q_R,	Q_R,	AFLAG_IDLE,		350,	BLK_NO,		LS_NONE,		LS_NONE,		0 },	// LS_NONE		= 0,

																																	// General movements with saber
	{ "Ready",		BOTH_STAND2,		Q_R,	Q_R,	AFLAG_IDLE,		350,	BLK_WIDE,	LS_READY,		LS_S_R2L,		0 },	// LS_READY,
	{ "Draw",		BOTH_STAND1TO2,		Q_R,	Q_R,	AFLAG_FINISH,	350,	BLK_NO,		LS_READY,		LS_S_R2L,		0 },	// LS_DRAW,
	{ "Draw2",		BOTH_SABER_IGNITION,Q_R,	Q_R,	AFLAG_FINISH,	350,	BLK_NO,		LS_READY,		LS_S_R2L,		0 },	// LS_DRAW2,
	{ "Draw3",		BOTH_GRIEVOUS_SABERON,Q_R,	Q_R,	AFLAG_FINISH,	350,	BLK_NO,		LS_READY,		LS_S_R2L,		0 },	// LS_DRAW3,
	{ "Putaway",	BOTH_STAND2TO1,		Q_R,	Q_R,	AFLAG_FINISH,	350,	BLK_NO,		LS_READY,		LS_S_R2L,		0 },	// LS_PUTAWAY,

	// Attacks
	//UL2LR
	{ "TL2BR Att",	BOTH_A1_TL_BR,		Q_TL,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_TL2BR,		LS_R_TL2BR,		200 },	// LS_A_TL2BR
	//SLASH LEFT
	{ "L2R Att",	BOTH_A1__L__R,		Q_L,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_L2R,		LS_R_L2R,		200 },	// LS_A_L2R
	//LL2UR
	{ "BL2TR Att",	BOTH_A1_BL_TR,		Q_BL,	Q_TR,	AFLAG_ACTIVE,	50,		BLK_TIGHT,	LS_R_BL2TR,		LS_R_BL2TR,		200 },	// LS_A_BL2TR
	//LR2UL
	{ "BR2TL Att",	BOTH_A1_BR_TL,		Q_BR,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_BR2TL,		LS_R_BR2TL,		200 },	// LS_A_BR2TL
	//SLASH RIGHT
	{ "R2L Att",	BOTH_A1__R__L,		Q_R,	Q_L,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_R2L,		LS_R_R2L,		200 },// LS_A_R2L
	//UR2LL
	{ "TR2BL Att",	BOTH_A1_TR_BL,		Q_TR,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_TR2BL,		LS_R_TR2BL,		200 },	// LS_A_TR2BL
	//SLASH DOWN
	{ "T2B Att",	BOTH_A1_T__B_,		Q_T,	Q_B,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_R_T2B,		LS_R_T2B,		200 },	// LS_A_T2B
	//special attacks
	{ "Back Stab",	BOTH_A2_STABBACK1,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_A_BACKSTAB
	{ "Back Stab_b",BOTH_A2_STABBACK1B,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_A_BACKSTAB_B
	{ "Back Att",	BOTH_ATTACK_BACK,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_A_BACK
	{ "CR Back Att",BOTH_CROUCHATTACKBACK1,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_A_BACK_CR
	{ "RollStab",	BOTH_ROLL_STAB,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_ROLL_STAB
	{ "Lunge Att",	BOTH_LUNGE2_B__T_,	Q_B,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_A_LUNGE
	{ "Jump Att",	BOTH_FORCELEAP2_T__B_,Q_T,	Q_B,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_A_JUMP_T__B_
	{ "Palp Att",	BOTH_FORCELEAP_PALP,Q_T,	Q_B,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_A_JUMP_PALP_
	{ "Flip Stab",	BOTH_JUMPFLIPSTABDOWN,Q_R,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1_T___R,	200 },	// LS_A_FLIP_STAB
	{ "Flip Slash",	BOTH_JUMPFLIPSLASHDOWN1,Q_L,Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1__R_T_,	200 },	// LS_A_FLIP_SLASH
	{ "DualJump Atk",BOTH_JUMPATTACK6,	Q_R,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1_BL_TR,	200 },	// LS_JUMPATTACK_DUAL
	{ "DualJump Atkgrie",BOTH_GRIEVOUS_LUNGE, Q_R, Q_BL, AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1_BL_TR,	200 },	// LS_GRIEVOUS_LUNGE

	{ "DualJumpAtkL_A",BOTH_ARIAL_LEFT,	Q_R,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_A_TL2BR,		200 },	// LS_JUMPATTACK_ARIAL_LEFT
	{ "DualJumpAtkR_A",BOTH_ARIAL_RIGHT,Q_R,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_A_TR2BL,		200 },	// LS_JUMPATTACK_ARIAL_RIGHT

	{ "DualJumpAtkL_A",BOTH_CARTWHEEL_LEFT,	Q_R,Q_TL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1_TL_BR,	200 },	// LS_JUMPATTACK_CART_LEFT
	{ "DualJumpAtkR_A",BOTH_CARTWHEEL_RIGHT,Q_R,Q_TR,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1_TR_BL,	200 },	// LS_JUMPATTACK_CART_RIGHT

	{ "DualJumpAtkLStaff", BOTH_BUTTERFLY_FL1,Q_R,Q_L,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1__L__R,	200 },	// LS_JUMPATTACK_STAFF_LEFT
	{ "DualJumpAtkRStaff", BOTH_BUTTERFLY_FR1,Q_R,Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1__R__L,	200 },	// LS_JUMPATTACK_STAFF_RIGHT

	{ "ButterflyLeft", BOTH_BUTTERFLY_LEFT,Q_R,Q_L,		AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1__L__R,	200 },	// LS_BUTTERFLY_LEFT
	{ "ButterflyRight", BOTH_BUTTERFLY_RIGHT,Q_R,Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1__R__L,	200 },	// LS_BUTTERFLY_RIGHT

	{ "BkFlip Atk",	BOTH_JUMPATTACK7,	Q_B,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_T1_T___R,	200 },	// LS_A_BACKFLIP_ATK
	{ "DualSpinAtk",BOTH_SPINATTACK6,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_SPINATTACK_DUAL
	{ "DualSpinAtkgrie",BOTH_SPINATTACKGRIEVOUS,Q_R,	Q_R,AFLAG_ACTIVE,100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_SPINATTACK_GRIEV
	{ "StfSpinAtk",	BOTH_SPINATTACK7,	Q_L,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_SPINATTACK
	{ "LngLeapAtk",	BOTH_FORCELONGLEAP_ATTACK,Q_R,Q_L,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_LEAP_ATTACK
	{ "SwoopAtkR",	BOTH_VS_ATR_S,		Q_R,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_SWOOP_ATTACK_RIGHT
	{ "SwoopAtkL",	BOTH_VS_ATL_S,		Q_L,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_SWOOP_ATTACK_LEFT
	{ "TauntaunAtkR",BOTH_VT_ATR_S,		Q_R,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_TAUNTAUN_ATTACK_RIGHT
	{ "TauntaunAtkL",BOTH_VT_ATL_S,		Q_L,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_TAUNTAUN_ATTACK_LEFT
	{ "StfKickFwd",	BOTH_A7_KICK_F,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_KICK_F
	{ "StfKickFwd2",BOTH_A7_KICK_F2,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_KICK_F2
	{ "StfKickBack",BOTH_A7_KICK_B,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_KICK_B
	{ "StfKickBack2",BOTH_A7_KICK_B2,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_KICK_B2
	{ "StfKickBack3",BOTH_A7_KICK_B3,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_KICK_B3
	{ "StfKickRight",BOTH_A7_KICK_R,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_KICK_R
	{ "StfslapRight",BOTH_A7_SLAP_R,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_SLAP_R
	{ "StfKickLeft",BOTH_A7_KICK_L,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_KICK_L
	{ "StfslapLeft",BOTH_A7_SLAP_L,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_SLAP_L
	{ "StfKickSpin",BOTH_A7_KICK_S,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,		LS_READY,		LS_S_R2L,		200 },	// LS_KICK_S
	{ "StfKickBkFwd",BOTH_A7_KICK_BF,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,		LS_READY,		LS_S_R2L,		200 },	// LS_KICK_BF
	{ "StfKickSplit",BOTH_A7_KICK_RL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,		LS_READY,		LS_S_R2L,		200 },	// LS_KICK_RL
	{ "StfKickFwdAir",BOTH_A7_KICK_F_AIR,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_KICK_F_AIR
	{ "StfKickFwdAir",BOTH_A7_KICK_F_AIR2,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_KICK_F_AIR2
	{ "StfKickBackAir",BOTH_A7_KICK_B_AIR,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_KICK_B_AIR
	{ "StfKickRightAir",BOTH_A7_KICK_R_AIR,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_KICK_R_AIR
	{ "StfKickLeftAir",BOTH_A7_KICK_L_AIR,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_KICK_L_AIR
	{ "StabDown",	BOTH_STABDOWN,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_STABDOWN
	{ "StabDownbhd",BOTH_STABDOWN_BACKHAND,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_STABDOWN_BACKHAND
	{ "StabDownStf",BOTH_STABDOWN_STAFF,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_STABDOWN_STAFF
	{ "StabDownDual",BOTH_STABDOWN_DUAL,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_S_R2L,		200 },	// LS_STABDOWN_DUAL
	{ "dualspinprot",BOTH_A6_SABERPROTECT,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		500 },	// LS_DUAL_SPIN_PROTECT
	{ "dualspinprotgrie",BOTH_GRIEVOUS_PROTECT,Q_R,	Q_R,AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		500 },	// LS_DUAL_SPIN_PROTECT_GRIE
	{ "StfSoulCal",	BOTH_A7_SOULCAL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		500 },	// LS_STAFF_SOULCAL
	{ "specialyoda",BOTH_YODA_SPECIAL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		2000 },	// LS_YODA_SPECIAL
	{ "specialfast",BOTH_A1_SPECIAL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		2000 },	// LS_A1_SPECIAL
	{ "specialmed",	BOTH_A2_SPECIAL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		2000 },	// LS_A2_SPECIAL
	{ "specialstr",	BOTH_A3_SPECIAL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		2000 },	// LS_A3_SPECIAL
	{ "specialtav",	BOTH_A4_SPECIAL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		2000 },	// LS_A4_SPECIAL
	{ "specialdes",	BOTH_A5_SPECIAL,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		2000 },	// LS_A5_SPECIAL
	{ "specialgri",	BOTH_GRIEVOUS_SPIN,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		2000 },	// LS_GRIEVOUS_SPECIAL
	{ "upsidedwnatk",BOTH_FLIP_ATTACK7,	Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_UPSIDE_DOWN_ATTACK
	{ "pullatkstab",BOTH_PULL_IMPALE_STAB,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_PULL_ATTACK_STAB
	{ "pullatkswing",BOTH_PULL_IMPALE_SWING,Q_R,Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_PULL_ATTACK_SWING
	{ "AloraSpinAtk",BOTH_ALORA_SPIN_SLASH,Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_SPINATTACK_ALORA
	{ "Dual FB Atk",BOTH_A6_FB,			Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_DUAL_FB
	{ "Dual LR Atk",BOTH_A6_LR,			Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_DUAL_LR
	{ "StfHiltBash",BOTH_A7_HILT,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_HILT_BASH

	//starts
	{ "TL2BR St",	BOTH_S1_S1_TL,		Q_R,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_TL2BR,		LS_A_TL2BR,		200 },	// LS_S_TL2BR
	{ "L2R St",		BOTH_S1_S1__L,		Q_R,	Q_L,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_L2R,		LS_A_L2R,		200 },	// LS_S_L2R
	{ "BL2TR St",	BOTH_S1_S1_BL,		Q_R,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_BL2TR,		LS_A_BL2TR,		200 },	// LS_S_BL2TR
	{ "BR2TL St",	BOTH_S1_S1_BR,		Q_R,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_BR2TL,		LS_A_BR2TL,		200 },	// LS_S_BR2TL
	{ "R2L St",		BOTH_S1_S1__R,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_R2L,		LS_A_R2L,		200 },	// LS_S_R2L
	{ "TR2BL St",	BOTH_S1_S1_TR,		Q_R,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_TR2BL,		LS_A_TR2BL,		200 },	// LS_S_TR2BL
	{ "T2B St",		BOTH_S1_S1_T_,		Q_R,	Q_T,	AFLAG_ACTIVE,	100,	BLK_TIGHT,	LS_A_T2B,		LS_A_T2B,		200 },	// LS_S_T2B

	//returns
	{ "TL2BR Ret",	BOTH_R1_BR_S1,		Q_BR,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_R_TL2BR
	{ "L2R Ret",	BOTH_R1__R_S1,		Q_R,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_R_L2R
	{ "BL2TR Ret",	BOTH_R1_TR_S1,		Q_TR,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_R_BL2TR
	{ "BR2TL Ret",	BOTH_R1_TL_S1,		Q_TL,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_R_BR2TL
	{ "R2L Ret",	BOTH_R1__L_S1,		Q_L,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_R_R2L
	{ "TR2BL Ret",	BOTH_R1_BL_S1,		Q_BL,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_R_TR2BL
	{ "T2B Ret",	BOTH_R1_B__S1,		Q_B,	Q_R,	AFLAG_FINISH,	100,	BLK_TIGHT,	LS_READY,		LS_READY,		200 },	// LS_R_T2B

	//Transitions
	{ "BR2R Trans",	BOTH_T1_BR__R,		Q_BR,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150 },	//# Fast arc bottom right to right
	{ "BR2TR Trans",BOTH_T1_BR_TR,		Q_BR,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150 },	//# Fast arc bottom right to top right		(use: BOTH_T1_TR_BR)
	{ "BR2T Trans",	BOTH_T1_BR_T_,		Q_BR,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150 },	//# Fast arc bottom right to top			(use: BOTH_T1_T__BR)
	{ "BR2TL Trans",BOTH_T1_BR_TL,		Q_BR,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150 },	//# Fast weak spin bottom right to top left
	{ "BR2L Trans",	BOTH_T1_BR__L,		Q_BR,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150 },	//# Fast weak spin bottom right to left
	{ "BR2BL Trans",BOTH_T1_BR_BL,		Q_BR,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150 },	//# Fast weak spin bottom right to bottom left
	{ "R2BR Trans",	BOTH_T1__R_BR,		Q_R,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150 },	//# Fast arc right to bottom right			(use: BOTH_T1_BR__R)
	{ "R2TR Trans",	BOTH_T1__R_TR,		Q_R,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150 },	//# Fast arc right to top right
	{ "R2T Trans",	BOTH_T1__R_T_,		Q_R,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150 },	//# Fast ar right to top				(use: BOTH_T1_T___R)
	{ "R2TL Trans",	BOTH_T1__R_TL,		Q_R,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150 },	//# Fast arc right to top left
	{ "R2L Trans",	BOTH_T1__R__L,		Q_R,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150 },	//# Fast weak spin right to left
	{ "R2BL Trans",	BOTH_T1__R_BL,		Q_R,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150 },	//# Fast weak spin right to bottom left
	{ "TR2BR Trans",BOTH_T1_TR_BR,		Q_TR,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150 },	//# Fast arc top right to bottom right
	{ "TR2R Trans",	BOTH_T1_TR__R,		Q_TR,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150 },	//# Fast arc top right to right			(use: BOTH_T1__R_TR)
	{ "TR2T Trans",	BOTH_T1_TR_T_,		Q_TR,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150 },	//# Fast arc top right to top				(use: BOTH_T1_T__TR)
	{ "TR2TL Trans",BOTH_T1_TR_TL,		Q_TR,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150 },	//# Fast arc top right to top left
	{ "TR2L Trans",	BOTH_T1_TR__L,		Q_TR,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150 },	//# Fast arc top right to left
	{ "TR2BL Trans",BOTH_T1_TR_BL,		Q_TR,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150 },	//# Fast weak spin top right to bottom left
	{ "T2BR Trans",	BOTH_T1_T__BR,		Q_T,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150 },	//# Fast arc top to bottom right
	{ "T2R Trans",	BOTH_T1_T___R,		Q_T,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150 },	//# Fast arc top to right
	{ "T2TR Trans",	BOTH_T1_T__TR,		Q_T,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150 },	//# Fast arc top to top right
	{ "T2TL Trans",	BOTH_T1_T__TL,		Q_T,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150 },	//# Fast arc top to top left
	{ "T2L Trans",	BOTH_T1_T___L,		Q_T,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150 },	//# Fast arc top to left
	{ "T2BL Trans",	BOTH_T1_T__BL,		Q_T,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150 },	//# Fast arc top to bottom left
	{ "TL2BR Trans",BOTH_T1_TL_BR,		Q_TL,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150 },	//# Fast weak spin top left to bottom right
	{ "TL2R Trans",	BOTH_T1_TL__R,		Q_TL,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150 },	//# Fast arc top left to right			(use: BOTH_T1__R_TL)
	{ "TL2TR Trans",BOTH_T1_TL_TR,		Q_TL,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150 },	//# Fast arc top left to top right			(use: BOTH_T1_TR_TL)
	{ "TL2T Trans",	BOTH_T1_TL_T_,		Q_TL,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150 },	//# Fast arc top left to top				(use: BOTH_T1_T__TL)
	{ "TL2L Trans",	BOTH_T1_TL__L,		Q_TL,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150 },	//# Fast arc top left to left				(use: BOTH_T1__L_TL)
	{ "TL2BL Trans",BOTH_T1_TL_BL,		Q_TL,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150 },	//# Fast arc top left to bottom left
	{ "L2BR Trans",	BOTH_T1__L_BR,		Q_L,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150 },	//# Fast weak spin left to bottom right
	{ "L2R Trans",	BOTH_T1__L__R,		Q_L,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150 },	//# Fast weak spin left to right
	{ "L2TR Trans",	BOTH_T1__L_TR,		Q_L,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150 },	//# Fast arc left to top right			(use: BOTH_T1_TR__L)
	{ "L2T Trans",	BOTH_T1__L_T_,		Q_L,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150 },	//# Fast arc left to top				(use: BOTH_T1_T___L)
	{ "L2TL Trans",	BOTH_T1__L_TL,		Q_L,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150 },	//# Fast arc left to top left
	{ "L2BL Trans",	BOTH_T1__L_BL,		Q_L,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_A_BL2TR,		150 },	//# Fast arc left to bottom left			(use: BOTH_T1_BL__L)
	{ "BL2BR Trans",BOTH_T1_BL_BR,		Q_BL,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_A_BR2TL,		150 },	//# Fast weak spin bottom left to bottom right
	{ "BL2R Trans",	BOTH_T1_BL__R,		Q_BL,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_A_R2L,		150 },	//# Fast weak spin bottom left to right
	{ "BL2TR Trans",BOTH_T1_BL_TR,		Q_BL,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_TR2BL,		150 },	//# Fast weak spin bottom left to top right
	{ "BL2T Trans",	BOTH_T1_BL_T_,		Q_BL,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_A_T2B,		150 },	//# Fast arc bottom left to top			(use: BOTH_T1_T__BL)
	{ "BL2TL Trans",BOTH_T1_BL_TL,		Q_BL,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_A_TL2BR,		150 },	//# Fast arc bottom left to top left		(use: BOTH_T1_TL_BL)
	{ "BL2L Trans",	BOTH_T1_BL__L,		Q_BL,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_A_L2R,		150 },	//# Fast arc bottom left to left

	//Bounces
	{ "Bounce BR",	BOTH_B1_BR___,		Q_BR,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_T1_BR_TR,	150 },
	{ "Bounce R",	BOTH_B1__R___,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_T1__R__L,	150 },
	{ "Bounce TR",	BOTH_B1_TR___,		Q_TR,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_T1_TR_TL,	150 },
	{ "Bounce T",	BOTH_B1_T____,		Q_T,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_T1_T__BL,	150 },
	{ "Bounce TL",	BOTH_B1_TL___,		Q_TL,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_T1_TL_TR,	150 },
	{ "Bounce L",	BOTH_B1__L___,		Q_L,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_T1__L__R,	150 },
	{ "Bounce BL",	BOTH_B1_BL___,		Q_BL,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_T1_BL_TR,	150 },

	//Deflected attacks (like bounces, but slide off enemy saber, not straight back)
	{ "Deflect BR",	BOTH_D1_BR___,		Q_BR,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TL2BR,		LS_T1_BR_TR,	150 },
	{ "Deflect R",	BOTH_D1__R___,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_L2R,		LS_T1__R__L,	150 },
	{ "Deflect TR",	BOTH_D1_TR___,		Q_TR,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_T1_TR_TL,	150 },
	{ "Deflect T",	BOTH_B1_T____,		Q_T,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_T1_T__BL,	150 },
	{ "Deflect TL",	BOTH_D1_TL___,		Q_TL,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BR2TL,		LS_T1_TL_TR,	150 },
	{ "Deflect L",	BOTH_D1__L___,		Q_L,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_R2L,		LS_T1__L__R,	150 },
	{ "Deflect BL",	BOTH_D1_BL___,		Q_BL,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_TR2BL,		LS_T1_BL_TR,	150 },
	{ "Deflect B",	BOTH_D1_B____,		Q_B,	Q_B,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_R_BL2TR,		LS_T1_T__BL,	150 },

	//Reflected attacks
	{ "Reflected BR",BOTH_V1_BR_S1,		Q_BR,	Q_BR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150 },//	LS_V1_BR
	{ "Reflected R", BOTH_V1__R_S1,		Q_R,	Q_R,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150 },//	LS_V1__R
	{ "Reflected TR",BOTH_V1_TR_S1,		Q_TR,	Q_TR,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150 },//	LS_V1_TR
	{ "Reflected T", BOTH_V1_T__S1,		Q_T,	Q_T,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150 },//	LS_V1_T_
	{ "Reflected TL",BOTH_V1_TL_S1,		Q_TL,	Q_TL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150 },//	LS_V1_TL
	{ "Reflected L", BOTH_V1__L_S1,		Q_L,	Q_L,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150 },//	LS_V1__L
	{ "Reflected BL",BOTH_V1_BL_S1,		Q_BL,	Q_BL,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150 },//	LS_V1_BL
	{ "Reflected B", BOTH_V1_B__S1,		Q_B,	Q_B,	AFLAG_ACTIVE,	100,	BLK_NO,	LS_READY,		LS_READY,	150 },//	LS_V1_B_

	// Broken parries
	{ "BParry Top",	BOTH_H1_S1_T_,		Q_T,	Q_B,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150 },	// LS_PARRY_UP,
	{ "BParry UR",	BOTH_H1_S1_TR,		Q_TR,	Q_BL,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150 },	// LS_PARRY_UR,
	{ "BParry UL",	BOTH_H1_S1_TL,		Q_TL,	Q_BR,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150 },	// LS_PARRY_UL,
	{ "BParry LR",	BOTH_H1_S1_BR,		Q_BL,	Q_TR,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150 },	// LS_PARRY_LR,
	{ "BParry Bot",	BOTH_H1_S1_B_,		Q_B,	Q_T,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150 },	// LS_PARRY_LR
	{ "BParry LL",	BOTH_H1_S1_BL,		Q_BR,	Q_TL,	AFLAG_ACTIVE,	50,		BLK_NO,	LS_READY,		LS_READY,		150 },	// LS_PARRY_LL

	// Knockaways
	{ "Knock Top",	BOTH_K1_S1_T_,		Q_R,	Q_T,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_T1_T__BR,		150 },	// LS_PARRY_UP,
	{ "Knock UR",	BOTH_K1_S1_TR,		Q_R,	Q_TR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_T1_TR__R,		150 },	// LS_PARRY_UR,
	{ "Knock UL",	BOTH_K1_S1_TL,		Q_R,	Q_TL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BR2TL,		LS_T1_TL__L,		150 },	// LS_PARRY_UL,
	{ "Knock LR",	BOTH_K1_S1_BR,		Q_R,	Q_BL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TL2BR,		LS_T1_BL_TL,		150 },	// LS_PARRY_LR,
	{ "Knock LL",	BOTH_K1_S1_BL,		Q_R,	Q_BR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TR2BL,		LS_T1_BR_TR,		150 },	// LS_PARRY_LL

	// Parry
	{ "Parry Top",	BOTH_P1_S1_T_,		Q_R,	Q_T,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_A_T2B,		150 },	// LS_PARRY_UP,
	{ "Parry Front",BOTH_P1_S1_T1_,		Q_R,	Q_T,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_A_T2B,		150 },	// LS_PARRY_FRONT,
	{ "Parry UR",	BOTH_P1_S1_TR,		Q_R,	Q_TL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_A_TR2BL,		150 },	// LS_PARRY_UR,
	{ "Parry UL",	BOTH_P1_S1_TL,		Q_R,	Q_TR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BR2TL,		LS_A_TL2BR,		150 },	// LS_PARRY_UL,
	{ "Parry LR",	BOTH_P1_S1_BR,		Q_R,	Q_BR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TL2BR,		LS_A_BR2TL,		150 },	// LS_PARRY_LR,
	{ "Parry LL",	BOTH_P1_S1_BL,		Q_R,	Q_BL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TR2BL,		LS_A_BL2TR,		150 },	// LS_PARRY_LL

	// Reflecting a missile
	{ "Reflect Top",BOTH_P1_S1_T1_,		Q_R,	Q_T,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_A_T2B,		300 },	// LS_REFLECT_UP,
	{ "Reflect Front",BOTH_P1_S1_T1_,	Q_R,	Q_T,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_A_T2B,		300 },	// LS_REFLECT_FRONT,
	{ "Reflect UR",	BOTH_P1_S1_TR,		Q_R,	Q_TR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BR2TL,		LS_A_TL2BR,		300 },	// LS_REFLECT_UR,
	{ "Reflect UL",	BOTH_P1_S1_TL,		Q_R,	Q_TL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_BL2TR,		LS_A_TR2BL,		300 },	// LS_REFLECT_UL,
	{ "Reflect LR",	BOTH_P1_S1_BR,		Q_R,	Q_BL,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TR2BL,		LS_A_BL2TR,		300 },	// LS_REFLECT_LR
	{ "Reflect LL",	BOTH_P1_S1_BL,		Q_R,	Q_BR,	AFLAG_ACTIVE,	50,		BLK_WIDE,	LS_R_TL2BR,		LS_A_BR2TL,		300 },	// LS_REFLECT_LL,

	{ "Reflect Attack_L",	BOTH_BLOCKATTACK_LEFT,		Q_R, Q_TR, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BR2TL,  LS_A_TL2BR,     150 },	// LS_REFLECT_ATTACK_LEFT
	{ "Reflect Attack_R",	BOTH_BLOCKATTACK_RIGHT,		Q_R, Q_TL, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR,  LS_A_TR2BL,     150 },	// LS_REFLECT_ATTACK_RIGHT,
	{ "Reflect Attack_F",	BOTH_P7_S7_T_,		        Q_R, Q_TL, AFLAG_ACTIVE, 50, BLK_WIDE, LS_R_BL2TR,  LS_A_TR2BL,     150 },	// LS_REFLECT_ATTACK_FRONT,
};

saberMoveName_t transitionMove[Q_NUM_QUADS][Q_NUM_QUADS] =
{
	{
		LS_NONE,	//Can't transition to same pos!
		LS_T1_BR__R,//40
		LS_T1_BR_TR,
		LS_T1_BR_T_,
		LS_T1_BR_TL,
		LS_T1_BR__L,
		LS_T1_BR_BL,
		LS_NONE	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1__R_BR,//46
		LS_NONE,	//Can't transition to same pos!
		LS_T1__R_TR,
		LS_T1__R_T_,
		LS_T1__R_TL,
		LS_T1__R__L,
		LS_T1__R_BL,
		LS_NONE	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_TR_BR,//52
		LS_T1_TR__R,
		LS_NONE,	//Can't transition to same pos!
		LS_T1_TR_T_,
		LS_T1_TR_TL,
		LS_T1_TR__L,
		LS_T1_TR_BL,
		LS_NONE	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_T__BR,//58
		LS_T1_T___R,
		LS_T1_T__TR,
		LS_NONE,	//Can't transition to same pos!
		LS_T1_T__TL,
		LS_T1_T___L,
		LS_T1_T__BL,
		LS_NONE	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_TL_BR,//64
		LS_T1_TL__R,
		LS_T1_TL_TR,
		LS_T1_TL_T_,
		LS_NONE,	//Can't transition to same pos!
		LS_T1_TL__L,
		LS_T1_TL_BL,
		LS_NONE 	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1__L_BR,//70
		LS_T1__L__R,
		LS_T1__L_TR,
		LS_T1__L_T_,
		LS_T1__L_TL,
		LS_NONE,	//Can't transition to same pos!
		LS_T1__L_BL,
		LS_NONE	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_BL_BR,//76
		LS_T1_BL__R,
		LS_T1_BL_TR,
		LS_T1_BL_T_,
		LS_T1_BL_TL,
		LS_T1_BL__L,
		LS_NONE,	//Can't transition to same pos!
		LS_NONE	//No transitions to bottom, and no anims start there, so shouldn't need any
	},
	{
		LS_T1_BL_BR,//NOTE: there are no transitions from bottom, so re-use the bottom right transitions
		LS_T1_BR__R,
		LS_T1_BR_TR,
		LS_T1_BR_T_,
		LS_T1_BR_TL,
		LS_T1_BR__L,
		LS_T1_BR_BL,
		LS_NONE		//No transitions to bottom, and no anims start there, so shouldn't need any
	}
};

saberMoveName_t PM_NPCSaberAttackFromQuad(int quad)
{
	saberMoveName_t autoMove = LS_NONE;

	if (autoMove != LS_NONE && PM_SaberInSpecial(autoMove))
	{//if have opportunity to do a special attack, do one
		return autoMove;
	}
	else
	{//pick another one
		saberMoveName_t newmove = LS_NONE;
		switch (quad)
		{
		case Q_T://blocked top
			if (Q_irand(0, 1))
			{
				newmove = LS_A_T2B;
			}
			else
			{
				newmove = LS_A_TR2BL;
			}
			break;
		case Q_TR:
			if (!Q_irand(0, 2))
			{
				newmove = LS_A_R2L;
			}
			else if (!Q_irand(0, 1))
			{
				newmove = LS_A_TR2BL;
			}
			else
			{
				newmove = LS_T1_TR_BR;
			}
			break;
		case Q_TL:
			if (!Q_irand(0, 2))
			{
				newmove = LS_A_L2R;
			}
			else if (!Q_irand(0, 1))
			{
				newmove = LS_A_TL2BR;
			}
			else
			{
				newmove = LS_T1_TL_BL;
			}
			break;
		case Q_BR:
			if (!Q_irand(0, 2))
			{
				newmove = LS_A_BR2TL;
			}
			else if (!Q_irand(0, 1))
			{
				newmove = LS_T1_BR_TR;
			}
			else
			{
				newmove = LS_A_R2L;
			}
			break;
		case Q_BL:
			if (!Q_irand(0, 2))
			{
				newmove = LS_A_BL2TR;
			}
			else if (!Q_irand(0, 1))
			{
				newmove = LS_T1_BL_TL;
			}
			else
			{
				newmove = LS_A_L2R;
			}
			break;
		case Q_L:
			if (!Q_irand(0, 2))
			{
				newmove = LS_A_L2R;
			}
			else if (!Q_irand(0, 1))
			{
				newmove = LS_T1__L_T_;
			}
			else
			{
				newmove = LS_A_R2L;
			}
			break;
		case Q_R:
			if (!Q_irand(0, 2))
			{
				newmove = LS_A_R2L;
			}
			else if (!Q_irand(0, 1))
			{
				newmove = LS_T1__R_T_;
			}
			else
			{
				newmove = LS_A_L2R;
			}
			break;
		case Q_B:
			newmove = PM_SaberLungeAttackMove(qtrue);
			break;
		default:
			break;
		}
		return newmove;
	}
}

saberMoveName_t PM_AttackMoveForQuad(int quad)
{
	switch (quad)
	{
	case Q_B:
	case Q_BR:
		return LS_A_BR2TL;
		break;
	case Q_R:
		return LS_A_R2L;
		break;
	case Q_TR:
		return LS_A_TR2BL;
		break;
	case Q_T:
		return LS_A_T2B;
		break;
	case Q_TL:
		return LS_A_TL2BR;
		break;
	case Q_L:
		return LS_A_L2R;
		break;
	case Q_BL:
		return LS_A_BL2TR;
		break;
	}
	return LS_NONE;
}

qboolean PM_SaberKataDone(int curmove, int newmove);
int PM_ReturnforQuad(int quad);
saberMoveName_t PM_SaberAnimTransitionMove(saberMoveName_t curmove, saberMoveName_t newmove)
{
	int retmove = newmove;

	if (curmove == LS_READY)
	{//just standing there
		switch (newmove)
		{
		case LS_A_TL2BR:
		case LS_A_L2R:
		case LS_A_BL2TR:
		case LS_A_BR2TL:
		case LS_A_R2L:
		case LS_A_TR2BL:
		case LS_A_T2B:
			//transition is the start
			retmove = LS_S_TL2BR + (newmove - LS_A_TL2BR);
			break;
		}
	}
	else
	{
		switch (newmove)
		{
			//transitioning to ready pose
		case LS_READY:
			switch (curmove)
			{
				//transitioning from an attack
			case LS_A_TL2BR:
			case LS_A_L2R:
			case LS_A_BL2TR:
			case LS_A_BR2TL:
			case LS_A_R2L:
			case LS_A_TR2BL:
			case LS_A_T2B:
				//transition is the return
				retmove = LS_R_TL2BR + (newmove - LS_A_TL2BR);
				break;
				//bounces
			case LS_B1_BR:
			case LS_B1__R:
			case LS_B1_TR:
			case LS_B1_T_:
			case LS_B1_TL:
			case LS_B1__L:
			case LS_B1_BL:
				//transitioning from a parry/reflection/knockaway/broken parry
			case LS_PARRY_UP:
			case LS_PARRY_FRONT:
			case LS_PARRY_UR:
			case LS_PARRY_UL:
			case LS_PARRY_LR:
			case LS_PARRY_LL:
			case LS_REFLECT_UP:
			case LS_REFLECT_FRONT:
			case LS_REFLECT_UR:
			case LS_REFLECT_UL:
			case LS_REFLECT_LR:
			case LS_REFLECT_LL:
			case LS_REFLECT_ATTACK_LEFT:
			case LS_REFLECT_ATTACK_RIGHT:
			case LS_REFLECT_ATTACK_FRONT:
			case LS_K1_T_:
			case LS_K1_TR:
			case LS_K1_TL:
			case LS_K1_BR:
			case LS_K1_BL:
			case LS_V1_BR:
			case LS_V1__R:
			case LS_V1_TR:
			case LS_V1_T_:
			case LS_V1_TL:
			case LS_V1__L:
			case LS_V1_BL:
			case LS_V1_B_:
			case LS_H1_T_:
			case LS_H1_TR:
			case LS_H1_TL:
			case LS_H1_BR:
			case LS_H1_BL:
				retmove = PM_ReturnforQuad(saberMoveData[curmove].endQuad);
				break;
			}
			break;
			//transitioning to an attack
		case LS_A_TL2BR:
		case LS_A_L2R:
		case LS_A_BL2TR:
		case LS_A_BR2TL:
		case LS_A_R2L:
		case LS_A_TR2BL:
		case LS_A_T2B:
			if (newmove == curmove)
			{
				if (PM_SaberKataDone(curmove, newmove))
				{//done with this kata, must return to ready before attack again
					retmove = LS_R_TL2BR + (newmove - LS_A_TL2BR);
				}
				else
				{//okay to chain to another attack
					retmove = transitionMove[saberMoveData[curmove].endQuad][saberMoveData[newmove].startQuad];
				}
			}
			else if (saberMoveData[curmove].endQuad == saberMoveData[newmove].startQuad)
			{//new move starts from same quadrant
				retmove = newmove;
			}
			else
			{
				switch (curmove)
				{
					//transitioning from an attack
				case LS_A_TL2BR:
					if (newmove == LS_A_BR2TL)
					{
						retmove = LS_T1_TL_BR;
						break;
					}
				case LS_A_L2R:
					if (newmove == LS_A_R2L)
					{
						retmove = LS_T1_BL__R;
						break;
					}
				case LS_A_BL2TR:
					if (newmove == LS_A_TR2BL)
					{
						retmove = LS_T1_BL_TR;
						break;
					}
				case LS_A_BR2TL:
					if (newmove == LS_A_TL2BR)
					{
						retmove = LS_T1_BR_TL;
						break;
					}
				case LS_A_R2L:
					if (newmove == LS_A_L2R)
					{
						retmove = LS_T1_BL__L;
						break;
					}
				case LS_A_TR2BL:
					if (newmove == LS_A_BL2TR)
					{
						retmove = LS_T1_TR_BL;
						break;
					}
				case LS_A_T2B:
				case LS_D1_BR:
				case LS_D1__R:
				case LS_D1_TR:
				case LS_D1_T_:
				case LS_D1_TL:
				case LS_D1__L:
				case LS_D1_BL:
				case LS_D1_B_:
					retmove = transitionMove[saberMoveData[curmove].endQuad][saberMoveData[newmove].startQuad];
					break;
					//transitioning from a return
				case LS_R_R2L:
					if (newmove == LS_A_R2L)
					{
						retmove = LS_T1_BL__R;
						break;
					}
					else if (newmove == LS_A_BR2TL)
					{
						retmove = LS_T1_TL_BR;
						break;
					}
				case LS_R_TL2BR:
				case LS_R_L2R:
				case LS_R_BL2TR:
				case LS_R_BR2TL:
				case LS_R_TR2BL:
				case LS_R_T2B:
					//transition is the start
					retmove = LS_S_TL2BR + (newmove - LS_A_TL2BR);
					break;
					//transitioning from a bounce
					//bounces should transition to transitions before attacks.
				case LS_B1_BR:
				case LS_B1__R:
				case LS_B1_TR:
				case LS_B1_T_:
				case LS_B1_TL:
				case LS_B1__L:
				case LS_B1_BL:
					//transitioning from a parry/reflection/knockaway/broken parry
				case LS_PARRY_UP:
				case LS_PARRY_FRONT:
				case LS_PARRY_UR:
				case LS_PARRY_UL:
				case LS_PARRY_LR:
				case LS_PARRY_LL:
				case LS_REFLECT_UP:
				case LS_REFLECT_FRONT:
				case LS_REFLECT_UR:
				case LS_REFLECT_UL:
				case LS_REFLECT_LR:
				case LS_REFLECT_LL:
				case LS_REFLECT_ATTACK_LEFT:
				case LS_REFLECT_ATTACK_RIGHT:
				case LS_REFLECT_ATTACK_FRONT:
				case LS_K1_T_:
				case LS_K1_TR:
				case LS_K1_TL:
				case LS_K1_BR:
				case LS_K1_BL:
				case LS_V1_BR:
				case LS_V1__R:
				case LS_V1_TR:
				case LS_V1_T_:
				case LS_V1_TL:
				case LS_V1__L:
				case LS_V1_BL:
				case LS_V1_B_:
				case LS_H1_T_:
				case LS_H1_TR:
				case LS_H1_TL:
				case LS_H1_BR:
				case LS_H1_BL:
					retmove = transitionMove[saberMoveData[curmove].endQuad][saberMoveData[newmove].startQuad];
					break;
					//NB: transitioning from transitions is fine
				}
			}
			break;
			//transitioning to any other anim is not supported
		}
	}

	if (retmove == LS_NONE)
	{
		return newmove;
	}

	return ((saberMoveName_t)retmove);
}

extern qboolean BG_InKnockDown(int anim);
saberMoveName_t PM_CheckStabDown(void)
{
	vec3_t faceFwd, facingAngles;
	vec3_t fwd;
	bgEntity_t* ent = NULL;
	trace_t tr;
	//yeah, vm's may complain, but.. who cares!
	vec3_t trmins = { -15, -15, -15 };
	vec3_t trmaxs = { 15, 15, 15 };

	saberInfo_t* saber1;
	saberInfo_t* saber2;

	saber1 = BG_MySaber(pm->ps->clientNum, 0);
	saber2 = BG_MySaber(pm->ps->clientNum, 1);

	
	if (saber1
		&& (saber1->saberFlags & SFL_NO_STABDOWN))
	{
		return LS_NONE;
	}
	if (saber2
		&& (saber2->saberFlags& SFL_NO_STABDOWN))
	{
		return LS_NONE;
	}

	if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
	{//sorry must be on ground!
		return LS_NONE;
	}
	if (pm->ps->clientNum < MAX_CLIENTS)
	{//player
		pm->ps->velocity[2] = 0;
		pm->cmd.upmove = 0;
	}

	VectorSet(facingAngles, 0, pm->ps->viewangles[YAW], 0);
	AngleVectors(facingAngles, faceFwd, NULL, NULL);

	//FIXME: need to only move forward until we bump into our target...?
	VectorMA(pm->ps->origin, 164.0f, faceFwd, fwd);

	pm->trace(&tr, pm->ps->origin, trmins, trmaxs, fwd, pm->ps->clientNum, MASK_PLAYERSOLID);

	if (tr.entityNum < ENTITYNUM_WORLD)
	{
		ent = PM_BGEntForNum(tr.entityNum);
	}

	if (ent &&
		(ent->s.eType == ET_PLAYER || ent->s.eType == ET_NPC) &&
		BG_InKnockDown(ent->s.legsAnim))
	{//guy is on the ground below me, do a top-down attack
		if (pm->ps->fd.saberAnimLevel == SS_DUAL)
		{
			return LS_STABDOWN_DUAL;
		}
		else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
		{
			if (saber1->type == SABER_BACKHAND)//saber backhand
			{
				return LS_STABDOWN_BACKHAND;
			}
			else if (saber1->type == SABER_ASBACKHAND)//saber backhand
			{
				return LS_STABDOWN_BACKHAND;
			}
			else
			{
				return LS_STABDOWN_STAFF;
			}
		}
		else
		{
			return LS_STABDOWN;
		}
	}
	return LS_NONE;
}

int PM_SaberMoveQuadrantForMovement(usercmd_t * ucmd)
{
	if (ucmd->rightmove > 0)
	{//moving right
		if (ucmd->forwardmove > 0)
		{//forward right = TL2BR slash
			return Q_TL;
		}
		else if (ucmd->forwardmove < 0)
		{//backward right = BL2TR uppercut
			return Q_BL;
		}
		else
		{//just right is a left slice
			return Q_L;
		}
	}
	else if (ucmd->rightmove < 0)
	{//moving left
		if (ucmd->forwardmove > 0)
		{//forward left = TR2BL slash
			return Q_TR;
		}
		else if (ucmd->forwardmove < 0)
		{//backward left = BR2TL uppercut
			return Q_BR;
		}
		else
		{//just left is a right slice
			return Q_R;
		}
	}
	else
	{//not moving left or right
		if (ucmd->forwardmove > 0)
		{//forward= T2B slash
			return Q_T;
		}
		else if (ucmd->forwardmove < 0)
		{//backward= T2B slash	//or B2T uppercut?
			return Q_T;
		}
		else
		{//Not moving at all
			return Q_R;
		}
	}
}

//===================================================================
qboolean PM_SaberInBounce(int move)
{
	if (move >= LS_B1_BR && move <= LS_B1_BL)
	{
		return qtrue;
	}
	if (move >= LS_D1_BR && move <= LS_D1_BL)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInTransition(int move);

int saberMoveTransitionAngle[Q_NUM_QUADS][Q_NUM_QUADS] =
{
	{
		0,//Q_BR,Q_BR,
		45,//Q_BR,Q_R,
		90,//Q_BR,Q_TR,
		135,//Q_BR,Q_T,
		180,//Q_BR,Q_TL,
		215,//Q_BR,Q_L,
		270,//Q_BR,Q_BL,
		45,//Q_BR,Q_B,
	},
	{
		45,//Q_R,Q_BR,
		0,//Q_R,Q_R,
		45,//Q_R,Q_TR,
		90,//Q_R,Q_T,
		135,//Q_R,Q_TL,
		180,//Q_R,Q_L,
		215,//Q_R,Q_BL,
		90,//Q_R,Q_B,
	},
	{
		90,//Q_TR,Q_BR,
		45,//Q_TR,Q_R,
		0,//Q_TR,Q_TR,
		45,//Q_TR,Q_T,
		90,//Q_TR,Q_TL,
		135,//Q_TR,Q_L,
		180,//Q_TR,Q_BL,
		135,//Q_TR,Q_B,
	},
	{
		135,//Q_T,Q_BR,
		90,//Q_T,Q_R,
		45,//Q_T,Q_TR,
		0,//Q_T,Q_T,
		45,//Q_T,Q_TL,
		90,//Q_T,Q_L,
		135,//Q_T,Q_BL,
		180,//Q_T,Q_B,
	},
	{
		180,//Q_TL,Q_BR,
		135,//Q_TL,Q_R,
		90,//Q_TL,Q_TR,
		45,//Q_TL,Q_T,
		0,//Q_TL,Q_TL,
		45,//Q_TL,Q_L,
		90,//Q_TL,Q_BL,
		135,//Q_TL,Q_B,
	},
	{
		215,//Q_L,Q_BR,
		180,//Q_L,Q_R,
		135,//Q_L,Q_TR,
		90,//Q_L,Q_T,
		45,//Q_L,Q_TL,
		0,//Q_L,Q_L,
		45,//Q_L,Q_BL,
		90,//Q_L,Q_B,
	},
	{
		270,//Q_BL,Q_BR,
		215,//Q_BL,Q_R,
		180,//Q_BL,Q_TR,
		135,//Q_BL,Q_T,
		90,//Q_BL,Q_TL,
		45,//Q_BL,Q_L,
		0,//Q_BL,Q_BL,
		45,//Q_BL,Q_B,
	},
	{
		45,//Q_B,Q_BR,
		90,//Q_B,Q_R,
		135,//Q_B,Q_TR,
		180,//Q_B,Q_T,
		135,//Q_B,Q_TL,
		90,//Q_B,Q_L,
		45,//Q_B,Q_BL,
		0//Q_B,Q_B,
	}
};

int PM_SaberAttackChainAngle(int move1, int move2)
{
	if (move1 == -1 || move2 == -1)
	{
		return -1;
	}
	return saberMoveTransitionAngle[saberMoveData[move1].endQuad][saberMoveData[move2].startQuad];
}

qboolean PM_SaberKataDone(int curmove , int newmove )
{
	saberInfo_t* saber1;

	saber1 = BG_MySaber(pm->ps->clientNum, 0);

	if (pm->ps->m_iVehicleNum)
	{ //never continue kata on vehicle
		if (pm->ps->saberAttackChainCount > MISHAPLEVEL_NONE)
		{
			return qtrue;
		}
	}

	if (pm->ps->fd.forceRageRecoveryTime > pm->cmd.serverTime)
	{//rage recovery, only 1 swing at a time (tired)
		if (pm->ps->saberAttackChainCount > MISHAPLEVEL_NONE)
		{//swung once
			return qtrue;
		}
		else
		{//allow one attack
			return qfalse;
		}
	}
	else if ((pm->ps->fd.forcePowersActive & (1 << FP_RAGE)))
	{//infinite chaining when raged
		return qfalse;
	}
	else if (saber1[0].maxChain == -1)
	{
		return qfalse;
	}
	else if (saber1[0].maxChain != 0)
	{
		if (pm->ps->saberAttackChainCount >= saber1[0].maxChain)
		{
			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}

	if (pm->ps->fd.saberAnimLevel == SS_DESANN
		|| pm->ps->fd.saberAnimLevel == SS_STRONG
		|| pm->ps->fd.saberAnimLevel == SS_TAVION
		|| pm->ps->fd.saberAnimLevel == SS_STAFF
		|| pm->ps->fd.saberAnimLevel == SS_DUAL
		|| pm->ps->fd.saberAnimLevel == SS_MEDIUM)
	{//desann and tavion can link up as many attacks as they want
		return qfalse;
	}
	else if (pm->ps->fd.saberAnimLevel == FORCE_LEVEL_3)
	{
		if (curmove == LS_NONE || newmove == LS_NONE)
		{
			if (pm->ps->fd.saberAnimLevel >= FORCE_LEVEL_3 && pm->ps->saberAttackChainCount > Q_irand(MISHAPLEVEL_NONE, MISHAPLEVEL_MIN))
			{
				return qtrue;
			}
		}
		else if (pm->ps->saberAttackChainCount > Q_irand(MISHAPLEVEL_RUNINACCURACY, MISHAPLEVEL_SNIPER))
		{
			return qtrue;
		}
		else if (pm->ps->saberAttackChainCount > MISHAPLEVEL_NONE)
		{
			int chainAngle = PM_SaberAttackChainAngle(curmove, newmove);
			if (chainAngle < 135 || chainAngle > 215)
			{//if trying to chain to a move that doesn't continue the momentum
				return qtrue;
			}
			else if (chainAngle == 180)
			{//continues the momentum perfectly, allow it to chain 66% of the time
				if (pm->ps->saberAttackChainCount > MISHAPLEVEL_MIN)
				{
					return qtrue;
				}
			}
			else
			{//would continue the movement somewhat, 50% chance of continuing
				if (pm->ps->saberAttackChainCount > MISHAPLEVEL_RUNINACCURACY)
				{
					return qtrue;
				}
			}
		}
	}
	else
	{//FIXME: have chainAngle influence fast and medium chains as well?
		if (newmove == LS_A_TL2BR ||
			newmove == LS_A_L2R ||
			newmove == LS_A_BL2TR ||
			newmove == LS_A_BR2TL ||
			newmove == LS_A_R2L ||
			newmove == LS_A_TR2BL)
		{ //lower chaining tolerance for spinning saber anims
			int chainTolerance;

			if (pm->ps->fd.saberAnimLevel == FORCE_LEVEL_1)
			{
				chainTolerance = 5;
			}
			else
			{
				chainTolerance = 3;
			}

			if (pm->ps->saberAttackChainCount >= chainTolerance && Q_irand(MISHAPLEVEL_MIN, pm->ps->saberAttackChainCount) > chainTolerance)
			{
				return qtrue;
			}
		}
		if ((pm->ps->fd.saberAnimLevel == FORCE_LEVEL_2 || pm->ps->fd.saberAnimLevel == SS_DUAL) && pm->ps->saberAttackChainCount > Q_irand(MISHAPLEVEL_RUNINACCURACY, MISHAPLEVEL_LIGHT))
		{
			return qtrue;
		}
	}
	return qfalse;
}

void PM_SetAnimFrame(playerState_t * gent, int frame, qboolean torso, qboolean legs)
{
	gent->saberLockFrame = frame;
}

int PM_SaberLockWinAnim(qboolean victory, qboolean superBreak)
{
	int winAnim = -1;
	switch (pm->ps->torsoAnim)
	{
	case BOTH_BF2LOCK:
		if (superBreak)
		{
			winAnim = BOTH_LK_S_S_T_SB_1_W;
		}
		else if (!victory)
		{
			winAnim = BOTH_BF1BREAK;
		}
		else
		{
			pm->ps->saberMove = LS_A_T2B;
			winAnim = BOTH_A3_T__B_;
		}
		break;
	case BOTH_BF1LOCK:
		if (superBreak)
		{
			winAnim = BOTH_LK_S_S_T_SB_1_W;
		}
		else if (!victory)
		{
			winAnim = BOTH_KNOCKDOWN4;
		}
		else
		{
			pm->ps->saberMove = LS_K1_T_;
			winAnim = BOTH_K1_S1_T_;
		}
		break;
	case BOTH_CWCIRCLELOCK:
		if (superBreak)
		{
			winAnim = BOTH_LK_S_S_S_SB_1_W;
		}
		else if (!victory)
		{
			pm->ps->saberMove = LS_V1_BL;//pm->ps->saberBounceMove =
			pm->ps->saberBlocked = BLOCKED_PARRY_BROKEN;
			winAnim = BOTH_V1_BL_S1;
		}
		else
		{
			winAnim = BOTH_CWCIRCLEBREAK;
		}
		break;
	case BOTH_CCWCIRCLELOCK:
		if (superBreak)
		{
			winAnim = BOTH_LK_S_S_S_SB_1_W;
		}
		else if (!victory)
		{
			pm->ps->saberMove = LS_V1_BR;//pm->ps->saberBounceMove =
			pm->ps->saberBlocked = BLOCKED_PARRY_BROKEN;
			winAnim = BOTH_V1_BR_S1;
		}
		else
		{
			winAnim = BOTH_CCWCIRCLEBREAK;
		}
		break;
	default:
		//must be using new system:
		break;
	}
	if (winAnim != -1)
	{
		PM_SetAnim(SETANIM_BOTH, winAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		pm->ps->weaponTime = pm->ps->torsoTimer;
		pm->ps->saberBlocked = BLOCKED_NONE;
		pm->ps->weaponstate = WEAPON_FIRING;
	}
	return winAnim;
}

// Need to avoid nesting namespaces!
#ifdef _GAME //including game headers on cgame is FORBIDDEN ^_^
#include "g_local.h"
extern void NPC_SetAnim(gentity_t * ent, int setAnimParts, int anim, int setAnimFlags);
extern gentity_t g_entities[];
#elif defined(_CGAME)
#include "cgame/cg_local.h" //ahahahahhahahaha@$!$!
#endif

int PM_SaberLockLoseAnim(playerState_t * genemy, qboolean victory, qboolean superBreak)
{
	int loseAnim = -1;

	switch (genemy->torsoAnim)
	{
	case BOTH_BF2LOCK:
		if (superBreak)
		{
			loseAnim = BOTH_LK_S_S_T_SB_1_L;
		}
		else if (!victory)
		{
			loseAnim = BOTH_BF1BREAK;
		}
		else
		{
			if (!victory)
			{//no-one won
				genemy->saberMove = LS_K1_T_;
				loseAnim = BOTH_K1_S1_T_;
			}
			else
			{//FIXME: this anim needs to transition back to ready when done
				loseAnim = BOTH_BF1BREAK;
			}
		}
		break;
	case BOTH_BF1LOCK:
		if (superBreak)
		{
			loseAnim = BOTH_LK_S_S_T_SB_1_L;
		}
		else if (!victory)
		{
			loseAnim = BOTH_KNOCKDOWN4;
		}
		else
		{
			if (!victory)
			{//no-one won
				genemy->saberMove = LS_A_T2B;
				loseAnim = BOTH_A3_T__B_;
			}
			else
			{
				loseAnim = BOTH_KNOCKDOWN4;
			}
		}
		break;
	case BOTH_CWCIRCLELOCK:
		if (superBreak)
		{
			loseAnim = BOTH_LK_S_S_S_SB_1_L;
		}
		else if (!victory)
		{
			genemy->saberMove = genemy->saberBounceMove = LS_V1_BL;
			genemy->saberBlocked = BLOCKED_PARRY_BROKEN;
			loseAnim = BOTH_V1_BL_S1;
		}
		else
		{
			if (!victory)
			{//no-one won
				loseAnim = BOTH_CCWCIRCLEBREAK;
			}
			else
			{
				genemy->saberMove = genemy->saberBounceMove = LS_V1_BL;
				genemy->saberBlocked = BLOCKED_PARRY_BROKEN;
				loseAnim = BOTH_V1_BL_S1;
			}
		}
		break;
	case BOTH_CCWCIRCLELOCK:
		if (superBreak)
		{
			loseAnim = BOTH_LK_S_S_S_SB_1_L;
		}
		else if (!victory)
		{
			genemy->saberMove = genemy->saberBounceMove = LS_V1_BR;
			genemy->saberBlocked = BLOCKED_PARRY_BROKEN;
			loseAnim = BOTH_V1_BR_S1;
		}
		else
		{
			if (!victory)
			{//no-one won
				loseAnim = BOTH_CWCIRCLEBREAK;
			}
			else
			{
				genemy->saberMove = genemy->saberBounceMove = LS_V1_BR;
				genemy->saberBlocked = BLOCKED_PARRY_BROKEN;
				loseAnim = BOTH_V1_BR_S1;
			}
		}
		break;
	}
	if (loseAnim != -1)
	{
#ifdef _GAME
		NPC_SetAnim(&g_entities[genemy->clientNum], SETANIM_BOTH, loseAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		genemy->weaponTime = genemy->torsoTimer;// + 250;
#endif
		genemy->saberBlocked = BLOCKED_NONE;
		genemy->weaponstate = WEAPON_READY;
	}
	return loseAnim;
}

int PM_SaberLockResultAnim(playerState_t * duelist, qboolean superBreak, qboolean won)
{
	int baseAnim = duelist->torsoAnim;
	switch (baseAnim)
	{
	case BOTH_LK_S_S_S_L_2:		//lock if I'm using single vs. a single and other intitiated
		baseAnim = BOTH_LK_S_S_S_L_1;
		break;
	case BOTH_LK_S_S_T_L_2:		//lock if I'm using single vs. a single and other initiated
		baseAnim = BOTH_LK_S_S_T_L_1;
		break;
	case BOTH_LK_DL_DL_S_L_2:	//lock if I'm using dual vs. dual and other initiated
		baseAnim = BOTH_LK_DL_DL_S_L_1;
		break;
	case BOTH_LK_DL_DL_T_L_2:	//lock if I'm using dual vs. dual and other initiated
		baseAnim = BOTH_LK_DL_DL_T_L_1;
		break;
	case BOTH_LK_ST_ST_S_L_2:	//lock if I'm using staff vs. a staff and other initiated
		baseAnim = BOTH_LK_ST_ST_S_L_1;
		break;
	case BOTH_LK_ST_ST_T_L_2:	//lock if I'm using staff vs. a staff and other initiated
		baseAnim = BOTH_LK_ST_ST_T_L_1;
		break;
	}
	//what kind of break?
	if (!superBreak)
	{
		baseAnim -= 2;
	}
	else if (superBreak)
	{
		baseAnim += 1;
	}
	else
	{//WTF?  Not a valid result
		return -1;
	}
	//win or lose?
	if (won)
	{
		baseAnim += 1;
	}

	//play the anim and hold it
#ifdef _GAME
	//server-side: set it on the other guy, too
	if (duelist->clientNum == pm->ps->clientNum)
	{//me
		PM_SetAnim(SETANIM_BOTH, baseAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	else
	{//other guy
		NPC_SetAnim(&g_entities[duelist->clientNum], SETANIM_BOTH, baseAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
#else
	PM_SetAnim(SETANIM_BOTH, baseAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
#endif

	if (superBreak
		&& !won)
	{//if you lose a superbreak, you're defenseless
#ifdef _GAME
		if (1)
#else
		if (duelist->clientNum == pm->ps->clientNum)
#endif
		{
			//set sabermove to none
			duelist->saberMove = LS_NONE;
			//Hold the anim a little longer than it is
			duelist->torsoTimer += 250;
		}
	}

#ifdef _GAME
	if (1)
#else
	if (duelist->clientNum == pm->ps->clientNum)
#endif
	{
		//no attacking during this anim
		duelist->weaponTime = duelist->torsoTimer;
		duelist->saberBlocked = BLOCKED_NONE;
	}
	return baseAnim;
}

#ifdef _GAME
extern void G_Knockdown(gentity_t * self, gentity_t * attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock);
#endif
void PM_SaberLockBreak(playerState_t * genemy, qboolean victory, int strength)
{
	int	winAnim = BOTH_STAND1, loseAnim = BOTH_STAND1;
	qboolean singleVsSingle = qtrue;
	qboolean superBreak = (pm->cmd.buttons & BUTTON_ATTACK);

	pm->ps->userInt3 &= ~(1 << FLAG_LOCKWINNER);
	genemy->userInt3 &= ~(1 << FLAG_LOCKWINNER);

	winAnim = PM_SaberLockWinAnim(victory, superBreak);

	if (winAnim != -1)
	{//a single vs. single break
		loseAnim = PM_SaberLockLoseAnim(genemy, victory, superBreak);
	}
	else
	{//must be a saberlock that's not between single and single...
		singleVsSingle = qfalse;
		winAnim = PM_SaberLockResultAnim(pm->ps, superBreak, qtrue);
		pm->ps->weaponstate = WEAPON_FIRING;
		loseAnim = PM_SaberLockResultAnim(genemy, superBreak, qfalse);
		genemy->weaponstate = WEAPON_READY;
	}

	if (victory)
	{ //someone lost the lock, so punish them by knocking them down
		if (!superBreak)
		{//if we're not in a superbreak, force the loser to mishap.
			pm->checkDuelLoss = genemy->clientNum + 1;
		}
	}
	else
	{ //If no one lost, then shove each player away from the other
		vec3_t oppDir;
		strength = 10;

		VectorSubtract(genemy->origin, pm->ps->origin, oppDir);
		VectorNormalize(oppDir);
		genemy->velocity[0] = oppDir[0] * (strength * 40);
		genemy->velocity[1] = oppDir[1] * (strength * 40);
		genemy->velocity[2] = 0;

		VectorSubtract(pm->ps->origin, genemy->origin, oppDir);
		VectorNormalize(oppDir);
		pm->ps->velocity[0] = oppDir[0] * (strength * 40);
		pm->ps->velocity[1] = oppDir[1] * (strength * 40);
		pm->ps->velocity[2] = 0;
	}

	pm->ps->saberLockTime = genemy->saberLockTime = 0;
	pm->ps->saberLockFrame = genemy->saberLockFrame = 0;
	pm->ps->saberLockEnemy = genemy->saberLockEnemy = 0;

	PM_AddEvent(EV_JUMP);
	if (!victory)
	{//no-one won
		BG_AddPredictableEventToPlayerstate(EV_JUMP, 0, genemy);
	}
	else
	{
		if (PM_irand_timesync(0, 1))
		{
			BG_AddPredictableEventToPlayerstate(EV_JUMP, PM_irand_timesync(0, 75), genemy);
		}
	}
}

qboolean G_CheckIncrementLockAnim(int anim, int winOrLose)
{
	qboolean increment = qfalse;//???
	//RULE: if you are the first style in the lock anim, you advance from LOSING position to WINNING position
	//		if you are the second style in the lock anim, you advance from WINNING position to LOSING position
	switch (anim)
	{
		//increment to win:
	case BOTH_LK_DL_DL_S_L_1:	//lock if I'm using dual vs. dual and I initiated
	case BOTH_LK_DL_DL_S_L_2:	//lock if I'm using dual vs. dual and other initiated
	case BOTH_LK_DL_DL_T_L_1:	//lock if I'm using dual vs. dual and I initiated
	case BOTH_LK_DL_DL_T_L_2:	//lock if I'm using dual vs. dual and other initiated
	case BOTH_LK_DL_S_S_L_1:		//lock if I'm using dual vs. a single
	case BOTH_LK_DL_S_T_L_1:		//lock if I'm using dual vs. a single
	case BOTH_LK_DL_ST_S_L_1:	//lock if I'm using dual vs. a staff
	case BOTH_LK_DL_ST_T_L_1:	//lock if I'm using dual vs. a staff
	case BOTH_LK_S_S_S_L_1:		//lock if I'm using single vs. a single and I initiated
	case BOTH_LK_S_S_T_L_2:		//lock if I'm using single vs. a single and other initiated
	case BOTH_LK_ST_S_S_L_1:		//lock if I'm using staff vs. a single
	case BOTH_LK_ST_S_T_L_1:		//lock if I'm using staff vs. a single
	case BOTH_LK_ST_ST_T_L_1:	//lock if I'm using staff vs. a staff and I initiated
	case BOTH_LK_ST_ST_T_L_2:	//lock if I'm using staff vs. a staff and other initiated
		if (winOrLose == SABERLOCK_WIN)
		{
			increment = qtrue;
		}
		else
		{
			increment = qfalse;
		}
		break;

		//decrement to win:
	case BOTH_LK_S_DL_S_L_1:		//lock if I'm using single vs. a dual
	case BOTH_LK_S_DL_T_L_1:		//lock if I'm using single vs. a dual
	case BOTH_LK_S_S_S_L_2:		//lock if I'm using single vs. a single and other intitiated
	case BOTH_LK_S_S_T_L_1:		//lock if I'm using single vs. a single and I initiated
	case BOTH_LK_S_ST_S_L_1:		//lock if I'm using single vs. a staff
	case BOTH_LK_S_ST_T_L_1:		//lock if I'm using single vs. a staff
	case BOTH_LK_ST_DL_S_L_1:	//lock if I'm using staff vs. dual
	case BOTH_LK_ST_DL_T_L_1:	//lock if I'm using staff vs. dual
	case BOTH_LK_ST_ST_S_L_1:	//lock if I'm using staff vs. a staff and I initiated
	case BOTH_LK_ST_ST_S_L_2:	//lock if I'm using staff vs. a staff and other initiated
		if (winOrLose == SABERLOCK_WIN)
		{
			increment = qfalse;
		}
		else
		{
			increment = qtrue;
		}
		break;
	default:
		break;
	}
	return increment;
}

void PM_SaberLocked(void)
{
	int	remaining = 0;
	playerState_t* genemy;
	bgEntity_t* eGenemy = PM_BGEntForNum(pm->ps->saberLockEnemy);

	if (!eGenemy)
	{
		return;
	}

	genemy = eGenemy->playerState;

	if (!genemy)
	{
		return;
	}

	PM_AddEventWithParm(EV_SABERLOCK, pm->ps->saberLockEnemy);

	if (pm->ps->saberLockFrame &&
		genemy->saberLockFrame &&
		BG_InSaberLock(pm->ps->torsoAnim) &&
		BG_InSaberLock(genemy->torsoAnim))
	{
		float dist = 0;

		pm->ps->torsoTimer = 0;
		pm->ps->weaponTime = 0;
		genemy->torsoTimer = 0;
		genemy->weaponTime = 0;

		dist = DistanceSquared(pm->ps->origin, genemy->origin);
		if (dist < 64 || dist > 6400)
		{//between 8 and 80 from each other
			PM_SaberLockBreak(genemy, qfalse, 0);
			return;
		}
		if (pm->ps->saberLockAdvance)
		{//holding attack
			animation_t* anim;
			float		currentFrame;
			int			curFrame;
			int			strength = 1;

			pm->ps->saberLockAdvance = qfalse;

			anim = &pm->animations[pm->ps->torsoAnim];

			currentFrame = pm->ps->saberLockFrame;

			//advance/decrement my frame number
			if (BG_InSaberLockOld(pm->ps->torsoAnim))
			{ //old locks
				if ((pm->ps->torsoAnim) == BOTH_CCWCIRCLELOCK ||
					(pm->ps->torsoAnim) == BOTH_BF2LOCK)
				{
					curFrame = floor(currentFrame) - strength;
					//drop my frame one
					if (curFrame <= anim->firstFrame)
					{//I won!  Break out
						PM_SaberLockBreak(genemy, qtrue, strength);
						return;
					}
					else
					{
						PM_SetAnimFrame(pm->ps, curFrame, qtrue, qtrue);
						remaining = curFrame - anim->firstFrame;
					}
				}
				else
				{
					curFrame = ceil(currentFrame) + strength;
					//advance my frame one
					if (curFrame >= anim->firstFrame + anim->numFrames)
					{//I won!  Break out
						PM_SaberLockBreak(genemy, qtrue, strength);
						return;
					}
					else
					{
						PM_SetAnimFrame(pm->ps, curFrame, qtrue, qtrue);
						remaining = anim->firstFrame + anim->numFrames - curFrame;
					}
				}
			}
			else
			{ //new locks
				if (G_CheckIncrementLockAnim(pm->ps->torsoAnim, SABERLOCK_WIN))
				{
					curFrame = ceil(currentFrame) + strength;
					//advance my frame one
					if (curFrame >= anim->firstFrame + anim->numFrames)
					{//I won!  Break out
						PM_SaberLockBreak(genemy, qtrue, strength);
						return;
					}
					else
					{
						PM_SetAnimFrame(pm->ps, curFrame, qtrue, qtrue);
						remaining = anim->firstFrame + anim->numFrames - curFrame;
					}
				}
				else
				{
					curFrame = floor(currentFrame) - strength;
					//drop my frame one
					if (curFrame <= anim->firstFrame)
					{//I won!  Break out
						PM_SaberLockBreak(genemy, qtrue, strength);
						return;
					}
					else
					{
						PM_SetAnimFrame(pm->ps, curFrame, qtrue, qtrue);
						remaining = curFrame - anim->firstFrame;
					}
				}
			}
			if (!PM_irand_timesync(0, 2))
			{
				PM_AddEvent(EV_JUMP);
			}
			//advance/decrement enemy frame number
			anim = &pm->animations[(genemy->torsoAnim)];

			if (BG_InSaberLockOld(genemy->torsoAnim))
			{
				if ((genemy->torsoAnim) == BOTH_CWCIRCLELOCK ||
					(genemy->torsoAnim) == BOTH_BF1LOCK)
				{
					if (!PM_irand_timesync(0, 2))
					{
						BG_AddPredictableEventToPlayerstate(EV_PAIN, floor((float)80 / 100 * 100.0f), genemy);
					}
					PM_SetAnimFrame(genemy, anim->firstFrame + remaining, qtrue, qtrue);
				}
				else
				{
					PM_SetAnimFrame(genemy, anim->firstFrame + anim->numFrames - remaining, qtrue, qtrue);
				}
			}
			else
			{//new locks
				if (G_CheckIncrementLockAnim(genemy->torsoAnim, SABERLOCK_LOSE))
				{
					if (!PM_irand_timesync(0, 2))
					{
						BG_AddPredictableEventToPlayerstate(EV_PAIN, floor((float)80 / 100 * 100.0f), genemy);
					}
					PM_SetAnimFrame(genemy, anim->firstFrame + anim->numFrames - remaining, qtrue, qtrue);
				}
				else
				{
					PM_SetAnimFrame(genemy, anim->firstFrame + remaining, qtrue, qtrue);
				}
			}
		}
	}
	else
	{//something broke us out of it
		PM_SaberLockBreak(genemy, qfalse, 0);
	}
}

qboolean PM_SaberInBrokenParry(int move)
{
	if (move == 139 || move == 133)
	{
		return qfalse;
	}
	if (move >= LS_V1_BR && move <= LS_V1_B_)
	{
		return qtrue;
	}
	if (move >= LS_H1_T_ && move <= LS_H1_BL)
	{
		return qtrue;
	}
	return qfalse;
}


saberMoveName_t PM_BrokenParryForParry(int move)
{
	switch (move)
	{
	case LS_PARRY_UP:
	case LS_PARRY_FRONT:
		//Hmm... since we don't know what dir the hit came from, randomly pick knock down or knock back
		if (Q_irand(0, 1))
		{
			return LS_H1_B_;
		}
		else
		{
			return LS_H1_T_;
		}
		break;
	case LS_PARRY_UR:
		return LS_H1_TR;
		break;
	case LS_PARRY_UL:
		return LS_H1_TL;
		break;
	case LS_PARRY_LR:
		return LS_H1_BR;
		break;
	case LS_PARRY_LL:
		return LS_H1_BL;
		break;
	case LS_READY:
		return LS_H1_B_;//???
		break;
	}
	return LS_NONE;
}

#define BACK_STAB_DISTANCE 128

qboolean PM_CanBackstab(void)
{
	trace_t tr;
	vec3_t flatAng;
	vec3_t fwd, back;
	vec3_t trmins = { -15, -15, -8 };
	vec3_t trmaxs = { 15, 15, 8 };

	VectorCopy(pm->ps->viewangles, flatAng);
	flatAng[PITCH] = 0;

	AngleVectors(flatAng, fwd, 0, 0);

	back[0] = pm->ps->origin[0] - fwd[0] * BACK_STAB_DISTANCE;
	back[1] = pm->ps->origin[1] - fwd[1] * BACK_STAB_DISTANCE;
	back[2] = pm->ps->origin[2] - fwd[2] * BACK_STAB_DISTANCE;

	pm->trace(&tr, pm->ps->origin, trmins, trmaxs, back, pm->ps->clientNum, MASK_PLAYERSOLID);

	if (tr.fraction != 1.0 && tr.entityNum >= 0 && tr.entityNum < ENTITYNUM_NONE)
	{
		bgEntity_t* bgEnt = PM_BGEntForNum(tr.entityNum);

		if (bgEnt && (bgEnt->s.eType == ET_PLAYER || bgEnt->s.eType == ET_NPC))
		{
			return qtrue;
		}
	}

	return qfalse;
}

saberMoveName_t PM_SaberFlipOverAttackMove(void)
{
	vec3_t fwdAngles, jumpFwd;

	saberInfo_t* saber1;
	saberInfo_t* saber2;

	saber1 = BG_MySaber(pm->ps->clientNum, 0);
	saber2 = BG_MySaber(pm->ps->clientNum, 1);
	//see if we have an overridden (or cancelled) lunge move
	if (saber1
		&& saber1->jumpAtkFwdMove != LS_INVALID)
	{
		if (saber1->jumpAtkFwdMove != LS_NONE)
		{
			return (saberMoveName_t)saber1->jumpAtkFwdMove;
		}
	}
	if (saber2
		&& saber2->jumpAtkFwdMove != LS_INVALID)
	{
		if (saber2->jumpAtkFwdMove != LS_NONE)
		{
			return (saberMoveName_t)saber2->jumpAtkFwdMove;
		}
	}
	//no overrides, cancelled?
	if (saber1
		&& saber1->jumpAtkFwdMove == LS_NONE)
	{
		return LS_A_T2B;//LS_NONE;
	}
	if (saber2
		&& saber2->jumpAtkFwdMove == LS_NONE)
	{
		return LS_A_T2B;//LS_NONE;
	}
	//just do it
	VectorCopy(pm->ps->viewangles, fwdAngles);
	fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
	AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
	VectorScale(jumpFwd, 150, pm->ps->velocity);//was 50
	pm->ps->velocity[2] = 400;

	PM_SetForceJumpZStart(pm->ps->origin[2]);//so we don't take damage if we land at same height

	PM_AddEvent(EV_JUMP);
	pm->ps->fd.forceJumpSound = 1;
	pm->cmd.upmove = 0;

	if (pm->ps->fd.saberAnimLevel == SS_FAST || pm->ps->fd.saberAnimLevel == SS_TAVION)
	{
		return LS_A_FLIP_STAB;
	}
	else
	{
		return LS_A_FLIP_SLASH;
	}
}

qboolean BG_SaberAttacking(playerState_t * ps)
{
	if (PM_SaberInParry(ps->saberMove))
	{
		return qfalse;
	}
	if (PM_SaberInBrokenParry(ps->saberMove))
	{
		return qfalse;
	}
	if (PM_SaberInDeflect(ps->saberMove))
	{
		return qfalse;
	}
	if (PM_SaberInBounce(ps->saberMove))
	{
		return qfalse;
	}
	if (PM_SaberInKnockaway(ps->saberMove))
	{
		return qfalse;
	}

	if (PM_SaberInAttack(ps->saberMove))
	{
		if (ps->weaponstate == WEAPON_FIRING && ps->saberBlocked == BLOCKED_NONE)
		{ //if we're firing and not blocking, then we're attacking.
			return qtrue;
		}
	}

	if (PM_SaberInSpecial(ps->saberMove))
	{
		return qtrue;
	}

	return qfalse;
}

int PM_SaberBackflipAttackMove(void)
{

	saberInfo_t* saber1;
	saberInfo_t* saber2;

	saber1 = BG_MySaber(pm->ps->clientNum, 0);
	saber2 = BG_MySaber(pm->ps->clientNum, 1);
	//see if we have an overridden (or cancelled) lunge move
	if (saber1
		&& saber1->jumpAtkBackMove != LS_INVALID)
	{
		if (saber1->jumpAtkBackMove != LS_NONE)
		{
			return (saberMoveName_t)saber1->jumpAtkBackMove;
		}
	}
	if (saber2
		&& saber2->jumpAtkBackMove != LS_INVALID)
	{
		if (saber2->jumpAtkBackMove != LS_NONE)
		{
			return (saberMoveName_t)saber2->jumpAtkBackMove;
		}
	}
	//no overrides, cancelled?
	if (saber1
		&& saber1->jumpAtkBackMove == LS_NONE)
	{
		return LS_A_T2B;//LS_NONE;
	}
	if (saber2
		&& saber2->jumpAtkBackMove == LS_NONE)
	{
		return LS_A_T2B;//LS_NONE;
	}
	//just do it
	pm->cmd.upmove = 127;
	pm->ps->velocity[2] = 500;
	return LS_A_BACKFLIP_ATK;
}

int PM_SaberDualJumpAttackMove(void)
{

	saberInfo_t* saber1;
	saberInfo_t* saber2;

	saber1 = BG_MySaber(pm->ps->clientNum, 0);
	saber2 = BG_MySaber(pm->ps->clientNum, 1);

	pm->cmd.upmove = 0;//no jump just yet

	if (saber1->type == SABER_GRIE4 || saber1->type == SABER_GRIE || saber1->type == SABER_PALP
		//&& saber2->type == SABER_GRIE4 || saber2->type == SABER_GRIE || saber2->type == SABER_PALP
		&& pm->ps->fd.saberAnimLevel == SS_DUAL)
	{
		return LS_GRIEVOUS_LUNGE;
	}
	else
	{
		return LS_JUMPATTACK_DUAL;
	}
}

saberMoveName_t PM_SaberLungeAttackMove(qboolean noSpecials)
{
	vec3_t fwdAngles, jumpFwd;

	saberInfo_t* saber1;
	saberInfo_t* saber2;

	saber1 = BG_MySaber(pm->ps->clientNum, 0);
	saber2 = BG_MySaber(pm->ps->clientNum, 1);
	//see if we have an overridden (or cancelled) lunge move
	if (saber1
		&& saber1->lungeAtkMove != LS_INVALID)
	{
		if (saber1->lungeAtkMove != LS_NONE)
		{
			return (saberMoveName_t)saber1->lungeAtkMove;
		}
	}
	if (saber2
		&& saber2->lungeAtkMove != LS_INVALID)
	{
		if (saber2->lungeAtkMove != LS_NONE)
		{
			return (saberMoveName_t)saber2->lungeAtkMove;
		}
	}
	//no overrides, cancelled?
	if (saber1
		&& saber1->lungeAtkMove == LS_NONE)
	{
		return LS_A_T2B;//LS_NONE;
	}
	if (saber2
		&& saber2->lungeAtkMove == LS_NONE)
	{
		return LS_A_T2B;//LS_NONE;
	}
	//just do it
	saber1 = BG_MySaber(pm->ps->clientNum, 0);

	if (pm->ps->fd.saberAnimLevel == SS_FAST)
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);
		return LS_PULL_ATTACK_STAB;
	}
	else if (pm->ps->fd.saberAnimLevel == SS_MEDIUM)
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);
		return LS_A_LUNGE;
	}
	else if (pm->ps->fd.saberAnimLevel == SS_STRONG)
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);
		if (saber1->type == SABER_YODA || saber1->type == SABER_PALP)
		{
			return LS_A_JUMP_PALP_;
		}
		else
		{
			return LS_A_JUMP_T__B_;
		}
	}
	else if (pm->ps->fd.saberAnimLevel == SS_DESANN)
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);
		if (saber1->type == SABER_YODA || saber1->type == SABER_PALP)
		{
			return LS_A_JUMP_PALP_;
		}
		else
		{
			return LS_A_JUMP_T__B_;
		}
	}
	else if (pm->ps->fd.saberAnimLevel == SS_TAVION)
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);
		return LS_A_LUNGE;
	}
	else if (pm->ps->fd.saberAnimLevel == SS_DUAL)
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);
		if (saber1->type == SABER_GRIE)
		{
			return LS_GRIEVOUS_SPECIAL;
		}
		else if (saber1->type == SABER_GRIE4)
		{
			return LS_SPINATTACK_GRIEV;
		}
		else
		{
			return LS_SPINATTACK_DUAL;
		}
	}
	else if (!noSpecials && pm->ps->fd.saberAnimLevel == SS_STAFF)
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);
		return LS_SPINATTACK;
	}
	else if (!noSpecials && pm->ps->fd.saberAnimLevel == SS_STAFF && saber1->numBlades < 2)
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);
		return LS_A_LUNGE;
	}
	else if (!noSpecials && (!saber2 || saber2->numBlades == 0))
	{
		VectorCopy(pm->ps->viewangles, fwdAngles);
		fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
		//do the lunge
		AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
		VectorScale(jumpFwd, 150, pm->ps->velocity);
		PM_AddEvent(EV_JUMP);
		return LS_A_LUNGE;
	}
	else if (!noSpecials)
	{
		if (saber1->type == SABER_GRIE)
		{
			return LS_GRIEVOUS_SPECIAL;
		}
		else if (saber1->type == SABER_GRIE4)
		{
			return LS_SPINATTACK_GRIEV;
		}
		else
		{
			return LS_SPINATTACK_DUAL;
		}
	}
	return LS_A_T2B;
}

#ifdef _GAME
#define JM_BACK_STAB_DISTANCE 256

qboolean PM_JMCanBackstab(void)
{
	trace_t tr;
	vec3_t flatAng;
	vec3_t fwd, back;
	vec3_t trmins = { -15, -15, -8 };
	vec3_t trmaxs = { 15, 15, 8 };

	VectorCopy(pm->ps->viewangles, flatAng);
	flatAng[PITCH] = 0;

	AngleVectors(flatAng, fwd, 0, 0);

	back[0] = pm->ps->origin[0] - fwd[0] * JM_BACK_STAB_DISTANCE;
	back[1] = pm->ps->origin[1] - fwd[1] * JM_BACK_STAB_DISTANCE;
	back[2] = pm->ps->origin[2] - fwd[2] * JM_BACK_STAB_DISTANCE;

	pm->trace(&tr, pm->ps->origin, trmins, trmaxs, back, pm->ps->clientNum, MASK_PLAYERSOLID);

	if (tr.fraction != 1.0 && tr.entityNum >= 0 && (tr.entityNum < MAX_CLIENTS))
	{ //We don't have real entity access here so we can't do an indepth check. But if it's a client and it's behind us, I guess that's reason enough to stab backward
		return qtrue;
	}

	return qfalse;
}


#define LUNGE_DISTANCE 128

qboolean PM_CanLunge(void)
{//AotC AI check for lunge attacks.
 //gentity_t *ent;
	trace_t tr;
	vec3_t flatAng;
	vec3_t fwd, back;
	vec3_t trmins = { -15, -15, -8 };
	vec3_t trmaxs = { 15, 15, 8 };

	VectorCopy(pm->ps->viewangles, flatAng);
	flatAng[PITCH] = 0;

	AngleVectors(flatAng, fwd, 0, 0);

	back[0] = pm->ps->origin[0] + fwd[0] * LUNGE_DISTANCE;
	back[1] = pm->ps->origin[1] + fwd[1] * LUNGE_DISTANCE;
	back[2] = pm->ps->origin[2] + fwd[2] * LUNGE_DISTANCE;

	pm->trace(&tr, pm->ps->origin, trmins, trmaxs, back, pm->ps->clientNum, MASK_PLAYERSOLID);

	if (tr.fraction != 1.0 && tr.entityNum >= 0 && (tr.entityNum < MAX_CLIENTS))
	{ //We don't have real entity access here so we can't do an indepth check. But if it's a client and it's behind us, I guess that's reason enough to stab backward
		return qtrue;
	}

	return qfalse;
}


#define JM_LUNGE_DISTANCE 512

qboolean PM_JMCanLunge(void)
{
	trace_t tr;
	vec3_t flatAng;
	vec3_t fwd, back;
	vec3_t trmins = { -15, -15, -8 };
	vec3_t trmaxs = { 15, 15, 8 };

	VectorCopy(pm->ps->viewangles, flatAng);
	flatAng[PITCH] = 0;

	AngleVectors(flatAng, fwd, 0, 0);

	back[0] = pm->ps->origin[0] + fwd[0] * JM_LUNGE_DISTANCE;
	back[1] = pm->ps->origin[1] + fwd[1] * JM_LUNGE_DISTANCE;
	back[2] = pm->ps->origin[2] + fwd[2] * JM_LUNGE_DISTANCE;

	pm->trace(&tr, pm->ps->origin, trmins, trmaxs, back, pm->ps->clientNum, MASK_PLAYERSOLID);

	if (tr.fraction != 1.0 && tr.entityNum >= 0 && tr.entityNum < MAX_CLIENTS)
	{ //We don't have real entity access here so we can't do an indepth check. But if it's a client and it's behind us, I guess that's reason enough to stab backward
		return qtrue;
	}

	return qfalse;
}
#endif

saberMoveName_t PM_SaberJumpAttackMove2(void)
{

	saberInfo_t* saber1;
	saberInfo_t* saber2;

	saber1 = BG_MySaber(pm->ps->clientNum, 0);
	saber2 = BG_MySaber(pm->ps->clientNum, 1);
	//see if we have an overridden (or cancelled) lunge move
	if (saber1
		&& saber1->jumpAtkFwdMove != LS_INVALID)
	{
		if (saber1->jumpAtkFwdMove != LS_NONE)
		{
			return (saberMoveName_t)saber1->jumpAtkFwdMove;
		}
	}
	if (saber2
		&& saber2->jumpAtkFwdMove != LS_INVALID)
	{
		if (saber2->jumpAtkFwdMove != LS_NONE)
		{
			return (saberMoveName_t)saber2->jumpAtkFwdMove;
		}
	}
	//no overrides, cancelled?
	if (saber1
		&& saber1->jumpAtkFwdMove == LS_NONE)
	{
		return LS_A_T2B;//LS_NONE;
	}
	if (saber2
		&& saber2->jumpAtkFwdMove == LS_NONE)
	{
		return LS_A_T2B;//LS_NONE;
	}
	//just do it
	if (pm->ps->fd.saberAnimLevel == SS_DUAL)
	{
		return PM_SaberDualJumpAttackMove();
	}
	else
	{
		if (PM_irand_timesync(0, 1))
		{
			return LS_JUMPATTACK_STAFF_LEFT;
		}
		else
		{
			return LS_JUMPATTACK_STAFF_RIGHT;
		}
	}
}

saberMoveName_t PM_SaberJumpAttackMove(void)
{
	vec3_t fwdAngles, jumpFwd;

	saberInfo_t* saber1;
	saberInfo_t* saber2;

	saber1 = BG_MySaber(pm->ps->clientNum, 0);
	saber2 = BG_MySaber(pm->ps->clientNum, 1);

	//see if we have an overridden (or cancelled) lunge move
	if (saber1
		&& saber1->jumpAtkFwdMove != LS_INVALID)
	{
		if (saber1->jumpAtkFwdMove != LS_NONE)
		{
			return (saberMoveName_t)saber1->jumpAtkFwdMove;
		}
	}
	if (saber2
		&& saber2->jumpAtkFwdMove != LS_INVALID)
	{
		if (saber2->jumpAtkFwdMove != LS_NONE)
		{
			return (saberMoveName_t)saber2->jumpAtkFwdMove;
		}
	}
	//no overrides, cancelled?
	if (saber1
		&& saber1->jumpAtkFwdMove == LS_NONE)
	{
		return LS_A_T2B;//LS_NONE;
	}
	if (saber2
		&& saber2->jumpAtkFwdMove == LS_NONE)
	{
		return LS_A_T2B;//LS_NONE;
	}
	//just do it
	VectorCopy(pm->ps->viewangles, fwdAngles);
	fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
	AngleVectors(fwdAngles, jumpFwd, NULL, NULL);
	VectorScale(jumpFwd, 300, pm->ps->velocity);
	pm->ps->velocity[2] = 280;
	PM_SetForceJumpZStart(pm->ps->origin[2]);//so we don't take damage if we land at same height

	PM_AddEvent(EV_JUMP);
	pm->ps->fd.forceJumpSound = 1;
	pm->cmd.upmove = 0;
	saber1 = BG_MySaber(pm->ps->clientNum, 0);

	if (saber1->type == SABER_YODA || saber1->type == SABER_PALP)
	{
		return LS_A_JUMP_PALP_;
	}
	else
	{
		return LS_A_JUMP_T__B_;
	}
}

float PM_GroundDistance(void)
{
	trace_t tr;
	vec3_t down;

	VectorCopy(pm->ps->origin, down);

	down[2] -= 4096;

	pm->trace(&tr, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, MASK_SOLID);

	VectorSubtract(pm->ps->origin, tr.endpos, down);

	return VectorLength(down);
}

float PM_WalkableGroundDistance(void)
{
	trace_t tr;
	vec3_t down;

	VectorCopy(pm->ps->origin, down);

	down[2] -= 4096;

	pm->trace(&tr, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, MASK_SOLID);

	if (tr.plane.normal[2] < MIN_WALK_NORMAL)
	{//can't stand on this plane
		return 4096;
	}

	VectorSubtract(pm->ps->origin, tr.endpos, down);

	return VectorLength(down);
}

qboolean PM_SaberInTransitionAny(int move);
static qboolean PM_CanDoDualDoubleAttacks(void)
{
	if (pm->ps->weapon == WP_SABER)
	{
		saberInfo_t* saber = BG_MySaber(pm->ps->clientNum, 0);
		if (saber
			&& (saber->saberFlags& SFL_NO_MIRROR_ATTACKS))
		{
			return qfalse;
		}
		saber = BG_MySaber(pm->ps->clientNum, 1);
		if (saber
			&& (saber->saberFlags& SFL_NO_MIRROR_ATTACKS))
		{
			return qfalse;
		}
	}
	if (PM_SaberInSpecialAttack(pm->ps->torsoAnim) ||
		PM_SaberInSpecialAttack(pm->ps->legsAnim))
	{
		return qfalse;
	}
	return qtrue;
}

static qboolean PM_CheckEnemyPresence(int dir, float radius)
{ //anyone in this dir?
	vec3_t angles;
	vec3_t checkDir = { 0.0f };
	vec3_t tTo;
	vec3_t tMins, tMaxs;
	trace_t tr;
	const float tSize = 12.0f;
	//sp uses a bbox ent list check, but.. that's not so easy/fast to
	//do in predicted code. So I'll just do a single box trace in the proper direction,
	//and take whatever is first hit.

	VectorSet(tMins, -tSize, -tSize, -tSize);
	VectorSet(tMaxs, tSize, tSize, tSize);

	VectorCopy(pm->ps->viewangles, angles);
	angles[PITCH] = 0.0f;

	switch (dir)
	{
	case DIR_RIGHT:
		AngleVectors(angles, NULL, checkDir, NULL);
		break;
	case DIR_LEFT:
		AngleVectors(angles, NULL, checkDir, NULL);
		VectorScale(checkDir, -1, checkDir);
		break;
	case DIR_FRONT:
		AngleVectors(angles, checkDir, NULL, NULL);
		break;
	case DIR_BACK:
		AngleVectors(angles, checkDir, NULL, NULL);
		VectorScale(checkDir, -1, checkDir);
		break;
	}

	VectorMA(pm->ps->origin, radius, checkDir, tTo);
	pm->trace(&tr, pm->ps->origin, tMins, tMaxs, tTo, pm->ps->clientNum, MASK_PLAYERSOLID);

	if (tr.fraction != 1.0f && tr.entityNum < ENTITYNUM_WORLD)
	{ //let's see who we hit
		bgEntity_t* bgEnt = PM_BGEntForNum(tr.entityNum);

		if (bgEnt &&
			(bgEnt->s.eType == ET_PLAYER || bgEnt->s.eType == ET_NPC))
		{ //this guy can be considered an "enemy"... if he is on the same team, oh well. can't bg-check that (without a whole lot of hassle).
			return qtrue;
		}
	}

	//no one in the trace
	return qfalse;
}

#define SABER_ALT_ATTACK_POWER		50//75?
#define SABER_ALT_ATTACK_POWER_LR	FATIGUE_CARTWHEEL
#define SABER_ALT_ATTACK_POWER_FB	25//30/50?
#define SABER_ALT_ATTACK_POWER_LRA	FATIGUE_CARTWHEEL_ATARU

saberMoveName_t PM_CheckPullAttack(void)
{//add pull attack swing,

	if (!(pm->cmd.buttons & BUTTON_ATTACK))
	{
		return LS_NONE;
	}

	if ((pm->ps->saberMove == LS_READY || PM_SaberInReturn(pm->ps->saberMove) || PM_SaberInReflect(pm->ps->saberMove))//ready
		&& pm->ps->groundEntityNum != ENTITYNUM_NONE
		&& pm->ps->fd.saberAnimLevel >= SS_FAST
		&& pm->ps->fd.saberAnimLevel <= SS_STRONG
		&& pm->ps->powerups[PW_DISINT_4] > pm->cmd.serverTime
		&& !(pm->ps->fd.forcePowersActive & (1 << FP_PULL))
		&& pm->ps->powerups[PW_PULL] > pm->cmd.serverTime
		&& (pm->cmd.buttons & BUTTON_ATTACK)
		&& BG_EnoughForcePowerForMove(FATIGUE_JUMPATTACK))
	{
		qboolean doMove = qtrue;

		saberMoveName_t pullAttackMove = LS_NONE;
		if (pm->ps->fd.saberAnimLevel == SS_FAST)
		{
			pullAttackMove = LS_PULL_ATTACK_STAB;
		}
		else
		{
			pullAttackMove = LS_PULL_ATTACK_SWING;
		}

		if (doMove)
		{
			WP_ForcePowerDrain(pm->ps, FP_PULL, FATIGUE_JUMPATTACK);
			return pullAttackMove;
		}
	}
	return LS_NONE;
}

qboolean PM_InSecondaryStyle(void)
{
	if (pm->ps->fd.saberAnimLevelBase == SS_STAFF
		|| pm->ps->fd.saberAnimLevelBase == SS_DUAL)
	{
		if (pm->ps->fd.saberAnimLevel != pm->ps->fd.saberAnimLevelBase)
		{
			return qtrue;
		}
	}
	return qfalse;
}

#ifdef _GAME
int nextcheck[MAX_CLIENTS]; // Next special move check.
#endif

saberMoveName_t PM_CheckPlayerAttackFromParry(int curmove)
{
	if (curmove >= LS_PARRY_UP && curmove <= LS_REFLECT_LL)
	{//in a parry
		switch (saberMoveData[curmove].endQuad)
		{
		case Q_T:
			return LS_A_T2B;
			break;
		case Q_TR:
			return LS_A_TR2BL;
			break;
		case Q_TL:
			return LS_A_TL2BR;
			break;
		case Q_BR:
			return LS_A_BR2TL;
			break;
		case Q_BL:
			return LS_A_BL2TR;
			break;
		}
	}
	return LS_NONE;
}

saberMoveName_t PM_SaberAttackForMovement(saberMoveName_t curmove)
{
	saberMoveName_t newmove = LS_NONE;
	qboolean noSpecials = PM_InSecondaryStyle() || PM_InCartwheel(pm->ps->legsAnim);
	qboolean allowCartwheels = qtrue;
	saberMoveName_t overrideJumpRightAttackMove = LS_INVALID;
	saberMoveName_t overrideJumpLeftAttackMove = LS_INVALID;
	saberInfo_t* saber1;
	saberInfo_t* saber2;

	saber1 = BG_MySaber(pm->ps->clientNum, 0);
	saber2 = BG_MySaber(pm->ps->clientNum, 1);

	if (pm->ps->weapon == WP_SABER)
	{
		if (saber1
			&& saber1->jumpAtkRightMove != LS_INVALID)
		{
			if (saber1->jumpAtkRightMove != LS_NONE)
			{//actually overriding
				overrideJumpRightAttackMove = (saberMoveName_t)saber1->jumpAtkRightMove;
			}
			else if (saber2
				&& saber2->jumpAtkRightMove > LS_NONE)
			{//would be cancelling it, but check the second saber, too
				overrideJumpRightAttackMove = (saberMoveName_t)saber2->jumpAtkRightMove;
			}
			else
			{//nope, just cancel it
				overrideJumpRightAttackMove = LS_NONE;
			}
		}
		else if (saber2
			&& saber2->jumpAtkRightMove != LS_INVALID)
		{//first saber not overridden, check second
			overrideJumpRightAttackMove = (saberMoveName_t)saber2->jumpAtkRightMove;
		}

		if (saber1
			&& saber1->jumpAtkLeftMove != LS_INVALID)
		{
			if (saber1->jumpAtkLeftMove != LS_NONE)
			{//actually overriding
				overrideJumpLeftAttackMove = (saberMoveName_t)saber1->jumpAtkLeftMove;
			}
			else if (saber2
				&& saber2->jumpAtkLeftMove > LS_NONE)
			{//would be cancelling it, but check the second saber, too
				overrideJumpLeftAttackMove = (saberMoveName_t)saber2->jumpAtkLeftMove;
			}
			else
			{//nope, just cancel it
				overrideJumpLeftAttackMove = LS_NONE;
			}
		}
		else if (saber2
			&& saber2->jumpAtkLeftMove != LS_INVALID)
		{//first saber not overridden, check second
			overrideJumpLeftAttackMove = (saberMoveName_t)saber1->jumpAtkLeftMove;
		}

		if (saber1
			&& (saber1->saberFlags & SFL_NO_CARTWHEELS))
		{
			allowCartwheels = qfalse;
		}
		if (saber2
			&& (saber2->saberFlags & SFL_NO_CARTWHEELS))
		{
			allowCartwheels = qfalse;
		}
	}

	if (pm->cmd.rightmove > 0)
	{//moving right
		if (!noSpecials
			&& overrideJumpRightAttackMove != LS_NONE
			&& pm->ps->velocity[2] > 20.0f //pm->ps->groundEntityNum != ENTITYNUM_NONE//on ground
			&& ((pm->cmd.buttons & BUTTON_ATTACK) && !(pm->cmd.buttons & BUTTON_BLOCK))//hitting attack
			&& PM_GroundDistance() < 70.0f //not too high above ground
			&& (pm->cmd.upmove > 0 || (pm->ps->pm_flags & PMF_JUMP_HELD))//focus-holding player
			&& BG_EnoughForcePowerForMove(SABER_ALT_ATTACK_POWER_LR))//have enough power
		{//cartwheel right
			WP_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER_LR);
			if (overrideJumpRightAttackMove != LS_INVALID)
			{//overridden with another move
				return overrideJumpRightAttackMove;
			}
			else
			{
				vec3_t right, fwdAngles;

				VectorSet(fwdAngles, 0.0f, pm->ps->viewangles[YAW], 0.0f);

				AngleVectors(fwdAngles, NULL, right, NULL);
				pm->ps->velocity[0] = pm->ps->velocity[1] = 0.0f;
				VectorMA(pm->ps->velocity, 190.0f, right, pm->ps->velocity);

				if (pm->ps->fd.saberAnimLevel == SS_STAFF)
				{
					newmove = LS_BUTTERFLY_RIGHT;
					pm->ps->velocity[2] = 350.0f;
				}
				else if (allowCartwheels)
				{
					PM_AddEvent(EV_JUMP);
					pm->ps->velocity[2] = 300.0f;
					if (1)
					{
						newmove = LS_JUMPATTACK_ARIAL_RIGHT;
					}
					else
					{
						newmove = LS_JUMPATTACK_CART_RIGHT;
					}
				}
			}
		}
		else if (pm->cmd.forwardmove > 0)
		{//forward right = TL2BR slash
			newmove = LS_A_TL2BR;
		}
		else if (pm->cmd.forwardmove < 0)
		{//backward right = BL2TR uppercut
			newmove = LS_A_BL2TR;
		}
		else
		{//just right is a left slice
			newmove = LS_A_L2R;
		}
	}
	else if (pm->cmd.rightmove < 0)
	{//moving left
		if (!noSpecials
			&& overrideJumpLeftAttackMove != LS_NONE
			&& pm->ps->velocity[2] > 20.0f //pm->ps->groundEntityNum != ENTITYNUM_NONE//on ground
			&& ((pm->cmd.buttons& BUTTON_ATTACK) && !(pm->cmd.buttons& BUTTON_BLOCK))//hitting attack
			&& PM_GroundDistance() < 70.0f //not too high above ground
			&& (pm->cmd.upmove > 0 || (pm->ps->pm_flags & PMF_JUMP_HELD))//focus-holding player
			&& BG_EnoughForcePowerForMove(SABER_ALT_ATTACK_POWER_LR))//have enough power
		{//cartwheel left
			WP_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER_LR);

			if (overrideJumpLeftAttackMove != LS_INVALID)
			{//overridden with another move
				return overrideJumpLeftAttackMove;
			}
			else
			{
				vec3_t right, fwdAngles;

				VectorSet(fwdAngles, 0.0f, pm->ps->viewangles[YAW], 0.0f);
				AngleVectors(fwdAngles, NULL, right, NULL);
				pm->ps->velocity[0] = pm->ps->velocity[1] = 0.0f;
				VectorMA(pm->ps->velocity, -190.0f, right, pm->ps->velocity);
				if (pm->ps->fd.saberAnimLevel == SS_STAFF)
				{
					newmove = LS_BUTTERFLY_LEFT;
					pm->ps->velocity[2] = 250.0f;
				}
				else if (allowCartwheels)
				{
					PM_AddEvent(EV_JUMP);
					pm->ps->velocity[2] = 350.0f;

					if (1)
					{
						newmove = LS_JUMPATTACK_ARIAL_LEFT;
					}
					else
					{
						newmove = LS_JUMPATTACK_CART_LEFT;
					}
				}
			}
		}
		else if (pm->cmd.forwardmove > 0)
		{//forward left = TR2BL slash
			newmove = LS_A_TR2BL;
		}
		else if (pm->cmd.forwardmove < 0)
		{//backward left = BR2TL uppercut
			newmove = LS_A_BR2TL;
		}
		else
		{//just left is a right slice
			newmove = LS_A_R2L;
		}
	}
	else
	{//not moving left or right
		if (pm->cmd.forwardmove > 0)
		{//forward= T2B slash
			if (!noSpecials &&
				(pm->ps->fd.saberAnimLevel == SS_DUAL || pm->ps->fd.saberAnimLevel == SS_STAFF) &&
				pm->ps->fd.forceRageRecoveryTime < pm->cmd.serverTime &&
				(pm->ps->groundEntityNum != ENTITYNUM_NONE || PM_GroundDistance() <= 40) &&
				pm->ps->velocity[2] >= 0 &&
				(pm->cmd.upmove > 0 || pm->ps->pm_flags & PMF_JUMP_HELD) &&
				!PM_SaberInTransitionAny(pm->ps->saberMove) &&
				!PM_SaberInAttack(pm->ps->saberMove) &&
				pm->ps->weaponTime <= 0 &&
				pm->ps->forceHandExtend == HANDEXTEND_NONE &&
				((pm->cmd.buttons& BUTTON_ATTACK) && !(pm->cmd.buttons& BUTTON_BLOCK)) &&
				BG_EnoughForcePowerForMove(FATIGUE_JUMPATTACK))
			{ //DUAL/STAFF JUMP ATTACK
				newmove = PM_SaberJumpAttackMove2();
				if (newmove != LS_A_T2B
					&& newmove != LS_NONE)
				{
					WP_ForcePowerDrain(pm->ps, FP_GRIP, FATIGUE_JUMPATTACK);
				}
			}
			else if (!noSpecials &&
				(pm->ps->fd.saberAnimLevel == SS_FAST
					|| pm->ps->fd.saberAnimLevel == SS_MEDIUM
					|| pm->ps->fd.saberAnimLevel == SS_TAVION) &&
				pm->ps->velocity[2] > 100 &&
				PM_GroundDistance() < 32 &&
				!PM_InSpecialJump(pm->ps->legsAnim) &&
				!PM_SaberInSpecialAttack(pm->ps->torsoAnim) &&
				BG_EnoughForcePowerForMove(FATIGUE_JUMPATTACK))
			{ //FLIP AND DOWNWARD ATTACK
				newmove = PM_SaberFlipOverAttackMove();
				if (newmove != LS_A_T2B && newmove != LS_NONE)
				{
					WP_ForcePowerDrain(pm->ps, FP_GRIP, FATIGUE_JUMPATTACK);
				}
			}
			else if (!noSpecials &&
				(pm->ps->fd.saberAnimLevel == SS_STRONG
					|| pm->ps->fd.saberAnimLevel == SS_DESANN) &&
				pm->ps->velocity[2] > 100 &&
				PM_GroundDistance() < 32 &&
				!PM_InSpecialJump(pm->ps->legsAnim) &&
				!PM_SaberInSpecialAttack(pm->ps->torsoAnim) &&
				BG_EnoughForcePowerForMove(FATIGUE_JUMPATTACK))
			{ //DFA
				newmove = PM_SaberJumpAttackMove();
				if (newmove != LS_A_T2B
					&& newmove != LS_NONE)
				{
					WP_ForcePowerDrain(pm->ps, FP_GRIP, FATIGUE_JUMPATTACK);
				}
			}
			else if (pm->ps->groundEntityNum != ENTITYNUM_NONE &&
				(pm->ps->pm_flags & PMF_DUCKED) &&
				pm->ps->weaponTime <= 0 &&
				!PM_SaberInSpecialAttack(pm->ps->torsoAnim) &&
				BG_EnoughForcePowerForMove(FATIGUE_GROUNDATTACK))
			{ //LUNGE (weak)
				newmove = PM_SaberLungeAttackMove(noSpecials);
				if (newmove != LS_A_T2B
					&& newmove != LS_NONE)
				{
					WP_ForcePowerDrain(pm->ps, FP_GRIP, FATIGUE_GROUNDATTACK);
				}
			}
			else if (!noSpecials)
			{
				saberMoveName_t stabDownMove = PM_CheckStabDown();
				if (stabDownMove != LS_NONE
					&& BG_EnoughForcePowerForMove(FATIGUE_GROUNDATTACK))
				{
					newmove = stabDownMove;
					WP_ForcePowerDrain(pm->ps, FP_GRIP, FATIGUE_GROUNDATTACK);
				}
				else
				{
					newmove = LS_A_T2B;
				}
			}
		}
		else if (pm->cmd.forwardmove < 0)
		{//backward= T2B slash//B2T uppercut?
			if (!noSpecials &&
				pm->ps->fd.saberAnimLevel == SS_STAFF &&
				pm->ps->fd.forceRageRecoveryTime < pm->cmd.serverTime &&
				pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 &&
				(pm->ps->groundEntityNum != ENTITYNUM_NONE || PM_GroundDistance() <= 40) &&
				pm->ps->velocity[2] >= 0 &&
				(pm->cmd.upmove > 0 || pm->ps->pm_flags & PMF_JUMP_HELD) &&
				!PM_SaberInTransitionAny(pm->ps->saberMove) &&
				!PM_SaberInAttack(pm->ps->saberMove) &&
				pm->ps->weaponTime <= 0 &&
				pm->ps->forceHandExtend == HANDEXTEND_NONE &&
				(pm->cmd.buttons & BUTTON_ATTACK))
			{ //BACKFLIP ATTACK
				newmove = PM_SaberBackflipAttackMove();
			}
			else if (PM_CanBackstab() && !PM_SaberInSpecialAttack(pm->ps->torsoAnim))
			{ //BACKSTAB (attack varies by level)
				if (pm->ps->fd.saberAnimLevel >= FORCE_LEVEL_2 && pm->ps->fd.saberAnimLevel != SS_STAFF)
				{//medium and higher attacks
					if ((pm->ps->pm_flags & PMF_DUCKED) || pm->cmd.upmove < 0)
					{
						newmove = LS_A_BACK_CR;
					}
					else
					{
						newmove = LS_A_BACK;
					}
				}
				else
				{ //weak attack
					if (saber1->type == SABER_BACKHAND)//saber backhand
					{
						newmove = LS_A_BACKSTAB_B;
					}
					else if (saber1->type == SABER_ASBACKHAND)//saber backhand
					{
						newmove = LS_A_BACKSTAB_B;
					}
					else
					{
						newmove = LS_A_BACKSTAB;
					}
				}
			}
			else
			{
				newmove = LS_A_T2B;
			}
		}
		else if (PM_SaberInParry(curmove) || PM_SaberInBrokenParry(curmove))
		{//parries, return to the start position if a direction isn't given.
			newmove = LS_READY;
		}
		else if (PM_SaberInBounce(curmove))
		{//bounces, parries, etc return to the start position if a direction isn't given.
			
#ifdef _GAME
			if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC && Q_irand(0, 3))
			{//use NPC random
				newmove = PM_NPCSaberAttackFromQuad(saberMoveData[curmove].endQuad);
			}
			else
#endif
			{
				newmove = saberMoveData[curmove].chain_attack;
			}
			if (PM_SaberKataDone(curmove, newmove))
			{
				return saberMoveData[curmove].chain_idle;
			}
			else
			{
				newmove = LS_READY;
			}
		}
		else if (PM_SaberInKnockaway(curmove))
		{//bounces should go to their default attack if you don't specify a direction but are attacking

#ifdef _GAME
			if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC && Q_irand(0, 3))
			{//use bot random
				newmove = PM_NPCSaberAttackFromQuad(saberMoveData[curmove].endQuad);
			}
			else
#endif
			{
				if (pm->ps->fd.saberAnimLevel == SS_FAST || pm->ps->fd.saberAnimLevel == SS_TAVION)
				{//player is in fast attacks, so come right back down from the same spot
					newmove = PM_AttackMoveForQuad(saberMoveData[curmove].endQuad);
				}
				else
				{//use a transition to wrap to another attack from a different dir
					newmove = saberMoveData[curmove].chain_attack;
				}
			}
			if (PM_SaberKataDone(curmove, newmove))
			{
				return saberMoveData[curmove].chain_idle;
			}
			else
			{
				newmove = LS_READY;
			}
		}
		else if (curmove == LS_A_FLIP_STAB
			|| curmove == LS_A_FLIP_SLASH
			|| (curmove >= LS_PARRY_UP
			&& curmove <= LS_REFLECT_LL))
		{//Not moving at all, not too busy to attack
			 //checked all special attacks, if we're in a parry, attack from that move
			saberMoveName_t parryAttackMove = PM_CheckPlayerAttackFromParry(curmove);

			if (parryAttackMove != LS_NONE)
			{
				return parryAttackMove;
			}
			//check regular attacks
			if (pm->ps->clientNum)
			{//auto-aim
			 /*if (pm->ent && pm->gent->enemy)
				{//based on enemy position, pick a proper attack
					saberMoveName_t autoMove = PM_AttackForEnemyPos(qtrue, (qboolean)(pm->ps->clientNum >= MAX_CLIENTS));
					if (autoMove != LS_INVALID)
					{
						return autoMove;
					}
				}
				else*/ if (fabs(pm->ps->viewangles[0]) > 30)
				{//looking far up or far down uses the top to bottom attack, presuming you want a vertical attack
					return LS_A_T2B;
				}
			}
			else
			{//for now, just pick a random attack
				return ((saberMoveName_t)Q_irand(LS_A_TL2BR, LS_A_T2B));
			}
		}
		else if (curmove == LS_READY)
		{
			newmove = LS_A_T2B; //decided we don't like random attacks when idle, use an overhead instead.
		}
	}

	if (pm->ps->fd.saberAnimLevel == SS_DUAL)
	{
		if ((newmove == LS_A_R2L || newmove == LS_S_R2L
			|| newmove == LS_A_L2R || newmove == LS_S_L2R)
			&& PM_CanDoDualDoubleAttacks()
			&& PM_CheckEnemyPresence(DIR_RIGHT, 100.0f)
			&& PM_CheckEnemyPresence(DIR_LEFT, 100.0f))
		{//enemy both on left and right
			newmove = LS_DUAL_LR;
			//probably already moved, but...
			pm->cmd.rightmove = 0;
		}
		else if ((newmove == LS_A_T2B || newmove == LS_S_T2B
			|| newmove == LS_A_BACK || newmove == LS_A_BACK_CR)
			&& PM_CanDoDualDoubleAttacks()
			&& PM_CheckEnemyPresence(DIR_FRONT, 100.0f)
			&& PM_CheckEnemyPresence(DIR_BACK, 100.0f))
		{//enemy both in front and back
			newmove = LS_DUAL_FB;
			//probably already moved, but...
			pm->cmd.forwardmove = 0;
		}
	}

#ifdef _GAME
	if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
	{// Some special bot stuff.
		if (nextcheck[pm->ps->clientNum] <= level.time && bot_thinklevel.integer >= 0)
		{
			int check_val = 0; // Times 500 for next check interval.

			if (PM_JMCanBackstab())
			{ //Bot BACKSTAB (attack varies by level) (This one for JM and above bots!)
				if ((pm->ps->pm_flags & PMF_DUCKED) || pm->cmd.upmove < 0)
				{
					newmove = LS_A_BACK_CR;
				}
				else
				{
					int choice = rand() % 3;

					if (choice == 1)
					{
						newmove = LS_A_BACK;
					}
					else if (choice == 2)
					{
						newmove = PM_SaberBackflipAttackMove();
					}
					else if (choice == 3)
					{
						newmove = LS_A_BACKFLIP_ATK;
					}
					else
					{
						if (saber1->type == SABER_BACKHAND)//saber backhand
						{
							newmove = LS_A_BACKSTAB_B;
						}
						else if (saber1->type == SABER_ASBACKHAND)//saber backhand
						{
							newmove = LS_A_BACKSTAB_B;
						}
						else
						{
							newmove = LS_A_BACKSTAB;
						}
					}
				}
			}
			else if (PM_CanBackstab())
			{ //Bot BACKSTAB (attack varies by level)
				if ((pm->ps->pm_flags & PMF_DUCKED) || pm->cmd.upmove < 0)
				{
					newmove = LS_A_BACK_CR;
				}
				else
				{
					newmove = LS_A_BACK;
				}
			}
			else if (PM_JMCanLunge())
			{ //Bot Lunge (attack varies by level)  (This one for JM and above bots!)
				int choice = rand() % 12;

				if (choice == 1
					&& pm->ps->fd.saberAnimLevel == SS_DUAL)
				{
					newmove = PM_SaberDualJumpAttackMove();
				}
				else if (choice == 2
					&& pm->ps->fd.saberAnimLevel == SS_DUAL)
				{
					newmove = LS_SPINATTACK_DUAL;
				}
				else if (choice == 1)
				{
					newmove = LS_SPINATTACK_ALORA;
				}
				else if (choice == 2)
				{
					newmove = LS_A1_SPECIAL;
				}
				else if (choice == 3)
				{
					newmove = LS_A2_SPECIAL;
				}
				else if (choice == 4)
				{
					newmove = LS_A3_SPECIAL;
				}
				else if (choice == 5)
				{
					newmove = LS_SPINATTACK;
				}
				else if (choice == 6)
				{
					newmove = LS_BUTTERFLY_RIGHT;
				}
				else if (choice == 7)
				{
					newmove = LS_BUTTERFLY_LEFT;
				}
				else if (choice == 8)
				{
					newmove = LS_A4_SPECIAL;
				}
				else if (choice == 9)
				{
					newmove = LS_A5_SPECIAL;
				}
				else if (choice == 10)
				{
					newmove = PM_SaberFlipOverAttackMove();
				}
				else if (choice == 11)
				{
					newmove = PM_SaberJumpAttackMove2();
				}
				else if (((pm->ps->pm_flags & PMF_DUCKED) || pm->cmd.upmove < 0)
					&& choice == 12) // Less often when close.
				{
					newmove = PM_SaberJumpAttackMove();
				}
				else
				{
					newmove = PM_SaberLungeAttackMove(qfalse);
				}
			}
			else if (PM_CanLunge())
			{ //Bot Lunge (attack varies by level)
				int choice = rand() % 4;

				if (choice == 1)
				{
					newmove = PM_SaberDualJumpAttackMove();
				}
				else if (choice == 2)
				{
					newmove = PM_SaberFlipOverAttackMove();
				}
				else if (choice == 3)
				{
					newmove = PM_SaberJumpAttackMove2();
				}
				else if (((pm->ps->pm_flags& PMF_DUCKED) || pm->cmd.upmove < 0)
					&& choice == 4) // Less often when close.
				{
					newmove = PM_SaberLungeAttackMove(qfalse);
				}
				else
				{
					newmove = PM_SaberJumpAttackMove();
				}
			}

			check_val = bot_thinklevel.integer;

			if (check_val <= 0)
				check_val = 1;

			nextcheck[pm->ps->clientNum] = level.time + (20000 / check_val);
		}
	}
#endif

	return newmove;
}

int PM_KickMoveForConditions(void)
{
	int kickMove = -1;

	if (pm->cmd.rightmove)
	{//kick to side
		if (pm->cmd.rightmove > 0)
		{//kick right
			kickMove = LS_KICK_R;
		}
		else
		{//kick left
			kickMove = LS_KICK_L;
		}
		pm->cmd.rightmove = 0;
	}
	else if (pm->cmd.forwardmove)
	{//kick front/back
		if (pm->cmd.forwardmove > 0)
		{//kick fwd
			if (pm->ps->groundEntityNum != ENTITYNUM_NONE && pm->ps->weapon == WP_SABER && PM_CheckEnemyPresence(DIR_FRONT, 64.0f))
			{
				kickMove = LS_HILT_BASH;
			}
			else
			{
				kickMove = LS_KICK_F;
			}
		}
		else
		{//kick back
			if (PM_CrouchingAnim(pm->ps->legsAnim))
			{
				kickMove = LS_KICK_B3;
			}
			else
			{
				kickMove = LS_KICK_B;
			}
		}
		pm->cmd.forwardmove = 0;
	}
	else
	{
		if (0)
		{ //ok, let's try some fancy kicks
			//qboolean is actually of type int anyway, but just for safeness.
			int front = (int)PM_CheckEnemyPresence(DIR_FRONT, 100.0f);
			int back = (int)PM_CheckEnemyPresence(DIR_BACK, 100.0f);
			int right = (int)PM_CheckEnemyPresence(DIR_RIGHT, 100.0f);
			int left = (int)PM_CheckEnemyPresence(DIR_LEFT, 100.0f);
			int numEnemy = front + back + right + left;

			if (numEnemy >= 3 ||
				((!right || !left) && numEnemy >= 2))
			{ //> 2 enemies near, or, >= 2 enemies near and they are not to the right and left.
				kickMove = LS_KICK_S;
			}
			else if (right&& left)
			{ //enemies on both sides
				kickMove = LS_KICK_RL;
			}
			else
			{ //oh well, just do a forward kick
				kickMove = LS_KICK_F;
			}

			pm->cmd.upmove = 0;
		}
	}

	return kickMove;
}

int PM_MeleeMoveForConditions(void)
{
	int kickMove = -1;

	if (pm->cmd.rightmove)
	{//kick to side
		if (pm->cmd.rightmove > 0)
		{//kick right
			kickMove = LS_SLAP_R;
		}
		else
		{//kick left
			kickMove = LS_SLAP_L;
		}
		pm->cmd.rightmove = 0;
	}
	else if (pm->cmd.forwardmove)
	{//kick front/back
		if (pm->cmd.forwardmove > 0)
		{//kick fwd
			if (pm->ps->groundEntityNum != ENTITYNUM_NONE && pm->ps->weapon == WP_SABER && PM_CheckEnemyPresence(DIR_FRONT, 64.0f))
			{
				kickMove = LS_HILT_BASH;
			}
			else
			{
				kickMove = LS_KICK_F2;
			}
		}
		else
		{//kick back
			if (PM_CrouchingAnim(pm->ps->legsAnim))
			{
				kickMove = LS_KICK_B3;
			}
			else
			{
				kickMove = LS_KICK_B2;
			}
		}
		pm->cmd.forwardmove = 0;
	}
	else
	{
		if (0)
		{ //ok, let's try some fancy kicks
		  //qboolean is actually of type int anyway, but just for safeness.
			int front = (int)PM_CheckEnemyPresence(DIR_FRONT, 100.0f);
			int back = (int)PM_CheckEnemyPresence(DIR_BACK, 100.0f);
			int right = (int)PM_CheckEnemyPresence(DIR_RIGHT, 100.0f);
			int left = (int)PM_CheckEnemyPresence(DIR_LEFT, 100.0f);
			int numEnemy = front + back + right + left;

			if (numEnemy >= 3 ||
				((!right || !left) && numEnemy >= 2))
			{ //> 2 enemies near, or, >= 2 enemies near and they are not to the right and left.
				kickMove = LS_KICK_S;
			}
			else if (right&& left)
			{ //enemies on both sides
				kickMove = LS_KICK_RL;
			}
			else
			{ //oh well, just do a forward kick
				kickMove = LS_KICK_F2;
			}

			pm->cmd.upmove = 0;
		}
	}

	return kickMove;
}

qboolean PM_InSlopeAnim(int anim);
qboolean PM_RunningAnim(int anim);

qboolean PM_SaberMoveOkayForKata(void)
{
	if (pm->ps->saberMove == LS_READY
		|| PM_SaberInStart(pm->ps->saberMove))
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

qboolean PM_CanDoKata(void)
{
	if (PM_InSecondaryStyle())
	{
		return qfalse;
	}

	if (!pm->ps->saberInFlight//not throwing saber
		&& PM_SaberMoveOkayForKata()
		&& !BG_SaberInKata(pm->ps->saberMove)
		&& !BG_InKataAnim(pm->ps->legsAnim)
		&& !BG_InKataAnim(pm->ps->torsoAnim)
		&& pm->ps->groundEntityNum != ENTITYNUM_NONE//not in the air
		&& (pm->cmd.buttons & BUTTON_ATTACK)//pressing attack
		&& (pm->cmd.buttons & BUTTON_ALT_ATTACK)//pressing alt attack
		&& !pm->cmd.forwardmove//not moving f/b
		&& !pm->cmd.rightmove//not moving r/l
		&& pm->cmd.upmove <= 0//not jumping...?
		&& BG_EnoughForcePowerForMove(SABER_ALT_ATTACK_POWER))// have enough power
	{//FIXME: check rage, etc...
		saberInfo_t* saber = BG_MySaber(pm->ps->clientNum, 0);
		if (saber
			&& saber->kataMove == LS_NONE)
		{//kata move has been overridden in a way that should stop you from doing it at all
			return qfalse;
		}
		saber = BG_MySaber(pm->ps->clientNum, 1);
		if (saber
			&& saber->kataMove == LS_NONE)
		{//kata move has been overridden in a way that should stop you from doing it at all
			return qfalse;
		}
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberThrowable(void)
{
	saberInfo_t* saber = BG_MySaber(pm->ps->clientNum, 0);
	if (!saber)
	{//this is bad, just drop out.
		return qfalse;
	}

	if (!(saber->saberFlags & SFL_NOT_THROWABLE))
	{//yes, this saber is always throwable
		return qtrue;
	}

	//saber is not normally throwable
	if ((saber->saberFlags& SFL_SINGLE_BLADE_THROWABLE))
	{//it is throwable if only one blade is on
		if (saber->numBlades > 1)
		{//it has more than one blade
			int i = 0;
			int numBladesActive = 0;
			for (; i < saber->numBlades; i++)
			{
				if (saber->blade[i].active)
				{
					numBladesActive++;
				}
			}
			if (numBladesActive == 1)
			{//only 1 blade is on
				return qtrue;
			}
		}
	}
	//nope, can't throw it
	return qfalse;
}

qboolean PM_CheckAltKickAttack(void)
{
	if ((pm->cmd.buttons& BUTTON_ALT_ATTACK || pm->cmd.buttons & BUTTON_KICK)
		&& (!PM_FlippingAnim(pm->ps->legsAnim) || pm->ps->legsTimer <= 250))
	{
		return qtrue;
	}
	return qfalse;
}

int bg_parryDebounce[NUM_FORCE_POWER_LEVELS] =
{
	500,//if don't even have defense, can't use defense!
	300,
	150,
	50
};

qboolean PM_SaberPowerCheck(void)
{
	if (pm->ps->saberInFlight)
	{ //so we don't keep doing stupid force out thing while guiding saber.
		if (pm->ps->fd.forcePower > forcePowerNeeded[pm->ps->fd.forcePowerLevel[FP_SABERTHROW]][FP_SABERTHROW])
		{
			return qtrue;
		}
	}
	else
	{
		return BG_EnoughForcePowerForMove(forcePowerNeeded[pm->ps->fd.forcePowerLevel[FP_SABERTHROW]][FP_SABERTHROW]);
	}

	return qfalse;
}

qboolean PM_CanDoRollStab(void)
{
	if (pm->ps->weapon == WP_SABER && !(pm->ps->userInt3 & (1 << FLAG_DODGEROLL)))
	{
		saberInfo_t* saber = BG_MySaber(pm->ps->clientNum, 0);
		if (saber
			&& (saber->saberFlags& SFL_NO_ROLL_STAB))
		{
			return qfalse;
		}
		saber = BG_MySaber(pm->ps->clientNum, 1);
		if (saber
			&& (saber->saberFlags& SFL_NO_ROLL_STAB))
		{
			return qfalse;
		}
	}
	else
	{
		return qfalse;
	}
	return qtrue;
}
/*
=================
PM_WeaponLightsaber

Consults a chart to choose what to do with the lightsaber.
While this is a little different than the Quake 3 code, there is no clean way of using the Q3 code for this kind of thing.
=================
*/
qboolean InSaberDelayAnimation(int move)
{
	if ((move >= 665 && move <= 669)
		|| (move >= 690 && move <= 694)
		|| (move >= 715 && move <= 719))
	{
		return qtrue;
	}
	return qfalse;
}
// Ultimate goal is to set the sabermove to the proper next location
// Note that if the resultant animation is NONE, then the animation is essentially "idle", and is set in WP_TorsoAnim

//Sets your saber block position based on your current movement commands.
int PM_SetBlock(void)
{
	int anim = PM_ReadyPoseForSaberAnimLevel();

	saberInfo_t* saber1;

	saber1 = BG_MySaber(pm->ps->clientNum, 0);
	
	if (pm->ps->fd.blockPoints < BLOCKPOINTS_FAIL)
	{
		return qfalse;
	}

	if (PM_InKnockDown(pm->ps) || BG_InRoll(pm->ps, pm->ps->legsAnim))
	{//in knockdown
		return qfalse;
	}

	if (pm->ps->weaponTime > 0)
	{ //weapon busy
		return qfalse;
	}

	if (pm->cmd.upmove <= 0)
	{
		if (pm->cmd.forwardmove < 0)//this is the walking BACK BLOCK 
		{//Upper Top block          
			if (pm->cmd.rightmove < 0)
			{//upper left block
				pm->ps->saberBlocked = BLOCKED_UPPER_LEFT;
			}
			else if (pm->cmd.rightmove > 0)
			{//upper right block
				pm->ps->saberBlocked = BLOCKED_UPPER_RIGHT;
			}
			else if (pm->cmd.buttons & BUTTON_ATTACK)
			{
				pm->ps->saberBlocked = BLOCKED_TOP;
			}
			else
			{//Top Block
				pm->ps->saberBlocked = BLOCKED_TOP;
			}
		}
		else if (pm->cmd.forwardmove > 0)//Walking Forward BLOCK 
		{
			if (pm->cmd.rightmove < 0)
			{//lower left block
				pm->ps->saberBlocked = BLOCKED_LOWER_LEFT;
			}
			else if (pm->cmd.rightmove > 0)
			{//lower right block
				pm->ps->saberBlocked = BLOCKED_LOWER_RIGHT;
			}
			else
			{
				if (pm->ps->fd.saberAnimLevel == SS_MEDIUM)
				{
#ifdef _GAME
					if (!(g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC))
					{
						anim = PM_ReadyPoseForSaberAnimLevel();

					}
					else
					{
						anim = PM_ReadyPoseForSaberAnimLevelBOT();
					}
#endif				
				}
				else
				{
					pm->ps->saberBlocked = BLOCKED_TOP;
				}
			}
		}
		else
		{//standing still block
			if (pm->cmd.rightmove < 0)//left block
			{
				pm->ps->saberBlocked = BLOCKED_UPPER_LEFT;
			}
			else if (pm->cmd.rightmove > 0)//right block
			{
				pm->ps->saberBlocked = BLOCKED_UPPER_RIGHT;
			}
			else if (pm->cmd.buttons & BUTTON_ATTACK)
			{
				pm->ps->saberBlocked = BLOCKED_TOP;
			}
			else
			{
#ifdef _GAME
				if (!(g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC))
				{
					anim = PM_ReadyPoseForSaberAnimLevel();
				}
				else
				{
					anim = PM_ReadyPoseForSaberAnimLevelBOT();
				}
#endif
			}
		}
	}
	return anim;
}

void PM_SetMeleeBlock(void)
{
	int anim = -1;

	if (pm->ps->weapon != WP_MELEE)
	{
		return;
	}

	if (!BG_KickMove(pm->ps->saberMove)//not already in a kick
		&& !(pm->ps->pm_flags & PMF_DUCKED)//not ducked
		&& (pm->cmd.upmove >= 0))//not trying to duck
	{
		if (pm->cmd.rightmove || pm->cmd.forwardmove)  //pushing a direction
		{
			if (pm->cmd.rightmove > 0)
			{//lean right
				if (pm->cmd.forwardmove > 0)
				{//lean forward right
					if (pm->ps->torsoAnim == MELEE_STANCE_HOLD_RT)
					{
						anim = pm->ps->torsoAnim;
					}
					else
					{
						anim = MELEE_STANCE_RT;
					}
					pm->cmd.rightmove = 0;
					pm->cmd.forwardmove = 0;
				}
				else if (pm->cmd.forwardmove < 0)
				{//lean backward right
					if (pm->ps->torsoAnim == MELEE_STANCE_HOLD_BR)
					{
						anim = pm->ps->torsoAnim;
					}
					else
					{
						anim = MELEE_STANCE_BR;
					}
					pm->cmd.rightmove = 0;
					pm->cmd.forwardmove = 0;
				}
				else
				{//lean right
					if (pm->ps->torsoAnim == MELEE_STANCE_HOLD_RT)
					{
						anim = pm->ps->torsoAnim;
					}
					else
					{
						anim = MELEE_STANCE_RT;
					}
				}
				pm->cmd.rightmove = 0;
				pm->cmd.forwardmove = 0;
			}
			else if (pm->cmd.rightmove < 0)
			{//lean left
				if (pm->cmd.forwardmove > 0)
				{//lean forward left
					if (pm->ps->torsoAnim == MELEE_STANCE_HOLD_LT)
					{
						anim = pm->ps->torsoAnim;
					}
					else
					{
						anim = MELEE_STANCE_LT;
					}
					pm->cmd.rightmove = 0;
					pm->cmd.forwardmove = 0;
				}
				else if (pm->cmd.forwardmove < 0)
				{//lean backward left
					if (pm->ps->torsoAnim == MELEE_STANCE_HOLD_BL)
					{
						anim = pm->ps->torsoAnim;
					}
					else
					{
						anim = MELEE_STANCE_BL;
					}
					pm->cmd.rightmove = 0;
					pm->cmd.forwardmove = 0;
				}
				else
				{//lean left
					if (pm->ps->torsoAnim == MELEE_STANCE_HOLD_LT)
					{
						anim = pm->ps->torsoAnim;
					}
					else
					{
						anim = MELEE_STANCE_LT;
					}
				}
				pm->cmd.rightmove = 0;
				pm->cmd.forwardmove = 0;
			}
			else
			{//not pressing either side
				if (pm->cmd.forwardmove > 0)
				{//lean forward
					if (PM_MeleeblockAnim(pm->ps->torsoAnim))
					{
						anim = pm->ps->torsoAnim;
					}
					else if (Q_irand(0, 1))
					{
						anim = MELEE_STANCE_T;
					}
					else
					{
						anim = MELEE_STANCE_T;
					}
					pm->cmd.rightmove = 0;
					pm->cmd.forwardmove = 0;
				}
				else if (pm->cmd.forwardmove < 0)
				{//lean backward
					if (PM_MeleeblockAnim(pm->ps->torsoAnim))
					{
						anim = pm->ps->torsoAnim;
					}
					else if (Q_irand(0, 1))
					{
						anim = MELEE_STANCE_B;
					}
					else
					{
						anim = MELEE_STANCE_B;
					}
					pm->cmd.rightmove = 0;
					pm->cmd.forwardmove = 0;
				}
				pm->cmd.rightmove = 0;
				pm->cmd.forwardmove = 0;
			}
			if (anim != -1)
			{
				int extraHoldTime = 0;
				if (PM_MeleeblockAnim(pm->ps->torsoAnim) && !PM_MeleeblockHoldAnim(pm->ps->torsoAnim))
				{//use the hold pose, don't start it all over again
					anim = MELEE_STANCE_HOLD_LT + (anim - MELEE_STANCE_LT);
					extraHoldTime = 600;
				}
				if (anim == pm->ps->torsoAnim)
				{
					if (pm->ps->torsoTimer < 600)
					{
						pm->ps->torsoTimer = 600;
					}
				}
				else
				{
					PM_SetAnim(SETANIM_TORSO, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					pm->cmd.forwardmove = 0;
					pm->cmd.rightmove = 0;
					pm->cmd.upmove = 0;
				}
				if (extraHoldTime && pm->ps->torsoTimer < extraHoldTime)
				{
					pm->ps->torsoTimer += extraHoldTime;
					pm->cmd.forwardmove = 0;
					pm->cmd.rightmove = 0;
					pm->cmd.upmove = 0;
				}
				if (pm->ps->groundEntityNum != ENTITYNUM_NONE && !pm->cmd.upmove)
				{
					PM_SetAnim(SETANIM_LEGS, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					pm->ps->legsTimer = pm->ps->torsoTimer;
					pm->cmd.forwardmove = 0;
					pm->cmd.rightmove = 0;
					pm->cmd.upmove = 0;
				}
				else
				{
					PM_SetAnim(SETANIM_LEGS, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					pm->cmd.forwardmove = 0;
					pm->cmd.rightmove = 0;
					pm->cmd.upmove = 0;
				}
				pm->ps->weaponTime = pm->ps->torsoTimer;
				pm->cmd.forwardmove = 0;
				pm->cmd.rightmove = 0;
				pm->cmd.upmove = 0;
			}
		}
	}
	pm->cmd.forwardmove = 0;
	pm->cmd.rightmove = 0;
	pm->cmd.upmove = 0;
}


int PM_SetStandingSaberBlock(void)
{
	int anim = -1;

	if (pm->ps->weapon != WP_SABER)
	{
		return qfalse;
	}
	if (PM_WalkingOrRunningAnim(pm->ps->legsAnim))
	{
		return qfalse;
	}

	saberInfo_t* saber1;

	saber1 = BG_MySaber(pm->ps->clientNum, 0);

	//standing block
	if (!BG_KickMove(pm->ps->saberMove)//not already in a kick
		&& !pm->ps->saberInFlight
		&& !(pm->ps->pm_flags & PMF_DUCKED))
	{
		if ((pm->cmd.rightmove)
			&& !PM_WalkingOrRunningAnim(pm->ps->legsAnim))  //pushing a direction
		{
			if (pm->cmd.rightmove > 0)
			{//lean right
				if (saber1->type == SABER_STAFF || saber1->type == SABER_STAFF_SFX || saber1->type == SABER_STAFF_MAUL
					|| saber1->type == SABER_STAFF_UNSTABLE || saber1->type == SABER_STAFF_THIN || saber1->type == SABER_ELECTROSTAFF
					|| saber1->type == SABER_PALP || saber1->type == SABER_ANAKIN)//saber staff
				{
					if (pm->ps->torsoAnim == BOTH_BLOCK_BACK_HOLD_R)
					{
						anim = pm->ps->torsoAnim;
					}
					else
					{
						anim = BOTH_BLOCK_BACK_R;
					}
				}
				else
				{
					if (pm->ps->torsoAnim == BOTH_BLOCK_HOLD_R)
					{
						anim = pm->ps->torsoAnim;
					}
					else
					{
						anim = BOTH_BLOCK_R;
					}
				}
			}
			else if (pm->cmd.rightmove < 0)
			{//lean left
				if (saber1->type == SABER_STAFF || saber1->type == SABER_STAFF_SFX || saber1->type == SABER_STAFF_MAUL
					|| saber1->type == SABER_STAFF_UNSTABLE || saber1->type == SABER_STAFF_THIN || saber1->type == SABER_ELECTROSTAFF
					|| saber1->type == SABER_PALP || saber1->type == SABER_ANAKIN)//saber staff
				{
					if (pm->ps->torsoAnim == BOTH_BLOCK_BACK_HOLD_L)
					{
						anim = pm->ps->torsoAnim;
					}
					else
					{
						anim = BOTH_BLOCK_BACK_L;
					}
				}
				else
				{
					if (pm->ps->torsoAnim == BOTH_BLOCK_HOLD_L)
					{
						anim = pm->ps->torsoAnim;
					}
					else
					{
						anim = BOTH_BLOCK_L;
					}
				}
			}
			else if ((pm->cmd.buttons & BUTTON_ATTACK))
			{
				switch (pm->ps->fd.saberAnimLevel)
				{
				case SS_STAFF:
					anim = BOTH_P7_S7_T_;
					break;
				case SS_DUAL:
					anim = BOTH_P6_S6_T_;
					break;
				default:
					anim = BOTH_P1_S1_T_;
					break;
				}
			}
		}
		else if (!pm->cmd.rightmove && (pm->cmd.buttons & BUTTON_ATTACK))
		{
			switch (pm->ps->fd.saberAnimLevel)
			{
			case SS_STAFF:
				anim = BOTH_P7_S7_T_;
				break;
			case SS_DUAL:
				anim = BOTH_P6_S6_T_;
				break;
			default:
				anim = BOTH_P1_S1_T_;
				break;
			}
		}
		if (anim != -1)
		{
			int extraHoldTime = 0;
			if (PM_BlockAnim(pm->ps->torsoAnim) && !PM_BlockHoldAnim(pm->ps->torsoAnim))
			{//use the hold pose, don't start it all over again
				if (saber1->type == SABER_STAFF || saber1->type == SABER_STAFF_SFX || saber1->type == SABER_STAFF_MAUL
					|| saber1->type == SABER_STAFF_UNSTABLE || saber1->type == SABER_STAFF_THIN || saber1->type == SABER_ELECTROSTAFF
					|| saber1->type == SABER_PALP || saber1->type == SABER_ANAKIN)//saber staff
				{
					anim = BOTH_BLOCK_BACK_HOLD_L + (anim - BOTH_BLOCK_BACK_L);
				}
				else
				{
					anim = BOTH_BLOCK_HOLD_L + (anim - BOTH_BLOCK_L);
				}
				extraHoldTime = 75;
			}
			if (anim == pm->ps->torsoAnim)
			{
				if (pm->ps->torsoTimer < 75)
				{
					pm->ps->torsoTimer = 75;
				}
			}
			else
			{
				PM_SetAnim(SETANIM_TORSO, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			if (extraHoldTime&& pm->ps->torsoTimer < extraHoldTime)
			{
				pm->ps->torsoTimer += extraHoldTime;
			}
			if (pm->ps->groundEntityNum != ENTITYNUM_NONE && !pm->cmd.upmove)
			{
				PM_SetAnim(SETANIM_LEGS, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->legsTimer = pm->ps->torsoTimer;
			}
			else
			{
				PM_SetAnim(SETANIM_LEGS, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			pm->ps->weaponTime = pm->ps->torsoTimer;
		}
	}
	return anim;
}

int PM_ReturnforQuad(int quad)
{
	switch (quad)
	{
	case Q_BR:
		return LS_R_TL2BR;
		break;
	case Q_R:
		return LS_R_L2R;
		break;
	case Q_TR:
		return LS_R_BL2TR;
		break;
	case Q_T:
		if (!Q_irand(0, 1))
		{
			return LS_R_BL2TR;
		}
		else
		{
			return LS_R_BR2TL;
		}
		break;
	case Q_TL:
		return LS_R_BR2TL;
		break;
	case Q_L:
		return LS_R_R2L;
		break;
	case Q_BL:
		return LS_R_TR2BL;
		break;
	case Q_B:
		return LS_R_T2B;
		break;
	default:
		return LS_READY;
	};
}

int BlockedforQuad(int quad)
{//returns the saberBlocked direction for given quad.
	switch (quad)
	{
	case Q_BR:
		return BLOCKED_LOWER_RIGHT;
		break;
	case Q_R:
		return BLOCKED_UPPER_RIGHT;
		break;
	case Q_TR:
		return BLOCKED_UPPER_RIGHT;
		break;
	case Q_T:
		return BLOCKED_TOP;
		break;
	case Q_TL:
		return BLOCKED_UPPER_LEFT;
		break;
	case Q_L:
		return BLOCKED_UPPER_LEFT;
		break;
	case Q_BL:
		return BLOCKED_LOWER_LEFT;
		break;
	case Q_B:
		return BLOCKED_LOWER_LEFT;
		break;
	default:
		return BLOCKED_FRONT;
	};
}

int PM_DoFake(int curmove)
{
	int newQuad = -1;

	if (pm->ps->userInt3 & (1 << FLAG_ATTACKFAKE))
	{//already attack faking, can't do another one until this one is over.
		return LS_NONE;
	}

	if (pm->cmd.rightmove > 0)
	{//moving right
		if (pm->cmd.forwardmove > 0)
		{//forward right = TL2BR slash
			newQuad = Q_TL;
		}
		else if (pm->cmd.forwardmove < 0)
		{//backward right = BL2TR uppercut
			newQuad = Q_BL;
		}
		else
		{//just right is a left slice
			newQuad = Q_L;
		}
	}
	else if (pm->cmd.rightmove < 0)
	{//moving left
		if (pm->cmd.forwardmove > 0)
		{//forward left = TR2BL slash
			newQuad = Q_TR;
		}
		else if (pm->cmd.forwardmove < 0)
		{//backward left = BR2TL uppercut
			newQuad = Q_BR;
		}
		else
		{//just left is a right slice
			newQuad = Q_R;
		}
	}
	else
	{//not moving left or right
		if (pm->cmd.forwardmove > 0)
		{//forward= T2B slash
			newQuad = Q_T;
		}
		else if (pm->cmd.forwardmove < 0)
		{//backward= T2B slash	//or B2T uppercut?
			newQuad = Q_T;
		}
		else
		{//Not moving at all
		}
	}

	if (newQuad == -1)
	{//assume that we're trying to fake in our current direction so we'll automatically fake 
	 //in the completely opposite direction.  This allows the player to do a fake while standing still.
		newQuad = saberMoveData[curmove].endQuad;
	}

	if (newQuad == saberMoveData[curmove].endQuad)
	{//player is attempting to do a fake move to the same quadrant 
	 //as such, fake to the completely opposite quad
		newQuad += 4;
		if (newQuad > Q_B)
		{//rotated past Q_B, shift back to get the proper quadrant
			newQuad -= Q_NUM_QUADS;
		}
	}

	if (newQuad == Q_B)
	{//attacks can't be launched from this quad, just randomly fake to the bottom left/right
		if (PM_irand_timesync(0, 9) <= 4)
		{
			newQuad = Q_BL;
		}
		else
		{
			newQuad = Q_BR;
		}

	}

	//add faking flag
	pm->ps->userInt3 |= (1 << FLAG_ATTACKFAKE);
	pm->ps->fd.forcePower -= 20;
	return transitionMove[saberMoveData[curmove].endQuad][newQuad];
}

void PM_SaberDroidWeapon(void)
{
	// make weapon function
	if (pm->ps->weaponTime > 0)
	{
		pm->ps->weaponTime -= pml.msec;
		if (pm->ps->weaponTime <= 0)
		{
			pm->ps->weaponTime = 0;
		}
	}

	// Now we react to a block action by the player's lightsaber.
	if (pm->ps->saberBlocked)
	{

		switch (pm->ps->saberBlocked)
		{
		case BLOCKED_PARRY_BROKEN:
			PM_SetAnim(SETANIM_BOTH, Q_irand(BOTH_PAIN1, BOTH_PAIN3), SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_ATK_BOUNCE:
			PM_SetAnim(SETANIM_BOTH, Q_irand(BOTH_PAIN1, BOTH_PAIN3), SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_UPPER_RIGHT:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_TR, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_UPPER_RIGHT_PROJ:
			PM_SetAnim(SETANIM_BOTH, BOTH_BLOCKATTACK_RIGHT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_LOWER_RIGHT:
		case BLOCKED_LOWER_RIGHT_PROJ:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_BR, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_UPPER_LEFT_PROJ:
			PM_SetAnim(SETANIM_BOTH, BOTH_BLOCKATTACK_LEFT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_UPPER_LEFT:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_TL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_LOWER_LEFT:
		case BLOCKED_LOWER_LEFT_PROJ:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_BL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_TOP:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_T_, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_TOP_PROJ:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_T_, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_FRONT:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_T1_, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_FRONT_PROJ:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_T1_, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_BLOCKATTACK_LEFT:
			PM_SetAnim(SETANIM_BOTH, BOTH_BLOCKATTACK_LEFT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_BLOCKATTACK_RIGHT:
			PM_SetAnim(SETANIM_BOTH, BOTH_BLOCKATTACK_RIGHT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_BLOCKATTACK_FRONT:
			PM_SetAnim(SETANIM_BOTH, BOTH_P7_S7_T_, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		case BLOCKED_BACK:
			PM_SetAnim(SETANIM_BOTH, BOTH_P1_S1_B1_, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			pm->ps->legsTimer += Q_irand(200, 1000);
			pm->ps->weaponTime = pm->ps->legsTimer;
			break;
		default:
			pm->ps->saberBlocked = BLOCKED_NONE;
			break;
		}

		pm->ps->saberBlocked = BLOCKED_NONE;
		pm->ps->saberMove = LS_NONE;
		pm->ps->weaponstate = WEAPON_READY;

		// Done with block, so stop these active weapon branches.
		return;
	}
}

void PM_CheckClearSaberBlock(void)
{

#ifdef _GAME
	if (!(g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC))
#endif
	{//player
		if (pm->ps->saberBlocked >= BLOCKED_UPPER_RIGHT_PROJ && pm->ps->saberBlocked <= BLOCKED_BACK)
		{//blocking a projectile
			if (pm->ps->fd.forcePowerDebounce[FP_SABER_DEFENSE] < pm->cmd.serverTime)
			{//block is done or breaking out of it with an attack
				pm->ps->weaponTime = 0;
				pm->ps->saberBlocked = BLOCKED_NONE;
			}
			else if ((pm->cmd.buttons & BUTTON_ATTACK) && (!(pm->cmd.buttons & BUTTON_BLOCK)))
			{//block is done or breaking out of it with an attack
				pm->ps->weaponTime = 0;
				pm->ps->saberBlocked = BLOCKED_NONE;
			}
		}
	}
}

void PM_WeaponLightsaber(void)
{
	int			addTime;
	qboolean	delayed_fire = qfalse;
	int			anim = -1, curmove, newmove = LS_NONE;

	saberInfo_t* saber1;

	saber1 = BG_MySaber(pm->ps->clientNum, 0);

	qboolean checkOnlyWeap = qfalse;

	if (pm_entSelf->s.NPC_class == CLASS_SABER_DROID)
	{//Saber droid does it's own attack logic
		PM_SaberDroidWeapon();
		return;
	}

	if (pm_entSelf->s.NPC_class == CLASS_SBD)
	{//Saber droid does it's own attack logic
		PM_SaberDroidWeapon();
		return;
	}

	if (PM_InKnockDown(pm->ps) || BG_InRoll(pm->ps, pm->ps->legsAnim))
	{//in knockdown
		// make weapon function
		if (pm->ps->weaponTime > 0)
		{
			pm->ps->weaponTime -= pml.msec;
			if (pm->ps->weaponTime <= 0)
			{
				pm->ps->weaponTime = 0;
			}
		}
		if (pm->ps->legsAnim == BOTH_ROLL_F ||
			pm->ps->legsAnim == BOTH_ROLL_F1 ||
			pm->ps->legsAnim == BOTH_ROLL_F2 &&
			pm->ps->legsTimer <= 250)
		{
			if ((pm->cmd.buttons& BUTTON_ATTACK))
			{
				if (BG_EnoughForcePowerForMove(FATIGUE_GROUNDATTACK) && !pm->ps->saberInFlight)
				{
					if (PM_CanDoRollStab())
					{
						//make sure the saber is on for this move!
						if (pm->ps->saberHolstered == 2)
						{//all the way off
							pm->ps->saberHolstered = 0;
							PM_AddEvent(EV_SABER_UNHOLSTER);
						}
						PM_SetSaberMove(LS_ROLL_STAB);
						WP_ForcePowerDrain(pm->ps, FP_GRIP, FATIGUE_GROUNDATTACK);
					}
				}
			}
		}
		return;
	}

	if (pm->ps->torsoAnim == BOTH_FORCELONGLEAP_LAND
		|| pm->ps->torsoAnim == BOTH_FORCELONGLEAP_LAND2
		|| (pm->ps->torsoAnim == BOTH_FORCELONGLEAP_START && !(pm->cmd.buttons & BUTTON_ATTACK)))
	{//if you're in the long-jump and you're not attacking (or are landing), you're not doing anything
		if (pm->ps->torsoTimer)
		{
			return;
		}
	}

	if (pm->ps->legsAnim == BOTH_FLIP_HOLD7
		&& !(pm->cmd.buttons & BUTTON_ATTACK))
	{//if you're in the upside-down attack hold, don't do anything unless you're attacking
		return;
	}

	if (pm->ps->saberLockTime > pm->cmd.serverTime)
	{
		pm->ps->saberMove = LS_NONE;
		PM_SaberLocked();
		return;
	}
	else
	{
		if (pm->ps->saberLockFrame)
		{
			if (pm->ps->saberLockEnemy < ENTITYNUM_NONE &&
				pm->ps->saberLockEnemy >= 0)
			{
				bgEntity_t* bgEnt;
				playerState_t* en;

				bgEnt = PM_BGEntForNum(pm->ps->saberLockEnemy);

				if (bgEnt)
				{
					en = bgEnt->playerState;

					if (en)
					{
						PM_SaberLockBreak(en, qfalse, 0);
						return;
					}
				}
			}

			if (pm->ps->saberLockFrame)
			{
				pm->ps->torsoTimer = 0;
				PM_SetAnim(SETANIM_TORSO, BOTH_STAND1, SETANIM_FLAG_OVERRIDE);
				pm->ps->saberLockFrame = 0;
			}
		}
	}

	if (BG_KickingAnim(pm->ps->legsAnim) ||
		BG_KickingAnim(pm->ps->torsoAnim))
	{
		if (pm->ps->legsTimer > 0)
		{//you're kicking, no interruptions
			return;
		}
		//done?  be immeditately ready to do an attack
		pm->ps->saberMove = LS_READY;
		pm->ps->weaponTime = 0;
	}

	if (BG_SabersOff(pm->ps))
	{
		if (pm->ps->saberMove != LS_READY)
		{
			PM_SetSaberMove(LS_READY);
		}

		if ((pm->ps->legsAnim) != (pm->ps->torsoAnim) && !PM_InSlopeAnim(pm->ps->legsAnim) &&
			pm->ps->torsoTimer <= 0)
		{
			PM_SetAnim(SETANIM_TORSO, (pm->ps->legsAnim), SETANIM_FLAG_OVERRIDE);
		}
		else if (PM_InSlopeAnim(pm->ps->legsAnim) && pm->ps->torsoTimer <= 0)
		{
#ifdef _GAME
			if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
			{// Some special bot stuff.
				PM_SetAnim(SETANIM_TORSO, PM_ReadyPoseForSaberAnimLevelBOT(), SETANIM_FLAG_OVERRIDE);
			}
			else
			{
				PM_SetAnim(SETANIM_TORSO, PM_ReadyPoseForSaberAnimLevel(), SETANIM_FLAG_OVERRIDE);
			}
#endif

		}

		if (pm->ps->weaponTime < 1 && (pm->cmd.buttons & BUTTON_ATTACK) && !pm->ps->saberInFlight)
		{
			if (pm->ps->duelTime < pm->cmd.serverTime)
			{
				if (!pm->ps->m_iVehicleNum)
				{ //don't let em unholster the saber by attacking while on vehicle
					pm->ps->saberHolstered = 0;
					PM_AddEvent(EV_SABER_UNHOLSTER);
				}
				else
				{
					pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
					pm->cmd.buttons &= ~BUTTON_ATTACK;
				}
			}
		}

		if (pm->ps->weaponTime > 0)
		{
			pm->ps->weaponTime -= pml.msec;
		}

		checkOnlyWeap = qtrue;
		goto weapChecks;
	}

	if (!pm->ps->saberEntityNum && pm->ps->saberInFlight)
	{ //this means our saber has been knocked away
		if (pm->ps->fd.saberAnimLevel == SS_DUAL)
		{
			if (pm->ps->saberHolstered > 1 || !pm->ps->saberHolstered)
			{
				pm->ps->saberHolstered = 1;
			}
		}
		else
		{
			pm->cmd.buttons &= ~BUTTON_ATTACK;
		}
		pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
	}

	//preblocks can be interrupted
	if (PM_SaberInParry(pm->ps->saberMove) && pm->ps->userInt3 & (1 << FLAG_PREBLOCK) // in a pre-block
		&& ((pm->cmd.buttons & BUTTON_ALT_ATTACK) || (pm->cmd.buttons & BUTTON_ATTACK))) //and attempting an attack
	{//interrupting a preblock
		pm->ps->weaponTime = 0;
		pm->ps->torsoTimer = 0;
	}

	if ((pm->cmd.buttons & BUTTON_ALT_ATTACK))
	{ //might as well just check for a saber throw right here
		if (pm->ps->saberInFlight && (pm->cmd.buttons & BUTTON_KICK))
		{ //kick after doing a saberthrow,whalst saber is still being controlled
			if (!(pm->cmd.buttons & BUTTON_ATTACK))//not trying to swing the saber
			{
				if ((pm->cmd.forwardmove || pm->cmd.rightmove)//trying to kick in a specific direction
					&& PM_CheckAltKickAttack())//trying to do a kick
				{//allow them to do the kick now!
					int kickMove = PM_MeleeMoveForConditions();
					if (kickMove == LS_HILT_BASH)
					{ //yeah.. no hilt to bash with!
						kickMove = LS_KICK_F2;
					}
					if (kickMove != -1)
					{
						pm->ps->weaponTime = 0;
						PM_SetSaberMove(kickMove);
						return;
					}
				}
			}
		}
		else if (!PM_SaberThrowable() && (pm->cmd.buttons & BUTTON_ALT_ATTACK))//not trying to swing the saber
		{ //kick instead of doing a throw 
			if (PM_DoKick())
			{
				return;
			}
		}
		else if (!PM_SaberThrowable() && (pm->cmd.buttons & BUTTON_KICK))//not trying to swing the saber
		{ //kick instead of doing a throw 
			if (PM_DoSlap())
			{
				return;
			}
		}
		else if (pm->ps->saberInFlight && pm->ps->saberEntityNum)
		{//saber is already in flight continue moving it with the force.
			switch (pm->ps->fd.saberAnimLevel)
			{
			case SS_FAST:
				PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				break;
			case SS_TAVION:
				PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL_ALT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				break;
			case SS_MEDIUM:
				PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				break;
			case SS_STRONG:
				PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				break;
			case SS_DESANN:
				PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL_ALT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				break;
			case SS_DUAL:
				PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				break;
			case SS_STAFF:
				PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL_ALT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				break;
			}
			pm->ps->torsoTimer = 1;
			return;
		}
		else if (pm->ps->weaponTime < 1
			&& pm->ps->saberCanThrow
			&& !BG_HasYsalamiri(pm->gametype, pm->ps) &&
			BG_CanUseFPNow(pm->gametype, pm->ps, pm->cmd.serverTime, FP_SABERTHROW) &&
			pm->ps->fd.forcePowerLevel[FP_SABERTHROW] > 0 &&
			PM_SaberPowerCheck())
		{
			trace_t sabTr;
			vec3_t	fwd, minFwd, sabMins, sabMaxs;

			VectorSet(sabMins, SABERMINS_X, SABERMINS_Y, SABERMINS_Z);
			VectorSet(sabMaxs, SABERMAXS_X, SABERMAXS_Y, SABERMAXS_Z);

			AngleVectors(pm->ps->viewangles, fwd, NULL, NULL);
			VectorMA(pm->ps->origin, SABER_MIN_THROW_DIST, fwd, minFwd);
#ifdef _GAME
			if (m_nerf.integer & (1 << EOC_CLOSETHROW))
#else
			if (cgs.m_nerf & (1 << EOC_CLOSETHROW))
#endif
			{
				pm->trace(&sabTr, pm->ps->origin, sabMins, sabMaxs, minFwd, pm->ps->clientNum, MASK_PLAYERSOLID);

				if (sabTr.allsolid || sabTr.startsolid || sabTr.fraction < 1.0f)
				{
					//not enough room to throw
				}
				else
				{//throw it
				 //This will get set to false again once the saber makes it back to its owner game-side
					if (!pm->ps->saberInFlight)
					{
						pm->ps->fd.forcePower -= forcePowerNeeded[pm->ps->fd.forcePowerLevel[FP_SABERTHROW]][FP_SABERTHROW];
					}

					pm->ps->saberInFlight = qtrue;
				}
			}
			else
			{ //always throw no matter how close 
				if (!pm->ps->saberInFlight)
				{
					pm->ps->fd.forcePower -= forcePowerNeeded[pm->ps->fd.forcePowerLevel[FP_SABERTHROW]][FP_SABERTHROW];
				}

				pm->ps->saberInFlight = qtrue;

				switch (pm->ps->fd.saberAnimLevel)
				{
				case SS_FAST:
					PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_TAVION:
					PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL_ALT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_MEDIUM:
					PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_STRONG:
					PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_DESANN:
					PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL_ALT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_DUAL:
					PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				case SS_STAFF:
					PM_SetAnim(SETANIM_TORSO, BOTH_SABERPULL_ALT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					break;
				}
				pm->ps->torsoTimer = 1;
				return;
			}
		}
	}

	// check for dead player
	if (pm->ps->stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	// make weapon function
	if (pm->ps->weaponTime > 0)
	{
		//check for special pull move while busy
		saberMoveName_t pullmove = PM_CheckPullAttack();
		if (pullmove != LS_NONE)
		{
			pm->ps->weaponTime = 0;
			pm->ps->torsoTimer = 0;
			pm->ps->legsTimer = 0;
			pm->ps->forceHandExtend = HANDEXTEND_NONE;
			pm->ps->weaponstate = WEAPON_READY;
			PM_SetSaberMove(pullmove);
			return;
		}

		pm->ps->weaponTime -= pml.msec;
	}
	else
	{
		if ((pm->cmd.buttons & BUTTON_BLOCK)
			&& pm->ps->weapon == WP_SABER
			&& pm->ps->saberHolstered > 1 || pm->ps->saberHolstered == 2)
		{//turn on the saber if it's not on
			pm->ps->saberHolstered = 0;
			PM_AddEvent(EV_SABER_UNHOLSTER);
		}
		else if ((pm->cmd.buttons & BUTTON_BLOCK) && !(pm->cmd.buttons & BUTTON_WALKING) 
			&& pm->ps->weapon == WP_SABER
			&& !BG_SabersOff(pm->ps)
			&& !pm->ps->weaponTime
			&& !PM_SaberInBounce(pm->ps->saberMove)
			&& !PM_SaberInKnockaway(pm->ps->saberMove)
			&& !PM_SaberInBrokenParry(pm->ps->saberMove)
			&& !PM_SaberInDamageMove(pm->ps->saberMove)
			&& !PM_WalkingOrRunningAnim(pm->ps->legsAnim))
		{//set up the block position				
			PM_SetStandingSaberBlock();
		}
		else if ((pm->cmd.buttons& BUTTON_WALKING && pm->cmd.buttons& BUTTON_BLOCK) 
			&& pm->ps->weapon == WP_SABER
			&& !BG_SabersOff(pm->ps)
			&& !pm->ps->weaponTime
			&& !PM_SaberInBounce(pm->ps->saberMove)
			&& !PM_SaberInKnockaway(pm->ps->saberMove)
			&& !PM_SaberInBrokenParry(pm->ps->saberMove)
			&& !PM_SaberInDamageMove(pm->ps->saberMove)
			&& pm->ps->fd.blockPoints > BLOCKPOINTS_FAIL)
		{//set up the block position				
			PM_SetBlock();
		}
		else
		{
			pm->ps->weaponstate = WEAPON_READY;
		}
	}

	PM_CheckClearSaberBlock();

	if (PM_LockedAnim(pm->ps->torsoAnim)
		&& pm->ps->torsoTimer)
	{//can't interrupt these anims ever
		return;
	}
	if (PM_SuperBreakLoseAnim(pm->ps->torsoAnim)
		&& pm->ps->torsoTimer)
	{//don't interrupt these anims
		return;
	}
	if (PM_SuperBreakWinAnim(pm->ps->torsoAnim)
		&& pm->ps->torsoTimer)
	{//don't interrupt these anims
		return;
	}

	// Now we react to a block action by the player's lightsaber.
	if (pm->ps->saberBlocked)
	{
		qboolean wasAttackedByGun = qfalse;

		if (pm->ps->saberMove > LS_PUTAWAY && pm->ps->saberMove <= LS_A_BL2TR && pm->ps->saberBlocked != BLOCKED_PARRY_BROKEN &&
			(pm->ps->saberBlocked < BLOCKED_UPPER_RIGHT_PROJ || pm->ps->saberBlocked > BLOCKED_TOP_PROJ))
		{//we parried another lightsaber while attacking, so treat it as a bounce
			pm->ps->saberBlocked = BLOCKED_ATK_BOUNCE;
		}
		else if ((pm->cmd.buttons&BUTTON_USE) && pm->ps->saberBlocked >= BLOCKED_UPPER_RIGHT && pm->ps->saberBlocked <= BLOCKED_FRONT &&
			!PM_SaberInKnockaway(pm->ps->saberBounceMove))
		{//if hitting attack during a parry (not already a knockaway)
			pm->ps->saberBounceMove = PM_KnockawayForParry(pm->ps->saberBlocked);
		}
		else
		{
			if (pm->ps->saberBlocked >= BLOCKED_UPPER_RIGHT_PROJ && pm->ps->saberBlocked <= BLOCKED_TOP_PROJ)
			{//blocking a projectile
				if ((pm->cmd.buttons & BUTTON_ATTACK))
				{//trying to attack
					if (pm->ps->saberMove == LS_READY || PM_SaberInReflect(pm->ps->saberMove))
					{
						pm->ps->saberBlocked = BLOCKED_NONE;
						pm->ps->saberBounceMove = LS_NONE;
						pm->ps->weaponstate = WEAPON_READY;
						if (PM_SaberInReflect(pm->ps->saberMove) && pm->ps->weaponTime > 0)
						{
							pm->ps->weaponTime = 0;
						}
						return;
					}
				}
			}
		}
		switch (pm->ps->saberBlocked)
		{
		case BLOCKED_BOUNCE_MOVE:
		{ //act as a bounceMove and reset the saberMove instead of using a seperate value for it
			pm->ps->torsoTimer = 0;
			PM_SetSaberMove((saberMoveName_t)pm->ps->saberMove);
			pm->ps->weaponTime = pm->ps->torsoTimer;
			pm->ps->saberBlocked = 0;
		}
		break;
		case BLOCKED_PARRY_BROKEN:
			//whatever parry we were is in now broken, play the appropriate knocked-away anim
		{
			saberMoveName_t nextMove;

			if (PM_SaberInBrokenParry(pm->ps->saberMove))
			{//already have one...?
				nextMove = (saberMoveName_t)pm->ps->saberBounceMove;
			}
			else
			{
				nextMove = PM_BrokenParryForParry((saberMoveName_t)pm->ps->saberMove);
			}
			if (nextMove != LS_NONE)
			{
				PM_SetSaberMove((saberMoveName_t)nextMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else
			{
				//Maybe in a knockaway?
			}
		}
		break;
		case BLOCKED_ATK_BOUNCE:
			if (pm->ps->saberMove >= LS_T1_BR__R || PM_SaberInReturn(pm->ps->saberMove))
			{//an actual bounce?  Other bounces before this are actually transitions?
				pm->ps->saberBlocked = BLOCKED_NONE;
			}
			else if ((pm->ps->userInt3 & (1 << FLAG_SLOWBOUNCE) && !(pm->ps->userInt3 & (1 << FLAG_OLDSLOWBOUNCE)))
				|| (PM_SaberInBounce(pm->ps->saberMove) || !PM_SaberInAttack(pm->ps->saberMove) && !PM_SaberInStart(pm->ps->saberMove)))
			{//already in the bounce, go into an attack or transition to ready.. should never get here since can't be blocked in a bounce!
				int nextMove;

				if (pm->cmd.buttons & BUTTON_ATTACK)
				{//transition to a new attack
#ifdef _GAME
					if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
					{// Some special bot stuff.
						nextMove = saberMoveData[pm->ps->saberMove].chain_attack;
					}
					else
#endif
					{//player
						int newQuad = PM_SaberMoveQuadrantForMovement(&pm->cmd);
						while (newQuad == saberMoveData[pm->ps->saberMove].startQuad)
						{
							newQuad = Q_irand(Q_BR, Q_BL);
						}

						nextMove = transitionMove[saberMoveData[pm->ps->saberMove].startQuad][newQuad];
					}
				}
				else
				{//return to ready
#ifdef _GAME
					if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
					{// Some special bot stuff.
						nextMove = saberMoveData[pm->ps->saberMove].chain_idle;
					}
					else
#endif
					{//player
						if (saberMoveData[pm->ps->saberMove].startQuad == Q_T)
						{
							nextMove = LS_R_BL2TR;
						}
						else if (saberMoveData[pm->ps->saberMove].startQuad < Q_T)
						{
							nextMove = LS_R_TL2BR + (saberMoveName_t)(saberMoveData[pm->ps->saberMove].startQuad - Q_BR);
						}
						else
						{
							nextMove = LS_R_BR2TL + (saberMoveName_t)(saberMoveData[pm->ps->saberMove].startQuad - Q_TL);
						}
					}
					pm->ps->torsoTimer = 0;
				}
				PM_SetSaberMove((saberMoveName_t)nextMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else
			{//start the bounce move
				saberMoveName_t bounceMove;
				if (pm->ps->saberBounceMove != LS_NONE)
				{
					bounceMove = (saberMoveName_t)pm->ps->saberBounceMove;
				}
				else
				{
					bounceMove = PM_SaberBounceForAttack((saberMoveName_t)pm->ps->saberMove);
				}
				PM_SetSaberMove(bounceMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			break;
		case BLOCKED_UPPER_RIGHT:
			if (pm->ps->saberBounceMove != LS_NONE)
			{
				PM_SetSaberMove((saberMoveName_t)pm->ps->saberBounceMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if (saber1->type == SABER_BACKHAND)
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_BLOCK_BACK_HAND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->torsoTimer += Q_irand(200, 1000);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if (saber1->type == SABER_ASBACKHAND)
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_BLOCK_BACK_HAND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->torsoTimer += Q_irand(200, 1000);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else
			{
				PM_SetSaberMove(LS_PARRY_UR);
			}
			break;
		case BLOCKED_UPPER_RIGHT_PROJ:
			if ((saber1->type == SABER_DOOKU) 
				|| (saber1->type == SABER_ANAKIN) 
				|| (saber1->type == SABER_REY)
				|| (saber1->type == SABER_PALP))
			{
				PM_SetSaberMove(LS_K1_TR);
			}
			else
			{
				PM_SetSaberMove(LS_REFLECT_UR);
			}
			wasAttackedByGun = qtrue;
			break;
		case BLOCKED_UPPER_LEFT:
			if (pm->ps->saberBounceMove != LS_NONE)
			{
				PM_SetSaberMove((saberMoveName_t)pm->ps->saberBounceMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if (saber1->type == SABER_BACKHAND)
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_BLOCK_BACK_HAND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->torsoTimer += Q_irand(200, 1000);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if (saber1->type == SABER_ASBACKHAND)
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_BLOCK_BACK_HAND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->torsoTimer += Q_irand(200, 1000);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else
			{
				PM_SetSaberMove(LS_PARRY_UL);
			}
			break;
		case BLOCKED_UPPER_LEFT_PROJ:
			if ((saber1->type == SABER_DOOKU)
				|| (saber1->type == SABER_ANAKIN)
				|| (saber1->type == SABER_REY)
				|| (saber1->type == SABER_PALP))
			{
				PM_SetSaberMove(LS_K1_TL);
			}
			else
			{
				PM_SetSaberMove(LS_REFLECT_UL);
			}			
			wasAttackedByGun = qtrue;
			break;
		case BLOCKED_LOWER_RIGHT:
			if (pm->ps->saberBounceMove != LS_NONE)
			{
				PM_SetSaberMove((saberMoveName_t)pm->ps->saberBounceMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else
			{
				PM_SetSaberMove(LS_PARRY_LR);
			}
			break;
		case BLOCKED_LOWER_RIGHT_PROJ:
			PM_SetSaberMove(LS_REFLECT_LR);
			wasAttackedByGun = qtrue;
			break;
		case BLOCKED_LOWER_LEFT:
			if (pm->ps->saberBounceMove != LS_NONE)
			{
				PM_SetSaberMove((saberMoveName_t)pm->ps->saberBounceMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else
			{
				PM_SetSaberMove(LS_PARRY_LL);
			}
			break;
		case BLOCKED_LOWER_LEFT_PROJ:
			PM_SetSaberMove(LS_REFLECT_LL);
			wasAttackedByGun = qtrue;
			break;
		case BLOCKED_TOP:
			if (pm->ps->saberBounceMove != LS_NONE)
			{
				PM_SetSaberMove((saberMoveName_t)pm->ps->saberBounceMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if (saber1->type == SABER_BACKHAND)
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_BLOCK_BACK_HAND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->torsoTimer += Q_irand(200, 1000);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if (saber1->type == SABER_ASBACKHAND)
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_BLOCK_BACK_HAND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->torsoTimer += Q_irand(200, 1000);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else
			{
				PM_SetSaberMove(LS_PARRY_UP);
			}
			break;
		case BLOCKED_TOP_PROJ:
			PM_SetSaberMove(LS_REFLECT_UP);
			wasAttackedByGun = qtrue;
			break;
		case BLOCKED_FRONT:
			if (pm->ps->saberBounceMove != LS_NONE)
			{
				PM_SetSaberMove((saberMoveName_t)pm->ps->saberBounceMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if (saber1->type == SABER_BACKHAND)
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_BLOCK_BACK_HAND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->torsoTimer += Q_irand(200, 1000);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if (saber1->type == SABER_ASBACKHAND)
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_BLOCK_BACK_HAND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->torsoTimer += Q_irand(200, 1000);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if ((saber1->type == SABER_DOOKU)
				|| (saber1->type == SABER_ANAKIN)
				|| (saber1->type == SABER_REY)
				|| (saber1->type == SABER_PALP))
			{
				PM_SetSaberMove(LS_REFLECT_ATTACK_FRONT);
			}
			else
			{
				PM_SetSaberMove(LS_PARRY_FRONT);
			}
			break;
		case BLOCKED_FRONT_PROJ:
			if (pm->ps->saberBounceMove != LS_NONE)
			{
				PM_SetSaberMove((saberMoveName_t)pm->ps->saberBounceMove);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if (saber1->type == SABER_BACKHAND)
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_BLOCK_BACK_HAND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->torsoTimer += Q_irand(200, 1000);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if (saber1->type == SABER_ASBACKHAND)
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_BLOCK_BACK_HAND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				pm->ps->torsoTimer += Q_irand(200, 1000);
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
			else if ((saber1->type == SABER_DOOKU)
				|| (saber1->type == SABER_ANAKIN)
				|| (saber1->type == SABER_REY)
				|| (saber1->type == SABER_PALP))
			{
				PM_SetSaberMove(LS_REFLECT_ATTACK_FRONT);
			}
			else
			{
				PM_SetSaberMove(LS_REFLECT_FRONT);
			}			
			wasAttackedByGun = qtrue;
			break;
		case BLOCKED_BLOCKATTACK_LEFT:
			PM_SetSaberMove(LS_REFLECT_ATTACK_LEFT);
			break;
		case BLOCKED_BLOCKATTACK_RIGHT:
			PM_SetSaberMove(LS_REFLECT_ATTACK_RIGHT);
			break;
		case BLOCKED_BLOCKATTACK_FRONT:
			PM_SetSaberMove(LS_REFLECT_ATTACK_FRONT);
			break;
		default:
			pm->ps->saberBlocked = BLOCKED_NONE;
			break;
		}
		if (InSaberDelayAnimation(pm->ps->torsoAnim) && (pm->cmd.buttons& BUTTON_ATTACK) && wasAttackedByGun)
		{
			pm->ps->weaponTime += 700;
			pm->ps->torsoTimer += 1500;
			if (!(pm->ps->ManualBlockingFlags& (1 << MBF_MBLOCKING)))
			{
				pm->ps->ManualBlockingFlags |= (1 << MBF_MBLOCKING);
			}
			if (pm->ps->saberBlocked >= BLOCKED_UPPER_RIGHT && pm->ps->saberBlocked < BLOCKED_UPPER_RIGHT_PROJ)
			{//hold the parry for a bit
				if (pm->ps->torsoTimer < pm->ps->weaponTime)
				{
					pm->ps->torsoTimer = pm->ps->weaponTime;
				}
			}
		}

		//clear block
		pm->ps->saberBlocked = 0;

		// Charging is like a lead-up before attacking again.  This is an appropriate use, or we can create a new weaponstate for blocking
		pm->ps->saberBounceMove = LS_NONE;
		pm->ps->weaponstate = WEAPON_READY;

		// Done with block, so stop these active weapon branches.
		return;
	}

weapChecks:

	// check for weapon change
	// can't change if weapon is firing, but can change again if lowering or raising
	if ((pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING) && pm->ps->weaponstate != WEAPON_CHARGING_ALT && pm->ps->weaponstate != WEAPON_CHARGING)
	{
		if (pm->ps->weapon != pm->cmd.weapon)
		{
			PM_BeginWeaponChange(pm->cmd.weapon);
		}
	}

	if (PM_CanDoKata())
	{
		saberMoveName_t overrideMove = LS_INVALID;

		saberInfo_t* saber1;
		saberInfo_t* saber2;

		saber1 = BG_MySaber(pm->ps->clientNum, 0);
		saber2 = BG_MySaber(pm->ps->clientNum, 1);
		//see if we have an overridden (or cancelled) kata move
		if (saber1 && saber1->kataMove != LS_INVALID)
		{
			if (saber1->kataMove != LS_NONE)
			{
				overrideMove = (saberMoveName_t)saber1->kataMove;
			}
		}
		if (overrideMove == LS_INVALID)
		{//not overridden by first saber, check second
			if (saber2
				&& saber2->kataMove != LS_INVALID)
			{
				if (saber2->kataMove != LS_NONE)
				{
					overrideMove = (saberMoveName_t)saber2->kataMove;
				}
			}
		}
		//no overrides, cancelled?
		if (overrideMove == LS_INVALID)
		{
			if (saber2
				&& saber2->kataMove == LS_NONE)
			{
				overrideMove = LS_NONE;
			}
			else if (saber2
				&& saber2->kataMove == LS_NONE)
			{
				overrideMove = LS_NONE;
			}
		}
		if (overrideMove == LS_INVALID)
		{//not overridden
			//FIXME: make sure to turn on saber(s)!
			switch (pm->ps->fd.saberAnimLevel)
			{
			case SS_FAST:
				if (saber1->type == SABER_YODA)
				{
					PM_SetSaberMove(LS_YODA_SPECIAL);
				}
				else
				{
					PM_SetSaberMove(LS_A1_SPECIAL);
				}
				break;
			case SS_MEDIUM:
				PM_SetSaberMove(LS_A2_SPECIAL);
				break;
			case SS_STRONG:
				if (saber1->type == SABER_PALP)
				{
					PM_SetSaberMove(LS_A_JUMP_PALP_);
				}
				else
				{
					PM_SetSaberMove(LS_A3_SPECIAL);
				}
				break;
			case SS_DESANN:
				if (saber1->type == SABER_PALP)
				{
					PM_SetSaberMove(LS_A_JUMP_PALP_);
				}
				else
				{
					PM_SetSaberMove(LS_A5_SPECIAL);
				}
				break;
			case SS_TAVION:
				if (saber1->type == SABER_YODA)
				{
					PM_SetSaberMove(LS_YODA_SPECIAL);
				}
				else
				{
					PM_SetSaberMove(LS_A4_SPECIAL);
				}
				break;
			case SS_DUAL:
				if (saber1->type == SABER_GRIE)
				{
					PM_SetSaberMove(LS_DUAL_SPIN_PROTECT_GRIE);
				}
				else if (saber1->type == SABER_GRIE4)
				{
					PM_SetSaberMove(LS_DUAL_SPIN_PROTECT_GRIE);
				}
				else if (saber1->type == SABER_DAGGER)
				{
					PM_SetSaberMove(LS_STAFF_SOULCAL);
				}
				else if (saber1->type == SABER_ASBACKHAND)
				{
					PM_SetSaberMove(LS_STAFF_SOULCAL);
				}
				else
				{
					PM_SetSaberMove(LS_DUAL_SPIN_PROTECT);
				}
				break;
			case SS_STAFF:
				PM_SetSaberMove(LS_STAFF_SOULCAL);
				break;
			}
			pm->ps->weaponstate = WEAPON_FIRING;
			WP_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER);
		}
		else if (overrideMove != LS_NONE)
		{
			PM_SetSaberMove(overrideMove);
			pm->ps->weaponstate = WEAPON_FIRING;
			WP_ForcePowerDrain(pm->ps, FP_GRIP, SABER_ALT_ATTACK_POWER);
		}
		if (overrideMove != LS_NONE)
		{//not cancelled
			return;
		}
	}

	if (pm->ps->weaponTime > 0)
	{
		return;
	}

	// *********************************************************
	// WEAPON_DROPPING
	// *********************************************************

	// change weapon if time
	if (pm->ps->weaponstate == WEAPON_DROPPING)
	{
		PM_FinishWeaponChange();
		return;
	}

	// *********************************************************
	// WEAPON_RAISING
	// *********************************************************

	if (pm->ps->weaponstate == WEAPON_RAISING)
	{//Just selected the weapon
		pm->ps->weaponstate = WEAPON_IDLE;
		if ((pm->ps->legsAnim) == BOTH_WALK1)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK1, SETANIM_FLAG_NORMAL);
		}
		else if ((pm->ps->legsAnim) == BOTH_RUN1)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN1, SETANIM_FLAG_NORMAL);
		}
		else if ((pm->ps->legsAnim) == BOTH_RUN2)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN2, SETANIM_FLAG_NORMAL);
		}
		else if ((pm->ps->legsAnim) == BOTH_SPRINT)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_SPRINT, SETANIM_FLAG_NORMAL);
		}
		else if ((pm->ps->legsAnim) == BOTH_RUN3)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN3, SETANIM_FLAG_NORMAL);
		}
		else if ((pm->ps->legsAnim) == BOTH_RUN4)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN4, SETANIM_FLAG_NORMAL);
		}
		else if ((pm->ps->legsAnim) == BOTH_RUN5)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN5, SETANIM_FLAG_NORMAL);
		}
		else if ((pm->ps->legsAnim) == BOTH_RUN6)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN6, SETANIM_FLAG_NORMAL);
		}
		else if ((pm->ps->legsAnim) == BOTH_RUN7)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN7, SETANIM_FLAG_NORMAL);
		}
		else if ((pm->ps->legsAnim) == BOTH_RUN8)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN8, SETANIM_FLAG_NORMAL);
		}
		else if ((pm->ps->legsAnim) == BOTH_RUN9)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN9, SETANIM_FLAG_NORMAL);
		}
		else if ((pm->ps->legsAnim) == BOTH_RUN10)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN10, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_RUN_STAFF)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN_STAFF, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_RUN_DUAL)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_RUN_DUAL, SETANIM_FLAG_NORMAL);
		}
		else if ((pm->ps->legsAnim) == BOTH_VADERRUN1)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_VADERRUN1, SETANIM_FLAG_NORMAL);
		}
		else if ((pm->ps->legsAnim) == BOTH_VADERRUN2)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_VADERRUN2, SETANIM_FLAG_NORMAL);
		}
		else if ((pm->ps->legsAnim) == BOTH_WALK1)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK1, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK2)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK2, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK5)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK5, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK6)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK6, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK7)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK7, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK8)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK8, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK9)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK9, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK10)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK10, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK_STAFF)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK_STAFF, SETANIM_FLAG_NORMAL);
		}
		else if (pm->ps->legsAnim == BOTH_WALK_DUAL)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_WALK_DUAL, SETANIM_FLAG_NORMAL);
		}
		else
		{
#ifdef _GAME
			if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
			{// Some special bot stuff.
				PM_SetAnim(SETANIM_TORSO, PM_ReadyPoseForSaberAnimLevelBOT(), SETANIM_FLAG_NORMAL);
			}
			else
			{
				PM_SetAnim(SETANIM_TORSO, PM_ReadyPoseForSaberAnimLevel(), SETANIM_FLAG_NORMAL);
			}
#endif
		}

		if (pm->ps->weaponstate == WEAPON_RAISING)
		{
			return;
		}
	}

	if (checkOnlyWeap)
	{
		return;
	}

	// *********************************************************
	// Check for WEAPON ATTACK
	// *********************************************************

	if (pm->ps->saberInFlight && pm->ps->forceHandExtend != HANDEXTEND_SABERPULL && (pm->cmd.buttons & BUTTON_ALT_ATTACK))
	{//don't have our saber so we can punch instead.
		PM_DoKick();
		return;
	}
	else if (pm->ps->saberInFlight && pm->ps->forceHandExtend != HANDEXTEND_SABERPULL && (pm->cmd.buttons & BUTTON_KICK))
	{//don't have our saber so we can punch instead.
		PM_DoSlap();
		return;
	}

	if (pm->cmd.buttons & BUTTON_KICK)
	{ //ok, try a kick I guess.
		if (!BG_KickingAnim(pm->ps->torsoAnim) &&
			!BG_KickingAnim(pm->ps->legsAnim) &&
			!BG_InRoll(pm->ps, pm->ps->legsAnim))
		{
			int kickMove = PM_MeleeMoveForConditions();

			if (kickMove != -1)
			{
				if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
				{//if in air, convert kick to an in-air kick
					float gDist = PM_GroundDistance();

					if ((!PM_FlippingAnim(pm->ps->legsAnim) || pm->ps->legsTimer <= 0) &&
						gDist > 64.0f &&
						gDist > (-pm->ps->velocity[2]) - 64.0f)
					{
						switch (kickMove)
						{
						case LS_KICK_F2:
							kickMove = LS_KICK_F_AIR;
							break;
						case LS_KICK_B2:
							kickMove = LS_KICK_B_AIR;
							break;
						case LS_SLAP_R:
							kickMove = LS_KICK_R_AIR;
							break;
						case LS_SLAP_L:
							kickMove = LS_KICK_L_AIR;
							break;
						default: //oh well, can't do any other kick move while in-air
							kickMove = -1;
							break;
						}
					}
					else
					{//leave it as a normal kick unless we're too high up
						if (gDist > 128.0f || pm->ps->velocity[2] >= 0)
						{ //off ground, but too close to ground
							kickMove = -1;
						}
					}
				}
			}
			if (kickMove != -1)
			{
				int kickAnim = saberMoveData[kickMove].animToUse;

				if (kickAnim != -1)
				{
					PM_SetAnim(SETANIM_BOTH, kickAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					if (pm->ps->legsAnim == kickAnim)
					{
						pm->ps->weaponTime = pm->ps->legsTimer;
						return;
					}
				}
			}
		}
		//if got here then no move to do so put torso into leg idle or whatever
		if (pm->ps->torsoAnim != pm->ps->legsAnim)
		{
			PM_SetAnim(SETANIM_BOTH, pm->ps->legsAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		pm->ps->weaponTime = 0;
		return;
	}
	//this is never a valid regular saber attack button
	pm->cmd.buttons &= ~BUTTON_KICK;
	pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;

	if (!delayed_fire)
	{
		// Start with the current move, and cross index it with the current control states.
		if (pm->ps->saberMove > LS_NONE && pm->ps->saberMove < LS_MOVE_MAX)
		{
			curmove = (saberMoveName_t)pm->ps->saberMove;
		}
		else
		{
			curmove = LS_READY;
		}
		if (curmove == LS_A_JUMP_T__B_ || curmove == LS_A_JUMP_PALP_ || pm->ps->torsoAnim == BOTH_FORCELEAP2_T__B_ || pm->ps->torsoAnim == BOTH_FORCELEAP_PALP)
		{//must transition back to ready from this anim
			newmove = LS_R_T2B;
		}
		// check for fire
		else if (!(pm->cmd.buttons & (BUTTON_ATTACK)))
		{//not attacking
			pm->ps->weaponTime = 0;

			if (pm->ps->weaponTime > 0)
			{//Still firing
				pm->ps->weaponstate = WEAPON_FIRING;
			}
			else if (pm->ps->weaponstate != WEAPON_READY)
			{
				pm->ps->weaponstate = WEAPON_IDLE;
			}
			//Check for finishing an anim if necc.
			if (curmove >= LS_S_TL2BR && curmove <= LS_S_T2B)
			{//started a swing, must continue from here
				if (pm_entSelf->s.NPC_class != CLASS_NONE)
				{//NPCs never do attack fakes, just follow thru with attack.
					newmove = LS_A_TL2BR + (curmove - LS_S_TL2BR);
				}
				else
				{//perform attack fake
					newmove = PM_ReturnforQuad(saberMoveData[curmove].endQuad);
					BG_AddFatigue(pm->ps, FATIGUE_ATTACKFAKE);
				}
			}
			else if (curmove >= LS_A_TL2BR && curmove <= LS_A_T2B)
			{//finished an attack, must continue from here
				newmove = LS_R_TL2BR + (curmove - LS_A_TL2BR);
			}
			else if (PM_SaberInTransition(curmove))
			{//in a transition, must play sequential attack
				if (pm_entSelf->s.NPC_class != CLASS_NONE)
				{//NPCs never stop attacking mid-attack, just follow thru with attack.
					newmove = saberMoveData[curmove].chain_attack;
				}
				else
				{//exit out of transition without attacking
					newmove = PM_ReturnforQuad(saberMoveData[curmove].endQuad);
				}
			}
			else if (PM_SaberInBounce(curmove))
			{//in a bounce
				if (pm_entSelf->s.NPC_class != CLASS_NONE)
				{//NPCs must play sequential attack
					if (PM_SaberKataDone(LS_NONE, LS_NONE))
					{//done with this kata, must return to ready before attack again
						newmove = saberMoveData[curmove].chain_idle;
					}
					else
					{//okay to chain to another attack
						newmove = saberMoveData[curmove].chain_attack;//we assume they're attacking, even if they're not
						pm->ps->saberAttackChainCount++;
					}
				}
				else
				{//player gets his by directional control
					newmove = saberMoveData[curmove].chain_idle;//oops, not attacking, so don't chain
				}
			}
			else
			{
				PM_SetSaberMove(LS_READY);
				return;
			}
		}
		else if ((pm->cmd.buttons & BUTTON_ATTACK) && (pm->cmd.buttons & BUTTON_USE))
		{//do some fancy faking stuff.
			pm->ps->weaponTime = 0;

			if (pm->ps->weaponTime > 0)
			{//Still firing
				pm->ps->weaponstate = WEAPON_FIRING;
			}
			else if (pm->ps->weaponstate != WEAPON_READY)
			{
				pm->ps->weaponstate = WEAPON_IDLE;
			}
			//Check for finishing an anim if necc.
			if (curmove >= LS_S_TL2BR && curmove <= LS_S_T2B)
			{//allow the player to fake into another transition
				newmove = PM_DoFake(curmove);
				if (newmove == LS_NONE)
				{//no movement, just do the attack
					newmove = LS_A_TL2BR + (curmove - LS_S_TL2BR);
				}
			}
			else if (curmove >= LS_A_TL2BR && curmove <= LS_A_T2B)
			{
				//finished attack, let attack code handle the next step.
			}
			else if (PM_SaberInTransition(curmove))
			{//in a transition, must play sequential attack
				newmove = PM_DoFake(curmove);
				if (newmove == LS_NONE)
				{//no movement, just let the normal attack code handle it
					newmove = saberMoveData[curmove].chain_attack;
				}
			}
			else if (PM_SaberInBounce(curmove))
			{
				//in a bounce
			}
			else
			{
				//returning from a parry I think.
			}
		}

		// ***************************************************
		// Pressing attack, so we must look up the proper attack move.

		if (pm->ps->weaponTime > 0)
		{	// Last attack is not yet complete.
			// But it is if we're blocking!
			if (pm->ps->ManualBlockingFlags & (1 << MBF_BLOCKING))
			{
				PM_SetAnim(SETANIM_TORSO, PM_ReadyPoseForSaberAnimLevel(), SETANIM_FLAG_OVERRIDE);
				return;
			}
			else
			{
				pm->ps->weaponstate = WEAPON_FIRING;
				return;
			}
		}
		else
		{
			int	both = qfalse;
			if (pm->ps->torsoAnim == BOTH_FORCELONGLEAP_ATTACK
				|| pm->ps->torsoAnim == BOTH_FORCELONGLEAP_ATTACK2
				|| pm->ps->torsoAnim == BOTH_FORCELONGLEAP_LAND
				|| pm->ps->torsoAnim == BOTH_FORCELONGLEAP_LAND2)
			{//can't attack in these anims
				return;
			}
			else if ((pm->cmd.buttons & BUTTON_ATTACK) && pm->ps->torsoAnim == BOTH_FORCELONGLEAP_START)
			{//only 1 attack you can do from this anim
				if (pm->ps->saberHolstered == 2)
				{
					pm->ps->saberHolstered = 0;
					PM_AddEvent(EV_SABER_UNHOLSTER);
				}
				PM_SetSaberMove(LS_LEAP_ATTACK);
				return;
			}
			else if (pm->ps->torsoAnim == BOTH_FORCELONGLEAP_START)
			{//only 1 attack you can do from this anim
				if (pm->ps->torsoTimer >= 200)
				{//hit it early enough to do the attack
					PM_SetSaberMove(LS_LEAP_ATTACK);
				}
				return;
			}
			if (curmove >= LS_PARRY_UP && curmove <= LS_REFLECT_LL)
			{//from a parry or reflection, can go directly into an attack
				if (pm_entSelf->s.NPC_class != CLASS_NONE)
				{//NPCs
					newmove = PM_NPCSaberAttackFromQuad(saberMoveData[curmove].endQuad);
				}
				else
				{
					switch (saberMoveData[curmove].endQuad)
					{
					case Q_T:
						newmove = LS_A_T2B;
						break;
					case Q_TR:
						newmove = LS_A_TR2BL;
						break;
					case Q_TL:
						newmove = LS_A_TL2BR;
						break;
					case Q_BR:
						newmove = LS_A_BR2TL;
						break;
					case Q_BL:
						newmove = LS_A_BL2TR;
						break;
						//shouldn't be a parry that ends at L, R or B
					}
				}
			}

			if (newmove != LS_NONE)
			{//have a valid, final LS_ move picked, so skip findingt he transition move and just get the anim
				anim = saberMoveData[newmove].animToUse;
			}

			if (anim == -1)
			{
				if (PM_SaberInTransition(curmove))
				{//in a transition, must play sequential attack
					newmove = saberMoveData[curmove].chain_attack;
				}
				else if (curmove >= LS_S_TL2BR && curmove <= LS_S_T2B)
				{//started a swing, must continue from here
					newmove = LS_A_TL2BR + (curmove - LS_S_TL2BR);
				}
				else if (PM_SaberInBrokenParry(curmove))
				{//broken parries must always return to ready
					newmove = LS_READY;
				}
				else if (PM_SaberInBounce(curmove) && pm->ps->userInt3 & (1 << FLAG_PARRIED))
				{//can't combo if we were parried.
					newmove = LS_READY;
				}
				else
				{//get attack move from movement command
#ifdef _GAME
					if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC
						&& (Q_irand(0, pm->ps->fd.forcePowerLevel[FP_SABER_OFFENSE] - 1)
						|| (pm->ps->fd.saberAnimLevel == SS_FAST
							&& Q_irand(0, 1))))//minor change to make fast-attack users use the special attacks more
					{//NPCs use more randomized attacks the more skilled they are
						newmove = PM_NPCSaberAttackFromQuad(saberMoveData[curmove].endQuad);
					}
					else
#endif
					{
						newmove = PM_SaberAttackForMovement(curmove);

						if ((PM_SaberInBounce(curmove) || PM_SaberInBrokenParry(curmove))
							&& saberMoveData[newmove].startQuad == saberMoveData[curmove].endQuad)
						{//this attack would be a repeat of the last (which was blocked), so don't actually use it, use the default chain attack for this bounce
							newmove = saberMoveData[curmove].chain_attack;
						}
						pm->ps->userInt3 &= ~(1 << FLAG_ATTACKFAKE);
					}
					if (PM_SaberKataDone(curmove, newmove))
					{//cannot chain this time
						newmove = saberMoveData[curmove].chain_idle;
					}
				}
				if (newmove != LS_NONE)
				{
					if (!PM_InCartwheel(pm->ps->legsAnim))
					{//don't do transitions when cartwheeling - could make you spin!
						newmove = PM_SaberAnimTransitionMove((saberMoveName_t)curmove, (saberMoveName_t)newmove);
						anim = saberMoveData[newmove].animToUse;
					}
				}
			}

			if (anim == -1)
			{//not side-stepping, pick neutral anim

				if (PM_InCartwheel(pm->ps->legsAnim) && pm->ps->legsTimer > 100)
				{//if in the middle of a cartwheel, the chain attack is just a normal attack
					switch (pm->ps->legsAnim)
					{
					case BOTH_ARIAL_LEFT://swing from l to r
					case BOTH_CARTWHEEL_LEFT:
						newmove = LS_A_L2R;
						break;
					case BOTH_ARIAL_RIGHT://swing from r to l
					case BOTH_CARTWHEEL_RIGHT:
						newmove = LS_A_R2L;
						break;
					case BOTH_ARIAL_F1://random l/r attack
						if (Q_irand(0, 1))
						{
							newmove = LS_A_L2R;
						}
						else
						{
							newmove = LS_A_R2L;
						}
						break;
					}
				}
				else
				{
					newmove = saberMoveData[curmove].chain_attack;
				}

				if (newmove != LS_NONE)
				{
					anim = saberMoveData[newmove].animToUse;
				}
				

				if (!pm->cmd.forwardmove && !pm->cmd.rightmove && pm->cmd.upmove >= 0 && pm->ps->groundEntityNum != ENTITYNUM_NONE)
				{//not moving at all, so set the anim on entire body
					both = qtrue;
				}
			}

			if (anim == -1)
			{
				switch (pm->ps->legsAnim)
				{
				case BOTH_WALK1:
				case BOTH_WALK1TALKCOMM1:
				case BOTH_WALK2:
				case BOTH_WALK2B:
				case BOTH_WALK3:
				case BOTH_WALK4:
				case BOTH_WALK5:
				case BOTH_WALK6:
				case BOTH_WALK7:
				case BOTH_WALK8:		//# pistolwalk
				case BOTH_WALK9:				//# riflewalk
				case BOTH_WALK10:				//# grenadewalk
				case BOTH_WALK_STAFF:
				case BOTH_WALK_DUAL:
				case BOTH_WALKBACK1:
				case BOTH_WALKBACK2:
				case BOTH_WALKBACK_STAFF:
				case BOTH_WALKBACK_DUAL:
				case BOTH_VADERWALK1:
				case BOTH_VADERWALK2:
				case BOTH_SPRINT:
				case BOTH_RUN1:
				case BOTH_RUN2:
				case BOTH_RUN3:
				case BOTH_RUN4:
				case BOTH_RUN5:
				case BOTH_RUN6:
				case BOTH_RUN7:
				case BOTH_RUN8:
				case BOTH_RUN9:
				case BOTH_RUN10:
				case BOTH_RUN_STAFF:
				case BOTH_RUN_DUAL:
				case BOTH_RUNBACK1:
				case BOTH_RUNBACK2:
				case BOTH_RUNBACK_STAFF:
				case SBD_WALK_WEAPON:
				case SBD_WALK_NORMAL:
				case SBD_WALKBACK_NORMAL:
				case SBD_WALKBACK_WEAPON:
				case SBD_RUNBACK_NORMAL:
				case SBD_RUNING_WEAPON:
				case SBD_RUNBACK_WEAPON:
				case BOTH_RUNINJURED1:
				case BOTH_VADERRUN1:
				case BOTH_VADERRUN2:
					anim = pm->ps->legsAnim;
					break;
#ifdef _GAME
					if (g_entities[pm->ps->clientNum].r.svFlags& SVF_BOT || pm_entSelf->s.eType == ET_NPC)
					{
						anim = PM_ReadyPoseForSaberAnimLevelBOT();
					}
					else
					{
						anim = PM_ReadyPoseForSaberAnimLevel();
					}
#endif
					break;
				}
				newmove = LS_READY;
			}

			if (pm->ps->saberHolstered == 2)
			{//turn on the saber if it's not on
				pm->ps->saberHolstered = 0;
				PM_AddEvent(EV_SABER_UNHOLSTER);
			}

			PM_SetSaberMove((saberMoveName_t)newmove);

			if (both && pm->ps->torsoAnim == anim)
			{
				PM_SetAnim(SETANIM_LEGS, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}

			if (!PM_InCartwheel(pm->ps->torsoAnim))
			{//can still attack during a cartwheel/arial
     			//don't fire again until anim is done
				pm->ps->weaponTime = pm->ps->torsoTimer;
			}
		}
	}

	// *********************************************************
	// WEAPON_FIRING
	// *********************************************************

	pm->ps->weaponstate = WEAPON_FIRING;

	if (pm->ps->weaponTime > 0)
	{// Clear these out since we're not actually firing yet
		pm->ps->eFlags &= ~EF_FIRING;
		pm->ps->eFlags &= ~EF_ALT_FIRING;
		return;
	}

	addTime = pm->ps->weaponTime;

	if (pm->cmd.buttons & BUTTON_ALT_ATTACK)
	{
		PM_AddEvent(EV_ALT_FIRE);

		if (!addTime)
		{
			addTime = weaponData[pm->ps->weapon].altFireTime;
		}
	}
	else
	{
		PM_AddEvent(EV_FIRE_WEAPON);

		if (!addTime)
		{
			addTime = weaponData[pm->ps->weapon].fireTime;
		}
	}

	if (pm->ps->fd.forcePowersActive & (1 << FP_RAGE))
	{
		addTime *= 0.75;
	}
	else if (pm->ps->fd.forceRageRecoveryTime > pm->cmd.serverTime)
	{
		addTime *= 1.5;
	}

	if (!PM_InCartwheel(pm->ps->torsoAnim))
	{//can still attack during a cartwheel/arial
		pm->ps->weaponTime = addTime;
	}
}

//Add Fatigue to a player
void BG_AddFatigue(playerState_t * ps, int Fatigue)
{
	//For now, all saber attacks cost one FP.
	if (ps->fd.forcePower > Fatigue)
	{
		ps->fd.forcePower -= Fatigue;
	}
	else
	{//don't have enough so just completely drain FP then.
		ps->fd.forcePower = 0;
	}

	if (ps->fd.forcePower <= (ps->fd.forcePowerMax * FATIGUEDTHRESHHOLD))
	{//Pop the Fatigued flag
		ps->userInt3 |= (1 << FLAG_FATIGUED);
	}
}

//Add Fatigue to a player
void PM_AddBlockFatigue(playerState_t * ps, int Fatigue)
{
	//For now, all saber attacks cost one BP.
	if (ps->fd.blockPoints > Fatigue)
	{
		ps->fd.blockPoints -= Fatigue;
	}
	else
	{//don't have enough so just completely drain FP then.
		ps->fd.blockPoints = 0;
	}

	if (ps->fd.blockPoints < BLOCKPOINTS_HALF)
	{
		ps->userInt3 |= (1 << FLAG_BLOCKDRAINED);
	}

	if (ps->fd.blockPoints <= (ps->fd.blockPointsMax * FATIGUEDTHRESHHOLD))
	{//Pop the Fatigued flag
		ps->userInt3 |= (1 << FLAG_FATIGUED);
	}
}

int Fatigue_SaberAttack(playerState_t * ps)
{//returns the FP cost for a saber attack by this player.

	return FATIGUE_SABERATTACK;
}

extern qboolean PM_SaberInAttackPure(int move);
extern saberMoveName_t PM_BrokenParryForAttack(int move);
//Add Fatigue for all the sabermoves.
void BG_BlockFatigue(playerState_t * ps, int newMove, int anim)
{
	if (ps->saberMove != newMove)
	{//wasn't playing that attack before
		if (BG_KickMove(newMove))
		{//melee move
			BG_AddFatigue(ps, FATIGUE_MELEE);
		}
		else if (PM_SaberInAttackPure(newMove))
		{//simple saber attack
			PM_AddBlockFatigue(ps, Fatigue_SaberAttack(ps));
		}
		else if (PM_BrokenParryForParry(newMove))
		{//simple saber attack
			PM_AddBlockFatigue(ps, FATIGUE_BLOCKED);
		}
		else if (PM_BrokenParryForAttack(newMove))
		{//simple saber attack
			PM_AddBlockFatigue(ps, FATIGUE_BLOCKED);
		}
		else if (PM_KnockawayForParry(newMove))
		{//simple saber attack
			PM_AddBlockFatigue(ps, FATIGUE_BLOCKED);
		}
		else if (PM_AbsorbtheParry(newMove))
		{//simple saber attack
			PM_AddBlockFatigue(ps, FATIGUE_BLOCKED);
		}
		else if (PM_SaberInbackblock(newMove))
		{//simple back block
			PM_AddBlockFatigue(ps, FATIGUE_BACKBLOCK);
		}
		else if (PM_SaberInTransition(newMove) && pm->ps->userInt3& (1 << FLAG_ATTACKFAKE))
		{//attack fakes cost FP as well
			if (ps->fd.saberAnimLevel == SS_DUAL)
			{
				//dual sabers don't have transition/FP costs.
			}
			else
			{//single sabers
				PM_AddBlockFatigue(ps, FATIGUE_SABERTRANS);
			}
		}
	}

	return;
}

void BG_SaberFatigue(playerState_t * ps, int newMove, int anim)
{
	if (ps->saberMove != newMove)
	{//wasn't playing that attack before
		if (BG_KickMove(newMove))
		{//melee move
			BG_AddFatigue(ps, FATIGUE_MELEE);
		}
		else if (PM_SaberInAttackPure(newMove))
		{//simple saber attack
			BG_AddFatigue(ps, Fatigue_SaberAttack(ps));
		}
		else if (PM_SaberInTransition(newMove) && pm->ps->userInt3& (1 << FLAG_ATTACKFAKE))
		{//attack fakes cost FP as well
			if (ps->fd.saberAnimLevel == SS_DUAL)
			{
				//dual sabers don't have transition/FP costs.
			}
			else
			{//single sabers
				BG_AddFatigue(ps, FATIGUE_SABERTRANS);
			}
		}
	}
	else if ((ps->saberMove != newMove) && !(pm->cmd.buttons & BUTTON_BLOCK) && !(pm->ps->ManualBlockingFlags & (1 << MBF_BLOCKING)) && !(pm->ps->ManualBlockingFlags & (1 << MBF_MBLOCKING)))
	{//wasn't playing that attack before
		if (PM_BrokenParryForParry(newMove))
		{//simple saber attack
			PM_AddBlockFatigue(ps, FATIGUE_AUTOSABERDEFENSE);
		}
		else if (PM_BrokenParryForAttack(newMove))
		{//simple saber attack
			PM_AddBlockFatigue(ps, FATIGUE_AUTOSABERDEFENSE);
		}
		else if (PM_KnockawayForParry(newMove))
		{//simple saber attack
			PM_AddBlockFatigue(ps, FATIGUE_AUTOSABERDEFENSE);
		}
		else if (PM_AbsorbtheParry(newMove))
		{//simple saber attack
			PM_AddBlockFatigue(ps, FATIGUE_AUTOSABERDEFENSE);
		}
		else if (PM_SaberInbackblock(newMove))
		{//simple back block
			PM_AddBlockFatigue(ps, FATIGUE_AUTOSABERDEFENSE);
		}
	}

	return;
}

void PM_SaberFakeFlagUpdate(playerState_t * ps, int newMove, int currentMove);

void PM_SetJumped(float height, qboolean force)
{
	pm->ps->velocity[2] = height;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pm->ps->pm_flags |= PMF_JUMP_HELD;
	pm->ps->pm_flags |= PMF_JUMPING;
	pm->cmd.upmove = 0;

	if (force)
	{
		pm->ps->fd.forceJumpZStart = pm->ps->origin[2];
		pm->ps->pm_flags |= PMF_SLOW_MO_FALL;
		pm->ps->fd.forcePowersActive |= (1 << FP_LEVITATION);
	}
	else
	{
		PM_AddEvent(EV_JUMP);
	}
}

void PM_SetSaberMove(saberMoveName_t newMove)
{
	unsigned int setflags = saberMoveData[newMove].animSetFlags;
	int	anim = saberMoveData[newMove].animToUse;
	int parts = SETANIM_TORSO;

	saberInfo_t* saber1;
	saberInfo_t* saber2;

	saber1 = BG_MySaber(pm->ps->clientNum, 0);
	saber2 = BG_MySaber(pm->ps->clientNum, 1);

	if (newMove == LS_A_FLIP_STAB || newMove == LS_A_FLIP_SLASH)
	{//finished with a kata (or in a special move) reset attack counter
		pm->ps->saberAttackChainCount = MISHAPLEVEL_LIGHT;
	}
	else if (PM_SaberInAttack(newMove))
	{//continuing with a kata, increment attack counter
		pm->ps->saberAttackChainCount++;
	}

	if (pm->ps->saberAttackChainCount > MISHAPLEVEL_OVERLOAD)
	{ //for the sake of being able to send the value over the net within a reasonable bit count
		pm->ps->saberAttackChainCount = MISHAPLEVEL_OVERLOAD;
	}

	if (pm->ps->fd.blockPoints > BLOCK_POINTS_MAX)
	{ //for the sake of being able to send the value over the net within a reasonable bit count
		pm->ps->fd.blockPoints = BLOCK_POINTS_MAX;
	}

	if (pm->ps->fd.blockPoints < BLOCK_POINTS_MIN)
	{ //for the sake of being able to send the value over the net within a reasonable bit count
		pm->ps->fd.blockPoints = BLOCK_POINTS_MIN;
	}

	if (pm->ps->fd.blockPoints < BLOCKPOINTS_HALF)
	{
		pm->ps->userInt3 |= (1 << FLAG_BLOCKDRAINED);
	}
	else if (pm->ps->fd.blockPoints > BLOCKPOINTS_HALF)
	{ //CANCEL THE BLOCKDRAINED FLAG
		pm->ps->userInt3 &= ~(1 << FLAG_BLOCKDRAINED);
	}

	if (newMove == LS_DRAW || newMove == LS_DRAW2 || newMove == LS_DRAW3)
	{
		if (saber1
			&& saber1->drawAnim != -1)
		{
			anim = saber1->drawAnim;
		}
		else if (saber2
			&& saber2->drawAnim != -1)
		{
			anim = saber2->drawAnim;
		}
		else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
		{
			if (saber1->type == SABER_BACKHAND)
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_SABER_BACKHAND_IGNITION, AFLAG_PACE);
			}
			else if (saber1->type == SABER_ASBACKHAND)
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_SABER_BACKHAND_IGNITION, AFLAG_PACE);
			}
			else if (saber1->type == SABER_STAFF_MAUL)
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_S1_S7, AFLAG_PACE);
			}
			else
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_S1_S7, AFLAG_PACE);
			}
		}
		else if (pm->ps->fd.saberAnimLevel == SS_DUAL)
		{
			if (saber1->type == SABER_GRIE || saber1->type == SABER_GRIE4)
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_GRIEVOUS_SABERON, AFLAG_PACE);
			}
			else
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_S1_S6, AFLAG_PACE);
			}
		}
	}
	else if (newMove == LS_PUTAWAY)
	{
		if (saber1
			&& saber1->putawayAnim != -1)
		{
			anim = saber1->putawayAnim;
		}
		else if (saber2
			&& saber2->putawayAnim != -1)
		{
			anim = saber2->putawayAnim;
		}
		else if (pm->ps->fd.saberAnimLevel == SS_STAFF)
		{
			PM_SetAnim(SETANIM_TORSO, BOTH_S7_S1, AFLAG_PACE);
		}
		else if (pm->ps->fd.saberAnimLevel == SS_DUAL)
		{
			if (saber1->type == SABER_GRIE || saber1->type == SABER_GRIE4)
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_GRIEVOUS_SABERON, AFLAG_PACE);
			}
			else
			{
				PM_SetAnim(SETANIM_TORSO, BOTH_S6_S1, AFLAG_PACE);
			}
		}
	}
	//different styles use different animations for the DFA move.
	else if (newMove == LS_A_JUMP_T__B_ && pm->ps->fd.saberAnimLevel == SS_DESANN)
	{
		anim = BOTH_FJSS_TR_BL;
	}
	else if (pm->ps->fd.saberAnimLevel == SS_STAFF && newMove >= LS_S_TL2BR && newMove < LS_REFLECT_LL)
	{//staff has an entirely new set of anims, besides special attacks
		if (newMove >= LS_V1_BR && newMove <= LS_REFLECT_LL)
		{//there aren't 1-7, just 1, 6 and 7, so just set it
			anim = BOTH_P7_S7_T_ + (anim - BOTH_P1_S1_T_);//shift it up to the proper set
		}
		else
		{//add the appropriate animLevel
			anim += (pm->ps->fd.saberAnimLevel - FORCE_LEVEL_1) * SABER_ANIM_GROUP_SIZE;
		}
	}
	else if (pm->ps->fd.saberAnimLevel == SS_DUAL && newMove >= LS_S_TL2BR && newMove < LS_REFLECT_LL)
	{ //akimbo has an entirely new set of anims, besides special attacks
		if (newMove >= LS_V1_BR && newMove <= LS_REFLECT_LL)
		{//there aren't 1-7, just 1, 6 and 7, so just set it
			anim = BOTH_P6_S6_T_ + (anim - BOTH_P1_S1_T_);//shift it up to the proper set
		}
		else if ((newMove == LS_A_R2L || newMove == LS_S_R2L
			|| newMove == LS_A_L2R || newMove == LS_S_L2R)
			&& PM_CanDoDualDoubleAttacks()
			&& PM_CheckEnemyPresence(DIR_RIGHT, 150.0f)
			&& PM_CheckEnemyPresence(DIR_LEFT, 150.0f))
		{//enemy both on left and right
			newMove = LS_DUAL_LR;
			anim = saberMoveData[newMove].animToUse;
			//probably already moved, but...
			pm->cmd.rightmove = 0;
		}
		else if ((newMove == LS_A_T2B || newMove == LS_S_T2B
			|| newMove == LS_A_BACK || newMove == LS_A_BACK_CR)
			&& PM_CanDoDualDoubleAttacks()
			&& PM_CheckEnemyPresence(DIR_FRONT, 150.0f)
			&& PM_CheckEnemyPresence(DIR_BACK, 150.0f))
		{//enemy both in front and back
			newMove = LS_DUAL_FB;
			anim = saberMoveData[newMove].animToUse;
			//probably already moved, but...
			pm->cmd.forwardmove = 0;
		}
		else
		{//add the appropriate animLevel
			anim += (pm->ps->fd.saberAnimLevel - FORCE_LEVEL_1) * SABER_ANIM_GROUP_SIZE;
		}
	}
	else if (pm->ps->fd.saberAnimLevel > FORCE_LEVEL_1 &&
		!PM_SaberInIdle(newMove) && !PM_SaberInParry(newMove) && !PM_SaberInKnockaway(newMove) && !PM_SaberInBrokenParry(newMove) && !PM_SaberInReflect(newMove) && !PM_SaberInSpecial(newMove))
	{//readies, parries and reflections have only 1 level
		if (saber1->type == SABER_LANCE || saber1->type == SABER_TRIDENT)
		{
			//FIXME: hack for now - these use the fast anims, but slowed down.  Should have own style
		}
		else
		{//increment the anim to the next level of saber anims
			anim += (pm->ps->fd.saberAnimLevel - FORCE_LEVEL_1) * SABER_ANIM_GROUP_SIZE;
		}
	}
	else if (newMove == LS_KICK_F_AIR
		|| newMove == LS_KICK_F_AIR2
		|| newMove == LS_KICK_B_AIR
		|| newMove == LS_KICK_R_AIR
		|| newMove == LS_KICK_L_AIR)
	{
		if (pm->ps->groundEntityNum != ENTITYNUM_NONE)
		{
			PM_SetJumped(200, qtrue);
		}
	}

	// If the move does the same animation as the last one, we need to force a restart...
	if (saberMoveData[pm->ps->saberMove].animToUse == anim && newMove > LS_PUTAWAY)
	{
		setflags |= SETANIM_FLAG_RESTART;
	}

	//saber torso anims should always be highest priority (4/12/02 - for special anims only)
	if (!pm->ps->m_iVehicleNum)
	{ //if not riding a vehicle
		if (PM_SaberInSpecial(newMove))
		{
			setflags |= SETANIM_FLAG_OVERRIDE;
		}
	}
	if (PM_InSaberStandAnim(anim) || anim == BOTH_STAND1)
	{
		anim = (pm->ps->legsAnim);

		if ((anim >= BOTH_STAND1 && anim <= BOTH_STAND4TOATTACK2) ||
			(anim >= TORSO_DROPWEAP1 && anim <= TORSO_WEAPONIDLE10))
		{ //If standing then use the special saber stand anim
#ifdef _GAME
			if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
			{
				anim = PM_ReadyPoseForSaberAnimLevelBOT();
			}
			else
			{
				anim = PM_ReadyPoseForSaberAnimLevel();
			}
#endif
		}

		if (pm->ps->pm_flags & PMF_DUCKED)
		{ //Playing torso walk anims while crouched makes you look like a monkey
			anim = PM_ReadyPoseForSaberAnimLevelDucked();
		}

		if (anim == BOTH_WALKBACK1 || anim == BOTH_WALKBACK2 || anim == BOTH_WALK1)
		{ //normal stance when walking backward so saber doesn't look like it's cutting through leg
#ifdef _GAME
			if (g_entities[pm->ps->clientNum].r.svFlags & SVF_BOT || pm_entSelf->s.eType == ET_NPC)
			{
				anim = PM_ReadyPoseForSaberAnimLevelBOT();
			}
			else
			{
				anim = PM_ReadyPoseForSaberAnimLevel();
			}
#endif
		}

		if (PM_InSlopeAnim(anim))
		{
#ifdef _GAME
			if (g_entities[pm->ps->clientNum].r.svFlags& SVF_BOT || pm_entSelf->s.eType == ET_NPC)
			{// Some special bot stuff.
				anim = PM_ReadyPoseForSaberAnimLevelBOT();
			}
			else
			{
				anim = PM_ReadyPoseForSaberAnimLevel();
			}
#endif
		}

		parts = SETANIM_TORSO;
	}

	if (!pm->ps->m_iVehicleNum)
	{ //if not riding a vehicle
		if (newMove == LS_JUMPATTACK_ARIAL_RIGHT ||
			newMove == LS_JUMPATTACK_ARIAL_LEFT)
		{ //force only on legs
			parts = SETANIM_LEGS;
		}
		else if (newMove == LS_A_LUNGE
			|| newMove == LS_A_JUMP_T__B_
			|| newMove == LS_A_JUMP_PALP_
			|| newMove == LS_A_BACKSTAB
			|| newMove == LS_A_BACKSTAB_B
			|| newMove == LS_A_BACK
			|| newMove == LS_A_BACK_CR
			|| newMove == LS_ROLL_STAB
			|| newMove == LS_A_FLIP_STAB
			|| newMove == LS_A_FLIP_SLASH
			|| newMove == LS_JUMPATTACK_DUAL
			|| newMove == LS_GRIEVOUS_LUNGE
			|| newMove == LS_JUMPATTACK_ARIAL_LEFT
			|| newMove == LS_JUMPATTACK_ARIAL_RIGHT
			|| newMove == LS_JUMPATTACK_CART_LEFT
			|| newMove == LS_JUMPATTACK_CART_RIGHT
			|| newMove == LS_JUMPATTACK_STAFF_LEFT
			|| newMove == LS_JUMPATTACK_STAFF_RIGHT
			|| newMove == LS_BUTTERFLY_LEFT
			|| newMove == LS_BUTTERFLY_RIGHT
			|| newMove == LS_A_BACKFLIP_ATK
			|| newMove == LS_STABDOWN
			|| newMove == LS_STABDOWN_BACKHAND
			|| newMove == LS_STABDOWN_STAFF
			|| newMove == LS_STABDOWN_DUAL
			|| newMove == LS_DUAL_SPIN_PROTECT
			|| newMove == LS_DUAL_SPIN_PROTECT_GRIE
			|| newMove == LS_STAFF_SOULCAL
			|| newMove == LS_YODA_SPECIAL
			|| newMove == LS_A1_SPECIAL
			|| newMove == LS_A2_SPECIAL
			|| newMove == LS_A3_SPECIAL
			|| newMove == LS_A4_SPECIAL
			|| newMove == LS_A5_SPECIAL
			|| newMove == LS_GRIEVOUS_SPECIAL
			|| newMove == LS_UPSIDE_DOWN_ATTACK
			|| newMove == LS_PULL_ATTACK_STAB
			|| newMove == LS_PULL_ATTACK_SWING
			|| PM_SaberInBrokenParry(newMove)
			|| BG_KickMove(newMove))
		{
			parts = SETANIM_BOTH;
		}
		else if (PM_SpinningSaberAnim(anim))
		{//spins must be played on entire body
			parts = SETANIM_BOTH;
		}
		//coming out of a spin, force full body setting
		else if (PM_SpinningSaberAnim(pm->ps->legsAnim))
		{//spins must be played on entire body
			parts = SETANIM_BOTH;
			pm->ps->legsTimer = pm->ps->torsoTimer = 0;
		}
		else if ((!pm->cmd.forwardmove && !pm->cmd.rightmove && !pm->cmd.upmove && !(pm->ps->pm_flags&PMF_DUCKED)) || pm->ps->speed == 0)
		{//not trying to run, duck or jump
			if (!PM_FlippingAnim(pm->ps->legsAnim) &&
				!BG_InRoll(pm->ps, pm->ps->legsAnim) &&
				!PM_InKnockDown(pm->ps) &&
				!PM_JumpingAnim(pm->ps->legsAnim) &&
				!PM_InSpecialJump(pm->ps->legsAnim) &&
				anim != PM_ReadyPoseForSaberAnimLevel() &&
				anim != PM_ReadyPoseForSaberAnimLevelBOT() &&
				anim != PM_ReadyPoseForSaberAnimLevelDucked() &&
				pm->ps->groundEntityNum != ENTITYNUM_NONE &&
				!(pm->ps->pm_flags & PMF_DUCKED))
			{
				parts = SETANIM_BOTH;
			}
			else if (!(pm->ps->pm_flags& PMF_DUCKED)
				&& (newMove == LS_SPINATTACK_DUAL || newMove == LS_SPINATTACK || newMove == LS_SPINATTACK_GRIEV || newMove == LS_GRIEVOUS_SPECIAL))
			{
				parts = SETANIM_BOTH;
			}
		}

		PM_SetAnim(parts, anim, setflags);

		if (parts != SETANIM_LEGS &&
			(pm->ps->legsAnim == BOTH_ARIAL_LEFT ||
				pm->ps->legsAnim == BOTH_ARIAL_RIGHT))
		{
			if (pm->ps->legsTimer > pm->ps->torsoTimer)
			{
				pm->ps->legsTimer = pm->ps->torsoTimer;
			}
		}

	}

	if ((pm->ps->torsoAnim) == anim)
	{//successfully changed anims
	 //special check for *starting* a saber swing
		BG_SaberFatigue(pm->ps, newMove, anim);

		if (pm->ps->weapon == WP_SABER && (pm->ps->ManualBlockingFlags & (1 << MBF_BLOCKING) || pm->ps->ManualBlockingFlags & (1 << MBF_MBLOCKING)))
		{
			BG_BlockFatigue(pm->ps, newMove, anim);//drainblockpoints
		}
		//update the attack fake flag
		PM_SaberFakeFlagUpdate(pm->ps, newMove, anim);

		if (!PM_SaberInBounce(newMove) && !PM_SaberInReturn(newMove)) //or new move isn't slowbounce move
		{//switched away from a slow bounce move, remove the flags.
			pm->ps->userInt3 &= ~(1 << FLAG_SLOWBOUNCE);
			pm->ps->userInt3 &= ~(1 << FLAG_OLDSLOWBOUNCE);
			pm->ps->userInt3 &= ~(1 << FLAG_PARRIED);
			pm->ps->userInt3 &= ~(1 << FLAG_BLOCKING);
			pm->ps->userInt3 &= ~(1 << FLAG_QUICKPARRY);
			pm->ps->userInt3 &= ~(1 << FLAG_BLOCKEDBOUNCE);
		}

		if (!PM_SaberInParry(newMove))
		{//cancel out pre-block flag
			pm->ps->userInt3 &= ~(1 << FLAG_PREBLOCK);
		}

		if (PM_SaberInAttack(newMove) || PM_SaberInSpecialAttack(anim))
		{
			if (pm->ps->saberMove != newMove)
			{//wasn't playing that attack before
				if (newMove != LS_KICK_F
					&& newMove != LS_KICK_F2
					&& newMove != LS_KICK_B
					&& newMove != LS_KICK_B2
					&& newMove != LS_KICK_B3
					&& newMove != LS_KICK_R
					&& newMove != LS_SLAP_R
					&& newMove != LS_KICK_L
					&& newMove != LS_SLAP_L
					&& newMove != LS_KICK_F_AIR
					&& newMove != LS_KICK_F_AIR2
					&& newMove != LS_KICK_B_AIR
					&& newMove != LS_KICK_R_AIR
					&& newMove != LS_KICK_L_AIR)
				{
					PM_AddEvent(EV_SABER_ATTACK);
				}

				if (pm->ps->brokenLimbs)
				{ //randomly make pain sounds with a broken arm because we are suffering.
					int iFactor = -1;

					if (pm->ps->brokenLimbs & (1 << BROKENLIMB_RARM))
					{ //You're using it more. So it hurts more.
						iFactor = 5;
					}
					else if (pm->ps->brokenLimbs & (1 << BROKENLIMB_LARM))
					{
						iFactor = 10;
					}

					if (iFactor != -1)
					{
						if (!PM_irand_timesync(0, iFactor))
						{
							BG_AddPredictableEventToPlayerstate(EV_PAIN, PM_irand_timesync(1, 100), pm->ps);
						}
					}
				}
			}
		}

		if (PM_SaberInSpecial(newMove) &&
			pm->ps->weaponTime < pm->ps->torsoTimer)
		{ //rww 01-02-03 - I think this will solve the issue of special attacks being interruptable, hopefully without side effects
			pm->ps->weaponTime = pm->ps->torsoTimer;
		}

		pm->ps->saberMove = newMove;
		pm->ps->saberBlocking = saberMoveData[newMove].blocking;

		pm->ps->torsoAnim = anim;

		if (pm->ps->weaponTime <= 0)
		{
			pm->ps->saberBlocked = BLOCKED_NONE;
		}
	}
}

saberInfo_t* BG_MySaber(int clientNum, int saberNum)
{
	//returns a pointer to the requested saberNum
#ifdef _GAME
	gentity_t* ent = &g_entities[clientNum];
	if (ent->inuse && ent->client)
	{
		if (!ent->client->saber[saberNum].model[0])
		{ //don't have saber anymore!
			return NULL;
		}
		return &ent->client->saber[saberNum];
	}
#elif defined(_CGAME)
	clientInfo_t* ci = NULL;
	if (clientNum < MAX_CLIENTS)
	{
		ci = &cgs.clientinfo[clientNum];
	}
	else
	{
		centity_t* cent = &cg_entities[clientNum];
		if (cent->npcClient)
		{
			ci = cent->npcClient;
		}
	}
	if (ci
		&& ci->infoValid)
	{
		if (!ci->saber[saberNum].model[0])
		{ //don't have sabers anymore!
			return NULL;
		}
		return &ci->saber[saberNum];
	}
#endif

	return NULL;
}

qboolean PM_DoKick(void)
{//perform a kick.
	int kickMove = -1;

	if (!BG_KickingAnim(pm->ps->torsoAnim) &&
		!BG_KickingAnim(pm->ps->legsAnim) &&
		!BG_InRoll(pm->ps, pm->ps->legsAnim) &&
		pm->ps->weaponTime <= 0)
	{//player kicks
		kickMove = PM_KickMoveForConditions();
	}

	if (kickMove != -1)
	{
		if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
		{//if in air, convert kick to an in-air kick
			float gDist = PM_GroundDistance();
			//also looks wrong to transition from a non-complete flip anim...
			if ((!PM_FlippingAnim(pm->ps->legsAnim) || pm->ps->legsTimer <= 0) &&
				gDist > 64.0f && //strict minimum
				gDist > (-pm->ps->velocity[2]) - 64.0f //make sure we are high to ground relative to downward velocity as well
				)
			{
				switch (kickMove)
				{
				case LS_KICK_F:
					kickMove = LS_KICK_F_AIR2;
					break;
				case LS_KICK_B:
					kickMove = LS_KICK_B_AIR;
					break;
				case LS_KICK_R:
					kickMove = LS_KICK_R_AIR;
					break;
				case LS_KICK_L:
					kickMove = LS_KICK_L_AIR;
					break;
				default: //oh well, can't do any other kick move while in-air
					kickMove = -1;
					break;
				}
			}
			else
			{//leave it as a normal kick unless we're too high up
				if (gDist > 128.0f || pm->ps->velocity[2] >= 0)
				{ //off ground, but too close to ground
					kickMove = -1;
				}
			}
		}

		if (kickMove != -1 && BG_EnoughForcePowerForMove(FATIGUE_SABERATTACK))
		{
			PM_SetSaberMove(kickMove);
			return qtrue;
		}
	}

	return qfalse;
}

qboolean PM_DoSlap(void)
{//perform a kick.
	int kickMove = -1;

	if (!BG_KickingAnim(pm->ps->torsoAnim) &&
		!BG_KickingAnim(pm->ps->legsAnim) &&
		!BG_InRoll(pm->ps, pm->ps->legsAnim) &&
		pm->ps->weaponTime <= 0)
	{//player kicks
		kickMove = PM_MeleeMoveForConditions();
	}

	if (kickMove != -1)
	{
		if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
		{//if in air, convert kick to an in-air kick
			float gDist = PM_GroundDistance();
			//also looks wrong to transition from a non-complete flip anim...
			if ((!PM_FlippingAnim(pm->ps->legsAnim) || pm->ps->legsTimer <= 0) &&
				gDist > 64.0f && //strict minimum
				gDist > (-pm->ps->velocity[2]) - 64.0f) //make sure we are high to ground relative to downward velocity as well
			{
				switch (kickMove)
				{
				case LS_KICK_F2:
					kickMove = LS_KICK_F_AIR;
					break;
				case LS_KICK_B2:
					kickMove = LS_KICK_B_AIR;
					break;
				case LS_SLAP_R:
					kickMove = LS_KICK_R_AIR;
					break;
				case LS_SLAP_L:
					kickMove = LS_KICK_L_AIR;
					break;
				default: //oh well, can't do any other kick move while in-air
					kickMove = -1;
					break;
				}
			}
			else
			{//leave it as a normal kick unless we're too high up
				if (gDist > 128.0f || pm->ps->velocity[2] >= 0)
				{ //off ground, but too close to ground
					kickMove = -1;
				}
			}
		}

		if (kickMove != -1 && BG_EnoughForcePowerForMove(FATIGUE_SABERATTACK))
		{
			PM_SetSaberMove(kickMove);
			return qtrue;
		}
	}

	return qfalse;
}

void PM_SaberFakeFlagUpdate(playerState_t * ps, int newMove, int currentMove)
{//checks to see if the attack fake flag needs to be removed.
	if (!PM_SaberInTransition(newMove) && !PM_SaberInStart(newMove) && !PM_SaberInAttackPure(newMove))
	{//not going into an attack move, clear the flag
		pm->ps->userInt3 &= ~(1 << FLAG_ATTACKFAKE);
	}
}

//saber status utility tools
qboolean BG_SaberInFullDamageMove(playerState_t * ps, int AnimIndex)
{//The player is attacking with a saber attack that does full damage
	if ((PM_SaberInAttack(ps->saberMove) &&
		!BG_KickMove(ps->saberMove) &&
		!BG_InSaberLock(ps->torsoAnim))
		|| PM_SuperBreakWinAnim(ps->torsoAnim))
	{//in attack animation
		if (ps->saberBlocked == BLOCKED_NONE)
		{//and not attempting to do some sort of block animation
			return qtrue;
		}
	}
	return qfalse;
}

qboolean BG_SaberInTransitionDamageMove(playerState_t * ps)
{//player is in a saber move where it does transitional damage
	if (PM_SaberInTransition(ps->saberMove))
	{
		if (ps->saberBlocked == BLOCKED_NONE)
		{//and not attempting to do some sort of block animation
			return qtrue;
		}
	}
	return qfalse;
}


qboolean BG_SaberInNonIdleDamageMove(playerState_t * ps, int AnimIndex)
{//player is in a saber move that does something more than idle saber damage
	return BG_SaberInFullDamageMove(ps, AnimIndex);
}


extern qboolean PM_BounceAnim(int anim);
extern qboolean PM_SaberReturnAnim(int anim);

qboolean BG_InSlowBounce(playerState_t * ps)
{//checks for a bounce/return animation in combination with the slow bounce flag
	if (ps->userInt3& (1 << FLAG_SLOWBOUNCE)
		&& (PM_BounceAnim(ps->torsoAnim) || PM_SaberReturnAnim(ps->torsoAnim)))
	{//in slow bounce
		return qtrue;
	}
	if (PM_SaberInBounce(ps->saberMove))
	{//in slow bounce
		return qtrue;
	}
	return qfalse;
}

qboolean BG_InBlockedAttackBounce(playerState_t * ps)
{//checks for the slow bounce flag
	if ((ps->userInt3& (1 << FLAG_BLOCKEDBOUNCE)) || (ps->userInt3& (1 << FLAG_SLOWBOUNCE)))
	{//in slow bounce
		return qtrue;
	}
	return qfalse;
}






