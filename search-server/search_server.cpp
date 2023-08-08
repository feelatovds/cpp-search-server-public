#include "search_server.h"
#include <numeric>
#include <cmath>

SearchServer::SearchServer(const std::string& stop_words_text)
        : SearchServer(std::string_view(stop_words_text))
{
}

SearchServer::SearchServer(const std::string_view& stop_words_view)
        : SearchServer(SplitIntoWords(stop_words_view))
{
}

void SearchServer::AddDocument(int document_id,
                               const std::string_view& document,
                               const DocumentStatus& status,
                               const std::vector<int>& ratings) {
    if (document_id < 0 ||
        documents_.count(document_id) > 0) {
        throw std::invalid_argument("Invalid range when adding a document!");
    }
    if (!IsValidWord(document)) {
        throw std::invalid_argument("Invalid document!");
    }
    ids_.emplace(document_id);

    const std::vector<std::string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    std::map<std::string_view, double> word_freq;

    for (const std::string_view& word : std::move(words)) {
        const std::string& word_as_str = static_cast<std::string>(std::move(word));
        words_documents_.insert(word_as_str);
        auto it = words_documents_.find(word_as_str);
        word_to_document_freqs_[*it][document_id] += inv_word_count;
        word_freq[*it] += inv_word_count;
    }

    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_word_freq_.emplace(document_id, std::move(word_freq));
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, const DocumentStatus& status) const {
    return FindTopDocuments(raw_query,
                            [&status](int document_id, const DocumentStatus& document_status, int rating) {
                                return document_status == status;
                            });
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::set<int>::const_iterator SearchServer::begin() const {
    return ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {
    return ids_.end();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view& raw_query,
                                                                                      int document_id) const {
    Query query = ParseQuery(raw_query);
    auto& minus = query.minus_words;
    auto& plus = query.plus_words;

    if (std::any_of(minus.begin(), minus.end(), [&](const std::string_view& word) {
        return word_to_document_freqs_.count(word) != 0 && word_to_document_freqs_.at(word).count(document_id) != 0;
    })
            ) {
        return {std::vector<std::string_view>{}, documents_.at(document_id).status};
    }

    std::vector<std::string_view> matched_words(plus.size());
    auto last = std::copy_if(plus.begin(), plus.end(), matched_words.begin(), [&](const std::string_view& word) {
        return word_to_document_freqs_.count(word) != 0 && word_to_document_freqs_.at(word).count(document_id) != 0;
    });
    matched_words.erase(last, matched_words.end());

    return {std::move(matched_words), documents_.at(document_id).status};
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string_view, double> dummy;
    return document_word_freq_.count(document_id) == 0 ? dummy : document_word_freq_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    if (documents_.count(document_id) == 0) {
        return;
    }

    for (const auto& [word, _] : document_word_freq_.at(document_id)) {
        word_to_document_freqs_.at(word).erase(document_id);
    }

    documents_.erase(document_id);
    document_word_freq_.erase(document_id);
    ids_.erase(document_id);
}

bool SearchServer::IsValidWord(const std::string_view& word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](const char c) {
        return c >= '\0' && c < ' ';
    });
}

bool SearchServer::IsStopWord(const std::string_view& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const {
    auto v = SplitIntoWordsView(text);

    std::vector<std::string_view> words;
    words.reserve(v.size());
    for (const std::string_view& word : std::move(v)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (!IsValidWord(text)) {
        throw std::invalid_argument("Incorrect symbols at query!");
    }
    if (text.empty()) {
        throw std::invalid_argument("There isn't word after \"-\"");
    }
    if (text[0] == '-') {
        throw std::invalid_argument("Two \"-\" before word");
    }
    return {std::move(text), is_minus, std::move(IsStopWord(text))};
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view& text) const {
    Query query;
    auto& minus = query.minus_words;
    auto& plus = query.plus_words;

    auto v = SplitIntoWordsView(text);
    minus.reserve(v.size());
    minus.reserve(v.size());

    for (std::string_view& word : std::move(v)) {
        const QueryWord& query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            query_word.is_minus
            ? minus.push_back(query_word.data)
            : plus.push_back(query_word.data);
        }
    }

    std::sort(minus.begin(), minus.end());
    auto last_minus = std::unique(minus.begin(), minus.end());
    minus.erase(last_minus, minus.end());

    std::sort(plus.begin(), plus.end());
    auto last_plus = std::unique(plus.begin(), plus.end());
    plus.erase(last_plus, plus.end());

    return query;
}

SearchServer::Query SearchServer::ParseQuery(std::execution::parallel_policy policy, const std::string_view& text) const {
    Query query;
    auto& minus = query.minus_words;
    auto& plus = query.plus_words;

    auto v = SplitIntoWordsView(text);
    minus.reserve(v.size());
    minus.reserve(v.size());

    for (std::string_view& word : std::move(v)) {
        const QueryWord& query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            query_word.is_minus
            ? minus.push_back(query_word.data)
            : plus.push_back(query_word.data);
        }
    }

    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy policy,
                                                                                      const std::string_view& raw_query,
                                                                                      int document_id) const {
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy policy,
                                                                                      const std::string_view& raw_query,
                                                                                      int document_id) const {
    Query query = ParseQuery(policy, raw_query);
    auto& minus = query.minus_words;
    auto& plus = query.plus_words;

    if (std::any_of(minus.begin(), minus.end(), [&](const std::string_view& word) {
        return word_to_document_freqs_.count(word) != 0 && word_to_document_freqs_.at(word).count(document_id) != 0;
    })
            ) {
        return {std::vector<std::string_view>{}, documents_.at(document_id).status};
    }

    std::vector<std::string_view> matched_words(plus.size());
    auto last = std::copy_if(std::execution::par, plus.begin(), plus.end(), matched_words.begin(), [&](const std::string_view& word) {
        return word_to_document_freqs_.count(word) != 0 && word_to_document_freqs_.at(word).count(document_id) != 0;
    });
    matched_words.erase(last, matched_words.end());

    std::sort(matched_words.begin(), matched_words.end());
    auto last_plus = std::unique(matched_words.begin(), matched_words.end());
    matched_words.erase(last_plus, matched_words.end());

    return {std::move(matched_words), documents_.at(document_id).status};
}
