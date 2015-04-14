#pragma once

#include "Suggestion.h"

struct FBlueprintSuggestionContext;
class ISuggestionDatabase
{
public:
	ISuggestionDatabase();
	~ISuggestionDatabase();

	virtual void ParseBlueprint(const UBlueprint& a_Blueprint) = 0;
	virtual void FlushDatabase() = 0;
	virtual void ProvideSuggestions(const FBlueprintSuggestionContext& a_Context, int32 a_SuggestionCount, TArray<Suggestion>& a_Output) const = 0;
private:
};
