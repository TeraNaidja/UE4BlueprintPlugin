#pragma once

class PathNodeEntry
{
public:
	PathNodeEntry();
	PathNodeEntry(const UK2Node& a_Node);
	bool operator ==(const PathNodeEntry& a_Other) const;
	friend uint32 GetTypeHash(const PathNodeEntry& a_Instance);

	FBlueprintNodeSignature m_NodeSignature;
	FText m_NodeTitle;
};