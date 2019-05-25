#include "cg_local.h"

extern int BG_SiegeGetPairedValue(char *buf, char *key, char *outbuf);
extern int BG_SiegeGetValueGroup(char *buf, char *group, char *outbuf);

stringID_table_t holsterTypeTable[] =
{
	ENUM2STRING(HLR_NONE),
	ENUM2STRING(HLR_SINGLESABER_1),	//first single saber
	ENUM2STRING(HLR_SINGLESABER_2),	//second single saber
	ENUM2STRING(HLR_STAFFSABER),		//staff saber
	ENUM2STRING(HLR_PISTOL_L),		//left hip blaster pistol
	ENUM2STRING(HLR_PISTOL_R),		//right hip blaster pistol
	ENUM2STRING(HLR_BLASTER_L),		//left hip blaster rifle
	ENUM2STRING(HLR_BLASTER_R),		//right hip blaster rifle
	ENUM2STRING(HLR_BRYARPISTOL_L),	//left hip bryer pistol
	ENUM2STRING(HLR_BRYARPISTOL_R),	//right hip bryer pistol
	ENUM2STRING(HLR_BOWCASTER),		//bowcaster
	ENUM2STRING(HLR_ROCKET_LAUNCHER),//rocket launcher
	ENUM2STRING(HLR_DEMP2),			//demp2
	ENUM2STRING(HLR_CONCUSSION),		//concussion
	ENUM2STRING(HLR_REPEATER),		//repeater
	ENUM2STRING(HLR_FLECHETTE),		//flechette
	ENUM2STRING(HLR_DISRUPTOR),		//disruptor
	ENUM2STRING(MAX_HOLSTER)	
};

stringID_table_t holsterBoneTable[] =
{
	ENUM2STRING(HOLSTER_NONE),
	ENUM2STRING(HOLSTER_UPPERBACK),	
	ENUM2STRING(HOLSTER_LOWERBACK),
	ENUM2STRING(HOLSTER_LEFTHIP),
	ENUM2STRING(HOLSTER_RIGHTHIP)
};

void InitHolsterData (clientInfo_t *ci)
{//initialize holster data with the premade defaults.
	int i;
	for( i = 0; i < MAX_HOLSTER; i++ )
	{
		ci->holsterData[i].boneIndex = HOLSTER_NONE;
		VectorCopy(vec3_origin, ci->holsterData[i].posOffset);
		VectorCopy(vec3_origin, ci->holsterData[i].angOffset);
	}
}

extern char *BG_GetNextValueGroup(char *inbuf, char *outbuf);
void CG_LoadHolsterData (clientInfo_t *ci)
{//adjusts the manual holster positional data based on the holster.cfg file associated with the model or simply
	//use the default values

	int				i;
	fileHandle_t	f;
	int				fLen = 0;
	char			fileBuffer[MAX_HOLSTER_INFO_SIZE];
	char			holsterTypeValue[MAX_QPATH];
	char			holsterTypeGroup[MAX_HOLSTER_INFO_SIZE];
	char			*s;
	vec3_t			vectorData;

	InitHolsterData(ci);

	if ( !ci->skinName || !Q_stricmp( "default", ci->skinName ) )
	{//try default holster.cfg first
		fLen = trap->FS_Open(va("models/players/%s/holster.cfg", ci->modelName), &f, FS_READ);

		if( !f )
		{//no file, use kyle's then.
			fLen = trap->FS_Open("models/players/kyle/holster.cfg", &f, FS_READ);
		}
	}
	else
	{//use the holster.cfg associated with this skin
		fLen = trap->FS_Open(va("models/players/%s/holster_%s.cfg", ci->modelName, ci->skinName), &f, FS_READ);
		if ( !f )
		{//fall back to default holster.cfg
			fLen = trap->FS_Open(va("models/players/%s/holster.cfg", ci->modelName), &f, FS_READ);
		}

		if( !f )
		{//still no dice, use kyle's then.
			fLen = trap->FS_Open("models/players/kyle/holster.cfg", &f, FS_READ);
		}
	}

	if ( !f || !fLen )
	{//couldn't open file or it was empty, just use the defaults
		return;
	}

	if( fLen >= MAX_HOLSTER_INFO_SIZE )
	{
		trap->FS_Close( f );
		return;
	}

	trap->FS_Read(fileBuffer, fLen, f);

	trap->FS_Close( f );

	s = fileBuffer;

	//parse file
	while( (s = BG_GetNextValueGroup(s, holsterTypeGroup)) != NULL )
	{
		if( !BG_SiegeGetPairedValue(holsterTypeGroup, "holsterType", holsterTypeValue) )
		{//couldn't find holster type in group
			continue;
		}

		i = GetIDForString(holsterTypeTable, holsterTypeValue);

		if( i == -1 )
		{//bad holster type
			continue;
		}

		if( BG_SiegeGetPairedValue(holsterTypeGroup, "boneIndex", holsterTypeValue) )
		{//have bone index data for this holster type, use it
			if(!Q_stricmp(holsterTypeValue, "disabled") )
			{//disable the rendering of this holster type on this model
				ci->holsterData[i].boneIndex = HOLSTER_NONE;
			}
			else
			{
				ci->holsterData[i].boneIndex = GetIDForString(holsterBoneTable, holsterTypeValue);
			}
		}

		if( BG_SiegeGetPairedValue(holsterTypeGroup, "posOffset", holsterTypeValue) )
		{//parsing positional offset data
			sscanf (holsterTypeValue, "%f, %f, %f", &vectorData[0], &vectorData[1], &vectorData[2]);
			VectorCopy(vectorData, ci->holsterData[i].posOffset);
		}

		if( BG_SiegeGetPairedValue(holsterTypeGroup, "angOffset", holsterTypeValue) )
		{//parsing angular offset
			sscanf (holsterTypeValue, "%f, %f, %f", &vectorData[0], &vectorData[1], &vectorData[2]);
			VectorCopy(vectorData, ci->holsterData[i].angOffset);
		}
	}
#ifdef _DEBUG
	Com_Printf("Holstered Weapon Data Loaded for %s.\n", ci->modelName);
#endif
}

