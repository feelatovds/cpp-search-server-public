#include "request_queue.h"
#include <algorithm>


RequestQueue::RequestQueue(const SearchServer& search_server)
: search_server_(search_server)
, count_query(0)
{
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    std::vector<Document> top_documents = search_server_.FindTopDocuments(raw_query, status);
    UpdateDeque(top_documents);
    return top_documents;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    std::vector<Document> top_documents = search_server_.FindTopDocuments(raw_query);
    UpdateDeque(top_documents);
    return top_documents;
}

int RequestQueue::GetNoResultRequests() const {
    return std::count_if(requests_.begin(), requests_.end(), [](const QueryResult& query_result) {
        return query_result.isEmpty;
    });
}

void RequestQueue::UpdateDeque(const std::vector<Document>& top_document) {
    QueryResult query_result(top_document);
    ++count_query;
    if (count_query > min_in_day_) {
        requests_.pop_front();
    }
    requests_.push_back(query_result);
}
