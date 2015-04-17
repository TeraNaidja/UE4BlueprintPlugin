#include "BIPluginPrivatePCH.h"
#include "GraphNodeInformation.h"

namespace
{
	void DiscoverNodePinTypes(const UK2Node& a_Node, EEdGraphPinDirection a_PinDirection, TArray<FEdGraphPinType>& a_Output)
	{
		for (UEdGraphPin* pin : a_Node.Pins)
		{
			if (pin->Direction == a_PinDirection)
			{
				a_Output.Push(FEdGraphPinType(pin->PinType));
			}
		}
	}
}

GraphNodeInformation::GraphNodeInformation(const UK2Node& a_Node)
{
	DiscoverNodePinTypes(a_Node, EEdGraphPinDirection::EGPD_Input, m_InputPins);
	DiscoverNodePinTypes(a_Node, EEdGraphPinDirection::EGPD_Output, m_OutputPins);
}

GraphNodeInformation::~GraphNodeInformation()
{	
}

const TArray<FEdGraphPinType>& GraphNodeInformation::GetInputPins() const
{
	return m_InputPins;
}

const TArray<FEdGraphPinType>& GraphNodeInformation::GetOutputPins() const
{
	return m_OutputPins;
}

bool GraphNodeInformation::HasPinTypeInDirection(const FEdGraphPinType& a_PinType, EEdGraphPinDirection a_Direction) const
{
	const TArray<FEdGraphPinType>& pinData = GetPinArrayForDirection(a_Direction);
	return pinData.ContainsByPredicate([&](const FEdGraphPinType& obj) -> bool {
		return obj.PinCategory == a_PinType.PinCategory;
	});
}

const TArray<FEdGraphPinType>& GraphNodeInformation::GetPinArrayForDirection(EEdGraphPinDirection a_Direction) const
{
	return a_Direction == EEdGraphPinDirection::EGPD_Input ? m_InputPins : m_OutputPins;
}
