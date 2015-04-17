#pragma once

#include "GraphNodeInformation.h"

class GraphNodeInformationDatabase
{
public:
	GraphNodeInformationDatabase();
	~GraphNodeInformationDatabase();

	void FillDatabase();

	const GraphNodeInformation* FindNodeInformation(const FGuid& a_NodeSignatureGuid) const;

private:
	TMap<FGuid, GraphNodeInformation> m_GraphNodeInformation;
};
