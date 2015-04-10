#pragma once

class ISuggestionDatabase
{
public:
	ISuggestionDatabase();
	~ISuggestionDatabase();

	virtual void ParseBlueprint(const UBlueprint& a_Blueprint) = 0;
	virtual void FlushDatabase() = 0;
private:
};
