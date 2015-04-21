#include "BIPluginPrivatePCH.h"
#include "SuggestionDatabaseBase.h"

SuggestionDatabaseBase::SuggestionDatabaseBase()
	: m_GraphNodeDatabase(nullptr)
{
}

SuggestionDatabaseBase::~SuggestionDatabaseBase()
{
}

void SuggestionDatabaseBase::FillSuggestionDatabase()
{
	for (TObjectIterator<UBlueprint> BlueprintIt; BlueprintIt; ++BlueprintIt)
	{
		UBlueprint* Blueprint = *BlueprintIt;
		ParseBlueprint(*Blueprint);
	}
}

void SuggestionDatabaseBase::SetGraphNodeDatabase(GraphNodeInformationDatabase* a_Database)
{
	m_GraphNodeDatabase = a_Database;
}

GraphNodeInformationDatabase& SuggestionDatabaseBase::GetGraphNodeDatabase() const
{
	verify(m_GraphNodeDatabase != nullptr);
	return *m_GraphNodeDatabase;
}
