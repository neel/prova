// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#include "prova/string_indexer.h"
#include <stdexcept>

std::size_t prova::string_indexer::add(std::string&& key) {
		key_hash_container::const_iterator inserted;
    if (exists(key)) {
        inserted = _keys.find(std::move(key));
    } else {
				auto pair = _keys.insert({std::move(key), _keys.size()});
				inserted = pair.first;
		}
    _reversed_keys[inserted->second] = inserted;
    return inserted->second;
}
std::size_t prova::string_indexer::add(const std::string& key){
		key_hash_container::const_iterator inserted;
    if (exists(key)) {
        inserted = _keys.find(key);
    } else {
				auto pair = _keys.insert({key, _keys.size()});
				inserted = pair.first;
		}
    _reversed_keys[inserted->second] = inserted;
    return inserted->second;
}

std::size_t prova::string_indexer::operator[](const std::string& key){
	if(exists(key)){
		return _keys.at(key);
	} else {
		return add(key);
	}
}

std::size_t prova::string_indexer::operator[](std::string&& key){
	if(exists(std::forward<std::string>(key))){
		return _keys.at(std::forward<std::string>(key));
	} else {
		return add(std::forward<std::string>(key));
	}
}

std::size_t prova::string_indexer::operator[](std::string && key) const{
	if(!exists(std::forward<std::string>(key))){
		return _keys.at(std::forward<std::string>(key));
	} else {
		throw std::runtime_error("Key does not exist");
	}
}

std::size_t prova::string_indexer::size() const{
	return _keys.size();
}

const std::string& prova::string_indexer::resolve(std::size_t hash) const {
    auto it = _reversed_keys.find(hash);
    if (it == _reversed_keys.end()) {
        throw std::runtime_error("Hash does not exist");
    }
    return it->second->first;
}

const std::string& prova::string_indexer::operator[](std::size_t hash) const{
	return resolve(hash);
}


bool prova::string_indexer::exists(const std::string& key) const {
    return _keys.find(key) != _keys.end();
}

bool prova::string_indexer::exists(std::size_t hash) const {
    return _reversed_keys.find(hash) != _reversed_keys.end();
}
