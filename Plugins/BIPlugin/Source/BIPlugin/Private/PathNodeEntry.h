#pragma once

class PathNodeEntry
{
public:
	PathNodeEntry();
	PathNodeEntry(const UK2Node& a_Node);
	bool operator ==(const PathNodeEntry& a_Other) const;
	friend uint32 GetTypeHash(const PathNodeEntry& a_Instance);
	friend FArchive& operator << (FArchive& a_Archive, PathNodeEntry& a_Value);

	FBlueprintNodeSignature m_NodeSignature;
	FGuid m_NodeSignatureGuid;
	FText m_NodeTitle;
};