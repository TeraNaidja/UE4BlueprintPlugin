#pragma once

#include "PathNodeEntry.h"
#include "PathContextPath.h"
#include "EPathDirection.h"

class PathPredictionEntry
{
public:
	PathPredictionEntry();
	~PathPredictionEntry();

	bool CompareExcludingUses(const PathPredictionEntry& a_Other) const;

	//In graph this is ContextPath -> AnchorVertex -> PredictionVertex in the direction of Direction
	EPathDirection m_Direction;
	PathNodeEntry m_PredictionVertex;
	PathNodeEntry m_AnchorVertex;
	PathContextPath m_ContextPath;
	int32 m_NumUses;
};
