#pragma once

#include <iostream>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin_page, Iterator end_page)
    : begin_page_(begin_page)
    , end_page_(end_page)
    , size_page_(std::distance(begin_page_, end_page_))
    {
    }

    Iterator begin() const {
        return begin_page_;
    }

    Iterator end() const {
        return end_page_;
    }

    int size() const {
        return size_page_;
    }

private:
    Iterator begin_page_;
    Iterator end_page_;
    size_t size_page_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange <Iterator>& it_range) {
    for (auto it = it_range.begin(); it != it_range.end(); ++it) {
        out << *it;
    }
    return out;
}

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator first, Iterator last, size_t size_page)
    : first_(first)
    , last_(last)
    , size_page_(size_page)
    {
        FillPages();
    }

    auto begin() const {
        return iterator_range_.begin();
    }

    auto end() const {
        return iterator_range_.end();
    }

    int size() const {
        return std::distance(begin(), end());
    }

private:
    std::vector<IteratorRange <Iterator>> iterator_range_;
    Iterator first_;
    Iterator last_;
    size_t size_page_;

    void FillPage(Iterator begin_page, Iterator end_page) {
        IteratorRange page(begin_page, end_page);
        iterator_range_.push_back(page);
    }

    void FillPages() {
        size_t count_pages = std::distance(first_, last_) / size_page_;
        bool last_full = true;
        if (count_pages * size_page_ != std::distance(first_, last_)) {
            last_full = false;
        }
        Iterator begin_page = first_;
        Iterator end_page = begin_page;
        std::advance(end_page, size_page_);
        size_t page = 0;
        while (page < count_pages) {
            FillPage(begin_page, end_page);
            begin_page = end_page;
            std::advance(end_page, size_page_);
            ++page;
        }

        if (!last_full) {
            FillPage(begin_page, last_);
        }
    }
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}