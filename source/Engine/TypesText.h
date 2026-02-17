#pragma once
#include <format>

#include "Math/Math.h"

template <>
struct std::formatter<V4> : std::formatter<std::string> {
	auto format(V4 v, format_context& ctx) const {
		return formatter<string>::format(
		  std::format("[{:.2f} {:.2f} {:.2f} {:.2f}]", v.x, v.y, v.z, v.w), ctx);
	}
};

template <>
struct std::formatter<Quat> : std::formatter<std::string> {
	auto format(Quat q, format_context& ctx) const {
		return formatter<string>::format(
		  std::format("[{:.2f} {:.2f} {:.2f} {:.2f}]", q.x, q.y, q.z, q.w), ctx);
	}
};