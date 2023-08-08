#include "test_example_functions.h"
#include "search_server.h"

#include <tuple>

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

void TestAddDocument() {
    SearchServer search_server("и в на"s);

    //search_server.AddDocument(0, "и в на"s, DocumentStatus::ACTUAL, {8, 8});
    //assert(search_server.GetDocumentCount() == 0);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, 8});
    search_server.AddDocument(1, "кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, 8});
    search_server.AddDocument(2, "черная собака пушистый хвост белый ошейник"s, DocumentStatus::ACTUAL, {8, 8});
    ASSERT_EQUAL(search_server.GetDocumentCount(), 3);
    search_server.AddDocument(3, "белый кот и модный ошейник"s, DocumentStatus::BANNED, {8, 8});
    search_server.AddDocument(4, "черная собака пушистый хвост"s, DocumentStatus::IRRELEVANT, {8, 8});
    ASSERT_EQUAL(search_server.GetDocumentCount(), 5);

    string query_1 = "кот"s;
    vector<Document> documents_1 = search_server.FindTopDocuments(query_1, DocumentStatus::ACTUAL);
    set<int> id_1;
    for (const auto document : documents_1) id_1.insert(document.id);
    set<int> id_ans_1 = {0, 1};
    ASSERT_EQUAL(id_1, id_ans_1);
    documents_1.clear();
    id_1.clear();
    id_ans_1.clear();

    documents_1 = search_server.FindTopDocuments(query_1, DocumentStatus::BANNED);
    for (const auto document : documents_1) id_1.insert(document.id);
    id_ans_1 = {3};
    ASSERT_EQUAL(id_1, id_ans_1);
    documents_1.clear();
    id_1.clear();
    id_ans_1.clear();

    documents_1 = search_server.FindTopDocuments(query_1, DocumentStatus::IRRELEVANT);
    for (const auto document : documents_1) id_1.insert(document.id);
    ASSERT(documents_1.empty());
    query_1.clear();

    string query_2 = "и в на"s;
    vector<Document> documents_2 = search_server.FindTopDocuments(query_2, DocumentStatus::ACTUAL);
    ASSERT(documents_2.empty());
    query_2.clear();

    string query_3 = "-кот модный ошейник"s;
    vector<Document> documents_3 = search_server.FindTopDocuments(query_3, DocumentStatus::ACTUAL);
    set<int> id_3;
    for (const auto document : documents_3) id_3.insert(document.id);
    set<int> id_ans_3 = {2};
    ASSERT_EQUAL(id_3, id_ans_3);
    query_3.clear();
    documents_3.clear();
    id_3.clear();
    id_ans_3.clear();

    string query_4 = "черный кот -собака"s;
    vector<Document> documents_4 = search_server.FindTopDocuments(query_4, DocumentStatus::BANNED);
    set<int> id_4;
    for (const auto document : documents_4) id_4.insert(document.id);
    set<int> id_ans_4 = {3};
    ASSERT_EQUAL(id_4, id_ans_4);
    query_4.clear();
    documents_4.clear();
    id_4.clear();
    id_ans_4.clear();
}

void TestStopWords() {
    SearchServer search_server("и в на"s);

    //search_server.AddDocument(0, "и в на"s, DocumentStatus::ACTUAL, {8, 8});
    //assert(search_server.GetDocumentCount() == 0);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, 8});
    string query_1 = "собака и кот"s;
    vector<Document> documents_1 = search_server.FindTopDocuments(query_1, DocumentStatus::ACTUAL);
    set<int> id_1;
    for (const auto document : documents_1) id_1.insert(document.id);
    set<int> id_ans_1 = {0};
    ASSERT_EQUAL(id_1, id_ans_1);
    query_1.clear();
    documents_1.clear();
    id_1.clear();
    id_ans_1.clear();

    query_1 = "и";
    documents_1 = search_server.FindTopDocuments(query_1, DocumentStatus::ACTUAL);
    ASSERT(documents_1.empty());
    query_1.clear();
}

void TestMinusWords() {
    SearchServer search_server("и в на с"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, 8});
    search_server.AddDocument(1, "разноцветный попугай Кеша"s, DocumentStatus::ACTUAL, {8, 8});
    search_server.AddDocument(2, "черная собака пушистый хвост белый ошейник"s, DocumentStatus::ACTUAL, {8, 8});
    search_server.AddDocument(3, "зеленый крокодил ищет друзей"s, DocumentStatus::ACTUAL, {8, 8});
    search_server.AddDocument(4, "непонятное животное в коробке с апельсинами"s, DocumentStatus::ACTUAL, {8, 8});

    const string query_1 = "белый кот попугай Кеша пушистый хвост зеленый крокодил животное в коробке";
    vector<Document> documents_1 = search_server.FindTopDocuments(query_1, DocumentStatus::ACTUAL);
    set<int> id_1;
    for (const auto document : documents_1) id_1.insert(document.id);
    set<int> id_ans_1 = {0, 1, 2, 3, 4};
    ASSERT_EQUAL(id_1, id_ans_1);

    const string query_2 = "-белый кот попугай Кеша пушистый хвост зеленый крокодил животное в коробке";
    vector<Document> documents_2 = search_server.FindTopDocuments(query_2, DocumentStatus::ACTUAL);
    set<int> id_2;
    for (const auto document : documents_2) id_2.insert(document.id);
    set<int> id_ans_2 = {1, 3, 4};
    ASSERT_EQUAL(id_2, id_ans_2);

    const string query_3 = "-белый кот -попугай Кеша пушистый хвост зеленый крокодил животное в коробке";
    vector<Document> documents_3 = search_server.FindTopDocuments(query_3, DocumentStatus::ACTUAL);
    set<int> id_3;
    for (const auto document : documents_3) id_3.insert(document.id);
    set<int> id_ans_3 = {3, 4};
    ASSERT_EQUAL(id_3, id_ans_3);
}

void TestMatching() {
    SearchServer search_server("и в на с"s);
    search_server.AddDocument(0, "непонятное животное в коробке с апельсинами"s, DocumentStatus::ACTUAL, {8, 8});
    search_server.AddDocument(1, "черная собака пушистый хвост белый ошейник"s, DocumentStatus::IRRELEVANT, {8, 8});

    const string query_1 = "непонятное животное в коробке с апельсинами"s;
    tuple<vector<string_view>, DocumentStatus> match_1 = search_server.MatchDocument(query_1, 0);
    const vector<string> words_1 = {"апельсинами"s, "животное"s, "коробке"s, "непонятное"s};
    vector<string_view> words_1_view;
    for (const string_view word : words_1) { words_1_view.push_back(word);}
    tuple<vector<string_view>, DocumentStatus> answer_1 = {words_1_view, DocumentStatus::ACTUAL};
    ASSERT(match_1 == answer_1);

    const string query_2 = "черная собака белый ошейник"s;
    tuple<vector<string_view>, DocumentStatus> match_2 = search_server.MatchDocument(query_2, 1);
    const vector<string> words_2 = {"белый"s, "ошейник"s, "собака"s, "черная"s};
    vector<string_view> words_2_view;
    for (const string_view word : words_2) { words_2_view.push_back(word);}
    tuple<vector<string_view>, DocumentStatus> answer_2 = {words_2_view, DocumentStatus::IRRELEVANT};
    ASSERT(match_2 == answer_2);

    const string query_3 = "черная собака -белый ошейник"s;
    tuple<vector<string_view>, DocumentStatus> match_3 = search_server.MatchDocument(query_3, 1);
    ASSERT(get<0>(match_3).empty());
}

void TestSortRelevance() {
    SearchServer search_server("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});

    vector<double> relevance;
    for (const Document& document : search_server.FindTopDocuments("ухоженный кот"s)) {
        relevance.push_back(document.relevance);
    }

    ASSERT(relevance[0] >= relevance[1] && relevance[1] >= relevance[2]);
}

void TestCalcRating() {
    SearchServer search_server("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, 3});
    search_server.AddDocument(1, "чебурашка в коробке"s, DocumentStatus::ACTUAL, {-7, -2, -2});
    vector<int> vec;
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, vec);

    vector<Document> document_1 = search_server.FindTopDocuments("кот"s, DocumentStatus::ACTUAL);
    ASSERT_EQUAL(document_1[0].rating, 5);

    vector<Document> document_2 = search_server.FindTopDocuments("чебурашка"s, DocumentStatus::ACTUAL);
    ASSERT_EQUAL(document_2[0].rating, -3);

    vector<Document> document_3 = search_server.FindTopDocuments("пёс"s, DocumentStatus::ACTUAL);
    ASSERT_EQUAL(document_3[0].rating, 0);
}

void TestFilter() {
    SearchServer search_server("и в на с"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {1, 1});
    search_server.AddDocument(1, "непонятное животное в коробке с апельсинами"s, DocumentStatus::ACTUAL, {8, 8});
    search_server.AddDocument(2, "белый кот и модный ошейник"s, DocumentStatus::IRRELEVANT, {2, 2});
    search_server.AddDocument(3, "белый кот и модный ошейник"s, DocumentStatus::BANNED, {3, 3});
    search_server.AddDocument(4, "белый кот и модный ошейник"s, DocumentStatus::REMOVED, {4, 4});

    const string query_1 = "кот животное"s;
    set<int> id_1_1;
    vector<Document> documents_1_1 = search_server.FindTopDocuments(query_1, DocumentStatus::ACTUAL);
    for (const auto document : documents_1_1) id_1_1.insert(document.id);
    set<int> answer_1_1 = {0, 1};
    ASSERT_EQUAL(id_1_1, answer_1_1);

    vector<Document> documents_1_2 =
            search_server.FindTopDocuments(query_1, [](int id, DocumentStatus status, int rating) {
                if (id == 0) return true;
                else return false;
            });
    set<int> id_1_2;
    for (const auto document : documents_1_2) id_1_2.insert(document.id);
    set<int> answer_1_2 = {0};
    ASSERT_EQUAL(id_1_2, answer_1_2);

    const string query_2 = "кот"s;
    vector<Document> documents_2 =
            search_server.FindTopDocuments(query_2, [](int id, DocumentStatus status, int rating) {
                if (status == DocumentStatus::BANNED) return true;
                else return false;
            });
    set<int> id_2;
    for (const auto document : documents_2) id_2.insert(document.id);
    set<int> answer_2 = {3};
    ASSERT_EQUAL(id_2, answer_2);

    const string query_3 = "кот"s;
    vector<Document> documents_3 =
            search_server.FindTopDocuments(query_3, [](int id, DocumentStatus status, int rating) {
                if (rating > 0) return true;
                else return false;
            });
    set<int> id_3;
    for (const auto document : documents_3) id_3.insert(document.id);
    set<int> answer_3 = {0, 2, 3, 4};
    ASSERT_EQUAL(id_3, answer_3);

    const string query_4 = "кот"s;
    vector<Document> documents_4 =
            search_server.FindTopDocuments(query_3, [](int id, DocumentStatus status, int rating) {
                if (rating > 1) return true;
                else return false;
            });
    set<int> id_4;
    for (const auto document : documents_4) id_4.insert(document.id);
    set<int> answer_4 = {2, 3, 4};
    ASSERT_EQUAL(id_4, answer_4);
}

void TestStatus() {
    SearchServer search_server("и в на с"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {1, 1});
    search_server.AddDocument(1, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {2, 2});
    search_server.AddDocument(2, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {3, 3});
    search_server.AddDocument(3, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {4, 4});
    search_server.AddDocument(4, "белый кот и модный ошейник"s, DocumentStatus::IRRELEVANT, {1, 1});
    search_server.AddDocument(5, "белый кот и модный ошейник"s, DocumentStatus::IRRELEVANT, {2, 2});
    search_server.AddDocument(6, "белый кот и модный ошейник"s, DocumentStatus::IRRELEVANT, {3, 3});
    search_server.AddDocument(7, "белый кот и модный ошейник"s, DocumentStatus::IRRELEVANT, {4, 4});

    const string query = "кот"s;
    vector<Document> documents_1 = search_server.FindTopDocuments(query, DocumentStatus::ACTUAL);
    set<int> id_1;
    for (const auto document : documents_1) id_1.insert(document.id);
    set<int> id_ans_1 = {0, 1, 2, 3};
    ASSERT_EQUAL(id_1, id_ans_1);

    vector<Document> documents_2 = search_server.FindTopDocuments(query, DocumentStatus::IRRELEVANT);
    set<int> id_2;
    for (const auto document : documents_2) id_2.insert(document.id);
    set<int> id_ans_2 = {4, 5, 6, 7};
    ASSERT_EQUAL(id_2, id_ans_2);
}

void TestRelevance() {
    SearchServer search_server("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});

    vector<double> relevance;
    for (const Document& document : search_server.FindTopDocuments("ухоженный кот"s)) {
        relevance.push_back(document.relevance);
    }
    const double EPS = 1e-6;
    ASSERT(abs(relevance[0] - 0.274653) < EPS);
    ASSERT(abs(relevance[1] - 0.101366) < EPS);
    ASSERT(abs(relevance[2] - 0.101366) < EPS);
}

void TestDontChangeQuery() { // test, что ParseQuery не изменяет запрос пользователя
    SearchServer search_server("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});

    const std::string query_source = "ухоженный кот"s;
    std::string query = query_source; // копируем
    search_server.FindTopDocuments(query);

    ASSERT(query_source == query);

}

void TestSearchServer() {
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestStopWords);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatching);
    RUN_TEST(TestSortRelevance);
    RUN_TEST(TestCalcRating);
    RUN_TEST(TestFilter);
    RUN_TEST(TestStatus);
    RUN_TEST(TestRelevance);
    RUN_TEST(TestDontChangeQuery);
}
