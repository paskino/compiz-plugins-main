<<<<<<< HEAD:domino.c
#include "animation.h"
=======
#include "animation-internal.h"
>>>>>>> 1d600624bf9a0956375a82cfdd1f99da511af55f:domino.c
void fxDomino3DInit(CompScreen * s, CompWindow * w)
{
	ANIM_WINDOW(w);
	ANIM_SCREEN(s);

	// Sub effects:
	// 1: Domino
	// 2: Razr
	int subEffectNo = aw->subEffectNo;

	int fallDir;

	if (subEffectNo == 2)
		fallDir = getAnimationDirection
				(w, &as->opt[ANIM_SCREEN_OPTION_RAZR_DIRECTION].value, TRUE);
	else
		fallDir = getAnimationDirection
				(w, &as->opt[ANIM_SCREEN_OPTION_DOMINO_DIRECTION].value,
				 TRUE);

	int defaultGridSize = 20;
	float minCellSize = 30;
	int gridSizeX;
	int gridSizeY;
	int fallDirGridSize;
	float minDistStartEdge;		// half piece size in [0,1] range
	float gridCellW;
	float gridCellH;
	float cellAspectRatio = 1.25;

	if (subEffectNo == 2)
		cellAspectRatio = 1;

	// Determine sensible domino piece sizes
	if (fallDir == AnimDirectionDown || fallDir == AnimDirectionUp)
	{
		if (minCellSize > BORDER_W(w))
			minCellSize = BORDER_W(w);
		if (BORDER_W(w) / defaultGridSize < minCellSize)
			gridSizeX = (int)(BORDER_W(w) / minCellSize);
		else
			gridSizeX = defaultGridSize;
		gridCellW = BORDER_W(w) / gridSizeX;
		gridSizeY = (int)(BORDER_H(w) / (gridCellW * cellAspectRatio));
		if (gridSizeY == 0)
			gridSizeY = 1;
		gridCellH = BORDER_H(w) / gridSizeY;
		fallDirGridSize = gridSizeY;
	}
	else
	{
		if (minCellSize > BORDER_H(w))
			minCellSize = BORDER_H(w);
		if (BORDER_H(w) / defaultGridSize < minCellSize)
			gridSizeY = (int)(BORDER_H(w) / minCellSize);
		else
			gridSizeY = defaultGridSize;
		gridCellH = BORDER_H(w) / gridSizeY;
		gridSizeX = (int)(BORDER_W(w) / (gridCellH * cellAspectRatio));
		if (gridSizeX == 0)
			gridSizeX = 1;
		gridCellW = BORDER_W(w) / gridSizeX;
		fallDirGridSize = gridSizeX;
	}
	minDistStartEdge = (1.0 / fallDirGridSize) / 2;

	float thickness = MIN(gridCellW, gridCellH) / 3.5;

	if (!tessellateIntoRectangles(w, gridSizeX, gridSizeY, thickness))
		return;

	float rotAxisX = 0;
	float rotAxisY = 0;
	Point3d rotAxisOff = { 0, 0, thickness / 2 };
	float posX = 0;				// position of standing piece
	float posY = 0;
	float posZ = 0;
	int nFallingColumns = gridSizeX;
	float gridCellHalfW = gridCellW / 2;
	float gridCellHalfH = gridCellH / 2;

	switch (fallDir)
	{
	case AnimDirectionDown:
		rotAxisX = -1;
		if (subEffectNo == 2)
			rotAxisOff.y = -gridCellHalfH;
		else
		{
			posY = -(gridCellHalfH + thickness);
			posZ = gridCellHalfH - thickness / 2;
		}
		break;
	case AnimDirectionLeft:
		rotAxisY = -1;
		if (subEffectNo == 2)
			rotAxisOff.x = gridCellHalfW;
		else
		{
			posX = gridCellHalfW + thickness;
			posZ = gridCellHalfW - thickness / 2;
		}
		nFallingColumns = gridSizeY;
		break;
	case AnimDirectionUp:
		rotAxisX = 1;
		if (subEffectNo == 2)
			rotAxisOff.y = gridCellHalfH;
		else
		{
			posY = gridCellHalfH + thickness;
			posZ = gridCellHalfH - thickness / 2;
		}
		break;
	case AnimDirectionRight:
		rotAxisY = 1;
		if (subEffectNo == 2)
			rotAxisOff.x = -gridCellHalfW;
		else
		{
			posX = -(gridCellHalfW + thickness);
			posZ = gridCellHalfW - thickness / 2;
		}
		nFallingColumns = gridSizeY;
		break;
	}

	float fadeDuration;
	float riseDuration;
	float riseTimeRandMax = 0.2;

	if (subEffectNo == 2)
	{
		riseDuration = (1 - riseTimeRandMax) / fallDirGridSize;
		fadeDuration = riseDuration / 2;
	}
	else
	{
		fadeDuration = 0.18;
		riseDuration = 0.2;
	}
	float *riseTimeRandSeed = calloc(1, nFallingColumns * sizeof(float));

	if (!riseTimeRandSeed)
		// TODO: log error here
		return;
	int i;

	for (i = 0; i < nFallingColumns; i++)
		riseTimeRandSeed[i] = RAND_FLOAT();

	PolygonSet *pset = aw->polygonSet;
	PolygonObject *p = pset->polygons;

	for (i = 0; i < pset->nPolygons; i++, p++)
	{
		p->rotAxis.x = rotAxisX;
		p->rotAxis.y = rotAxisY;
		p->rotAxis.z = 0;

		p->finalRelPos.x = posX;
		p->finalRelPos.y = posY;
		p->finalRelPos.z = posZ;

		// dist. from rise-start / fall-end edge in window ([0,1] range)
		float distStartEdge = 0;

		// dist. from edge perpendicular to movement (for move start time)
		// so that same the blocks in same row/col. appear to knock down
		// the next one
		float distPerpEdge = 0;

		switch (fallDir)
		{
		case AnimDirectionUp:
			distStartEdge = p->centerRelPos.y;
			distPerpEdge = p->centerRelPos.x;
			break;
		case AnimDirectionRight:
			distStartEdge = 1 - p->centerRelPos.x;
			distPerpEdge = p->centerRelPos.y;
			break;
		case AnimDirectionDown:
			distStartEdge = 1 - p->centerRelPos.y;
			distPerpEdge = p->centerRelPos.x;
			break;
		case AnimDirectionLeft:
			distStartEdge = p->centerRelPos.x;
			distPerpEdge = p->centerRelPos.y;
			break;
		}

		float riseTimeRand =
				riseTimeRandSeed[(int)(distPerpEdge * nFallingColumns)] *
				riseTimeRandMax;

		p->moveDuration = riseDuration;

		float mult = 1;
		if (fallDirGridSize > 1)
			mult = ((distStartEdge - minDistStartEdge) /
					(1 - 2 * minDistStartEdge));
		if (subEffectNo == 2)
		{
			p->moveStartTime =
					mult *
					(1 - riseDuration - riseTimeRandMax) + riseTimeRand;
			p->fadeStartTime = p->moveStartTime + riseDuration / 2;
			p->finalRotAng = -180;

			p->rotAxisOffset.x = rotAxisOff.x;
			p->rotAxisOffset.y = rotAxisOff.y;
			p->rotAxisOffset.z = rotAxisOff.z;
		}
		else
		{
			p->moveStartTime =
					mult *
					(1 - riseDuration - riseTimeRandMax) +
					riseTimeRand;
			p->fadeStartTime =
					p->moveStartTime + riseDuration - riseTimeRand + 0.03;
			p->finalRotAng = -90;
		}
		if (p->fadeStartTime > 1 - fadeDuration)
			p->fadeStartTime = 1 - fadeDuration;
		p->fadeDuration = fadeDuration;
	}
	free(riseTimeRandSeed);
	pset->doDepthTest = TRUE;
	pset->doLighting = TRUE;
	pset->correctPerspective = TRUE;
}
