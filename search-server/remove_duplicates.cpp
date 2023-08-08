#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::map<std::set<std::string_view>, std::set<int>> document_duplicator;
    std::set<int> id_duplicates;
    for (const int id : search_server) {
        std::set<std::string_view> words;
        for (const auto& [word, _] : search_server.GetWordFrequencies(id)) {
            words.insert(word);
        }

        if (document_duplicator.count(words) != 0) {
            id_duplicates.insert(id);
            document_duplicator.at(words).insert(id);
        } else {
            document_duplicator[words].insert(id);
        }

    }

    for (const int document_id : id_duplicates) {
        std::cout << "Found duplicate document id " << document_id << std::endl;
        search_server.RemoveDocument(document_id);
    }
}
