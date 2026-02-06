#pragma once
#include "third_party/json.hpp"
using json = nlohmann::json;

class Object;

json serializeObject(const Object* obj);
Object* deserializeObject(const json& json);