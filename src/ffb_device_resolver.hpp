#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace FFBDeviceResolver
{
	// A saved interface gets first refusal, followed by same-name siblings.
	// If they fail, try the remaining attached candidates rather than leaving
	// the player without FFB while another usable wheel is present.
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
		for (size_t index = 0; index < devices.size(); ++index)
			if (index != saved && devices[index].name != devices[saved].name)
				order.push_back(index);
		return order;
	}

	// A temporarily absent preferred endpoint must not be replaced by the first
	// device that happens to enumerate on this launch.
	template <typename Device>
	bool should_persist_choice(const std::vector<Device>& devices,
		const std::string& savedId, const std::string& chosenId)
	{
		if (savedId.empty()) return true;
		for (const auto& device : devices)
			if (device.id == savedId)
				return chosenId != savedId;
		return false;
	}
}
