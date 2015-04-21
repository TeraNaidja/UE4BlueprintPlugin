#include "BIPluginPrivatePCH.h"
#include "Suggestion.h"

Suggestion::Suggestion()
{
}

Suggestion::Suggestion(const FBlueprintNodeSignature &a_NodeSignature, float a_ContextScore, int32 a_UsesScore)
	: m_NodeSignature(a_NodeSignature)
	, m_NodeSignatureGuid(a_NodeSignature.AsGuid())
	, m_SuggestionScoreContext(a_ContextScore)
	, m_SuggestionScoreUses(a_UsesScore)
{
}

Suggestion::~Suggestion()
{
}

bool Suggestion::CompareSignatures(const Suggestion& a_Other) const
{
	return a_Other.m_NodeSignatureGuid == m_NodeSignatureGuid;
}

const FBlueprintNodeSignature& Suggestion::GetNodeSignature() const
{
	return m_NodeSignature;
}

const FGuid& Suggestion::GetNodeSignatureGuid() const
{
	return m_NodeSignatureGuid;
}

float Suggestion::GetSuggestionContextScore() const
{
	return m_SuggestionScoreContext;
}

int32 Suggestion::GetSuggestionUsesScore() const
{
	return m_SuggestionScoreUses;
}

void Suggestion::SetSuggestionContextScore(float a_ContextScore)
{
	m_SuggestionScoreContext = a_ContextScore;
}

void Suggestion::SetSuggestionUsesScore(int32 a_UsesScore)
{
	m_SuggestionScoreUses = a_UsesScore;
}
