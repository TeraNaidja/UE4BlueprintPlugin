#include "BIPluginPrivatePCH.h"
#include "StackTimer.h"

StackTimer::StackTimer(const FString& a_Name)
	: m_Name(a_Name)
	, m_StartCycles(FPlatformTime::Cycles())
{
}

void StackTimer::LogElapsedMs() const
{
	UE_LOG(BILog, BI_VERBOSE, TEXT("%s elapsed: %.2f"), *m_Name, FPlatformTime::ToMilliseconds(FPlatformTime::Cycles() - m_StartCycles))
}