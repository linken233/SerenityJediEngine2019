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

// BG_PAnimate.c
// game and cgame, NOT ui


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
#include "anims.h"
#include "cgame/animtable.h"

#ifdef _GAME
#include "g_local.h"
#elif _CGAME
#include "cgame/cg_local.h"
#elif UI_BUILD
#include "ui/ui_local.h"
#endif

qboolean PM_SaberInTransition(int move);
qboolean PM_SaberInDeflect(int move);
extern qboolean PM_SaberInBounce(int move);
extern qboolean PM_SaberInBrokenParry(int move);
extern saberInfo_t *BG_MySaber(int clientNum, int saberNum);
/*
==============================================================================
BEGIN: Animation utility functions (sequence checking)
==============================================================================
*/
//Called regardless of pm validity:

// VVFIXME - Most of these functions are totally stateless and stupid. Don't
// need multiple copies of this, but it's much easier (and less likely to
// break in the future) if I keep separate namespace versions now.

qboolean PM_SaberStanceAnim(int anim)
{
	switch (anim)
	{
	case BOTH_STAND1://not really a saberstance anim, actually... "saber off" stance
	case BOTH_STAND2://single-saber, medium style
	case BOTH_SABERFAST_STANCE://single-saber, fast style
	case BOTH_SABERSLOW_STANCE://single-saber, strong style
	case BOTH_SABERSTAFF_STANCE://saber staff style
	case BOTH_SABERSTAFF_STANCE_IDLE://saber staff style
	case BOTH_SABERDUAL_STANCE://dual saber style
	case BOTH_SABERDUAL_STANCE_GRIEVOUS://dual saber style
	case BOTH_SABERDUAL_STANCE_IDLE://dual saber style
	case BOTH_SABERTAVION_STANCE://tavion saberstyle
	case BOTH_SABERDESANN_STANCE://desann saber style
	case BOTH_SABERSTANCE_STANCE://similar to stand2
	case BOTH_SABERYODA_STANCE://YODA saber style
	case BOTH_SABERBACKHAND_STANCE://BACKHAND saber style
	case BOTH_SABERDUAL_STANCE_ALT://alt dual
	case BOTH_SABERSTAFF_STANCE_ALT://alt staff
	case BOTH_SABERSTANCE_STANCE_ALT://alt idle
	case BOTH_SABEROBI_STANCE://obiwan
	case BOTH_SABEREADY_STANCE://ready
	case BOTH_SABER_REY_STANCE://rey
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_RestAnim(int anim)
{
	switch (anim)
	{
	case BOTH_MEDITATE:			// default taunt
	case BOTH_MEDITATE1:			// default taunt
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_CrouchAnim(int anim)
{
	switch (anim)
	{
	case BOTH_SIT1:				//# Normal chair sit.
	case BOTH_SIT2:				//# Lotus position.
	case BOTH_SIT3:				//# Sitting in tired position: elbows on knees
	case BOTH_CROUCH1:			//# Transition from standing to crouch
	case BOTH_CROUCH1IDLE:		//# Crouching idle
	case BOTH_CROUCH1WALK:		//# Walking while crouched
	case BOTH_CROUCH1WALKBACK:	//# Walking while crouched
	case BOTH_CROUCH2:			//# Transition from standing to crouch
	case BOTH_CROUCH2TOSTAND1:	//# going from crouch2 to stand1
	case BOTH_CROUCH3:			//# Desann crouching down to Kyle (cin 9)
	case BOTH_KNEES1:			//# Tavion on her knees
	case BOTH_CROUCHATTACKBACK1://FIXME: not if in middle of anim?
	case BOTH_ROLL_STAB:
	case BOTH_STAND_TO_KNEEL:
	case BOTH_KNEEL_TO_STAND:
	case BOTH_TURNCROUCH1:
	case BOTH_CROUCH4:
	case BOTH_KNEES2:			//# Tavion on her knees looking down
	case BOTH_KNEES2TO1:		//# Transition of KNEES2 to KNEES1
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_CrouchingAnim(int anim)
{
	switch (anim)
	{
	case BOTH_CROUCH1:			//# Transition from standing to crouch
	case BOTH_CROUCH2:			//# Transition from standing to crouch
	case BOTH_CROUCH1IDLE:		//# Crouching idle
	case BOTH_CROUCH1WALK:		//# Walking while crouched
	case BOTH_CROUCH1WALKBACK:	//# Walking while crouched
	case BOTH_CROUCH2TOSTAND1:	//# going from crouch2 to stand1
	case BOTH_CROUCH3:			//# Desann crouching down to Kyle (cin 9)
	case BOTH_CROUCHATTACKBACK1://FIXME: not if in middle of anim?
	case BOTH_TURNCROUCH1:
	case BOTH_CROUCH4:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_InSpecialJump(int anim)
{
	switch ((anim))
	{
	case BOTH_WALL_RUN_RIGHT:
	case BOTH_WALL_RUN_RIGHT_STOP:
	case BOTH_WALL_RUN_RIGHT_FLIP:
	case BOTH_WALL_RUN_LEFT:
	case BOTH_WALL_RUN_LEFT_STOP:
	case BOTH_WALL_RUN_LEFT_FLIP:
	case BOTH_WALL_FLIP_RIGHT:
	case BOTH_WALL_FLIP_LEFT:
	case BOTH_FLIP_BACK1:
	case BOTH_FLIP_BACK2:
	case BOTH_FLIP_BACK3:
	case BOTH_WALL_FLIP_BACK1:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_FORCELEAP_PALP:
	case BOTH_JUMPFLIPSLASHDOWN1://#
	case BOTH_JUMPFLIPSTABDOWN://#
	case BOTH_JUMPATTACK6:
	case BOTH_GRIEVOUS_LUNGE:
	case BOTH_JUMPATTACK7:
	case BOTH_ARIAL_LEFT:
	case BOTH_ARIAL_RIGHT:
	case BOTH_ARIAL_F1:
	case BOTH_CARTWHEEL_LEFT:
	case BOTH_CARTWHEEL_RIGHT:
	case BOTH_FORCELONGLEAP_START:
	case BOTH_FORCELONGLEAP_ATTACK:
	case BOTH_FORCELONGLEAP_ATTACK2:
	case BOTH_FORCEWALLRUNFLIP_START:
	case BOTH_FORCEWALLRUNFLIP_END:
	case BOTH_FORCEWALLRUNFLIP_ALT:
	case BOTH_FLIP_ATTACK7:
	case BOTH_FLIP_HOLD7:
	case BOTH_FLIP_LAND:
	case BOTH_A7_SOULCAL:
		return qtrue;
	}
	if (BG_InReboundJump(anim))
	{
		return qtrue;
	}
	if (BG_InReboundHold(anim))
	{
		return qtrue;
	}
	if (BG_InReboundRelease(anim))
	{
		return qtrue;
	}
	if (BG_InBackFlip(anim))
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_InSaberStandAnim(int anim)
{
	switch ((anim))
	{
	case BOTH_STAND1://not really a saberstance anim, actually... "saber off" stance
	case BOTH_STAND2://single-saber, medium style
	case BOTH_SABERFAST_STANCE://single-saber, fast style
	case BOTH_SABERSLOW_STANCE://single-saber, strong style
	case BOTH_SABERSTAFF_STANCE://saber staff style
	case BOTH_SABERSTAFF_STANCE_IDLE://saber staff style
	case BOTH_SABERDUAL_STANCE://dual saber style
	case BOTH_SABERDUAL_STANCE_GRIEVOUS://dual saber style
	case BOTH_SABERDUAL_STANCE_IDLE://dual saber style
	case BOTH_SABERTAVION_STANCE://tavion saberstyle
	case BOTH_SABERDESANN_STANCE://desann saber style
	case BOTH_SABERSTANCE_STANCE://similar to stand2
	case BOTH_SABERYODA_STANCE://YODA saber style
	case BOTH_SABERBACKHAND_STANCE://BACKHAND saber style
	case BOTH_SABERDUAL_STANCE_ALT://alt dual
	case BOTH_SABERSTAFF_STANCE_ALT://alt staff
	case BOTH_SABERSTANCE_STANCE_ALT://alt idle
	case BOTH_SABEROBI_STANCE://obiwan
	case BOTH_SABEREADY_STANCE://ready
	case BOTH_SABER_REY_STANCE://rey
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean BG_InReboundJump(int anim)
{
	switch (anim)
	{
	case BOTH_FORCEWALLREBOUND_FORWARD:
	case BOTH_FORCEWALLREBOUND_LEFT:
	case BOTH_FORCEWALLREBOUND_BACK:
	case BOTH_FORCEWALLREBOUND_RIGHT:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean BG_InReboundHold(int anim)
{
	switch (anim)
	{
	case BOTH_FORCEWALLHOLD_FORWARD:
	case BOTH_FORCEWALLHOLD_LEFT:
	case BOTH_FORCEWALLHOLD_BACK:
	case BOTH_FORCEWALLHOLD_RIGHT:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean BG_InReboundRelease(int anim)
{
	switch (anim)
	{
	case BOTH_FORCEWALLRELEASE_FORWARD:
	case BOTH_FORCEWALLRELEASE_LEFT:
	case BOTH_FORCEWALLRELEASE_BACK:
	case BOTH_FORCEWALLRELEASE_RIGHT:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean BG_InLedgeMove(int anim)
{
	switch (anim)
	{
	case BOTH_LEDGE_GRAB:
	case BOTH_LEDGE_HOLD:
	case BOTH_LEDGE_LEFT:
	case BOTH_LEDGE_RIGHT:
	case BOTH_LEDGE_MERCPULL:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean In_LedgeIdle(int anim)
{
	switch (anim)
	{
	case BOTH_LEDGE_GRAB:
	case BOTH_LEDGE_HOLD:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean BG_InBackFlip(int anim)
{
	switch (anim)
	{
	case BOTH_FLIP_BACK1:
	case BOTH_FLIP_BACK2:
	case BOTH_FLIP_BACK3:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean BG_DirectFlippingAnim(int anim)
{
	switch ((anim))
	{
	case BOTH_FLIP_F:			//# Flip forward
	case BOTH_FLIP_B:			//# Flip backwards
	case BOTH_FLIP_L:			//# Flip left
	case BOTH_FLIP_R:			//# Flip right
		return qtrue;
		break;
	}

	return qfalse;
}

qboolean BG_GunInAttack(int anim)
{
	if (pm->cmd.buttons & BUTTON_ATTACK ||
		pm->cmd.buttons & BUTTON_ALT_ATTACK ||
		pm->cmd.buttons & BUTTON_BLOCK ||
		pm->cmd.buttons & BUTTON_FORCEPOWER ||
		pm->cmd.buttons & BUTTON_KICK ||
		pm->cmd.buttons & BUTTON_DASH)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInAttackPure(int move)
{
	if (move >= LS_A_TL2BR && move <= LS_A_T2B)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInAttack(int move)
{
	if (move >= LS_A_TL2BR && move <= LS_A_T2B)
	{
		return qtrue;
	}
	switch (move)
	{
	case LS_A_BACK:
	case LS_A_BACK_CR:
	case LS_A_BACKSTAB:
	case LS_A_BACKSTAB_B:
	case LS_ROLL_STAB:
	case LS_A_LUNGE:
	case LS_A_JUMP_T__B_:
	case LS_A_JUMP_PALP_:
	case LS_A_FLIP_STAB:
	case LS_A_FLIP_SLASH:
	case LS_JUMPATTACK_DUAL:
	case LS_GRIEVOUS_LUNGE:
	case LS_JUMPATTACK_ARIAL_LEFT:
	case LS_JUMPATTACK_ARIAL_RIGHT:
	case LS_JUMPATTACK_CART_LEFT:
	case LS_JUMPATTACK_CART_RIGHT:
	case LS_JUMPATTACK_STAFF_LEFT:
	case LS_JUMPATTACK_STAFF_RIGHT:
	case LS_BUTTERFLY_LEFT:
	case LS_BUTTERFLY_RIGHT:
	case LS_A_BACKFLIP_ATK:
	case LS_SPINATTACK_DUAL:
	case LS_SPINATTACK_GRIEV:
	case LS_SPINATTACK:
	case LS_LEAP_ATTACK:
	case LS_SWOOP_ATTACK_RIGHT:
	case LS_SWOOP_ATTACK_LEFT:
	case LS_TAUNTAUN_ATTACK_RIGHT:
	case LS_TAUNTAUN_ATTACK_LEFT:
	case LS_KICK_F:
	case LS_KICK_F2:
	case LS_KICK_B:
	case LS_KICK_B2:
	case LS_KICK_B3:
	case LS_KICK_R:
	case LS_SLAP_R:
	case LS_KICK_L:
	case LS_SLAP_L:
	case LS_KICK_S:
	case LS_KICK_BF:
	case LS_KICK_RL:
	case LS_KICK_F_AIR:
	case LS_KICK_F_AIR2:
	case LS_KICK_B_AIR:
	case LS_KICK_R_AIR:
	case LS_KICK_L_AIR:
	case LS_STABDOWN:
	case LS_STABDOWN_BACKHAND:
	case LS_STABDOWN_STAFF:
	case LS_STABDOWN_DUAL:
	case LS_DUAL_SPIN_PROTECT:
	case LS_DUAL_SPIN_PROTECT_GRIE:
	case LS_STAFF_SOULCAL:
	case LS_YODA_SPECIAL:
	case LS_A1_SPECIAL:
	case LS_A2_SPECIAL:
	case LS_A3_SPECIAL:
	case LS_A4_SPECIAL:
	case LS_A5_SPECIAL:
	case LS_GRIEVOUS_SPECIAL:
	case LS_UPSIDE_DOWN_ATTACK:
	case LS_PULL_ATTACK_STAB:
	case LS_PULL_ATTACK_SWING:
	case LS_SPINATTACK_ALORA:
	case LS_DUAL_FB:
	case LS_DUAL_LR:
	case LS_HILT_BASH:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean BG_SaberInKata(int saberMove)
{
	switch (saberMove)
	{
	case LS_YODA_SPECIAL:
	case LS_A1_SPECIAL:
	case LS_A2_SPECIAL:
	case LS_A3_SPECIAL:
	case LS_A4_SPECIAL:
	case LS_A5_SPECIAL:
	case LS_GRIEVOUS_SPECIAL:
	case LS_DUAL_SPIN_PROTECT:
	case LS_DUAL_SPIN_PROTECT_GRIE:
	case LS_STAFF_SOULCAL:
		return qtrue;
	}
	return qfalse;
}

qboolean PM_LockedAnim(int anim)
{//anims that can *NEVER* be overridden, regardless
	switch (anim)
	{
	case BOTH_KYLE_PA_1:
	case BOTH_KYLE_PA_2:
	case BOTH_KYLE_PA_3:
	case BOTH_PLAYER_PA_1:
	case BOTH_PLAYER_PA_2:
	case BOTH_PLAYER_PA_3:
	case BOTH_PLAYER_PA_3_FLY:
	case BOTH_TAVION_SCEPTERGROUND:
	case BOTH_TAVION_SWORDPOWER:
	case BOTH_SCEPTER_START:
	case BOTH_SCEPTER_HOLD:
	case BOTH_SCEPTER_STOP:
	case BOTH_GRABBED:	//#
	case BOTH_RELEASED:	//#	when Wampa drops player, transitions into fall on back
	case BOTH_HANG_IDLE:	//#
	case BOTH_HANG_ATTACK:	//#
	case BOTH_HANG_PAIN:	//#
		return qtrue;
	}
	return qfalse;
}

qboolean PM_LightningBlockAnim(int anim)
{//Protect against lightning
	switch (anim)
	{
	case BOTH_WIND:
		return qtrue;
	}
	return qfalse;
}


qboolean BG_Saberinstab(int move)
{
	switch (move)
	{
	case LS_ROLL_STAB:
	case LS_A_BACKSTAB:
	case LS_A_BACKSTAB_B:
	case LS_A_FLIP_STAB:
	case LS_A_FLIP_SLASH:
	case LS_UPSIDE_DOWN_ATTACK:
	case LS_PULL_ATTACK_STAB:
	case LS_PULL_ATTACK_SWING:
	case LS_A_JUMP_T__B_:
	case LS_A_JUMP_PALP_:
		return qtrue;
	}
	return qfalse;
}

int PM_AnimLevelForSaberAnim(int anim)
{
	if (anim >= BOTH_A1_T__B_ && anim <= BOTH_D1_B____)
	{
		return FORCE_LEVEL_1;
	}
	if (anim >= BOTH_A2_T__B_ && anim <= BOTH_D2_B____)
	{
		return FORCE_LEVEL_2;
	}
	if (anim >= BOTH_A3_T__B_ && anim <= BOTH_D3_B____)
	{
		return FORCE_LEVEL_3;
	}
	if (anim >= BOTH_A4_T__B_ && anim <= BOTH_D4_B____)
	{//desann
		return FORCE_LEVEL_4;
	}
	if (anim >= BOTH_A5_T__B_ && anim <= BOTH_D5_B____)
	{//tavion
		return FORCE_LEVEL_5;
	}
	if (anim >= BOTH_A6_T__B_ && anim <= BOTH_D6_B____)
	{//dual
		return SS_DUAL;
	}
	if (anim >= BOTH_A7_T__B_ && anim <= BOTH_D7_B____)
	{//staff
		return SS_STAFF;
	}
	return FORCE_LEVEL_0;
}

qboolean PM_SaberDrawPutawayAnim(int anim)
{
	switch (anim)
	{
	case BOTH_STAND1TO2:
	case BOTH_STAND2TO1:
	case BOTH_S1_S7:
	case BOTH_S7_S1:
	case BOTH_S1_S6:
	case BOTH_S6_S1:
	case BOTH_DOOKU_FULLDRAW:
	case BOTH_DOOKU_SMALLDRAW:
	case BOTH_SABER_IGNITION:
	case BOTH_SABER_BACKHAND_IGNITION:
	case BOTH_GRIEVOUS_SABERON:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_InAnimForSaberMove(int anim, int saberMove)
{
	switch (anim)
	{//special case anims
	case BOTH_A2_STABBACK1:
	case BOTH_A2_STABBACK1B:
	case BOTH_ATTACK_BACK:
	case BOTH_CROUCHATTACKBACK1:
	case BOTH_ROLL_STAB:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_LUNGE2_B__T_:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_FORCELEAP_PALP:
	case BOTH_JUMPFLIPSLASHDOWN1://#
	case BOTH_JUMPFLIPSTABDOWN://#
	case BOTH_JUMPATTACK6:
	case BOTH_GRIEVOUS_LUNGE:
	case BOTH_JUMPATTACK7:
	case BOTH_SPINATTACK6:
	case BOTH_SPINATTACK7:
	case BOTH_SPINATTACKGRIEVOUS:
	case BOTH_VS_ATR_S:
	case BOTH_VS_ATL_S:
	case BOTH_VT_ATR_S:
	case BOTH_VT_ATL_S:
	case BOTH_FORCELONGLEAP_ATTACK:
	case BOTH_A7_KICK_F:
	case BOTH_A7_KICK_F2:
	case BOTH_A7_KICK_B:
	case BOTH_A7_KICK_B2:
	case BOTH_A7_KICK_B3:
	case BOTH_A7_KICK_R:
	case BOTH_A7_SLAP_R:
	case BOTH_A7_SLAP_L:
	case BOTH_A7_KICK_L:
	case BOTH_A7_KICK_S:
	case BOTH_A7_KICK_BF:
	case BOTH_A7_KICK_RL:
	case BOTH_A7_KICK_F_AIR:
	case BOTH_A7_KICK_F_AIR2:
	case BOTH_A7_KICK_B_AIR:
	case BOTH_A7_KICK_R_AIR:
	case BOTH_A7_KICK_L_AIR:
	case BOTH_STABDOWN:
	case BOTH_STABDOWN_BACKHAND:
	case BOTH_STABDOWN_STAFF:
	case BOTH_STABDOWN_DUAL:
	case BOTH_A6_SABERPROTECT:
	case BOTH_A7_SOULCAL:
	case BOTH_A1_SPECIAL:
	case BOTH_A2_SPECIAL:
	case BOTH_A3_SPECIAL:
	case BOTH_A4_SPECIAL:
	case BOTH_A5_SPECIAL:
	case BOTH_GRIEVOUS_SPIN:
	case BOTH_GRIEVOUS_PROTECT:
	case BOTH_YODA_SPECIAL:
	case BOTH_FLIP_ATTACK7:
	case BOTH_PULL_IMPALE_STAB:
	case BOTH_PULL_IMPALE_SWING:
	case BOTH_ALORA_SPIN_SLASH_MD2:
	case BOTH_ALORA_SPIN_SLASH:
	case BOTH_A6_FB:
	case BOTH_A6_LR:
	case BOTH_A7_HILT:
	case BOTH_LK_S_DL_S_SB_1_W:
	case BOTH_LK_S_DL_T_SB_1_W:
	case BOTH_LK_S_ST_S_SB_1_W:
	case BOTH_LK_S_ST_T_SB_1_W:
	case BOTH_LK_S_S_S_SB_1_W:
	case BOTH_LK_S_S_T_SB_1_W:
	case BOTH_LK_DL_DL_S_SB_1_W:
	case BOTH_LK_DL_DL_T_SB_1_W:
	case BOTH_LK_DL_ST_S_SB_1_W:
	case BOTH_LK_DL_ST_T_SB_1_W:
	case BOTH_LK_DL_S_S_SB_1_W:
	case BOTH_LK_DL_S_T_SB_1_W:
	case BOTH_LK_ST_DL_S_SB_1_W:
	case BOTH_LK_ST_DL_T_SB_1_W:
	case BOTH_LK_ST_ST_S_SB_1_W:
	case BOTH_LK_ST_ST_T_SB_1_W:
	case BOTH_LK_ST_S_S_SB_1_W:
	case BOTH_LK_ST_S_T_SB_1_W:
	case BOTH_HANG_ATTACK:
		return qtrue;
	}
	if (PM_SaberDrawPutawayAnim(anim))
	{
		if (saberMove == LS_DRAW || saberMove == LS_DRAW2 || saberMove == LS_DRAW3 || saberMove == LS_PUTAWAY)
		{
			return qtrue;
		}
		return qfalse;
	}
	else if (PM_SaberStanceAnim(anim))
	{
		if (saberMove == LS_READY)
		{
			return qtrue;
		}
		return qfalse;
	}
	int animLevel = PM_AnimLevelForSaberAnim(anim);
	if (animLevel <= 0)
	{//NOTE: this will always return false for the ready poses and putaway/draw...
		return qfalse;
	}
	//drop the anim to the first level and start the checks there
	anim -= (animLevel - FORCE_LEVEL_1)*SABER_ANIM_GROUP_SIZE;
	//check level 1
	if (anim == saberMoveData[saberMove].animToUse)
	{
		return qtrue;
	}
	//check level 2
	anim += SABER_ANIM_GROUP_SIZE;
	if (anim == saberMoveData[saberMove].animToUse)
	{
		return qtrue;
	}
	//check level 3
	anim += SABER_ANIM_GROUP_SIZE;
	if (anim == saberMoveData[saberMove].animToUse)
	{
		return qtrue;
	}
	//check level 4
	anim += SABER_ANIM_GROUP_SIZE;
	if (anim == saberMoveData[saberMove].animToUse)
	{
		return qtrue;
	}
	//check level 5
	anim += SABER_ANIM_GROUP_SIZE;
	if (anim == saberMoveData[saberMove].animToUse)
	{
		return qtrue;
	}
	if (anim >= BOTH_P1_S1_T_ && anim <= BOTH_H1_S1_BR)
	{//parries, knockaways and broken parries
		return (qboolean)(anim == saberMoveData[saberMove].animToUse);
	}
	return qfalse;
}

qboolean PM_SaberInDamageMove(int move)
{
	if (move >= LS_A_TL2BR && move <= LS_A_T2B)
	{
		return qtrue;
	}
	switch (move)
	{
	case LS_A_BACK:
	case LS_A_BACK_CR:
	case LS_A_BACKSTAB:
	case LS_A_BACKSTAB_B:
	case LS_ROLL_STAB:
	case LS_A_LUNGE:
	case LS_A_JUMP_T__B_:
	case LS_A_JUMP_PALP_:
	case LS_A_FLIP_STAB:
	case LS_A_FLIP_SLASH:
	case LS_JUMPATTACK_DUAL:
	case LS_GRIEVOUS_LUNGE:
	case LS_JUMPATTACK_ARIAL_LEFT:
	case LS_JUMPATTACK_ARIAL_RIGHT:
	case LS_JUMPATTACK_CART_LEFT:
	case LS_JUMPATTACK_CART_RIGHT:
	case LS_JUMPATTACK_STAFF_LEFT:
	case LS_JUMPATTACK_STAFF_RIGHT:
	case LS_BUTTERFLY_LEFT:
	case LS_BUTTERFLY_RIGHT:
	case LS_A_BACKFLIP_ATK:
	case LS_SPINATTACK_DUAL:
	case LS_SPINATTACK_GRIEV:
	case LS_SPINATTACK:
	case LS_LEAP_ATTACK:
	case LS_SWOOP_ATTACK_RIGHT:
	case LS_SWOOP_ATTACK_LEFT:
	case LS_TAUNTAUN_ATTACK_RIGHT:
	case LS_TAUNTAUN_ATTACK_LEFT:
	case LS_STABDOWN:
	case LS_STABDOWN_BACKHAND:
	case LS_STABDOWN_STAFF:
	case LS_STABDOWN_DUAL:
	case LS_DUAL_SPIN_PROTECT:
	case LS_DUAL_SPIN_PROTECT_GRIE:
	case LS_STAFF_SOULCAL:
	case LS_YODA_SPECIAL:
	case LS_A1_SPECIAL:
	case LS_A2_SPECIAL:
	case LS_A3_SPECIAL:
	case LS_A4_SPECIAL:
	case LS_A5_SPECIAL:
	case LS_GRIEVOUS_SPECIAL:
	case LS_UPSIDE_DOWN_ATTACK:
	case LS_PULL_ATTACK_STAB:
	case LS_PULL_ATTACK_SWING:
	case LS_SPINATTACK_ALORA:
	case LS_DUAL_FB:
	case LS_DUAL_LR:
	case LS_HILT_BASH:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean BG_InKataAnim(int anim)
{
	switch (anim)
	{
	case BOTH_A6_SABERPROTECT:
	case BOTH_A7_SOULCAL:
	case BOTH_A1_SPECIAL:
	case BOTH_A2_SPECIAL:
	case BOTH_A3_SPECIAL:
	case BOTH_A4_SPECIAL:
	case BOTH_A5_SPECIAL:
	case BOTH_YODA_SPECIAL:
	case BOTH_GRIEVOUS_SPIN:
	case BOTH_GRIEVOUS_PROTECT:
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInSpecial(int move)
{
	switch (move)
	{
	case LS_A_BACK:
	case LS_A_BACK_CR:
	case LS_A_BACKSTAB:
	case LS_A_BACKSTAB_B:
	case LS_ROLL_STAB:
	case LS_A_LUNGE:
	case LS_A_JUMP_T__B_:
	case LS_A_JUMP_PALP_:
	case LS_A_FLIP_STAB:
	case LS_A_FLIP_SLASH:
	case LS_JUMPATTACK_DUAL:
	case LS_GRIEVOUS_LUNGE:
	case LS_JUMPATTACK_ARIAL_LEFT:
	case LS_JUMPATTACK_ARIAL_RIGHT:
	case LS_JUMPATTACK_CART_LEFT:
	case LS_JUMPATTACK_CART_RIGHT:
	case LS_JUMPATTACK_STAFF_LEFT:
	case LS_JUMPATTACK_STAFF_RIGHT:
	case LS_BUTTERFLY_LEFT:
	case LS_BUTTERFLY_RIGHT:
	case LS_A_BACKFLIP_ATK:
	case LS_SPINATTACK_DUAL:
	case LS_SPINATTACK_GRIEV:
	case LS_SPINATTACK:
	case LS_LEAP_ATTACK:
	case LS_SWOOP_ATTACK_RIGHT:
	case LS_SWOOP_ATTACK_LEFT:
	case LS_TAUNTAUN_ATTACK_RIGHT:
	case LS_TAUNTAUN_ATTACK_LEFT:
	case LS_KICK_F:
	case LS_KICK_F2:
	case LS_KICK_B:
	case LS_KICK_B2:
	case LS_KICK_B3:
	case LS_KICK_R:
	case LS_SLAP_R:
	case LS_KICK_L:
	case LS_SLAP_L:
	case LS_KICK_S:
	case LS_KICK_BF:
	case LS_KICK_RL:
	case LS_KICK_F_AIR:
	case LS_KICK_F_AIR2:
	case LS_KICK_B_AIR:
	case LS_KICK_R_AIR:
	case LS_KICK_L_AIR:
	case LS_STABDOWN:
	case LS_STABDOWN_BACKHAND:
	case LS_STABDOWN_STAFF:
	case LS_STABDOWN_DUAL:
	case LS_DUAL_SPIN_PROTECT:
	case LS_DUAL_SPIN_PROTECT_GRIE:
	case LS_STAFF_SOULCAL:
	case LS_YODA_SPECIAL:
	case LS_A1_SPECIAL:
	case LS_A2_SPECIAL:
	case LS_A3_SPECIAL:
	case LS_A4_SPECIAL:
	case LS_A5_SPECIAL:
	case LS_GRIEVOUS_SPECIAL:
	case LS_UPSIDE_DOWN_ATTACK:
	case LS_PULL_ATTACK_STAB:
	case LS_PULL_ATTACK_SWING:
	case LS_SPINATTACK_ALORA:
	case LS_DUAL_FB:
	case LS_DUAL_LR:
	case LS_HILT_BASH:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_KickMove(int move)
{
	switch (move)
	{
	case LS_KICK_F:
	case LS_KICK_F2:
	case LS_KICK_B:
	case LS_KICK_B2:
	case LS_KICK_B3:
	case LS_KICK_R:
	case LS_SLAP_R:
	case LS_KICK_L:
	case LS_SLAP_L:
	case LS_KICK_S:
	case LS_KICK_BF:
	case LS_KICK_RL:
	case LS_KICK_F_AIR:
	case LS_KICK_F_AIR2:
	case LS_KICK_B_AIR:
	case LS_KICK_R_AIR:
	case LS_KICK_L_AIR:
	case LS_HILT_BASH:
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInIdle(int move)
{
	switch (move)
	{
	case LS_NONE:
	case LS_READY:
	case LS_DRAW:
	case LS_DRAW2:
	case LS_DRAW3:
	case LS_PUTAWAY:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean BG_InExtraDefenseSaberMove(int move)
{
	switch (move)
	{
	case LS_SPINATTACK_DUAL:
	case LS_SPINATTACK_GRIEV:
	case LS_SPINATTACK:
	case LS_DUAL_SPIN_PROTECT:
	case LS_DUAL_SPIN_PROTECT_GRIE:
	case LS_STAFF_SOULCAL:
	case LS_YODA_SPECIAL:
	case LS_A1_SPECIAL:
	case LS_A2_SPECIAL:
	case LS_A3_SPECIAL:
	case LS_A4_SPECIAL:
	case LS_A5_SPECIAL:
	case LS_GRIEVOUS_SPECIAL:
	case LS_JUMPATTACK_DUAL:
	case LS_GRIEVOUS_LUNGE:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_FlippingAnim(int anim)
{
	switch (anim)
	{
	case BOTH_FLIP_F:			//# Flip forward
	case BOTH_FLIP_B:			//# Flip backwards
	case BOTH_FLIP_L:			//# Flip left
	case BOTH_FLIP_R:			//# Flip right
	case BOTH_ALORA_FLIP_1_MD2:
	case BOTH_ALORA_FLIP_2_MD2:
	case BOTH_ALORA_FLIP_3_MD2:
	case BOTH_ALORA_FLIP_1:
	case BOTH_ALORA_FLIP_2:
	case BOTH_ALORA_FLIP_3:
	case BOTH_WALL_RUN_RIGHT_FLIP:
	case BOTH_WALL_RUN_LEFT_FLIP:
	case BOTH_WALL_FLIP_RIGHT:
	case BOTH_WALL_FLIP_LEFT:
	case BOTH_FLIP_BACK1:
	case BOTH_FLIP_BACK2:
	case BOTH_FLIP_BACK3:
	case BOTH_WALL_FLIP_BACK1:
	case BOTH_ALORA_FLIP_B_MD2:
	case BOTH_ALORA_FLIP_B:
	case BOTH_WALL_RUN_RIGHT:
	case BOTH_WALL_RUN_LEFT:
	case BOTH_WALL_RUN_RIGHT_STOP:
	case BOTH_WALL_RUN_LEFT_STOP:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
	case BOTH_ARIAL_LEFT:
	case BOTH_ARIAL_RIGHT:
	case BOTH_ARIAL_F1:
	case BOTH_CARTWHEEL_LEFT:
	case BOTH_CARTWHEEL_RIGHT:
	case BOTH_JUMPFLIPSLASHDOWN1:
	case BOTH_JUMPFLIPSTABDOWN:
	case BOTH_JUMPATTACK6:
	case BOTH_GRIEVOUS_LUNGE:
	case BOTH_JUMPATTACK7:
	case BOTH_FORCEWALLRUNFLIP_END:
	case BOTH_FORCEWALLRUNFLIP_ALT:
	case BOTH_FLIP_ATTACK7:
	case BOTH_A7_SOULCAL:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_SpinningSaberAnim(int anim)
{
	switch (anim)
	{
		//level 1 - FIXME: level 1 will have *no* spins
	case BOTH_T1_BR_BL:
	case BOTH_T1__R__L:
	case BOTH_T1__R_BL:
	case BOTH_T1_TR_BL:
	case BOTH_T1_BR_TL:
	case BOTH_T1_BR__L:
	case BOTH_T1_TL_BR:
	case BOTH_T1__L_BR:
	case BOTH_T1__L__R:
	case BOTH_T1_BL_BR:
	case BOTH_T1_BL__R:
	case BOTH_T1_BL_TR:
		//level 2
	case BOTH_T2_BR__L:
	case BOTH_T2_BR_BL:
	case BOTH_T2__R_BL:
	case BOTH_T2__L_BR:
	case BOTH_T2_BL_BR:
	case BOTH_T2_BL__R:
		//level 3
	case BOTH_T3_BR__L:
	case BOTH_T3_BR_BL:
	case BOTH_T3__R_BL:
	case BOTH_T3__L_BR:
	case BOTH_T3_BL_BR:
	case BOTH_T3_BL__R:
		//level 4
	case BOTH_T4_BR__L:
	case BOTH_T4_BR_BL:
	case BOTH_T4__R_BL:
	case BOTH_T4__L_BR:
	case BOTH_T4_BL_BR:
	case BOTH_T4_BL__R:
		//level 5
	case BOTH_T5_BR_BL:
	case BOTH_T5__R__L:
	case BOTH_T5__R_BL:
	case BOTH_T5_TR_BL:
	case BOTH_T5_BR_TL:
	case BOTH_T5_BR__L:
	case BOTH_T5_TL_BR:
	case BOTH_T5__L_BR:
	case BOTH_T5__L__R:
	case BOTH_T5_BL_BR:
	case BOTH_T5_BL__R:
	case BOTH_T5_BL_TR:
		//level 6
	case BOTH_T6_BR_TL:
	case BOTH_T6__R_TL:
	case BOTH_T6__R__L:
	case BOTH_T6__R_BL:
	case BOTH_T6_TR_TL:
	case BOTH_T6_TR__L:
	case BOTH_T6_TR_BL:
	case BOTH_T6_T__TL:
	case BOTH_T6_T__BL:
	case BOTH_T6_TL_BR:
	case BOTH_T6__L_BR:
	case BOTH_T6__L__R:
	case BOTH_T6_TL__R:
	case BOTH_T6_TL_TR:
	case BOTH_T6__L_TR:
	case BOTH_T6__L_T_:
	case BOTH_T6_BL_T_:
	case BOTH_T6_BR__L:
	case BOTH_T6_BR_BL:
	case BOTH_T6_BL_BR:
	case BOTH_T6_BL__R:
	case BOTH_T6_BL_TR:
		//level 7
	case BOTH_T7_BR_TL:
	case BOTH_T7_BR__L:
	case BOTH_T7_BR_BL:
	case BOTH_T7__R__L:
	case BOTH_T7__R_BL:
	case BOTH_T7_TR__L:
	case BOTH_T7_T___R:
	case BOTH_T7_TL_BR:
	case BOTH_T7__L_BR:
	case BOTH_T7__L__R:
	case BOTH_T7_BL_BR:
	case BOTH_T7_BL__R:
	case BOTH_T7_BL_TR:
	case BOTH_T7_TL_TR:
	case BOTH_T7_T__BR:
	case BOTH_T7__L_TR:
	case BOTH_V7_BL_S7:
		//special
	case BOTH_ATTACK_BACK:
	case BOTH_CROUCHATTACKBACK1:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_JUMPFLIPSLASHDOWN1:
	case BOTH_JUMPFLIPSTABDOWN:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_SaberInSpecialAttack(int anim)
{
	switch (anim)
	{
	case BOTH_A2_STABBACK1:
	case BOTH_A2_STABBACK1B:
	case BOTH_ATTACK_BACK:
	case BOTH_CROUCHATTACKBACK1:
	case BOTH_ROLL_STAB:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_LUNGE2_B__T_:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_FORCELEAP_PALP:
	case BOTH_JUMPFLIPSLASHDOWN1://#
	case BOTH_JUMPFLIPSTABDOWN://#
	case BOTH_JUMPATTACK6:
	case BOTH_GRIEVOUS_LUNGE:
	case BOTH_JUMPATTACK7:
	case BOTH_SPINATTACK6:
	case BOTH_SPINATTACK7:
	case BOTH_SPINATTACKGRIEVOUS:
	case BOTH_FORCELONGLEAP_ATTACK:
	case BOTH_FORCELONGLEAP_ATTACK2:
	case BOTH_VS_ATR_S:
	case BOTH_VS_ATL_S:
	case BOTH_VT_ATR_S:
	case BOTH_VT_ATL_S:
	case BOTH_A7_KICK_F:
	case BOTH_A7_KICK_F2:
	case BOTH_A7_KICK_B:
	case BOTH_A7_KICK_B2:
	case BOTH_A7_KICK_B3:
	case BOTH_A7_KICK_R:
	case BOTH_A7_KICK_L:
	case BOTH_A7_SLAP_R:
	case BOTH_A7_SLAP_L:
	case BOTH_A7_KICK_S:
	case BOTH_A7_KICK_BF:
	case BOTH_A7_KICK_RL:
	case BOTH_A7_KICK_F_AIR:
	case BOTH_A7_KICK_F_AIR2:
	case BOTH_A7_KICK_B_AIR:
	case BOTH_A7_KICK_R_AIR:
	case BOTH_A7_KICK_L_AIR:
	case BOTH_STABDOWN:
	case BOTH_STABDOWN_BACKHAND:
	case BOTH_STABDOWN_STAFF:
	case BOTH_STABDOWN_DUAL:
	case BOTH_A6_SABERPROTECT:
	case BOTH_A7_SOULCAL:
	case BOTH_A1_SPECIAL:
	case BOTH_A2_SPECIAL:
	case BOTH_A3_SPECIAL:
	case BOTH_A4_SPECIAL:
	case BOTH_A5_SPECIAL:
	case BOTH_YODA_SPECIAL:
	case BOTH_GRIEVOUS_SPIN:
	case BOTH_GRIEVOUS_PROTECT:
	case BOTH_FLIP_ATTACK7:
	case BOTH_PULL_IMPALE_STAB:
	case BOTH_PULL_IMPALE_SWING:
	case BOTH_ALORA_SPIN_SLASH_MD2:
	case BOTH_ALORA_SPIN_SLASH:
	case BOTH_A6_FB:
	case BOTH_A6_LR:
	case BOTH_A7_HILT:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_PunchAnim(int anim)
{
	switch (anim)
	{
	case BOTH_MELEE1:
	case BOTH_MELEE2:
	case BOTH_MELEE3:
	case BOTH_MELEE4:
	case BOTH_MELEE5:
	case BOTH_MELEE6:
	case BOTH_MELEE_L:
	case BOTH_MELEE_R:
	case BOTH_MELEEUP:
	case BOTH_WOOKIE_SLAP:
		return qtrue;
		break;
	};
	return qfalse;
}

qboolean BG_KickingAnim(int anim)
{
	switch (anim)
	{
	case BOTH_A7_KICK_F:
	case BOTH_A7_KICK_F2:
	case BOTH_A7_KICK_B:
	case BOTH_A7_KICK_B2:
	case BOTH_A7_KICK_B3:
	case BOTH_A7_KICK_R:
	case BOTH_A7_SLAP_R:
	case BOTH_A7_SLAP_L:
	case BOTH_A7_KICK_L:
	case BOTH_A7_KICK_S:
	case BOTH_A7_KICK_BF:
	case BOTH_A7_KICK_RL:
	case BOTH_A7_KICK_F_AIR:
	case BOTH_A7_KICK_F_AIR2:
	case BOTH_A7_KICK_B_AIR:
	case BOTH_A7_KICK_R_AIR:
	case BOTH_A7_KICK_L_AIR:
	case BOTH_A7_HILT:
		//NOT kicks, but do kick traces anyway
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case MELEE_STANCE_HOLD_LT:
	case MELEE_STANCE_HOLD_RT:
	case MELEE_STANCE_HOLD_BL:
	case MELEE_STANCE_HOLD_BR:
	case MELEE_STANCE_HOLD_B:
	case MELEE_STANCE_HOLD_T:
	case BOTH_TUSKENLUNGE1:
		return qtrue;
		break;
	}
	return qfalse;
}

int BG_InGrappleMove(int anim)
{
	switch (anim)
	{
	case BOTH_KYLE_GRAB:
	case BOTH_KYLE_MISS:
		return 1; //grabbing at someone
	case BOTH_KYLE_PA_1:
	case BOTH_KYLE_PA_2:
	case BOTH_KYLE_PA_3:
		return 2; //beating the shit out of someone
	case BOTH_PLAYER_PA_1:
	case BOTH_PLAYER_PA_2:
	case BOTH_PLAYER_PA_3:
	case BOTH_PLAYER_PA_FLY:
		return 3; //getting the shit beaten out of you
		break;
	}

	return 0;
}

qboolean PM_SaberCanInterruptMove(int move, int anim)
{
	if (PM_InAnimForSaberMove(anim, move))
	{
		switch (move)
		{
		case LS_A_BACK:
		case LS_A_BACK_CR:
		case LS_A_BACKSTAB:
		case LS_A_BACKSTAB_B:
		case LS_ROLL_STAB:
		case LS_A_LUNGE:
		case LS_A_JUMP_T__B_:
		case LS_A_JUMP_PALP_:
		case LS_A_FLIP_STAB:
		case LS_A_FLIP_SLASH:
		case LS_JUMPATTACK_DUAL:
		case LS_GRIEVOUS_LUNGE:
		case LS_JUMPATTACK_CART_LEFT:
		case LS_JUMPATTACK_CART_RIGHT:
		case LS_JUMPATTACK_STAFF_LEFT:
		case LS_JUMPATTACK_STAFF_RIGHT:
		case LS_BUTTERFLY_LEFT:
		case LS_BUTTERFLY_RIGHT:
		case LS_A_BACKFLIP_ATK:
		case LS_SPINATTACK_DUAL:
		case LS_SPINATTACK_GRIEV:
		case LS_SPINATTACK:
		case LS_LEAP_ATTACK:
		case LS_SWOOP_ATTACK_RIGHT:
		case LS_SWOOP_ATTACK_LEFT:
		case LS_TAUNTAUN_ATTACK_RIGHT:
		case LS_TAUNTAUN_ATTACK_LEFT:
		case LS_KICK_S:
		case LS_KICK_BF:
		case LS_KICK_RL:
		case LS_STABDOWN:
		case LS_STABDOWN_BACKHAND:
		case LS_STABDOWN_STAFF:
		case LS_STABDOWN_DUAL:
		case LS_DUAL_SPIN_PROTECT:
		case LS_DUAL_SPIN_PROTECT_GRIE:
		case LS_STAFF_SOULCAL:
		case LS_YODA_SPECIAL:
		case LS_A1_SPECIAL:
		case LS_A2_SPECIAL:
		case LS_A3_SPECIAL:
		case LS_A4_SPECIAL:
		case LS_A5_SPECIAL:
		case LS_GRIEVOUS_SPECIAL:
		case LS_UPSIDE_DOWN_ATTACK:
		case LS_PULL_ATTACK_STAB:
		case LS_PULL_ATTACK_SWING:
		case LS_SPINATTACK_ALORA:
		case LS_DUAL_FB:
		case LS_DUAL_LR:
		case LS_HILT_BASH:
			return qfalse;
		}

		if (PM_SaberInAttackPure(move))
		{
			return qfalse;
		}
		if (PM_SaberInStart(move))
		{
			return qfalse;
		}
		if (PM_SaberInTransition(move))
		{
			return qfalse;
		}
		if (PM_SaberInBounce(move))
		{
			return qfalse;
		}
		if (PM_SaberInBrokenParry(move))
		{
			return qfalse;
		}
		if (PM_SaberInDeflect(move))
		{
			return qfalse;
		}
		if (PM_SaberInParry(move))
		{
			return qfalse;
		}
		if (PM_SaberInKnockaway(move))
		{
			return qfalse;
		}
		if (PM_SaberInReflect(move))
		{
			return qfalse;
		}
	}
	switch (anim)
	{
	case BOTH_A2_STABBACK1:
	case BOTH_A2_STABBACK1B:
	case BOTH_ATTACK_BACK:
	case BOTH_CROUCHATTACKBACK1:
	case BOTH_ROLL_STAB:
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
	case BOTH_LUNGE2_B__T_:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_FORCELEAP_PALP:
	case BOTH_JUMPFLIPSLASHDOWN1://#
	case BOTH_JUMPFLIPSTABDOWN://#
	case BOTH_JUMPATTACK6:
	case BOTH_GRIEVOUS_LUNGE:
	case BOTH_JUMPATTACK7:
	case BOTH_SPINATTACK6:
	case BOTH_SPINATTACK7:
	case BOTH_SPINATTACKGRIEVOUS:
	case BOTH_FORCELONGLEAP_ATTACK:
	case BOTH_VS_ATR_S:
	case BOTH_VS_ATL_S:
	case BOTH_VT_ATR_S:
	case BOTH_VT_ATL_S:
	case BOTH_A7_KICK_S:
	case BOTH_A7_KICK_BF:
	case BOTH_A7_KICK_RL:
	case BOTH_STABDOWN:
	case BOTH_STABDOWN_BACKHAND:
	case BOTH_STABDOWN_STAFF:
	case BOTH_STABDOWN_DUAL:
	case BOTH_A6_SABERPROTECT:
	case BOTH_A7_SOULCAL:
	case BOTH_A1_SPECIAL:
	case BOTH_A2_SPECIAL:
	case BOTH_A3_SPECIAL:
	case BOTH_A4_SPECIAL:
	case BOTH_A5_SPECIAL:
	case BOTH_GRIEVOUS_SPIN:
	case BOTH_GRIEVOUS_PROTECT:
	case BOTH_YODA_SPECIAL:
	case BOTH_FLIP_ATTACK7:
	case BOTH_PULL_IMPALE_STAB:
	case BOTH_PULL_IMPALE_SWING:
	case BOTH_ALORA_SPIN_SLASH_MD2:
	case BOTH_ALORA_SPIN_SLASH:
	case BOTH_A6_FB:
	case BOTH_A6_LR:
	case BOTH_A7_HILT:
	case BOTH_LK_S_DL_S_SB_1_W:
	case BOTH_LK_S_DL_T_SB_1_W:
	case BOTH_LK_S_ST_S_SB_1_W:
	case BOTH_LK_S_ST_T_SB_1_W:
	case BOTH_LK_S_S_S_SB_1_W:
	case BOTH_LK_S_S_T_SB_1_W:
	case BOTH_LK_DL_DL_S_SB_1_W:
	case BOTH_LK_DL_DL_T_SB_1_W:
	case BOTH_LK_DL_ST_S_SB_1_W:
	case BOTH_LK_DL_ST_T_SB_1_W:
	case BOTH_LK_DL_S_S_SB_1_W:
	case BOTH_LK_DL_S_T_SB_1_W:
	case BOTH_LK_ST_DL_S_SB_1_W:
	case BOTH_LK_ST_DL_T_SB_1_W:
	case BOTH_LK_ST_ST_S_SB_1_W:
	case BOTH_LK_ST_ST_T_SB_1_W:
	case BOTH_LK_ST_S_S_SB_1_W:
	case BOTH_LK_ST_S_T_SB_1_W:
	case BOTH_HANG_ATTACK:
		return qfalse;
	}
	return qtrue;
}

saberMoveName_t PM_BrokenParryForAttack(int move)
{
	switch (saberMoveData[move].startQuad)
	{
	case Q_B:
		return LS_V1_B_;
		break;
	case Q_BR:
		return LS_V1_BR;
		break;
	case Q_R:
		return LS_V1__R;
		break;
	case Q_TR:
		return LS_V1_TR;
		break;
	case Q_T:
		return LS_V1_T_;
		break;
	case Q_TL:
		return LS_V1_TL;
		break;
	case Q_L:
		return LS_V1__L;
		break;
	case Q_BL:
		return LS_V1_BL;
		break;
	}
	return LS_NONE;
}

saberMoveName_t PM_AbsorbtheParry(int move)
{
	switch (move)
	{
	case BLOCKED_FRONT:
	case BLOCKED_BLOCKATTACK_FRONT:
	case BLOCKED_TOP://LS_PARRY_UP:
		return LS_K1_T_;//push up
		break;
	case BLOCKED_BLOCKATTACK_RIGHT:
	case BLOCKED_UPPER_RIGHT://LS_PARRY_UR:
	default:
		return LS_REFLECT_ATTACK_RIGHT;//push up, slightly to right
		break;
	case BLOCKED_BLOCKATTACK_LEFT:
	case BLOCKED_UPPER_LEFT://LS_PARRY_UL:
		return LS_REFLECT_ATTACK_LEFT;//push up and to left
		break;
	case BLOCKED_LOWER_RIGHT://LS_PARRY_LR:
		return LS_K1_BR;//push down and to left
		break;
	case BLOCKED_LOWER_LEFT://LS_PARRY_LL:
		return LS_K1_BL;//push down and to right
		break;
	}
}

saberMoveName_t PM_KnockawayForParry(int move)
{
	switch (move)
	{
	case BLOCKED_FRONT:
	case BLOCKED_BLOCKATTACK_FRONT:
	case BLOCKED_TOP://LS_PARRY_UP:
		return LS_K1_T_;//push up
		break;
	case BLOCKED_BLOCKATTACK_RIGHT:
	case BLOCKED_UPPER_RIGHT://LS_PARRY_UR:
	default:
		return LS_K1_TR;//push up, slightly to right
		break;
	case BLOCKED_BLOCKATTACK_LEFT:
	case BLOCKED_UPPER_LEFT://LS_PARRY_UL:
		return LS_K1_TL;//push up and to left
		break;
	case BLOCKED_LOWER_RIGHT://LS_PARRY_LR:
		return LS_K1_BR;//push down and to left
		break;
	case BLOCKED_LOWER_LEFT://LS_PARRY_LL:
		return LS_K1_BL;//push down and to right
		break;
	}
}

qboolean PM_InRollIgnoreTimer(playerState_t *ps)
{
	switch (ps->legsAnim)
	{
	case BOTH_ROLL_F:
	case BOTH_ROLL_F1:
	case BOTH_ROLL_F2:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
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

qboolean PM_InRoll(playerState_t *ps)
{
	if (ps->legsTimer && PM_InRollIgnoreTimer(ps))
	{
		return qtrue;
	}

	return qfalse;
}

qboolean BG_InRoll(playerState_t *ps, int anim)
{
	switch ((anim))
	{
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
	case BOTH_ROLL_F:
	case BOTH_ROLL_F1:
	case BOTH_ROLL_F2:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		if (ps->legsTimer > 0)
		{
			return qtrue;
		}
		break;
	}
	return qfalse;
}

qboolean BG_InSpecialDeathAnim(int anim)
{
	switch (anim)
	{
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
		return qtrue;
		break;
	default:
		return qfalse;
		break;
	}
}

qboolean BG_InDeathAnim(int anim)
{//Purposely does not cover stumbledeath and falldeath...
	switch (anim)
	{
	case BOTH_DEATH1:		//# First Death anim
	case BOTH_DEATH2:			//# Second Death anim
	case BOTH_DEATH3:			//# Third Death anim
	case BOTH_DEATH4:			//# Fourth Death anim
	case BOTH_DEATH5:			//# Fifth Death anim
	case BOTH_DEATH6:			//# Sixth Death anim
	case BOTH_DEATH7:			//# Seventh Death anim
	case BOTH_DEATH8:			//#
	case BOTH_DEATH9:			//#
	case BOTH_DEATH10:			//#
	case BOTH_DEATH11:			//#
	case BOTH_DEATH12:			//#
	case BOTH_DEATH13:			//#
	case BOTH_DEATH14:			//#
	case BOTH_DEATH14_UNGRIP:	//# Desann's end death (cin #35)
	case BOTH_DEATH14_SITUP:		//# Tavion sitting up after having been thrown (cin #23)
	case BOTH_DEATH15:			//#
	case BOTH_DEATH16:			//#
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
	case BOTH_DEATHFORWARD3:		//# Tavion's falling in cin# 23
	case BOTH_DEATHBACKWARD1:	//# First Death in which they get thrown backward
	case BOTH_DEATHBACKWARD2:	//# Second Death in which they get thrown backward

	case BOTH_DEATH1IDLE:		//# Idle while close to death
	case BOTH_LYINGDEATH1:		//# Death to play when killed lying down
	case BOTH_STUMBLEDEATH1:		//# Stumble forward and fall face first death
	case BOTH_FALLDEATH1:		//# Fall forward off a high cliff and splat death - start
	case BOTH_FALLDEATH1INAIR:	//# Fall forward off a high cliff and splat death - loop
	case BOTH_FALLDEATH1LAND:	//# Fall forward off a high cliff and splat death - hit bottom
	//# #sep case BOTH_ DEAD POSES # Should be last frame of corresponding previous anims
	case BOTH_DEAD1:				//# First Death finished pose
	case BOTH_DEAD2:				//# Second Death finished pose
	case BOTH_DEAD3:				//# Third Death finished pose
	case BOTH_DEAD4:				//# Fourth Death finished pose
	case BOTH_DEAD5:				//# Fifth Death finished pose
	case BOTH_DEAD6:				//# Sixth Death finished pose
	case BOTH_DEAD7:				//# Seventh Death finished pose
	case BOTH_DEAD8:				//#
	case BOTH_DEAD9:				//#
	case BOTH_DEAD10:			//#
	case BOTH_DEAD11:			//#
	case BOTH_DEAD12:			//#
	case BOTH_DEAD13:			//#
	case BOTH_DEAD14:			//#
	case BOTH_DEAD15:			//#
	case BOTH_DEAD16:			//#
	case BOTH_DEAD17:			//#
	case BOTH_DEAD18:			//#
	case BOTH_DEAD19:			//#
	case BOTH_DEAD20:			//#
	case BOTH_DEAD21:			//#
	case BOTH_DEAD22:			//#
	case BOTH_DEAD23:			//#
	case BOTH_DEAD24:			//#
	case BOTH_DEAD25:			//#
	case BOTH_DEADFORWARD1:		//# First thrown forward death finished pose
	case BOTH_DEADFORWARD2:		//# Second thrown forward death finished pose
	case BOTH_DEADBACKWARD1:		//# First thrown backward death finished pose
	case BOTH_DEADBACKWARD2:		//# Second thrown backward death finished pose
	case BOTH_LYINGDEAD1:		//# Killed lying down death finished pose
	case BOTH_STUMBLEDEAD1:		//# Stumble forward death finished pose
	case BOTH_FALLDEAD1LAND:		//# Fall forward and splat death finished pose
	//# #sep case BOTH_ DEAD TWITCH/FLOP # React to being shot from death poses
	case BOTH_DEADFLOP1:		//# React to being shot from First Death finished pose
	case BOTH_DEADFLOP2:		//# React to being shot from Second Death finished pose
	case BOTH_DISMEMBER_HEAD1:	//#
	case BOTH_DISMEMBER_TORSO1:	//#
	case BOTH_DISMEMBER_LLEG:	//#
	case BOTH_DISMEMBER_RLEG:	//#
	case BOTH_DISMEMBER_RARM:	//#
	case BOTH_DISMEMBER_LARM:	//#
		return qtrue;
		break;
	default:
		return BG_InSpecialDeathAnim(anim);
		break;
	}
}

qboolean BG_InKnockDownOnly(int anim)
{
	switch (anim)
	{
	case BOTH_KNOCKDOWN1:
	case BOTH_KNOCKDOWN2:
	case BOTH_KNOCKDOWN3:
	case BOTH_KNOCKDOWN4:
	case BOTH_KNOCKDOWN5:
	case BOTH_SLAPDOWNRIGHT:
	case BOTH_SLAPDOWNLEFT:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_InSaberLockOld(int anim)
{
	switch (anim)
	{
	case BOTH_BF2LOCK:
	case BOTH_BF1LOCK:
	case BOTH_CWCIRCLELOCK:
	case BOTH_CCWCIRCLELOCK:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_InSaberLock(int anim)
{
	switch (anim)
	{
	case BOTH_LK_S_DL_S_L_1:		//lock if I'm using single vs. a dual
	case BOTH_LK_S_DL_T_L_1:		//lock if I'm using single vs. a dual
	case BOTH_LK_S_ST_S_L_1:		//lock if I'm using single vs. a staff
	case BOTH_LK_S_ST_T_L_1:		//lock if I'm using single vs. a staff
	case BOTH_LK_S_S_S_L_1:		//lock if I'm using single vs. a single and I initiated
	case BOTH_LK_S_S_T_L_1:		//lock if I'm using single vs. a single and I initiated
	case BOTH_LK_DL_DL_S_L_1:	//lock if I'm using dual vs. dual and I initiated
	case BOTH_LK_DL_DL_T_L_1:	//lock if I'm using dual vs. dual and I initiated
	case BOTH_LK_DL_ST_S_L_1:	//lock if I'm using dual vs. a staff
	case BOTH_LK_DL_ST_T_L_1:	//lock if I'm using dual vs. a staff
	case BOTH_LK_DL_S_S_L_1:		//lock if I'm using dual vs. a single
	case BOTH_LK_DL_S_T_L_1:		//lock if I'm using dual vs. a single
	case BOTH_LK_ST_DL_S_L_1:	//lock if I'm using staff vs. dual
	case BOTH_LK_ST_DL_T_L_1:	//lock if I'm using staff vs. dual
	case BOTH_LK_ST_ST_S_L_1:	//lock if I'm using staff vs. a staff and I initiated
	case BOTH_LK_ST_ST_T_L_1:	//lock if I'm using staff vs. a staff and I initiated
	case BOTH_LK_ST_S_S_L_1:		//lock if I'm using staff vs. a single
	case BOTH_LK_ST_S_T_L_1:		//lock if I'm using staff vs. a single
	case BOTH_LK_S_S_S_L_2:
	case BOTH_LK_S_S_T_L_2:
	case BOTH_LK_DL_DL_S_L_2:
	case BOTH_LK_DL_DL_T_L_2:
	case BOTH_LK_ST_ST_S_L_2:
	case BOTH_LK_ST_ST_T_L_2:
		return qtrue;
		break;
	default:
		return BG_InSaberLockOld(anim);
		break;
	}
	//return qfalse;
}

//Called only where pm is valid (not all require pm, but some do):
qboolean PM_InCartwheel(int anim)
{
	switch (anim)
	{
	case BOTH_ARIAL_LEFT:
	case BOTH_ARIAL_RIGHT:
	case BOTH_ARIAL_F1:
	case BOTH_CARTWHEEL_LEFT:
	case BOTH_CARTWHEEL_RIGHT:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_InButterfly(int anim)
{
	switch (anim)
	{
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_StandingAnim(int anim)
{//NOTE: does not check idles or special (cinematic) stands
	switch (anim)
	{
	case BOTH_STANDMELEE:
	case BOTH_SABERSTANCE_STANCE:
	case BOTH_SABERDUAL_STANCE:
	case BOTH_SABERDUAL_STANCE_GRIEVOUS://dual saber style
	case BOTH_SABERDUAL_STANCE_IDLE:
	case BOTH_SABERDUAL_STANCE_ALT:
	case BOTH_SABERSTAFF_STANCE:
	case BOTH_SABERSTAFF_STANCE_IDLE:
	case BOTH_SABERSTAFF_STANCE_ALT:
	case BOTH_STAND1:
	case BOTH_STAND2:
	case BOTH_STAND3:
	case BOTH_STAND4:
	case BOTH_ATTACK3:
	case BOTH_ATTACK5:
	case BOTH_ATTACK6:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_InAirKickingAnim(int anim)
{//in a air kick animation
	switch (anim)
	{
	case BOTH_A7_KICK_F_AIR:
	case BOTH_A7_KICK_F_AIR2:
	case BOTH_A7_KICK_B_AIR:
	case BOTH_A7_KICK_R_AIR:
	case BOTH_A7_KICK_L_AIR:
		return qtrue;
	}
	return qfalse;
}

qboolean BG_InKnockDownOnGround(playerState_t *ps)
{
	switch (ps->legsAnim)
	{
	case BOTH_KNOCKDOWN1:
	case BOTH_KNOCKDOWN2:
	case BOTH_KNOCKDOWN3:
	case BOTH_KNOCKDOWN4:
	case BOTH_KNOCKDOWN5:
	case BOTH_SLAPDOWNRIGHT:
	case BOTH_SLAPDOWNLEFT:
	case BOTH_RELEASED:
	{//at end of fall down anim
		return qtrue;
	}
	break;
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_GETUP_CROUCH_F1:
	case BOTH_GETUP_CROUCH_B1:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_FORCE_GETUP_B6:
		if (BG_AnimLength(0, (animNumber_t)ps->legsAnim) - ps->legsTimer < 500)
		{//at beginning of getup anim
			return qtrue;
		}
		break;
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		if (BG_AnimLength(0, (animNumber_t)ps->legsAnim) - ps->legsTimer < 500)
		{//at beginning of getup anim
			return qtrue;
		}
		break;
	case BOTH_LK_DL_ST_T_SB_1_L:
		if (ps->legsTimer < 1000)
		{
			return qtrue;
		}
		break;
	case BOTH_PLAYER_PA_3_FLY:
		if (ps->legsTimer < 300)
		{
			return qtrue;
		}
		break;
	}
	return qfalse;
}

qboolean BG_StabDownAnim(int anim)
{
	switch (anim)
	{
	case BOTH_STABDOWN:
	case BOTH_STABDOWN_BACKHAND:
	case BOTH_STABDOWN_STAFF:
	case BOTH_STABDOWN_DUAL:
		return qtrue;
	}
	return qfalse;
}

saberMoveName_t PM_SaberBounceForAttack(int move)
{
	switch (saberMoveData[move].startQuad)
	{
	case Q_B:
	case Q_BR:
		return LS_B1_BR;
		break;
	case Q_R:
		return LS_B1__R;
		break;
	case Q_TR:
		return LS_B1_TR;
		break;
	case Q_T:
		return LS_B1_T_;
		break;
	case Q_TL:
		return LS_B1_TL;
		break;
	case Q_L:
		return LS_B1__L;
		break;
	case Q_BL:
		return LS_B1_BL;
		break;
	}
	return LS_NONE;
}

int InvertQuad(int quad)
{//Returns the reflection quad for the given quad.
 //This is used for setting a player's block direction based on his attacker's move's start/end quad.
	switch (quad)
	{
	case Q_B:
		return Q_B;
		break;
	case Q_BR:
		return Q_BL;
		break;
	case Q_R:
		return Q_L;
		break;
	case Q_TR:
		return Q_TL;
		break;
	case Q_T:
		return Q_T;
		break;
	case Q_TL:
		return Q_TR;
		break;
	case Q_L:
		return Q_R;
		break;
	case Q_BL:
		return Q_BR;
		break;
	default:
		return quad;
		break;
	};
}

int PM_SaberDeflectionForQuad(int quad)
{
	switch (quad)
	{
	case Q_B:
		return LS_D1_B_;
		break;
	case Q_BR:
		return LS_D1_BR;
		break;
	case Q_R:
		return LS_D1__R;
		break;
	case Q_TR:
		return LS_D1_TR;
		break;
	case Q_T:
		return LS_D1_T_;
		break;
	case Q_TL:
		return LS_D1_TL;
		break;
	case Q_L:
		return LS_D1__L;
		break;
	case Q_BL:
		return LS_D1_BL;
		break;
	}
	return LS_NONE;
}

qboolean PM_SaberInDeflect(int move)
{
	if (move >= LS_D1_BR && move <= LS_D1_B_)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInParry(int move)
{
	if (move >= LS_PARRY_UP && move <= LS_REFLECT_ATTACK_FRONT)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInKnockaway(int move)
{
	switch (move)
	{
	case LS_REFLECT_ATTACK_LEFT:
	case LS_REFLECT_ATTACK_RIGHT:
	case LS_REFLECT_ATTACK_FRONT:
	case LS_K1_T_:
	case LS_K1_TR:
	case LS_K1_TL:
	case LS_K1_BR:
	case LS_K1_BL:
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInReflect(int move)
{
	if (move >= LS_REFLECT_UP && move <= LS_REFLECT_LL)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInStart(int move)
{
	if (move >= LS_S_TL2BR && move <= LS_S_T2B)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInReturn(int move)
{
	if (move >= LS_R_TL2BR && move <= LS_R_T2B)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean BG_SaberInReturn(int move)
{
	return PM_SaberInReturn(move);
}

qboolean PM_SaberInbackblock(int move)
{
	switch (move)
	{
	case BOTH_P7_S1_B1_:
	case BOTH_P6_S1_B1_:
	case BOTH_P1_S1_B1_:
		return qtrue;
	}
	return qfalse;
}

qboolean PM_InSaberAnim(int anim)
{
	if (anim >= BOTH_A1_T__B_ && anim <= BOTH_H1_S1_BR)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInMassiveBounce(int anim)
{
	switch (anim)
	{
	case BOTH_BASHED1:
	case BOTH_H1_S1_T_:
	case BOTH_H1_S1_TR:
	case BOTH_H1_S1_TL:
	case BOTH_H1_S1_BL:
	case BOTH_H1_S1_B_:
	case BOTH_H1_S1_BR:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean BG_InWalk(int anim)
{
	switch (anim)
	{
	case BOTH_WALK1:
	case BOTH_WALK1TALKCOMM1:
	case BOTH_WALK2:
	case BOTH_WALK5:
	case BOTH_WALK6:
	case BOTH_WALK7:
	case BOTH_WALK_DUAL:
	case BOTH_WALK_STAFF:
	case BOTH_WALKBACK1:
	case BOTH_WALKBACK2:
	case BOTH_WALKBACK_DUAL:
	case BOTH_WALKBACK_STAFF:
		return qtrue;
		break;

	default:
		return qfalse;
		break;

	};
}

qboolean PM_InForceGetUp(playerState_t *ps)
{//racc - Are we in a getup animation that uses the Force?
	switch (ps->legsAnim)
	{
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_FORCE_GETUP_B6:
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		if (ps->legsTimer)
		{
			return qtrue;
		}
		break;
	}
	return qfalse;
}

qboolean PM_InGetUp(playerState_t *ps)
{//racc - player in getup animation.
	switch (ps->legsAnim)
	{
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_GETUP_CROUCH_F1:
	case BOTH_GETUP_CROUCH_B1:
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		if (ps->legsTimer)
		{
			return qtrue;
		}
		break;
	default:
		return PM_InForceGetUp(ps);
		break;
	}
	//what the hell, redundant, but...
	return qfalse;
}

qboolean PM_InKnockDown(playerState_t *ps)
{
	switch (ps->legsAnim)
	{
	case BOTH_KNOCKDOWN1:
	case BOTH_KNOCKDOWN2:
	case BOTH_KNOCKDOWN3:
	case BOTH_KNOCKDOWN4:
	case BOTH_KNOCKDOWN5:
	case BOTH_SLAPDOWNRIGHT:
	case BOTH_SLAPDOWNLEFT:
	case BOTH_RELEASED:
		return qtrue;
		break;
	case BOTH_LK_DL_ST_T_SB_1_L:
		if (ps->legsTimer < 550)
		{
			return qtrue;
		}
		break;
	case BOTH_PLAYER_PA_3_FLY:
		if (ps->legsTimer < 300)
		{
			return qtrue;
		}
		break;
	default:
		return PM_InGetUp(ps);
		break;
	}
	return qfalse;
}

int PM_PowerLevelForSaberAnims(playerState_t *ps)
{
	int anim = ps->torsoAnim;
	int	animTimer = ps->torsoAnim;
	int	animTimeElapsed = BG_AnimLength(0, (animNumber_t)anim) - animTimer;

	if (anim >= BOTH_A2_T__B_ && anim <= BOTH_D2_B____)
	{
		return FORCE_LEVEL_2;
	}
	if (anim >= BOTH_A3_T__B_ && anim <= BOTH_D3_B____)
	{
		return FORCE_LEVEL_3;
	}
	if (anim >= BOTH_A4_T__B_ && anim <= BOTH_D4_B____)
	{//desann
		return FORCE_LEVEL_4;
	}
	if (anim >= BOTH_A5_T__B_ && anim <= BOTH_D5_B____)
	{//tavion
		return FORCE_LEVEL_2;
	}
	if (anim >= BOTH_A6_T__B_ && anim <= BOTH_D6_B____)
	{//dual
		return FORCE_LEVEL_2;
	}
	if (anim >= BOTH_A7_T__B_ && anim <= BOTH_D7_B____)
	{//staff
		return FORCE_LEVEL_2;
	}
	if ((anim >= BOTH_P1_S1_T_ && anim <= BOTH_P1_S1_BR)
		|| (anim >= BOTH_P6_S6_T_ && anim <= BOTH_P6_S6_BR)
		|| (anim >= BOTH_P7_S7_T_ && anim <= BOTH_P7_S7_BR))
	{//parries
		switch (ps->fd.saberAnimLevel)
		{
		case SS_STRONG:
		case SS_DESANN:
			return FORCE_LEVEL_3;
			break;
		case SS_TAVION:
		case SS_STAFF:
		case SS_DUAL:
		case SS_MEDIUM:
			return FORCE_LEVEL_2;
			break;
		case SS_FAST:
			return FORCE_LEVEL_1;
			break;
		default:
			return FORCE_LEVEL_0;
			break;
		}
	}
	if ((anim >= BOTH_K1_S1_T_ && anim <= BOTH_K1_S1_BR)
		|| (anim >= BOTH_K6_S6_T_ && anim <= BOTH_K6_S6_BR)
		|| (anim >= BOTH_K7_S7_T_ && anim <= BOTH_K7_S7_BR))
	{//knockaways
		return FORCE_LEVEL_3;
	}
	if ((anim >= BOTH_V1_BR_S1 && anim <= BOTH_V1_B__S1)
		|| (anim >= BOTH_V6_BR_S6 && anim <= BOTH_V6_B__S6)
		|| (anim >= BOTH_V7_BR_S7 && anim <= BOTH_V7_B__S7))
	{//knocked-away attacks
		return FORCE_LEVEL_1;
	}
	if ((anim >= BOTH_H1_S1_T_ && anim <= BOTH_H1_S1_BR)
		|| (anim >= BOTH_H6_S6_T_ && anim <= BOTH_H6_S6_BR)
		|| (anim >= BOTH_H7_S7_T_ && anim <= BOTH_H7_S7_BR))
	{//broken parries
		return FORCE_LEVEL_0;
	}
	switch (anim)
	{
	case BOTH_A2_STABBACK1B:
	case BOTH_A2_STABBACK1:
		if (ps->torsoTimer < 450)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 400)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_ATTACK_BACK:
		if (ps->torsoTimer < 500)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_CROUCHATTACKBACK1:
		if (ps->torsoTimer < 800)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
		//FIXME: break up?
		return FORCE_LEVEL_3;
		break;
	case BOTH_FJSS_TR_BL:
	case BOTH_FJSS_TL_BR:
		//FIXME: break up?
		return FORCE_LEVEL_3;
		break;
	case BOTH_K1_S1_T_:	//# knockaway saber top
	case BOTH_K1_S1_TR:	//# knockaway saber top right
	case BOTH_K1_S1_TL:	//# knockaway saber top left
	case BOTH_K1_S1_BL:	//# knockaway saber bottom left
	case BOTH_K1_S1_B_:	//# knockaway saber bottom
	case BOTH_K1_S1_BR:	//# knockaway saber bottom right
						//FIXME: break up?
		return FORCE_LEVEL_3;
		break;
	case BOTH_LUNGE2_B__T_:
		if (ps->torsoTimer < 400)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 150)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_FORCELEAP_PALP:
	case BOTH_FORCELEAP2_T__B_:
		if (ps->torsoTimer < 400)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 550)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_VS_ATR_S:
	case BOTH_VS_ATL_S:
	case BOTH_VT_ATR_S:
	case BOTH_VT_ATL_S:
		return FORCE_LEVEL_3;//???
		break;
	case BOTH_JUMPFLIPSLASHDOWN1:
		if (ps->torsoTimer <= 900)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 550)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_JUMPFLIPSTABDOWN:
		if (ps->torsoTimer <= 1200)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed <= 250)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_JUMPATTACK6:
	case BOTH_GRIEVOUS_LUNGE:
		if ((ps->torsoTimer >= 1450
			&& animTimeElapsed >= 400)
			|| (ps->torsoTimer >= 400
				&& animTimeElapsed >= 1100))
		{//pretty much sideways
			return FORCE_LEVEL_3;
		}
		return FORCE_LEVEL_0;
		break;
	case BOTH_JUMPATTACK7:
		if (ps->torsoTimer <= 1200)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 200)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_SPINATTACK6:
	case BOTH_SPINATTACKGRIEVOUS:
		if (animTimeElapsed <= 200)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_SPINATTACK7:
		if (ps->torsoTimer <= 500)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 500)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_FORCELONGLEAP_ATTACK:
		if (animTimeElapsed <= 200)
		{//1st four frames of anim
			return FORCE_LEVEL_3;
		}
		break;
	case BOTH_STABDOWN:
		if (ps->torsoTimer <= 900)
		{//end of anim
			return FORCE_LEVEL_3;
		}
		break;
	case BOTH_STABDOWN_BACKHAND:
	case BOTH_STABDOWN_STAFF:
		if (ps->torsoTimer <= 850)
		{//end of anim
			return FORCE_LEVEL_3;
		}
		break;
	case BOTH_STABDOWN_DUAL:
		if (ps->torsoTimer <= 900)
		{//end of anim
			return FORCE_LEVEL_3;
		}
		break;
	case BOTH_A6_SABERPROTECT:
	case BOTH_GRIEVOUS_PROTECT:
		if (ps->torsoTimer < 650)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 250)
		{//start of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_A7_SOULCAL:
		if (ps->torsoTimer < 650)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 600)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_YODA_SPECIAL:
	case BOTH_A1_SPECIAL:
		if (ps->torsoTimer < 600)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 200)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_A2_SPECIAL:
	case BOTH_GRIEVOUS_SPIN:
		if (ps->torsoTimer < 300)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 200)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_A3_SPECIAL:
		if (ps->torsoTimer < 700)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 200)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_A4_SPECIAL:
		if (ps->torsoTimer < 700)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 200)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_A5_SPECIAL:
		if (ps->torsoTimer < 700)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 200)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_FLIP_ATTACK7:
		return FORCE_LEVEL_3;
		break;
	case BOTH_PULL_IMPALE_STAB:
		if (ps->torsoTimer < 1000)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_PULL_IMPALE_SWING:
		if (ps->torsoTimer < 500)//750 )
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 650)//600 )
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_ALORA_SPIN_SLASH_MD2:
		if (ps->torsoTimer < 900)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 250)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_ALORA_SPIN_SLASH:
		if (ps->torsoTimer < 900)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 250)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_A6_FB:
		if (ps->torsoTimer < 250)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 250)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_A6_LR:
		if (ps->torsoTimer < 250)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 250)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_3;
		break;
	case BOTH_A7_HILT:
		return FORCE_LEVEL_0;
		break;
		//===SABERLOCK SUPERBREAKS START===========================================================================
	case BOTH_LK_S_DL_T_SB_1_W:
		if (ps->torsoTimer < 700)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_S_ST_S_SB_1_W:
		if (ps->torsoTimer < 300)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_S_DL_S_SB_1_W:
	case BOTH_LK_S_S_S_SB_1_W:
		if (ps->torsoTimer < 700)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 400)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_S_ST_T_SB_1_W:
	case BOTH_LK_S_S_T_SB_1_W:
		if (ps->torsoTimer < 150)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 400)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_DL_DL_T_SB_1_W:
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_DL_DL_S_SB_1_W:
	case BOTH_LK_DL_ST_S_SB_1_W:
		if (animTimeElapsed < 1000)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_DL_ST_T_SB_1_W:
		if (ps->torsoTimer < 950)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 650)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_DL_S_S_SB_1_W:
		if (ps->torsoTimer < 900)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 450)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_DL_S_T_SB_1_W:
		if (ps->torsoTimer < 250)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 150)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_ST_DL_S_SB_1_W:
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_ST_DL_T_SB_1_W:
		//special suberbreak - doesn't kill, just kicks them backwards
		return FORCE_LEVEL_0;
		break;
	case BOTH_LK_ST_ST_S_SB_1_W:
	case BOTH_LK_ST_S_S_SB_1_W:
		if (ps->torsoTimer < 800)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 350)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		return FORCE_LEVEL_5;
		break;
	case BOTH_LK_ST_ST_T_SB_1_W:
	case BOTH_LK_ST_S_T_SB_1_W:
		return FORCE_LEVEL_5;
		break;
		//===SABERLOCK SUPERBREAKS START===========================================================================
	case BOTH_HANG_ATTACK:
		//FIME: break up
		if (ps->torsoTimer < 1000)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else if (animTimeElapsed < 250)
		{//beginning of anim
			return FORCE_LEVEL_0;
		}
		else
		{//sweet spot
			return FORCE_LEVEL_5;
		}
		break;
	case BOTH_ROLL_STAB:
		if (animTimeElapsed > 400)
		{//end of anim
			return FORCE_LEVEL_0;
		}
		else
		{
			return FORCE_LEVEL_3;
		}
		break;
	}
	return FORCE_LEVEL_0;
}

qboolean PM_PainAnim(int anim)
{
	switch ((anim))
	{
	case BOTH_PAIN1:				//# First take pain anim
	case BOTH_PAIN2:				//# Second take pain anim
	case BOTH_PAIN3:				//# Third take pain anim
	case BOTH_PAIN4:				//# Fourth take pain anim
	case BOTH_PAIN5:				//# Fifth take pain anim - from behind
	case BOTH_PAIN6:				//# Sixth take pain anim - from behind
	case BOTH_PAIN7:				//# Seventh take pain anim - from behind
	case BOTH_PAIN8:				//# Eigth take pain anim - from behind
	case BOTH_PAIN9:				//#
	case BOTH_PAIN10:			//#
	case BOTH_PAIN11:			//#
	case BOTH_PAIN12:			//#
	case BOTH_PAIN13:			//#
	case BOTH_PAIN14:			//#
	case BOTH_PAIN15:			//#
	case BOTH_PAIN16:			//#
	case BOTH_PAIN17:			//#
	case BOTH_PAIN18:			//#
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_DodgeHoldAnim(int anim)
{
	switch (anim)
	{
	case BOTH_DODGE_HOLD_FL:
	case BOTH_DODGE_HOLD_FR:
	case BOTH_DODGE_HOLD_BL:
	case BOTH_DODGE_HOLD_BR:
	case BOTH_DODGE_HOLD_L:
	case BOTH_DODGE_HOLD_R:
	case BOTH_CROUCHDODGE:
	case BOTH_DODGE_B:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_DodgeAnim(int anim)
{
	switch (anim)
	{
	case BOTH_DODGE_FL:			//# lean-dodge forward left
	case BOTH_DODGE_FR:			//# lean-dodge forward right
	case BOTH_DODGE_BL:			//# lean-dodge backwards left
	case BOTH_DODGE_BR:			//# lean-dodge backwards right
	case BOTH_DODGE_L:			//# lean-dodge left
	case BOTH_DODGE_R:			//# lean-dodge right
	case BOTH_CROUCHDODGE:
	case BOTH_DODGE_B:
	case BOTH_HOP_L:
	case BOTH_HOP_R:
	case BOTH_DASH_L:
	case BOTH_DASH_R:
		return qtrue;
		break;
	default:
		return PM_DodgeHoldAnim(anim);
		break;
	}
}

qboolean PM_BlockHoldAnim(int anim)
{
	switch (anim)
	{
	case BOTH_BLOCK_HOLD_FL:
	case BOTH_BLOCK_HOLD_FR:
	case BOTH_BLOCK_HOLD_BL:
	case BOTH_BLOCK_HOLD_BR:
	case BOTH_BLOCK_HOLD_L:
	case BOTH_BLOCK_HOLD_R:
	case BOTH_BLOCK_BACK_HOLD_L:
	case BOTH_BLOCK_BACK_HOLD_R:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_BlockAnim(int anim)
{
	switch (anim)
	{
	case BOTH_BLOCK_FL:
	case BOTH_BLOCK_FR:
	case BOTH_BLOCK_BL:
	case BOTH_BLOCK_BR:
	case BOTH_BLOCK_L:
	case BOTH_BLOCK_R:
	case BOTH_BLOCK_BACK_L:
	case BOTH_BLOCK_BACK_R:
		return qtrue;
		break;
	default:
		return PM_BlockHoldAnim(anim);
		break;
	}
}

//melee kungfu==========================
qboolean PM_MeleeblockHoldAnim(int meleeblockAmin)
{
	switch (meleeblockAmin)
	{
	case MELEE_STANCE_HOLD_LT:
	case MELEE_STANCE_HOLD_RT:
	case MELEE_STANCE_HOLD_BL:
	case MELEE_STANCE_HOLD_BR:
	case MELEE_STANCE_HOLD_B:
	case MELEE_STANCE_HOLD_T:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_MeleeblockAnim(int meleeblockAmin)
{
	switch (meleeblockAmin)
	{
	case MELEE_STANCE_LT:
	case MELEE_STANCE_RT:
	case MELEE_STANCE_BL:
	case MELEE_STANCE_BR:
	case MELEE_STANCE_B:
	case MELEE_STANCE_T:
		return qtrue;
		break;
	default:
		return PM_MeleeblockHoldAnim(meleeblockAmin);
		break;
	}
}

qboolean PM_ForceJumpingAnim(int anim)
{
	switch (anim)
	{
	case BOTH_FORCEJUMP1:				//# Jump - wind-up and leave ground
	case BOTH_FORCEJUMP2:				//# Jump - wind-up and leave ground
	case BOTH_FORCEINAIR1:			//# In air loop (from jump)
	case BOTH_FORCELAND1:				//# Landing (from in air loop)
	case BOTH_FORCEJUMPBACK1:			//# Jump backwards - wind-up and leave ground
	case BOTH_FORCEINAIRBACK1:		//# In air loop (from jump back)
	case BOTH_FORCELANDBACK1:			//# Landing backwards(from in air loop)
	case BOTH_FORCEJUMPLEFT1:			//# Jump left - wind-up and leave ground
	case BOTH_FORCEINAIRLEFT1:		//# In air loop (from jump left)
	case BOTH_FORCELANDLEFT1:			//# Landing left(from in air loop)
	case BOTH_FORCEJUMPRIGHT1:		//# Jump right - wind-up and leave ground
	case BOTH_FORCEINAIRRIGHT1:		//# In air loop (from jump right)
	case BOTH_FORCELANDRIGHT1:		//# Landing right(from in air loop)
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_JumpingAnim(int anim)
{
	switch (anim)
	{
	case BOTH_JUMP1:				//# Jump - wind-up and leave ground
	case BOTH_JUMP2:				//# Jump - wind-up and leave ground
	case BOTH_INAIR1:			//# In air loop (from jump)
	case BOTH_LAND1:				//# Landing (from in air loop)
	case BOTH_LAND2:				//# Landing Hard (from a great height)
	case BOTH_JUMPBACK1:			//# Jump backwards - wind-up and leave ground
	case BOTH_INAIRBACK1:		//# In air loop (from jump back)
	case BOTH_LANDBACK1:			//# Landing backwards(from in air loop)
	case BOTH_JUMPLEFT1:			//# Jump left - wind-up and leave ground
	case BOTH_INAIRLEFT1:		//# In air loop (from jump left)
	case BOTH_LANDLEFT1:			//# Landing left(from in air loop)
	case BOTH_JUMPRIGHT1:		//# Jump right - wind-up and leave ground
	case BOTH_INAIRRIGHT1:		//# In air loop (from jump right)
	case BOTH_LANDRIGHT1:		//# Landing right(from in air loop)
		return qtrue;
		break;
	default:
		if (PM_InAirKickingAnim(anim))
		{
			return qtrue;
		}
		return PM_ForceJumpingAnim(anim);
		break;
	}
}

qboolean PM_LandingAnim(int anim)
{
	switch ((anim))
	{
	case BOTH_LAND1:				//# Landing (from in air loop)
	case BOTH_LAND2:				//# Landing Hard (from a great height)
	case BOTH_LANDBACK1:			//# Landing backwards(from in air loop)
	case BOTH_LANDLEFT1:			//# Landing left(from in air loop)
	case BOTH_LANDRIGHT1:		//# Landing right(from in air loop)
	case BOTH_FORCELAND1:		//# Landing (from in air loop)
	case BOTH_FORCELANDBACK1:	//# Landing backwards(from in air loop)
	case BOTH_FORCELANDLEFT1:	//# Landing left(from in air loop)
	case BOTH_FORCELANDRIGHT1:	//# Landing right(from in air loop)
	case BOTH_FORCELONGLEAP_LAND:		//# Landing right(from in air loop)
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_ForceAnim(int anim)
{
	switch (anim)
	{
	case BOTH_CHOKE1:			//being choked...???
	case BOTH_GESTURE1:			//taunting...
	case BOTH_RESISTPUSH:		//# plant yourself to resist force push/pulls.
	case BOTH_RESISTPUSH2:		//# plant yourself to resist force push/pulls.
	case BOTH_YODA_RESISTFORCE:		//# plant yourself to resist force push/pulls.
	case BOTH_FORCEPUSH:		//# Use off-hand to do force power.
	case BOTH_FORCEPULL:		//# Use off-hand to do force power.
	case BOTH_MINDTRICK1:		//# Use off-hand to do mind trick
	case BOTH_MINDTRICK2:		//# Use off-hand to do distraction
	case BOTH_FORCELIGHTNING:	//# Use off-hand to do lightning
	case BOTH_FORCELIGHTNING_START:
	case BOTH_FORCELIGHTNING_HOLD:	//# Use off-hand to do lightning
	case BOTH_FLAMETHROWER:	//# Use off-hand to do lightning
	case BOTH_FORCELIGHTNING_RELEASE:	//# Use off-hand to do lightning
	case BOTH_FORCEHEAL_START:	//# Healing meditation pose start
	case BOTH_FORCEHEAL_STOP:	//# Healing meditation pose end
	case BOTH_FORCEHEAL_QUICK:	//# Healing meditation gesture
	case BOTH_FORCEGRIP1:		//# temp force-grip anim (actually re-using push)
	case BOTH_FORCEGRIP_HOLD:		//# temp force-grip anim (actually re-using push)
	case BOTH_FORCEGRIP_OLD:		//# temp force-grip anim (actually re-using push)
	case BOTH_FORCEGRIP_RELEASE:		//# temp force-grip anim (actually re-using push)
	case BOTH_FORCEGRIP3:		//# force-gripping
	case BOTH_FORCE_RAGE:
	case BOTH_FORCE_2HANDEDLIGHTNING:
	case BOTH_FORCE_2HANDEDLIGHTNING_START:
	case BOTH_FORCE_2HANDEDLIGHTNING_HOLD:
	case BOTH_FORCE_2HANDEDLIGHTNING_NEW:
	case BOTH_FORCE_2HANDEDLIGHTNING_OLD:
	case BOTH_FORCE_2HANDEDLIGHTNING_RELEASE:
	case BOTH_FORCE_DRAIN:
	case BOTH_FORCE_DRAIN_START:
	case BOTH_FORCE_DRAIN_HOLD:
	case BOTH_FORCE_DRAIN_GRAB_HOLD_OLD:
	case BOTH_FORCE_DRAIN_RELEASE:
	case BOTH_FORCE_ABSORB:
	case BOTH_FORCE_ABSORB_START:
	case BOTH_FORCE_ABSORB_END:
	case BOTH_FORCE_PROTECT:
	case BOTH_FORCE_PROTECT_FAST:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_BounceAnim(int anim);
qboolean PM_SaberReturnAnim(int anim)
{
	if ((anim >= BOTH_R1_B__S1 && anim <= BOTH_R1_TR_S1)
		|| (anim >= BOTH_R2_B__S1 && anim <= BOTH_R2_TR_S1)
		|| (anim >= BOTH_R3_B__S1 && anim <= BOTH_R3_TR_S1)
		|| (anim >= BOTH_R4_B__S1 && anim <= BOTH_R4_TR_S1)
		|| (anim >= BOTH_R5_B__S1 && anim <= BOTH_R5_TR_S1)
		|| (anim >= BOTH_R6_B__S6 && anim <= BOTH_R6_TR_S6)
		|| (anim >= BOTH_R7_B__S7 && anim <= BOTH_R7_TR_S7))
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SpinningAnim(int anim)
{
	return PM_SpinningSaberAnim(anim);
}

qboolean PM_InOnGroundAnim(int anim)
{
	switch (anim)
	{
	case BOTH_DEAD1:
	case BOTH_DEAD2:
	case BOTH_DEAD3:
	case BOTH_DEAD4:
	case BOTH_DEAD5:
	case BOTH_DEADFORWARD1:
	case BOTH_DEADBACKWARD1:
	case BOTH_DEADFORWARD2:
	case BOTH_DEADBACKWARD2:
	case BOTH_LYINGDEATH1:
	case BOTH_LYINGDEAD1:
	case BOTH_SLEEP1:			//# laying on back-rknee up-rhand on torso
	case BOTH_KNOCKDOWN1:		//#
	case BOTH_KNOCKDOWN2:		//#
	case BOTH_KNOCKDOWN3:		//#
	case BOTH_KNOCKDOWN4:		//#
	case BOTH_KNOCKDOWN5:		//#
	case BOTH_SLAPDOWNRIGHT:
	case BOTH_SLAPDOWNLEFT:
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_GETUP_CROUCH_F1:
	case BOTH_GETUP_CROUCH_B1:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_FORCE_GETUP_B6:
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
		return qtrue;
		break;
	}

	return qfalse;
}

qboolean PM_InOnGroundAnims(playerState_t *ps)
{
	switch (ps->legsAnim)
	{
	case BOTH_DEAD1:
	case BOTH_DEAD2:
	case BOTH_DEAD3:
	case BOTH_DEAD4:
	case BOTH_DEAD5:
	case BOTH_DEADFORWARD1:
	case BOTH_DEADBACKWARD1:
	case BOTH_DEADFORWARD2:
	case BOTH_DEADBACKWARD2:
	case BOTH_LYINGDEATH1:
	case BOTH_LYINGDEAD1:
	case BOTH_PAIN2WRITHE1:		//# Transition from upright position to writhing on ground anim
	case BOTH_WRITHING1:			//# Lying on ground writhing in pain
	case BOTH_WRITHING1RLEG:		//# Lying on ground writhing in pain: holding right leg
	case BOTH_WRITHING1LLEG:		//# Lying on ground writhing in pain: holding left leg
	case BOTH_WRITHING2:			//# Lying on stomache writhing in pain
	case BOTH_INJURED1:			//# Lying down: against wall - can also be sleeping
	case BOTH_CRAWLBACK1:			//# Lying on back: crawling backwards with elbows
	case BOTH_INJURED2:			//# Injured pose 2
	case BOTH_INJURED3:			//# Injured pose 3
	case BOTH_INJURED6:			//# Injured pose 6
	case BOTH_INJURED6ATTACKSTART:	//# Start attack while in injured 6 pose 
	case BOTH_INJURED6ATTACKSTOP:	//# End attack while in injured 6 pose
	case BOTH_INJURED6COMBADGE:	//# Hit combadge while in injured 6 pose
	case BOTH_INJURED6POINT:		//# Chang points to door while in injured state
	case BOTH_SLEEP1:			//# laying on back-rknee up-rhand on torso
	case BOTH_SLEEP2:			//# on floor-back against wall-arms crossed
	case BOTH_SLEEP5:			//# Laying on side sleeping on flat sufrace
	case BOTH_SLEEP_IDLE1:		//# rub face and nose while asleep from sleep pose 1
	case BOTH_SLEEP_IDLE2:		//# shift position while asleep - stays in sleep2
	case BOTH_SLEEP1_NOSE:		//# Scratch nose from SLEEP1 pose
	case BOTH_SLEEP2_SHIFT:		//# Shift in sleep from SLEEP2 pose
	case BOTH_DOWNTOPRONE:		//# 
	case BOTH_PRONEIDLE:		//# 
	case BOTH_FIREPRONE:		//# 
		return qtrue;
		break;
	case BOTH_KNOCKDOWN1:		//# 
	case BOTH_KNOCKDOWN2:		//# 
	case BOTH_KNOCKDOWN3:		//# 
	case BOTH_KNOCKDOWN4:		//# 
	case BOTH_KNOCKDOWN5:		//# 
	case BOTH_SLAPDOWNRIGHT:
	case BOTH_SLAPDOWNLEFT:
	case BOTH_LK_DL_ST_T_SB_1_L:
	case BOTH_RELEASED:
		if (ps->legsTimer < 500)
		{//pretty much horizontal by this point
			return qtrue;
		}
		break;
	case BOTH_PLAYER_PA_3_FLY:
		if (ps->legsTimer < 300)
		{//pretty much horizontal by this point
			return qtrue;
		}
		break;
	case BOTH_GETUP1:
	case BOTH_GETUP2:
	case BOTH_GETUP3:
	case BOTH_GETUP4:
	case BOTH_GETUP5:
	case BOTH_GETUP_CROUCH_F1:
	case BOTH_GETUP_CROUCH_B1:
	case BOTH_FORCE_GETUP_F1:
	case BOTH_FORCE_GETUP_F2:
	case BOTH_FORCE_GETUP_B1:
	case BOTH_FORCE_GETUP_B2:
	case BOTH_FORCE_GETUP_B3:
	case BOTH_FORCE_GETUP_B4:
	case BOTH_FORCE_GETUP_B5:
	case BOTH_FORCE_GETUP_B6:
	case BOTH_STANDFROMPRONE:
		if (ps->legsTimer > PM_AnimLength(0, (animNumber_t)ps->legsAnim) - 400)
		{//still pretty much horizontal at this point
			return qtrue;
		}
		break;
	}

	return qfalse;
}

qboolean PM_InRollComplete(playerState_t *ps, int anim)
{
	switch ((anim))
	{
	case BOTH_ROLL_F:
	case BOTH_ROLL_F1:
	case BOTH_ROLL_F2:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		if (ps->legsTimer < 1)
		{
			return qtrue;
		}
		break;
	}
	return qfalse;
}

qboolean PM_CanRollFromSoulCal(playerState_t *ps)
{
	if (ps->legsAnim == BOTH_A7_SOULCAL
		&& ps->legsTimer < 700
		&& ps->legsTimer > 250)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SuperBreakLoseAnim(int anim)
{
	switch (anim)
	{
	case BOTH_LK_S_DL_S_SB_1_L:	//super break I lost
	case BOTH_LK_S_DL_T_SB_1_L:	//super break I lost
	case BOTH_LK_S_ST_S_SB_1_L:	//super break I lost
	case BOTH_LK_S_ST_T_SB_1_L:	//super break I lost
	case BOTH_LK_S_S_S_SB_1_L:	//super break I lost
	case BOTH_LK_S_S_T_SB_1_L:	//super break I lost
	case BOTH_LK_DL_DL_S_SB_1_L:	//super break I lost
	case BOTH_LK_DL_DL_T_SB_1_L:	//super break I lost
	case BOTH_LK_DL_ST_S_SB_1_L:	//super break I lost
	case BOTH_LK_DL_ST_T_SB_1_L:	//super break I lost
	case BOTH_LK_DL_S_S_SB_1_L:	//super break I lost
	case BOTH_LK_DL_S_T_SB_1_L:	//super break I lost
	case BOTH_LK_ST_DL_S_SB_1_L:	//super break I lost
	case BOTH_LK_ST_DL_T_SB_1_L:	//super break I lost
	case BOTH_LK_ST_ST_S_SB_1_L:	//super break I lost
	case BOTH_LK_ST_ST_T_SB_1_L:	//super break I lost
	case BOTH_LK_ST_S_S_SB_1_L:	//super break I lost
	case BOTH_LK_ST_S_T_SB_1_L:	//super break I lost
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean PM_SuperBreakWinAnim(int anim)
{
	switch (anim)
	{
	case BOTH_LK_S_DL_S_SB_1_W:	//super break I won
	case BOTH_LK_S_DL_T_SB_1_W:	//super break I won
	case BOTH_LK_S_ST_S_SB_1_W:	//super break I won
	case BOTH_LK_S_ST_T_SB_1_W:	//super break I won
	case BOTH_LK_S_S_S_SB_1_W:	//super break I won
	case BOTH_LK_S_S_T_SB_1_W:	//super break I won
	case BOTH_LK_DL_DL_S_SB_1_W:	//super break I won
	case BOTH_LK_DL_DL_T_SB_1_W:	//super break I won
	case BOTH_LK_DL_ST_S_SB_1_W:	//super break I won
	case BOTH_LK_DL_ST_T_SB_1_W:	//super break I won
	case BOTH_LK_DL_S_S_SB_1_W:	//super break I won
	case BOTH_LK_DL_S_T_SB_1_W:	//super break I won
	case BOTH_LK_ST_DL_S_SB_1_W:	//super break I won
	case BOTH_LK_ST_DL_T_SB_1_W:	//super break I won
	case BOTH_LK_ST_ST_S_SB_1_W:	//super break I won
	case BOTH_LK_ST_ST_T_SB_1_W:	//super break I won
	case BOTH_LK_ST_S_S_SB_1_W:	//super break I won
	case BOTH_LK_ST_S_T_SB_1_W:	//super break I won
		return qtrue;
		break;
	}
	return qfalse;
}


qboolean PM_SaberLockBreakAnim(int anim)
{
	switch (anim)
	{
	case BOTH_BF1BREAK:
	case BOTH_BF2BREAK:
	case BOTH_CWCIRCLEBREAK:
	case BOTH_CCWCIRCLEBREAK:
	case BOTH_LK_S_DL_S_B_1_L:	//normal break I lost
	case BOTH_LK_S_DL_S_B_1_W:	//normal break I won
	case BOTH_LK_S_DL_T_B_1_L:	//normal break I lost
	case BOTH_LK_S_DL_T_B_1_W:	//normal break I won
	case BOTH_LK_S_ST_S_B_1_L:	//normal break I lost
	case BOTH_LK_S_ST_S_B_1_W:	//normal break I won
	case BOTH_LK_S_ST_T_B_1_L:	//normal break I lost
	case BOTH_LK_S_ST_T_B_1_W:	//normal break I won
	case BOTH_LK_S_S_S_B_1_L:	//normal break I lost
	case BOTH_LK_S_S_S_B_1_W:	//normal break I won
	case BOTH_LK_S_S_T_B_1_L:	//normal break I lost
	case BOTH_LK_S_S_T_B_1_W:	//normal break I won
	case BOTH_LK_DL_DL_S_B_1_L:	//normal break I lost
	case BOTH_LK_DL_DL_S_B_1_W:	//normal break I won
	case BOTH_LK_DL_DL_T_B_1_L:	//normal break I lost
	case BOTH_LK_DL_DL_T_B_1_W:	//normal break I won
	case BOTH_LK_DL_ST_S_B_1_L:	//normal break I lost
	case BOTH_LK_DL_ST_S_B_1_W:	//normal break I won
	case BOTH_LK_DL_ST_T_B_1_L:	//normal break I lost
	case BOTH_LK_DL_ST_T_B_1_W:	//normal break I won
	case BOTH_LK_DL_S_S_B_1_L:	//normal break I lost
	case BOTH_LK_DL_S_S_B_1_W:	//normal break I won
	case BOTH_LK_DL_S_T_B_1_L:	//normal break I lost
	case BOTH_LK_DL_S_T_B_1_W:	//normal break I won
	case BOTH_LK_ST_DL_S_B_1_L:	//normal break I lost
	case BOTH_LK_ST_DL_S_B_1_W:	//normal break I won
	case BOTH_LK_ST_DL_T_B_1_L:	//normal break I lost
	case BOTH_LK_ST_DL_T_B_1_W:	//normal break I won
	case BOTH_LK_ST_ST_S_B_1_L:	//normal break I lost
	case BOTH_LK_ST_ST_S_B_1_W:	//normal break I won
	case BOTH_LK_ST_ST_T_B_1_L:	//normal break I lost
	case BOTH_LK_ST_ST_T_B_1_W:	//normal break I won
	case BOTH_LK_ST_S_S_B_1_L:	//normal break I lost
	case BOTH_LK_ST_S_S_B_1_W:	//normal break I won
	case BOTH_LK_ST_S_T_B_1_L:	//normal break I lost
	case BOTH_LK_ST_S_T_B_1_W:	//normal break I won
		return qtrue;
		break;
	}
	return (PM_SuperBreakLoseAnim(anim) || PM_SuperBreakWinAnim(anim));
}

qboolean BG_KnockDownAnim(int anim)
{//racc - is this a "normal" knockdown animation?
	switch (anim)
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
	}
	return qfalse;
}

qboolean PM_KnockDownAnimExtended(int anim)
{//racc - check anim to see if it's one of the getting-knockdowned/have-been-knockeddown ones.  
 //This doesn't include any of the knockdown getup animations
	switch (anim)
	{
	case BOTH_KNOCKDOWN1:
	case BOTH_KNOCKDOWN2:
	case BOTH_KNOCKDOWN3:
	case BOTH_KNOCKDOWN4:
	case BOTH_KNOCKDOWN5:
	case BOTH_SLAPDOWNRIGHT:
	case BOTH_SLAPDOWNLEFT:
		//special anims:
	case BOTH_RELEASED:
	case BOTH_LK_DL_ST_T_SB_1_L:
	case BOTH_PLAYER_PA_3_FLY:
		return qtrue;
		break;
	}
	return qfalse;
}


qboolean BG_FullBodyTauntAnim(int anim)
{
	switch (anim)
	{
	case BOTH_GESTURE1:
	case BOTH_DUAL_TAUNT:
	case BOTH_STAFF_TAUNT:
	case BOTH_BOW:
	case BOTH_MEDITATE:
	case BOTH_MEDITATE1:			// default taunt
	case BOTH_SHOWOFF_FAST:
	case BOTH_SHOWOFF_MEDIUM:
	case BOTH_SHOWOFF_STRONG:
	case BOTH_SHOWOFF_DUAL:
	case BOTH_SHOWOFF_STAFF:
	case BOTH_VICTORY_FAST:
	case BOTH_VICTORY_MEDIUM:
	case BOTH_VICTORY_STRONG:
	case BOTH_VICTORY_DUAL:
	case BOTH_VICTORY_STAFF:
	case BOTH_TUSKENTAUNT1:
	case BOTH_SHOWOFF_OBI:
		return qtrue;
		break;
	}
	return qfalse;
}


/*
=============
BG_AnimLength

Get the "length" of an anim given the local anim index (which skeleton)
and anim number. Obviously does not take things like the length of the
anim while force speeding (as an example) and whatnot into account.
=============
*/
int BG_AnimLength(int index, animNumber_t anim)
{
	if ((int)anim < 0 || anim >= MAX_ANIMATIONS)
	{
		return 0;
	}
	return bgAllAnims[index].anims[anim].numFrames * fabs((float)(bgAllAnims[index].anims[anim].frameLerp));

}

//just use whatever pm->animations is 
int PM_AnimLength(int index, animNumber_t anim)
{
	if (!pm->animations || (int)anim < 0 || anim >= MAX_ANIMATIONS)
	{
		return 0;
	}
	return pm->animations[anim].numFrames * fabs((float)(pm->animations[anim].frameLerp));
}

void PM_DebugLegsAnim(int anim)
{
	int oldAnim = (pm->ps->legsAnim);
	int newAnim = (anim);

	if (oldAnim < MAX_TOTALANIMATIONS && oldAnim >= BOTH_DEATH1 &&
		newAnim < MAX_TOTALANIMATIONS && newAnim >= BOTH_DEATH1)
	{
		Com_Printf("OLD: %s\n", animTable[oldAnim]);
		Com_Printf("NEW: %s\n", animTable[newAnim]);
	}
}

qboolean PM_SaberInTransition(int move)
{
	if (move >= LS_T1_BR__R && move <= LS_T1_BL__L)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_SaberInTransitionAny(int move)
{
	if (PM_SaberInStart(move))
	{
		return qtrue;
	}
	else if (PM_SaberInTransition(move))
	{
		return qtrue;
	}
	else if (PM_SaberInReturn(move))
	{
		return qtrue;
	}
	return qfalse;
}

/*
==============================================================================
END: Animation utility functions (sequence checking)
==============================================================================
*/

void BG_FlipPart(playerState_t *ps, int part)
{
	if (part == SETANIM_TORSO)
	{
		if (ps->torsoFlip)
		{
			ps->torsoFlip = qfalse;
		}
		else
		{
			ps->torsoFlip = qtrue;
		}
	}
	else if (part == SETANIM_LEGS)
	{
		if (ps->legsFlip)
		{
			ps->legsFlip = qfalse;
		}
		else
		{
			ps->legsFlip = qtrue;
		}
	}
}

qboolean	BGPAFtextLoaded = qfalse;
animation_t	bgHumanoidAnimations[MAX_TOTALANIMATIONS]; //humanoid animations are the only ones that are statically allocated.

bgLoadedAnim_t bgAllAnims[MAX_ANIM_FILES];
int bgNumAllAnims = 2; //start off at 2, because 0 will always be assigned to humanoid, and 1 will always be rockettrooper

//ALWAYS call on game/cgame init
void BG_InitAnimsets(void)
{
	memset(&bgAllAnims, 0, sizeof(bgAllAnims));
	BGPAFtextLoaded = qfalse;	// VVFIXME - The PC doesn't seem to need this, but why?
}

//ALWAYS call on game/cgame shutdown
void BG_ClearAnimsets(void)
{
}

animation_t *BG_AnimsetAlloc(void)
{
	assert(bgNumAllAnims < MAX_ANIM_FILES);
	bgAllAnims[bgNumAllAnims].anims = (animation_t *)BG_Alloc(sizeof(animation_t)*MAX_TOTALANIMATIONS);

	return bgAllAnims[bgNumAllAnims].anims;
}

void BG_AnimsetFree(animation_t *animset)
{
}

#ifdef _CGAME //none of this is actually needed serverside. Could just be moved to cgame code but it's here since it used to tie in a lot with the anim loading stuff.
stringID_table_t animEventTypeTable[MAX_ANIM_EVENTS + 1] =
{
	ENUM2STRING(AEV_SOUND),			//# animID AEV_SOUND framenum soundpath randomlow randomhi chancetoplay
	ENUM2STRING(AEV_FOOTSTEP),		//# animID AEV_FOOTSTEP framenum footstepType
	ENUM2STRING(AEV_EFFECT),		//# animID AEV_EFFECT framenum effectpath boltName
	ENUM2STRING(AEV_FIRE),			//# animID AEV_FIRE framenum altfire chancetofire
	ENUM2STRING(AEV_MOVE),			//# animID AEV_MOVE framenum forwardpush rightpush uppush
	ENUM2STRING(AEV_SOUNDCHAN),		//# animID AEV_SOUNDCHAN framenum CHANNEL soundpath randomlow randomhi chancetoplay
	ENUM2STRING(AEV_SABER_SWING),	//# animID AEV_SABER_SWING framenum CHANNEL randomlow randomhi chancetoplay
	ENUM2STRING(AEV_SABER_SPIN),	//# animID AEV_SABER_SPIN framenum CHANNEL chancetoplay
	ENUM2STRING(AEV_AMBIENT),
	//must be terminated
	{ NULL,-1 }
};

stringID_table_t footstepTypeTable[NUM_FOOTSTEP_TYPES + 1] =
{
	ENUM2STRING(FOOTSTEP_R),
	ENUM2STRING(FOOTSTEP_L),
	ENUM2STRING(FOOTSTEP_HEAVY_R),
	ENUM2STRING(FOOTSTEP_HEAVY_L),
	//must be terminated
	{ NULL,-1 }
};

int CheckAnimFrameForEventType(animevent_t *animEvents, int keyFrame, animEventType_t eventType)
{
	int i;

	for (i = 0; i < MAX_ANIM_EVENTS; i++)
	{
		if (eventType == AEV_AMBIENT)
		{//find the ambient animevent
			if (animEvents[i].eventType == AEV_AMBIENT)
				return i;
		}
		else if (animEvents[i].keyFrame == keyFrame)
		{//there is an animevent on this frame already
			if (animEvents[i].eventType == eventType)
			{//and it is of the same type
				return i;
			}
		}
	}
	//nope
	return -1;
}

void ParseAnimationEvtBlock(const char *aeb_filename, animevent_t *animEvents, animation_t *animations, int *i, const char **text_p)
{
	const char		*token;
	int				num, n, animNum, keyFrame, lowestVal, highestVal, curAnimEvent, lastAnimEvent = 0;
	animEventType_t	eventType;
	char			stringData[MAX_QPATH];

	// get past starting bracket
	while (1)
	{
		token = COM_Parse(text_p);
		if (!Q_stricmp(token, "{"))
		{
			break;
		}
	}

	//NOTE: instead of a blind increment, increase the index
	//			this way if we have an event on an anim that already
	//			has an event of that type, it stomps it

	// read information for each frame
	while (1)
	{
		if (lastAnimEvent >= MAX_ANIM_EVENTS)
		{
			Com_Error(ERR_DROP, "ParseAnimationEvtBlock: number events in animEvent file %s > MAX_ANIM_EVENTS(%i)", aeb_filename, MAX_ANIM_EVENTS);
			return;
		}
		// Get base frame of sequence
		token = COM_Parse(text_p);
		if (!token || !token[0])
		{
			break;
		}

		if (!Q_stricmp(token, "}"))		// At end of block
		{
			break;
		}

		//Parse ambient sound
		if (!Q_stricmp(token, "AEV_AMBIENT"))
		{//It's an ambient sound, do your thing.
		 //AEV_AMBIENT intervals randomfactor soundpath randomlow randomhi chancetoplay
		 //see if this frame already has an event of this type on it, if so, overwrite it
			curAnimEvent = CheckAnimFrameForEventType(animEvents, 0, AEV_AMBIENT);
			if (curAnimEvent == -1)
			{//this anim frame doesn't already have an event of this type on it
				curAnimEvent = lastAnimEvent;
			}
			animEvents[curAnimEvent].eventType = AEV_AMBIENT;

			//Lets grab the time intervals
			token = COM_Parse(text_p);
			if (!token)
			{
				break;
			}
			animEvents[curAnimEvent].ambtime = atoi(token);

			//Lets grab the random factor
			token = COM_Parse(text_p);
			if (!token)
			{
				break;
			}
			animEvents[curAnimEvent].ambrandom = atoi(token);

			//set sound channel
			token = COM_Parse(text_p);
			if (!token)
			{
				break;
			}
			if (stricmp(token, "CHAN_VOICE_ATTEN") == 0)
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE_ATTEN;
			}
			else if (stricmp(token, "CHAN_VOICE_GLOBAL") == 0)
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE_GLOBAL;
			}
			else if (stricmp(token, "CHAN_ANNOUNCER") == 0)
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_ANNOUNCER;
			}
			else if (stricmp(token, "CHAN_BODY") == 0)
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_BODY;
			}
			else if (stricmp(token, "CHAN_WEAPON") == 0)
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_WEAPON;
			}
			else if (stricmp(token, "CHAN_VOICE") == 0)
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE;
			}
			else
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_AUTO;
			}

			//get soundstring
			token = COM_Parse(text_p);
			if (!token)
			{
				break;
			}
			strcpy(stringData, token);
			//get lowest value
			token = COM_Parse(text_p);
			if (!token)
			{//WARNING!  BAD TABLE!
				break;
			}
			lowestVal = atoi(token);
			//get highest value
			token = COM_Parse(text_p);
			if (!token)
			{//WARNING!  BAD TABLE!
				break;
			}
			highestVal = atoi(token);
			//Now precache all the sounds
			if (lowestVal && highestVal)
			{
				if ((highestVal - lowestVal) >= MAX_RANDOM_ANIM_SOUNDS)
				{
					highestVal = lowestVal + (MAX_RANDOM_ANIM_SOUNDS - 1);
				}
				for (n = lowestVal, num = AED_SOUNDINDEX_START; n <= highestVal && num <= AED_SOUNDINDEX_END; n++, num++)
				{
					if (stringData[0] == '*' && n == lowestVal)
					{//You only need to do this once
						animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] = 0;
						animEvents[curAnimEvent].eventData[AED_CSOUND_RANDSTART] = lowestVal;
						if (!animEvents[curAnimEvent].stringData)
						{
							animEvents[curAnimEvent].stringData = (char *)BG_Alloc(MAX_QPATH);
						}
						strcpy(animEvents[curAnimEvent].stringData, stringData);
					}
					else if (stringData[0] != '*')
						//else
					{
						animEvents[curAnimEvent].eventData[num] = trap->S_RegisterSound(va(stringData, n));
					}
				}
				animEvents[curAnimEvent].eventData[AED_SOUND_NUMRANDOMSNDS] = num - 1;
			}
			else
			{
				if (stringData[0] == '*')
				{
					animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] = 0;
					animEvents[curAnimEvent].eventData[AED_CSOUND_RANDSTART] = 0;
					if (!animEvents[curAnimEvent].stringData)
					{
						animEvents[curAnimEvent].stringData = (char *)BG_Alloc(MAX_QPATH);
					}
					strcpy(animEvents[curAnimEvent].stringData, stringData);
				}
				else
				{
					animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] = trap->S_RegisterSound(stringData);
				}
			}

			//get probability
			token = COM_Parse(text_p);
			if (!token)
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_SOUND_PROBABILITY] = atoi(token);

			//bump the AnimEvent counter if this took the last number.
			if (curAnimEvent == lastAnimEvent)
			{
				lastAnimEvent++;
			}
			continue;
		}

		//Compare to same table as animations used
		//	so we don't have to use actual numbers for animation first frames,
		//	just need offsets.
		//This way when animation numbers change, this table won't have to be updated,
		//	at least not much.
		animNum = GetIDForString(animTable, token);
		if (animNum == -1)
		{//Unrecognized ANIM ENUM name, or we're skipping this line, keep going till you get a good one
			//Com_Printf(S_COLOR_YELLOW"WARNING: Unknown token %s in animEvent file %s\n", token, aeb_filename );
			while (token[0])
			{
				token = COM_ParseExt(text_p, qfalse);	//returns empty string when next token is EOL
			}
			continue;
		}

		if (animations[animNum].numFrames == 0)
		{//we don't use this anim
			Com_Printf(S_COLOR_YELLOW"WARNING: %s mpanimevents.cfg: anim %s not used by this model\n", aeb_filename, token);
			//skip this entry
			SkipRestOfLine(text_p);
			continue;
		}

		token = COM_Parse(text_p);
		eventType = (animEventType_t)GetIDForString(animEventTypeTable, token);
		if (eventType == AEV_NONE || eventType == (animEventType_t)-1)
		{//Unrecognized ANIM EVENT TYOE, or we're skipping this line, keep going till you get a good one
			//Com_Printf(S_COLOR_YELLOW"WARNING: Unknown token %s in animEvent file %s\n", token, aeb_filename );
			continue;
		}

		//set our start frame
		keyFrame = animations[animNum].firstFrame;
		// Get offset to frame within sequence
		token = COM_Parse(text_p);
		if (!token)
		{
			break;
		}
		keyFrame += atoi(token);

		//see if this frame already has an event of this type on it, if so, overwrite it
		curAnimEvent = CheckAnimFrameForEventType(animEvents, keyFrame, eventType);
		if (curAnimEvent == -1)
		{//this anim frame doesn't already have an event of this type on it
			curAnimEvent = lastAnimEvent;
		}

		//now that we know which event index we're going to plug the data into, start doing it
		animEvents[curAnimEvent].eventType = eventType;
		animEvents[curAnimEvent].keyFrame = keyFrame;

		//now read out the proper data based on the type
		switch (animEvents[curAnimEvent].eventType)
		{
		case AEV_SOUNDCHAN:		//# animID AEV_SOUNDCHAN framenum CHANNEL soundpath randomlow randomhi chancetoplay
			token = COM_Parse(text_p);
			if (!token)
				break;

			if (!Q_stricmp(token, "CHAN_VOICE_ATTEN"))
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE_ATTEN;
			else if (!Q_stricmp(token, "CHAN_VOICE_GLOBAL"))
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE_GLOBAL;
			else if (!Q_stricmp(token, "CHAN_ANNOUNCER"))
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_ANNOUNCER;
			else if (!Q_stricmp(token, "CHAN_BODY"))
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_BODY;
			else if (!Q_stricmp(token, "CHAN_WEAPON"))
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_WEAPON;
			else if (!Q_stricmp(token, "CHAN_VOICE"))
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE;
			else
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_AUTO;

			//fall through to normal sound
		case AEV_SOUND:			//# animID AEV_SOUND framenum soundpath randomlow randomhi chancetoplay
			//get soundstring
			token = COM_Parse(text_p);
			if (!token)
			{
				break;
			}
			strcpy(stringData, token);
			//get lowest value
			token = COM_Parse(text_p);
			if (!token)
			{//WARNING!  BAD TABLE!
				break;
			}
			lowestVal = atoi(token);
			//get highest value
			token = COM_Parse(text_p);
			if (!token)
			{//WARNING!  BAD TABLE!
				break;
			}
			highestVal = atoi(token);
			//Now precache all the sounds
			//NOTE: If we can be assured sequential handles, we can store sound indices
			//		instead of strings, unfortunately, if these sounds were previously
			//		registered, we cannot be guaranteed sequential indices.  Thus an array
			if (lowestVal && highestVal)
			{
				//assert(highestVal - lowestVal < MAX_RANDOM_ANIM_SOUNDS);
				if ((highestVal - lowestVal) >= MAX_RANDOM_ANIM_SOUNDS)
				{
					highestVal = lowestVal + (MAX_RANDOM_ANIM_SOUNDS - 1);
				}
				for (n = lowestVal, num = AED_SOUNDINDEX_START; n <= highestVal && num <= AED_SOUNDINDEX_END; n++, num++)
				{
					if (stringData[0] == '*')
					{ //FIXME? Would be nice to make custom sounds work with mpanimevents.
						animEvents[curAnimEvent].eventData[num] = 0;
					}
					else
					{
						animEvents[curAnimEvent].eventData[num] = trap->S_RegisterSound(va(stringData, n));
					}
				}
				animEvents[curAnimEvent].eventData[AED_SOUND_NUMRANDOMSNDS] = num - 1;
			}
			else
			{
				if (stringData[0] == '*')
				{ //FIXME? Would be nice to make custom sounds work with mpanimevents.
					animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] = 0;
				}
				else
				{
					animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] = trap->S_RegisterSound(stringData);
				}
#ifndef FINAL_BUILD
				if (!animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] &&
					stringData[0] != '*')
				{//couldn't register it - file not found
					Com_Printf(S_COLOR_RED "ParseAnimationSndBlock: sound %s does not exist (mpanimevents.cfg %s)!\n", stringData, aeb_filename);
				}
#endif
				animEvents[curAnimEvent].eventData[AED_SOUND_NUMRANDOMSNDS] = 0;
			}
			//get probability
			token = COM_Parse(text_p);
			if (!token)
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_SOUND_PROBABILITY] = atoi(token);

			//last part - cheat and check and see if it's a special overridable saber sound we know of...
			if (!Q_stricmpn("sound/weapons/saber/saberhup", stringData, 28))
			{//a saber swing
				animEvents[curAnimEvent].eventType = AEV_SABER_SWING;
				animEvents[curAnimEvent].eventData[AED_SABER_SWING_SABERNUM] = 0;//since we don't know which one they meant if we're hacking this, always use first saber
				animEvents[curAnimEvent].eventData[AED_SABER_SWING_PROBABILITY] = animEvents[curAnimEvent].eventData[AED_SOUND_PROBABILITY];
				if (lowestVal < 4)
				{//fast swing
					animEvents[curAnimEvent].eventData[AED_SABER_SWING_TYPE] = 0;//SWING_FAST;
				}
				else if (lowestVal < 7)
				{//medium swing
					animEvents[curAnimEvent].eventData[AED_SABER_SWING_TYPE] = 1;//SWING_MEDIUM;
				}
				else
				{//strong swing
					animEvents[curAnimEvent].eventData[AED_SABER_SWING_TYPE] = 2;//SWING_STRONG;
				}
			}
			else if (!Q_stricmpn("sound/weapons/saber/saberspin", stringData, 29))
			{//a saber spin
				animEvents[curAnimEvent].eventType = AEV_SABER_SPIN;
				animEvents[curAnimEvent].eventData[AED_SABER_SPIN_SABERNUM] = 0;//since we don't know which one they meant if we're hacking this, always use first saber
				animEvents[curAnimEvent].eventData[AED_SABER_SPIN_PROBABILITY] = animEvents[curAnimEvent].eventData[AED_SOUND_PROBABILITY];
				if (stringData[29] == 'o')
				{//saberspinoff
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 0;
				}
				else if (stringData[29] == '1')
				{//saberspin1
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 2;
				}
				else if (stringData[29] == '2')
				{//saberspin2
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 3;
				}
				else if (stringData[29] == '3')
				{//saberspin3
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 4;
				}
				else if (stringData[29] == '%')
				{//saberspin%d
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 5;
				}
				else
				{//just plain saberspin
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 1;
				}
			}
			break;
		case AEV_FOOTSTEP:		//# animID AEV_FOOTSTEP framenum footstepType
			//get footstep type
			token = COM_Parse(text_p);
			if (!token)
			{
				break;
			}
			animEvents[curAnimEvent].eventData[AED_FOOTSTEP_TYPE] = GetIDForString(footstepTypeTable, token);
			//get probability
			token = COM_Parse(text_p);
			if (!token)
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_FOOTSTEP_PROBABILITY] = atoi(token);
			break;
		case AEV_EFFECT:		//# animID AEV_EFFECT framenum effectpath boltName
			//get effect index
			token = COM_Parse(text_p);
			if (!token)
			{
				break;
			}
			animEvents[curAnimEvent].eventData[AED_EFFECTINDEX] = trap->FX_RegisterEffect(token);
			//get bolt index
			token = COM_Parse(text_p);
			if (!token)
			{
				break;
			}
			if (Q_stricmp("none", token) != 0 && Q_stricmp("NULL", token) != 0)
			{//actually are specifying a bolt to use
				if (!animEvents[curAnimEvent].stringData)
				{ //eh, whatever. no dynamic stuff, so this will do.
					animEvents[curAnimEvent].stringData = (char *)BG_Alloc(2048);
				}
				strcpy(animEvents[curAnimEvent].stringData, token);
			}
			//NOTE: this string will later be used to add a bolt and store the index, as below:
			//animEvent->eventData[AED_BOLTINDEX] = trap->G2API_AddBolt( &cent->gent->ghoul2[cent->gent->playerModel], animEvent->stringData );
			//get probability
			token = COM_Parse(text_p);
			if (!token)
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_EFFECT_PROBABILITY] = atoi(token);
			break;
		case AEV_FIRE:			//# animID AEV_FIRE framenum altfire chancetofire
			//get altfire
			token = COM_Parse(text_p);
			if (!token)
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_FIRE_ALT] = atoi(token);
			//get probability
			token = COM_Parse(text_p);
			if (!token)
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_FIRE_PROBABILITY] = atoi(token);
			break;
		case AEV_MOVE:			//# animID AEV_MOVE framenum forwardpush rightpush uppush
			//get forward push
			token = COM_Parse(text_p);
			if (!token)
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_MOVE_FWD] = atoi(token);
			//get right push
			token = COM_Parse(text_p);
			if (!token)
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_MOVE_RT] = atoi(token);
			//get upwards push
			token = COM_Parse(text_p);
			if (!token)
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_MOVE_UP] = atoi(token);
			break;
		default:				//unknown?
			SkipRestOfLine(text_p);
			continue;
			break;
		}

		if (curAnimEvent == lastAnimEvent)
		{
			lastAnimEvent++;
		}
	}
}

/*
======================
BG_ParseAnimationEvtFile

Read a configuration file containing animation events
models/players/kyle/mpanimevents.cfg, etc

This file's presence is not required

======================
*/
bgLoadedEvents_t bgAllEvents[MAX_ANIM_FILES];
int bgNumAnimEvents = 1;
static int bg_animParseIncluding = 0;

int BG_ParseAnimationEvtFile(const char* as_filename, int animFileIndex, int eventFileIndex)
{
	const char	*text_p;
	int			len;
	const char	*token;
	char		text[80000];
	char		sfilename[MAX_QPATH];
	fileHandle_t	f;
	int			i, j, upper_i, lower_i;
	int				usedIndex = -1;
	animevent_t	*legsAnimEvents;
	animevent_t	*torsoAnimEvents;
	animation_t		*animations;
	int				forcedIndex;

	assert(animFileIndex < MAX_ANIM_FILES);
	assert(eventFileIndex < MAX_ANIM_FILES);

	if (eventFileIndex == -1)
	{
		forcedIndex = 0;
	}
	else
	{
		forcedIndex = eventFileIndex;
	}

	if (bg_animParseIncluding <= 0)
	{ //if we should be parsing an included file, skip this part
		if (bgAllEvents[forcedIndex].eventsParsed)
		{//already cached this one
			return forcedIndex;
		}
	}

	legsAnimEvents = bgAllEvents[forcedIndex].legsAnimEvents;
	torsoAnimEvents = bgAllEvents[forcedIndex].torsoAnimEvents;
	animations = bgAllAnims[animFileIndex].anims;

	if (bg_animParseIncluding <= 0)
	{ //if we should be parsing an included file, skip this part
		//Go through and see if this filename is already in the table.
		i = 0;
		while (i < bgNumAnimEvents && forcedIndex != 0)
		{
			if (!Q_stricmp(as_filename, bgAllEvents[i].filename))
			{ //looks like we have it already.
				return i;
			}
			i++;
		}
	}

	// Load and parse mpanimevents.cfg file
	Com_sprintf(sfilename, sizeof(sfilename), "%smpanimevents.cfg", as_filename);

	if (bg_animParseIncluding <= 0)
	{ //should already be done if we're including
		//initialize anim event array
		for (i = 0; i < MAX_ANIM_EVENTS; i++)
		{
			//Type of event
			torsoAnimEvents[i].eventType = AEV_NONE;
			legsAnimEvents[i].eventType = AEV_NONE;
			//Frame to play event on
			torsoAnimEvents[i].keyFrame = -1;
			legsAnimEvents[i].keyFrame = -1;
			//we allow storage of one string, temporarily (in case we have to look up an index later, then make sure to set stringData to NULL so we only do the look-up once)
			torsoAnimEvents[i].stringData = NULL;
			legsAnimEvents[i].stringData = NULL;
			//Unique IDs, can be soundIndex of sound file to play OR effect index or footstep type, etc.
			for (j = 0; j < AED_ARRAY_SIZE; j++)
			{
				torsoAnimEvents[i].eventData[j] = -1;
				legsAnimEvents[i].eventData[j] = -1;
			}
			torsoAnimEvents[i].ambtime = 0;
			legsAnimEvents[i].ambtime = 0;


			torsoAnimEvents[i].ambrandom = 0;
			legsAnimEvents[i].ambrandom = 0;
		}
	}

	// load the file
	len = trap->FS_Open(sfilename, &f, FS_READ);
	if (len <= 0)
	{//no file
		goto fin;
	}
	if (len >= sizeof(text) - 1)
	{
		trap->FS_Close(f);
#ifndef FINAL_BUILD
		Com_Error(ERR_DROP, "File %s too long\n", sfilename);
#else
		Com_Printf("File %s too long\n", sfilename);
#endif
		goto fin;
	}

	trap->FS_Read(text, len, f);
	text[len] = 0;
	trap->FS_Close(f);

	// parse the text
	text_p = text;
	upper_i = 0;
	lower_i = 0;

	COM_BeginParseSession("BG_ParseAnimationEvtFile");

	// read information for batches of sounds (UPPER or LOWER)
	while (1)
	{
		// Get base frame of sequence
		token = COM_Parse(&text_p);
		if (!token || !token[0])
		{
			break;
		}

		if (!Q_stricmp(token, "include"))	// grab from another mpanimevents.cfg
		{//NOTE: you REALLY should NOT do this after the main block of UPPERSOUNDS and LOWERSOUNDS
			const char	*include_filename = COM_Parse(&text_p);
			if (include_filename != NULL)
			{
				char fullIPath[MAX_QPATH];
				strcpy(fullIPath, va("models/players/%s/", include_filename));
				bg_animParseIncluding++;
				BG_ParseAnimationEvtFile(fullIPath, animFileIndex, forcedIndex);
				bg_animParseIncluding--;
			}
		}

		if (!Q_stricmp(token, "UPPEREVENTS"))	// A batch of upper sounds
		{
			ParseAnimationEvtBlock(as_filename, torsoAnimEvents, animations, &upper_i, &text_p);
		}
		else if (!Q_stricmp(token, "LOWEREVENTS"))	// A batch of lower sounds
		{
			ParseAnimationEvtBlock(as_filename, legsAnimEvents, animations, &lower_i, &text_p);
		}
	}

	usedIndex = forcedIndex;
fin:
	//Mark this anim set so that we know we tried to load he sounds, don't care if the load failed
	if (bg_animParseIncluding <= 0)
	{ //if we should be parsing an included file, skip this part
		bgAllEvents[forcedIndex].eventsParsed = qtrue;
		strcpy(bgAllEvents[forcedIndex].filename, as_filename);
		if (forcedIndex)
		{
			bgNumAnimEvents++;
		}
	}

	return usedIndex;
}
#endif

/*
======================
BG_ParseAnimationFile

Read a configuration file containing animation counts and rates
models/players/visor/animation.cfg, etc

======================
*/
int BG_ParseAnimationFile(const char *filename, animation_t *animset, qboolean isHumanoid)
{
	char		*text_p;
	int			len;
	int			i;
	char		*token;
	float		fps;
	int			usedIndex = -1;
	int			nextIndex = bgNumAllAnims;
	qboolean	dynAlloc = qfalse;
	static char BGPAFtext[120000];
	fileHandle_t	f;
	int				animNum;

	BGPAFtext[0] = '\0';

	if (!isHumanoid)
	{
		i = 0;
		while (i < bgNumAllAnims)
		{ //see if it's been loaded already
			if (!Q_stricmp(bgAllAnims[i].filename, filename))
			{
				animset = bgAllAnims[i].anims;
				return i; //alright, we already have it.
			}
			i++;
		}

		//Looks like it has not yet been loaded. Allocate space for the anim set if we need to, and continue along.
		if (!animset)
		{
			if (strstr(filename, "players/_humanoid/"))
			{ //then use the static humanoid set.
				animset = bgHumanoidAnimations;
				nextIndex = 0;
			}
			else if (strstr(filename, "players/rockettrooper/"))
			{ //rockettrooper always index 1
				nextIndex = 1;
				animset = BG_AnimsetAlloc();
				dynAlloc = qtrue; //so we know to free this memory in case we have to return early. Don't want any leaks.

				if (!animset)
				{
					assert(!"Anim set alloc failed!");
					return -1;
				}
			}
			else
			{
				animset = BG_AnimsetAlloc();
				dynAlloc = qtrue; //so we know to free this memory in case we have to return early. Don't want any leaks.

				if (!animset)
				{
					assert(!"Anim set alloc failed!");
					return -1;
				}
			}
		}
	}
#ifdef _DEBUG
	else
	{
		assert(animset);
	}
#endif

	// load the file
	if (!BGPAFtextLoaded || !isHumanoid)
	{ //rww - We are always using the same animation config now. So only load it once.
		len = trap->FS_Open(filename, &f, FS_READ);
		if ((len <= 0) || (len >= sizeof(BGPAFtext) - 1))
		{
			trap->FS_Close(f);
			if (dynAlloc)
			{
				BG_AnimsetFree(animset);
			}
			if (len > 0)
			{
				Com_Error(ERR_DROP, "%s exceeds the allowed game-side animation buffer!", filename);
			}
			return -1;
		}

		trap->FS_Read(BGPAFtext, len, f);

		BGPAFtext[len] = 0;
		trap->FS_Close(f);
	}
	else
	{
		if (dynAlloc)
		{
			assert(!"Should not have allocated dynamically for humanoid");
			BG_AnimsetFree(animset);
		}
		return 0; //humanoid index
	}

	// parse the text
	text_p = BGPAFtext;

	//initialize anim array so that from 0 to MAX_ANIMATIONS, set default values of 0 1 0 100
	for (i = 0; i < MAX_ANIMATIONS; i++)
	{
		animset[i].firstFrame = 0;
		animset[i].numFrames = 0;
		animset[i].loopFrames = -1;
		animset[i].frameLerp = 100;
	}

	// read information for each frame
	while (1)
	{
		token = COM_Parse((const char **)(&text_p));

		if (!token || !token[0])
		{
			break;
		}

		animNum = GetIDForString(animTable, token);
		if (animNum == -1)
		{
#ifdef _DEBUG
			if (strcmp(token, "ROOT"))
			{
				Com_Printf(S_COLOR_RED"WARNING: Unknown token %s in %s\n", token, filename);
			}
			while (token[0])
			{
				token = COM_ParseExt((const char **)&text_p, qfalse);	//returns empty string when next token is EOL
			}
#endif
			continue;
		}

		token = COM_Parse((const char **)(&text_p));
		if (!token)
		{
			break;
		}
		animset[animNum].firstFrame = atoi(token);

		token = COM_Parse((const char **)(&text_p));
		if (!token)
		{
			break;
		}
		animset[animNum].numFrames = atoi(token);

		token = COM_Parse((const char **)(&text_p));
		if (!token)
		{
			break;
		}
		animset[animNum].loopFrames = atoi(token);

		token = COM_Parse((const char **)(&text_p));
		if (!token)
		{
			break;
		}
		fps = atof(token);
		if (fps == 0)
		{
			fps = 1;//Don't allow divide by zero error
		}
		if (fps < 0)
		{//backwards
			animset[animNum].frameLerp = floor(1000.0f / fps);

#ifdef __SABER_COMBATANIMATION_SMOOTH__
			{//Slow down saber moves...
				int x;

				for (x = 4; x < LS_MOVE_MAX; x++)
				{
					//if (saberMoveData[x].animToUse + 77 == animNum) // SS_MEDIUM
					//{
					//	animset[animNum].frameLerp *= 1.1;
					//	break;
					//}
#ifdef __NO_SABER_SPINAROUND__
					if (saberMoveData[x].animToUse + (77 * 2) == animNum // SS_STRONG
						|| saberMoveData[x].animToUse + (77 * 3) == animNum) // SS_DESANN
					{
						animset[animNum].frameLerp *= 1.3;
						break;
					}
#endif 
					else if (saberMoveData[x].animToUse + (77 * 4) == animNum) // SS_TAVION
					{
						animset[animNum].frameLerp *= 1.2;
						break;
					}
					else if (saberMoveData[x].animToUse + (77 * 5) == animNum // SS_DUAL
						|| saberMoveData[x].animToUse + (77 * 6) == animNum) // SS_STAFF
					{
						animset[animNum].frameLerp *= 1.2;
						break;
					}
				}
			}
#endif 
		}
		else
		{
			animset[animNum].frameLerp = ceil(1000.0f / fps);

#ifdef __SABER_COMBATANIMATION_SMOOTH__
			{// Slow down saber moves...
				int x;

				for (x = 4; x < LS_MOVE_MAX; x++)
				{
					//if (saberMoveData[x].animToUse + 77 == animNum ) // SS_MEDIUM
					//{
					//	animset[animNum].frameLerp *= 1.1;
					//	break;
					//}
#ifdef __NO_SABER_SPINAROUND__
					if (saberMoveData[x].animToUse + (77 * 2) == animNum // SS_STRONG
						|| saberMoveData[x].animToUse + (77 * 3) == animNum) // SS_DESANN
					{
						animset[animNum].frameLerp *= 1.3;
						break;
					}
#endif
					else if (saberMoveData[x].animToUse + (77 * 4) == animNum) // SS_TAVION
					{
						animset[animNum].frameLerp *= 1.2;
						break;
					}
					else if (saberMoveData[x].animToUse + (77 * 5) == animNum // SS_DUAL
						|| saberMoveData[x].animToUse + (77 * 6) == animNum) // SS_STAFF
					{
						animset[animNum].frameLerp *= 1.2;
						break;
					}
				}
			}
#endif 
		}
	}

#ifdef CONVENIENT_ANIMATION_FILE_DEBUG_THING
	SpewDebugStuffToFile();
#endif


	if (isHumanoid)
	{
		bgAllAnims[0].anims = animset;
		strcpy(bgAllAnims[0].filename, filename);
		BGPAFtextLoaded = qtrue;

		usedIndex = 0;
	}
	else
	{
		bgAllAnims[nextIndex].anims = animset;
		strcpy(bgAllAnims[nextIndex].filename, filename);

		usedIndex = bgNumAllAnims;

		if (nextIndex > 1)
		{ //don't bother increasing the number if this ended up as a humanoid/rockettrooper load.
			bgNumAllAnims++;
		}
		else
		{
			BGPAFtextLoaded = qtrue;
			usedIndex = nextIndex;
		}
	}

	return usedIndex;
}

/*
===================
LEGS Animations
Base animation for overall body
===================
*/
static void BG_StartLegsAnim(playerState_t *ps, int anim)
{
	if (ps->pm_type >= PM_DEAD)
	{
		//vehicles are allowed to do this.. IF it's a vehicle death anim
		if (ps->clientNum < MAX_CLIENTS || anim != BOTH_VT_DEATH1)
		{
			return;
		}
	}
	if (ps->legsTimer > 0)
	{
		return;		// a high priority animation is running
	}

	if (ps->legsAnim == anim)
	{
		BG_FlipPart(ps, SETANIM_LEGS);
	}
#ifdef _GAME
	else if (g_entities[ps->clientNum].s.legsAnim == anim)
	{ //toggled anim to one anim then back to the one we were at previously in
		//one frame, indicating that anim should be restarted.
		BG_FlipPart(ps, SETANIM_LEGS);
	}
#endif
	ps->legsAnim = anim;
}

void PM_ContinueLegsAnim(int anim)
{
	if ((pm->ps->legsAnim) == anim)
	{
		return;
	}
	if (pm->ps->legsTimer > 0 || pm->ps->legsTimer == -1)
	{
		return;		// a high priority animation is running
	}

	BG_StartLegsAnim(pm->ps, anim);
}

void PM_ForceLegsAnim(int anim) {
	if (PM_InSpecialJump(pm->ps->legsAnim) &&
		pm->ps->legsTimer > 0 &&
		!PM_InSpecialJump(anim))
	{
		return;
	}

	if (BG_InRoll(pm->ps, pm->ps->legsAnim) &&
		pm->ps->legsTimer > 0 &&
		!BG_InRoll(pm->ps, anim))
	{
		return;
	}

	pm->ps->legsTimer = 0;
	BG_StartLegsAnim(pm->ps, anim);
}



/*
===================
TORSO Animations
Override animations for upper body
===================
*/
void BG_StartTorsoAnim(playerState_t *ps, int anim)
{
	if (ps->pm_type >= PM_DEAD)
	{
		return;
	}

	if (ps->torsoAnim == anim)
	{
		BG_FlipPart(ps, SETANIM_TORSO);
	}
#ifdef _GAME
	else if (g_entities[ps->clientNum].s.torsoAnim == anim)
	{ //toggled anim to one anim then back to the one we were at previously in
		//one frame, indicating that anim should be restarted.
		BG_FlipPart(ps, SETANIM_TORSO);
	}
#endif
	ps->torsoAnim = anim;
}

void PM_StartTorsoAnim(int anim)
{
	BG_StartTorsoAnim(pm->ps, anim);
}


/*
-------------------------
PM_SetLegsAnimTimer
-------------------------
*/
#ifdef _GAME
extern void Q3_TaskIDClear(int *taskID);
#endif


void BG_SetLegsAnimTimer(playerState_t *ps, int time)
{
	ps->legsTimer = time;

	if (ps->legsTimer < 0 && time != -1)
	{//Cap timer to 0 if was counting down, but let it be -1 if that was intentional.  NOTENOTE Yeah this seems dumb, but it mirrors SP.
		ps->legsTimer = 0;
	}
#ifdef _GAME
	if (!ps->legsTimer && trap->ICARUS_TaskIDPending((sharedEntity_t *)&g_entities[ps->clientNum], TID_ANIM_LOWER))
	{//Waiting for legsAnimTimer to complete, and it just got set to zero
		if (!trap->ICARUS_TaskIDPending((sharedEntity_t *)&g_entities[ps->clientNum], TID_ANIM_BOTH))
		{//Not waiting for top
			trap->ICARUS_TaskIDComplete((sharedEntity_t *)&g_entities[ps->clientNum], TID_ANIM_LOWER);
		}
		else
		{//Waiting for both to finish before complete 
			Q3_TaskIDClear(&g_entities[ps->clientNum].taskID[TID_ANIM_LOWER]);//Bottom is done, regardless
			if (!trap->ICARUS_TaskIDPending((sharedEntity_t *)&g_entities[ps->clientNum], TID_ANIM_UPPER))
			{//top is done and we're done
				trap->ICARUS_TaskIDComplete((sharedEntity_t *)&g_entities[ps->clientNum], TID_ANIM_BOTH);
			}
		}
	}
#endif
}

void PM_SetLegsAnimTimer(int time)
{
	BG_SetLegsAnimTimer(pm->ps, time);
}

/*
-------------------------
PM_SetTorsoAnimTimer
-------------------------
*/
void BG_SetTorsoAnimTimer(playerState_t *ps, int time)
{
	ps->torsoTimer = time;

	if (ps->torsoTimer < 0 && time != -1)
	{//Cap timer to 0 if was counting down, but let it be -1 if that was intentional.  NOTENOTE Yeah this seems dumb, but it mirrors SP.
		ps->torsoTimer = 0;
	}
#ifdef _GAME
	if (!ps->torsoTimer && trap->ICARUS_TaskIDPending((sharedEntity_t *)&g_entities[ps->clientNum], TID_ANIM_UPPER))
	{//Waiting for torsoAnimTimer to complete, and it just got set to zero
		if (!trap->ICARUS_TaskIDPending((sharedEntity_t *)&g_entities[ps->clientNum], TID_ANIM_BOTH))
		{//Not waiting for bottom
			trap->ICARUS_TaskIDComplete((sharedEntity_t *)&g_entities[ps->clientNum], TID_ANIM_UPPER);
		}
		else
		{//Waiting for both to finish before complete 
			Q3_TaskIDClear(&g_entities[ps->clientNum].taskID[TID_ANIM_UPPER]);//Top is done, regardless
			if (!trap->ICARUS_TaskIDPending((sharedEntity_t *)&g_entities[ps->clientNum], TID_ANIM_LOWER))
			{//lower is done and we're done
				trap->ICARUS_TaskIDComplete((sharedEntity_t *)&g_entities[ps->clientNum], TID_ANIM_BOTH);
			}
		}
	}
#endif
}

void PM_SetTorsoAnimTimer(int time)
{
	BG_SetTorsoAnimTimer(pm->ps, time);
}

void PM_SaberStartTransAnim(int clientNum, int saberAnimLevel, int weapon, int anim, float *animSpeed, int broken, int fatigued)
{
	char	buf[128];
	float	saberanimscale  = 0.80f;
	float	redanimscale    = 1.1f;
	float	dualanimscale   = 0.9f;
	float	staffanimscale  = 0.8f;
	float	blueanimscale   = 1.4f;
	float	mediumanimscale = 1.0f;
	float	tavionanimscale = 0.85f;

	trap->Cvar_VariableStringBuffer("g_saberAnimSpeed", buf, sizeof(buf));
	saberanimscale = atof(buf);

	if (weapon != WP_SABER)
	{
		return;
	}

	if (anim >= BOTH_A1_T__B_ && anim <= BOTH_ROLL_STAB)
	{
		if (weapon == WP_SABER)
		{
			saberInfo_t *saber = BG_MySaber(clientNum, 0);
			if (saber && saber->animSpeedScale != 1.0f)
			{
				*animSpeed *= saber->animSpeedScale;
			}
			saber = BG_MySaber(clientNum, 1);

			if (saber && saber->animSpeedScale != 1.0f)
			{
				*animSpeed *= saber->animSpeedScale;
			}
		}
	}

	if ((anim) >= BOTH_A1_T__B_ && (anim) <= BOTH_H7_S7_BR)
	{
		if (saberAnimLevel == SS_STRONG)
		{
			*animSpeed *= redanimscale;
		}
		else if (saberAnimLevel == SS_DUAL)
		{
			*animSpeed *= dualanimscale;
		}
		else if (saberAnimLevel == SS_STAFF)
		{
			*animSpeed *= staffanimscale;
		}
	}

	if (saberAnimLevel == SS_MEDIUM)
	{
		*animSpeed *= mediumanimscale;
	}

	if ((anim >= BOTH_S1_S1_T_ && anim <= BOTH_S1_S1_TR) ||
		(anim >= BOTH_S2_S1_T_ && anim <= BOTH_S2_S1_TR) ||
		(anim >= BOTH_S3_S1_T_ && anim <= BOTH_S3_S1_TR) ||
		(anim >= BOTH_S4_S1_T_ && anim <= BOTH_S4_S1_TR) ||
		(anim >= BOTH_S5_S1_T_ && anim <= BOTH_S5_S1_TR))
	{
		if (saberAnimLevel == SS_FAST)
		{
			*animSpeed *= blueanimscale;
		}
		else if (saberAnimLevel == SS_TAVION)
		{
			*animSpeed *= tavionanimscale;
		}
		else if (saberAnimLevel == SS_STRONG)
		{
			*animSpeed *= redanimscale;
		}
		else
		{
			*animSpeed *= saberanimscale;
		}
	}
	else if (broken && PM_InSaberAnim(anim))
	{
		if (broken & (1 << BROKENLIMB_RARM))
		{
			*animSpeed *= 0.5f;
		}
		else if (broken & (1 << BROKENLIMB_LARM))
		{
			*animSpeed *= 0.65f;
		}
	}

	if ((anim >= BOTH_T1_BR__R && anim <= BOTH_T1_BL_TL) ||
		(anim >= BOTH_T2_BR__R && anim <= BOTH_T2_BL_TL) ||
		(anim >= BOTH_T3_BR__R && anim <= BOTH_T3_BL_TL) ||
		(anim >= BOTH_T4_BR__R && anim <= BOTH_T4_BL_TL) ||
		(anim >= BOTH_T5_BR__R && anim <= BOTH_T5_BL_TL) ||
		(anim >= BOTH_T6_BR__R && anim <= BOTH_T6_BL_TL) ||
		(anim >= BOTH_T7_BR__R && anim <= BOTH_T7_BL_TL))
	{
		if (saberAnimLevel == SS_FAST)
		{
			*animSpeed *= blueanimscale;
		}
		else if (saberAnimLevel == SS_TAVION)
		{
			*animSpeed *= tavionanimscale;
		}
		else if (saberAnimLevel == SS_STRONG)
		{
			*animSpeed *= redanimscale;
		}
		else
		{
			*animSpeed *= saberanimscale;
		}
	}
	else if (broken && PM_InSaberAnim(anim))
	{
		if (broken & (1 << BROKENLIMB_RARM))
		{
			*animSpeed *= 0.5f;
		}
		else if (broken & (1 << BROKENLIMB_LARM))
		{
			*animSpeed *= 0.65f;
		}
	}

	if (weapon == WP_SABER && PM_InSaberAnim(anim))
	{
		if (saberAnimLevel == SS_FAST)
		{
			*animSpeed *= 0.75f;
		}
		else
		{
			*animSpeed *= saberanimscale;
		}
	}

	//slow down the saber speeds
	if (anim >= BOTH_A1_T__B_ && anim <= BOTH_ROLL_STAB && !PM_SaberInSpecialAttack(anim)
		&& !PM_SaberReturnAnim(anim) && anim != BOTH_FORCEWALLRELEASE_FORWARD
		&& anim != BOTH_FORCEWALLRUNFLIP_START && anim != BOTH_FORCEWALLRUNFLIP_END)
	{
		*animSpeed *= saberanimscale;
	}
	if ((anim >= BOTH_H1_S1_T_ && anim <= BOTH_H1_S1_BR)
		|| (anim >= BOTH_H6_S6_T_ && anim <= BOTH_H6_S6_BR)
		|| (anim >= BOTH_H7_S7_T_ && anim <= BOTH_H7_S7_BR))
	{//slow down broken parries
		if (anim >= BOTH_H6_S6_T_ && anim <= BOTH_H6_S6_BR)
		{//dual broken parries are 1/3 the frames of the single broken parries
			*animSpeed *= 0.6f;
		}
		else if (anim >= BOTH_H7_S7_T_ && anim <= BOTH_H7_S7_BR)
		{//doubles are 1/2 the frames of single broken parries
			*animSpeed *= 0.8f;
		}
	}

	if ((fatigued & (1 << FLAG_FATIGUED)) && anim >= BOTH_A1_T__B_ && anim <= BOTH_ROLL_STAB
		//not a wall run move
		&& anim != BOTH_FORCEWALLRELEASE_FORWARD && anim != BOTH_FORCEWALLRUNFLIP_START
		&& anim != BOTH_FORCEWALLRUNFLIP_END
		&& anim != BOTH_JUMPFLIPSTABDOWN
		&& anim != BOTH_JUMPFLIPSLASHDOWN1
		&& anim != BOTH_LUNGE2_B__T_)
	{//You're pooped.  Move slower
		*animSpeed *= 0.8f;
	}

	if ((fatigued & (1 << FLAG_SLOWBOUNCE)))
	{//slow animation for slow bounces
		if (PM_BounceAnim(anim))
		{
			*animSpeed *= 0.6f;
		}
		else if (PM_SaberReturnAnim(anim))
		{
			*animSpeed *= 0.8f;
		}
	}
	else if ((fatigued & (1 << FLAG_OLDSLOWBOUNCE)))
	{//getting parried slows down your reaction
		if (PM_BounceAnim(anim) || PM_SaberReturnAnim(anim))
		{//only apply to bounce and returns since this flag is technically turned off immediately after the animation is set.
			*animSpeed *= 0.6f;
		}
	}
	else if ((fatigued & (1 << FLAG_PARRIED)))
	{//getting parried slows down your reaction
		if (PM_BounceAnim(anim) || PM_SaberReturnAnim(anim))
		{//only apply to bounce and returns since this flag is technically turned off immediately after the animation is set.
			*animSpeed *= 0.8f;
		}
	}
	else if ((fatigued & (1 << FLAG_QUICKPARRY)))
	{
		if (PM_BounceAnim(anim) || PM_SaberReturnAnim(anim))
		{
			*animSpeed *= 0.95f;
		}
	}
}

/*
-------------------------
PM_SetAnimFinal
-------------------------
*/
qboolean PM_RunningAnim(int anim);
qboolean PM_WalkingAnim(int anim);

void BG_SetAnimFinal(playerState_t *ps, animation_t *animations, int setAnimParts, int anim, int setAnimFlags)
{
	float editAnimSpeed = 1;

	if (!animations)
	{
		return;
	}

	assert(anim > -1);
	assert(animations[anim].firstFrame > 0 || animations[anim].numFrames > 0);

	PM_SaberStartTransAnim(ps->clientNum, ps->fd.saberAnimLevel, ps->weapon, anim, &editAnimSpeed, ps->brokenLimbs, ps->userInt3);

	// Set torso anim
	if (setAnimParts & SETANIM_TORSO)
	{
		// Don't reset if it's already running the anim
		if ((ps->torsoAnim) == anim && !(setAnimFlags & SETANIM_FLAG_RESTART) && !(setAnimFlags & SETANIM_FLAG_PACE))
		{
			goto setAnimLegs;
		}
		// or if a more important anim is running
		if (((ps->torsoTimer > 0) || (ps->torsoTimer == -1)) &&
			(((setAnimFlags & SETANIM_FLAG_PACE) && (ps->torsoAnim) == anim)
				|| !(setAnimFlags & SETANIM_FLAG_OVERRIDE)))
		{
			goto setAnimLegs;
		}

		BG_StartTorsoAnim(ps, anim);

		if (setAnimFlags & SETANIM_FLAG_HOLD)
		{
			if (setAnimFlags & SETANIM_FLAG_HOLDLESS)
			{	// Make sure to only wait in full 1/20 sec server frame intervals.
				if (editAnimSpeed > 0)
				{
					if (animations[anim].numFrames < 2)
					{//single frame animations should just run with one frame worth of animation.
						ps->torsoTimer = fabs((float)(animations[anim].frameLerp)) * (1 / editAnimSpeed);
					}
					else
					{
						ps->torsoTimer = (animations[anim].numFrames - 1) * fabs((float)(animations[anim].frameLerp)) * (1 / editAnimSpeed);
					}

					if (ps->torsoTimer > 1)
					{
						//set the timer to be one unit of time less than the actual animation time so the timer will expire on the frame at which the animation finishes.
						ps->torsoTimer--;
					}
				}
			}
			else
			{
				ps->torsoTimer = ((animations[anim].numFrames) * fabs((float)(animations[anim].frameLerp)));
			}

			if (ps->fd.forcePowersActive & (1 << FP_RAGE))
			{
				ps->torsoTimer /= 1.7;
			}
		}
	}

setAnimLegs:
	// Set legs anim
	if (setAnimParts & SETANIM_LEGS)
	{
		// Don't reset if it's already running the anim
		if ((ps->legsAnim) == anim && !(setAnimFlags & SETANIM_FLAG_RESTART) && !(setAnimFlags & SETANIM_FLAG_PACE))
		{
			goto setAnimDone;
		}
		// or if a more important anim is running
		if (((ps->legsTimer > 0) || (ps->legsTimer == -1)) && (((setAnimFlags & SETANIM_FLAG_PACE) && (ps->legsAnim) == anim)
			|| !(setAnimFlags & SETANIM_FLAG_OVERRIDE)))
		{
			goto setAnimDone;
		}

		BG_StartLegsAnim(ps, anim);

		if (setAnimFlags & SETANIM_FLAG_HOLD)
		{
			if (setAnimFlags & SETANIM_FLAG_HOLDLESS)
			{	// Make sure to only wait in full 1/20 sec server frame intervals.
				int dur;
				int speedDif;

				dur = (animations[anim].numFrames - 1) * fabs((float)(animations[anim].frameLerp));
				speedDif = dur - (dur * editAnimSpeed);
				dur += speedDif;
				if (dur > 1)
				{
					ps->legsTimer = dur - 1;
				}
				else
				{
					ps->legsTimer = fabs((float)(animations[anim].frameLerp));
				}
			}
			else
			{
				ps->legsTimer = ((animations[anim].numFrames) * fabs((float)(animations[anim].frameLerp)));
			}

			if (PM_RunningAnim(anim) ||
				PM_WalkingAnim(anim)) //these guys are ok, they don't actually reference pm
			{
				if (ps->fd.forcePowersActive & (1 << FP_RAGE))
				{
					ps->legsTimer /= 1.3;
				}
				else if (ps->fd.forcePowersActive & (1 << FP_SPEED))
				{
					ps->legsTimer /= 1.7;
				}
			}
		}
	}

setAnimDone:
	return;
}

void PM_SetAnimFinal(int setAnimParts, int anim, int setAnimFlags)
{
	BG_SetAnimFinal(pm->ps, pm->animations, setAnimParts, anim, setAnimFlags);
}


qboolean BG_HasAnimation(int animIndex, int animation)
{
	animation_t *animations;

	//must be a valid anim number
	if (animation < 0 || animation >= MAX_ANIMATIONS)
	{
		return qfalse;
	}

	//Must have a file index entry
	if (animIndex < 0 || animIndex > bgNumAllAnims)
		return qfalse;

	animations = bgAllAnims[animIndex].anims;

	//No frames, no anim
	if (animations[animation].numFrames == 0)
		return qfalse;

	//Has the sequence
	return qtrue;
}

int PM_PickAnim(int animIndex, int minAnim, int maxAnim)
{
	int anim;
	int count = 0;

	do
	{
		anim = Q_irand(minAnim, maxAnim);
		count++;
	} while (!BG_HasAnimation(animIndex, anim) && count < 1000);

	if (count == 1000)
	{ //guess we just don't have a death anim then.
		return -1;
	}

	return anim;
}

//I want to be able to use this on a playerstate even when we are not the focus
//of a pmove too so I have ported it to true BGishness.
//Please do not reference pm in this function or any functions that it calls,
//or I will cry. -rww
void BG_SetAnim(playerState_t *ps, animation_t *animations, int setAnimParts, int anim, int setAnimFlags)
{
	if (!animations)
	{
		animations = bgAllAnims[0].anims;
	}

	if (animations[anim].firstFrame == 0 && animations[anim].numFrames == 0)
	{
		if (anim == BOTH_RUNBACK1 ||
			anim == BOTH_WALKBACK1 ||
			anim == BOTH_RUN1)
		{ //hack for droids
			anim = BOTH_WALK2;
		}

		if (animations[anim].firstFrame == 0 && animations[anim].numFrames == 0)
		{ //still? Just return then I guess.
			return;
		}
	}

	if (ps->stats[STAT_HEALTH] > 0)
	{//don't lock anims if the guy is dead
		if (ps->torsoTimer
			&& PM_LockedAnim(ps->torsoAnim)
			&& !PM_LockedAnim(anim))
		{//nothing can override these special anims
			setAnimParts &= ~SETANIM_TORSO;
		}

		if (ps->legsTimer
			&& PM_LockedAnim(ps->legsAnim)
			&& !PM_LockedAnim(anim))
		{//nothing can override these special anims
			setAnimParts &= ~SETANIM_LEGS;
		}
	}

	if (setAnimFlags&SETANIM_FLAG_OVERRIDE)
	{
		if (setAnimParts & SETANIM_TORSO)
		{
			if ((setAnimFlags & SETANIM_FLAG_RESTART) || (ps->torsoAnim) != anim)
			{
				BG_SetTorsoAnimTimer(ps, 0);
			}
		}
		if (setAnimParts & SETANIM_LEGS)
		{
			if ((setAnimFlags & SETANIM_FLAG_RESTART) || (ps->legsAnim) != anim)
			{
				BG_SetLegsAnimTimer(ps, 0);
			}
		}
	}

	BG_SetAnimFinal(ps, animations, setAnimParts, anim, setAnimFlags);
}

void PM_SetAnim(int setAnimParts, int anim, int setAnimFlags)
{
	BG_SetAnim(pm->ps, pm->animations, setAnimParts, anim, setAnimFlags);
}

float BG_GetTorsoAnimPoint(playerState_t * ps, int AnimIndex)
{
	float attackAnimLength = 0;
	float currentPoint = 0;
	float animSpeedFactor = 1.0f;
	float animPercentage = 0;

	//Be sure to scale by the proper anim speed just as if we were going to play the animation
	PM_SaberStartTransAnim(ps->clientNum, ps->fd.saberAnimLevel, ps->weapon, ps->torsoAnim, &animSpeedFactor, ps->brokenLimbs, ps->userInt3);

	if (animSpeedFactor > 0)
	{
		if (bgAllAnims[AnimIndex].anims[ps->torsoAnim].numFrames < 2)
		{//single frame animations should just run with one frame worth of animation.
			attackAnimLength = fabs((float)(bgAllAnims[AnimIndex].anims[ps->torsoAnim].frameLerp)) * (1 / animSpeedFactor);
		}
		else
		{
			attackAnimLength = (bgAllAnims[AnimIndex].anims[ps->torsoAnim].numFrames - 1) * fabs((float)(bgAllAnims[AnimIndex].anims[ps->torsoAnim].frameLerp)) * (1 / animSpeedFactor);
		}

		if (attackAnimLength > 1)
		{
			//set the timer to be one unit of time less than the actual animation time so the timer will expire on the frame at which the animation finishes.
			attackAnimLength--;
		}
	}

	currentPoint = ps->torsoTimer;

	animPercentage = currentPoint / attackAnimLength;

	return animPercentage;
}

float BG_GetLegsAnimPoint(playerState_t * ps, int AnimIndex)
{
	float attackAnimLength = 0;
	float currentPoint = 0;
	float animSpeedFactor = 1.0f;
	float animPercentage = 0;

	//Be sure to scale by the proper anim speed just as if we were going to play the animation
	PM_SaberStartTransAnim(ps->clientNum, ps->fd.saberAnimLevel, ps->weapon, ps->legsAnim, &animSpeedFactor, ps->brokenLimbs, ps->userInt3);

	if (animSpeedFactor > 0)
	{
		if (bgAllAnims[AnimIndex].anims[ps->legsAnim].numFrames < 2)
		{//single frame animations should just run with one frame worth of animation.
			attackAnimLength = fabs((float)(bgAllAnims[AnimIndex].anims[ps->legsAnim].frameLerp)) * (1 / animSpeedFactor);
		}
		else
		{
			attackAnimLength = (bgAllAnims[AnimIndex].anims[ps->legsAnim].numFrames - 1) * fabs((float)(bgAllAnims[AnimIndex].anims[ps->legsAnim].frameLerp)) * (1 / animSpeedFactor);
		}

		if (attackAnimLength > 1)
		{
			//set the timer to be one unit of time less than the actual animation time so the timer will expire on the frame at which the animation finishes.
			attackAnimLength--;
		}
	}

	currentPoint = ps->legsTimer;

	animPercentage = currentPoint / attackAnimLength;

	return animPercentage;
}

qboolean BG_HopAnim(int anim)
{//check to see if anim is a hop animation.
	if (BOTH_HOP_F <= anim && anim <= BOTH_HOP_R
		|| BOTH_DASH_F <= anim && anim <= BOTH_DASH_R)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean PM_BounceAnim(int anim)
{//check for saber bounce animation
	if ((anim >= BOTH_B1_BR___ && anim <= BOTH_B1_BL___)
		|| (anim >= BOTH_B2_BR___ && anim <= BOTH_B2_BL___)
		|| (anim >= BOTH_B3_BR___ && anim <= BOTH_B3_BL___)
		|| (anim >= BOTH_B4_BR___ && anim <= BOTH_B4_BL___)
		|| (anim >= BOTH_B5_BR___ && anim <= BOTH_B5_BL___)
		|| (anim >= BOTH_B6_BR___ && anim <= BOTH_B6_BL___)
		|| (anim >= BOTH_B7_BR___ && anim <= BOTH_B7_BL___))
	{
		return qtrue;
	}

	return qfalse;
}

qboolean BG_SprintAnim(int anim)
{
	switch (anim)
	{
	case BOTH_SPRINT:
		return qtrue;
		break;
	}
	return qfalse;
}

qboolean Manual_Saberreadyanim(int anim)
{//check for saber block animation
	switch (anim)
	{//special case anims
	case BOTH_STAND1://not really a saberstance anim, actually... "saber off" stance
	case BOTH_STAND2://single-saber, medium style
	case BOTH_SABERFAST_STANCE://single-saber, fast style
	case BOTH_SABERSLOW_STANCE://single-saber, strong style
	case BOTH_SABERSTAFF_STANCE://saber staff style
	case BOTH_SABERSTAFF_STANCE_IDLE://saber staff style
	case BOTH_SABERDUAL_STANCE://dual saber style
	case BOTH_SABERDUAL_STANCE_GRIEVOUS://dual saber style
	case BOTH_SABERDUAL_STANCE_IDLE://dual saber style
	case BOTH_SABERTAVION_STANCE://tavion saberstyle
	case BOTH_SABERDESANN_STANCE://desann saber style
	case BOTH_SABERSTANCE_STANCE://similar to stand2
	case BOTH_SABERYODA_STANCE://YODA saber style
	case BOTH_SABERBACKHAND_STANCE://BACKHAND saber style
	case BOTH_SABERDUAL_STANCE_ALT://alt dual
	case BOTH_SABERSTAFF_STANCE_ALT://alt staff
	case BOTH_SABERSTANCE_STANCE_ALT://alt idle
	case BOTH_SABEROBI_STANCE://obiwan
	case BOTH_SABEREADY_STANCE://ready
	case BOTH_SABER_REY_STANCE://rey
		return qtrue;
	}
	return qfalse;
}

qboolean Manual_Saberblockinganim(int anim)
{//check for saber block animation
	switch (anim)
	{//special case anims
	case BOTH_BLOCK_BL:
	case BOTH_BLOCK_HOLD_BL:
	case BOTH_BLOCK_BR:
	case BOTH_BLOCK_HOLD_BR:
	case BOTH_BLOCK_FL:
	case BOTH_BLOCK_HOLD_FL:
	case BOTH_BLOCK_FR:
	case BOTH_BLOCK_HOLD_FR:
	case BOTH_BLOCK_L:
	case BOTH_BLOCK_HOLD_L:
	case BOTH_BLOCK_R:
	case BOTH_BLOCK_HOLD_R:
		//
	case BOTH_BLOCK_BACK_R:
	case BOTH_BLOCK_BACK_HOLD_R:
	case BOTH_BLOCK_BACK_L:
	case BOTH_BLOCK_BACK_HOLD_L:
		//
	case BOTH_BLOCK_BACK_HAND:
		//
	case BOTH_P1_S1_BL:
	case BOTH_P1_S1_BR:
	case BOTH_P1_S1_TL:
	case BOTH_P1_S1_TR:
	case BOTH_P1_S1_T1_:
	case BOTH_P1_S1_T_:
	case BOTH_P1_S1_B_:
	case BOTH_P1_S1_B1_:
		//
	case BOTH_P6_S6_BL:
	case BOTH_P6_S6_BR:
	case BOTH_P6_S6_TL:
	case BOTH_P6_S6_TR:
	case BOTH_P6_S6_T_:
	case BOTH_P6_S1_B_:
	case BOTH_P6_S1_B1_:
	case BOTH_P6_S6_TG_:
		//
	case BOTH_P7_S7_BL:
	case BOTH_P7_S7_BR:
	case BOTH_P7_S7_TL:
	case BOTH_P7_S7_TR:
	case BOTH_P7_S7_T_:
	case BOTH_P7_S1_B_:
	case BOTH_P7_S1_B1_:
		//
		return qtrue;
	}
	return qfalse;
}

