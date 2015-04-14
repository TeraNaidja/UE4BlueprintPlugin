#include "BIPluginPrivatePCH.h"
#include "PathNodeEntry.h"

PathNodeEntry::PathNodeEntry() :
m_NodeSignature("DEFAULT_INVALID_SIGNATURE")
{
}

PathNodeEntry::PathNodeEntry(const FString& a_NodeSignature) :
m_NodeSignature(a_NodeSignature)
{
}

PathNodeEntry::PathNodeEntry(const UK2Node& a_Node) :
	m_NodeSignature(a_Node.GetSignature().ToString())
{
}

bool PathNodeEntry::operator ==(const PathNodeEntry& a_Other) const
{
	return m_NodeSignature == a_Other.m_NodeSignature;
}

uint32 GetTypeHash(const PathNodeEntry& a_Instance)
{
	return FCrc::MemCrc32(*a_Instance.m_NodeSignature, a_Instance.m_NodeSignature.Len() * sizeof(TCHAR));
}