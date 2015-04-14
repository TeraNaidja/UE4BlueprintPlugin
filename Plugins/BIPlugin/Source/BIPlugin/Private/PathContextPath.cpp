#include "BIPluginPrivatePCH.h"
#include "PathContextPath.h"

PathContextPath::PathContextPath()
	: m_ContextPath()
{
}

PathContextPath::PathContextPath(const PathContextPath& a_Other)
	: m_ContextPath(a_Other.m_ContextPath)
{
}

PathContextPath::~PathContextPath()
{
}

bool PathContextPath::operator ==(const PathContextPath& a_Other) const
{
	return m_ContextPath == a_Other.m_ContextPath;
}

void PathContextPath::PushNode(const PathNodeEntry& a_Node)
{
	m_ContextPath.Push(a_Node);
}

float PathContextPath::CompareContext(const PathContextPath& a_Other) const
{
	int32 matchingNodes = 0;
	int32 compareLength = FMath::Min(m_ContextPath.Num(), a_Other.m_ContextPath.Num());
	for (int32 i = 0; i < compareLength; ++i)
	{
		if (m_ContextPath[i] == a_Other.m_ContextPath[i])
		{
			matchingNodes++;
		}
	}

	return (float)matchingNodes / (float)compareLength;
}

FString PathContextPath::GetPathString() const
{
	FString path;
	for (PathNodeEntry nodeEntry : m_ContextPath)
	{
		path.Append(nodeEntry.m_NodeSignature);
	}
	return path;
}
