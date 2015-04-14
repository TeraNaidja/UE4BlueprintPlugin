#include "BIPluginPrivatePCH.h"
#include "Suggestion.h"

Suggestion::Suggestion(const FString& a_NodeSignature, float a_Score)
	: m_NodeSignature(a_NodeSignature)
	, m_SuggestionScore(a_Score)
{
}

Suggestion::~Suggestion()
{
}

const FString& Suggestion::GetNodeSignature() const
{
	return m_NodeSignature;
}

float Suggestion::GetSuggestionScore() const
{
	return m_SuggestionScore;
}

void Suggestion::SetSuggestionScore(float a_Score)
{
	m_SuggestionScore = a_Score;
}
