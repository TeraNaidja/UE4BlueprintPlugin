#pragma once

#include "EdGraph/EdGraphPin.h"

class GraphNodeInformation
{
public:
	GraphNodeInformation(UK2Node& a_Node);
	~GraphNodeInformation();

	const TArray<FEdGraphPinType>& GetInputPins() const;
	const TArray<FEdGraphPinType>& GetOutputPins() const;

	bool HasPinTypeInDirection(const FEdGraphPinType& a_PinType, EEdGraphPinDirection a_Direction) const;

private:
	const TArray<FEdGraphPinType>& GetPinArrayForDirection(EEdGraphPinDirection a_Direction) const;

	TArray<FEdGraphPinType> m_InputPins;
	TArray<FEdGraphPinType> m_OutputPins;
};
