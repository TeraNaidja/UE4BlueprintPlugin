#pragma once

class Suggestion
{
public:
	Suggestion(const FString& a_NodeSignature, float a_Score);
	~Suggestion();

	const FString& GetNodeSignature() const;
	float GetSuggestionScore() const;

	void SetSuggestionScore(float a_Score);
private:
	FString m_NodeSignature;
	float m_SuggestionScore;
};
