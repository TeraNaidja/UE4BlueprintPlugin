#pragma once

class Suggestion
{
public:
	Suggestion();
	Suggestion(const FBlueprintNodeSignature& a_NodeSignature, float a_ContextScore, int32 a_UsesScore);
	~Suggestion();

	bool CompareSignatures(const Suggestion& a_Other) const;

	const FBlueprintNodeSignature& GetNodeSignature() const;
	const FGuid& GetNodeSignatureGuid() const;
	float GetSuggestionContextScore() const;
	int32 GetSuggestionUsesScore() const;

	void SetSuggestionContextScore(float a_ContextScore);
	void SetSuggestionUsesScore(int32 a_UsesScore);
private:
	FBlueprintNodeSignature m_NodeSignature;
	FGuid m_NodeSignatureGuid;
	float m_SuggestionScoreContext;
	int32 m_SuggestionScoreUses;
};
