#pragma once
#include "pch.h"

template<typename... Args>
class MultiCastDeleagate;

class DelegateHandle
{
	template<typename... Args>
		friend class MultiCastDeleagate;

public:
	DelegateHandle() : id(0) {};

	bool operator==(const DelegateHandle& other) const
	{
		return id == other.id;
	}
	bool operator!=(const DelegateHandle& other) const
	{
		return id != other.id;
	}

private:
	// private 생성자로 외부에서 함부로 유효한 핸들 생성 막기
	// explicit로 컴파일 묵시적 형변환 이를 테면 handle = 2 이런것을 방지
	explicit DelegateHandle(uint64 _id) : id(_id) {}

	static DelegateHandle Generate()
	{
		static uint64 counter = 0;
		return DelegateHandle(++counter);
	}
private:
	uint64 id;
};
