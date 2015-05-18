#include "BIPluginPrivatePCH.h"
#include "GraphNodeInformationDatabase.h"
#include "BlueprintActionDatabase.h"
#include "GraphNodeInformation.h"

namespace
{
	UK2Node* FindTemplateNodeForNodeGuid(const FGuid& a_NodeSignatureGuid, UEdGraph* a_OwningGraph)
	{
		UK2Node* result = nullptr;
		FBlueprintActionDatabase::FActionRegistry const& actionDatabase = FBlueprintActionDatabase::Get().GetAllActions();
		for (auto const& actionEntry : actionDatabase)
		{
			for (UBlueprintNodeSpawner const* nodeSpawner : actionEntry.Value)
			{
				UEdGraphNode* nodeTemplate = nodeSpawner->GetTemplateNode(a_OwningGraph);
				if (nodeTemplate != nullptr && nodeTemplate->IsA(UK2Node::StaticClass()))
				{
					UK2Node* ukNode = Cast<UK2Node>(nodeTemplate);
					if (ukNode->GetSignature().AsGuid() == a_NodeSignatureGuid)
					{
						result = ukNode;
						break;
					}
				}
			}

			if (result != nullptr)
			{
				break;
			}
		}
		return result;
	}
}

GraphNodeInformationDatabase::GraphNodeInformationDatabase()
	: m_HasBuiltDatabase(false)
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
	m_HasBuiltDatabase = true;
}

void GraphNodeInformationDatabase::FlushDatabase()
{
	m_GraphNodeInformation.Empty();
	m_HasBuiltDatabase = false;
}

const GraphNodeInformation* GraphNodeInformationDatabase::FindNodeInformation(const FGuid& a_NodeSignatureGuid, UEdGraph* a_TargetGraph)
{
	const GraphNodeInformation* info = m_GraphNodeInformation.Find(a_NodeSignatureGuid);

	if (info == nullptr && !m_HasBuiltDatabase)
	{
		FillDatabase();
		info = m_GraphNodeInformation.Find(a_NodeSignatureGuid);
		/*UK2Node* templateNode = FindTemplateNodeForNodeGuid(a_NodeSignatureGuid, a_TargetGraph);
		if (templateNode != nullptr)
		{
			GraphNodeInformation nodeInfo(*templateNode);
			info = &(m_GraphNodeInformation.Add(a_NodeSignatureGuid, nodeInfo));
		}*/
	}

	return info;
}
