#pragma once

struct FBlueprintSuggestionContext;
class ISuggestionDatabase
{
public:
	struct Suggestion
	{
		FString m_NodeSignature;
	};

	ISuggestionDatabase();
	~ISuggestionDatabase();

	virtual void ParseBlueprint(const UBlueprint& a_Blueprint) = 0;
	virtual void FlushDatabase() = 0;
	virtual void ProvideSuggestions(const FBlueprintSuggestionContext& a_Context, int32 a_SuggestionCount, const TArray<Suggestion>& a_Output) const = 0;
private:
};
