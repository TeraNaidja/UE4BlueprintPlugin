#pragma once

#include "ISuggestionDatabase.h"

class SuggestionDatabaseTrie: public ISuggestionDatabase
{
	class TrieNode
	{
	};

public:
	SuggestionDatabaseTrie();
	~SuggestionDatabaseTrie();

	virtual void ParseBlueprint(const UBlueprint& a_Blueprint) override;
	virtual void FlushDatabase() override;
private:
};
