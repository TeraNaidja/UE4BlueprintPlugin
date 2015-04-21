#include "BIPluginPrivatePCH.h"
#include "GraphNodeInformationDatabase.h"
#include "BlueprintActionDatabase.h"
#include "GraphNodeInformation.h"

GraphNodeInformationDatabase::GraphNodeInformationDatabase()
{
}

GraphNodeInformationDatabase::~GraphNodeInformationDatabase()
{
}

void GraphNodeInformationDatabase::FillDatabase()
{
	FBlueprintActionDatabase::FActionRegistry const& actionDatabase = FBlueprintActionDatabase::Get().GetAllActions();
	for (auto const& actionEntry : actionDatabase)
	{
		for (UBlueprintNodeSpawner const* nodeSpawner : actionEntry.Value)
		{
			UEdGraphNode* nodeTemplate = nodeSpawner->GetTemplateNode();
			if (nodeTemplate != nullptr && nodeTemplate->IsA(UK2Node::StaticClass()))
			{
				UK2Node* ukNode = Cast<UK2Node>(nodeTemplate);
				
				FBlueprintNodeSignature signature = ukNode->GetSignature();
				FGuid nodeSignatureGuid = signature.AsGuid();
				
				GraphNodeInformation nodeInfo(*ukNode);

				m_GraphNodeInformation.Add(nodeSignatureGuid, nodeInfo);
			}
		}
	}
}

const GraphNodeInformation* GraphNodeInformationDatabase::FindNodeInformation(const FGuid& a_NodeSignatureGuid) const
{
	return m_GraphNodeInformation.Find(a_NodeSignatureGuid);
}
