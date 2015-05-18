#pragma once

#include "GraphNodeInformation.h"

class GraphNodeInformationDatabase
{
public:
	GraphNodeInformationDatabase();
	~GraphNodeInformationDatabase();

	void FillDatabase();
	void FlushDatabase();

	const GraphNodeInformation* FindNodeInformation(const FGuid& a_NodeSignatureGuid, UEdGraph* a_ContainingGraph);

private:
	TMap<FGuid, GraphNodeInformation> m_GraphNodeInformation;
	bool m_HasBuiltDatabase;
};
