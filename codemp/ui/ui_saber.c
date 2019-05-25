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

/*
=======================================================================

USER INTERFACE SABER LOADING & DISPLAY CODE

=======================================================================
*/

#include "ui_local.h"
#include "ui_shared.h"

void WP_SaberLoadParms( void );
qboolean WP_SaberParseParm( const char *saberName, const char *parmname, char *saberData );
saber_colors_t TranslateSaberColor( const char *name );
const char *SaberColorToString( saber_colors_t color );
saber_styles_t TranslateSaberStyle( const char *name );
saberType_t TranslateSaberType( const char *name );

qboolean	ui_saber_parms_parsed = qfalse;

static qhandle_t redSaberGlowShader;
static qhandle_t redSaberCoreShader;
static qhandle_t orangeSaberGlowShader;
static qhandle_t orangeSaberCoreShader;
static qhandle_t yellowSaberGlowShader;
static qhandle_t yellowSaberCoreShader;
static qhandle_t greenSaberGlowShader;
static qhandle_t greenSaberCoreShader;
static qhandle_t blueSaberGlowShader;
static qhandle_t blueSaberCoreShader;
static qhandle_t purpleSaberGlowShader;
static qhandle_t purpleSaberCoreShader;
static qhandle_t rgbSaberCoreShader;
static qhandle_t rgbSaberGlowShader;
static qhandle_t rgbSaberCore2Shader;
static qhandle_t blackSaberGlowShader;
static qhandle_t blackSaberCoreShader;
static qhandle_t limeSaberGlowShader;
static qhandle_t limeSaberCoreShader;
static qhandle_t rgbTFASaberCoreShader;

//Episode I Sabers
static qhandle_t ep1SaberCoreShader;
static qhandle_t redEp1GlowShader;
static qhandle_t orangeEp1GlowShader;
static qhandle_t yellowEp1GlowShader;
static qhandle_t greenEp1GlowShader;
static qhandle_t blueEp1GlowShader;
static qhandle_t purpleEp1GlowShader;

//Episode II Sabers
static qhandle_t ep2SaberCoreShader;
static qhandle_t whiteIgniteFlare;
static qhandle_t blackIgniteFlare;
static qhandle_t redEp2GlowShader;
static qhandle_t orangeEp2GlowShader;
static qhandle_t yellowEp2GlowShader;
static qhandle_t greenEp2GlowShader;
static qhandle_t blueEp2GlowShader;
static qhandle_t purpleEp2GlowShader;

//Episode III Sabers
static qhandle_t ep3SaberCoreShader;
static qhandle_t whiteIgniteFlare02;
static qhandle_t blackIgniteFlare02;
static qhandle_t redEp3GlowShader;
static qhandle_t orangeEp3GlowShader;
static qhandle_t yellowEp3GlowShader;
static qhandle_t greenEp3GlowShader;
static qhandle_t blueEp3GlowShader;
static qhandle_t purpleEp3GlowShader;

//Original Trilogy Sabers
static qhandle_t otSaberCoreShader;
static qhandle_t redOTGlowShader;
static qhandle_t orangeOTGlowShader;
static qhandle_t yellowOTGlowShader;
static qhandle_t greenOTGlowShader;
static qhandle_t blueOTGlowShader;
static qhandle_t purpleOTGlowShader;

void UI_CacheSaberGlowGraphics( void )
{//FIXME: these get fucked by vid_restarts
	redSaberGlowShader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/red_glow" );
	redSaberCoreShader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/red_line" );
	orangeSaberGlowShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/orange_glow" );
	orangeSaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/orange_line" );
	yellowSaberGlowShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/yellow_glow" );
	yellowSaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/yellow_line" );
	greenSaberGlowShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/green_glow" );
	greenSaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/green_line" );
	blueSaberGlowShader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/blue_glow" );
	blueSaberCoreShader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/blue_line" );
	purpleSaberGlowShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/purple_glow" );
	purpleSaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/purple_line" );
	rgbSaberGlowShader          = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/rgb_glow");
	rgbSaberCoreShader          = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/rgb_line");
	rgbSaberCore2Shader         = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/rgb_core");
	blackSaberGlowShader        = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/black_glow");
	blackSaberCoreShader        = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/black_blade");
	limeSaberGlowShader	    	= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/lime_glow" );
	limeSaberCoreShader		    = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/lime_line" );
	rgbTFASaberCoreShader       = trap->R_RegisterShaderNoMip( "gfx/effects/TFASabers/blade_TFA");


	//Episode I Sabers
	ep1SaberCoreShader          = trap->R_RegisterShaderNoMip( "gfx/effects/Ep1Sabers/saber_core" );
	redEp1GlowShader            = trap->R_RegisterShaderNoMip( "gfx/effects/Ep1Sabers/red_glowa" );
	orangeEp1GlowShader         = trap->R_RegisterShaderNoMip( "gfx/effects/Ep1Sabers/orange_glowa" );
	yellowEp1GlowShader         = trap->R_RegisterShaderNoMip( "gfx/effects/Ep1Sabers/yellow_glowa" );
	greenEp1GlowShader          = trap->R_RegisterShaderNoMip( "gfx/effects/Ep1Sabers/green_glowa" );
	blueEp1GlowShader           = trap->R_RegisterShaderNoMip( "gfx/effects/Ep1Sabers/blue_glowa" );
	purpleEp1GlowShader         = trap->R_RegisterShaderNoMip( "gfx/effects/Ep1Sabers/purple_glowa" );
	//Episode II Sabers
	ep2SaberCoreShader          = trap->R_RegisterShaderNoMip( "gfx/effects/Ep2Sabers/saber_core" );
	redEp2GlowShader            = trap->R_RegisterShaderNoMip( "gfx/effects/Ep2Sabers/red_glowa" );
	orangeEp2GlowShader         = trap->R_RegisterShaderNoMip( "gfx/effects/Ep2Sabers/orange_glowa" );
	yellowEp2GlowShader         = trap->R_RegisterShaderNoMip( "gfx/effects/Ep2Sabers/yellow_glowa" );
	greenEp2GlowShader          = trap->R_RegisterShaderNoMip( "gfx/effects/Ep2Sabers/green_glowa" );
	blueEp2GlowShader           = trap->R_RegisterShaderNoMip( "gfx/effects/Ep2Sabers/blue_glowa" );
	purpleEp2GlowShader         = trap->R_RegisterShaderNoMip( "gfx/effects/Ep2Sabers/purple_glowa" );
	//Episode III Sabers
	ep3SaberCoreShader          = trap->R_RegisterShaderNoMip( "gfx/effects/Ep3Sabers/saber_core" );
	whiteIgniteFlare02          = trap->R_RegisterShaderNoMip( "gfx/effects/Ep3Sabers/white_ignite_flare" );
	blackIgniteFlare02          = trap->R_RegisterShaderNoMip( "gfx/effects/Ep3Sabers/black_ignite_flare" );
	redEp3GlowShader            = trap->R_RegisterShaderNoMip( "gfx/effects/Ep3Sabers/red_glowa" );
	orangeEp3GlowShader         = trap->R_RegisterShaderNoMip( "gfx/effects/Ep3Sabers/orange_glowa" );
	yellowEp3GlowShader         = trap->R_RegisterShaderNoMip( "gfx/effects/Ep3Sabers/yellow_glowa" );
	greenEp3GlowShader          = trap->R_RegisterShaderNoMip( "gfx/effects/Ep3Sabers/green_glowa" );
	blueEp3GlowShader           = trap->R_RegisterShaderNoMip( "gfx/effects/Ep3Sabers/blue_glowa" );
	purpleEp3GlowShader         = trap->R_RegisterShaderNoMip( "gfx/effects/Ep3Sabers/purple_glowa" );
	//Original Trilogy Sabers
	otSaberCoreShader           = trap->R_RegisterShaderNoMip( "gfx/effects/OTsabers/ot_saberCore" );
	redOTGlowShader             = trap->R_RegisterShaderNoMip( "gfx/effects/OTsabers/ot_redGlow" );
	orangeOTGlowShader          = trap->R_RegisterShaderNoMip( "gfx/effects/OTsabers/ot_orangeGlow" );
	yellowOTGlowShader          = trap->R_RegisterShaderNoMip( "gfx/effects/OTsabers/ot_yellowGlow" );
	greenOTGlowShader           = trap->R_RegisterShaderNoMip( "gfx/effects/OTsabers/ot_greenGlow" );
	blueOTGlowShader            = trap->R_RegisterShaderNoMip( "gfx/effects/OTsabers/ot_blueGlow" );
	purpleOTGlowShader          = trap->R_RegisterShaderNoMip( "gfx/effects/OTsabers/ot_purpleGlow" );
}

qboolean UI_SaberModelForSaber( const char *saberName, char *saberModel )
{
	return WP_SaberParseParm( saberName, "saberModel", saberModel );
}

qboolean UI_SaberSkinForSaber( const char *saberName, char *saberSkin )
{
	return WP_SaberParseParm( saberName, "customSkin", saberSkin );
}

qboolean UI_SaberTypeForSaber( const char *saberName, char *saberType )
{
	return WP_SaberParseParm( saberName, "saberType", saberType );
}

int UI_SaberNumBladesForSaber( const char *saberName )
{
	int numBlades;
	char	numBladesString[8]={0};
	WP_SaberParseParm( saberName, "numBlades", numBladesString );
	numBlades = atoi( numBladesString );
	if ( numBlades < 1 )
	{
		numBlades = 1;
	}
	else if ( numBlades > 8 )
	{
		numBlades = 8;
	}
	return numBlades;
}

qboolean UI_SaberShouldDrawBlade( const char *saberName, int bladeNum )
{
	int bladeStyle2Start = 0, noBlade = 0;
	char	bladeStyle2StartString[8]={0};
	char	noBladeString[8]={0};
	WP_SaberParseParm( saberName, "bladeStyle2Start", bladeStyle2StartString );
	if ( bladeStyle2StartString[0] )
	{
		bladeStyle2Start = atoi( bladeStyle2StartString );
	}
	if ( bladeStyle2Start
		&& bladeNum >= bladeStyle2Start )
	{//use second blade style
		WP_SaberParseParm( saberName, "noBlade2", noBladeString );
		if ( noBladeString[0] )
		{
			noBlade = atoi( noBladeString );
		}
	}
	else
	{//use first blade style
		WP_SaberParseParm( saberName, "noBlade", noBladeString );
		if ( noBladeString[0] )
		{
			noBlade = atoi( noBladeString );
		}
	}
	return ((qboolean)(noBlade==0));
}

qboolean UI_IsSaberTwoHanded( const char *saberName )
{
	int twoHanded;
	char	twoHandedString[8]={0};
	WP_SaberParseParm( saberName, "twoHanded", twoHandedString );
	if ( !twoHandedString[0] )
	{//not defined defaults to "no"
		return qfalse;
	}
	twoHanded = atoi( twoHandedString );
	return ((qboolean)(twoHanded!=0));
}

float UI_SaberBladeLengthForSaber( const char *saberName, int bladeNum )
{
	char	lengthString[8]={0};
	float	length = 40.0f;
	WP_SaberParseParm( saberName, "saberLength", lengthString );
	if ( lengthString[0] )
	{
		length = atof( lengthString );
		if ( length < 0.0f )
		{
			length = 0.0f;
		}
	}

	WP_SaberParseParm( saberName, va("saberLength%d", bladeNum+1), lengthString );
	if ( lengthString[0] )
	{
		length = atof( lengthString );
		if ( length < 0.0f )
		{
			length = 0.0f;
		}
	}

	return length;
}

float UI_SaberBladeRadiusForSaber( const char *saberName, int bladeNum )
{
	char	radiusString[8]={0};
	float	radius = 3.0f;
	WP_SaberParseParm( saberName, "saberRadius", radiusString );
	if ( radiusString[0] )
	{
		radius = atof( radiusString );
		if ( radius < 0.0f )
		{
			radius = 0.0f;
		}
	}

	WP_SaberParseParm( saberName, va("saberRadius%d", bladeNum+1), radiusString );
	if ( radiusString[0] )
	{
		radius = atof( radiusString );
		if ( radius < 0.0f )
		{
			radius = 0.0f;
		}
	}

	return radius;
}

qboolean UI_SaberProperNameForSaber( const char *saberName, char *saberProperName )
{
	char	stringedSaberName[1024];
	qboolean ret = WP_SaberParseParm( saberName, "name", stringedSaberName );
	// if it's a stringed reference translate it
	if( ret && stringedSaberName[0] == '@')
	{
		trap->SE_GetStringTextString(&stringedSaberName[1], saberProperName, 1024);
	}
	else
	{
		// no stringed so just use it as it
		strcpy( saberProperName, stringedSaberName );
	}

	return ret;
}

qboolean UI_SaberValidForPlayerInMP( const char *saberName )
{
	char allowed [8]={0};
	if ( !WP_SaberParseParm( saberName, "notInMP", allowed ) )
	{//not defined, default is yes
		return qtrue;
	}
	if ( !allowed[0] )
	{//not defined, default is yes
		return qtrue;
	}
	else
	{//return value
		return ((qboolean)(atoi(allowed)==0));
	}
}

void UI_SaberLoadParms( void )
{
	ui_saber_parms_parsed = qtrue;
	UI_CacheSaberGlowGraphics();

	WP_SaberLoadParms();
}

void RGB_LerpColor(vec3_t from, vec3_t to, float frac, vec3_t out)
{
	vec3_t diff;
	int i;

	VectorSubtract(to, from, diff);

	VectorCopy(from, out);

	for (i = 0; i<3; i++)
	{
		out[i] += diff[i] * frac;
	}

}

int getint(char **buf)
{
	double temp;
	temp = strtod(*buf, buf);
	return (int)temp;
}

void ParseRGBSaber(char * str, vec3_t c)
{
	char *p = str;
	int i;

	for (i = 0; i<3; i++)
	{
		c[i] = getint(&p);
		p++;
	}
}

vec3_t  ScriptedColors[10][2] = { 0 };
int		ScriptedTimes[10][2]  = { 0 };
int		ScriptedNum[2]        = { 0 };
int		ScriptedActualNum[2]  = { 0 };
int		ScriptedStartTime[2]  = { 0 };
int		ScriptedEndTime[2]    = { 0 };

void UI_ParseScriptedSaber(char *script, int snum)
{
	int n = 0, l;
	char *p = script;

	l = strlen(p);
	p++; 

	while (p[0] && p - script < l && n<10)
	{
		ParseRGBSaber(p, ScriptedColors[n][snum]);
		while (p[0] != ':')
			p++;
		p++;        

		ScriptedTimes[n][snum] = getint(&p);

		p++;
		n++;
	}
	ScriptedNum[snum] = n;
}


void RGB_AdjustSciptedSaberColor(vec3_t color, int n)
{
	int actual;
	float frac;
	int time = uiInfo.uiDC.realTime, i;

	if (!ScriptedStartTime[n])
	{
		ScriptedActualNum[n] = 0;
		ScriptedStartTime[n] = time;
		ScriptedEndTime[n] = time + ScriptedTimes[0][n];
	}
	else if (ScriptedEndTime[n] < time)
	{
		ScriptedActualNum[n] = (ScriptedActualNum[n] + 1) % ScriptedNum[n];
		actual = ScriptedActualNum[n];
		ScriptedStartTime[n] = time;
		ScriptedEndTime[n] = time + ScriptedTimes[actual][n];
	}

	actual = ScriptedActualNum[n];

	frac = (float)(time - ScriptedStartTime[n]) / (float)(ScriptedEndTime[n] - ScriptedStartTime[n]);


	if (actual + 1 != ScriptedNum[n])
		RGB_LerpColor(ScriptedColors[actual][n], ScriptedColors[actual + 1][n], frac, color);
	else
		RGB_LerpColor(ScriptedColors[actual][n], ScriptedColors[0][n], frac, color);

	for (i = 0; i<3; i++)
		color[i] /= 255;

}

#define PIMP_MIN_INTESITY 120

void RGB_RandomRGB(vec3_t c)
{
	int i;
	for (i = 0; i<3; i++)
		c[i] = 0;

	while (c[0] + c[1] + c[2] < PIMP_MIN_INTESITY)
		for (i = 0; i<3; i++)
			c[i] = rand() % 255;
}

int PimpStartTime[2];
int PimpEndTime[2];
vec3_t PimpColorFrom[2];
vec3_t PimpColorTo[2];

void RGB_AdjustPimpSaberColor(vec3_t color, int n)
{
	int time, i;
	float frac;

	if (!PimpStartTime[n])
	{
		PimpStartTime[n] = uiInfo.uiDC.realTime;
		RGB_RandomRGB(PimpColorFrom[n]);
		RGB_RandomRGB(PimpColorTo[n]);
		time = 250 + rand() % 250;
		PimpEndTime[n] = uiInfo.uiDC.realTime + time;
	}
	else if (PimpEndTime[n] < uiInfo.uiDC.realTime)
	{
		VectorCopy(PimpColorTo[n], PimpColorFrom[n]);
		RGB_RandomRGB(PimpColorTo[n]);
		time = 250 + rand() % 250;
		PimpStartTime[n] = uiInfo.uiDC.realTime;
		PimpEndTime[n] = uiInfo.uiDC.realTime + time;
	}

	frac = (float)(uiInfo.uiDC.realTime - PimpStartTime[n]) / (float)(PimpEndTime[n] - PimpStartTime[n]);

	RGB_LerpColor(PimpColorFrom[n], PimpColorTo[n], frac, color);

	for (i = 0; i<3; i++)
		color[i] /= 255;
}

void UI_DoSaber(vec3_t origin, vec3_t dir, float length, float lengthMax, float radius, saber_colors_t color, int snum)
{
	vec3_t		mid, rgb = { 1,1,1 };
	qhandle_t	blade = 0, glow = 0;
	refEntity_t saber;
	float radiusmult;
	float radiusRange;
	float radiusStart;
	int i;


	if (length < 0.5f)
	{
		// if the thing is so short, just forget even adding me.
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA(origin, length * 0.5f, dir, mid);

	switch (color)
	{
	case SABER_RED:
		glow = redSaberGlowShader;
		blade = redSaberCoreShader;
		VectorSet(rgb, 1.0f, 0.2f, 0.2f);
		break;
	case SABER_ORANGE:
		glow = orangeSaberGlowShader;
		blade = orangeSaberCoreShader;
		VectorSet(rgb, 1.0f, 0.5f, 0.1f);
		break;
	case SABER_YELLOW:
		glow = yellowSaberGlowShader;
		blade = yellowSaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 0.2f);
		break;
	case SABER_GREEN:
		glow = greenSaberGlowShader;
		blade = greenSaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_BLUE:
		glow = blueSaberGlowShader;
		blade = blueSaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	case SABER_PURPLE:
		glow = purpleSaberGlowShader;
		blade = purpleSaberCoreShader;
		VectorSet(rgb, 0.9f, 0.2f, 1.0f);
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = limeSaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_RGB:
	{
		if (snum == 0)
			VectorSet(rgb, ui_sab1_r.value, ui_sab1_g.value, ui_sab1_b.value);
		else
			VectorSet(rgb, ui_sab2_r.value, ui_sab2_g.value, ui_sab2_b.value);

		for (i = 0; i<3; i++)
			rgb[i] /= 255;

		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
	}
	break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = blackSaberCoreShader;
		VectorSet(rgb, .0f, .0f, .0f);
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		RGB_AdjustSciptedSaberColor(rgb, snum);
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		RGB_AdjustSciptedSaberColor(rgb, snum);
		break;
	default:
		glow = blueSaberGlowShader;
		blade = blueSaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	}

	memset(&saber, 0, sizeof(refEntity_t));

	saber.saberLength = length;

	if (length < lengthMax)
	{
		radiusmult = 1.0 + (2.0 / length);		// Note this creates a curve, and length cannot be < 0.5.
	}
	else
	{
		radiusmult = 1.0;
	}

	radiusRange = radius * 0.075f;
	radiusStart = radius - radiusRange;

	saber.radius = (radiusStart + Q_flrand(-1.0f, 1.0f) * radiusRange)*radiusmult;

	VectorCopy(origin, saber.origin);
	VectorCopy(dir, saber.axis[0]);
	saber.reType = RT_SABER_GLOW;
	saber.customShader = glow;
	saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

	if (color != SABER_RGB)
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
	else
	{
		int i;
		for (i = 0; i<3; i++)
			saber.shaderRGBA[i] = rgb[i] * 255;
		saber.shaderRGBA[3] = 255;
	}


	trap->R_AddRefEntityToScene(&saber);

	// Do the hot core
	VectorMA(origin, length, dir, saber.origin);
	VectorMA(origin, -1, dir, saber.oldorigin);
	saber.customShader = blade;
	saber.reType = RT_LINE;
	radiusStart = radius / 3.0f;
	saber.radius = (radiusStart + Q_flrand(-1.0f, 1.0f) * radiusRange)*radiusmult;

	trap->R_AddRefEntityToScene(&saber);

	if (color != SABER_RGB)
		return;

	saber.customShader = rgbSaberCoreShader;
	saber.reType = RT_LINE;
	saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
	saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
	saber.radius = (radiusStart + Q_flrand(-1.0f, 1.0f) * radiusRange)*radiusmult;
	trap->R_AddRefEntityToScene(&saber);

}


void UI_DobattlefrontSaber(vec3_t origin, vec3_t dir, float length, float lengthMax, float radius, saber_colors_t color, int snum)
{
	vec3_t	    mid, rgb = { 1,1,1 };
	qhandle_t	glow = 0, blade = 0;
	refEntity_t saber;
	float	    radiusmult, effectradius, coreradius;
	float	   effectalpha = 0.8f;
	float	   AngleScale = 1.0f;
	int i;

	if (length < 0.5f)
	{
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA(origin, length * 0.5f, dir, mid);

	switch (color)
	{
	case SABER_RED:
		glow = redSaberGlowShader;
		blade = redSaberCoreShader;
		VectorSet(rgb, 1.0f, 0.2f, 0.2f);
		break;
	case SABER_ORANGE:
		glow = orangeSaberGlowShader;
		blade = orangeSaberCoreShader;
		VectorSet(rgb, 1.0f, 0.5f, 0.1f);
		break;
	case SABER_YELLOW:
		glow = yellowSaberGlowShader;
		blade = yellowSaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 0.2f);
		break;
	case SABER_GREEN:
		glow = greenSaberGlowShader;
		blade = greenSaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_BLUE:
		glow = blueSaberGlowShader;
		blade = blueSaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	case SABER_PURPLE:
		glow = purpleSaberGlowShader;
		blade = purpleSaberCoreShader;
		VectorSet(rgb, 0.9f, 0.2f, 1.0f);
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = limeSaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_RGB:
	{
		if (snum == 0)
			VectorSet(rgb, ui_sab1_r.value, ui_sab1_g.value, ui_sab1_b.value);
		else
			VectorSet(rgb, ui_sab2_r.value, ui_sab2_g.value, ui_sab2_b.value);

		for (i = 0; i<3; i++)
			rgb[i] /= 255;

		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
	}
	break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		RGB_AdjustPimpSaberColor(rgb, snum);
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = blackSaberCoreShader;
		VectorSet(rgb, .0f, .0f, .0f);
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		RGB_AdjustSciptedSaberColor(rgb, snum);
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		RGB_AdjustSciptedSaberColor(rgb, snum);
		break;
	default:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		break;

	}

	memset(&saber, 0, sizeof(refEntity_t));

	// Saber glow is it's own ref type because it uses a ton of sprites, otherwise it would eat up too many
	//	refEnts to do each glow blob individually
	saber.saberLength = length;

	if (length < lengthMax)
	{
		radiusmult = 1.0 + (2.0 / length);		// Note this creates a curve, and length cannot be < 0.5.
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = ((radius * 1.6f) + Q_flrand(-1.0f, 1.0f) * 0.1f)*radiusmult*cg_SFXSabersGlowSize.value;
	coreradius = ((radius * 0.4f) + Q_flrand(-1.0f, 1.0f) * 0.1f)*radiusmult*cg_SFXSabersCoreSize.value;

	coreradius *= 0.9f;

	{
		if (length - ((effectradius*AngleScale) / 2) > 0)
		{
			saber.radius = effectradius * AngleScale;
			saber.saberLength = (length - (saber.radius / 2));
			VectorCopy(origin, saber.origin);
			VectorCopy(dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;

			if (color != SABER_RGB)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			else
			{
				int i;
				for (i = 0; i<3; i++)
					saber.shaderRGBA[i] = rgb[i] * 255;
				saber.shaderRGBA[3] = 255;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(origin, length, dir, saber.origin);
		VectorMA(origin, -1, dir, saber.oldorigin);
		saber.customShader = blade;
		saber.reType = RT_LINE;
		saber.radius = coreradius;
		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

		trap->R_AddRefEntityToScene(&saber);
	}
}

void UI_DoSFXSaber(vec3_t origin, vec3_t dir, float length, float lengthMax, float radius, saber_colors_t color, int snum)
{
	vec3_t	    mid, rgb = { 1,1,1 };
	qhandle_t	glow = 0, blade = 0;
	refEntity_t saber;
	float	    radiusmult, effectradius, coreradius;
	float	   effectalpha = 0.8f;
	float	   AngleScale = 1.0f;
	int i;

	if (length < 0.5f)
	{
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA(origin, length * 0.5f, dir, mid);

	switch (color)
	{
	case SABER_RED:
		glow = redSaberGlowShader;
		blade = redSaberCoreShader;
		VectorSet(rgb, 1.0f, 0.2f, 0.2f);
		break;
	case SABER_ORANGE:
		glow = orangeSaberGlowShader;
		blade = orangeSaberCoreShader;
		VectorSet(rgb, 1.0f, 0.5f, 0.1f);
		break;
	case SABER_YELLOW:
		glow = yellowSaberGlowShader;
		blade = yellowSaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 0.2f);
		break;
	case SABER_GREEN:
		glow = greenSaberGlowShader;
		blade = greenSaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_BLUE:
		glow = blueSaberGlowShader;
		blade = blueSaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	case SABER_PURPLE:
		glow = purpleSaberGlowShader;
		blade = purpleSaberCoreShader;
		VectorSet(rgb, 0.9f, 0.2f, 1.0f);
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = limeSaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_RGB:
	{
		if (snum == 0)
			VectorSet(rgb, ui_sab1_r.value, ui_sab1_g.value, ui_sab1_b.value);
		else
			VectorSet(rgb, ui_sab2_r.value, ui_sab2_g.value, ui_sab2_b.value);

		for (i = 0; i<3; i++)
			rgb[i] /= 255;

		glow = rgbSaberGlowShader;
		blade = rgbSaberCore2Shader;
	}
	break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCore2Shader;
		RGB_AdjustPimpSaberColor(rgb, snum);
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCore2Shader;
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = blackSaberCoreShader;
		VectorSet(rgb, .0f, .0f, .0f);
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCore2Shader;
		RGB_AdjustSciptedSaberColor(rgb, snum);
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		break;
	default:
		glow = rgbSaberGlowShader;
		break;

	}

	memset(&saber, 0, sizeof(refEntity_t));

	// Saber glow is it's own ref type because it uses a ton of sprites, otherwise it would eat up too many
	//	refEnts to do each glow blob individually
	saber.saberLength = length;

	if (length < lengthMax)
	{
		radiusmult = 1.0 + (2.0 / length);		// Note this creates a curve, and length cannot be < 0.5.
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = ((radius * 1.6f) + Q_flrand(-1.0f, 1.0f) * 0.1f)*radiusmult*cg_SFXSabersGlowSize.value;
	coreradius = ((radius * 0.4f) + Q_flrand(-1.0f, 1.0f) * 0.1f)*radiusmult*cg_SFXSabersCoreSize.value;

	coreradius *= 0.9f;

	{
		if (length - ((effectradius*AngleScale) / 2) > 0)
		{
			saber.radius = effectradius * AngleScale;
			saber.saberLength = (length - (saber.radius / 2));
			VectorCopy(origin, saber.origin);
			VectorCopy(dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;

			if (color != SABER_RGB)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			else
			{
				int i;
				for (i = 0; i<3; i++)
					saber.shaderRGBA[i] = rgb[i] * 255;
				saber.shaderRGBA[3] = 255;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(origin, length, dir, saber.origin);
		VectorMA(origin, -1, dir, saber.oldorigin);
		saber.customShader = rgbSaberCore2Shader;
		saber.reType = RT_LINE;
		saber.radius = coreradius;
		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

		trap->R_AddRefEntityToScene(&saber);
	}
}

void UI_DoEp1Saber(vec3_t origin, vec3_t dir, float length, float lengthMax, float radius, saber_colors_t color, int snum)
{
	vec3_t	    mid, rgb = { 1,1,1 };
	qhandle_t	glow = 0, blade = 0;
	refEntity_t saber;
	float	    radiusmult, effectradius, coreradius;
	float	   effectalpha = 0.8f;
	float	   AngleScale = 1.0f;
	int i;

	if (length < 0.5f)
	{
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA(origin, length * 0.5f, dir, mid);

	switch (color)
	{
	case SABER_RED:
		glow = redEp1GlowShader;
		blade = ep1SaberCoreShader;
		VectorSet(rgb, 1.0f, 0.2f, 0.2f);
		break;
	case SABER_ORANGE:
		glow = orangeEp1GlowShader;
		blade = ep1SaberCoreShader;
		VectorSet(rgb, 1.0f, 0.5f, 0.1f);
		break;
	case SABER_YELLOW:
		glow = yellowEp1GlowShader;
		blade = ep1SaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 0.2f);
		break;
	case SABER_GREEN:
		glow = greenEp1GlowShader;
		blade = ep1SaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_BLUE:
		glow = blueEp1GlowShader;
		blade = ep1SaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	case SABER_PURPLE:
		glow = purpleEp1GlowShader;
		blade = ep1SaberCoreShader;
		VectorSet(rgb, 0.9f, 0.2f, 1.0f);
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = ep1SaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_RGB:
	{
		if (snum == 0)
			VectorSet(rgb, ui_sab1_r.value, ui_sab1_g.value, ui_sab1_b.value);
		else
			VectorSet(rgb, ui_sab2_r.value, ui_sab2_g.value, ui_sab2_b.value);

		for (i = 0; i < 3; i++)
			rgb[i] /= 255;

		glow = rgbSaberGlowShader;
		blade = ep1SaberCoreShader;
	}
	break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = ep1SaberCoreShader;
		RGB_AdjustPimpSaberColor(rgb, snum);
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = ep1SaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = blackSaberCoreShader;
		VectorSet(rgb, .0f, .0f, .0f);
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = ep1SaberCoreShader;
		RGB_AdjustSciptedSaberColor(rgb, snum);
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = ep1SaberCoreShader;
		break;
	default:
		glow = rgbSaberGlowShader;
		blade = ep1SaberCoreShader;
		break;

	}

	memset(&saber, 0, sizeof(refEntity_t));

	// Saber glow is it's own ref type because it uses a ton of sprites, otherwise it would eat up too many
	//	refEnts to do each glow blob individually
	saber.saberLength = length;

	if (length < lengthMax)
	{
		radiusmult = 1.0 + (2.0 / length);		// Note this creates a curve, and length cannot be < 0.5.
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = ((radius * 1.6f) + Q_flrand(-1.0f, 1.0f) * 0.1f)*radiusmult*cg_SFXSabersGlowSize.value;
	coreradius = ((radius * 0.4f) + Q_flrand(-1.0f, 1.0f) * 0.1f)*radiusmult*cg_SFXSabersCoreSize.value;

	coreradius *= 0.9f;

	{
		if (length - ((effectradius*AngleScale) / 2) > 0)
		{
			saber.radius = effectradius * AngleScale;
			saber.saberLength = (length - (saber.radius / 2));
			VectorCopy(origin, saber.origin);
			VectorCopy(dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;

			if (color != SABER_RGB)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			else
			{
				int i;
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * 255;
				saber.shaderRGBA[3] = 255;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(origin, length, dir, saber.origin);
		VectorMA(origin, -1, dir, saber.oldorigin);
		saber.customShader = ep1SaberCoreShader;
		saber.reType = RT_LINE;
		saber.radius = coreradius;
		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

		trap->R_AddRefEntityToScene(&saber);
	}
}

void UI_DoEp2Saber(vec3_t origin, vec3_t dir, float length, float lengthMax, float radius, saber_colors_t color, int snum)
{
	vec3_t	    mid, rgb = { 1,1,1 };
	qhandle_t	glow = 0, blade = 0;
	refEntity_t saber;
	float	    radiusmult, effectradius, coreradius;
	float	   effectalpha = 0.8f;
	float	   AngleScale = 1.0f;
	int i;

	if (length < 0.5f)
	{
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA(origin, length * 0.5f, dir, mid);

	switch (color)
	{
	case SABER_RED:
		glow = redEp2GlowShader;
		blade = ep2SaberCoreShader;
		VectorSet(rgb, 1.0f, 0.2f, 0.2f);
		break;
	case SABER_ORANGE:
		glow = orangeEp2GlowShader;
		blade = ep2SaberCoreShader;
		VectorSet(rgb, 1.0f, 0.5f, 0.1f);
		break;
	case SABER_YELLOW:
		glow = yellowEp2GlowShader;
		blade = ep2SaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 0.2f);
		break;
	case SABER_GREEN:
		glow = greenEp2GlowShader;
		blade = ep2SaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_BLUE:
		glow = blueEp2GlowShader;
		blade = ep2SaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	case SABER_PURPLE:
		glow = purpleEp2GlowShader;
		blade = ep2SaberCoreShader;
		VectorSet(rgb, 0.9f, 0.2f, 1.0f);
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = ep2SaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_RGB:
	{
		if (snum == 0)
			VectorSet(rgb, ui_sab1_r.value, ui_sab1_g.value, ui_sab1_b.value);
		else
			VectorSet(rgb, ui_sab2_r.value, ui_sab2_g.value, ui_sab2_b.value);

		for (i = 0; i < 3; i++)
			rgb[i] /= 255;

		glow = rgbSaberGlowShader;
		blade = ep2SaberCoreShader;
	}
	break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = ep2SaberCoreShader;
		RGB_AdjustPimpSaberColor(rgb, snum);
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = ep2SaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = ep2SaberCoreShader;
		VectorSet(rgb, .0f, .0f, .0f);
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = ep2SaberCoreShader;
		RGB_AdjustSciptedSaberColor(rgb, snum);
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = ep2SaberCoreShader;
		break;
	default:
		glow = rgbSaberGlowShader;
		blade = ep2SaberCoreShader;
		break;

	}

	memset(&saber, 0, sizeof(refEntity_t));

	// Saber glow is it's own ref type because it uses a ton of sprites, otherwise it would eat up too many
	//	refEnts to do each glow blob individually
	saber.saberLength = length;

	if (length < lengthMax)
	{
		radiusmult = 1.0 + (2.0 / length);		// Note this creates a curve, and length cannot be < 0.5.
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = ((radius * 1.6f) + Q_flrand(-1.0f, 1.0f) * 0.1f)*radiusmult*cg_SFXSabersGlowSize.value;
	coreradius = ((radius * 0.4f) + Q_flrand(-1.0f, 1.0f) * 0.1f)*radiusmult*cg_SFXSabersCoreSize.value;

	coreradius *= 0.9f;

	{
		if (length - ((effectradius*AngleScale) / 2) > 0)
		{
			saber.radius = effectradius * AngleScale;
			saber.saberLength = (length - (saber.radius / 2));
			VectorCopy(origin, saber.origin);
			VectorCopy(dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;

			if (color != SABER_RGB)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			else
			{
				int i;
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * 255;
				saber.shaderRGBA[3] = 255;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(origin, length, dir, saber.origin);
		VectorMA(origin, -1, dir, saber.oldorigin);
		saber.customShader = ep2SaberCoreShader;
		saber.reType = RT_LINE;
		saber.radius = coreradius;
		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

		trap->R_AddRefEntityToScene(&saber);
	}
}

void UI_DoEp3Saber(vec3_t origin, vec3_t dir, float length, float lengthMax, float radius, saber_colors_t color, int snum)
{
	vec3_t	    mid, rgb = { 1,1,1 };
	qhandle_t	glow = 0, blade = 0;
	refEntity_t saber;
	float	    radiusmult, effectradius, coreradius;
	float	   effectalpha = 0.8f;
	float	   AngleScale = 1.0f;
	int i;

	if (length < 0.5f)
	{
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA(origin, length * 0.5f, dir, mid);

	switch (color)
	{
	case SABER_RED:
		glow = redEp3GlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 1.0f, 0.2f, 0.2f);
		break;
	case SABER_ORANGE:
		glow = orangeEp3GlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 1.0f, 0.5f, 0.1f);
		break;
	case SABER_YELLOW:
		glow = yellowEp3GlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 0.2f);
		break;
	case SABER_GREEN:
		glow = greenEp3GlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_BLUE:
		glow = blueEp3GlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	case SABER_PURPLE:
		glow = purpleEp3GlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 0.9f, 0.2f, 1.0f);
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_RGB:
	{
		if (snum == 0)
			VectorSet(rgb, ui_sab1_r.value, ui_sab1_g.value, ui_sab1_b.value);
		else
			VectorSet(rgb, ui_sab2_r.value, ui_sab2_g.value, ui_sab2_b.value);

		for (i = 0; i<3; i++)
			rgb[i] /= 255;

		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
	}
	break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		RGB_AdjustPimpSaberColor(rgb, snum);
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = blackSaberCoreShader;
		VectorSet(rgb, .0f, .0f, .0f);
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		RGB_AdjustSciptedSaberColor(rgb, snum);
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	default:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;

	}

	memset(&saber, 0, sizeof(refEntity_t));

	// Saber glow is it's own ref type because it uses a ton of sprites, otherwise it would eat up too many
	//	refEnts to do each glow blob individually
	saber.saberLength = length;

	if (length < lengthMax)
	{
		radiusmult = 1.0 + (2.0 / length);		// Note this creates a curve, and length cannot be < 0.5.
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = ((radius * 1.6f) + Q_flrand(-1.0f, 1.0f) * 0.1f)*radiusmult*cg_SFXSabersGlowSize.value;
	coreradius = ((radius * 0.4f) + Q_flrand(-1.0f, 1.0f) * 0.1f)*radiusmult*cg_SFXSabersCoreSize.value;

	coreradius *= 0.9f;

	{
		if (length - ((effectradius*AngleScale) / 2) > 0)
		{
			saber.radius = effectradius * AngleScale;
			saber.saberLength = (length - (saber.radius / 2));
			VectorCopy(origin, saber.origin);
			VectorCopy(dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;

			if (color != SABER_RGB)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			else
			{
				int i;
				for (i = 0; i<3; i++)
					saber.shaderRGBA[i] = rgb[i] * 255;
				saber.shaderRGBA[3] = 255;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(origin, length, dir, saber.origin);
		VectorMA(origin, -1, dir, saber.oldorigin);
		saber.customShader = ep3SaberCoreShader;
		saber.reType = RT_LINE;
		saber.radius = coreradius;
		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

		trap->R_AddRefEntityToScene(&saber);
	}
}

void UI_DoOTSaber(vec3_t origin, vec3_t dir, float length, float lengthMax, float radius, saber_colors_t color, int snum)
{
	vec3_t	    mid, rgb = { 1,1,1 };
	qhandle_t	glow = 0, blade = 0;
	refEntity_t saber;
	float	    radiusmult, effectradius, coreradius;
	float	   effectalpha = 0.8f;
	float	   AngleScale = 1.0f;
	int i;

	if (length < 0.5f)
	{
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA(origin, length * 0.5f, dir, mid);

	switch (color)
	{
	case SABER_RED:
		glow = redOTGlowShader;
		blade = otSaberCoreShader;
		VectorSet(rgb, 1.0f, 0.2f, 0.2f);
		break;
	case SABER_ORANGE:
		glow = orangeOTGlowShader;
		blade = otSaberCoreShader;
		VectorSet(rgb, 1.0f, 0.5f, 0.1f);
		break;
	case SABER_YELLOW:
		glow = yellowOTGlowShader;
		blade = otSaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 0.2f);
		break;
	case SABER_GREEN:
		glow = greenOTGlowShader;
		blade = otSaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_BLUE:
		glow = blueOTGlowShader;
		blade = otSaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	case SABER_PURPLE:
		glow = purpleOTGlowShader;
		blade = otSaberCoreShader;
		VectorSet(rgb, 0.9f, 0.2f, 1.0f);
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = otSaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_RGB:
	{
		if (snum == 0)
			VectorSet(rgb, ui_sab1_r.value, ui_sab1_g.value, ui_sab1_b.value);
		else
			VectorSet(rgb, ui_sab2_r.value, ui_sab2_g.value, ui_sab2_b.value);

		for (i = 0; i<3; i++)
			rgb[i] /= 255;

		glow = rgbSaberGlowShader;
		blade = otSaberCoreShader;
	}
	break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = otSaberCoreShader;
		RGB_AdjustPimpSaberColor(rgb, snum);
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = otSaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = otSaberCoreShader;
		VectorSet(rgb, .0f, .0f, .0f);
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = otSaberCoreShader;
		RGB_AdjustSciptedSaberColor(rgb, snum);
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = otSaberCoreShader;
		break;
	default:
		glow = rgbSaberGlowShader;
		blade = otSaberCoreShader;
		break;

	}

	VectorMA(origin, length * 0.5f, dir, mid);

	memset(&saber, 0, sizeof(refEntity_t));

	if (length < lengthMax)
	{
		radiusmult = 0.5 + ((length / lengthMax) / 2);
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = ((radius * 1.6f) + Q_flrand(-1.0f, 1.0f) * 0.1f)*radiusmult*cg_SFXSabersGlowSize.value;
	coreradius = ((radius * 0.4f) + Q_flrand(-1.0f, 1.0f) * 0.1f)*radiusmult*cg_SFXSabersCoreSize.value;

	coreradius *= 0.5f;

	{
		if (length - ((effectradius*AngleScale) / 2) > 0)
		{
			saber.radius = effectradius * AngleScale;
			saber.saberLength = (length - (saber.radius / 2));
			VectorCopy(origin, saber.origin);
			VectorCopy(dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;

			if (color != SABER_RGB)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff * effectalpha;
			else
			{
				int i;
				for (i = 0; i<3; i++)
					saber.shaderRGBA[i] = rgb[i] * 255;
				saber.shaderRGBA[3] = 255;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(origin, length, dir, saber.origin);
		VectorMA(origin, -1, dir, saber.oldorigin);
		saber.customShader = otSaberCoreShader;
		saber.reType = RT_LINE;
		saber.radius = coreradius;
		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

		trap->R_AddRefEntityToScene(&saber);
	}
}

void UI_DoTFASaber(vec3_t origin, vec3_t dir, float length, float lengthMax, float radius, saber_colors_t color, int snum)
{
	vec3_t	    mid, rgb = { 1,1,1 };
	qhandle_t	glow = 0, blade = 0;
	refEntity_t saber;
	float	    radiusmult, effectradius, coreradius;
	float	   effectalpha = 0.8f;
	float	   AngleScale = 1.0f;
	int i;

	if (length < 0.5f)
	{
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA(origin, length * 0.5f, dir, mid);

	switch (color)
	{
	case SABER_RED:
		glow = redSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		VectorSet(rgb, 1.0f, 0.2f, 0.2f);
		break;
	case SABER_ORANGE:
		glow = orangeSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		VectorSet(rgb, 1.0f, 0.5f, 0.1f);
		break;
	case SABER_YELLOW:
		glow = yellowSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 0.2f);
		break;
	case SABER_GREEN:
		glow = greenSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_BLUE:
		glow = blueSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	case SABER_PURPLE:
		glow = purpleSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		VectorSet(rgb, 0.9f, 0.2f, 1.0f);
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_RGB:
	{
		if (snum == 0)
			VectorSet(rgb, ui_sab1_r.value, ui_sab1_g.value, ui_sab1_b.value);
		else
			VectorSet(rgb, ui_sab2_r.value, ui_sab2_g.value, ui_sab2_b.value);

		for (i = 0; i<3; i++)
			rgb[i] /= 255;

		glow = rgbSaberGlowShader;
		blade = rgbTFASaberCoreShader;
	}
	break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		RGB_AdjustPimpSaberColor(rgb, snum);
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		VectorSet(rgb, .0f, .0f, .0f);
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		RGB_AdjustSciptedSaberColor(rgb, snum);
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		break;
	default:
		glow = rgbSaberGlowShader;
		blade = rgbTFASaberCoreShader;
		break;

	}

	memset(&saber, 0, sizeof(refEntity_t));

	// Saber glow is it's own ref type because it uses a ton of sprites, otherwise it would eat up too many
	//	refEnts to do each glow blob individually
	saber.saberLength = length;

	if (length < lengthMax)
	{
		radiusmult = 1.0 + (2.0 / length);		// Note this creates a curve, and length cannot be < 0.5.
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = ((radius * 1.6f) + Q_flrand(-1.0f, 1.0f) * 0.1f)*radiusmult*cg_SFXSabersGlowSize.value;
	coreradius = ((radius * 0.4f) + Q_flrand(-1.0f, 1.0f) * 0.1f)*radiusmult*cg_SFXSabersCoreSize.value;

	coreradius *= 0.9f;

	{
		if (length - ((effectradius*AngleScale) / 2) > 0)
		{
			saber.radius = effectradius * AngleScale;
			saber.saberLength = (length - (saber.radius / 2));
			VectorCopy(origin, saber.origin);
			VectorCopy(dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;

			if (color != SABER_RGB)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			else
			{
				int i;
				for (i = 0; i<3; i++)
					saber.shaderRGBA[i] = rgb[i] * 255;
				saber.shaderRGBA[3] = 255;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(origin, length, dir, saber.origin);
		VectorMA(origin, -1, dir, saber.oldorigin);
		saber.customShader = rgbTFASaberCoreShader;
		saber.reType = RT_LINE;
		saber.radius = coreradius;
		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

		trap->R_AddRefEntityToScene(&saber);
	}
}

void UI_DoCustomSaber(vec3_t origin, vec3_t dir, float length, float lengthMax, float radius, saber_colors_t color, int snum)
{
	vec3_t	    mid, rgb = { 1,1,1 };
	qhandle_t	glow = 0, blade = 0;
	refEntity_t saber;
	float	    radiusmult, effectradius, coreradius;
	float	   effectalpha = 0.8f;
	float	   AngleScale = 1.0f;
	int i;

	if (length < 0.5f)
	{
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA(origin, length * 0.5f, dir, mid);

	switch (color)
	{
	case SABER_RED:
		glow = redSaberGlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 1.0f, 0.2f, 0.2f);
		break;
	case SABER_ORANGE:
		glow = orangeSaberGlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 1.0f, 0.5f, 0.1f);
		break;
	case SABER_YELLOW:
		glow = yellowSaberGlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 0.2f);
		break;
	case SABER_GREEN:
		glow = greenSaberGlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_BLUE:
		glow = blueSaberGlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 0.2f, 0.4f, 1.0f);
		break;
	case SABER_PURPLE:
		glow = purpleSaberGlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 0.9f, 0.2f, 1.0f);
		break;
	case SABER_LIME:
		glow = limeSaberGlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 0.2f, 1.0f, 0.2f);
		break;
	case SABER_RGB:
	{
		if (snum == 0)
			VectorSet(rgb, ui_sab1_r.value, ui_sab1_g.value, ui_sab1_b.value);
		else
			VectorSet(rgb, ui_sab2_r.value, ui_sab2_g.value, ui_sab2_b.value);

		for (i = 0; i < 3; i++)
			rgb[i] /= 255;

		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
	}
	break;
	case SABER_PIMP:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		RGB_AdjustPimpSaberColor(rgb, snum);
		break;
	case SABER_WHITE:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		VectorSet(rgb, 1.0f, 1.0f, 1.0f);
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = blackSaberCoreShader;
		VectorSet(rgb, .0f, .0f, .0f);
		break;
	case SABER_SCRIPTED:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		RGB_AdjustSciptedSaberColor(rgb, snum);
		break;
	case SABER_CUSTOM:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;
	default:
		glow = rgbSaberGlowShader;
		blade = ep3SaberCoreShader;
		break;

	}

	memset(&saber, 0, sizeof(refEntity_t));

	// Saber glow is it's own ref type because it uses a ton of sprites, otherwise it would eat up too many
	//	refEnts to do each glow blob individually
	saber.saberLength = length;

	if (length < lengthMax)
	{
		radiusmult = 1.0 + (2.0 / length);		// Note this creates a curve, and length cannot be < 0.5.
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius = ((radius * 1.6f) + Q_flrand(-1.0f, 1.0f) * 0.1f)*radiusmult*cg_SFXSabersGlowSize.value;
	coreradius = ((radius * 0.4f) + Q_flrand(-1.0f, 1.0f) * 0.1f)*radiusmult*cg_SFXSabersCoreSize.value;

	coreradius *= 0.9f;

	{
		if (length - ((effectradius*AngleScale) / 2) > 0)
		{
			saber.radius = effectradius * AngleScale;
			saber.saberLength = (length - (saber.radius / 2));
			VectorCopy(origin, saber.origin);
			VectorCopy(dir, saber.axis[0]);
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;

			if (color != SABER_RGB)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			else
			{
				int i;
				for (i = 0; i < 3; i++)
					saber.shaderRGBA[i] = rgb[i] * 255;
				saber.shaderRGBA[3] = 255;
			}

			trap->R_AddRefEntityToScene(&saber);
		}

		// Do the hot core
		VectorMA(origin, length, dir, saber.origin);
		VectorMA(origin, -1, dir, saber.oldorigin);
		saber.customShader = ep3SaberCoreShader;
		saber.reType = RT_LINE;
		saber.radius = coreradius;
		saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

		trap->R_AddRefEntityToScene(&saber);
	}
}

void UI_SaberDrawBlade( itemDef_t *item, char *saberName, int saberModel, saberType_t saberType, vec3_t origin, vec3_t angles, int bladeNum )
{
	char bladeColorString[MAX_QPATH];
	saber_colors_t bladeColor;
	float bladeLength,bladeRadius;
	vec3_t	bladeOrigin={0};
	matrix3_t	axis;
	mdxaBone_t	boltMatrix;
	qboolean tagHack = qfalse;
	char *tagName;
	int bolt;
	float scale;
	int snum;

    memset (axis, 0, sizeof (axis));

	if ( (item->flags&ITF_ISSABER) && saberModel < 2 )
	{
		snum = 0;
		trap->Cvar_VariableStringBuffer("ui_saber_color", bladeColorString, sizeof(bladeColorString) );
	}
	else
	{
		snum = 1;
		trap->Cvar_VariableStringBuffer("ui_saber2_color", bladeColorString, sizeof(bladeColorString) );
	}

	if ( !trap->G2API_HasGhoul2ModelOnIndex(&(item->ghoul2),saberModel) )
	{//invalid index!
		return;
	}

	bladeColor = TranslateSaberColor( bladeColorString );

	bladeLength = UI_SaberBladeLengthForSaber( saberName, bladeNum );
	bladeRadius = UI_SaberBladeRadiusForSaber( saberName, bladeNum );

	tagName = va( "*blade%d", bladeNum+1 );
	bolt = trap->G2API_AddBolt( item->ghoul2,saberModel, tagName );

	if ( bolt == -1 )
	{
		tagHack = qtrue;
		//hmm, just fall back to the most basic tag (this will also make it work with pre-JKA saber models
		bolt = trap->G2API_AddBolt( item->ghoul2,saberModel, "*flash" );
		if ( bolt == -1 )
		{//no tag_flash either?!!
			bolt = 0;
		}
	}

	trap->G2API_GetBoltMatrix( item->ghoul2, saberModel, bolt, &boltMatrix, angles, origin, uiInfo.uiDC.realTime, NULL, vec3_origin );//NULL was cgs.model_draw

	// work the matrix axis stuff into the original axis and origins used.
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, bladeOrigin);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, axis[0]);//front (was NEGATIVE_Y, but the md3->glm exporter screws up this tag somethin' awful)
																//		...changed this back to NEGATIVE_Y
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, axis[1]);//right ... and changed this to NEGATIVE_X
	BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_Z, axis[2]);//up

	// Where do I get scale from?
	scale = 1.0f;

	if ( tagHack )
	{
		switch ( saberType )
		{
		case SABER_SINGLE:
		case SABER_UNSTABLE:
		case SABER_THIN:
		case SABER_SFX:
		case SABER_CUSTOMSFX:
		case SABER_YODA:
		case SABER_DOOKU:
		case SABER_BACKHAND:
		case SABER_PALP:
		case SABER_ANAKIN:
		case SABER_GRIE:
		case SABER_GRIE4:
		case SABER_OBIWAN:
		case SABER_ASBACKHAND:
		case SABER_WINDU:
		case SABER_VADER:
		case SABER_KENOBI:
		case SABER_REY:
				VectorMA( bladeOrigin, scale, axis[0], bladeOrigin );
			break;
		case SABER_DAGGER:
		case SABER_LANCE:
			break;
		case SABER_STAFF:
		case SABER_STAFF_UNSTABLE:
		case SABER_STAFF_THIN:
		case SABER_STAFF_SFX:
		case SABER_STAFF_MAUL:
		case SABER_ELECTROSTAFF:
			if ( bladeNum == 0 )
			{
				VectorMA( bladeOrigin, 12*scale, axis[0], bladeOrigin );
			}
			if ( bladeNum == 1 )
			{
				VectorScale( axis[0], -1, axis[0] );
				VectorMA( bladeOrigin, 12*scale, axis[0], bladeOrigin );
			}
			break;
		case SABER_BROAD:
			if ( bladeNum == 0 )
			{
				VectorMA( bladeOrigin, -1*scale, axis[1], bladeOrigin );
			}
			else if ( bladeNum == 1 )
			{
				VectorMA( bladeOrigin, 1*scale, axis[1], bladeOrigin );
			}
			break;
		case SABER_PRONG:
			if ( bladeNum == 0 )
			{
				VectorMA( bladeOrigin, -3*scale, axis[1], bladeOrigin );
			}
			else if ( bladeNum == 1 )
			{
				VectorMA( bladeOrigin, 3*scale, axis[1], bladeOrigin );
			}
			break;
		case SABER_ARC:
			VectorSubtract( axis[1], axis[2], axis[1] );
			VectorNormalize( axis[1] );
			switch ( bladeNum )
			{
			case 0:
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				VectorScale( axis[0], 0.75f, axis[0] );
				VectorScale( axis[1], 0.25f, axis[1] );
				VectorAdd( axis[0], axis[1], axis[0] );
				break;
			case 1:
				VectorScale( axis[0], 0.25f, axis[0] );
				VectorScale( axis[1], 0.75f, axis[1] );
				VectorAdd( axis[0], axis[1], axis[0] );
				break;
			case 2:
				VectorMA( bladeOrigin, -8*scale, axis[0], bladeOrigin );
				VectorScale( axis[0], -0.25f, axis[0] );
				VectorScale( axis[1], 0.75f, axis[1] );
				VectorAdd( axis[0], axis[1], axis[0] );
				break;
			case 3:
				VectorMA( bladeOrigin, -16*scale, axis[0], bladeOrigin );
				VectorScale( axis[0], -0.75f, axis[0] );
				VectorScale( axis[1], 0.25f, axis[1] );
				VectorAdd( axis[0], axis[1], axis[0] );
				break;
			}
			break;
		case SABER_SAI:
			if ( bladeNum == 1 )
			{
				VectorMA( bladeOrigin, -3*scale, axis[1], bladeOrigin );
			}
			else if ( bladeNum == 2 )
			{
				VectorMA( bladeOrigin, 3*scale, axis[1], bladeOrigin );
			}
			break;
		case SABER_CLAW:
			switch ( bladeNum )
			{
			case 0:
				VectorMA( bladeOrigin, 2*scale, axis[0], bladeOrigin );
				VectorMA( bladeOrigin, 2*scale, axis[2], bladeOrigin );
				break;
			case 1:
				VectorMA( bladeOrigin, 2*scale, axis[0], bladeOrigin );
				VectorMA( bladeOrigin, 2*scale, axis[2], bladeOrigin );
				VectorMA( bladeOrigin, 2*scale, axis[1], bladeOrigin );
				break;
			case 2:
				VectorMA( bladeOrigin, 2*scale, axis[0], bladeOrigin );
				VectorMA( bladeOrigin, 2*scale, axis[2], bladeOrigin );
				VectorMA( bladeOrigin, -2*scale, axis[1], bladeOrigin );
				break;
			}
			break;
		case SABER_STAR:
			switch ( bladeNum )
			{
			case 0:
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 1:
				VectorScale( axis[0], 0.33f, axis[0] );
				VectorScale( axis[2], 0.67f, axis[2] );
				VectorAdd( axis[0], axis[2], axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 2:
				VectorScale( axis[0], -0.33f, axis[0] );
				VectorScale( axis[2], 0.67f, axis[2] );
				VectorAdd( axis[0], axis[2], axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 3:
				VectorScale( axis[0], -1, axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 4:
				VectorScale( axis[0], -0.33f, axis[0] );
				VectorScale( axis[2], -0.67f, axis[2] );
				VectorAdd( axis[0], axis[2], axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 5:
				VectorScale( axis[0], 0.33f, axis[0] );
				VectorScale( axis[2], -0.67f, axis[2] );
				VectorAdd( axis[0], axis[2], axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			}
			break;
		case SABER_TRIDENT:
			switch ( bladeNum )
			{
			case 0:
				VectorMA( bladeOrigin, 24*scale, axis[0], bladeOrigin );
				break;
			case 1:
				VectorMA( bladeOrigin, -6*scale, axis[1], bladeOrigin );
				VectorMA( bladeOrigin, 24*scale, axis[0], bladeOrigin );
				break;
			case 2:
				VectorMA( bladeOrigin, 6*scale, axis[1], bladeOrigin );
				VectorMA( bladeOrigin, 24*scale, axis[0], bladeOrigin );
				break;
			case 3:
				VectorMA( bladeOrigin, -32*scale, axis[0], bladeOrigin );
				VectorScale( axis[0], -1, axis[0] );
				break;
			}
			break;
		case SABER_SITH_SWORD:
			//no blade
			break;
		default:
			break;
		}
	}
	if ( saberType == SABER_SITH_SWORD )
	{//draw no blade
		return;
	}

	if (cg_SFXSabers.integer < 1)
	{// Draw the Raven blade.
		if (saberType == SABER_UNSTABLE)
		{
			UI_DoTFASaber(bladeOrigin, axis[0], bladeLength, bladeLength, bladeRadius, bladeColor, snum);
		}
		else
		{
			UI_DoSaber(bladeOrigin, axis[0], bladeLength, bladeLength, bladeRadius, bladeColor, snum);
		}
	}
	else
	{
		if (saberType == SABER_UNSTABLE)
		{
			UI_DoTFASaber(bladeOrigin, axis[0], bladeLength, bladeLength, bladeRadius, bladeColor, snum);
		}
		else
		{
			switch (cg_SFXSabers.integer)
			{
			case 1:
				UI_DobattlefrontSaber(bladeOrigin, axis[0], bladeLength, bladeLength, bladeRadius, bladeColor, snum);
				break;
			case 2:
				UI_DoSFXSaber(bladeOrigin, axis[0], bladeLength, bladeLength, bladeRadius, bladeColor, snum);
				break;
			case 3:
				UI_DoEp1Saber(bladeOrigin, axis[0], bladeLength, bladeLength, bladeRadius, bladeColor, snum);
				break;
			case 4:
				UI_DoEp2Saber(bladeOrigin, axis[0], bladeLength, bladeLength, bladeRadius, bladeColor, snum);
				break;
			case 5:
				UI_DoEp3Saber(bladeOrigin, axis[0], bladeLength, bladeLength, bladeRadius, bladeColor, snum);
				break;
			case 6:
				UI_DoOTSaber(bladeOrigin, axis[0], bladeLength, bladeLength, bladeRadius, bladeColor, snum);
				break;
			case 7:
				UI_DoTFASaber(bladeOrigin, axis[0], bladeLength, bladeLength, bladeRadius, bladeColor, snum);
				break;
			case 8:
				UI_DoCustomSaber(bladeOrigin, axis[0], bladeLength, bladeLength, bladeRadius, bladeColor, snum);
				break;
			}
		}
	}
}

void UI_GetSaberForMenu( char *saber, int saberNum )
{
	char saberTypeString[MAX_QPATH]={0};
	saberType_t saberType = SABER_NONE;

	if ( saberNum == 0 )
	{
		trap->Cvar_VariableStringBuffer("ui_saber", saber, MAX_QPATH );
		if ( !UI_SaberValidForPlayerInMP( saber ) )
		{
			trap->Cvar_Set( "ui_saber", DEFAULT_SABER );
			trap->Cvar_VariableStringBuffer("ui_saber", saber, MAX_QPATH );
		}
	}
	else
	{
		trap->Cvar_VariableStringBuffer("ui_saber2", saber, MAX_QPATH );
		if ( !UI_SaberValidForPlayerInMP( saber ) )
		{
			trap->Cvar_Set( "ui_saber2", DEFAULT_SABER );
			trap->Cvar_VariableStringBuffer("ui_saber2", saber, MAX_QPATH );
		}
	}
	//read this from the sabers.cfg
	UI_SaberTypeForSaber( saber, saberTypeString );
	if ( saberTypeString[0] )
	{
		saberType = TranslateSaberType( saberTypeString );
	}

	switch ( uiInfo.movesTitleIndex )
	{
	case 0://MD_ACROBATICS:
		break;
	case 1://MD_SINGLE_FAST:
	case 2://MD_SINGLE_MEDIUM:
	case 3://MD_SINGLE_STRONG:
		if ( saberType != SABER_SINGLE )
		{
			Q_strncpyz(saber, "saber_1", MAX_QPATH);
		}
		break;
	case 4://MD_DUAL_SABERS:
		if ( saberType != SABER_SINGLE )
		{
			Q_strncpyz(saber, "saber_1", MAX_QPATH);
		}
		break;
	case 5://MD_SABER_STAFF:
		if ( saberType == SABER_SINGLE || saberType == SABER_NONE )
		{
			Q_strncpyz(saber, "staff_1", MAX_QPATH);
		}
		break;
	}
}

void UI_SaberDrawBlades( itemDef_t *item, vec3_t origin, vec3_t angles )
{
	//NOTE: only allows one saber type in view at a time
	char saber[MAX_QPATH];
	int saberNum = 0;
	int saberModel = 0;
	int	numSabers = 1;

	if ( (item->flags&ITF_ISCHARACTER)//hacked sabermoves sabers in character's hand
		&& uiInfo.movesTitleIndex == 4 /*MD_DUAL_SABERS*/ )
	{
		numSabers = 2;
	}

	for ( saberNum = 0; saberNum < numSabers; saberNum++ )
	{
		if ( (item->flags&ITF_ISCHARACTER) )//hacked sabermoves sabers in character's hand
		{
			UI_GetSaberForMenu( saber, saberNum );
			saberModel = saberNum + 1;
		}
		else if ( (item->flags&ITF_ISSABER) )
		{
			trap->Cvar_VariableStringBuffer("ui_saber", saber, sizeof(saber) );
			if ( !UI_SaberValidForPlayerInMP( saber ) )
			{
				trap->Cvar_Set( "ui_saber", DEFAULT_SABER );
				trap->Cvar_VariableStringBuffer("ui_saber", saber, sizeof(saber) );
			}
			saberModel = 0;
		}
		else if ( (item->flags&ITF_ISSABER2) )
		{
			trap->Cvar_VariableStringBuffer("ui_saber2", saber, sizeof(saber) );
			if ( !UI_SaberValidForPlayerInMP( saber ) )
			{
				trap->Cvar_Set( "ui_saber2", DEFAULT_SABER );
				trap->Cvar_VariableStringBuffer("ui_saber2", saber, sizeof(saber) );
			}
			saberModel = 0;
		}
		else
		{
			return;
		}
		if ( saber[0] )
		{
			saberType_t saberType;
			int curBlade = 0;
			int numBlades = UI_SaberNumBladesForSaber( saber );
			if ( numBlades )
			{//okay, here we go, time to draw each blade...
				char	saberTypeString[MAX_QPATH]={0};
				UI_SaberTypeForSaber( saber, saberTypeString );
				saberType = TranslateSaberType( saberTypeString );
				for ( curBlade = 0; curBlade < numBlades; curBlade++ )
				{
					if ( UI_SaberShouldDrawBlade( saber, curBlade ) )
					{
						UI_SaberDrawBlade( item, saber, saberModel, saberType, origin, angles, curBlade );
					}
				}
			}
		}
	}
}

void UI_SaberAttachToChar( itemDef_t *item )
{
	int	numSabers = 1;
 	int	saberNum = 0;

	if ( trap->G2API_HasGhoul2ModelOnIndex(&(item->ghoul2),2) )
	{//remove any extra models
		trap->G2API_RemoveGhoul2Model(&(item->ghoul2), 2);
	}
	if ( trap->G2API_HasGhoul2ModelOnIndex(&(item->ghoul2),1) )
	{//remove any extra models
		trap->G2API_RemoveGhoul2Model(&(item->ghoul2), 1);
	}

	if ( uiInfo.movesTitleIndex == 4 /*MD_DUAL_SABERS*/ )
	{
		numSabers = 2;
	}

	for ( saberNum = 0; saberNum < numSabers; saberNum++ )
	{
		//bolt sabers
		char modelPath[MAX_QPATH];
		char skinPath[MAX_QPATH];
		char saber[MAX_QPATH];

		UI_GetSaberForMenu( saber, saberNum );

		if ( UI_SaberModelForSaber( saber, modelPath ) )
		{//successfully found a model
			int g2Saber = trap->G2API_InitGhoul2Model( &(item->ghoul2), modelPath, 0, 0, 0, 0, 0 ); //add the model
			if ( g2Saber )
			{
				int boltNum;
				//get the customSkin, if any
				if ( UI_SaberSkinForSaber( saber, skinPath ) )
				{
					int g2skin = trap->R_RegisterSkin(skinPath);
					trap->G2API_SetSkin( item->ghoul2, g2Saber, 0, g2skin );//this is going to set the surfs on/off matching the skin file
				}
				else
				{
					trap->G2API_SetSkin( item->ghoul2, g2Saber, 0, 0 );//turn off custom skin
				}
				if ( saberNum == 0 )
				{
					boltNum = trap->G2API_AddBolt( item->ghoul2, 0, "*r_hand");
				}
				else
				{
					boltNum = trap->G2API_AddBolt( item->ghoul2, 0, "*l_hand");
				}
				trap->G2API_AttachG2Model( item->ghoul2, g2Saber, item->ghoul2, boltNum, 0);
			}
		}
	}
}
