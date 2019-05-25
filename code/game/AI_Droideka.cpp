/*
This file is part of Serenity.
*/
// leave this line at the top of all AI_xxxx.cpp files for PCH reasons...

	    
#include "b_local.h"
#include "g_functions.h"


#define	MIN_MELEE_RANGE		640
#define	MIN_MELEE_RANGE_SQR	( MIN_MELEE_RANGE * MIN_MELEE_RANGE )

#define MIN_DISTANCE		128
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define DROIDEKA_SHIELD_HEALTH	250
#define SHIELD_EFFECT_TIME	500
#define	GENERATOR_HEALTH	10
#define TURN_ON				0x00000000
#define TURN_OFF			0x00000100
static vec3_t shieldMins = {-20, -20, -24 };
static vec3_t shieldMaxs = {20, 20, 40};
extern void NPC_SetPainEvent( gentity_t *self );

/*
-------------------------
NPC_ATST_Precache
-------------------------
*/
void NPC_DROIDEKA_Precache(void)
{
	G_SoundIndex( "sound/chars/droideka/pain25" );
	G_SoundIndex( "sound/chars/droideka/pain100" );	
	G_EffectIndex( "env/med_explode2");
	G_EffectIndex( "env/small_explode2");
	G_EffectIndex( "galak/explode");
}

void NPC_DROIDEKA_Init( gentity_t *ent )
{
	if (ent->NPC->behaviorState != BS_CINEMATIC)
	{
		ent->client->ps.stats[STAT_ARMOR] = DROIDEKA_SHIELD_HEALTH;
		ent->NPC->investigateCount = ent->NPC->investigateDebounceTime = 0;
		ent->flags |= FL_SHIELDED;//reflect normal shots
		ent->client->ps.powerups[PW_GALAK_SHIELD] = Q3_INFINITE;//temp, for effect
		ent->fx_time = level.time;
		VectorSet( ent->mins, -20, -20, -24 );
		VectorSet( ent->maxs, 20, 20, 40 );
		ent->flags |= FL_NO_KNOCKBACK;//don't get pushed
		TIMER_Set( ent, "attackDelay", 0 );//
		TIMER_Set( ent, "stand", 0 );//
		gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "torso_shield_off", TURN_ON );
	}
	else
	{
		gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "torso_shield_off", TURN_OFF );
	}
}

//-----------------------------------------------------------------
static void DROIDEKA_CreateExplosion( gentity_t *self, const int boltID, qboolean doSmall = qfalse )
{
	if ( boltID >=0 )
	{
		mdxaBone_t	boltMatrix;
		vec3_t		org, dir;
		gi.G2API_GetBoltMatrix( self->ghoul2, self->playerModel,boltID,&boltMatrix, self->currentAngles, self->currentOrigin, level.time,NULL, self->s.modelScale);
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, org );
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, dir );

		if ( doSmall )
		{
			G_PlayEffect( "env/small_explode2", org, dir );
		}
		else
		{
			G_PlayEffect( "env/med_explode2", org, dir );
		}
	}
}

/*
-------------------------
GM_Dying
-------------------------
*/

void DROIDEKA_Dying( gentity_t *self )
{
	if ( level.time - self->s.time < 4000 )
	{//FIXME: need a real effect
		self->s.powerups |= ( 1 << PW_SHOCKED );
		self->client->ps.powerups[PW_SHOCKED] = level.time + 1000;
		if ( TIMER_Done( self, "dyingExplosion" ) )
		{
			int	newBolt;
			switch ( Q_irand( 1, 14 ) )
			{
			// Find place to generate explosion
			case 1:
				if (!gi.G2API_GetSurfaceRenderStatus( &self->ghoul2[self->playerModel], "r_hand" ))
				{//r_hand still there
					DROIDEKA_CreateExplosion( self, self->handRBolt, qtrue );
					gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "r_hand", TURN_OFF );
				}
				else if (!gi.G2API_GetSurfaceRenderStatus( &self->ghoul2[self->playerModel], "r_arm_middle" ))
				{//r_arm_middle still there
					newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*r_arm_elbow" );
					gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "r_arm_middle", TURN_OFF );
				}
				break;
			case 2:
				//FIXME: do only once?
				if (!gi.G2API_GetSurfaceRenderStatus( &self->ghoul2[self->playerModel], "l_hand" ))
				{//l_hand still there
					DROIDEKA_CreateExplosion( self, self->handLBolt );
					gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "l_hand", TURN_OFF );
				}
				else if (!gi.G2API_GetSurfaceRenderStatus( &self->ghoul2[self->playerModel], "l_arm_wrist" ))
				{//l_arm_wrist still there
					newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*l_arm_cap_l_hand" );
					gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "l_arm_wrist", TURN_OFF );
				}
				else if (!gi.G2API_GetSurfaceRenderStatus( &self->ghoul2[self->playerModel], "l_arm_middle" ))
				{//l_arm_middle still there
					newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*l_arm_cap_l_hand" );
					gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "l_arm_middle", TURN_OFF );
				}
				else if (!gi.G2API_GetSurfaceRenderStatus( &self->ghoul2[self->playerModel], "l_arm_augment" ))
				{//l_arm_augment still there
					newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*l_arm_elbow" );
					gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "l_arm_augment", TURN_OFF );
				}
				break;
			case 3:
			case 4:
				newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*hip_fr" );
				DROIDEKA_CreateExplosion( self, newBolt );
				break;
			case 5:
			case 6:
				newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*shldr_l" );
				DROIDEKA_CreateExplosion( self, newBolt );
				break;
			case 7:
			case 8:
				newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*uchest_r" );
				DROIDEKA_CreateExplosion( self, newBolt );
				break;
			case 9:
			case 10:
				DROIDEKA_CreateExplosion( self, self->headBolt );
				break;
			case 11:
				newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*l_leg_knee" );
				DROIDEKA_CreateExplosion( self, newBolt, qtrue );
				break;
			case 12:
				newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*r_leg_knee" );
				DROIDEKA_CreateExplosion( self, newBolt, qtrue );
				break;
			case 13:
				newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*l_leg_foot" );
				DROIDEKA_CreateExplosion( self, newBolt, qtrue );
				break;
			case 14:
				newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*r_leg_foot" );
				DROIDEKA_CreateExplosion( self, newBolt, qtrue );
				break;
			}

			TIMER_Set( self, "dyingExplosion", Q_irand( 300, 1100 ) );
		}
	}
	else
	{//one final, huge explosion
		G_PlayEffect( "galak/explode", self->currentOrigin );
		self->nextthink = level.time + FRAMETIME;
		self->e_ThinkFunc = thinkF_G_FreeEntity;
	}
}

/*
-------------------------
G_DROIDEKACheckPain

Called by NPC's and player in an DROIDEKA
-------------------------
*/
void G_DROIDEKACheckPain( gentity_t *self, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc )
{	
	if ( rand() & 1 )
	{
		G_SoundOnEnt( self, CHAN_LESS_ATTEN, "sound/chars/droideka/pain25" );
	}
	else
	{
		G_SoundOnEnt( self, CHAN_LESS_ATTEN, "sound/chars/droideka/pain100" );
	}
}
/*
-------------------------
NPC_DROIDEKA_Pain
-------------------------
*/
void NPC_DROIDEKA_Pain( gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc ) 
{
	//if ( self->client->ps.powerups[PW_GALAK_SHIELD] == 0 )
	//{//shield is currently down
	//	if ( (hitLoc==HL_GENERIC1) && (self->locationDamage[HL_GENERIC1] > GENERATOR_HEALTH) )
	//	{
	//		gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "torso_shield_off", TURN_OFF );
	//		self->client->ps.powerups[PW_GALAK_SHIELD] = 0;//temp, for effect
	//		self->client->ps.stats[STAT_ARMOR] = 0;//no more armor
	//		self->NPC->investigateDebounceTime = 0;//stop recharging

	//		NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	//		TIMER_Set( self, "attackDelay", self->client->ps.torsoAnimTimer );
	//		G_AddEvent( self, Q_irand( EV_DEATH1, EV_DEATH3 ), self->health );
	//	}
	//}
	//else
	//{//store the point for shield impact
	//	if ( point )
	//	{
	//		VectorCopy( point, self->pos4 );
	//		self->client->poisonTime = level.time;
	//	}
	//}

	//if ( !self->lockCount && !self->client->ps.torsoAnimTimer )
	//{//don't interrupt laser sweep attack or other special attacks/moves
	//	NPC_Pain( self, inflictor, other, point, damage, mod );
	//}
	//else if ( hitLoc == HL_GENERIC1 )
	//{
	//	NPC_SetPainEvent( self );
	//	self->s.powerups |= ( 1 << PW_SHOCKED );
	//	self->client->ps.powerups[PW_SHOCKED] = level.time + Q_irand( 500, 2500 );
	//}
	
	NPC_Pain( self, inflictor, other, point, damage, mod );
	G_DROIDEKACheckPain( self, other, point, damage, mod, hitLoc );	
}

/*
-------------------------
DROIDEKA_Hunt
-------------------------`
*/
void DROIDEKA_Hunt( qboolean visible, qboolean advance )
{
	//If we're not supposed to stand still, pursue the player
	if ( NPCInfo->standTime < level.time )
	{
		//Move towards our goal
		NPCInfo->goalEntity = NPC->enemy;
		NPCInfo->goalRadius = 12;
		NPC_MoveToGoal( qtrue );
	}

	if ( NPCInfo->goalEntity == NULL )
	{//hunt
		NPCInfo->goalEntity = NPC->enemy;
	}
	NPCInfo->combatMove = qtrue;
	NPC_MoveToGoal( qtrue );
	NPC_FaceEnemy();
	//Update our angles regardless
	NPC_UpdateAngles( qtrue, qtrue );

}

/*
-------------------------
DROIDEKA_Ranged
-------------------------
*/
void DROIDEKA_Ranged( qboolean visible, qboolean advance, qboolean altAttack )
{

	if ( TIMER_Done( NPC, "atkDelay" ) && visible )	// Attack?
	{
		TIMER_Set( NPC, "atkDelay", Q_irand( 100, 500 ) );
		ucmd.buttons |= BUTTON_ATTACK;
	}

	if ( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
	{
		DROIDEKA_Hunt( visible, advance );
	}
}

/*
-------------------------
DROIDEKA_Attack
-------------------------
*/
void DROIDEKA_Attack( void )
{
	qboolean	altAttack=qfalse;

	// Rate our distance to the target, and our visibilty
	float		distance	= (int) DistanceHorizontalSquared( NPC->currentOrigin, NPC->enemy->currentOrigin );	
	distance_e	distRate	= ( distance > MIN_MELEE_RANGE_SQR ) ? DIST_LONG : DIST_MELEE;
	qboolean	visible		= NPC_ClearLOS( NPC->enemy );
	qboolean	advance		= (qboolean)(distance > MIN_DISTANCE_SQR);
	

	if ( NPC_CheckEnemyExt() == qfalse )
	{
		NPC->enemy = NULL;
		DROIDEKA_Hunt( visible, advance );
		return;
	}

	// If we cannot see our target, move to see it
	if ( visible == qfalse )
	{
		if ( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
		{
			DROIDEKA_Hunt( visible, advance );
			return;
		}
	}

	//If we're too far away, then keep walking forward
	if ( distRate != DIST_MELEE )
	{
		DROIDEKA_Hunt( visible, advance );
		return;
	}

	NPC_FaceEnemy( qtrue );

	DROIDEKA_Ranged( visible, advance,altAttack );
	
	if ( NPCInfo->touchedByPlayer != NULL && NPCInfo->touchedByPlayer == NPC->enemy )
	{//touched enemy
		if ( NPC->client->ps.powerups[PW_GALAK_SHIELD] > 0 )
		{//zap him!
			//animate me
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			TIMER_Set( NPC, "attackDelay", NPC->client->ps.torsoAnimTimer );
			NPCInfo->touchedByPlayer = NULL;
			NPC->client->ps.powerups[PW_BATTLESUIT] = level.time + SHIELD_EFFECT_TIME;
			vec3_t	smackDir;
			VectorSubtract( NPC->enemy->currentOrigin, NPC->currentOrigin, smackDir );
			smackDir[2] += 30;
			VectorNormalize( smackDir );
			G_Damage( NPC->enemy, NPC, NPC, smackDir, NPC->currentOrigin, (g_spskill->integer+1)*Q_irand( 2, 5), DAMAGE_NO_KNOCKBACK, MOD_ELECTROCUTE );
			G_Throw( NPC->enemy, smackDir, 50 );
			NPC->enemy->s.powerups |= ( 1 << PW_SHOCKED );
			if ( NPC->enemy->client )
			{
				NPC->enemy->client->ps.powerups[PW_SHOCKED] = level.time + 1000;
			}
			//stop any attacks
			ucmd.buttons = 0;
		}
	}
}

/*
-------------------------
DROIDEKA_Patrol
-------------------------
*/
void DROIDEKA_Patrol( void )
{
	if ( NPC_CheckPlayerTeamStealth() )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}
	//If we have somewhere to go, then do that
	if (!NPC->enemy)
	{
		if ( UpdateGoal() )
		{
			ucmd.buttons |= BUTTON_WALKING;
			NPC_MoveToGoal( qtrue );
			NPC_UpdateAngles( qtrue, qtrue );
		}
	}
}

/*
-------------------------
NPC_BSDROIDEKA_Default
-------------------------
*/
void NPC_BSDROIDEKA_Default( void )
{
	
	if ( NPC->client->ps.stats[STAT_ARMOR] <= 0 )
	{//armor gone
		if ( !NPCInfo->investigateDebounceTime )
		{//start regenerating the armor
			gi.G2API_SetSurfaceOnOff( &NPC->ghoul2[NPC->playerModel], "torso_shield_off", TURN_OFF );
			NPC->flags &= ~FL_SHIELDED;//no more reflections
			VectorSet( NPC->mins, -10, -10, -14 );
			VectorSet( NPC->maxs, 10, 10, 34 );
			NPC->client->crouchheight = NPC->client->standheight = 64;
			if ( NPC->locationDamage[HL_GENERIC1] < GENERATOR_HEALTH )
			{//still have the generator bolt-on
				if ( NPCInfo->investigateCount < 12 )
				{
					NPCInfo->investigateCount++;
				}
				NPCInfo->investigateDebounceTime = level.time + (NPCInfo->investigateCount * 5000);
			}
		}
		else if ( NPCInfo->investigateDebounceTime < level.time )
		{//armor regenerated, turn shield back on
			//do a trace and make sure we can turn this back on?
			trace_t	tr;
			gi.trace( &tr, NPC->currentOrigin, shieldMins, shieldMaxs, NPC->currentOrigin, NPC->s.number, NPC->clipmask, G2_NOCOLLIDE, 0 );
			if ( !tr.startsolid )
			{
				VectorCopy( shieldMins, NPC->mins );
				VectorCopy( shieldMaxs, NPC->maxs );
				NPC->client->crouchheight = NPC->client->standheight = shieldMaxs[2];
				NPC->client->ps.stats[STAT_ARMOR] = DROIDEKA_SHIELD_HEALTH;
				NPCInfo->investigateDebounceTime = 0;
				NPC->flags |= FL_SHIELDED;//reflect normal shots
				NPC->fx_time = level.time;
				gi.G2API_SetSurfaceOnOff( &NPC->ghoul2[NPC->playerModel], "torso_shield_off", TURN_ON );
			}
		}
	}
	if ( NPC->client->ps.stats[STAT_ARMOR] > 0 )
	{//armor present
		NPC->client->ps.powerups[PW_GALAK_SHIELD] = Q3_INFINITE;//temp, for effect
		gi.G2API_SetSurfaceOnOff( &NPC->ghoul2[NPC->playerModel], "torso_shield_off", TURN_ON );
	}
	else
	{
		gi.G2API_SetSurfaceOnOff( &NPC->ghoul2[NPC->playerModel], "torso_shield_off", TURN_OFF );
	}

	if( !NPC->enemy )
	{//don't have an enemy, look for one
		DROIDEKA_Patrol();
	}
	else
	{
		if( (NPCInfo->scriptFlags & SCF_CHASE_ENEMIES) )
		{
			NPCInfo->goalEntity = NPC->enemy;
		}
		DROIDEKA_Attack();
	}
}

