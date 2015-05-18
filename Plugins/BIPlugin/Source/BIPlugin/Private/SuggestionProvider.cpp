#include "BIPluginPrivatePCH.h"
#include "SuggestionProvider.h"

#include "SuggestionDatabaseBase.h"
#include "Suggestion.h"
#include "BlueprintSuggestion.h"
#include "SuggestionDatabaseBase.h"
#include "BlueprintSuggestionContext.h"

namespace
{
}

SuggestionProvider::SuggestionProvider(SuggestionDatabaseBase& a_Database, const RebuildDatabaseDelegate& a_RebuildDatabaseDelegate)
	: m_SuggestionDatabase(a_Database)
	, m_RebuildDatabaseDelegate(a_RebuildDatabaseDelegate)
	, m_LastGraphForSuggestions(nullptr)
	, m_EnabledConsoleCommand(TEXT("BIPlugin_Enabled"), TEXT("Toggles generation of the suggestions"), 
		FConsoleCommandDelegate::CreateRaw(this, &SuggestionProvider::OnEnabledConsoleCommand))
	, m_SuggestionsEnabled(true)
{
}

SuggestionProvider::~SuggestionProvider()
{
	if (m_LastGraphForSuggestions != nullptr)
	{
		m_LastGraphForSuggestions->RemoveOnGraphChangedHandler(m_OnGraphChangedHandle);
		m_LastGraphForSuggestions = nullptr;
	}
}

void SuggestionProvider::ProvideSuggestions(const FBlueprintSuggestionContext& InContext, TArray<TSharedPtr<FBlueprintSuggestion>>& OutEntries)
{
	const int32 NUM_SUGGESTIONS = 5;

	SubscribeToGraphChanged(InContext.Graphs[0]);

	if (m_SuggestionsEnabled)
	{
		TArray<Suggestion> suggestions;
		suggestions.Reserve(NUM_SUGGESTIONS);
		if (!m_SuggestionDatabase.HasSuggestions())
		{
			m_RebuildDatabaseDelegate.Execute();
		}

		m_SuggestionDatabase.ProvideSuggestions(InContext, NUM_SUGGESTIONS, suggestions);

		int32 suggestionId = 1;
		for (Suggestion suggested : suggestions)
		{
			OutEntries.Add(TSharedPtr<FBlueprintSuggestion>(new FBlueprintSuggestion(
				suggested.GetNodeSignature(),
				suggested.GetNodeSignatureGuid(),
				suggestionId,
				suggested.GetSuggestionContextScore(),
				suggested.GetSuggestionUsesScore())));
			++suggestionId;
		}
	}
}

void SuggestionProvider::SubscribeToGraphChanged(UEdGraph* a_Graph)
{
	if (m_LastGraphForSuggestions != a_Graph)
	{
		if (m_LastGraphForSuggestions != nullptr)
		{
			m_LastGraphForSuggestions->RemoveOnGraphChangedHandler(m_OnGraphChangedHandle);
		}
		m_LastGraphForSuggestions = a_Graph;
		m_OnGraphChangedHandle = m_LastGraphForSuggestions->AddOnGraphChangedHandler(
			FOnGraphChanged::FDelegate::CreateRaw(this, &SuggestionProvider::OnGraphChanged));
	}
}

void SuggestionProvider::OnGraphChanged(const FEdGraphEditAction& a_Action)
{
	//Need to add in suggestions here for when a node is created or re-linked.

	const FSetElementId firstIndex = FSetElementId::FromInteger(0);

	if (a_Action.Action == GRAPHACTION_AddNode || a_Action.Action == GRAPHACTION_AddNodeUI)
	{
		UE_LOG(BILog, BI_VERBOSE, TEXT("Blueprint Node Added: %s"),
			*a_Action.Nodes[firstIndex]->GetNodeTitle(ENodeTitleType::MenuTitle).ToString());
	}

	if (a_Action.Action == GRAPHACTION_PinConnectionCreated)
	{
		UE_LOG(BILog, Log, TEXT("Graph Changed; Node connection created. Adding suggestion to the database"));
		verify(a_Action.Nodes.Num() == 2); //We assume that we have two nodes in this action which the link is created from and to.
		const UEdGraphNode* uncastedNodeA = a_Action.Nodes[firstIndex];
		const UEdGraphNode* uncastedNodeB = a_Action.Nodes[FSetElementId::FromInteger(1)];

		verify(uncastedNodeA->IsA(UK2Node::StaticClass()));
		verify(uncastedNodeB->IsA(UK2Node::StaticClass()));

		const UK2Node* nodeA = Cast<UK2Node>(uncastedNodeA);
		const UK2Node* nodeB = Cast<UK2Node>(uncastedNodeB);

		m_SuggestionDatabase.GenerateSuggestionForCreatedLink(*nodeA, *nodeB);
	}
}

void SuggestionProvider::OnEnabledConsoleCommand()
{
	m_SuggestionsEnabled = !m_SuggestionsEnabled;
	UE_LOG(BILog, Log, TEXT("BIPlugin Suggestion generation is now %s"), m_SuggestionsEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
}
