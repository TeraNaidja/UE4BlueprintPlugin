#pragma once

#include "PathNodeEntry.h"

class PathContextPath
{
public:
	PathContextPath();
	PathContextPath(const PathContextPath& a_Other);
	~PathContextPath();

	bool operator == (const PathContextPath& a_Other) const;

	void PushNode(const PathNodeEntry& a_Node);
	float CompareContext(const PathContextPath& a_Other) const;
	FString GetPathString() const;
private:
	TArray<PathNodeEntry> m_ContextPath;
};