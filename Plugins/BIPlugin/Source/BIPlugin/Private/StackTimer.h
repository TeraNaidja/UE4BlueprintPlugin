#pragma once

class StackTimer
{
public:
	StackTimer(const FString& a_Name); 

	void LogElapsedMs() const;

private:
	uint32 m_StartCycles;
	FString m_Name;
};
