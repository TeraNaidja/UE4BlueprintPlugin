#include "BIPluginPrivatePCH.h"
#include "Suggestion.h"

Suggestion::Suggestion(const FBlueprintNodeSignature &a_NodeSignature, const FText& a_NodeTitle, float a_Score)
	: m_NodeSignature(a_NodeSignature)
	, m_NodeTitle(a_NodeTitle)
	, m_SuggestionScore(a_Score)
{
}

Suggestion::~Suggestion()
{
}

bool Suggestion::CompareSignatures(const Suggestion& a_Other) const
{
	return a_Other.m_NodeSignature.ToString() == m_NodeSignature.ToString();
}

const FBlueprintNodeSignature& Suggestion::GetNodeSignature() const
{
	return m_NodeSignature;
}

const FText& Suggestion::GetNodeTitle() const
{
	return m_NodeTitle;
}

float Suggestion::GetSuggestionScore() const
{
	return m_SuggestionScore;
}

void Suggestion::SetSuggestionScore(float a_Score)
{
	m_SuggestionScore = a_Score;
}
