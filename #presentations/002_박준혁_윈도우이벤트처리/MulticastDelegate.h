#pragma once
#include "DelegateHandle.h"

template<typename... Args>
class MultiCastDeleagate
{
public:
	DelegateHandle Add(std::function<void(Args...)> callback)
	{
		DelegateHandle handle = DelegateHandle::Generate();
		listeners.emplace_back(handle, std::move(callback));
		return handle;
	}

	void Remove(const DelegateHandle& handle)
	{
		for (size_t i = 0; i < listeners.size(); ++i)
		{
			if (listeners[i].first == handle)
			{
				listeners[i] = std::move(listeners.back());
				listeners.pop_back();
				return true;
			}
		}
		return false;
	}

	// 타입을 펼쳐라
	void BroadCast(Args... args) const
	{
		for (const auto& [handle, callback] : listeners)
		{
			// 값을 펼쳐라
			callback(args...);
		}
	}

	void RemoveAll()
	{
		listeners.clear();
	}

private:
	std::vector<std::pair<DelegateHandle, std::function<void(Args...)>>> listeners;
};