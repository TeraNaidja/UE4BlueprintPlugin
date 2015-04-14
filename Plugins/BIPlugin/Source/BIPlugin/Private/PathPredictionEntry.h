#pragma once

#include "PathNodeEntry.h"
#include "EPathDirection.h"

class PathPredictionEntry
{
public:
	PathPredictionEntry();

	bool CompareExcludingUses(const PathPredictionEntry& a_Other) const;

	//In graph this is ContextPath -> AnchorVertex -> PredictionVertex in the direction of Direction
	EPathDirection m_Direction;
	PathNodeEntry m_PredictionVertex;
	PathNodeEntry m_AnchorVertex;
	TArray<PathNodeEntry> m_ContextPath;
	int32 m_NumUses;
};
