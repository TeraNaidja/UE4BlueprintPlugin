#pragma once

class Suggestion
{
public:
	Suggestion(const FBlueprintNodeSignature& a_NodeSignature, float a_Score);
	~Suggestion();

	bool CompareSignatures(const Suggestion& a_Other) const;

	const FBlueprintNodeSignature& GetNodeSignature() const;
	const FGuid& GetNodeSignatureGuid() const;
	float GetSuggestionScore() const;

	void SetSuggestionScore(float a_Score);
private:
	FBlueprintNodeSignature m_NodeSignature;
	FGuid m_NodeSignatureGuid;
	float m_SuggestionScore;
};
