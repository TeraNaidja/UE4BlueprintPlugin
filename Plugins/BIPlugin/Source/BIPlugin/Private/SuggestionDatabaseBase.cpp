#include "BIPluginPrivatePCH.h"
#include "SuggestionDatabaseBase.h"

SuggestionDatabaseBase::SuggestionDatabaseBase()
	: m_GraphNodeDatabase(nullptr)
{
}

SuggestionDatabaseBase::~SuggestionDatabaseBase()
{
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
