#include "BIPluginPrivatePCH.h"
#include "SuggestionProvider.h"

#include "ISuggestionDatabase.h"
#include "Suggestion.h"

namespace
{
	void BacktrackNodeRecursive(UK2Node* a_Node, EEdGraphPinDirection a_ExploreDirection, FString& out_Result)
	{
		FBlueprintNodeSignature signature = a_Node->GetSignature();
		a_Node->GetClass()->AppendName(out_Result);
		out_Result.Append("(").Append(a_Node->GetSignature().ToString()).Append(")"); 
		out_Result.Append(" >> ");

		for (auto childPin : a_Node->Pins)
		{
			if (childPin->Direction == a_ExploreDirection && !childPin->bHidden && !childPin->bNotConnectable)
			{
				for (auto linkedPin : childPin->LinkedTo)
				{
					check(linkedPin->GetOuter()->IsA(UK2Node::StaticClass()));
					UK2Node* parentNode = Cast<UK2Node>(linkedPin->GetOuter());
					BacktrackNodeRecursive(parentNode, a_ExploreDirection, out_Result);
				}
			}
		}
	}

	FString GetBacktrackPath(UK2Node* a_SourceNode, EEdGraphPinDirection a_SourceDirection)
	{
		FString returnValue;

		EEdGraphPinDirection exploreDirection = EEdGraphPinDirection::EGPD_Output;
		switch (a_SourceDirection)
		{
		case EEdGraphPinDirection::EGPD_Input:
			exploreDirection = EEdGraphPinDirection::EGPD_Output;
			break;
		case EEdGraphPinDirection::EGPD_Output:
			exploreDirection = EEdGraphPinDirection::EGPD_Input;
			break;
		default:
			UE_LOG(LogTemp, Error, TEXT("Unknown EEdGraphPinDirection encountered"));
			break;
		}

		BacktrackNodeRecursive(a_SourceNode, exploreDirection, returnValue);

		UE_LOG(LogTemp, Warning, TEXT("%s"), *returnValue);

		return returnValue;
	}
}

SuggestionProvider::SuggestionProvider(const ISuggestionDatabase& a_Database)
	: m_SuggestionDatabase(a_Database)
{
}

SuggestionProvider::~SuggestionProvider()
{
}

void SuggestionProvider::ProvideSuggestions(const FBlueprintSuggestionContext& InContext, TArray<TSharedPtr<FBlueprintSuggestion>>& OutEntries)
{
	const int32 NUM_SUGGESTIONS = 5;

	TArray<Suggestion> suggestions;
	suggestions.Reserve(NUM_SUGGESTIONS);
	m_SuggestionDatabase.ProvideSuggestions(InContext, NUM_SUGGESTIONS, suggestions);

	for (Suggestion suggested : suggestions)
	{
		OutEntries.Add(TSharedPtr<FBlueprintSuggestion>(new FBlueprintSuggestion(suggested.GetNodeSignature(), suggested.GetNodeTitle())));
	}
}
