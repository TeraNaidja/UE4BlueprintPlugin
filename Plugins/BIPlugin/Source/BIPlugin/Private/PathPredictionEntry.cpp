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
