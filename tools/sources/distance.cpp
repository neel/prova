// SPDX-FileCopyrightText: 2025 Sunanda Bose <sunanda@simula.no>
// SPDX-License-Identifier: BSD-3-Clause

#include "prova/distance.h"
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include "prova/action.h"
#include "prova/artifact.h"
#include "prova/session.h"

int levenshteinDist(std::string word1, std::string word2) {
    int size1 = word1.size();
    int size2 = word2.size();

		if (size1 == 0)
        return size2;
    if (size2 == 0)
        return size1;

    std::vector<std::vector<int>> verif; // Verification matrix i.e. 2D array which will store the calculated distance.

    verif.resize(size1 + 1);
		for(auto& v: verif){
			v.resize(size2 + 1);
		}

    // Sets the first row and the first column of the verification matrix with the numerical order from 0 to the length of each word.
    for (int i = 0; i <= size1; i++)
        verif[i][0] = i;
    for (int j = 0; j <= size2; j++)
        verif[0][j] = j;

    // Verification step / matrix filling.
    for (int i = 1; i <= size1; i++) {
        for (int j = 1; j <= size2; j++) {
            // Sets the modification cost.
            // 0 means no modification (i.e. equal letters) and 1 means that a modification is needed (i.e. unequal letters).
            int cost = (word2[j - 1] == word1[i - 1]) ? 0 : 1;

            // Sets the current position of the matrix as the minimum value between a (deletion), b (insertion) and c (substitution).
            // a = the upper adjacent value plus 1: verif[i - 1][j] + 1
            // b = the left adjacent value plus 1: verif[i][j - 1] + 1
            // c = the upper left adjacent value plus the modification cost: verif[i - 1][j - 1] + cost
            verif[i][j] = std::min(
                std::min(verif[i - 1][j] + 1, verif[i][j - 1] + 1),
                verif[i - 1][j - 1] + cost
            );
        }
    }

    // The last position of the matrix will contain the Levenshtein distance.
    return verif[size1][size2];
}

template <typename T>
std::pair<double, double> _distance(const std::set<std::string>& categorical, const std::set<std::string>& ignored, const std::set<std::string>& numerical, const T& l, const T& r){
		std::set<std::string> lkeys, rkeys, ckeys;

	for(const auto& [key, val]: l.properties().items()){
		if (ignored.find(key) == ignored.end())  lkeys.insert(key);
	}
	for(const auto& [key, val]: r.properties().items()){
		if (ignored.find(key) == ignored.end())  rkeys.insert(key);
	}
	std::set_intersection(
		lkeys.begin(), lkeys.end(),
		rkeys.begin(), rkeys.end(),
		std::inserter(ckeys, ckeys.begin())
	);

	double delta = 0.0f;
	double matched_keys = 0.0f;
	// Calculate distances for each common key, taking into account categorical values
	for (const auto& key : ckeys) {
		if (categorical.find(key) != categorical.end()) {
			if (l.properties().at(key) == r.properties().at(key)) {
				matched_keys += 1;
			} else {
				// Didn't find an exact match for the categorical key, but found presence of the key
				matched_keys += 0.5;
			}
		} else {
			matched_keys += 1;
			if (numerical.find(key) != numerical.end()) {
				// For numerical values, use the absolute difference or other appropriate metric
				delta += std::abs(l.properties().at(key).template get<double>() - r.properties().at(key).template get<double>());
			} else {
				delta += levenshteinDist(l.properties().at(key).template get<std::string>(), r.properties().at(key).template get<std::string>());
			}
		}
	}

	return std::make_pair(delta, matched_keys);
}

double prova::distance(const prova::action& l, const prova::action& r){
	std::set<std::string> categorical = { "operation", "source", "flags", "mode" };
	std::set<std::string> ignored 		= { "id" };
	std::set<std::string> numerical 	= { };

	auto dist = _distance(categorical, ignored, numerical, l, r);

	prova::action::time_type ltime = l.time(), rtime = r.time();
	double delta_time = std::abs((ltime - rtime).count());
	dist.first += delta_time;

	std::string loperation = l.operation(), roperation = r.operation();
	if(loperation == roperation){
		dist.second += 1;
	}

	double total_keys = ((l.properties().size()+2) + (r.properties().size()+2));
	double dice_coeffecient = (2*dist.second)/total_keys;

	return (1-dice_coeffecient) * dist.first;
}

double prova::distance(const prova::artifact& l, const prova::artifact& r){
	std::set<std::string> categorical = { "permissions", "source", "type" };
	std::set<std::string> ignored 		= { };
	std::set<std::string> numerical 	= { };

	auto dist = _distance(categorical, ignored, numerical, l, r);

	std::string lsubtype = l.subtype(), rsubtype = r.subtype();
	if(lsubtype == rsubtype){
		dist.second += 1;
	}

	double total_keys = ((l.properties().size()+1) + (r.properties().size()+1));
	double dice_coeffecient = (2*dist.second)/total_keys;

	return (1-dice_coeffecient) * dist.first;
}


double prova::distance(const prova::session& l, const prova::session& r){

}

