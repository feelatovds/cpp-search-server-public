#include "process_queries.h"
#include <execution>
#include <algorithm>

std::vector<std::vector<Document>> ProcessQueries(
        const SearchServer& search_server,
        const std::vector<std::string>& queries) {

    std::vector<std::vector<Document>> documents_lists(queries.size());
    std::transform(std::execution::par,
                   queries.begin(), queries.end(),
                   documents_lists.begin(),
                   [&search_server](const auto& query) {return search_server.FindTopDocuments(query); }
    );
    return documents_lists;
}

std::vector<Document> ProcessQueriesJoined(
        const SearchServer& search_server,
        const std::vector<std::string>& queries) {

    std::vector<Document> result;
    for (const auto& documents : ProcessQueries(search_server, queries)) {
        for (const auto& document : documents) {
            result.push_back(std::move(document));
        }
    }
    return result;
}
