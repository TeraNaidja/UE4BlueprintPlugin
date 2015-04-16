#include "BIPluginPrivatePCH.h"
#include "PathNodeEntry.h"

PathNodeEntry::PathNodeEntry()
	: m_NodeSignature("DEFAULT_INVALID_SIGNATURE")
{
}

PathNodeEntry::PathNodeEntry(const UK2Node& a_Node)
	: m_NodeSignature(a_Node.GetSignature())
	, m_NodeTitle(a_Node.GetNodeTitle(ENodeTitleType::MenuTitle))
{
}

bool PathNodeEntry::operator ==(const PathNodeEntry& a_Other) const
{
	return m_NodeSignature.ToString() == a_Other.m_NodeSignature.ToString();
}

uint32 GetTypeHash(const PathNodeEntry& a_Instance)
{
	const FString signatureString = a_Instance.m_NodeSignature.ToString();
	return FCrc::MemCrc32(*signatureString, signatureString.Len() * sizeof(TCHAR));
}