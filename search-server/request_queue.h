#pragma once

#include "search_server.h"
#include "document.h"
#include <string>
#include <vector>
#include <deque>


class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;

private:
    struct QueryResult {
        std::vector<Document> top_matched_document;
        bool isEmpty = true;

        QueryResult(const std::vector<Document>& top_document) {
            top_matched_document = top_document;
            isEmpty = top_document.empty();
        }
    };

    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    int count_query;

    void UpdateDeque(const std::vector<Document>& top_document);
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    std::vector<Document> top_documents = search_server_.FindTopDocuments(raw_query, document_predicate);
    UpdateDeque(top_documents);
    return top_documents;
}
