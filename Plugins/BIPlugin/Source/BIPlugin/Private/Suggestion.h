#pragma once

class Suggestion
{
public:
	Suggestion(const FBlueprintNodeSignature& a_NodeSignature, const FText& a_NodeTitle, float a_Score);
	~Suggestion();

	bool CompareSignatures(const Suggestion& a_Other) const;

	const FBlueprintNodeSignature& GetNodeSignature() const;
	const FText& GetNodeTitle() const;
	float GetSuggestionScore() const;

	void SetSuggestionScore(float a_Score);
private:
	FBlueprintNodeSignature m_NodeSignature;
	FText m_NodeTitle;
	float m_SuggestionScore;
};
