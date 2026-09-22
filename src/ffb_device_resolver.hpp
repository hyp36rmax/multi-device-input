#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace FFBDeviceResolver
{
	// A saved interface gets first refusal, followed only by other interfaces
	// with the same product identity. If it is gone, discover a usable wheel
	// from the attached candidates in enumeration order.
	template <typename Device>
	std::vector<size_t> candidate_order(const std::vector<Device>& devices, const std::string& savedId)
	{
		std::vector<size_t> order;
		size_t saved = devices.size();
		for (size_t index = 0; index < devices.size(); ++index)
			if (!savedId.empty() && devices[index].id == savedId)
			{
				saved = index;
				break;
			}
		if (saved == devices.size())
		{
			for (size_t index = 0; index < devices.size(); ++index)
				order.push_back(index);
			return order;
		}
		order.push_back(saved);
		for (size_t index = 0; index < devices.size(); ++index)
			if (index != saved && devices[index].name == devices[saved].name)
				order.push_back(index);
		return order;
	}
}
