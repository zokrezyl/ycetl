
template <typename... Ts>
struct type_list {};

// This is what happens inside create_storage
template <typename TypeList>
struct storage_creator;



template <typename... Ts>
struct storage_creator<type_list<Ts...>> {
    using storage_type = dynamic_storage<Ts...>;
    
    constexpr static storage_type create() {
        return storage_type{};
    }
};

template <typename TypeList>
constexpr auto create_storage() {
    return storage_creator<TypeList>::create();
}


constexpr auto test_tl() {

  using tl = type_list<int, double>;

  auto storage = create_storage<tl>();

}

int main() {

}
