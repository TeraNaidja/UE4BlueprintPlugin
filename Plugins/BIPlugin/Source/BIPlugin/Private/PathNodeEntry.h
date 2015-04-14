#pragma once

class PathNodeEntry
{
public:
	PathNodeEntry();
	PathNodeEntry(const FString& a_NodeSignature);
	PathNodeEntry(const UK2Node& a_Node);
	bool operator ==(const PathNodeEntry& a_Other) const;
	friend uint32 GetTypeHash(const PathNodeEntry& a_Instance);

	FString m_NodeSignature;
};