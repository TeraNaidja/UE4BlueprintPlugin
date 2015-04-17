#include "BIPluginPrivatePCH.h"
#include "Suggestion.h"

Suggestion::Suggestion(const FBlueprintNodeSignature &a_NodeSignature, float a_Score)
	: m_NodeSignature(a_NodeSignature)
	, m_NodeSignatureGuid(a_NodeSignature.AsGuid())
	, m_SuggestionScore(a_Score)
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

float Suggestion::GetSuggestionScore() const
{
	return m_SuggestionScore;
}

void Suggestion::SetSuggestionScore(float a_Score)
{
	m_SuggestionScore = a_Score;
}
