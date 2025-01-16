// SPDX-FileCopyrightText: 2024 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef PROVA_STRING_INDEXER_H
#define PROVA_STRING_INDEXER_H

#include <map>
#include <cstdint>
#include <string>

namespace prova{

struct string_indexer {
	std::size_t add(std::string&& key);
	std::size_t add(const std::string& key);
	const std::string& resolve(std::size_t hash) const;
	bool exists(const std::string& key) const;
	bool exists(std::size_t hash) const;

	std::size_t operator[](const std::string& key);
	std::size_t operator[](std::string&& key);
	std::size_t operator[](std::string&& key) const;
	const std::string& operator[](std::size_t hash) const;

	std::size_t size() const;

  private:
    using key_hash_container 				= std::map<std::string, std::uint32_t>;
    using key_resolution_container 	= std::map<std::uint32_t, key_hash_container::const_iterator>;

    key_hash_container _keys;
    key_resolution_container _reversed_keys;
};

}

#endif // PROVA_STRING_INDEXER_H
