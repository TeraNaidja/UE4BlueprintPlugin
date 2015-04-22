#include "BIPluginPrivatePCH.h"
#include "PathPredictionEntry.h"

PathPredictionEntry::PathPredictionEntry()
	: m_NumUses(0)
{
}

PathPredictionEntry::~PathPredictionEntry()
{
}

bool PathPredictionEntry::CompareExcludingUses(const PathPredictionEntry& a_Other) const
{
	return m_AnchorVertex == a_Other.m_AnchorVertex &&
		m_PredictionVertex == a_Other.m_PredictionVertex &&
		m_ContextPath == a_Other.m_ContextPath;
}

FArchive& operator << (FArchive& a_Archive, PathPredictionEntry& a_Value)
{
	int32 direction = (int32)a_Value.m_Direction;
	return a_Archive << direction << a_Value.m_PredictionVertex << a_Value.m_AnchorVertex << a_Value.m_ContextPath << 
		a_Value.m_NumUses;
}
