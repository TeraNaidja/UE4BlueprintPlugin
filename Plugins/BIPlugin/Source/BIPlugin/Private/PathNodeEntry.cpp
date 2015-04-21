#include "BIPluginPrivatePCH.h"
#include "PathNodeEntry.h"

PathNodeEntry::PathNodeEntry()
	: m_NodeSignature("DEFAULT_INVALID_SIGNATURE")
{
}

PathNodeEntry::PathNodeEntry(const UK2Node& a_Node)
	: m_NodeSignature(a_Node.GetSignature())
	, m_NodeSignatureGuid(m_NodeSignature.AsGuid())
	, m_NodeTitle(a_Node.GetNodeTitle(ENodeTitleType::MenuTitle))
{
}

bool PathNodeEntry::operator ==(const PathNodeEntry& a_Other) const
{
	return m_NodeSignatureGuid == a_Other.m_NodeSignatureGuid;
}

uint32 GetTypeHash(const PathNodeEntry& a_Instance)
{
	return GetTypeHash(a_Instance.m_NodeSignatureGuid);
}