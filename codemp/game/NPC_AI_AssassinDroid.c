//ported from SP
#include "b_local.h"

#define	ASSASSIN_SHIELD_SIZE	75
#define TURN_ON					0x00000000
#define TURN_OFF				0x00000100



////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
qboolean BubbleShield_IsOn(gentity_t *self)
{
	return (self->flags&FL_SHIELDED);
}

////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
void BubbleShield_TurnOn(gentity_t *self)
{
	if (!BubbleShield_IsOn(self))
	{
		self->flags |= FL_SHIELDED;
		NPCS.NPC->client->ps.powerups[PW_GALAK_SHIELD] = Q3_INFINITE;
		NPC_SetSurfaceOnOff(self, "force_shield", TURN_ON);
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
void BubbleShield_TurnOff(gentity_t *self)
{
	if (BubbleShield_IsOn(self))
	{
		self->flags &= ~FL_SHIELDED;
		NPCS.NPC->client->ps.powerups[PW_GALAK_SHIELD] = 0;
		NPC_SetSurfaceOnOff(self, "force_shield", TURN_OFF);
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// Push A Particular Ent
////////////////////////////////////////////////////////////////////////////////////////
void BubbleShield_PushEnt(gentity_t* pushed, vec3_t smackDir)
{

	G_Damage(pushed, NPCS.NPC, NPCS.NPC, smackDir, NPCS.NPC->r.currentOrigin, (g_npcspskill.integer + 1)*Q_irand(5, 10), DAMAGE_NO_KNOCKBACK, MOD_ELECTROCUTE);
	G_Throw(pushed, smackDir, 10);

	// Make Em Electric
	//------------------
	if (pushed->client)
	{
		pushed->client->ps.electrifyTime = level.time + 1000;
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// Go Through All The Ents Within The Radius Of The Shield And Push Them
////////////////////////////////////////////////////////////////////////////////////////
void BubbleShield_PushRadiusEnts()
{
	int			numEnts, i;
	int entityList[MAX_GENTITIES];
	gentity_t*	radiusEnt;
	const float	radius = ASSASSIN_SHIELD_SIZE;
	vec3_t		mins, maxs;
	vec3_t		smackDir;
	float		smackDist;

	for (i = 0; i < 3; i++)
	{
		mins[i] = NPCS.NPC->r.currentOrigin[i] - radius;
		maxs[i] = NPCS.NPC->r.currentOrigin[i] + radius;
	}

	numEnts = trap->EntitiesInBox(mins, maxs, entityList, 128);
	for (i = 0; i<numEnts; i++)
	{
		radiusEnt = &g_entities[entityList[i]];
		// Only Clients
		//--------------
		if (!radiusEnt || !radiusEnt->client)
		{
			continue;
		}

		// Don't Push Away Other Assassin Droids
		//---------------------------------------
		if (radiusEnt->client->NPC_class == NPCS.NPC->client->NPC_class)
		{
			continue;
		}

		// Should Have Already Pushed The Enemy If He Touched Us
		//-------------------------------------------------------
		if (NPCS.NPC->enemy &&  NPCS.NPCInfo->touchedByPlayer == NPCS.NPC->enemy && radiusEnt == NPCS.NPC->enemy)
		{
			continue;
		}

		// Do The Vector Distance Test
		//-----------------------------
		VectorSubtract(radiusEnt->r.currentOrigin, NPCS.NPC->r.currentOrigin, smackDir);
		smackDist = VectorNormalize(smackDir);
		if (smackDist<radius)
		{
			BubbleShield_PushEnt(radiusEnt, smackDir);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
void BubbleShield_Update(void)
{
	// Shields Go When You Die
	//-------------------------
	if (NPCS.NPC->health <= 0)
	{
		BubbleShield_TurnOff(NPCS.NPC);
		return;
	}


	// Recharge Shields
	//------------------
	NPCS.NPC->client->ps.stats[STAT_ARMOR] += 1;
	if (NPCS.NPC->client->ps.stats[STAT_ARMOR]>250)
	{
		NPCS.NPC->client->ps.stats[STAT_ARMOR] = 250;
	}




	// If We Have Enough Armor And Are Not Shooting Right Now, Kick The Shield On
	//----------------------------------------------------------------------------
	if (NPCS.NPC->client->ps.stats[STAT_ARMOR]>100 && TIMER_Done(NPCS.NPC, "ShieldsDown"))
	{
		// Check On Timers To Raise And Lower Shields
		//--------------------------------------------
		if ((level.time - NPCS.NPCInfo->enemyLastSeenTime)<1000 && TIMER_Done(NPCS.NPC, "ShieldsUp"))
		{
			TIMER_Set(NPCS.NPC, "ShieldsDown", 2000);		// Drop Shields
			TIMER_Set(NPCS.NPC, "ShieldsUp", Q_irand(4000, 5000));	// Then Bring Them Back Up For At Least 3 sec
		}

		BubbleShield_TurnOn(NPCS.NPC);
		if (BubbleShield_IsOn(NPCS.NPC))
		{
			// Update Our Shader Value
			//-------------------------
			NPCS.NPC->s.customRGBA[0] =
				NPCS.NPC->s.customRGBA[1] =
				NPCS.NPC->s.customRGBA[2] =
				NPCS.NPC->s.customRGBA[3] = (NPCS.NPC->client->ps.stats[STAT_ARMOR] - 100);


			// If Touched By An Enemy, ALWAYS Shove Them
			//-------------------------------------------
			if (NPCS.NPC->enemy &&  NPCS.NPCInfo->touchedByPlayer == NPCS.NPC->enemy)
			{
				vec3_t dir;
				VectorSubtract(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, dir);
				VectorNormalize(dir);
				BubbleShield_PushEnt(NPCS.NPC->enemy, dir);
			}

			// Push Anybody Else Near
			//------------------------
			BubbleShield_PushRadiusEnts();
		}
	}
	else
	{
		BubbleShield_TurnOff(NPCS.NPC);
	}
}


