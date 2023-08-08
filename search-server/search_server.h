#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <execution>

#include "document.h"
#include "string_processing.h"
#include "log_duration.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double MIN_RELEVANCE_DIFFERENCE = 1e-6;
const int NUMBER_THREADS = 12;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string& stop_words_text);
    explicit SearchServer(const std::string_view& stop_words_view);

    void AddDocument(int document_id, const std::string_view& document, const DocumentStatus& status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, const DocumentStatus& status = DocumentStatus::ACTUAL) const;

    template <typename DocumentPredicate, class ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query, DocumentPredicate document_predicate) const;
    template <class ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query, const DocumentStatus& status = DocumentStatus::ACTUAL) const;

    int GetDocumentCount() const;
    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view& raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy, const std::string_view& raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy, const std::string_view& raw_query, int document_id) const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    template<class ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id);
    void RemoveDocument(int document_id);

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    std::set<std::string, std::less<>> stop_words_;
    std::set<std::string, std::less<>> words_documents_;

    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::map<int, std::map<std::string_view, double>> document_word_freq_;
    std::set<int> ids_;

    static bool IsValidWord(const std::string_view& word);
    bool IsStopWord(const std::string_view& word) const;
    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view& text) const;
    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };
    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };
    Query ParseQuery(const std::string_view& text) const;
    Query ParseQuery(std::execution::parallel_policy, const std::string_view& text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string_view& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate, class ExecutionPolicy>
    std::vector<Document> FindAllDocuments(ExecutionPolicy&& policy, const Query& query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words) {
    stop_words_ = MakeUniqueNonEmptyStrings(stop_words);
    if (std::any_of(stop_words_.begin(), stop_words_.end(), [](const std::string_view& word) {
        return !IsValidWord(word);
    })) {
        throw std::invalid_argument("Invalid stop word!");
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query,
                                                     DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename DocumentPredicate, class ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy,
                                                     const std::string_view& raw_query,
                                                     DocumentPredicate document_predicate) const {
    auto matched_documents = FindAllDocuments(policy, ParseQuery(raw_query), document_predicate);
    std::sort(matched_documents.begin(), matched_documents.end(),
              [](const Document& lhs, const Document& rhs) {
                  return std::abs(lhs.relevance - rhs.relevance) < MIN_RELEVANCE_DIFFERENCE ? lhs.rating > rhs.rating : lhs.relevance > rhs.relevance;
              });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <class ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy,
                                       const std::string_view& raw_query,
                                       const DocumentStatus& status) const {
    return FindTopDocuments(policy, raw_query,
                            [&status](int document_id, const DocumentStatus& document_status, int rating) {
                                return document_status == status;
                            });
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * ComputeWordInverseDocumentFreq(word);
            }
        }
    }

    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    matched_documents.reserve(document_to_relevance.size());
    for (const auto& [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;

}

template <typename DocumentPredicate, class ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy&& policy, const Query& query, DocumentPredicate document_predicate) const {
    if (std::is_same_v<std::decay_t<ExecutionPolicy>, std::decay_t<std::execution::sequenced_policy> >) {
        return FindAllDocuments(query, document_predicate);
    }

    ConcurrentMap<int, double> document_to_relevance(NUMBER_THREADS);
    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [&](const std::string_view& word) {
        if (word_to_document_freqs_.count(word) == 0) {
            return;
        }

        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id].ref_to_value += term_freq * ComputeWordInverseDocumentFreq(word);
            }
        }
    });
    std::for_each(policy, query.minus_words.begin(), query.minus_words.end(), [&](const std::string_view& word) {
        if (word_to_document_freqs_.count(word) == 0) {
            return;
        }

        for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.Erase(document_id);
        }
    });

    std::vector<Document> matched_documents;
    matched_documents.reserve(document_to_relevance.GetSize());
    for (const auto& [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

template<class ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id) {
    if (documents_.count(document_id) == 0) {
        return;
    }

    std::vector<std::string_view> words;
    words.reserve(document_word_freq_.at(document_id).size());
    for (const auto& [word, _] : document_word_freq_.at(document_id)) {
        words.push_back(word);
    }


    std::for_each(std::execution::par, policy, words.begin(), words.end(), [&](const auto it) {
        word_to_document_freqs_.at(*it).erase(document_id);
    });

    documents_.erase(document_id);
    document_word_freq_.erase(document_id);
    ids_.erase(document_id);
}
