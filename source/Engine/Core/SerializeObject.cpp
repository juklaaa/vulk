#include "SerializeObject.h"

#include "Common.h"
#include "Class.h"
#include "Object.h"
#include "Engine/Log.h"

json serializeObjectImpl(const Object* obj, std::unordered_set<const Object*>& serializedObjects);
json serializeField(const Field* f, const Object* obj, json& j, std::unordered_set<const Object*>& serializedObjects);

json serializeObject(const Object* obj)
{
	std::unordered_set<const Object*> serializedObjects;
	return serializeObjectImpl(obj, serializedObjects);
}

json serializeObjectImpl(const Object* obj, std::unordered_set<const Object*>& serializedObjects)
{
	if (!obj)
		return json();
	
	serializedObjects.insert(obj);
	auto clas = obj->getClass();
	assert(clas);
	
	json j;
	j["class"] = clas->className;
	j["address"] = reinterpret_cast<uint64>(obj);
	for (auto f : clas->getAllFields())
	{
		j[f->name] = serializeField(f, obj, j, serializedObjects);
	}
	return j;
}

json serializeField(const Field* f, const Object* obj, json& j, std::unordered_set<const Object*>& serializedObjects)
{
	if (f->isObjectPtr())
	{
		auto anotherObject = f->interpret<Object*>(obj);
		if (!serializedObjects.contains(anotherObject))
		{
			serializedObjects.insert(anotherObject);
			json jObj = serializeObjectImpl(f->interpret<Object*>(obj), serializedObjects);
			return jObj;
		}
		
		return reinterpret_cast<uint64>(anotherObject);
	}
	
	if (f->isVector())
	{
		auto subFields = f->getSubfields(obj);
		std::vector<json> subFieldsJ;
		for (auto& f : subFields)
		{
			subFieldsJ.push_back(serializeField(f.get(), obj, j, serializedObjects));
		}
		return subFieldsJ;
	}
	
	auto value = f->toString(obj);
	return value;
}

template<typename Predicate>
std::vector<json> findJSON(const json& j, const Predicate& predicate)
{
	std::vector<json> result;
	if (predicate(j))
	{
		result.push_back(j);
	}
	
	for (auto& [key, value] : j.items())
	{
		if (value.is_object())
		{
			auto subResult = findJSON(value.get<json>(), predicate);
			result.insert(result.end(), subResult.begin(), subResult.end());
		}
		else if (value.is_array())
		{
			auto jvec = value.get<std::vector<json>>();
			for (auto& subJ : jvec)
			{
				auto subResult = findJSON(subJ, predicate);
				result.insert(result.end(), subResult.begin(), subResult.end());
			}
		}
	}
	
	return result;
}

struct JsonObj
{
	const json* j;
	Object* obj;
};

void deserializeField(Field* f, Object* obj, const json& j, const std::map<uint64, JsonObj>& objectsMap);
Object* deserializeObject(const json& j)
{
	std::map<uint64, JsonObj> objectsMap;
	
	auto allObjectsJ = findJSON(j, [](const json& j) { return j.contains("address"); });
	for (auto& jObj : allObjectsJ)
	{
		uint64 address = jObj.at("address").get<uint64>();
		assert(jObj.contains("class"));
		std::string className = jObj.at("class").get<std::string>();
		auto claz = ClassRegistry::singleton().find(className);
		assert(claz);
		Object* obj = claz->newObject();
		objectsMap[address] = {&jObj, obj};
	}
	
	for (auto& [address, jsonObj] : objectsMap)
	{
		auto jObj = jsonObj.j;
		auto obj = jsonObj.obj;
		for (auto f : obj->getClass()->getAllFields())
		{
			if (!jObj->contains(f->name))
				continue;
			deserializeField(f, obj, jObj->at(f->name), objectsMap);
		}
	}
	
	assert(j.contains("address"));
	auto address = j.at("address").get<uint64>();
	auto itRootObject = objectsMap.find(address);
	assert(itRootObject != objectsMap.end());
	return itRootObject->second.obj;
}
			
void deserializeField(Field* f, Object* obj, const json& j, const std::map<uint64, JsonObj>& objectsMap)
{
	if (f->isObjectPtr())
	{
		uint64 subObjAddress = 0u;
		if (j.is_object())
		{
			subObjAddress = j.at("address").get<uint64>();
		}
		else if (j.is_number_unsigned())
		{
			subObjAddress = j.get<uint64>();
		}
		else if (j.is_null())
		{
			f->interpret<Object*>(obj) = nullptr;
			return;
		}
		else
		{
			assert(false && "Unknown object");
		}
		
		auto it = objectsMap.find(subObjAddress);
		assert(it != objectsMap.end());
		f->interpret<Object*>(obj) = it->second.obj;
		return;
	}
	
	if (f->isVector())
	{
		if (j.is_array())
		{
			auto jvec = j.get<std::vector<json>>();
			auto subfields = f->createSubfields(obj, jvec.size());
			for (int i = 0; i < jvec.size(); ++i)
			{
				deserializeField(subfields[i].get(), obj, jvec[i], objectsMap);
			}
		}
		else
		{
			assert(false && "Expected an array");
		}
		return;
	}
	
	f->fromString(obj, j.get<std::string>());
}
