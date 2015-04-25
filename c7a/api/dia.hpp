/*******************************************************************************
 * c7a/api/dia.hpp
 *
 * Model real-time or backtesting Portfolio with Positions, TradeLog and more.
 ******************************************************************************/

#ifndef C7A_API_DIA_HEADER
#define C7A_API_DIA_HEADER

#include <functional>
#include <vector>
#include <stack>
#include <iostream>
#include <fstream>
#include <cassert>
#include <memory>
#include <unordered_map>
#include <string>

#include "dia_node.hpp"
#include "function_traits.hpp"
#include "lop_node.hpp"
#include "reduce_node.hpp"

namespace c7a {

template <typename T>
class DIA : public std::shared_ptr< DIANode<T> > {
friend class Context;
public:
    // DIA(const std::vector<T>& init_data, DIANode<T> node) : data_(init_data), my_node_(node) { }

    // DIA(const DIA& other) : data_(other.data_) { }

    // explicit DIA(const std::vector<T>& init_data) : data_(init_data) { }

    // explicit DIA(DIA&& other) : DIA() {
    //     swap(*this, other);
    // }

    typedef std::shared_ptr< DIANode<T> > Super;

    using Super::get;

protected:
    //! Protected constructor used by Node generator functions to create graph
    //! nodes.
    explicit DIA(DIANode<T>* node)
        : Super(node)
    {
    }

public:
    //! TODO: remove this, this create the initial DIA node.
    static DIA<T> BigBang()
    {
        return DIA<T>(NULL);
    }

    struct Identity {
        template <typename U>
        constexpr auto operator () (U&& v) const noexcept
        ->decltype(std::forward<U>(v))
        {
            return std::forward<U>(v);
        }
    };

    friend void swap(DIA& first, DIA& second) {
        using std::swap;
        swap(first.data_, second.data_);
    }

    DIA& operator = (DIA rhs) {
        swap(*this, rhs);
        return *this;
    }

    //! Map each element of the current DIA to a list of new elements.
    //! The list for an element can be empty.
    //
    //! \param flatmap_fn Maps a single element of type T to a list of elements of type U.
    //
    //! \return Resulting DIA containing lists of elements of type U.
    template <typename flatmap_fn_t>
    auto FlatMap(const flatmap_fn_t &flatmap_fn) {
        // Create new LOpNode with flatmap_fn
        // Compose functions
        // Link Parent(s)
        // Link Child(s)
        // Return new DIARef
        static_assert(FunctionTraits<flatmap_fn_t>::arity == 2, "error");
        //using flatmap_arg_t
        //          = typename FunctionTraits<flatmap_fn_t>::template arg<0>;
        using emit_fn_t
                  = typename FunctionTraits<flatmap_fn_t>::template arg<1>;
        using emit_arg_t
                  = typename FunctionTraits<emit_fn_t>::template arg<0>;

        using LOpResultNode
            = LOpNode<emit_arg_t,flatmap_fn_t>;

        auto child_node = new LOpResultNode({ get() }, flatmap_fn);
        auto child = DIA<T>(child_node);
        get()->add_child(child_node);

        return child;
    }

    //! Hash elements of the current DIA onto buckets and reduce each bucket to
    //! a single value.
    //
    //! \param key_extr Hash function to get a key for each element.
    //! \param reduce_fn Reduces the elements (of type T) of each bucket to a single value of type U.
    //
    //! \return Resulting DIA containing elements of type U.
    template<typename key_extr_fn_t, typename reduce_fn_t>
    DIA<T> Reduce(const key_extr_fn_t& key_extr, const reduce_fn_t& reduce_fn) {
        static_assert(FunctionTraits<key_extr_fn_t>::arity == 1, "error");
        static_assert(FunctionTraits<reduce_fn_t>::arity == 2, "error");

        //using key_t = typename FunctionTraits<key_extr_fn_t>::result_type;

        using ReduceResultNode
            = ReduceNode<T,key_extr_fn_t,reduce_fn_t>;
        auto child_node = new ReduceResultNode({ get() }, key_extr, reduce_fn);
        auto child = DIA<T>(child_node);
        get()->add_child(child_node);

        return child;
    }

    size_t Size() {
        return data_.size();
    }

    //! Allow direct data access. This is EVIL!
    //
    //! \return Interal data.
    const std::vector<T> & evil_get_data() const
    {
        return data_;
    }

    std::string NodeString() {
        return get()->ToString();
    }

    void PrintNodes () {
        using BasePair = std::pair<DIABase*, int>;
        std::stack<BasePair> dia_stack;
        dia_stack.push(std::make_pair(get(), 0));
        while (!dia_stack.empty()) {
            auto curr = dia_stack.top();
            auto node = curr.first;
            auto depth = curr.second;
            dia_stack.pop();
            auto is_end = true;
            if (!dia_stack.empty()) is_end = dia_stack.top().second < depth;
            for (int i = 0; i < depth - 1; ++i) {
                std::cout << "│   ";
            }

            if (is_end && depth > 0) std::cout << "└── ";
            else if (depth > 0) std::cout << "├── ";
            std::cout << node->ToString() << std::endl;
            auto children = node->get_childs();
            for (auto c : children) {
                dia_stack.push(std::make_pair(c, depth + 1));
            }
        }
    }

private:
    std::vector<T> data_;
};

} // namespace c7a

#endif // !C7A_API_DIA_HEADER

/******************************************************************************/